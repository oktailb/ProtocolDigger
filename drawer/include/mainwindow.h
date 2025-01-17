#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
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
    void wheelEvent(QWheelEvent *event);

private slots:
    void on_variables_currentIndexChanged(int index);
    void on_variables_globalCheckStateChanged(int index);
    void on_offset_valueChanged(int offset);
    void on_type_currentTextChanged(const QString &arg1);
    void on_size_currentTextChanged(const QString &sizeStr);
    void on_mask_textChanged(const QString &mask);
    void on_shift_valueChanged(int shift);
    void on_ratio_valueChanged(double ratio);
    void on_applyButton_clicked();
    void on_endian_toggled(bool checked);
    void on_autoSearch_clicked();

    void on_len_valueChanged(int arg1);

    void on_fixRange_clicked();

private:
    Ui::DebugWindow *ui;
    void fillVariables(std::map<std::string, varDef_t> &variables);
    void SeriesFromOffset(uint32_t offset, uint32_t size, uint32_t len, DataType type, bool toHostEndian, uint64_t mask, uint8_t shift, double ratio);
    bool computeChartByCriteria(std::string name,
                                uint32_t offset,
                                DataSize size, uint32_t len,
                                DataType type,
                                bool toHostEndian,
                                uint64_t mask,
                                uint8_t shift,
                                double ratio, double diversityMin, double minVal, double maxVal, double amplitudeMin, double amplitudeMax);
    void updateChart(std::string name);
    void saveFGFSGenericProtocol(const std::map<std::string, std::string>& keyValuePairs, const std::string& filename);
    std::map<std::string, varDef_t> variables;
    ThreadSafeQueue &queue;
    std::deque<PacketData*> queueData;
    int timerId;
    uint32_t packetSize;
    bool localFileMode;
    QChart *chart;
    std::map<std::string, std::string> configuration;
    bool ready;
    std::map<uint32_t, QLineSeries *> series;
    std::map<uint32_t, uint32_t> pktIndex;
    double timestamp;
    double timeWindowSize;
};
#endif // MAINWINDOW_H
