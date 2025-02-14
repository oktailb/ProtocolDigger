#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QComboBox>
#include <QLineEdit>

class netSpyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit netSpyDialog(QStringList &devicesList, QWidget *parent = nullptr);
    ~netSpyDialog();

    QString getDeviceName() const;
    QString getFilter() const;

protected:
//    void closeEvent(QCloseEvent *e) override;

private:
    QVBoxLayout *   vlayout;

    QComboBox *     list;
    QLineEdit*      filter;
};
