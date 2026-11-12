#pragma once

#include <QObject>
#include <optional>
#include <QJsonObject>
#include <QString>
#include <QList>
#include <QVariantMap>
#include <QVariantList>

struct SensorInfo {
    QString id;          // e.g. "applesmc:TC0P", "coretemp:Package id 0", "cpu_average"
    QString label;       // e.g. "Central Processor Proximity (TC0P)"
    QString driver;      // e.g. "applesmc", "coretemp"
    QString inputPath;   // path to sysfs temp*_input (empty for virtual)
    std::optional<double> currentTemp;  // in Celsius
    bool isCpu;
    bool isDefault;
};

class SensorManager : public QObject {
    Q_OBJECT

public:
    explicit SensorManager(QObject *parent = nullptr, const QString &hwmonRoot = "/sys/class/hwmon");

    QList<SensorInfo> scanSensors();
    static QVariantList readFanSpeeds(const QString &hwmonRoot = "/sys/class/hwmon");
    QJsonObject snapshotJson() const;
    QString sensorError(const QString &id) const;
    std::optional<double> readSensor(const QString &id);
    std::optional<double> readRawPath(const QString &path);

    QString defaultSensorId() const;
    QString sensorLabel(const QString &id) const;

    static QString formatAppleSmcLabel(const QString &code);

private:
    std::optional<double> calculateAverage(bool cpuOnly) const;
    QString m_hwmonRoot;

    QList<SensorInfo> m_cachedSensors;
};
