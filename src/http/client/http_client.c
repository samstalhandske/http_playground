#include "http_client.h"
#include <string.h>
#include <assert.h>

static inline bool http_tcp_send_request_work(Worker_Context *context, const uint32_t lifetime) {
    (void)lifetime;
    HTTP_Client_Send_Request_Context *ctx = (HTTP_Client_Send_Request_Context *)context;

    // printf("Text:\n%s\n", ctx->text);

    printf("Worker: Sending bytes. Progress: %i/%i bytes.\n", ctx->amount_of_bytes_sent, ctx->amount_of_bytes_to_send);

    assert(ctx->amount_of_bytes_sent <= ctx->amount_of_bytes_to_send);
    uint32_t bytes_left_to_send = ctx->amount_of_bytes_to_send - ctx->amount_of_bytes_sent;

    if(bytes_left_to_send == 0) {
        printf("All bytes sent. :)\n");
        return true; // Task is done. All bytes have been sent. :)
    }

    uint32_t bytes_sent_this_time = 0;
    TCP_Socket_Result send_result = tcp_socket_send(
        &ctx->tcp_client->socket,
        &ctx->text[ctx->amount_of_bytes_sent],
        bytes_left_to_send,
        &bytes_sent_this_time
    );

    switch(send_result) {
        case TCP_Socket_Result_Not_Connected: {
            assert(false); // TEMP: SS - Handle this more gracefully.
            break;
        }
        case TCP_Socket_Result_OK: {
            break;
        }
        case TCP_Socket_Result_Not_Ready_To_Be_Written_To: {
            printf("Error: Socket %i is not ready.\n", ctx->tcp_client->socket.fd);
            break;
        }
        case TCP_Socket_Result_Failed_To_Send: {
            printf("Error: Failed to send.\n");
            break;
        }
        default: {
            printf("Unhandled case (%i) when sending data over the socket.\n", send_result);
            break;
        }
    }

    ctx->amount_of_bytes_sent += bytes_sent_this_time;

    return false; // Not done. There are still some bytes left to send.
}

static inline bool http_tcp_receive_response_work(Worker_Context *context, const uint32_t lifetime) {
    (void)lifetime;
    HTTP_Client_Receive_Response_Context *ctx = (HTTP_Client_Receive_Response_Context *)context;

    // printf("Worker: Reading bytes. Progress: %i/?? bytes.\n", ctx->amount_of_bytes_read);

    uint32_t bytes_read_this_time = 0;
    TCP_Socket_Result receive_result = tcp_socket_receive(
        &ctx->tcp_client->socket,
        ctx->buffer,
        HTTP_CLIENT_RESPONSE_BUFFER_INITIAL_SIZE,
        &bytes_read_this_time
    );

    // printf("Receive result: %i\n", receive_result);

    switch(receive_result) {
        case TCP_Socket_Result_Not_Connected: {
            assert(false); // TEMP: SS - Handle this more gracefully.
            break;
        }
        case TCP_Socket_Result_OK: {
            if(bytes_read_this_time == 0) { // Timeout or socket closed, probably.
                printf("Read 0 bytes. Work done.\n");
            }

            break;
        }
        case TCP_Socket_Result_Not_Ready_To_Be_Read: {
            // printf("Error: Socket %i is not ready to be read.\n", ctx->tcp_client->socket.fd);
            break;
        }
        case TCP_Socket_Result_Failed_To_Read: {
            printf("Error: Failed to read.\n");
            break;
        }
        default: {
            printf("Unhandled case (%i) when reading data from the socket.\n", receive_result);
            break;
        }
    }

    ctx->amount_of_bytes_read += bytes_read_this_time;

    if(bytes_read_this_time > 0) {
        string_buffer_append_buf(&ctx->sb, &ctx->buffer[0], bytes_read_this_time);

        memset(&ctx->buffer, 0, HTTP_CLIENT_RESPONSE_BUFFER_INITIAL_SIZE);

        // printf("Read %u bytes.\n", bytes_read_this_time);
        // printf("%s\n", ctx->sb.data);

        HTTP http;

        HTTP_Parse_Result result = http_try_parse(ctx->http_parser, &ctx->sb.data[0], ctx->sb.length, &http);
        switch(result) {
            case HTTP_Parse_Result_Done: {
                return true;
            }
            case HTTP_Parse_Result_Needs_More_Data: {
                return false;
            }
            case HTTP_Parse_Result_Invalid_Data:
            case HTTP_Parse_Result_TODO: {
                return true;
            }
        }
    }

    return false;
}


bool http_client_request_work(Worker_Context *context, const uint32_t lifetime) {
    (void)lifetime;
    HTTP_Client_Request_Context *ctx = (HTTP_Client_Request_Context *)context;

    tcp_client_work(&ctx->tcp_client);

    switch(ctx->state) {
        case HTTP_Client_Request_State_Resolving: {
            // Resolve hostname to an IP address.
            printf("'%s/%s': Resolving hostname ...\n", ctx->hostname, ctx->path);

            memset(&ctx->ip_address_candidates[0], 0, MAX_IP_ADDRESS_CANDIDATES);
            ctx->ip_address_candidates_found = 0;

            DNS_Resolve_Result resolve_result = dns_resolve_hostname(
                ctx->hostname,
                &ctx->ip_address_candidates[0],
                MAX_IP_ADDRESS_CANDIDATES,
                &ctx->ip_address_candidates_found
            );

            if(resolve_result != DNS_Resolve_Result_OK) {
                ctx->state = HTTP_Client_Request_State_Done;
            }
            else {
                assert(ctx->ip_address_candidates_found > 0);
                printf("Found %i addresses for hostname '%s':\n", ctx->ip_address_candidates_found, ctx->hostname);
                for(uint32_t i = 0; i < ctx->ip_address_candidates_found; i++) {
                    printf("- ");
                    ip_print(ctx->ip_address_candidates[i]);
                }

                printf("\n");

                // Okay. We now have atleast one ip-address candidate. Proceed to the 'Connect' state.
                ctx->state = HTTP_Client_Request_State_Connect;
            }

            break;
        }
        case HTTP_Client_Request_State_Connect: {
            // Now that we have some IP addresses, start the tcp-client and connect to one of them.

            bool connected_to_an_ip_address_successfully = false;
            for(uint32_t i = 0; i < ctx->ip_address_candidates_found; i++) {
                IP_Address *ip_to_connect_to = &ctx->ip_address_candidates[i];

                printf("'%s/%s': Start connecting to ip-adress: ", ctx->hostname, ctx->path);
                ip_print(*ip_to_connect_to);

                TCP_Client_Start_Connecting_Result start_connecting_result = tcp_client_connect(&ctx->tcp_client, *ip_to_connect_to);
                if(start_connecting_result != TCP_Client_Start_Connecting_Result_Connecting) {
                    printf("Failed. Got start-connecting-result: %i.\n", start_connecting_result);
                    continue;
                }

                connected_to_an_ip_address_successfully = true;
                break;
            }

            if(!connected_to_an_ip_address_successfully) {
                printf("Error: Failed to connect to any of the %i ip address candidates.\n", ctx->ip_address_candidates_found);
                ctx->state = HTTP_Client_Request_State_Done;
                break;
            }

            // Check the state of the TCP Client.
            ctx->state = HTTP_Client_Request_State_Connecting;
            break;
        }
        case HTTP_Client_Request_State_Connecting: {
            assert(
                ctx->tcp_client.connection_state == TCP_Client_Connection_State_Connecting ||
                ctx->tcp_client.connection_state == TCP_Client_Connection_State_Connected
            );

            // printf("'%s%s': TCP Client connecting ... \n", ctx->hostname, ctx->path);

            if(ctx->tcp_client.connection_state == TCP_Client_Connection_State_Connected) {
                printf("Connected to '%s'!\n", ctx->hostname);
                ctx->state = HTTP_Client_Request_State_Start_Sending_Request;
                break;
            }

            break;
        }
        case HTTP_Client_Request_State_Start_Sending_Request: {
            char request_string[512];
            memset(&request_string[0], 0, sizeof(request_string));

            switch(ctx->method) {
                case HTTP_Method_GET: {
                    snprintf(
                        &request_string[0],
                        sizeof(request_string),

                        "GET /%s HTTP/1.1\r\n"
                        "Host: %s\r\n"
                        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/140.0.0.0 Safari/537.36\r\n"
                        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7\r\n"
                        "\r\n" // Very important to signal that we're done with the headers.
                        ,

                        ctx->path,
                        ctx->hostname
                    );
                    break;
                }
                case HTTP_Method_POST: {
                    if(ctx->body == NULL) {
                        printf("Failed to POST. Body is NULL.\n");
                        ctx->state = HTTP_Client_Request_State_Done; // TEMP: SS - Go to disconnect or something instead.
                        break;
                    }

                    uint32_t body_text_length = strlen(ctx->body);
                    if(body_text_length == 0) {
                        printf("Failed to POST. Body's length is 0.\n");
                        ctx->state = HTTP_Client_Request_State_Done; // TEMP: SS - Go to disconnect or something instead.
                        break;
                    }

                    snprintf(
                        &request_string[0],
                        sizeof(request_string),

                        "POST /%s HTTP/1.1\r\n"
                        "Host: %s\r\n"
                        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/140.0.0.0 Safari/537.36\r\n"
                        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7\r\n"
                        "Content-Type: application/json\r\n" // TODO: SS - Make this customizable.
                        "Content-Length: %u\r\n"
                        "\r\n" // Very important to signal that we're done with the headers.
                        "%s"
                        ,

                        ctx->path,
                        ctx->hostname,
                        body_text_length,
                        ctx->body
                    );
                    break;
                }
                default: {
                    printf("Unhandled request method %i.\n", ctx->method);
                    assert(false);
                    break;
                }
            }

            assert(strlen(request_string) > 0);

#ifdef HTTP_CLIENT_DEBUG_PRINT_REQUEST_STRING
            printf("Request string:\n%s\n", request_string);
#endif

            HTTP_Client_Send_Request_Context message;
            memset(&message, 0, sizeof(HTTP_Client_Send_Request_Context));
            message.tcp_client = &ctx->tcp_client;
            message.text = request_string;
            message.amount_of_bytes_to_send = strlen(request_string);
            message.amount_of_bytes_sent = 0;

            bool ok = worker_add_task(
                &ctx->tcp_worker,
                &message,
                sizeof(HTTP_Client_Send_Request_Context),
                http_tcp_send_request_work
            );

            if(!ok) {
                ctx->state = HTTP_Client_Request_State_Done; // TEMP: SS - Go to disconnect or something instead.
                break;
            }

            ctx->state = HTTP_Client_Request_State_Sending_Request;

            break;
        }
        case HTTP_Client_Request_State_Sending_Request: {
            // printf("'%s%s': Sending request ...\n", ctx->hostname, ctx->path);

            uint32_t tasks_left = worker_work(&ctx->tcp_worker);
            if(tasks_left == 0) {
                // We've sent the entire request.
                printf("Let's wait for a response ...\n");

                ctx->state = HTTP_Client_Request_State_Waiting_For_Response;
                break;
            }

            break;
        }
        case HTTP_Client_Request_State_Waiting_For_Response: {
            HTTP_Client_Receive_Response_Context response;
            memset(&response, 0, sizeof(HTTP_Client_Receive_Response_Context));
            response.tcp_client = &ctx->tcp_client;
            memset(&response.buffer, 0, sizeof(response.buffer));

            string_buffer_init(&response.sb, HTTP_CLIENT_RESPONSE_BUFFER_INITIAL_SIZE);

            http_parser_init(&ctx->http_parser, HTTP_CLIENT_RESPONSE_BUFFER_INITIAL_SIZE);
            response.http_parser = &ctx->http_parser;

            bool ok = worker_add_task(
                &ctx->tcp_worker,
                &response,
                sizeof(HTTP_Client_Receive_Response_Context),
                http_tcp_receive_response_work
            );

            if(!ok) {
                string_buffer_free(&response.sb);
                ctx->state = HTTP_Client_Request_State_Done; // TEMP: SS - Go to disconnect or something instead.
                break;
            }

            ctx->state = HTTP_Client_Request_State_Receiving_Response;

            break;
        }
        case HTTP_Client_Request_State_Receiving_Response: {
            uint32_t tasks_left = worker_work(&ctx->tcp_worker);
            if(tasks_left == 0) {
                ctx->state = HTTP_Client_Request_State_Done;
                break;
            }

            break;
        }
        case HTTP_Client_Request_State_Done: {
            ctx->done_callback(
                ctx->hostname,
                ctx->path,
                &ctx->http_parser.http
            );

            http_dispose(&ctx->http_parser.http);
            tcp_client_disconnect(&ctx->tcp_client);
        }
    }

    switch(ctx->tcp_client.connection_state) {
        case TCP_Client_Connection_State_Disconnected:  return false;
        case TCP_Client_Connection_State_Connecting:    return false;
        case TCP_Client_Connection_State_Connected:     return false;
        case TCP_Client_Connection_State_Disconnecting: return true;
    }

    return false;
}

bool http_client_request(
    Worker *worker,
    HTTP_Method method,
    const char *hostname,
    const char *path,
    const char *body,
    HTTP_Client_Callback done_callback
) {
    HTTP_Client_Request_Context ctx;
    memset(&ctx, 0, sizeof(HTTP_Client_Request_Context));

    ctx.method = method;
    ctx.hostname = hostname;
    ctx.path = path;
    ctx.body = body;

    ctx.done_callback = done_callback;
    ctx.state = HTTP_Client_Request_State_Resolving;

    bool success_adding_task = worker_add_task(
        worker,
        &ctx,
        sizeof(HTTP_Client_Request_Context),
        http_client_request_work
    );

    if(!success_adding_task) {
        return false;
    }

    return true;
}