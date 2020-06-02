//
// Created by bartimaeus (sarunas.bartusevicius@gmail.com) on 20.5.31.
//

#ifndef ARP_UDP_H
#define ARP_UDP_H

#include <net/ethernet.h>
#include <netinet/ip.h>

void handle_udp(char* buffer, struct ether_header recv_ether_dgram, struct iphdr recv_ipv4_header);

#endif //ARP_UDP_H
