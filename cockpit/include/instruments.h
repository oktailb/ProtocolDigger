#ifndef INSTRUMENTS_H
#define INSTRUMENTS_H

#include "ThreadSafeQueue.h"
#include "variables.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class Instruments;
}
QT_END_NAMESPACE

class Instruments : public QMainWindow
{
    Q_OBJECT

public:
    Instruments(std::map<std::string, std::string> &configuration, ThreadSafeQueue &queue, QWidget *parent = nullptr);
    ~Instruments();

    void play();
    void stop();

protected:
    void timerEvent(QTimerEvent *event);

private:
    Ui::Instruments *ui;
    std::map<std::string, std::string> &configuration;
    std::map<std::string, varDef_t> variables;

    ThreadSafeQueue &queue;
    int timerId;
    bool run;

};
#endif // INSTRUMENTS_H
