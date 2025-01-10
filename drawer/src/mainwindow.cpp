#include "include/mainwindow.h"
#include "./ui_mainwindow.h"
#include "pCapUtils.h"
#include "variables.h"
#include "configuration.h"
#include <stdint.h>
#include <stdfloat>
#include <netinet/in.h>
#include <QtGui>

long double mylog2(long double number ) {
    return log( number ) / log( 2 ) ;
}

template <typename T> double entropy(std::vector<T> data)
{
    std::map<T , int> frequencies ;
    for ( auto value : data )
        frequencies[ value ] ++ ;
    int numlen = data.size();
    double infocontent = 0 ;
    for ( std::pair<T , int> p : frequencies ) {
        double freq = static_cast<double>( p.second ) / numlen ;
        infocontent += freq * mylog2( freq ) ;
    }
    infocontent *= -1 ;

    return infocontent;
}

template <typename T> uint32_t diversity(std::vector<T> data)
{
    std::map<T , int> frequencies ;
    for ( auto value : data )
        frequencies[ value ] ++ ;

    return frequencies.size();
}

DebugWindow::DebugWindow(std::map<std::string, std::string> &configuration, ThreadSafeQueue &queue, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::DebugWindow)
    , configuration(configuration)
    , queue(queue)
    , queueData(queue.data())
    , timerId(0)
    , packetSize(0)
{
    extractVariablesFromConfiguration(configuration, variables);

    ui->setupUi(this);
    fillVariables(variables);
    timerId = startTimer(20);
    localFileMode = true;
    if (configuration.find("Input/file") == configuration.end())
        localFileMode = false;
    else
        if (configuration.at("Input/file").compare("") == 0)
            localFileMode = false;
}

void DebugWindow::fillVariables(std::map<std::string, varDef_t> &variablesList)
{
    for (auto item : variablesList)
        ui->variables->addCheckItem(QString(item.first.c_str()), item.second.offset, Qt::CheckState::Unchecked);
    for (auto type : stringDataType)
        ui->type->insertItem(ui->type->count() + 1, QString(type.first.c_str()), type.second);
    for (auto type : stringDataSize)
        ui->size->insertItem(ui->size->count() + 1, QString(type.first.c_str()), type.second);
}

DebugWindow::~DebugWindow()
{
    killTimer(timerId);
    delete ui;
}

// void DebugWindow::timerEvent(QTimerEvent *event)
// {
// //    QMainWindow::timerEvent(event);
// //    ui->centralwidget->repaint();
// }

QLineSeries * DebugWindow::SeriesFromOffset(uint32_t offset, uint32_t size, DataType type, bool toHostEndian, uint64_t mask, uint8_t shift, double ratio)
{
    QLineSeries *series = new QLineSeries();
    //series->append(0, 0);
    double timestamp0 = queueData[0].ts.tv_sec * 1000 + queueData[0].ts.tv_usec / 1000;
    packetSize = queueData[0].payload.size();
    ui->offset->setMaximum(packetSize);
    for (uint32_t pkt = 0 ; pkt < queueData.size() ; pkt++)
    {
        if (queueData[pkt].payload.size() == 0)
            continue;
        double timestamp = (queueData[pkt].ts.tv_sec * 1000 + queueData[pkt].ts.tv_usec / 1000 - timestamp0) / 1000.0;
        uint8_t *mapper = (uint8_t*) queueData[pkt].payload.data();

        switch(type)
        {
        case DataType::e_float:
            switch(size)
            {
            case DataSize::e_32: {
                float tmp;
                memcpy(&tmp, mapper + offset, sizeof(float));
                if (toHostEndian) ntoh<float>(tmp);
                if (!std::isinf(tmp / ratio))
                    series->append(timestamp, tmp / ratio);
            }
            break;
            case DataSize::e_64: {
                double tmp;
                memcpy(&tmp, mapper + offset, sizeof(double));
                if (toHostEndian) ntoh<double>(tmp);
                if (!std::isinf(tmp / ratio))
                    series->append(timestamp, tmp / ratio);
            }
            break;
            default:
                break;
            }
            break;
        case DataType::e_int:
            switch(size)
            {
            case DataSize::e_8:
                series->append(timestamp, (int8_t)mapper[offset]);
                break;
            case DataSize::e_16: {
                int16_t tmp;
                memcpy(&tmp, mapper + offset, sizeof(int16_t));
                if (toHostEndian) tmp = ntoh<int16_t>(tmp);
                series->append(timestamp, ((tmp & mask) >> shift) / ratio);
            }
            break;
            case DataSize::e_32: {
                int32_t tmp;
                memcpy(&tmp, mapper + offset, sizeof(int32_t));
                if (toHostEndian) tmp = ntoh<int32_t>(tmp);
                series->append(timestamp, ((tmp & mask) >> shift) / ratio);
            }
            break;
            case DataSize::e_64: {
                int64_t tmp;
                memcpy(&tmp, mapper + offset, sizeof(int64_t));
                if (toHostEndian) tmp = ntoh<int64_t>(tmp);
                series->append(timestamp, ((tmp & mask) >> shift) / ratio);
            }
            break;
            default:
                break;
            }
            break;
        case DataType::e_uint:
            switch(size)
            {
            case DataSize::e_8:
                series->append(timestamp, (uint8_t)mapper[offset]);
                break;
            case DataSize::e_16: {
                uint16_t tmp;
                memcpy(&tmp, mapper + offset, sizeof(uint16_t));
                if (toHostEndian) tmp = ntoh<uint16_t>(tmp);
                series->append(timestamp, ((tmp & mask) >> shift) / ratio);
            }
            break;
            case DataSize::e_32: {
                uint32_t tmp;
                memcpy(&tmp, mapper + offset, sizeof(uint32_t));
                if (toHostEndian) tmp = ntoh<int32_t>(tmp);
                series->append(timestamp, ((tmp & mask) >> shift) / ratio);
            }
            break;
            case DataSize::e_64: {
                uint64_t tmp;
                memcpy(&tmp, mapper + offset, sizeof(uint64_t));
                if (toHostEndian) tmp = ntoh<uint64_t>(tmp);
                series->append(timestamp, ((tmp & mask) >> shift) / ratio);
            }
            break;
            default:
                break;
            }
            break;

        case DataType::e_string:
            break;
        default:
            break;
        }
    }

    return series;
}

template <typename T> std::vector<T> qLineSeriesXToStdVector(const QLineSeries& series)
{
    std::vector<T> result;

    const QList<QPointF>& points = series.points();
    result.reserve(points.size());
    for (const QPointF& point : points)
        result.emplace_back(point.x());

    return result;
}

template <typename T> std::vector<T> qLineSeriesYToStdVector(const QLineSeries& series)
{
    std::vector<T> result;

    const QList<QPointF>& points = series.points();
    result.reserve(points.size());
    for (const QPointF& point : points)
        result.emplace_back(point.y());

    return result;
}

bool DebugWindow::computeChartByCriteria(std::string name,
                                         uint32_t offset,
                                         DataSize size,
                                         DataType type,
                                         bool toHostEndian,
                                         uint64_t mask,
                                         uint8_t shift,
                                         double ratio,
                                         double diversityMin,
                                         double minVal,
                                         double maxVal,
                                         double amplitudeMin,
                                         double amplitudeMax)
{
    bool res = false;
    QLineSeries *series = SeriesFromOffset(offset, size, type, toHostEndian, mask, shift, ratio);

    std::vector<double> tmp = qLineSeriesYToStdVector<double>(*series);
    if (tmp.size() != 0)
    {
//        double e = entropy<double>(tmp);
        double d = diversity<double>(tmp);
        double max = *std::max_element(tmp.begin(), tmp.end());
        double min = *std::min_element(tmp.begin(), tmp.end());
        double amp = max - min;
        if (d >= diversityMin)
            if (min >= minVal)
                if (max <= maxVal)
                    if (amp <= amplitudeMax)
                        if (amp >= amplitudeMin)
                            res = true;
    }

    return res;
}

void DebugWindow::updateChart(std::string name)
{

    QChart *chart = new QChart();
    QString status = "";
    DataSize size;
    uint32_t offset;
    DataType type;
    bool toHostEndian;
    uint64_t mask;
    uint8_t shift;
    double ratio;

    std::vector<std::string> names = split(name, ',');
    for (auto currentName : names)
    {
        if (variables.find(currentName) == variables.end())
        {
            size = DataSize::e_64;
            offset = 0;
            type = DataType::e_float;
            toHostEndian = false;
            mask = 0xFFFFFFFFFFFFFFFF;
            shift = 0;
            ratio = 1.0;
        }
        else
        {
            varDef_t var = variables.at(currentName);
            offset = var.offset;
            size = var.size;
            type = var.type;
            toHostEndian = var.endian == DataEndian::e_host?true:false;
            mask = var.mask;
            shift = var.shift;
            ratio = var.ratio;
        }

        QLineSeries *series = SeriesFromOffset(offset, size, type, toHostEndian, mask, shift, ratio);
        chart->addSeries(series);

        std::vector<double> tmp = qLineSeriesYToStdVector<double>(*series);
        if (tmp.size() != 0)
        {
            double e = entropy<double>(tmp);
            double d = diversity<double>(tmp);
            double max = *std::max_element(tmp.begin(), tmp.end());
            double min = *std::min_element(tmp.begin(), tmp.end());

            status += "Entropy : " + QString::number(e) + " Diversity : " + QString::number(d) + " Min : " + QString::number(min) + " Max : " + QString::number(max);
        }
        else
            status = "Please select a variable or an offset";
    }
    //chart->legend()->hide();
    chart->createDefaultAxes();
    chart->setTitle(name.c_str());

    ui->chart->setChart(chart);
    ui->chart->adjustSize();
    ui->chart->repaint();
    if (names.size() == 1)
    {
        ui->variables->setCurrentText(name.c_str());
        ui->offset->setValue(offset);
        ui->size->setCurrentText(QString(intDataSize.at(size).c_str()));
        ui->type->setCurrentText(QString(intDataType.at(type).c_str()));
        ui->endian->setCheckState(toHostEndian?Qt::CheckState::Checked:Qt::CheckState::Unchecked);
        ui->mask->setText(QString::number(mask, 16));
        ui->shift->setValue(shift);
        ui->ratio->setValue(ratio);
    }
    ui->statusBar->showMessage(status);
}

void DebugWindow::on_variables_currentIndexChanged(int index)
{
    std::string name = ui->variables->currentText().toStdString();
    DataSize size = variables[name].size;
    uint32_t offset = variables[name].offset;
    DataType type = variables[name].type;
    bool toHostEndian = (variables[name].endian == DataEndian::e_host)?true:false;
    uint64_t mask = variables[name].mask;
    uint8_t shift = variables[name].shift;
    double ratio = variables[name].ratio;

    ui->variables->setCurrentText(name.c_str());
    ui->offset->setValue(offset);
    ui->size->setCurrentText(QString(intDataSize.at(size).c_str()));
    ui->type->setCurrentText(QString(intDataType.at(type).c_str()));
    ui->endian->setCheckState(toHostEndian?Qt::CheckState::Checked:Qt::CheckState::Unchecked);
    ui->mask->setText(QString::number(mask, 16));
    ui->shift->setValue(shift);
    ui->ratio->setValue(ratio);

    updateChart(name);
}

void DebugWindow::on_variables_globalCheckStateChanged(int index)
{
    std::string nameList = ui->variables->currentText().toStdString();
    updateChart(nameList);
}

void DebugWindow::on_offset_valueChanged(int offset)
{
    std::string name = ui->variables->currentText().toStdString();
    if (findByOffset(offset, variables).length() != 0)
    {
        name = findByOffset(offset, variables);
        updateChart(name);
    }
    else
    {
        name = "new ...";
        updateChart(name);
    }
}

void DebugWindow::on_type_currentTextChanged(const QString &typeStr)
{
    std::string name = ui->variables->currentText().toStdString();
    updateChart(name);
}


void DebugWindow::on_size_currentTextChanged(const QString &sizeStr)
{
    std::string name = ui->variables->currentText().toStdString();
    variables[name].size = stringDataSize.at(sizeStr.toStdString());

    updateChart(name);
}

void DebugWindow::on_mask_textChanged(const QString &mask)
{
    std::string name = ui->variables->currentText().toStdString();
    variables[name].mask = ui->mask->text().toULong(nullptr, 16);;

    updateChart(name);
}

void DebugWindow::on_shift_valueChanged(int shift)
{
    std::string name = ui->variables->currentText().toStdString();
    variables[name].shift = shift;

    updateChart(name);
}

void DebugWindow::on_ratio_valueChanged(double ratio)
{
    std::string name = ui->variables->currentText().toStdString();
    variables[name].ratio = ratio;

    updateChart(name);
}


void DebugWindow::on_applyButton_clicked()
{
    std::string name = ui->variables->currentText().toStdString();
    DataSize size = stringDataSize.at(ui->size->currentText().toStdString());
    DataType type = stringDataType.at(ui->type->currentText().toStdString());
    bool toHostEndian = (ui->endian->checkState() == Qt::CheckState::Checked)?true:false;
    uint32_t offset = ui->offset->text().toInt(nullptr, 16);
    uint64_t mask = ui->mask->text().toInt(nullptr, 16);
    uint8_t shift = ui->shift->value();
    double ratio = ui->ratio->value();

    QString out = "";

    out += "0x" + QString::number(offset, 16) + ",";
    out += intDataType.at(type) + ",";
    out += intDataSize.at(size) + ",";
    if (type != DataType::e_string)
    {
        out += QString(toHostEndian?"host":"network") + ",";
        out += "0x" + QString::number(mask, 16) + ",";
        out += QString::number(shift) + ",";
        out += QString::number(ratio);
    }
    configuration["Vars/" + name] = out.toStdString();
    saveConfiguration(configuration, "config.ini");
}


// void DebugWindow::on_variables_editTextChanged(const QString &name)
// {
//     DataSize size;
//     uint32_t offset;
//     DataType type;
//     Qt::CheckState endian;
//     bool toHostEndian;
//     uint64_t mask;
//     uint8_t shift;
//     double ratio;

//     if (variables.find(name.toStdString()) != variables.end())
//     {
//         size = variables[name.toStdString()].size;
//         offset = variables[name.toStdString()].offset;
//         type = variables[name.toStdString()].type;
//         toHostEndian = (variables[name.toStdString()].endian == DataEndian::e_host)?true:false;
//         mask = variables[name.toStdString()].mask;
//         shift = variables[name.toStdString()].shift;
//         ratio = variables[name.toStdString()].ratio;

//         updateChart(name.toStdString(), offset, size, type, toHostEndian, mask, shift, ratio);
//     }
// }

void DebugWindow::on_endian_toggled(bool checked)
{
    std::string name = ui->variables->currentText().toStdString();
    bool toHostEndian = checked;
    variables[name].endian = checked?DataEndian::e_host:DataEndian::e_network;

    updateChart(name);
}

void DebugWindow::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0)
        ui->chart->chart()->zoomIn();
    else
        ui->chart->chart()->zoomOut();

    QMainWindow::wheelEvent(event);
}

void DebugWindow::on_autoSearch_clicked()
{
    uint32_t currentOffset = ui->offset->value();

    double diversityMin = ui->diversityMin->value();
    double min = ui->minVal->value();
    double max = ui->maxVal->value();
    double ampMin = ui->ampMin->value();
    double ampMax = ui->ampMax->value();
    std::string name = "autoDetected";
    DataSize size = stringDataSize.at(ui->size->currentText().toStdString());
    DataType type = stringDataType.at(ui->type->currentText().toStdString());
    bool toHostEndian = (ui->endian->checkState() == Qt::CheckState::Checked)?true:false;
    uint64_t mask = ui->mask->text().toInt(nullptr, 16);
    uint8_t shift = ui->shift->value();
    double ratio = ui->ratio->value();

    bool found = false;
    while (!found)
    {
        currentOffset++;
        found = computeChartByCriteria(name, currentOffset, size, type, toHostEndian, mask, shift, ratio, diversityMin, min, max, ampMin, ampMax);
        if (currentOffset >= packetSize)
            break;
    }
    if (found)
        updateChart(name);
    else
        ui->statusBar->showMessage("No relevant candidate found.");
}

