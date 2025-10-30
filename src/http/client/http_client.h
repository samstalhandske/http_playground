#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "worker/worker.h"
#include "dns/dns.h"
#include "ip/ip.h"
#include "tcp/client/tcp_client.h"
#include "string/buffer/string_buffer.h"
#include "http/http.h"

typedef uint16_t HTTP_Client_Status_Code;

typedef void (*HTTP_Client_Callback)(const char *hostname, const char *path, HTTP *http); // TODO: SS - Add 'tcp error'?

typedef enum {
    HTTP_Client_Request_State_Resolving,
    HTTP_Client_Request_State_Connect,
    HTTP_Client_Request_State_Connecting,
    HTTP_Client_Request_State_Start_Sending_Request,
    HTTP_Client_Request_State_Sending_Request,
    HTTP_Client_Request_State_Waiting_For_Response,
    HTTP_Client_Request_State_Receiving_Response,
    HTTP_Client_Request_State_Done
} HTTP_Client_Request_State;

typedef enum {
    HTTP_Request_Method_GET,
    HTTP_Request_Method_POST
    // ..
} HTTP_Request_Method;

#ifndef MAX_IP_ADDRESS_CANDIDATES
#define MAX_IP_ADDRESS_CANDIDATES 16
#endif

#ifndef HTTP_CLIENT_RESPONSE_BUFFER_INITIAL_SIZE
#define HTTP_CLIENT_RESPONSE_BUFFER_INITIAL_SIZE 1024
#endif

typedef struct {
    TCP_Client *tcp_client;

    char buffer[HTTP_CLIENT_RESPONSE_BUFFER_INITIAL_SIZE];
    String_Buffer sb;

    uint32_t amount_of_bytes_read;

    HTTP_Parser *http_parser;
} HTTP_Client_Receive_Response_Context;

typedef struct {
    HTTP_Request_Method method;
    const char *hostname;
    const char *path;
    const char *body;

    HTTP_Client_Request_State state;

    HTTP_Client_Callback done_callback;

    IP_Address ip_address_candidates[MAX_IP_ADDRESS_CANDIDATES];
    uint32_t ip_address_candidates_found;

    TCP_Client tcp_client;
    Worker tcp_worker;

    HTTP_Client_Status_Code current_status_code;

    HTTP_Parser http_parser;
} HTTP_Client_Request_Context;

typedef struct {
    TCP_Client *tcp_client;
    const char *text;
    uint32_t amount_of_bytes_to_send; // TODO: SS - Change to uint64_t? Nah, probably alright.
    uint32_t amount_of_bytes_sent;
} HTTP_Client_Send_Request_Context;



bool http_client_request(
    Worker *worker,

    HTTP_Request_Method method,
    const char *hostname,
    const char *path,
    const char *body,

    HTTP_Client_Callback done_callback
);


#endif