cmake_minimum_required(VERSION 3.10)
project(arp C)

set(CMAKE_C_STANDARD 99)

add_executable(arp main.c arp.c arp.h ipv4.c ipv4.h)