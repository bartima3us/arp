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

struct arppld
{
    unsigned char __ar_sha[ETH_ALEN];	/* Sender hardware address.  */
    unsigned char __ar_sip[4];		/* Sender IP address.  */
    unsigned char __ar_tha[ETH_ALEN];	/* Target hardware address.  */
    unsigned char __ar_tip[4];		/* Target IP address.  */
};

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

    /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
     *        IFF_TAP   - TAP device
     *
     *        IFF_NO_PI - Do not provide packet information
     */
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

int main() {
    // malloc or calloc is used only forming array in a runtime (when we don't know a size in compile time)
    char if_name[IFNAMSIZ] = "tap0";
    char address[9] = "10.0.0.1";
    char ip[9] = "10.0.0.2";
    char mac[7] = "abcdef";
    char subnet_mask[14] = "255.255.255.0";
    char buffer[1500];
    int fd, sd, mtu, nread;
    size_t response_size;
    struct ether_header recv_ether_dgram, resp_ether_dgram;
    struct arphdr recv_arp_header, resp_arp_header;
    struct arppld recv_arp_payload;
    unsigned char arp_hw_addr[ETH_ALEN + ETH_ALEN + 8];
    unsigned char ip_bin[sizeof(struct in_addr)];
    inet_pton(AF_INET, ip, ip_bin);

    fd = tun_alloc(if_name);
    sd = tun_config(if_name, address, subnet_mask);
    mtu = get_mtu(sd);
    char *if_mac = get_mac(sd);
    // @todo free(mac);
    close(sd);

    printf("IF MAC: %s\n", if_mac);

    while (1) {
        nread = read(fd, buffer, mtu); // MTU = 1500
        recv_ether_dgram = *(struct ether_header*)buffer;
        printf("Read bytes: %d\n", nread);

        // htons(arp_packet.ether_type) - convert to network byte order
        if (htons(recv_ether_dgram.ether_type) == ETHERTYPE_ARP) {
            recv_arp_header = *(struct arphdr*)&buffer[sizeof(recv_ether_dgram)]; // & needed because buffer = &buffer[0] and *buffer = buffer[0].
            if (htons(recv_arp_header.ar_op) == ARPOP_REQUEST) {
                recv_arp_payload = *(struct arppld*)&buffer[sizeof(recv_ether_dgram) + sizeof(recv_arp_header)];

                printf("---------------\n");
                printf("ARP request\n");
                printf("Protocol type: %d\n", htons(recv_arp_header.ar_pro));

                // Create Ethernet part
//                strcpy((char*)resp_ether_dgram.ether_dhost, (char*)recv_ether_dgram.ether_shost);
//                strcpy((char*)resp_ether_dgram.ether_shost, mac);
                memcpy(resp_ether_dgram.ether_dhost, recv_ether_dgram.ether_shost, ETH_ALEN);
                memcpy(resp_ether_dgram.ether_shost, mac, ETH_ALEN);
                resp_ether_dgram.ether_type = htons(ETHERTYPE_ARP);
                size_t eth_dgram_size = sizeof(resp_ether_dgram);

                // Create ARP fixed part
                resp_arp_header.ar_hrd = htons(ARPHRD_ETHER);
                resp_arp_header.ar_pro = htons(0x0800);
                resp_arp_header.ar_hln = ETH_ALEN;
                resp_arp_header.ar_pln = 4;
                resp_arp_header.ar_op = htons(ARPOP_REPLY);
                size_t arp_fixed_part_size = sizeof(resp_arp_header);

                // Create ARP dynamic part (wrong attempt)
                // This does not work because if strcpy or strcat finds "0" in a string (for example in IP), it stops because "0" is string end symbol. Must use memcpy to avoid.
//                strncpy((char*)arp_hw_addr, mac, sizeof(mac) - 1); // Sender MAC
//                strncat((char*)arp_hw_addr, ip_bin, 4); // Sender IP
//                strncat((char*)arp_hw_addr, recv_arp_payload.__ar_sha, sizeof(recv_arp_payload.__ar_sha)); // Target MAC
//                strncat((char*)arp_hw_addr, recv_arp_payload.__ar_sip, sizeof(recv_arp_payload.__ar_sip)); // Target IP

                // Create ARP dynamic part
                // Sender hardware address (ETH_ALEN) + Sender IP address (4) + Target hardware address (ETH_ALEN) + Target IP address (4) - NULL (1)
                memcpy(arp_hw_addr, mac, sizeof(mac));
                memcpy(arp_hw_addr + sizeof(mac) - 1, ip_bin, sizeof(ip_bin));
                memcpy(arp_hw_addr + sizeof(mac) + sizeof(ip_bin) - 1, recv_arp_payload.__ar_sha, sizeof(recv_arp_payload.__ar_sha)); // Target MAC
                memcpy(arp_hw_addr + sizeof(mac) + sizeof(ip_bin) + sizeof(recv_arp_payload.__ar_sha) - 1, recv_arp_payload.__ar_sip, sizeof(recv_arp_payload.__ar_sip)); // Target IP
                size_t arp_dynamic_part_size = sizeof(arp_hw_addr);

                // Concat Ethernet headers with ARP
                response_size = eth_dgram_size + arp_fixed_part_size + arp_dynamic_part_size;
                char response[response_size];
                memcpy(response, &resp_ether_dgram, eth_dgram_size);
                memcpy(response + eth_dgram_size, &resp_arp_header, arp_fixed_part_size);
                memcpy(response + eth_dgram_size + arp_fixed_part_size, &arp_hw_addr, arp_dynamic_part_size);

                // Successful: write(fd, &resp_ether_dgram, sizeof(resp_ether_dgram))
                if (write(fd, response, response_size) < 0) {
                    printf("Writing to descriptor failed\n");
                } else {
                    printf("Writing to descriptor successful\n");
                }

                printf("---------------\n");

//                free(response);
            }
        }

        if (nread == 5000) { // Impossible. Just to relax IDE...
            break;
        }
    }

    return 0;
}
