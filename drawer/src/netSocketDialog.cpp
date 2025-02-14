#include "netSocketDialog.h"
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>

netSocketDialog::netSocketDialog(QWidget *parent)
    :
    QDialog(parent)
{
    QHBoxLayout *   hlayout1 = new QHBoxLayout;
    QHBoxLayout *   hlayout2 = new QHBoxLayout;
    QHBoxLayout *   hlayout3 = new QHBoxLayout;

    vlayout = new QVBoxLayout;

    hlayout1->addWidget(new QLabel("UDP Address:"));
    address = new QLineEdit();
    hlayout1->addWidget(address);

    hlayout2->addWidget(new QLabel("UDP Port:"));
    port = new QSpinBox();
    port->setMinimum(1);
    port->setMaximum(65535);
    port->setValue(3033);
    hlayout2->addWidget(port);

    QDialogButtonBox * buttons = new QDialogButtonBox( QDialogButtonBox::Ok
                                                     | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    hlayout3->addWidget(buttons);

    vlayout->addLayout(hlayout1);
    vlayout->addLayout(hlayout2);
    vlayout->addLayout(hlayout3);

    setLayout(vlayout);
}

netSocketDialog::~netSocketDialog()
{
    delete vlayout;
}

QString netSocketDialog::getAddress() const
{
    return address->text().split(":")[0];
}

uint16_t netSocketDialog::getPort() const
{
    return address->text().split(":")[1].toInt();
}
