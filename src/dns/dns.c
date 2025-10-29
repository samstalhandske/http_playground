#include "dns.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

#if defined(LINUX)
#include <netdb.h>
#include <arpa/inet.h>

#elif defined(WINDOWS)

#elif defined(MACOS)

#endif

DNS_Resolve_Result dns_resolve_hostname(const char *hostname, IP_Address *out_ip_addresses, uint32_t ip_addresses_size, uint32_t *out_ip_address_count) {
    // printf("Resolving hostname '%s' ... \n", hostname);

    *out_ip_address_count = 0;

#if defined(LINUX)
    struct addrinfo hints;
    struct addrinfo *result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(hostname, NULL, &hints, &result);
    if (status != 0) {
        printf("Failed to get addrinfo: %s.\n", gai_strerror(status));
        return DNS_Resolve_Result_Failed_To_Get_Address_Info;
    }

    for (struct addrinfo *p = result; p != NULL; p = p->ai_next) {
        IP_Address ip;
        memset(&ip, 0, sizeof(IP_Address));

        if (p->ai_family == AF_INET) {
            ip.is_ipv6 = false;

            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            uint32_t addr = ipv4->sin_addr.s_addr;

            ip.address.ipv4[0] = (addr >>  0) & 0xFF;
            ip.address.ipv4[1] = (addr >>  8) & 0xFF;
            ip.address.ipv4[2] = (addr >> 16) & 0xFF;
            ip.address.ipv4[3] = (addr >> 24) & 0xFF;
        }
        else {
            ip.is_ipv6 = true;

            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;

            memcpy(ip.address.ipv6, ipv6->sin6_addr.s6_addr, 16);
        }

        memcpy(&out_ip_addresses[*out_ip_address_count], &ip, sizeof(IP_Address));
        *out_ip_address_count += 1;

        if(*out_ip_address_count > ip_addresses_size) {
            break;
        }
    }

#elif defined(WINDOWS)

#elif defined(MACOS)
    assert(false); // TODO!
#endif
    
    assert(*out_ip_address_count > 0);
    return DNS_Resolve_Result_OK;
}