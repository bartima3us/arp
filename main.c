#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <fcntl.h>

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
    ifr.ifr_flags = IFF_TAP;
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
    char if_name[IFNAMSIZ] = "tap0";

    tun_alloc(if_name);

    while (1) {
        // freeze program
    }

    return 0;
}