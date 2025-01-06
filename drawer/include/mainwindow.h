#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qlineseries.h>
#include "variables.h"
#include "ThreadSafeQueue.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class DebugWindow;
}
QT_END_NAMESPACE

class DebugWindow : public QMainWindow
{
    Q_OBJECT

public:
    DebugWindow(std::map<std::string, std::string> &configuration, ThreadSafeQueue &queue, QWidget *parent = nullptr);
    ~DebugWindow();

protected:
    void timerEvent(QTimerEvent *event);

private slots:
    void on_variables_currentIndexChanged(int index);
    void on_offset_valueChanged(int offset);
    void on_type_currentTextChanged(const QString &arg1);
    void on_size_currentTextChanged(const QString &sizeStr);
    void on_endian_checkStateChanged(const Qt::CheckState &arg1);
    void on_mask_textChanged(const QString &mask);
    void on_shift_valueChanged(int shift);
    void on_ratio_valueChanged(double ratio);

    void on_applyButton_clicked();

    void on_variables_editTextChanged(const QString &arg1);

private:
    Ui::DebugWindow *ui;
    void fillVariables(std::map<std::string, varDef_t> &variables);
    QLineSeries * SeriesFromOffset(uint32_t offset, uint32_t size, DataType type, bool toHostEndian, uint64_t mask, uint8_t shift, double ratio);
    void updateChart(std::string name, uint32_t offset, DataSize size, DataType type, bool toHostEndian, uint64_t mask, uint8_t shift, double ratio);

    std::map<std::string, varDef_t> variables;
    std::deque<PacketData> queue;
    int timerId;

    std::map<std::string, std::string> configuration;
};
#endif // MAINWINDOW_H
