#pragma once
#include "ThreadSafeQueue.h"
#include <map>
#include <string>
#include <netinet/ip.h>

typedef uint32_t addr_t;
typedef uint16_t port_t;

#pragma pack(push, 1)
typedef struct {
    uint8_t  dst_addr[6];
    uint8_t  src_addr[6];
    uint16_t llc_len;
} ether_header_t;

typedef struct {
    uint8_t  ver_ihl;  // 4 bits version and 4 bits internet header length
    uint8_t  tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fo; // 3 bits flags and 13 bits fragment-offset
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    addr_t   src_addr;
    addr_t   dst_addr;
} ip_header_t;

typedef struct {
    port_t   src_port;
    port_t   dst_port;
    uint16_t length;
    uint16_t checksum;
} udp_header_t;

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint16_t  data_offset:4;
    uint16_t flags:12;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_p;
} tcp_header_t;

typedef struct {
    ether_header_t  eth;
    ip_header_t     ip;
    tcp_header_t    tcp;
} eth_tcp_ip;

typedef struct {
    ether_header_t  eth;
    ip_header_t     ip;
    tcp_header_t    udp;
} eth_udp_ip;

#pragma pack(pop)

#ifdef WIN32
#elif __linux__
#endif

#ifdef WIN32
int main(int argc, char** argv);

extern HINSTANCE   hInstance;
extern HINSTANCE   hPrevInstance;
extern int         nShowCmd;

int __stdcall WinMain(HINSTANCE _hInstance, HINSTANCE _hPrevInstance, char*, int _nShowCmd);
#endif

void usage(int argc, char **argv);
std::string version();
void init(std::map<std::string, std::string> &configuration, ThreadSafeQueue &queue);
