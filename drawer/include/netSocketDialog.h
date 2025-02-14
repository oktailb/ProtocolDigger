#pragma once

#include <QSpinBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QComboBox>
#include <QLineEdit>

class netSocketDialog : public QDialog
{
    Q_OBJECT

public:
    explicit netSocketDialog(QWidget *parent = nullptr);
    ~netSocketDialog();

    QString  getAddress() const;
    uint16_t getPort() const;

protected:
//    void closeEvent(QCloseEvent *e) override;

private:
    QVBoxLayout *   vlayout;

    QSpinBox*       port;
    QLineEdit*      address;
};
