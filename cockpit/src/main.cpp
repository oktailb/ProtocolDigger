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

    std::string pcap_file = configuration["Input/file"];
    std::string pcap_device = configuration["Input/device"];
    std::string filter_expression = configuration["Input/filter"];
    std::string address = configuration["Input/address"];

    ThreadSafeQueue queue;

    if (pcap_file != "")
    {
        read_pcap_file(pcap_file, filter_expression, queue);
    }
    else if (pcap_device != "")
    {
        std::thread reader_thread(read_device, pcap_device, filter_expression, std::ref(queue));
        reader_thread.detach();
    }
    else if (address != "")
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
