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

int create_if(char *dev) { // erl way
    int sd;
    int opt = 1;
    struct sockaddr_un my_addr;
    struct ifreq ifr;

    // ioctl error numbers: https://www.linuxquestions.org/questions/linux-newbie-8/ioctl-errno-numbers-427930/
    if ((sd = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0) {
        printf("socket error: %d\n", sd);
        close(sd);
        return sd;
    }

//    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
//        printf("setsockopt");
//        close(sd);
//        return sd;
//    }

    /* Clear structure */
    memset(&my_addr, 0, sizeof(struct sockaddr_un));
    /* Clear structure */
    my_addr.sun_family = PF_LOCAL;
    strncpy(my_addr.sun_path, "sock.socket", sizeof(my_addr.sun_path) - 1); // If file extension is not specified, bind will return 98

    if (bind(sd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_un)) < 0) {
        printf("bind error: %d\n", errno);
        close(sd);
        return sd;
    }

    if (listen(sd, 5) < 0) {
        printf("listen error: %d\n", errno);
        close(sd);
        return sd;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    // * is dereference operator. Ir returns value behind a pointer
    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if (ioctl(sd, TUNSETIFF, (void *) &ifr) < 0) {
        printf("ioctl error: %d\n", errno);
        close(sd);
        return sd;
    }
    strcpy(dev, ifr.ifr_name);

    printf("TAP created\n");

    return sd;
}

int create_sock(char *dev) {
    int sd;
    struct ifreq ifr;
    char address[9] = "10.0.0.1";
    struct sockaddr ifaddr;

    if ((sd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("socket error: %d\n", sd);
        close(sd);
        return sd;
    }

    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    ifr.ifr_flags = (IFF_UP | IFF_RUNNING);
    if (ioctl(sd, SIOCSIFFLAGS, &ifr) < 0) {
        printf("ioctl error2: %s\n", strerror(errno));
        return -1;
    }

    strcpy(address, ifaddr.sa_data); // Array is not assignable because address = address[0]
    ifaddr.sa_family = AF_INET;
    ifr.ifr_addr = ifaddr;
    ifr.ifr_dstaddr = ifaddr;
    ifr.ifr_netmask = ifaddr;
    if (ioctl(sd, SIOCSIFADDR, &ifr) < 0) {
        printf("ioctl error4: %s\n", strerror(errno));
        return -1;
    }

    printf("Sock created\n");

    close(sd);
}

int main() {
    // malloc or calloc is used only forming array in a runtime (when we don't know a size in compile time)
    char if_name[IFNAMSIZ] = "tap0";
    char buffer[2000];
    int fd, nread;

    fd = tun_alloc(if_name);
    create_sock(if_name);

    while (1) {
        nread = read(fd, buffer, 2000);
        printf("Read bytes: %d\n", nread);
        // freeze program
    }
    return 0;
}