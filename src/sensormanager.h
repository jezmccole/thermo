#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QVariantMap>

struct SensorInfo {
    QString id;          // e.g. "applesmc:TC0P", "coretemp:Package id 0", "cpu_average"
    QString label;       // e.g. "Central Processor Proximity (TC0P)"
    QString driver;      // e.g. "applesmc", "coretemp"
    QString inputPath;   // path to sysfs temp*_input (empty for virtual)
    double currentTemp;  // in Celsius
    bool isCpu;
    bool isDefault;
};

class SensorManager : public QObject {
    Q_OBJECT

public:
    explicit SensorManager(QObject *parent = nullptr);

    QList<SensorInfo> scanSensors();
    double readSensor(const QString &id);
    double readRawPath(const QString &path);

    QString defaultSensorId() const;
    QString sensorLabel(const QString &id) const;

    static QString formatAppleSmcLabel(const QString &code);

private:
    double calculateCpuAverage();
    double calculateAllAverage();

    QList<SensorInfo> m_cachedSensors;
};
