#include "include/instruments.h"
#include "./ui_instruments.h"
#include "pCapUtils.h"
#include <thread>
#include <unistd.h>

Instruments::Instruments(std::map<std::string, std::string> &configuration, ThreadSafeQueue &queue, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Instruments)
    , configuration(configuration)
    , queue(queue)
    , timerId(0)
    , run(true)
{
    extractVariablesFromConfiguration(configuration, variables);

    ui->setupUi(this);
    timerId = startTimer(20);
    localFileMode = true;
    if (configuration.find("Input/file") == configuration.end())
        localFileMode = false;
    else
        if (configuration.at("Input/file").compare("") == 0)
            localFileMode = false;
}

Instruments::~Instruments()
{
    stop();
    killTimer(timerId);
    delete ui;
}

void Instruments::timerEvent(QTimerEvent *event)
{
    QMainWindow::timerEvent(event);
    ui->centralwidget->repaint();
    ui->EADI->redraw();
    ui->EHSI->redraw();
    ui->AI->redraw();
    ui->ALT->redraw();
    ui->HI->redraw();
    ui->TC->redraw();
    ui->VSI->redraw();
    ui->ASI->redraw();
}

template<typename T> T extractVarFromData(PacketData &data, varDef_t &var)
{
    T res;

    uint8_t *mapper = (uint8_t*) data.payload.data();

    memcpy(&res, mapper + var.offset, sizeof(T));
    if (var.endian == DataEndian::e_host)
        if ((var.type == DataType::e_int) || (var.type == DataType::e_uint))
            ntoh<T>(res);

    return res;
}

void Instruments::play()
{
    std::thread consumer_thread([this]() {

        PacketData data;
        queue.pop(data, std::chrono::milliseconds(200));
        double timestamp0 = data.ts.tv_sec * 1000000 + data.ts.tv_usec;
        double timesTotal = 0.0;
        while (run) {
            run = queue.pop(data, std::chrono::milliseconds(200));
            double timestamp = data.ts.tv_sec * 1000000 + data.ts.tv_usec;
            if (run)
            {
                auto start = std::chrono::system_clock::now();

                ui->EADI->setAltitude(extractVarFromData<double>(data, variables["altitude"]));
                ui->ALT->setAltitude(extractVarFromData<double>(data, variables["altitude"]));
                ui->EADI->setPitch(extractVarFromData<double>(data, variables["pitch"]));
                ui->EADI->setRoll(extractVarFromData<double>(data, variables["roll"]));
                ui->EADI->setAirspeed(extractVarFromData<double>(data, variables["speed"]));
                ui->ASI->setAirspeed(extractVarFromData<double>(data, variables["speed"]));
                ui->EADI->setTurnRate(extractVarFromData<double>(data, variables["engineTorque"]));
                ui->EADI->setHeading(extractVarFromData<double>(data, variables["yaw"]));
                ui->AI->setPitch(extractVarFromData<double>(data, variables["pitch"]));
                ui->AI->setRoll(extractVarFromData<double>(data, variables["roll"]));
                ui->EHSI->setHeading(extractVarFromData<double>(data, variables["yaw"]));

                //ui->VSI->setClimbRate()
                timesTotal += timestamp - timestamp0;
                int seconds = (int)(timesTotal / 1000000) % 60;
                int minutes = (int)((timesTotal / 1000000) / 60) % 60;
                this->setWindowTitle("Fake Cockpit - " + QString((minutes < 10)?"0":"") + QString::number(minutes) + ":" + QString((seconds < 10)?"0":"") + QString::number(seconds));

                if (localFileMode)
                {
                    auto end = std::chrono::system_clock::now();
                    auto elapsed =
                        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                    usleep(timestamp - timestamp0 - elapsed.count());
                }
                timestamp0 = timestamp;
            }
        }

    });

    consumer_thread.detach();
}

void Instruments::stop()
{
    run = false;
}
