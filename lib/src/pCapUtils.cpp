#include "pCapUtils.h"
#include "defs.h"

#include <iostream>

// Function to parse the UDP packet payload
void process_packet(const uint8_t* packet, const struct pcap_pkthdr * header, ThreadSafeQueue& queue)
{
    const size_t ethernet_header_size = sizeof(ether_header_t);
    const size_t ip_header_size = sizeof(ip_header_t); // Assuming no options
    const size_t udp_header_size = sizeof(udp_header_t);

    if (header->len < ethernet_header_size + ip_header_size + udp_header_size) {
        return;
    }

    const ip_header_t* ip_header = (const ip_header_t*)(packet + ethernet_header_size);
    const udp_header_t* udp_header = (const udp_header_t*)((uint8_t*)ip_header + ip_header_size);
    const uint8_t* payload = (uint8_t *)udp_header + udp_header_size;

    PacketData data;
    data.len = ntohs(udp_header->length) /*- (ethernet_header_size + ip_header_size + udp_header_size)*/;
    data.payload.assign(payload, payload + data.len);
    data.ts.tv_sec = header->ts.tv_sec;
    data.ts.tv_usec = header->ts.tv_usec;
    queue.push(data);
}

void pcapCallback(uint8_t *args, const pcap_pkthdr *header, const uint8_t *packet)
{
    ThreadSafeQueue* queue = reinterpret_cast<ThreadSafeQueue*>(args); // Correct cast to pointer
    if (queue) { // Always check for null pointers!
        process_packet(packet, header, *queue); // Dereference the pointer
    } else {
        std::cerr << "Error: Queue pointer is null in pcapCallback" << std::endl;
    }
}

void read_pcap_file(const std::string& file_name, const std::string& filter_exp, ThreadSafeQueue& queue)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* handle = pcap_open_offline(file_name.c_str(), errbuf);
    if (!handle) {
        std::cerr << "Could not open pcap file: " << errbuf << std::endl;
        return;
    }

    bpf_program filter;
    if (pcap_compile(handle, &filter, filter_exp.c_str(), 0, PCAP_NETMASK_UNKNOWN) == -1) {
        std::cerr << "Could not parse filter: " << pcap_geterr(handle) << std::endl;
        pcap_close(handle);
        return;
    }

    if (pcap_setfilter(handle, &filter) == -1) {
        std::cerr << "Could not apply filter: " << pcap_geterr(handle) << std::endl;
        pcap_close(handle);
        return;
    }

    const uint8_t* packet;
    struct pcap_pkthdr header;

    pcap_loop(handle, -1, pcapCallback, (uint8_t *)(&queue));

    pcap_close(handle);
}

std::string printIP(uint32_t val)
{
    std::string res = "";
    val = ntohl(val);
    res += std::to_string((val) & 0xFF);
    res += ".";
    res += std::to_string((val >> 8) & 0xFF);
    res += ".";
    res += std::to_string((val >> 16) & 0xFF);
    res += ".";
    res += std::to_string((val >> 24) & 0xFF);

    return res;
}
