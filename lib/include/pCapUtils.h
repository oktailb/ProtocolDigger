#pragma once

#include <stdint.h>
#include <pcap.h>
#include "ThreadSafeQueue.h"

// Function to parse the UDP packet payload
void process_packet(const uint8_t* packet, const struct pcap_pkthdr * header, ThreadSafeQueue& queue);
void pcapCallback(uint8_t *args, const pcap_pkthdr *header, const uint8_t *packet);
void read_pcap_file(const std::string& file_name, const std::string& filter_exp, ThreadSafeQueue& queue);
void read_device(const std::string& device, const std::string& filter_exp, ThreadSafeQueue& queue);
std::string printIP(uint32_t val);

template<typename T> T ntoh(T val)
{
    T ret = 0;
    if (sizeof(T) == 2)
    {
        ret = ntohs(val);
    }
    if (sizeof(T) == 4)
    {
        ret = ntohl(val);
    }
    if (sizeof(T) == 8)
    {
        uint64_t low = ntohl((uint64_t)val & 0x00000000FFFFFFFF);
        uint64_t high = ntohl(((uint64_t)val & 0xFFFFFFFF00000000) >> 32);
        //std::cerr << std::hex << low << " : " << high << std::dec << std::endl;
        ret = (T)(low + (high << 32));
    }
    return ret;
}

