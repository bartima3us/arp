//
// Created by bartimaeus (sarunas.bartusevicius@gmail.com) on 20.5.14.
//

#ifndef ARP_ICMP_H
#define ARP_ICMP_H

#include <net/ethernet.h>
#include <netinet/ip.h>

struct ipv4_resp handle_icmp(char* mac, char* buffer, struct ether_header recv_ether_dgram, struct iphdr recv_ipv4_header);

#endif //ARP_ICMP_H
