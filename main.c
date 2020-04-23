#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <fcntl.h>

// It is not possible in C to pass an array by value.
int tun_alloc(char *dev) {
    struct ifreq ifr;
    struct sockaddr ifaddr;
    int fd, err;
    char address[9] = "10.0.0.1";

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
    // @todo use: "$ ip addr add 10.0.0.1/24 dev tap0" because auto assign is not working
    strcpy(address, ifaddr.sa_data); // Array is not assignable because address = address[0]
    ifaddr.sa_family = AF_INET;
    ifr.ifr_addr = ifaddr;
    ifr.ifr_broadaddr = ifaddr;
    ifr.ifr_netmask = ifaddr;
    ifr.ifr_flags = IFF_TAP;
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

    printf("Tap created successfully\n");

    return fd;
}

int main() {
    // malloc or calloc is used only forming array in a runtime (when we don't know a size in compile time)
    char if_name[IFNAMSIZ] = "tap0";

    tun_alloc(if_name);

    while (1) {
        // freeze program
    }

    return 0;
}