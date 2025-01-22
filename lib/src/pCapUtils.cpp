#include "pCapUtils.h"
#include "defs.h"

#include <cstring>
#include <iostream>
#include <thread>
#include <unistd.h>

uint64_t timeval_diff(struct timeval *end_time, struct timeval *start_time)
{
    struct timeval difference;

    difference.tv_sec =end_time->tv_sec -start_time->tv_sec ;
    difference.tv_usec=end_time->tv_usec-start_time->tv_usec;

    /* Using while instead of if below makes the code slightly more robust. */

    while(difference.tv_usec<0)
    {
        difference.tv_usec+=1000000;
        difference.tv_sec -=1;
    }

    return 1000000LL * difference.tv_sec + difference.tv_usec;

}

int configureSocket(const std::string& address, uint16_t port, uint64_t packetLen, bool serverMode, struct sockaddr_in *sock_addr)
{
    int sockfd;
    PacketData *buffer = new PacketData();
    socklen_t addr_size;
    int n;

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0){
        perror("[-]socket error");
        exit(1);
    }

    memset(sock_addr, '\0', sizeof(*sock_addr));
    sock_addr->sin_family = AF_INET;
    sock_addr->sin_port = htons(port);
    sock_addr->sin_addr.s_addr = inet_addr(address.c_str());

    if (serverMode)
    {
        n = bind(sockfd, (struct sockaddr*)sock_addr, sizeof(*sock_addr));
        if (n < 0) {
            perror("[-]bind error");
            exit(1);
        }

        buffer->len = packetLen;
        buffer->payload.reserve(buffer->len);

        addr_size = sizeof(sock_addr);
        // Wait for a client connexion ...
        std::cerr << "Waiting for incoming connexion on port " << port <<  "..." << std::endl;
        recvfrom(sockfd, buffer->payload.data(), buffer->len, 0, (struct sockaddr*)sock_addr, &addr_size);
        std::cerr << "Client connected " << printIP(sock_addr->sin_addr.s_addr) << " !" << std::endl;
        std::cerr << buffer->payload.data() << std::endl;
    }
    else
    {
        std::cerr << "Knocking door on " << address << ":"  << port <<  "..." << std::endl;
        sendto(sockfd, buffer->payload.data(), 0, 0, (struct sockaddr*)sock_addr, sizeof(*sock_addr));
        std::cerr << "Connected granted to " << printIP(sock_addr->sin_addr.s_addr) << " !" << std::endl;
    }
    delete buffer;

    return sockfd;
}

void sendPcapTo(const std::string& address, uint16_t port, ThreadSafeQueue& queue, uint64_t packetLen, bool serverMode)
{
    u_thread_setname(__func__);
    PacketData *buffer = new PacketData();
    struct sockaddr_in client_addr;

    int sockfd = configureSocket(address, port, packetLen, serverMode, &client_addr);

    bool run = queue.pop(buffer, std::chrono::milliseconds(200));

    std::cerr << "Processing packets ..." << std::endl;

    timeval timestamp0 = buffer->ts;
    while (run)
    {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        run = queue.pop(buffer, std::chrono::milliseconds(200));
        timeval timestamp = buffer->ts;
        sendto(sockfd, buffer->payload.data(), buffer->len, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::microseconds(timeval_diff(&timestamp, &timestamp0)) - std::chrono::duration_cast<std::chrono::milliseconds>(end - begin));
        timestamp0 = timestamp;
    }
    std::cerr << "Done." << std::endl;
}

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

    PacketData *data = new PacketData();
    data->len = ntohs(udp_header->length) /*- (ethernet_header_size + ip_header_size + udp_header_size)*/;
    data->payload.assign(payload, payload + data->len);
    data->ts.tv_sec = header->ts.tv_sec;
    data->ts.tv_usec = header->ts.tv_usec;
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

void read_device(const std::string& device, const std::string& filter_exp, ThreadSafeQueue& queue)
{
    u_thread_setname(__func__);

    char errbuf[PCAP_ERRBUF_SIZE];

    pcap_t* handle = pcap_open_live(device.c_str(),8192, 1, 20, errbuf);
    if (!handle) {
        std::cerr << "Could not open device: " << errbuf << std::endl;
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

void read_socket(const std::string& address, uint16_t port, ThreadSafeQueue& queue, uint64_t packetLen, bool serverMode)
{
    u_thread_setname(__func__);
    PacketData *buffer = new PacketData();
    struct sockaddr_in server_addr;

    int sockfd = configureSocket(address, port, packetLen, !serverMode, &server_addr);

    buffer->len = packetLen;
    buffer->payload.reserve(buffer->len);
    buffer->payload.resize(buffer->len);

    int n;
    socklen_t addr_size = sizeof(server_addr);

    while (true)
    {
        n = recvfrom(sockfd, buffer->payload.data(), packetLen,
                     MSG_WAITALL, (struct sockaddr *)&server_addr,
                     &addr_size);

        buffer->len = n;
        gettimeofday(&buffer->ts, NULL);

        queue.push(buffer);
    }
    close(sockfd);
}

std::string printIP(uint32_t val)
{
    std::string res = "";
    //val = ntohl(val);
    res += std::to_string((val) & 0xFF);
    res += ".";
    res += std::to_string((val >> 8) & 0xFF);
    res += ".";
    res += std::to_string((val >> 16) & 0xFF);
    res += ".";
    res += std::to_string((val >> 24) & 0xFF);

    return res;
}
