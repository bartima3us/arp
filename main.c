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

int tun_alloc2(char *dev) {
    struct ifreq ifr;
    struct ifreq ifr2;
    struct ifreq ifr3;
//    struct sockaddr ifaddr;
    struct sockaddr ifaddr2;
    int fd, sd, err;
    char address[9] = "10.0.0.1";

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        printf("Error while opening tap device: %d\n", fd);
        return 0; // @todo change it
    }

    // Fill all ifr with 0
    memset(&ifr, 0, sizeof(ifr));
    memset(&ifr2, 0, sizeof(ifr2));
    memset(&ifr3, 0, sizeof(ifr2));

    /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
     *        IFF_TAP   - TAP device
     *
     *        IFF_NO_PI - Do not provide packet information
     */
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    // * is dereference operator. Ir returns value behind a pointer
    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
        strncpy(ifr2.ifr_name, dev, IFNAMSIZ);
    }

    if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
        printf("ioctl error: %d\n", err);
        close(fd);
        return err;
    }
    strcpy(dev, ifr.ifr_name);
//    close(fd);

    /* ////////////////////////////////////////////////////////////////////////////////////////////////////// */

    // https://www.linuxquestions.org/questions/linux-wireless-networking-41/what-is-siocsifflags-674992/
    // SIOCSIFFLAGS ?
    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("socket error: %d\n", sd);
        close(fd);
        return sd;
    }

    // @todo use:
    // $ sudo ip addr add 10.0.0.1/24 dev tap0
    // $ sudo ip link set dev tap0 up
    // because auto assign is not working

    strcpy(address, ifaddr2.sa_data); // Array is not assignable because address = address[0]
    ifaddr2.sa_family = AF_INET;
    ifr2.ifr_addr = ifaddr2;
//    ifr2.ifr_broadaddr = ifaddr2;
//    ifr2.ifr_netmask = ifaddr2;

    if ((err = ioctl(sd, SIOCSIFADDR, &ifr2)) < 0) {
        // 77 - File descriptor in bad state
        // 22 - EINVAL
        printf("ioctl error2: %d\n", err);
        printf("ioctl error2: %d\n", errno);
        close(sd);
        return err;
    }

    ifr3.ifr_flags = IFF_UP | IFF_RUNNING; // @todo why "$ /sbin/ifconfig tap0 up" is not working?
    if ((err = ioctl(sd, SIOCSIFFLAGS, &ifr3)) < 0) {
        // 19 - EMFILE Too many open files
        printf("ioctl error3: %d\n", err);
        printf("ioctl error3: %d\n", errno);
        close(sd);
        return err;
    }

    printf("Tap created successfully\n");

    return fd;
}

int main() {
    // malloc or calloc is used only forming array in a runtime (when we don't know a size in compile time)
    char if_name[IFNAMSIZ] = "tap0";
    char buffer[2000];
    int fd, nread;

    fd = tun_alloc(if_name);


    while (1) {
        nread = read(fd, buffer, 2000);
//        printf("Read bytes: %d\n", nread);
        // freeze program
    }

    return 0;
}