#include "defs.h"
#include "configuration.h"

#include <algorithm>
#include <iomanip>
#include <numeric>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <math.h>
#include <iostream>

#include "ThreadSafeQueue.h"
#include "pCapUtils.h"
#include "variables.h"

#define ttype int32_t

long double mylog2(long double number ) {
    return log( number ) / log( 2 ) ;
}

double entropy(std::vector<ttype> data)
{
    std::map<ttype , int> frequencies ;
    for ( auto value : data )
        frequencies[ value ] ++ ;
    int numlen = data.size();
    double infocontent = 0 ;
    for ( std::pair<ttype , int> p : frequencies ) {
        double freq = static_cast<double>( p.second ) / numlen ;
        infocontent += freq * mylog2( freq ) ;
    }
    infocontent *= -1 ;

    return infocontent;
}

uint32_t diversity(std::vector<ttype> data)
{
    std::map<ttype , int> frequencies ;
    for ( auto value : data )
        frequencies[ value ] ++ ;

    return frequencies.size();
}

template< typename T >
std::string int_to_hex( T i )
{
    std::stringstream stream;
    stream << "0x"
           << std::setfill ('0') << std::setw(sizeof(T)*2)
           << std::hex << i;
    return stream.str();
}

int main(int argc, char* argv[]) {

    usage(argc, argv);
    std::map<std::string, std::string> configuration = readConfiguration(argv[1]);
    std::map<std::string, varDef_t> variables;
    extractVariablesFromConfiguration(configuration, variables);

    std::string pcap_file = configuration["Input/file"];
    std::string pcap_device = configuration["Input/device"];
    std::string pcap_address = configuration["Input/address"];
    std::string filter_expression = configuration["Input/filter"];


    if (pcap_file != "")
    {
        ThreadSafeQueue queue;

        std::thread reader_thread(read_pcap_file, pcap_file, filter_expression, std::ref(queue));

        // Consumer thread to process packets from the queue
        std::thread consumer_thread([&queue, &configuration, &variables]() {
            bool fullScan = true;

            PacketData * data = new PacketData();
            std::map<uint32_t, std::vector<ttype> > csv;
            bool run = true;
            while (run) {
                run = queue.pop(data, std::chrono::milliseconds(200));
                if (run)
                {
                    uint32_t *mapper32 = (uint32_t*) data->payload.data();
                    uint32_t len32 = data->payload.size() / 4;
                    for (uint32_t offset = 0 ; offset < len32 / 4; offset++)
                    {
                        mapper32[offset] = ntoh(mapper32[offset]); // completement débile
                    }
                    uint8_t *mapper = (uint8_t*) data->payload.data();
                    uint32_t len = data->payload.size() / (fullScan?1:sizeof(ttype));

                    for (uint32_t offset = 0 ; offset < len - sizeof(ttype); offset++)
                    {
                        ttype tmp;
                        memcpy(&tmp, mapper + offset, sizeof(ttype));
                        tmp = ntoh(tmp);
                        uint32_t realOffset = offset * (fullScan?1:sizeof(ttype));
                        csv[realOffset].push_back(tmp);
                    }
                }
            }
            for (auto var : csv)
            {
                uint32_t offset = var.first;
                double avg = std::accumulate(var.second.begin(), var.second.end(), 0.0) / var.second.size();
                double ent = entropy(var.second);
                double div = diversity(var.second);
                ttype max = *std::max_element(var.second.begin(), var.second.end());
                ttype min = *std::min_element(var.second.begin(), var.second.end());
                // if (avg > 1000000)
                //    continue;
                // if (avg < 10)
                //    continue;
                // if (min == max)
                //     continue;
                // if (ent < 0.1)
                //     continue;
                if (div <= 16)
                    continue;
                uint32_t currentOffset = var.first;
                std::cerr << "OFFSET " << currentOffset << " : ";
                std::cerr << "AVG " << avg << " , ";
                std::cerr << "DIV " << div << " , ";
                std::cerr << "ENT " << ent << std::endl;
                std::string existingVar = findByOffset(currentOffset, variables);
                if (existingVar != "")
                {
                    varDef_t var = variables[existingVar];
                    std::cout << existingVar << ";";

                }
                else
                    std::cout << "OFFSET " << currentOffset << ";";
                for (auto value : var.second)
                {
                    std::cout << (ttype)value << ";";
                }
                std::cout << std::endl;
            }

        });

        consumer_thread.join();
        reader_thread.detach();
        return 0;
    }
    else if (configuration.find("Input/device") != configuration.end())
    {
    }
    else if (configuration.find("Input/address") != configuration.end())
    {
    }
}
