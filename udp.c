//
// Created by bartimaeus (sarunas.bartusevicius@gmail.com) on 20.5.31.
//

#include <stdio.h>
#include <netinet/udp.h>
#include "udp.h"
#include "string.h"

void handle_udp(char* buffer, struct ether_header recv_ether_dgram, struct iphdr recv_ipv4_header)
{
    struct udphdr recv_udp_header;

    int ether_ip_size = sizeof(recv_ether_dgram) + sizeof(recv_ipv4_header);
    recv_udp_header = *(struct udphdr*)&buffer[ether_ip_size];
    int payload_size = htons(recv_udp_header.len) - sizeof(struct udphdr);
    char payload[payload_size];

    memcpy(payload, &buffer[ether_ip_size + sizeof(struct udphdr)], (size_t)payload_size);

    printf("UDP. Src port: %i, dst port: %i, pld: %s\n", htons(recv_udp_header.source), htons(recv_udp_header.dest), payload);
}
