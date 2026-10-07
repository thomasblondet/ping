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
    int count;
    int ttl;
    bool verbose;
} Options;

typedef struct {
    double min;
    double max;
    double average;
    double stddev;
    double m2;
} RTT;

typedef struct {
    int fd;
    char hostname[NI_MAXHOST];
    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in addr;
    long packet_sent;
    long packet_received;
    RTT rtt;
} Host;

extern Options opts;

void build_packet(Host* h, uint8_t* buf);
void send_packet(Host* h);
void get_response(Host* h);
void fatal(char const* str);
[[maybe_unused]] void print_packet(const uint16_t* pkt, size_t size);

#endif
