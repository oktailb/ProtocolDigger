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
    , ready(false)
    , offsetChange(false)
{
    extractVariablesFromConfiguration(configuration, variables);
    chart = new QChart();
    ui->setupUi(this);
    fillVariables(variables);
    timerId = startTimer(100);
    localFileMode = true;
    if (configuration.find("Input/file") == configuration.end())
        localFileMode = false;
    else
        if (configuration.at("Input/file").compare("") == 0)
            localFileMode = false;
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->setAnimationDuration(0);
    chart->createDefaultAxes();
    ui->chart->setChart(chart);
    timeWindowSize = std::stof(configuration.at("Input/timeWindowSize"));
    uint64_t tsSecRef = queue.data()[0]->ts.tv_sec;
    uint64_t tsSecMax = queue.data()[queue.data().size() - 1]->ts.tv_sec;
    uint64_t tsDuration = tsSecMax - tsSecRef;
    uint64_t tsDurationSec = tsDuration % 60;
    uint64_t tsDurationMin = ((tsDuration - tsDurationSec) / 60) % 60;
    uint64_t tsDurationHour = tsDurationMin / 60;

    ui->sliderFrom->setMaximum(tsDuration);
    ui->sliderTo->setMaximum(tsDuration);
    ui->sliderTo->setValue(tsDuration);
    ui->timeFrom->setTime(QTime(0, 0, 0, 0));
    ui->timeTo->setTime(QTime(tsDurationHour, tsDurationMin, tsDurationSec, 0));
    ready = true;
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

void DebugWindow::timerEvent(QTimerEvent *event)
{
    updateChart(ui->variables->currentText().toStdString());
    chart->createDefaultAxes();
    ui->chart->repaint();
}

void DebugWindow::SeriesFromOffset(uint32_t offset, uint32_t size, uint32_t len, DataType type, bool toHostEndian, uint64_t mask, uint8_t shift, double ratio)
{
    QLineSeries *serie = nullptr;
    if (series.find(offset) == series.end())
    {
        serie = new QLineSeries();
        series[offset] = serie;
        pktIndex[offset] = 1;
    }
    else
        serie = series[offset];

    uint64_t count = serie->count();
    double frequency = 100.0;
    if (count > 2)
    {
        double diff = (serie->at(count - 1).x() - serie->at(count - 2).x());
        frequency = 1.0 / diff;
    }
    if (timeWindowSize > 0.001) // this not an oscilloscope ...
        if (count > timeWindowSize * frequency)
        {
            uint64_t overlap = count - timeWindowSize * frequency;
            serie->removePoints(0, overlap);
        }
    queueData = queue.data();
    if (queueData.size() == 0)
        return;
    if (queueData[0] == nullptr)
        return;
    packetSize = queueData[0]->payload.size();
    if (offset > packetSize)
        return;
    ui->offset->setMaximum(packetSize);
    uint32_t pkt = pktIndex[offset];
    uint32_t qsize = queueData.size();
    if (pkt >= qsize)
        return;
    timeval timestamp0 = queueData[0]->ts;
    while (pkt < qsize)
    {
        PacketData * pdata = queueData[pkt];
        if (pdata->payload.size() == 0)
            continue;
        timeval timestamp1 = pdata->ts;
        timestamp = (double)timeval_diff(&timestamp1, &timestamp0) / 1000000.0f;
        uint8_t *mapper = (uint8_t*) pdata->payload.data();

        switch(type)
        {
        case DataType::e_float:
            switch(size)
            {
            case DataSize::e_32: {
                float tmp;
                memcpy(&tmp, mapper + offset, sizeof(float));
                if (toHostEndian) ntoh<float>(tmp);
                if (!std::isinf(tmp * ratio))
                    if (!std::isnan(tmp * ratio))
                        serie->append(timestamp, tmp * ratio);
            }
            break;
            case DataSize::e_64: {
                double tmp;
                memcpy(&tmp, mapper + offset, sizeof(double));
                if (toHostEndian) ntoh<double>(tmp);
                if (!std::isinf(tmp * ratio))
                    if (!std::isnan(tmp * ratio))
                        serie->append(timestamp, tmp * ratio);
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
                serie->append(timestamp, (int8_t)mapper[offset]);
                break;
            case DataSize::e_16: {
                int16_t tmp;
                memcpy(&tmp, mapper + offset, sizeof(int16_t));
                if (toHostEndian) tmp = ntoh<int16_t>(tmp);
                serie->append(timestamp, ((tmp & mask) >> shift) * ratio);
            }
            break;
            case DataSize::e_32: {
                int32_t tmp;
                memcpy(&tmp, mapper + offset, sizeof(int32_t));
                if (toHostEndian) tmp = ntoh<int32_t>(tmp);
                serie->append(timestamp, ((tmp & mask) >> shift) * ratio);
            }
            break;
            case DataSize::e_64: {
                int64_t tmp;
                memcpy(&tmp, mapper + offset, sizeof(int64_t));
                if (toHostEndian) tmp = ntoh<int64_t>(tmp);
                serie->append(timestamp, ((tmp & mask) >> shift) * ratio);
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
                serie->append(timestamp, (uint8_t)mapper[offset]);
                break;
            case DataSize::e_16: {
                uint16_t tmp;
                memcpy(&tmp, mapper + offset, sizeof(uint16_t));
                if (toHostEndian) tmp = ntoh<uint16_t>(tmp);
                serie->append(timestamp, ((tmp & mask) >> shift) * ratio);
            }
            break;
            case DataSize::e_32: {
                uint32_t tmp;
                memcpy(&tmp, mapper + offset, sizeof(uint32_t));
                if (toHostEndian) tmp = ntoh<int32_t>(tmp);
                serie->append(timestamp, ((tmp & mask) >> shift) * ratio);
            }
            break;
            case DataSize::e_64: {
                uint64_t tmp;
                memcpy(&tmp, mapper + offset, sizeof(uint64_t));
                if (toHostEndian) tmp = ntoh<uint64_t>(tmp);
                serie->append(timestamp, ((tmp & mask) >> shift) * ratio);
            }
            break;
            default:
                break;
            }
            break;

        case DataType::e_string:
            break;
        case DataType::e_sint:
        {
            uint64_t tmp = 0;
            char * tmpStr = (char *)malloc(len + 1);
            strncpy(tmpStr, (char *)mapper + offset, len);
            tmpStr[len] = '\0';
            try {
                tmp = std::stoi(tmpStr);
            } catch (...) {
                std::cerr << offset << " : " << tmpStr << " -> 0x";
                int i = 0;
                while (i < len)
                    std::cerr << std::hex << (uint8_t)(tmpStr[i++]);
                std::cerr << std::endl;
            }
            serie->append(timestamp, tmp);
            free (tmpStr);
            break;
        }
        default:
            break;
        }
        pkt++;
    }
    pktIndex[offset] = pkt;
}

template <typename T> std::vector<T> qLineSeriesXToStdVector(const QLineSeries& series, double from, double to)
{
    std::vector<T> result;

    const QList<QPointF>& points = series.points();
    result.reserve(points.size());
    for (const QPointF& point : points)
        if (point.x() >= from)
            if (point.x() <= to)
                result.emplace_back(point.x());

    return result;
}

template <typename T> std::vector<T> qLineSeriesYToStdVector(const QLineSeries& series, double from, double to)
{
    std::vector<T> result;

    const QList<QPointF>& points = series.points();
    result.reserve(points.size());
    for (const QPointF& point : points)
        if (point.x() >= from)
            if (point.x() <= to)
                result.emplace_back(point.y());

    return result;
}

bool DebugWindow::computeChartByCriteria(std::string name,
                                         uint32_t offset,
                                         DataSize size,
                                         uint32_t len,
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
    if (series.find(offset) == series.end())
        SeriesFromOffset(offset, size, len, type, toHostEndian, mask, shift, ratio);

    pktIndex[offset] = 0;
    std::vector<double> tmp = qLineSeriesYToStdVector<double>(*series[offset], ui->sliderFrom->value(), ui->sliderTo->value());
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

inline bool hasSerieInChart(QChart* chart, QLineSeries *serie)
{
    for (auto itSerie : chart->series())
        if (itSerie == serie)
            return true;
    return false;
}

inline bool isSerieVisibleInChart(QChart* chart, QLineSeries *serie)
{
    for (auto itSerie : chart->series())
        if (itSerie == serie)
            return itSerie->isVisible();
    return false;
}

inline void hideAllSeriesInChart(QChart* chart)
{
    for (auto itSerie : chart->series())
        itSerie->hide();
}

inline void hideDisabledSeriesInChart(QChart* chart, QCheckList *list)
{
    auto check = list->getCheckedText();
    for (auto itSerie : chart->series())
    {
        if (!check.contains(itSerie->name()))
            itSerie->hide();
    }
}

template <typename T> T average(std::vector<T> const& v)
{
    if(v.empty())
        return 0;

    auto const count = static_cast<float>(v.size());
    return std::reduce(v.begin(), v.end()) / count;
}

template <typename T> T averageDiff(std::vector<T> const& v)
{
    if(v.empty())
        return 0;
    if(v.size() == 1)
        return 0;
    T time0 = 0;
    T sumDiff = 0;
    for (auto time : v)
    {
        sumDiff += time - time0;
        time0 = time;
    }
    return (sumDiff / v.size());
}

void DebugWindow::updateChart(std::string name)
{
    if (!ready)
        return;
    hideDisabledSeriesInChart(chart, ui->variables);
    if (name.size() == 0)
        return;

    ready = false;
    QString status = "";

    DataSize size = DataSize::e_64;
    uint32_t len = 0;
    uint32_t offset = 0;
    DataType type = DataType::e_float;
    bool toHostEndian = false;
    uint64_t mask = 0xFFFFFFFFFFFFFFFF;
    uint8_t shift = 0;
    double ratio = 1.0;

    std::vector<std::string> names = split(name, ',');

    for (auto currentName : names)
    {
        if (variables.find(currentName) == variables.end())
        {
            name = ui->variables->currentText().toStdString();
            size = stringDataSize.at(ui->size->currentText().toStdString());
            len = ui->len->value();
            type = stringDataType.at(ui->type->currentText().toStdString());
            toHostEndian = (ui->endian->checkState() == Qt::CheckState::Checked)?true:false;
            offset = ui->offset->value();
            mask = ui->mask->text().toInt(nullptr, 16);
            shift = ui->shift->value();
            ratio = ui->ratio->value();
        }
        else
        {
            varDef_t var = variables.at(currentName);
            offset = var.offset;
            size = var.size;
            len = var.len;
            type = var.type;
            toHostEndian = var.endian == DataEndian::e_host?true:false;
            mask = var.mask;
            shift = var.shift;
            ratio = var.ratio;
        }

        SeriesFromOffset(offset, size, len, type, toHostEndian, mask, shift, ratio);
        QLineSeries *serie = series[offset];

        if (!hasSerieInChart(chart, serie))
        {
            chart->addSeries(serie);
            QString serieName = QString(currentName.c_str())
                                + " ("
                                + QString(intDataType.at(type).c_str())
                                + " "
                                + QString(intDataSize.at(size).c_str())
                                + ")";
            serie->setName(serieName);
        }

        if (!isSerieVisibleInChart(chart, serie))
            serie->show();

        if (names.size() == 1)
        {
            ready = false;
            if (offsetChange == false)
            {
                ui->offset->setValue(offset);
                ui->size->setCurrentText(intDataSize.at(size).c_str());
                ui->len->setValue(len);
                ui->type->setCurrentText(intDataType.at(type).c_str());
                ui->endian->setCheckState(toHostEndian?Qt::CheckState::Checked:Qt::CheckState::Unchecked);
                ui->mask->setText(QString::number(mask, 16));
                ui->shift->setValue(shift);
                ui->ratio->setValue(ratio);
            }
            ready = true;
            std::vector<double> tmp = qLineSeriesYToStdVector<double>(*serie, ui->sliderFrom->value(), ui->sliderTo->value());
            if (tmp.size() != 0)
            {
                double max = *std::max_element(tmp.begin(), tmp.end());
                double min = *std::min_element(tmp.begin(), tmp.end());
                // double e = entropy<double>(tmp);
                // double d = diversity<double>(tmp);
                if (offsetChange == true)
                    if (ui->chart->chart()->axes(Qt::Vertical).size())
                        ui->chart->chart()->axes(Qt::Vertical)[0]->setRange(min, max);

//                status += "Entropy : " + QString::number(e) + " Diversity : " + QString::number(d) + " Min : " + QString::number(min) + " Max : " + QString::number(max);
            }
            else
                status = "No data";
        }
        else
        {
            if (serie->count() > 1)
                status += currentName + ": " + QString::number(serie->at(serie->count() - 1).y(), 'g', 6).toStdString() + " ";
        }
    }

    if (names.size() == 1)
    {
        // ui->variables->setCurrentText(name.c_str());
        // ui->offset->setValue(offset);
        // ui->size->setCurrentText(QString(intDataSize.at(size).c_str()));
        // ui->type->setCurrentText(QString(intDataType.at(type).c_str()));
        // ui->endian->setCheckState(toHostEndian?Qt::CheckState::Checked:Qt::CheckState::Unchecked);
        // ui->mask->setText(QString::number(mask, 16));
        // ui->shift->setValue(shift);
        // ui->ratio->setValue(ratio);
    }
    else
        ui->statusBar->showMessage(status);
    if (timeWindowSize > 0.001) // this is not an oscilloscope ...
    {
        if (!ui->chart->chart()->axes(Qt::Horizontal).empty())
            ui->chart->chart()->axes(Qt::Horizontal)[0]->setRange((timestamp > timeWindowSize)?(timestamp - timeWindowSize):0.0, timestamp);
    }
    else
    {
        if (!ui->chart->chart()->axes(Qt::Horizontal).empty())
            ui->chart->chart()->axes(Qt::Horizontal)[0]->setRange(ui->sliderFrom->value(), ui->sliderTo->value());
    }

    ready = true;
}

void DebugWindow::on_variables_currentIndexChanged(int index)
{
    if (!ready)
        return;
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

    offsetChange = false;

    //updateChart(name);
}

void DebugWindow::on_variables_globalCheckStateChanged(int index)
{
    if (!ready)
        return;
    std::string nameList = ui->variables->currentText().toStdString();
    bool enable =  (ui->variables->getCheckedIndex().size() == 1);
    ui->ratio->setEnabled(enable);
    ui->mask->setEnabled(enable);
    ui->shift->setEnabled(enable);
    ui->type->setEnabled(enable);
    ui->endian->setEnabled(enable);
    ui->offset->setEnabled(enable);
    ui->len->setEnabled(enable);
    ui->size->setEnabled(enable);

    offsetChange = false;

    //updateChart(nameList);
}

void DebugWindow::on_offset_valueChanged(int offset)
{
    if (!ready)
        return;
    offsetChange = true;
    QString name = "unknown on 0x" + QString::number(offset, 16);
    if (findByOffset(offset, variables).length() != 0)
        name = QString(findByOffset(offset, variables).c_str());
    ui->variables->setCurrentText(name);
}

void DebugWindow::on_type_currentTextChanged(const QString &typeStr)
{
    if (!ready)
        return;
    std::string name = ui->variables->currentText().toStdString();
    variables[name].type = stringDataType.at(typeStr.toStdString());
}


void DebugWindow::on_size_currentTextChanged(const QString &sizeStr)
{
    if (!ready)
        return;
    std::string name = ui->variables->currentText().toStdString();
    variables[name].size = stringDataSize.at(sizeStr.toStdString());
}

void DebugWindow::on_mask_textChanged(const QString &mask)
{
    if (!ready)
        return;
    std::string name = ui->variables->currentText().toStdString();
    variables[name].mask = ui->mask->text().toULong(nullptr, 16);;
}

void DebugWindow::on_shift_valueChanged(int shift)
{
    if (!ready)
        return;
    std::string name = ui->variables->currentText().toStdString();
    variables[name].shift = shift;
}

void DebugWindow::on_ratio_valueChanged(double ratio)
{
    if (!ready)
        return;
    std::string name = ui->variables->currentText().toStdString();

    std::cerr << name << std::endl;
    std::cerr << ratio << std::endl;
    variables[name].ratio = ratio;
    uint32_t offset = variables[name].offset;
    ready = false;
    uint32_t last = series[offset]->count() - 1;
    series[offset]->removePoints(0, last);
    series[offset]->clear();
    delete series[offset];
    series[offset] = new QLineSeries();
    pktIndex[offset] = 1;
    ready = true;
}

void DebugWindow::saveFGFSGenericProtocol(const std::map<std::string, std::string>& keyValuePairs, const std::string& filename)
{
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cout << "Erreur : Impossible d'ouvrir le fichier " << filename << " pour sauvegarder la configuration." << std::endl;
        return;
    }
    file << "<?xml version=\"1.0\"?>\n";
    file << "<PropertyList>\n";
    file << "   <generic>\n";
    file << "      <input>\n";
    file << "      <binary_mode>true</binary_mode>\n";
    file << "      <binary_footer>length</binary_footer>\n";
    file << "      <byte_order>host</byte_order>\n";

    std::map<uint32_t, QStringList> base;
    for (const auto& entry : keyValuePairs)
    {
        QString qentryk = QString(entry.first.c_str());
        QString qentryv = QString(entry.second.c_str());

        // yaw=0x12,float,64,network,0xffffffffffffffff,0,1
        if (qentryk.startsWith("Vars/"))
        {
            QStringList id = qentryk.split(",");
            QStringList params = qentryv.split(",");
            uint32_t offset = params[ConfigFields::e_offset].toInt(nullptr,16);
            QString type = params[ConfigFields::e_type];
            if (type == "string")
                continue;
            if (type == "sint")
                continue;
            params[0] = id[0];
            base[offset] = params;
        }
    }
    int32_t currentOffset = 0;
    for (auto item : base)
    {
        int32_t offset = item.first;
        QStringList params = item.second;
        int32_t countInt = (offset - currentOffset) / 4;
        int32_t countBool = (offset - currentOffset) % 4;
        while (countInt > 0)
        {
            file << "           <chunk><name>0x" << std::hex << currentOffset << std::dec << "</name><type>int</type><node>/padding</node> </chunk>\n";
            currentOffset += 4;
            countInt--;
        }
        while (countBool > 0)
        {
            file << "           <chunk><name>0x" << std::hex << currentOffset++ << std::dec << "</name><type>bool</type><node>/padding</node> </chunk>\n";
            countBool--;
        }
        file << "           <chunk><!-- 0x" << std::hex << offset << std::dec << " -->\n";
        file << "               <name>" << params[0].toStdString() << "</name>\n";
        std::string type = params[ConfigFields::e_type].toStdString();
        int size = params[ConfigFields::e_size].toInt() / 8;
        if (type.compare("float") == 0)
        {
            if (size == 4)
                type = "float";
            else if (size == 8)
                type = "double";
            else
                throw;
        }
        if (type.compare("int") == 0)
        {
            if (size == 4)
                type = "int";
            else if (size == 1)
                type = "bool";
            else
                throw;
        }
        file << "               <type>" << type << "</type>\n";
        if (params.size() > ConfigFields::e_customStr)
        {
            file << "               <node>" << params[ConfigFields::e_customStr].toStdString() << "</node>\n";
            if (std::abs(params[ConfigFields::e_ratio].toDouble() - 1.0) > 0.001)
                file << "               <factor>" << params[ConfigFields::e_ratio].toStdString() << "</factor>\n";
        }
        file << "           </chunk>\n";
        currentOffset += size;
    }

    while (currentOffset < packetSize)
        file << "           <chunk><name>0x" << std::hex << currentOffset++ << std::dec << "</name><type>bool</type><node>/padding</node> </chunk>\n";
    file << "       </input>\n";
    file << "   </generic>\n";
    file << "</PropertyList>\n";

    file.close();
}


void DebugWindow::on_applyButton_clicked()
{
    if (!ready)
        return;
    std::string name = ui->variables->currentText().toStdString();
    DataSize size = stringDataSize.at(ui->size->currentText().toStdString());
    uint32_t len = ui->len->value();
    DataType type = stringDataType.at(ui->type->currentText().toStdString());
    bool toHostEndian = (ui->endian->checkState() == Qt::CheckState::Checked)?true:false;
    uint32_t offset = ui->offset->text().toInt(nullptr, 16);
    uint64_t mask = ui->mask->text().toInt(nullptr, 16);
    uint8_t shift = ui->shift->value();
    double ratio = ui->ratio->value();

    QString out = "";

    out += "0x" + QString::number(offset, 16) + ",";
    out += intDataType.at(type) + ",";
    switch (type)
    {
    case DataType::e_string:
    case DataType::e_sint:
    {
        out += QString::number(len);
        break;
    }
    default:
    {
        out += intDataSize.at(size) + ",";
        out += QString(toHostEndian?"host":"network") + ",";
        out += "0x" + QString::number(mask, 16) + ",";
        out += QString::number(shift) + ",";
        out += QString::number(ratio);
        break;
    }
    }

    configuration["Vars/" + name] = out.toStdString();
}

void DebugWindow::on_endian_toggled(bool checked)
{
    if (!ready)
        return;
    std::string name = ui->variables->currentText().toStdString();
    bool toHostEndian = checked;
    variables[name].endian = checked?DataEndian::e_host:DataEndian::e_network;

    //updateChart(name);
}

void DebugWindow::wheelEvent(QWheelEvent *event)
{
    double ratio = 1.1666;
    if (!ready)
        return;
    if (event->angleDelta().y() > 0)
        ui->chart->chart()->zoom(ratio);
    else
        ui->chart->chart()->zoom(1.0 / ratio);

    QMainWindow::wheelEvent(event);
}

void DebugWindow::on_autoSearch_clicked()
{
    if (!ready)
        return;
    uint32_t currentOffset = ui->offset->value();

    double diversityMin = ui->diversityMin->value();
    double min = ui->minVal->value();
    double max = ui->maxVal->value();
    double ampMin = ui->ampMin->value();
    double ampMax = ui->ampMax->value();
    std::string name = "autoDetected";
    DataSize size = stringDataSize.at(ui->size->currentText().toStdString());
    uint32_t len = ui->len->value();
    DataType type = stringDataType.at(ui->type->currentText().toStdString());
    bool toHostEndian = (ui->endian->checkState() == Qt::CheckState::Checked)?true:false;
    uint64_t mask = ui->mask->text().toInt(nullptr, 16);
    uint8_t shift = ui->shift->value();
    double ratio = ui->ratio->value();

    bool found = false;
    while (!found)
    {
        currentOffset++;
        ui->statusBar->showMessage("Offset " + QString::number(currentOffset, 16));
        found = computeChartByCriteria(name, currentOffset, size, len, type, toHostEndian, mask, shift, ratio, diversityMin, min, max, ampMin, ampMax);
        if (currentOffset >= packetSize)
            break;
    }
    if (found)
        updateChart(name);
    else
        ui->statusBar->showMessage("No relevant candidate found.");
}


void DebugWindow::on_len_valueChanged(int len)
{
    if (!ready)
        return;
    std::string name = ui->variables->currentText().toStdString();
    variables[name].len = len;
}

void DebugWindow::on_actionFix_chart_range_triggered()
{
    uint32_t currentOffset = ui->offset->value();
    std::vector<double> tmp = qLineSeriesYToStdVector<double>(*series[currentOffset], ui->sliderFrom->value(), ui->sliderTo->value());
    if (tmp.size() != 0)
    {
        double max = *std::max_element(tmp.begin(), tmp.end());
        double min = *std::min_element(tmp.begin(), tmp.end());
        ui->chart->chart()->axes(Qt::Vertical)[0]->setRange(min, max);
    }
}


void DebugWindow::on_actionSave_Configuration_triggered()
{
    saveConfiguration(configuration, "config.ini");
}


void DebugWindow::on_actionExport_triggered()
{
    saveFGFSGenericProtocol(configuration, "fgfs_invis_saved.xml");
}

void DebugWindow::on_sliderFrom_valueChanged(int value)
{
    uint32_t to = ui->sliderTo->value();
    if (to < value)
        ui->sliderTo->setValue(value);
    uint32_t hh = value / 3600;
    uint32_t mm = (value % 3600) / 60;
    uint32_t ss = (value % 3600) % 60;
    ui->timeFrom->setTime(QTime(hh, mm, ss, 0));
}


void DebugWindow::on_sliderTo_valueChanged(int value)
{
    uint32_t from = ui->sliderFrom->value();
    if (from > value)
        ui->sliderFrom->setValue(value);
    uint32_t hh = value / 3600;
    uint32_t mm = (value % 3600) / 60;
    uint32_t ss = (value % 3600) % 60;
    ui->timeTo->setTime(QTime(hh, mm, ss, 0));
}

