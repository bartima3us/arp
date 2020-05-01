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

typedef struct {
    unsigned long   destination : 48;
    short pad0;
    unsigned long   source      : 48;
    short pad1;
//    short  type;
//    char payload[28];
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
    close(sd);

    return 0;
}

int main() {
    // malloc or calloc is used only forming array in a runtime (when we don't know a size in compile time)
    char if_name[IFNAMSIZ] = "tap0";
    char address[9] = "10.0.0.1";
    char subnet_mask[14] = "255.255.255.0";
    char buffer[1500];
    int fd, nread;
    ethernet arp_packet;

    fd = tun_alloc(if_name);
    tun_config(if_name, address, subnet_mask);

    while (1) {
        nread = read(fd, buffer, 1500); // MTU = 1500
        printf("Read bytes: %d\n", nread);

        if (nread == 42) {
            printf("---------------\n");
            arp_packet = *(ethernet*)buffer;
            printf("Dst: %ld\n", arp_packet.destination);
            printf("Src: %ld\n", arp_packet.source);
//            printf("Type: %d\n", arp_packet.type);
            printf("---------------\n");
        }
    }
    return 0;
}
