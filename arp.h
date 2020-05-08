//
// Created by bartimaeus (sarunas.bartusevicius@gmail.com) on 20.5.8.
//

#ifndef ARP_ARP_H
#define ARP_ARP_H

#include <net/ethernet.h>
#include <net/if_arp.h>

struct arppld
{
    unsigned char __ar_sha[ETH_ALEN];	/* Sender hardware address.  */
    unsigned char __ar_sip[4];		/* Sender IP address.  */
    unsigned char __ar_tha[ETH_ALEN];	/* Target hardware address.  */
    unsigned char __ar_tip[4];		/* Target IP address.  */
};

// Protect from adding padding
__attribute__((__packed__)) struct arp_resp
{
    // ETH header
    struct ether_header eh;
    // ARP header
    struct arphdr ah;
    // ARP payload
    struct arppld ap;
};

struct arp_resp handle_arp(char* mac, char* buffer, struct ether_header recv_ether_dgram);

#endif //ARP_ARP_H
