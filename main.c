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

static void build_packet(Host *h, uint8_t *buf) {
    uint8_t temp[PACKET_SIZE];

    temp[0] = ICMP_ECHO;
    temp[1] = 0;

    uint16_t checksum = 0;
    memcpy(temp + 2, &checksum, 2);

    uint16_t id = htons(getpid() & 0xffff);
    memcpy(temp + 4, &id, 2);

    uint16_t sequence = htons(h->packet_sent);
    memcpy(temp + 6, &sequence, 2);

    uint8_t payload[PAYLOAD_SIZE];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    size_t tv_len = sizeof(tv);
    memcpy(payload, &tv, tv_len);

    for (size_t i = tv_len; i < PAYLOAD_SIZE; i++) {
        payload[i] = 42;
    }
    memcpy(temp + ICMP_MINLEN, payload, PAYLOAD_SIZE);

    checksum = calculate_checksum(temp, PACKET_SIZE);
    checksum = htons(checksum);

    memcpy(temp + 2, &checksum, 2);

    memcpy(buf, temp, PACKET_SIZE);
}

static void send_packet(Host *h) {
    uint8_t buf[IP_MAXPACKET];

    build_packet(h, buf);

    if (sendto(h->fd, buf, PACKET_SIZE, 0, (struct sockaddr *)&h->addr, sizeof(h->addr)) < 0) {
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

    size_t ip_hdr_len = (buf[0] & 0x0F) * 4;

    const uint8_t *icmp = buf + ip_hdr_len;

    uint8_t type = icmp[0];
    if (type == ICMP_ECHOREPLY) {
        int ttl = buf[8];

        struct timeval start, end;
        memcpy(&start, icmp + ICMP_MINLEN, sizeof(struct timeval));
        gettimeofday(&end, NULL);

        fprintf(stdout, "%ld bytes from %s: icmp_seq=%ld ttl=%d time=%.3f ms\n",
            n - ip_hdr_len, h->ip, h->packet_received, ttl, time_diff(&start, &end));
    }

    h->packet_received++;
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

    double loss = (double)(h->packet_sent - h->packet_received) / (double)h->packet_sent;
    fprintf(stdout, "%ld packets transmitted, %ld packets received, %.1f%% packet loss\n",
        h->packet_sent, h->packet_received, 100.0 * loss);
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
