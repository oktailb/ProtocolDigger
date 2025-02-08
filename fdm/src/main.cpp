#include "ThreadSafeQueue.h"
#include "configuration.h"
#include "defs.h"
#include "variables.h"
#include "pCapUtils.h"
#include <charconv>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <net_fdm.hxx>

double htond (double x)	
{
    int * p = (int*)&x;
    int tmp = p[0];
    p[0] = htonl(p[1]);
    p[1] = htonl(tmp);

    return x;
}

float htonf (float x)	
{
    int * p = (int *)&x;
    *p = htonl(*p);
    return x;
}

void run(int sendSocket);

ThreadSafeQueue queue;
struct sockaddr_in server_addr;
std::map<std::string, varDef_t> variables;

int main(int argc, char ** argv)
{
    if (argc < 2)
        usage(argc, argv);
    std::map<std::string, std::string> configuration;
    configuration = readConfiguration(argv[1]);

    extractVariablesFromConfiguration(configuration, variables);

    std::string FDMaddress = configuration["Input/FDMaddress"];
    std::string spacketLen = configuration["Input/packetLen"];
    uint16_t packetLen = 0;
    std::from_chars(spacketLen.c_str(), spacketLen.c_str() + spacketLen.size(), packetLen);

    init(configuration, queue);

    std::vector<std::string> FDMtokens = split(FDMaddress, ':');
    std::string sport = FDMtokens[1];
    uint16_t port = 0;
    std::from_chars(sport.c_str(), sport.c_str() + sport.size(), port);
    int sockfd = configureSocket(FDMtokens[0], port, packetLen, false, &server_addr);
    run(sockfd);
}

#define D2R (3.14159 / 180.0)

template<typename T> T extractVarFromData(PacketData *data, varDef_t &var)
{
    T res;

    uint8_t *mapper = (uint8_t*) data->payload.data();

    memcpy(&res, mapper + var.offset, sizeof(T));
    if (var.endian == DataEndian::e_host)
        if ((var.type == DataType::e_int) || (var.type == DataType::e_uint))
            ntoh<T>(res);

    return res;
}

void run(int sendSocket)
{
    PacketData *buffer = new PacketData();
    timeval timestamp0 = buffer->ts;
    FGNetFDM fdm;
    memset(&fdm,0,sizeof(fdm));
    fdm.version = htonl(FG_NET_FDM_VERSION);

    bool run = true;
    while (run)
    {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        run = queue.pop(buffer, std::chrono::milliseconds(200));
        timeval timestamp = buffer->ts;

        double altitude = extractVarFromData<double>(buffer, variables["altitude"]);
        double roll = extractVarFromData<double>(buffer, variables["roll"]);
        double pitch = extractVarFromData<double>(buffer, variables["pitch"]);
        double yaw = extractVarFromData<double>(buffer, variables["yaw"]);

        //work ...
        // fdm.latitude = htond(latitude * D2R);
        // fdm.longitude = htond(longitude * D2R);
        fdm.altitude = htond(altitude);

        fdm.phi = htonf(roll * D2R);
        fdm.theta = htonf(pitch * D2R);
        fdm.psi = htonf(yaw * D2R);

        fdm.num_engines = htonl(1);

        fdm.num_tanks = htonl(1);
        fdm.fuel_quantity[0] = htonf(100.0);

        fdm.num_wheels = htonl(3);

        fdm.cur_time = htonl(time(0));
        fdm.warp = htonl(1);


        sendto(sendSocket,(char *)&fdm,sizeof(fdm),0,(struct sockaddr *)&server_addr,sizeof(server_addr));



        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::microseconds(timeval_diff(&timestamp, &timestamp0)) - std::chrono::duration_cast<std::chrono::milliseconds>(end - begin));
        timestamp0 = timestamp;
    }
}






















