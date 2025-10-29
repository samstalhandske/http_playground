#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include "ip/ip.h"

typedef struct {
    int fd;
} TCP_Socket;

typedef enum {
    TCP_Socket_Result_OK,
    TCP_Socket_Result_Failed_To_Create,
    TCP_Socket_Result_Failed_To_Connect,
    TCP_Socket_Result_Failed_To_Send,
    TCP_Socket_Result_Failed_To_Read,
    TCP_Socket_Result_Not_Ready_To_Be_Written_To,
    TCP_Socket_Result_Not_Ready_To_Be_Read,
    TCP_Socket_Result_Not_Connected,

} TCP_Socket_Result;

TCP_Socket_Result tcp_socket_create_and_start_connecting_to_ip(const IP_Address ip_address, const uint32_t timeout_s, TCP_Socket *out_socket);
bool tcp_socket_connected(const TCP_Socket *socket);

TCP_Socket_Result tcp_socket_close(TCP_Socket *socket);

TCP_Socket_Result tcp_socket_send(const TCP_Socket *socket, const void *buf, const size_t buf_size, uint32_t *out_bytes_sent);
TCP_Socket_Result tcp_socket_receive(const TCP_Socket *socket, void *buf, const size_t buf_size, uint32_t *out_bytes_received);

#endif