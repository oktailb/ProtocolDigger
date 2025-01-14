#include "pCapUtils.h"
#include "defs.h"

#include <cstring>
#include <iostream>
#include <thread>
#include <unistd.h>

void sendPcapTo(const std::string& address, uint16_t port, ThreadSafeQueue& queue, uint64_t packetLen)
{
    u_thread_setname(__func__);
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    PacketData *buffer = new PacketData();
    socklen_t addr_size;
    int n;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0){
        perror("[-]socket error");
        exit(1);
    }

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(address.c_str());

    n = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (n < 0) {
        perror("[-]bind error");
        exit(1);
    }

    buffer->len = packetLen;
    buffer->payload.reserve(buffer->len);

    addr_size = sizeof(client_addr);
    // Wait for a client connexion ...
    std::cerr << "Waiting for incoming connexion on port " << port <<  "..." << std::endl;
    recvfrom(sockfd, buffer->payload.data(), buffer->len, 0, (struct sockaddr*)&client_addr, &addr_size);
    std::cerr << "Client connected " << printIP(client_addr.sin_addr.s_addr) << " !" << std::endl;
    std::cerr << buffer->payload.data() << std::endl;

    bool run = queue.pop(buffer, std::chrono::milliseconds(200));

    std::cerr << "Processing packets ..." << std::endl;

    uint64_t timestamp0 = buffer->ts.tv_sec * 1000000 + buffer->ts.tv_usec;
    double timesTotal = 0.0;
    while (run)
    {
        run = queue.pop(buffer, std::chrono::milliseconds(200));
        uint64_t timestamp = buffer->ts.tv_sec * 1000000 + buffer->ts.tv_usec;
        std::this_thread::sleep_for(std::chrono::microseconds((timestamp - timestamp0)));
        sendto(sockfd, buffer->payload.data(), buffer->len, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));

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

void read_socket(const std::string& address, uint16_t port, ThreadSafeQueue& queue, uint64_t packetLen)
{
    u_thread_setname(__func__);

    int sockfd;
    PacketData *buffer = new PacketData();

    buffer->len = packetLen;
    buffer->payload.reserve(buffer->len);
    buffer->payload.resize(buffer->len);

    struct sockaddr_in     servaddr;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(address.c_str());

    ssize_t n = 0;
    socklen_t len;

    sendto(sockfd, buffer->payload.data(), buffer->len, 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
    while (true)
    {
        n = recvfrom(sockfd, buffer->payload.data(), packetLen,
                     MSG_WAITALL, (struct sockaddr *) &servaddr,
                     &len);

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
