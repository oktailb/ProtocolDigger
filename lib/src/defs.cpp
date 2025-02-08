#include "defs.h"
#include "pCapUtils.h"
#include "variables.h"
#include <charconv>
#include <iostream>
#include <thread>
#include <unistd.h>

#ifdef WIN32

HINSTANCE   hInstance;
HINSTANCE   hPrevInstance;
int         nShowCmd;

int __stdcall WinMain(HINSTANCE _hInstance, HINSTANCE _hPrevInstance, char*, int _nShowCmd)
{
    hInstance = _hInstance;
    hPrevInstance = _hPrevInstance;
    nShowCmd = _nShowCmd;

    return main(__argc, __argv);
}
#endif

void usage(int argc, char **argv)
{
    if (argc == 1)
    {
        std::cerr << argv[0] << " config_file.ini" << std::endl;
        exit(EXIT_FAILURE);
    }
}

std::string version()
{
    return (VERSION);
}

ThreadSafeQueue srvQueue;

void init(std::map<std::string, std::string> &configuration, ThreadSafeQueue &queue)
{

    std::string mode = configuration["Input/mode"];
    bool relay = configuration["Input/relayPcap"].compare("true") == 0?true:false;
    bool otherClient = configuration["Input/otherClient"].compare("true") == 0?true:false;
    bool serverMode = configuration["Input/serverMode"].compare("true") == 0?true:false;
    std::string pcap_file = configuration["Input/file"];
    std::string pcap_device = configuration["Input/device"];
    std::string filter_expression = configuration["Input/filter"];
    std::string address = configuration["Input/address"];
    uint64_t packetLen = 0;
    std::string spacketLen = configuration["Input/packetLen"];
    std::from_chars(spacketLen.c_str(), spacketLen.c_str() + spacketLen.size(), packetLen);

    if (mode.compare("file") == 0)
    {
        if (relay)
        {
            std::vector<std::string> tokens = split(configuration.find("Input/address")->second, ':');
            read_pcap_file(pcap_file, filter_expression, srvQueue);
            std::string sport = tokens[1];
            uint16_t port = 0;
            std::from_chars(sport.c_str(), sport.c_str() + sport.size(), port);
            std::thread server_thread(sendPcapTo, tokens[0], port, std::ref(srvQueue), packetLen, serverMode);

            if (!otherClient)
            {
                server_thread.detach();
                mode = "socket";
            }
            else
                server_thread.join();
        }
        else
            read_pcap_file(pcap_file, filter_expression, queue);
    }
    if (mode.compare("spy") == 0)
    {
        std::thread reader_thread(read_device, pcap_device, filter_expression, std::ref(queue));
        reader_thread.detach();
    }
    if (mode.compare("socket") == 0)
    {
        std::vector<std::string> tokens = split(configuration.find("Input/address")->second, ':');
        std::string sport = tokens[1];
        uint16_t port = 0;
        std::from_chars(sport.c_str(), sport.c_str() + sport.size(), port);
        std::thread reader_thread(read_socket, tokens[0], port, std::ref(queue), packetLen, serverMode);
        reader_thread.detach();
    }
}
