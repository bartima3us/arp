//
// Created by bartimaeus (sarunas.bartusevicius@gmail.com) on 20.4.21.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if_arp.h>
#include "arp.h"
#include "ipv4.h"
#include "icmp.h"
#include "udp.h"

// It is not possible in C to pass an array by value.
int tun_alloc(char *dev) {
    struct ifreq ifr;
    int fd, err;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        printf("Error while opening tap device: %d\n", fd);
        return 0; // @todo change it
    }

    // Fill all ifr with 0
    memset(&ifr, 0, sizeof(ifr));

    // IFF_TUN - TUN device (no Ethernet headers)
    // IFF_TAP - TAP device
    // IFF_NO_PI - Do not provide packet information
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    // * is dereference operator. Ir returns value behind a pointer
    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
        printf("ioctl error: %d\n", err);
        close(fd);
        return err;
    }
    strcpy(dev, ifr.ifr_name);

    printf("Interface created.\n");

    return fd;
}

int tun_config(char *dev, char *address, char *subnet_mask) {
    int sd;
    struct ifreq ifr;
    struct sockaddr_in* addr = (struct sockaddr_in*)&ifr.ifr_addr;
    struct sockaddr_in* broadaddr = (struct sockaddr_in*)&ifr.ifr_broadaddr;

    if ((sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0) {
        printf("socket error: %d\n", sd);
        close(sd);
        return -1;
    }

    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    ifr.ifr_addr.sa_family = AF_INET;
    inet_pton(AF_INET, address, &addr->sin_addr);
    if (ioctl(sd, SIOCSIFADDR, &ifr) < 0) {
        printf("SIOCSIFADDR error: %s\n", strerror(errno));
        return -1;
    }

    inet_pton(AF_INET, subnet_mask, &broadaddr->sin_addr);
    if (ioctl(sd, SIOCSIFNETMASK, &ifr) < 0) {
        printf("SIOCSIFNETMASK error: %s\n", strerror(errno));
        return -1;
    }

    ifr.ifr_flags = (IFF_UP | IFF_RUNNING);
    if (ioctl(sd, SIOCSIFFLAGS, &ifr) < 0) {
        printf("SIOCSIFFLAGS error: %s\n", strerror(errno));
        return -1;
    }

    printf("Interface configured.\n");

    return sd;
}

int get_mtu(int sd) {
    struct ifreq ifr;

    if (ioctl(sd, SIOCGIFMTU, &ifr) < 0) {
        printf("SIOCGIFMTU error: %s\n", strerror(errno));
        return -1;
    }

    return ifr.ifr_mtu;
}

char* get_mac(int sd) {
    struct ifreq ifr;

    if (ioctl(sd, SIOCGIFHWADDR, &ifr) < 0) {
        printf("SIOCGIFHWADDR error: %s\n", strerror(errno));
    }

    char* mac = malloc(sizeof(ifr.ifr_hwaddr.sa_data));
    strcpy(mac, ifr.ifr_hwaddr.sa_data);

    return mac;
}

__u8 find_fragment(struct fragment* fragments_ptr, struct fragment curr_fragment, int fragments_counter)
{
    for (int i = 0; i < fragments_counter; i++) {
        if (fragments_ptr[i].protocol == curr_fragment.protocol
            && fragments_ptr[i].saddr == curr_fragment.saddr
            && fragments_ptr[i].daddr == curr_fragment.daddr
            && fragments_ptr[i].id == curr_fragment.id
        ) {
            return fragments_ptr[i].bitmap_ptr;
        }
    }

    return 0;
}

int main() {
    // malloc or calloc is used only forming array in a runtime (when we don't know a size in compile time)
    char if_name[IFNAMSIZ] = "tap0";
    char address[9] = "10.0.0.1";
    /*
     * https://en.wikipedia.org/wiki/MAC_address#Universal_vs._local
     * Universally administered and locally administered addresses are distinguished by setting the second-least-significant bit of the first octet of the address.
     * If the bit is 0, the address is universally administered.
     * If it is 1, the address is locally administered.
    */
    char mac[ETH_ALEN + 1] = "\x02\x00\x00\x00\x00\x01";
    char subnet_mask[14] = "255.255.255.0";
    char buffer[1500];
    int fd, sd, mtu, nread;
    int frag_off, dont_fragment, more_fragments, offset; // Flags
    __be16 id;
    ssize_t send_res = -1;
    struct ether_header recv_ether_dgram_hdr;
    struct arp_resp arp_response;
    struct ipv4_resp ipv4_response;
    struct iphdr recv_ipv4_header;
    struct fragment *fragments_ptr = NULL;
    struct fragment curr_fragment;
    int fragments_counter = 0;

    fd = tun_alloc(if_name);
    sd = tun_config(if_name, address, subnet_mask);
    mtu = get_mtu(sd);
    char *if_mac = get_mac(sd);
    // @todo free(mac);
    close(sd);

    printf("IF MAC: %s\n", if_mac);

    while (1) {
        nread = read(fd, buffer, mtu); // MTU = 1500
        recv_ether_dgram_hdr = *(struct ether_header*)buffer;
        printf("Read bytes: %d\n", nread);

        if (nread == 188) {
            continue;
        }

        // ARP
        if (htons(recv_ether_dgram_hdr.ether_type) == ETHERTYPE_ARP) { // htons() - convert to network byte order
            arp_response = handle_arp(mac, buffer, recv_ether_dgram_hdr);
            send_res = write(fd, &arp_response, sizeof(arp_response));
        // IPv4
        } else if (htons(recv_ether_dgram_hdr.ether_type) == ETHERTYPE_IP) {
            recv_ipv4_header = *(struct iphdr*)&buffer[sizeof(recv_ether_dgram_hdr)];

            frag_off = htons(recv_ipv4_header.frag_off);
            more_fragments = (frag_off >> 13) & 1;
            dont_fragment = (frag_off >> 14) & 1;
            offset = frag_off & 0b0001111111111111; // @todo make prettier
            id = htons(recv_ipv4_header.id);

            // @todo if time ends, send ICMP time exceeded
            // Reassembling
            if (offset != 0 || more_fragments != 0) {
                __u8 searching_fragment;
                curr_fragment.saddr = recv_ipv4_header.saddr;
                curr_fragment.daddr = recv_ipv4_header.daddr;
                curr_fragment.id = id;
                curr_fragment.protocol = recv_ipv4_header.protocol;
                curr_fragment.bitmap_ptr = 123;

                if (!fragments_ptr) {
                    printf("Initiated fragment pointer!\n");
                    fragments_counter++;
                    fragments_ptr = malloc(sizeof(struct fragment));
                    memcpy(fragments_ptr, &curr_fragment, sizeof(curr_fragment));
                } else if ((searching_fragment = find_fragment(fragments_ptr, curr_fragment, fragments_counter))) {
                    printf("FRAGMENT FOUND: %d\n", searching_fragment);
                }
            }

            printf("dont_fragment: %d\n", dont_fragment);
            printf("more_fragments: %d\n", more_fragments);
            printf("offset: %d\n", offset);

            // ICMP
            if (recv_ipv4_header.protocol == 0x01) {
                ipv4_response = handle_icmp(mac, buffer, recv_ether_dgram_hdr, recv_ipv4_header);
                char resp[ipv4_response.length];
                memcpy(resp, &ipv4_response, ipv4_response.length);
                send_res = write(fd, &resp, sizeof(resp));
            // UDP
            } else if (recv_ipv4_header.protocol == 0x11) { // 0x11 = 17
                handle_udp(buffer, recv_ether_dgram_hdr, recv_ipv4_header);
            }
        }

        if (send_res < 0) {
            printf("Writing to descriptor failed\n");
        } else {
            printf("Writing to descriptor successful\n");
        }

        printf("---------------\n");

        if (nread == 5000) { // Impossible. Just to relax IDE...
            break;
        }
    }

    return 0;
}
