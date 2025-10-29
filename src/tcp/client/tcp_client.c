#include "tcp_client.h"

TCP_Client_Start_Connecting_Result tcp_client_connect(TCP_Client *client, IP_Address ip_address) {
    assert(client != NULL);
    assert(client->connection_state == TCP_Client_Connection_State_Disconnected);
    
    printf("Trying to connect to ip: ");
    ip_print(ip_address);

    // Create socket and start connecting to the server.
    TCP_Socket_Result create_socket_error = tcp_socket_create_and_start_connecting_to_ip(
        ip_address,
        10,
        &client->socket
    );
    if(create_socket_error != TCP_Socket_Result_OK) {
        printf("Failed to create a socket! Got error: %i.\n", create_socket_error);
        return TCP_Client_Start_Connecting_Result_Failed_To_Create_Socket;
    }

    client->connection_state = TCP_Client_Connection_State_Connecting;
    return TCP_Client_Start_Connecting_Result_Connecting;
}

void tcp_client_disconnect(TCP_Client *client) {
    assert(client != NULL);
    assert(client->connection_state == TCP_Client_Connection_State_Connected);
    
    client->connection_state = TCP_Client_Connection_State_Disconnecting;
}

void tcp_client_work(TCP_Client *client) {
    assert(client != NULL);
    
    switch(client->connection_state) {
        case TCP_Client_Connection_State_Disconnected: {
            // Do nothing.
            break;
        }
        case TCP_Client_Connection_State_Connecting: {
            if(tcp_socket_connected(&client->socket)) {
                client->connection_state = TCP_Client_Connection_State_Connected;
                break;
            }

            break;
        }
        case TCP_Client_Connection_State_Connected: {
            if(!tcp_socket_connected(&client->socket)) {
                client->connection_state = TCP_Client_Connection_State_Disconnecting;
                break;
            }
            break;
        }
        case TCP_Client_Connection_State_Disconnecting: {
            printf("CLOSING SOCKET!\n");
            assert(false);
            // Close socket.
            TCP_Socket_Result close_result = tcp_socket_close(&client->socket);
            assert(close_result == TCP_Socket_Result_OK);
            client->connection_state = TCP_Client_Connection_State_Disconnected;

            break;
        }
    }
}