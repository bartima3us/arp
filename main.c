#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <libexplain/ioctl.h> // $ sudo apt-get install libexplain-dev
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

// Unused for now
typedef struct {
    char dst[6];
    char src[6];
    u_int16_t type;
} ethernet;

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

char get_mac(int sd) {
    struct ifreq ifr;

    if (ioctl(sd, SIOCGIFHWADDR, &ifr) < 0) {
        printf("SIOCGIFHWADDR error: %s\n", strerror(errno));
    }

    printf("String: %s\n", ifr.ifr_hwaddr.sa_data);

    return *ifr.ifr_hwaddr.sa_data;
}

int main() {
    // malloc or calloc is used only forming array in a runtime (when we don't know a size in compile time)
    char if_name[IFNAMSIZ] = "tap0";
    char address[9] = "10.0.0.1";
    char subnet_mask[14] = "255.255.255.0";
    char buffer[1500], mac;
    int fd, sd, mtu, nread, response_size;
//    ethernet arp_packet;
    struct ether_header recv_ether_dgram, resp_ether_dgram;
    struct arphdr recv_arp_header, resp_arp_header;

    fd = tun_alloc(if_name);
    sd = tun_config(if_name, address, subnet_mask);
    mtu = get_mtu(sd);
    mac = get_mac(sd);
    close(sd);

    while (1) {
        nread = read(fd, buffer, mtu); // MTU = 1500
        recv_ether_dgram = *(struct ether_header*)buffer;
        printf("Read bytes: %d\n", nread);

        // htons(arp_packet.ether_type) - convert to network byte order
        if (htons(recv_ether_dgram.ether_type) == ETHERTYPE_ARP) {
            recv_arp_header = *(struct arphdr*)&buffer[sizeof(recv_ether_dgram)]; // @todo why needed &?
            if (htons(recv_arp_header.ar_op) == ARPOP_REQUEST) {
                printf("---------------\n");
                printf("ARP request\n");
                printf("Protocol type: %d\n", htons(recv_arp_header.ar_pro));

                // Create Ethernet part
                strcpy((char*)resp_ether_dgram.ether_dhost, (char*)recv_ether_dgram.ether_shost);
                strcpy((char*)resp_ether_dgram.ether_shost, &mac);
                resp_ether_dgram.ether_type = htons(ETHERTYPE_ARP);

                // Create ARP part
                resp_arp_header.ar_hrd = ARPHRD_ETHER;
                resp_arp_header.ar_pro = 0x0800;
                resp_arp_header.ar_hln = ETH_ALEN;
                resp_arp_header.ar_pln = 4;
                resp_arp_header.ar_op = ARPOP_REPLY;

                // Concat Ethernet headers with ARP
                response_size = sizeof(resp_ether_dgram);// + sizeof(resp_arp_header) + 1;
                char *response = (char*)malloc(response_size);
                strcpy(response, (char*)&resp_ether_dgram);
//                strcat(response, (char*)&resp_arp_header);

                // Successful: write(fd, &resp_ether_dgram, sizeof(resp_ether_dgram))
                write(fd, &resp_ether_dgram, sizeof(resp_ether_dgram));
//                if (write(fd, &response, response_size) < 0) {
//                    printf("Writing to descriptor failed\n");
//                } else {
//                    printf("Writing to descriptor successful\n");
//                }
//                printf("Dst: %d\n", arp_packet.ether_dhost);
//                printf("Src: %d\n", arp_packet.ether_shost);
//                printf("Pointer: %p\n", (void*)&buffer[14]);
                printf("---------------\n");

                free(response);
            }
        }

        if (nread == 5000) { // Impossible. Just to relax IDE...
            break;
        }
    }

    return 0;
}
