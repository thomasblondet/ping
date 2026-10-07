#include "ping.h"

static int sig = 0;
Options opts = { };

static void signal_handler(int tmp)
{
    sig = 1;
    (void)tmp;
}

void fatal(char const* str)
{
    fprintf(stderr, "ping: ");
    perror(str);
    exit(1);
}

static void help(void)
{
    printf(
        "Usage: ping [options] <host>\n\n"

        "Options:\n"
        "  -c <count>    stop after sending <count> packets\n"
        "  -m <ttl>      change the TTL in the IP header\n"
        "  -v            print non-echo replies\n"
        "  -h            display this help message\n"
    );

    exit(0);
}

static void hostname_resolution(Host* h)
{
    struct addrinfo hints = {
        .ai_flags = 0,
        .ai_family = AF_INET,
        .ai_socktype = 0,
        .ai_protocol = 0,
        .ai_addrlen = 0,
        .ai_addr = nullptr,
    };

    struct addrinfo* res = nullptr;
    int ret = getaddrinfo(h->hostname, nullptr, &hints, &res);
    if (ret) {
        printf("ping: cannot resolve %s: %s\n", h->hostname, gai_strerror(ret));
        exit(1);
    }

    struct sockaddr_in* sin = (struct sockaddr_in*)res->ai_addr;
    inet_ntop(AF_INET, &sin->sin_addr, h->ip, INET_ADDRSTRLEN);
    h->addr.sin_family = AF_INET;
    memcpy(&h->addr.sin_addr, &sin->sin_addr, sizeof(sin->sin_addr));

    freeaddrinfo(res);
}

static void ping_loop(Host* h)
{
    signal(SIGINT, signal_handler);

    printf("PING %s (%s): %d data bytes\n", h->hostname, h->ip, PAYLOAD_SIZE);

    while (!sig) {
        send_packet(h);
        get_response(h);
        if (opts.count && h->packet_received == opts.count)
            break;
        sleep(1);
    }

    printf("--- %s ping statistics ---\n", h->hostname);

    double loss = (double)(h->packet_sent - h->packet_received) / (double)h->packet_sent;
    printf("%ld packets transmitted, %ld packets received, %.1f%% packet loss\n",
        h->packet_sent, h->packet_received, 100.0 * loss);

    printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
        h->rtt.min, h->rtt.average, h->rtt.max, h->rtt.stddev);

    close(h->fd);
}

static void init_socket(Host* h)
{
    h->fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (h->fd < 0)
        fatal("socket");

    struct timeval tv = {
        .tv_sec = 3,
        .tv_usec = 0,
    };
    if (setsockopt(h->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        fatal("setsockopt");

    if (opts.ttl) {
        if (setsockopt(h->fd, IPPROTO_IP, IP_TTL, &opts.ttl, sizeof(opts.ttl)) < 0)
            fatal("setsockopt");
    }
}

int main(int argc, char* argv[])
{
    Host h = { };
    
    if (argc < 2) {
        help();
        return EXIT_FAILURE;
    }
    
    int c;
    while ((c = getopt(argc, argv, "c:m:vh")) != -1) {
        switch (c) {
            case 'c':
                opts.count = atoi(optarg);
                break;
            case 'm':
                opts.ttl = atoi(optarg);
                break;
            case 'v':
                opts.verbose = atoi(optarg);
                break;
            case 'h':
                help();
                break;
            default:
                break;
        }
    }

    if (optind < argc)
        snprintf(h.hostname, sizeof(h.hostname), "%s", argv[argc - 1]);

    hostname_resolution(&h);
    init_socket(&h);
    ping_loop(&h);
    
    return EXIT_SUCCESS;
}
