#ifndef INSTRUMENTS_H
#define INSTRUMENTS_H

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
    Instruments(QWidget *parent = nullptr);
    ~Instruments();

private:
    Ui::Instruments *ui;
};
#endif // INSTRUMENTS_H
