#include "tcp_socket.h"

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

TCP_Socket_Result tcp_socket_create_and_start_connecting_to_ip(const IP_Address ip_address, const uint32_t timeout_s, TCP_Socket *out_socket) {
    (void)timeout_s; // TODO: SS - Use the timeout.
    
    // Create a socket.
    int socket_fd = socket(
        ip_address.is_ipv6 ? AF_INET6 : AF_INET,
        SOCK_STREAM,
        0
    );
    if(socket_fd == -1) {
        close(socket_fd);
        return TCP_Socket_Result_Failed_To_Create;
    }

    { // Set the socket to be non-blocking.
        int flags = fcntl(socket_fd, F_GETFL, 0);
        fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    }

    { // Enable TCP_NODELAY and more.        
        const int flag = 1;
        if (setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) < 0) {
            printf("Failed to set TCP_NODELAY.\n"); 
            close(socket_fd);
            return TCP_Socket_Result_Failed_To_Create;
        }

        const int enable = 1;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
            printf("Failed to set SO_REUSEADDR.\n"); 
            close(socket_fd);
            return TCP_Socket_Result_Failed_To_Create;
        }

        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)) < 0) {
            printf("Failed to set SO_REUSEPORT.\n"); 
            close(socket_fd);
            return TCP_Socket_Result_Failed_To_Create;
        }
    }

    struct sockaddr *server_address;
    socklen_t address_length;
    if (ip_address.is_ipv6) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)malloc(sizeof(struct sockaddr_in6));
        memset(addr6, 0, sizeof(struct sockaddr_in6));

        addr6->sin6_family = AF_INET6;
        memcpy(addr6->sin6_addr.s6_addr, ip_address.address.ipv6, 16);
        addr6->sin6_port = htons(80); // TEMP: SS - Port hardcoded to http.

        server_address = (struct sockaddr *)addr6;
        address_length = sizeof(struct sockaddr_in6);
    } else {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        memset(addr4, 0, sizeof(struct sockaddr_in));

        addr4->sin_family = AF_INET;
        memcpy(&addr4->sin_addr.s_addr, ip_address.address.ipv4, 4);
        addr4->sin_port = htons(80); // TEMP: SS - Port hardcoded to http.

        server_address = (struct sockaddr *)addr4;
        address_length = sizeof(struct sockaddr_in);
    }

    // Start connecting.
    int connect_result = connect(
        socket_fd,
        server_address,
        address_length
    );

    // Check connect_result.
    if(connect_result == -1) {
        if(errno != EINPROGRESS) {
            printf("Got error %i when trying to connect socket.\n", errno);
            close(socket_fd);
            return TCP_Socket_Result_Failed_To_Connect;
        }
    }

    free(server_address);

    out_socket->fd = socket_fd;
    
    return TCP_Socket_Result_OK;
}

static inline bool socket_writable(const TCP_Socket *socket) {
    struct pollfd fds;
    fds.fd = socket->fd;
    fds.events = POLLOUT;

    int poll_result = poll(&fds, 1, 0);

    if (poll_result == -1) {
        return false;
    }

    if (poll_result == 0) {
        return false;
    }

    if (fds.revents & POLLOUT) {
        return true;
    }

    assert(false);
    return false;
}

bool tcp_socket_connected(const TCP_Socket *socket) {
    int error = 0;
    socklen_t len = sizeof(error);

    if (getsockopt(socket->fd, SOL_SOCKET, SO_ERROR, &error, &len) == -1) {
        printf("Failed to getsockopt(..) on socket.\n");
        return false;
    }

    if (error != 0) {
        return false;
    }

    if(!socket_writable(socket)) {
        // printf("Not connected: Socket is not writable.\n");
        return false;
    }
    
    return true;
}

TCP_Socket_Result tcp_socket_close(TCP_Socket *socket) {
    close(socket->fd);
    socket->fd = 0;
    return TCP_Socket_Result_OK;
}

TCP_Socket_Result tcp_socket_send(const TCP_Socket *socket, const void *buf, const size_t buf_size, uint32_t *out_bytes_sent) {
    assert(buf != NULL);
    assert(buf_size > 0); // NOTE: SS - Might want to avoid crashing here.
    
    *out_bytes_sent = 0;

    if(!tcp_socket_connected(socket)) {
        return TCP_Socket_Result_Not_Connected;
    }
    
    if (!socket_writable(socket)) {
        return TCP_Socket_Result_Not_Ready_To_Be_Written_To;
    }

    int flags = 0; // NOTE: SS - Make this customizable?
    ssize_t bytes_sent = send(socket->fd, buf, buf_size, flags);
    if(bytes_sent == -1) {
        if(errno == EAGAIN) {
            // printf("EAGAIN\n");
            return TCP_Socket_Result_Not_Ready_To_Be_Written_To;
        }
        if(errno == EWOULDBLOCK) {
            // printf("EWOULDBLOCK\n");
            return TCP_Socket_Result_Not_Ready_To_Be_Written_To;
        }

        printf("Failed to send bytes over socket. Errno is %i.\n", errno);
        assert(false);
        return TCP_Socket_Result_Failed_To_Send;
    }

    *out_bytes_sent = bytes_sent;
    
    return TCP_Socket_Result_OK;
}

static inline bool socket_readable(const TCP_Socket *socket) {
    struct pollfd fds;
    fds.fd = socket->fd;
    fds.events = POLLIN;
    fds.revents = 0;
    errno = 0;
    
    
    // printf("Checking if socket %i is readable ...\n", socket->fd);
    
    int poll_result = poll(&fds, 1, 0);

    // printf("fds.revents: %i\n", fds.revents);

    if (poll_result == -1) {
        printf("Poll result -1: %s\n", strerror(errno));
        return false;
    }

    if (poll_result == 0) {
        // printf("Poll result 0.\n");
        return false;
    }

    if (fds.revents & POLLIN) {
        return true;
    }

    assert(false);
    return false;
}


TCP_Socket_Result tcp_socket_receive(const TCP_Socket *socket, void *buf, const size_t buf_size, uint32_t *out_bytes_received) {
    assert(buf != NULL);
    assert(buf_size > 0);
    
    *out_bytes_received = 0;

    if(!tcp_socket_connected(socket)) {
        return TCP_Socket_Result_Not_Connected;
    }
    
    if (!socket_readable(socket)) {
        return TCP_Socket_Result_Not_Ready_To_Be_Read;
    }

    int flags = 0; // NOTE: SS - Make this customizable?
    ssize_t bytes_received = recv(socket->fd, buf, buf_size, flags);
    if(bytes_received == -1) {
        if(errno == EAGAIN) {
            printf("EAGAIN\n");
            return TCP_Socket_Result_Not_Ready_To_Be_Read;
        }
        if(errno == EWOULDBLOCK) {
            printf("EWOULDBLOCK\n");
            return TCP_Socket_Result_Not_Ready_To_Be_Read;
        }

        printf("Failed to read bytes in socket. Errno is %i.\n", errno);
        assert(false);
        return TCP_Socket_Result_Failed_To_Read;
    }

    printf("Socket %i received %lu bytes.\n", socket->fd, bytes_received);

    *out_bytes_received = bytes_received;
    
    return TCP_Socket_Result_OK;
}
