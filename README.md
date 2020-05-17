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