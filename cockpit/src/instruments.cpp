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
        double altitude = 0.0;
        double timestamp0 = data.ts.tv_sec * 1000000 + data.ts.tv_usec;
        while (run) {
            run = queue.pop(data, std::chrono::milliseconds(200));
            double timestamp = data.ts.tv_sec * 1000000 + data.ts.tv_usec;
            if (run)
            {
                altitude = extractVarFromData<double>(data, variables["altitude"]);
                ui->EADI->setAltitude(altitude);
                altitude += 0.1;
                usleep(timestamp - timestamp0);
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
