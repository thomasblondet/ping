#ifndef PING_H
#define PING_H

#include <arpa/inet.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define PACKET_SIZE 64
#define ICMP_HEADER_SIZE 8
#define PAYLOAD_SIZE 56

typedef struct {
    int fd;
    char hostname[NI_MAXHOST];
    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in addr;
    long packet_sent;
    long packet_received;
} Host;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
    uint8_t payload[PAYLOAD_SIZE];
} __attribute__((packed)) Packet;

#endif
