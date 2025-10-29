#ifndef DNS_H
#define DNS_H

#include <stdint.h>
#include <stdbool.h>
#include "ip/ip.h"

typedef enum {
    DNS_Resolve_Result_OK,
    DNS_Resolve_Result_Failed_To_Get_Address_Info,
    // ..
} DNS_Resolve_Result;

DNS_Resolve_Result dns_resolve_hostname(const char *hostname, IP_Address *out_ip_addresses, uint32_t ip_addresses_size, uint32_t *out_ip_address_count);

#endif