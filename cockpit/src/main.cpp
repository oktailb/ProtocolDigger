#include "include/instruments.h"
#include "configuration.h"

#include <string>

#include <QApplication>
#include <thread>
#include "pCapUtils.h"
#include "variables.h"

int main(int argc, char *argv[])
{
    std::map<std::string, std::string> configuration;

    configuration = readConfiguration(argv[1]);

    std::string mode = configuration["Input/mode"];
    bool relay = configuration["Input/relayPcap"].compare("true") == 0?true:false;
    std::string pcap_file = configuration["Input/file"];
    std::string pcap_device = configuration["Input/device"];
    std::string filter_expression = configuration["Input/filter"];
    std::string address = configuration["Input/address"];

    ThreadSafeQueue queue;

    if (mode.compare("file") == 0)
    {
        read_pcap_file(pcap_file, filter_expression, queue);
        if (relay)
        {
            std::vector<std::string> tokens = split(configuration.find("Input/address")->second, ':');
            std::thread server_thread(sendPcapTo, tokens[0], std::stoi(tokens[1]), std::ref(queue));
            server_thread.detach();
            mode = "socket";
        }
    }
    if (mode.compare("spy") == 0)
    {
        std::thread reader_thread(read_device, pcap_device, filter_expression, std::ref(queue));
        reader_thread.detach();
    }
    if (mode.compare("socket") == 0)
    {
        std::vector<std::string> tokens = split(configuration.find("Input/address")->second, ':');
        std::thread reader_thread(read_socket, tokens[0], std::stoi(tokens[1]), std::ref(queue));
        reader_thread.detach();
    }

    QApplication a(argc, argv);
    Instruments i(configuration, std::ref(queue));

    i.show();
    i.play();

    return a.exec();
}
