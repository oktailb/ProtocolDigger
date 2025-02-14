#include "netSpyDialog.h"
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>

netSpyDialog::netSpyDialog(QStringList &devicesList, QWidget *parent)
    :
    QDialog(parent)
{
    QHBoxLayout *   hlayout1 = new QHBoxLayout;
    QHBoxLayout *   hlayout2 = new QHBoxLayout;
    QHBoxLayout *   hlayout3 = new QHBoxLayout;

    vlayout = new QVBoxLayout;
    hlayout1->addWidget(new QLabel("Device:"));
    list = new QComboBox();
    list->addItems(devicesList);
    hlayout1->addWidget(list);

    hlayout2->addWidget(new QLabel("Filter:"));
    filter = new QLineEdit();
    hlayout2->addWidget(filter);

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

netSpyDialog::~netSpyDialog()
{
    delete vlayout;
}

QString netSpyDialog::getDeviceName() const
{
    return list->currentText();
}

QString netSpyDialog::getFilter() const
{
    return filter->text();
}
