Network stack implementation
=====

Test subnet mask (interface IP belongs to the host):
```
$ ip route get 10.0.0.2
```

Test ARP:
```
$ arping -I tap0 10.0.0.2
```

Test ICMP (currently not working with standard ping implementation possibly do to some incompatibilities between sockets)
```
$ ping 10.0.0.2
```

Test UDP
```
$ echo "Some payload" > /dev/udp/10.0.0.2/3000
```