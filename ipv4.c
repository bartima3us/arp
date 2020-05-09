//
// Created by bartimaeus (sarunas.bartusevicius@gmail.com) on 20.5.8.
//

#include <stdio.h>
#include <linux/ip.h>
#include <linux/icmp.h>
#include <string.h>
#include <netinet/in.h>
#include "ipv4.h"

#if BYTE_ORDER == LITTLE_ENDIAN
# define ODDBYTE(v)	(v)
#elif BYTE_ORDER == BIG_ENDIAN
# define ODDBYTE(v)	((unsigned short)(v) << 8)
#else
# define ODDBYTE(v)	htons((unsigned short)(v) << 8)
#endif

// Taken from ping.c
unsigned short in_cksum(const unsigned short *addr, register int len, unsigned short csum)
{
    register int nleft = len;
    const unsigned short *w = addr;
    register unsigned short answer;
    register int sum = csum;

    /*
     *  Our algorithm is simple, using a 32 bit accumulator (sum),
     *  we add sequential 16 bit words to it, and at the end, fold
     *  back all the carry bits from the top 16 bits into the lower
     *  16 bits.
     */
    while (nleft > 1)  {
        sum += *w++;
        nleft -= 2;
    }

    /* mop up an odd byte, if necessary */
    if (nleft == 1)
        sum += ODDBYTE(*(unsigned char *)w); /* le16toh() may be unavailable on old systems */

    /*
     * add back carry outs from top 16 bits to low 16 bits
     */
    sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
    sum += (sum >> 16);			/* add carry */
    answer = ~sum;				/* truncate to 16 bits */
    return (answer);
}

struct ipv4_resp handle_ipv4(char* mac, char* buffer, struct ether_header recv_ether_dgram)
{
    struct iphdr recv_ipv4_header;
    struct icmphdr recv_icmp_header;
    struct ipv4_resp ipv4_response;
    struct icmphdr2 icmp_response;
    char icmp_payload[48] = "qertyuiopasdfghjklzxcvbnm0123456789zxcvbnmasdfh";

    recv_ipv4_header = *(struct iphdr*)&buffer[sizeof(recv_ether_dgram)];

    // ICMP
    if (recv_ipv4_header.protocol == 0x01) {
        recv_icmp_header = *(struct icmphdr*)&buffer[sizeof(recv_ether_dgram) + sizeof(recv_ipv4_header)];

        if (recv_icmp_header.type == ICMP_ECHO) {
            printf("---------------\n");
            printf("ICMP request \n");

            // Create Ethernet part
            memcpy(ipv4_response.eh.ether_dhost, recv_ether_dgram.ether_shost, sizeof(recv_ether_dgram.ether_shost));
            memcpy(ipv4_response.eh.ether_shost, mac, ETH_ALEN);
            ipv4_response.eh.ether_type = htons(ETHERTYPE_IP);

            // Create IP part
            ipv4_response.ih.version = 0x45; // IPv4
            ipv4_response.ih.tos = 0x00;
            ipv4_response.ih.id = 0x0000;
            ipv4_response.ih.frag_off = 0x0000;
            ipv4_response.ih.ttl = 0x39;
            ipv4_response.ih.protocol = 0x01; // ICMP
            ipv4_response.ih.check = htons(0xfcef); // Header validation disabled
            ipv4_response.ih.saddr = recv_ipv4_header.daddr;
            ipv4_response.ih.daddr = recv_ipv4_header.saddr;

            icmp_response.type = ICMP_ECHOREPLY;
            icmp_response.code = 0;
            icmp_response.checksum = 0x0000;
            icmp_response.id = recv_icmp_header.un.echo.id;
            icmp_response.sequence = recv_icmp_header.un.echo.sequence;
            memcpy(icmp_response.payload, icmp_payload, sizeof(icmp_payload));
            icmp_response.checksum = in_cksum((unsigned short *)&icmp_response, sizeof(icmp_response), 0);

            // Set IPv4 packet size
            ipv4_response.ih.tot_len = htons(sizeof(ipv4_response.ih) + sizeof(icmp_response));

            // Set IPv4 payload ant real packet length
            memcpy(ipv4_response.payload, &icmp_response, sizeof(icmp_response));
            ipv4_response.length = sizeof(ipv4_response.eh) + sizeof(ipv4_response.ih) + sizeof(icmp_response);

            return ipv4_response;
        }
    }
}