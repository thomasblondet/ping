#include <stddef.h>

#include "ping.h"

static int g_sig;
static int g_count;

static void signal_handler(const int sig) {
    g_sig = 1;
    (void)sig;
}

static void fatal(const char *str) {
    fprintf(stderr, "ft_ping: ");
    perror(str);
    exit(1);
}

static void hostname_resolution(Host *h) {
    const struct addrinfo hints = {
        .ai_flags = 0,
        .ai_family = AF_INET,
        .ai_socktype = 0,
        .ai_protocol = 0,
        .ai_addrlen = 0,
        .ai_addr = NULL,
    };

    struct addrinfo *res = NULL;
    const int ret = getaddrinfo(h->hostname, NULL, &hints, &res);
    if (ret != 0) {
        printf("ft_ping: cannot resolve %s: %s\n", h->hostname, gai_strerror(ret));
        exit(1);
    }

    const struct sockaddr_in *sin = (const struct sockaddr_in *)res->ai_addr;
    inet_ntop(AF_INET, &sin->sin_addr, h->ip, INET_ADDRSTRLEN);
    h->addr.sin_family = AF_INET;
    memcpy(&h->addr.sin_addr, &sin->sin_addr, sizeof(sin->sin_addr));

    freeaddrinfo(res);
}

static uint16_t calculate_checksum(const uint8_t *pkt, const size_t size) {
    uint32_t sum = 0;
    size_t i;

    for (i = 0; i < size; i += 2) {
        const uint16_t word = pkt[i] << 8 | pkt[i + 1];
        sum += word;
    }

    // the size of the packet can be odd
    if (size % 2 != 0) {
        sum += pkt[i] << 8;
    }

    while (sum >> 16) {
        const uint32_t carry = sum >> 16;
        sum = sum & 0xffff;
        sum += carry;
    }
    return (uint16_t)~sum;
}

// FOR DEBUG
// static void print_packet(const uint16_t *pkt, const size_t size) {
//     for (size_t i = 0; i < size / 2; i++) {
//         printf("%04x%c", ntohs(pkt[i]), (i == (size / 2) - 1) ? '\n' : ' ');
//     }
// }

static Packet build_packet(Host *h) {
    Packet pkt = {
        .type = ICMP_ECHO,
        .code = 0,
        .checksum = 0,
        .identifier = htons(getpid() & 0xffff),
        .sequence = htons(h->packet_sent),
    };

    struct timeval tv;
    gettimeofday(&tv, NULL);
    size_t tv_len = sizeof(tv);
    memcpy(pkt.payload, &tv, tv_len);

    for (size_t i = tv_len; i < PAYLOAD_SIZE; i++) {
        pkt.payload[i] = 42;
    }

    pkt.checksum = htons(calculate_checksum((uint8_t *)&pkt, PACKET_SIZE));

    // FOR DEBUG
    // print_packet((uint16_t *)&pkt, PACKET_SIZE);

    return pkt;
}

static void send_packet(Host *h) {
    const Packet pkt = build_packet(h);

    if (sendto(h->fd, &pkt, PACKET_SIZE, 0, (struct sockaddr *)&h->addr, sizeof(h->addr)) < 0) {
        fatal("sendto");
    }
    
    h->packet_sent++;
}

static double time_diff(struct timeval *start, struct timeval *end) {
    return (((end->tv_sec * 1000.0) + (end->tv_usec / 1000.0))
        - ((start->tv_sec * 1000.0) + (start->tv_usec / 1000.0)));
}

static void get_response(Host *h) {
    uint8_t buf[IP_MAXPACKET];

    const ssize_t n = recvfrom(h->fd, buf, IP_MAXPACKET, 0, NULL, NULL);
    if (n < 0) {
        if (errno == EWOULDBLOCK) {
            fprintf(stdout, "Request timeout for icmp_seq=%ld\n", h->packet_received);
            return;
        }
        fatal("recvfrom");
    }

    struct ip *ip = (struct ip *)buf;
    size_t ip_len = ip->ip_hl * 4;

    const Packet *pkt = (Packet *)(buf + ip_len);

    struct timeval start, end;
    memcpy(&start, pkt->payload, sizeof(struct timeval));
    gettimeofday(&end, NULL);

    // this is the response
    if (pkt->type == ICMP_ECHOREPLY && pkt->identifier == htons(getpid() & 0xffff)) {
        fprintf(stdout, "%ld bytes from %s: icmp_seq=%ld ttl=%hu time=%.3f ms\n",
            n - ip_len, h->ip, h->packet_received, ip->ip_ttl, time_diff(&start, &end));
        h->packet_received++;
    }
}

static void ping_loop(Host *h) {
    signal(SIGINT, signal_handler);

    fprintf(stdout, "PING %s (%s): %d data bytes\n", h->hostname, h->ip, PAYLOAD_SIZE);

    while (!g_sig) {
        send_packet(h);
        get_response(h);
        if (g_count && h->packet_received == g_count)
            break;
        sleep(1);
    }

    fprintf(stdout, "--- %s ping statistics ---\n", h->hostname);
}

static void init_socket(Host *h) {
    h->fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (h->fd < 0) {
        fatal("socket");
    }

    const struct timeval tv = {
        .tv_sec = 3,
        .tv_usec = 0,
    };
    if (setsockopt(h->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        close(h->fd);
        fatal("setsockopt");
    }
}

int main(int argc, char *argv[]) {
    Host h = {0};

    if (argc == 2) {
    memcpy(h.hostname, argv[1], strlen(argv[1]));
    } else if (argc > 2) {
        if (strcmp(argv[1], "-c") == 0) {
            g_count = atoi(argv[2]);
        } else {
            return 1;
        }
        memcpy(h.hostname, argv[3], strlen(argv[3]));
    } else {
        return 1;
    }

    hostname_resolution(&h);
    init_socket(&h);
    ping_loop(&h);
    return 0;
}
