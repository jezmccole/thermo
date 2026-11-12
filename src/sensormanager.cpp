#include "sensormanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <cmath>

SensorManager::SensorManager(QObject *parent)
    : QObject(parent) {
    scanSensors();
}

QString SensorManager::formatAppleSmcLabel(const QString &code) {
    if (code == "TC0P") return "Central Processor Proximity (TC0P)";
    if (code == "TC0C") return "CPU Core 0 (TC0C)";
    if (code == "TC1C") return "CPU Core 1 (TC1C)";
    if (code == "TC0E") return "CPU Core 0 Diode (TC0E)";
    if (code == "TC0F") return "CPU Core 0 Die (TC0F)";
    if (code == "TCGC") return "CPU Graphics (TCGC)";
    if (code == "TCSA") return "CPU System Agent (TCSA)";
    if (code == "TCXC") return "CPU PECI (TCXC)";
    if (code == "TA0P") return "Ambient Proximity (TA0P)";
    if (code == "TM0P") return "Memory Proximity (TM0P)";
    if (code == "TPCD") return "Platform Controller Hub (TPCD)";
    if (code == "Th1H") return "Heatpipe 1 (Th1H)";
    if (code == "Ts0P") return "Palm Rest Skin (Ts0P)";
    if (code == "Ts0S") return "Enclosure Skin 1 (Ts0S)";
    if (code == "Ts1S") return "Enclosure Skin 2 (Ts1S)";
    if (code == "TB0T") return "Battery Sensor 1 (TB0T)";
    if (code == "TB1T") return "Battery Sensor 2 (TB1T)";
    if (code == "TB2T") return "Battery Sensor 3 (TB2T)";
    if (code == "TBXT") return "Battery Overall (TBXT)";
    return QString("Apple SMC %1").arg(code);
}

double SensorManager::readRawPath(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return -999.0;

    QTextStream stream(&file);
    bool ok = false;
    double val = stream.readLine().trimmed().toDouble(&ok);
    if (!ok)
        return -999.0;

    // hwmon reports in millidegrees Celsius
    return val / 1000.0;
}

QList<SensorInfo> SensorManager::scanSensors() {
    QList<SensorInfo> list;

    // 1. Add virtual composite sensors at the top
    SensorInfo cpuAvg;
    cpuAvg.id = "cpu_average";
    cpuAvg.label = "Average CPU Temperature";
    cpuAvg.driver = "virtual";
    cpuAvg.isCpu = true;
    cpuAvg.isDefault = false;
    cpuAvg.currentTemp = 0.0;
    list.append(cpuAvg);

    SensorInfo allAvg;
    allAvg.id = "all_average";
    allAvg.label = "Average System Temperature";
    allAvg.driver = "virtual";
    allAvg.isCpu = false;
    allAvg.isDefault = false;
    allAvg.currentTemp = 0.0;
    list.append(allAvg);

    // 2. Scan /sys/class/hwmon
    QDir hwmonDir("/sys/class/hwmon");
    QStringList hwmons = hwmonDir.entryList(QStringList() << "hwmon*", QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    for (const QString &hwEntry : hwmons) {
        QString hwPath = hwmonDir.filePath(hwEntry);

        // Read driver name
        QString driverName = hwEntry;
        QFile nameFile(hwPath + "/name");
        if (nameFile.open(QIODevice::ReadOnly)) {
            driverName = QTextStream(&nameFile).readLine().trimmed();
        }

        QDir dir(hwPath);
        QStringList inputs = dir.entryList(QStringList() << "temp*_input", QDir::Files, QDir::Name);

        for (const QString &inFile : inputs) {
            QString inPath = dir.filePath(inFile);
            double temp = readRawPath(inPath);

            // Filter out disconnected sensors (-127°C, etc.)
            if (temp < -20.0 || temp > 140.0)
                continue;

            // Read label if available
            QString baseName = inFile;
            baseName.remove("_input");
            QString labelPath = dir.filePath(baseName + "_label");
            QString labelText;

            QFile lblFile(labelPath);
            if (lblFile.open(QIODevice::ReadOnly)) {
                labelText = QTextStream(&lblFile).readLine().trimmed();
            } else {
                labelText = baseName;
            }

            SensorInfo info;
            info.driver = driverName;
            info.inputPath = inPath;
            info.currentTemp = temp;

            if (driverName == "applesmc") {
                info.id = QString("applesmc:%1").arg(labelText);
                info.label = formatAppleSmcLabel(labelText);
                info.isCpu = labelText.startsWith("TC");
                info.isDefault = (labelText == "TC0P");
            } else if (driverName == "coretemp") {
                info.id = QString("coretemp:%1").arg(labelText);
                info.label = QString("CPU %1 (coretemp)").arg(labelText);
                info.isCpu = true;
                info.isDefault = false;
            } else {
                info.id = QString("%1:%2").arg(driverName, labelText);
                info.label = QString("%1 (%2)").arg(labelText, driverName);
                info.isCpu = labelText.contains("CPU", Qt::CaseInsensitive);
                info.isDefault = false;
            }

            list.append(info);
        }
    }

    // Sort: if TC0P exists, make sure it's prominently near the top right below averages
    // Update live temperatures
    m_cachedSensors = list;

    // Compute live values for the virtual sensors
    for (int i = 0; i < m_cachedSensors.size(); ++i) {
        if (m_cachedSensors[i].id == "cpu_average") {
            m_cachedSensors[i].currentTemp = calculateCpuAverage();
        } else if (m_cachedSensors[i].id == "all_average") {
            m_cachedSensors[i].currentTemp = calculateAllAverage();
        }
    }

    return m_cachedSensors;
}

double SensorManager::calculateCpuAverage() {
    double sum = 0.0;
    int count = 0;

    for (const SensorInfo &s : m_cachedSensors) {
        if (s.driver != "virtual" && s.isCpu && s.currentTemp > 0.0 && s.currentTemp < 130.0) {
            sum += s.currentTemp;
            count++;
        }
    }

    return count > 0 ? (sum / count) : 45.0;
}

double SensorManager::calculateAllAverage() {
    double sum = 0.0;
    int count = 0;

    for (const SensorInfo &s : m_cachedSensors) {
        if (s.driver != "virtual" && s.currentTemp > 0.0 && s.currentTemp < 130.0) {
            sum += s.currentTemp;
            count++;
        }
    }

    return count > 0 ? (sum / count) : 40.0;
}

double SensorManager::readSensor(const QString &id) {
    if (id == "cpu_average")
        return calculateCpuAverage();
    if (id == "all_average")
        return calculateAllAverage();

    for (const SensorInfo &s : m_cachedSensors) {
        if (s.id == id) {
            if (!s.inputPath.isEmpty()) {
                double val = readRawPath(s.inputPath);
                if (val > -20.0 && val < 140.0)
                    return val;
            }
            return s.currentTemp;
        }
    }

    // If sensor not cached, rescan
    scanSensors();
    for (const SensorInfo &s : m_cachedSensors) {
        if (s.id == id && !s.inputPath.isEmpty())
            return readRawPath(s.inputPath);
    }

    return 50.0;
}

QString SensorManager::defaultSensorId() const {
    // 1. First priority: Apple SMC Central Processor Proximity
    for (const SensorInfo &s : m_cachedSensors) {
        if (s.id == "applesmc:TC0P")
            return s.id;
    }

    // 2. Second priority: any sensor matching TC0P
    for (const SensorInfo &s : m_cachedSensors) {
        if (s.id.contains("TC0P"))
            return s.id;
    }

    // 3. Third priority: coretemp package
    for (const SensorInfo &s : m_cachedSensors) {
        if (s.id.contains("Package id 0"))
            return s.id;
    }

    // 4. Fourth priority: cpu_average
    return "cpu_average";
}

QString SensorManager::sensorLabel(const QString &id) const {
    for (const SensorInfo &s : m_cachedSensors) {
        if (s.id == id)
            return s.label;
    }
    return id;
}

QVariantList SensorManager::readFanSpeeds(const QString &hwmonRoot) {
    QVariantList fans;
    QDir root(hwmonRoot);
    for (const QString &entry : root.entryList({"hwmon*"}, QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        QDir device(root.filePath(entry));
        for (const QString &input : device.entryList({"fan*_input"}, QDir::Files, QDir::Name)) {
            QString base = input.chopped(6);
            QFile labelFile(device.filePath(base + "_label"));
            QString label;
            if (labelFile.open(QIODevice::ReadOnly))
                label = QString::fromUtf8(labelFile.readAll()).trimmed();
            if (label.isEmpty())
                label = QString("Fan %1").arg(base.mid(3));
            QFile inputFile(device.filePath(input));
            bool ok = false;
            int rpm = -1;
            if (inputFile.open(QIODevice::ReadOnly))
                rpm = QString::fromUtf8(inputFile.readAll()).trimmed().toInt(&ok);
            fans.append(QVariantMap{{"label", label}, {"rpm", ok && rpm >= 0 ? rpm : -1}});
        }
    }
    return fans;
}
