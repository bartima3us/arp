//
// Created by sarunas on 20.5.8.
//

#ifndef ARP_IPV4_H
#define ARP_IPV4_H

#include <net/ethernet.h>
#include <linux/ip.h>

struct iphdr2 {
    __u8	version;
    __u8	tos;
    __be16	tot_len;
    __be16	id;
    __be16	frag_off;
    __u8	ttl;
    __u8	protocol;
    __sum16	check;
    __be32	saddr;
    __be32	daddr;
    /*The options start here. */
};

struct icmphdr2 {
    __u8	type;
    __u8	code;
    __sum16	checksum;
    __be16	id;
    __be16	sequence;
    __be64  ts; // Timestamp
    char    payload[48];
};

struct ipv4_resp
{
    // ETH header
    struct ether_header eh;
    // IPv4 header
    struct iphdr2 ih;
    // Max possible payload
    char payload[1466]; // 1500 (MTU) - 14 (Ether hdr) - 20 (IP hdr)
    // Real packet length
    size_t length;
} __attribute__ ((__packed__));

struct ipv4_resp handle_ipv4(char* mac, char* buffer, struct ether_header recv_ether_dgram);

#endif //ARP_IPV4_H
