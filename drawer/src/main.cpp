#include "include/mainwindow.h"
#include "include/instruments.h"
#include "configuration.h"
#include "variables.h"

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

#include <QApplication>
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

ttype ntoh(ttype val)
{
    ttype ret = 0;
    if (sizeof(ttype) == 2)
    {
        ret = ntohs(val);
    }
    if (sizeof(ttype) == 4)
    {
        ret = ntohl(val);
    }
    if (sizeof(ttype) == 8)
    {
        uint64_t low = ntohl((uint64_t)val & 0x00000000FFFFFFFF);
        uint64_t high = ntohl(((uint64_t)val & 0xFFFFFFFF00000000) >> 32);
        //std::cerr << std::hex << low << " : " << high << std::dec << std::endl;
        ret = (ttype)(low + (high << 32));
    }
    return ret;
}

int main(int argc, char *argv[])
{
    std::map<std::string, std::string> configuration = readConfiguration(argv[1]);
    std::map<std::string, varDef_t> variables;
    extractVariablesFromConfiguration(configuration, variables);

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
    }
    else if (configuration.find("Input/address") != configuration.end())
    {
    }

    QApplication a(argc, argv);
    DebugWindow w(variables, queue);
    Instruments i;

    w.show();
    i.show();

    return a.exec();
}
