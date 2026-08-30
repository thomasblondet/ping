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

#define PAYLOAD_SIZE 56
#define PACKET_SIZE (ICMP_MINLEN + PAYLOAD_SIZE)

typedef struct {
    int fd;
    char hostname[NI_MAXHOST];
    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in addr;
    long packet_sent;
    long packet_received;
} Host;

#endif
