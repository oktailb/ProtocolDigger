#include "include/instruments.h"
#include "configuration.h"

#include <string>

#include <QApplication>
#include <thread>
#include "pCapUtils.h"

int main(int argc, char *argv[])
{
    std::map<std::string, std::string> configuration;

    configuration = readConfiguration(argv[1]);

    std::string pcap_file = configuration["Input/file"];
    std::string pcap_device = configuration["Input/device"];
    std::string pcap_address = configuration["Input/address"];
    std::string filter_expression = configuration["Input/filter"];

    ThreadSafeQueue queue;

    if (pcap_file != "")
    {
        read_pcap_file(pcap_file, filter_expression, queue);
    }
    else if (configuration.find("Input/device") != configuration.end())
    {
        std::thread reader_thread(read_device, pcap_device, filter_expression, std::ref(queue));
        reader_thread.detach();
    }
    else if (configuration.find("Input/address") != configuration.end())
    {
    }

    QApplication a(argc, argv);
    Instruments i(configuration, std::ref(queue));

    i.show();

    return a.exec();
}
