#ifndef IP_H
#define IP_H

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <arpa/inet.h>

typedef union {
    uint8_t ipv4[4];
    uint8_t ipv6[16];
} IP_Address_Union;

typedef struct {
    bool is_ipv6;
    IP_Address_Union address;
} IP_Address;


static inline void print_ipv4_address(const uint8_t ipv4[4]) {
    char ip_str[INET_ADDRSTRLEN];
    
    assert(inet_ntop(AF_INET, ipv4, ip_str, sizeof(ip_str)) != NULL);
    printf("%s\n", ip_str); // TODO: SS - Print port?
}

static inline void print_ipv6_address(const uint8_t ipv6[16]) {
    char ip_str[INET6_ADDRSTRLEN];
    
    assert(inet_ntop(AF_INET6, ipv6, ip_str, sizeof(ip_str)) != NULL);
    printf("%s\n", ip_str); // TODO: SS - Print port?
}

static inline void ip_print(const IP_Address ip) {
    if(ip.is_ipv6) {
        print_ipv6_address(ip.address.ipv6);
    }
    else {
        print_ipv4_address(ip.address.ipv4);
    }
}



#endif