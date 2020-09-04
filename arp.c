//
// Created by bartimaeus (sarunas.bartusevicius@gmail.com) on 20.5.8.
//

#include <stdio.h>
#include <string.h>
#include "arp.h"
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <netinet/in.h>

struct arp_resp handle_arp(char* mac, char* buffer, struct ether_header recv_ether_dgram_hdr)
{
    struct arphdr recv_arp_header;
    struct arppld recv_arp_payload;
    struct arp_resp arp_response;

    recv_arp_header = *(struct arphdr*)&buffer[sizeof(recv_ether_dgram_hdr)];

    if (htons(recv_arp_header.ar_op) == ARPOP_REQUEST) {
        recv_arp_payload = *(struct arppld*)&buffer[sizeof(recv_ether_dgram_hdr) + sizeof(recv_arp_header)];

        printf("---------------\n");
        printf("ARP request\n");
        printf("Protocol type: %d\n", htons(recv_arp_header.ar_pro));

        // Create Ethernet part
        memcpy(arp_response.eh.ether_dhost, recv_ether_dgram_hdr.ether_shost, sizeof(recv_ether_dgram_hdr.ether_shost));
        memcpy(arp_response.eh.ether_shost, mac, ETH_ALEN);
        arp_response.eh.ether_type = htons(ETHERTYPE_ARP);

        // Create ARP fixed part
        arp_response.ah.ar_hrd = htons(ARPHRD_ETHER);
        arp_response.ah.ar_pro = htons(0x0800);
        arp_response.ah.ar_hln = ETH_ALEN;
        arp_response.ah.ar_pln = 4;
        arp_response.ah.ar_op = htons(ARPOP_REPLY);

        // Create ARP dynamic part (wrong attempt)
        // This does not work because if strcpy or strcat finds "0" in a string (for example in IP), it stops because "0" is string end symbol. Must use memcpy to avoid.
//        strncpy((char*)arp_hw_addr, mac, sizeof(mac) - 1); // Sender MAC
//        strncat((char*)arp_hw_addr, ip_bin, 4); // Sender IP
//        strncat((char*)arp_hw_addr, recv_arp_payload.__ar_sha, sizeof(recv_arp_payload.__ar_sha)); // Target MAC
//        strncat((char*)arp_hw_addr, recv_arp_payload.__ar_sip, sizeof(recv_arp_payload.__ar_sip)); // Target IP

        // Create ARP dynamic part
        memcpy(arp_response.ap.__ar_sha, mac, sizeof(mac));
        memcpy(arp_response.ap.__ar_sip, recv_arp_payload.__ar_tip, sizeof(recv_arp_payload.__ar_tip));
        memcpy(arp_response.ap.__ar_tha, recv_arp_payload.__ar_sha, sizeof(recv_arp_payload.__ar_sha));
        memcpy(arp_response.ap.__ar_tip, recv_arp_payload.__ar_sip, sizeof(recv_arp_payload.__ar_sip));

        // The other way (to move pointer)
        // Sender hardware address (ETH_ALEN) + Sender IP address (4) + Target hardware address (ETH_ALEN) + Target IP address (4) - NULL (1)
//        memcpy(arp_hw_addr, mac, sizeof(mac));
//        memcpy(arp_hw_addr + sizeof(mac) - 1, recv_arp_payload.__ar_tip, sizeof(recv_arp_payload.__ar_tip)); // @todo maybe use recv_arp_payload.__ar_tip ?
//        memcpy(arp_hw_addr + sizeof(mac) + sizeof(recv_arp_payload.__ar_tip) - 1, recv_arp_payload.__ar_sha, sizeof(recv_arp_payload.__ar_sha)); // Target MAC
//        memcpy(arp_hw_addr + sizeof(mac) + sizeof(recv_arp_payload.__ar_tip) + sizeof(recv_arp_payload.__ar_sha) - 1, recv_arp_payload.__ar_sip, sizeof(recv_arp_payload.__ar_sip)); // Target IP

        return arp_response;
    }
}