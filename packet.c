#include "ping.h"

static uint16_t calculate_checksum(uint8_t const* pkt, size_t size)
{
    uint32_t sum = 0;
    size_t i;

    for (i = 0; i + 1 < size; i += 2) {
        const uint16_t word = pkt[i] << 8 | pkt[i + 1];
        sum += word;
    }

    // the size of the packet can be odd
    if (size % 2 != 0)
        sum += pkt[i] << 8;

    while (sum >> 16) {
        const uint32_t carry = sum >> 16;
        sum = sum & 0xffff;
        sum += carry;
    }
    return (uint16_t)~sum;
}

void build_packet(Host* h, uint8_t* buf)
{
    uint8_t temp[PACKET_SIZE];

    temp[0] = ICMP_ECHO;
    temp[1] = 0;

    uint16_t checksum = 0;
    memcpy(temp + 2, &checksum, 2);

    uint16_t id = htons(getpid() & 0xffff);
    memcpy(temp + 4, &id, 2);

    uint16_t sequence = htons(h->packet_sent);
    memcpy(temp + 6, &sequence, 2);

    struct timeval tv;
    gettimeofday(&tv, nullptr);
    size_t tv_len = sizeof(tv);

    uint8_t payload[PAYLOAD_SIZE];
    memcpy(payload, &tv, tv_len);
    for (size_t i = tv_len; i < PAYLOAD_SIZE; i++)
        payload[i] = 42;
    memcpy(temp + ICMP_MINLEN, payload, PAYLOAD_SIZE);

    checksum = calculate_checksum(temp, PACKET_SIZE);
    checksum = htons(checksum);
    memcpy(temp + 2, &checksum, 2);

    memcpy(buf, temp, PACKET_SIZE);
}

void send_packet(Host* h)
{
    uint8_t buf[IP_MAXPACKET];

    build_packet(h, buf);

    if (sendto(h->fd, buf, PACKET_SIZE, 0, (struct sockaddr*)&h->addr, sizeof(h->addr)) < 0)
        fatal("sendto");

    h->packet_sent++;
}

static double time_diff(struct timeval* start, struct timeval* end)
{
    return (((end->tv_sec * 1000.0) + (end->tv_usec / 1000.0))
        - ((start->tv_sec * 1000.0) + (start->tv_usec / 1000.0)));
}

static void calculate_stddev(Host* h, double t)
{
    if (h->packet_received == 1)
        h->rtt.min = DBL_MAX;

    if (t < h->rtt.min)
        h->rtt.min = t;

    if (t > h->rtt.max)
        h->rtt.max = t;

    // Welford formula
    long n = h->packet_received;
    double delta = t - h->rtt.average;
    h->rtt.average = h->rtt.average + delta / n;
    double delta2 = t - h->rtt.average;
    h->rtt.m2 = h->rtt.m2 + delta * delta2;
    h->rtt.stddev = sqrt(h->rtt.m2 / n);
}

void get_response(Host* h)
{
    uint8_t buf[IP_MAXPACKET];

    ssize_t n = recvfrom(h->fd, buf, IP_MAXPACKET, 0, nullptr, nullptr);
    if (n < 0) {
        if (errno == EWOULDBLOCK) {
            printf("Request timeout for icmp_seq=%ld\n", h->packet_received);
            return;
        }
        fatal("recvfrom");
    }

    size_t ip_hdr_len = (buf[0] & 0x0F) * 4;

    uint8_t* icmp = buf + ip_hdr_len;

    uint8_t type = icmp[0];
    uint8_t code = icmp[1];
    if (type == ICMP_ECHOREPLY) {
        int ttl = buf[8];

        struct timeval start, end;
        memcpy(&start, icmp + ICMP_MINLEN, sizeof(struct timeval));
        gettimeofday(&end, nullptr);

        double rtt = time_diff(&start, &end);

        printf("%ld bytes from %s: icmp_seq=%ld ttl=%d time=%.3f ms\n",
            n - ip_hdr_len, h->ip, h->packet_received, ttl, rtt);
        h->packet_received++;
        calculate_stddev(h, rtt);
    } else {
        if (opts.verbose) {
            printf("%ld bytes from %s: type = %d, code = %d\n",
                n, h->ip, type, code);
        }
    }
}

void print_packet(const uint16_t* pkt, size_t size)
{
    for (size_t i = 0; i < size / 2; i++)
        printf("%04x%c", ntohs(pkt[i]), (i == (size / 2) - 1) ? '\n' : ' ');
}
