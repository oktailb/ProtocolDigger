#include "include/instruments.h"
#include "./ui_instruments.h"
#include <thread>
#include <unistd.h>

Instruments::Instruments(std::map<std::string, std::string> &configuration, ThreadSafeQueue &queue, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Instruments)
    , configuration(configuration)
    , queue(queue)
    , timerId(0)
{
    std::map<std::string, varDef_t> variables;
    extractVariablesFromConfiguration(configuration, variables);

    ui->setupUi(this);
    timerId = startTimer(20);

    std::thread consumer_thread([&queue, &configuration, &variables, this]() {

        PacketData data;
        bool run = true;
        queue.pop(data, std::chrono::milliseconds(200));
        double altitude = 0.0;
        double timestamp0 = data.ts.tv_sec * 1000000 + data.ts.tv_usec;
        while (run) {
            run = queue.pop(data, std::chrono::milliseconds(200));
            double timestamp = data.ts.tv_sec * 1000000 + data.ts.tv_usec;
            if (run)
            {
                ui->EADI->setAltitude(altitude);
                altitude += 0.1;
                usleep(timestamp - timestamp0);
                timestamp0 = timestamp;
            }
        }
    });

    consumer_thread.detach();
}

Instruments::~Instruments()
{
    killTimer(timerId);
    delete ui;
}

void Instruments::timerEvent(QTimerEvent *event)
{
    QMainWindow::timerEvent(event);
    ui->centralwidget->repaint();
}
