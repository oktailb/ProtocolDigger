#include "include/instruments.h"
#include "./ui_instruments.h"

Instruments::Instruments(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Instruments)
{
    ui->setupUi(this);
}

Instruments::~Instruments()
{
    delete ui;
}
