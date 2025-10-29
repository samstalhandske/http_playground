#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "ip/ip.h"
#include "worker/worker.h"
#include "tcp/tcp_socket.h"

typedef enum {
    TCP_Client_Connection_State_Disconnected,
    TCP_Client_Connection_State_Connecting,
    TCP_Client_Connection_State_Connected,
    TCP_Client_Connection_State_Disconnecting,
} TCP_Client_Connection_State;

typedef struct {
    TCP_Client_Connection_State connection_state;
    TCP_Socket socket;
} TCP_Client;

typedef enum {
    TCP_Client_Start_Connecting_Result_Connecting,
    TCP_Client_Start_Connecting_Result_Failed_To_Create_Socket,
} TCP_Client_Start_Connecting_Result;

TCP_Client_Start_Connecting_Result tcp_client_connect(TCP_Client *client, IP_Address ip_address);
void tcp_client_disconnect(TCP_Client *client);

void tcp_client_work(TCP_Client *client);

#endif