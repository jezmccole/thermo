#pragma once

#include <QColor>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>

#include "sensormanager.h"

class Backend : public QObject {
    Q_OBJECT

    Q_PROPERTY(double currentTemperature READ currentTemperature NOTIFY temperatureChanged)
    Q_PROPERTY(double minTemperature READ minTemperature NOTIFY rangeChanged)
    Q_PROPERTY(double maxTemperature READ maxTemperature NOTIFY rangeChanged)
    Q_PROPERTY(double normalizedTemperature READ normalizedTemperature NOTIFY temperatureChanged)
    Q_PROPERTY(QString formattedTemperature READ formattedTemperature NOTIFY temperatureChanged)
    Q_PROPERTY(QColor currentColor READ currentColor NOTIFY temperatureChanged)

    Q_PROPERTY(QString sensorId READ sensorId NOTIFY sensorChanged)
    Q_PROPERTY(QString sensorLabel READ sensorLabel NOTIFY sensorChanged)
    Q_PROPERTY(QVariantList availableSensors READ availableSensors NOTIFY sensorsUpdated)
    Q_PROPERTY(int sensorCount READ sensorCount NOTIFY sensorsUpdated)

    Q_PROPERTY(bool autostart READ autostart NOTIFY autostartChanged)
    Q_PROPERTY(QString unit READ unit NOTIFY unitChanged)
    Q_PROPERTY(int idleWidth READ idleWidth NOTIFY appearanceChanged)
    Q_PROPERTY(int expandedWidth READ expandedWidth NOTIFY appearanceChanged)
    Q_PROPERTY(int barHeight READ barHeight NOTIFY appearanceChanged)
    Q_PROPERTY(int railY READ railY NOTIFY positionChanged)
    Q_PROPERTY(QString edge READ edge NOTIFY appearanceChanged)

    Q_PROPERTY(bool settingsVisible READ settingsVisible NOTIFY settingsVisibilityChanged)

public:
    explicit Backend(QObject *parent = nullptr);
    ~Backend() override;

    double currentTemperature() const { return m_currentTemp; }
    double minTemperature() const { return m_minTemp; }
    double maxTemperature() const { return m_maxTemp; }
    double normalizedTemperature() const;
    QString formattedTemperature() const;
    QColor currentColor() const;

    QString sensorId() const { return m_sensorId; }
    QString sensorLabel() const;
    QVariantList availableSensors() const;
    int sensorCount() const { return m_sensors.size(); }

    bool autostart() const { return m_autostart; }
    QString unit() const { return m_unit; }
    int idleWidth() const { return m_idleWidth; }
    int expandedWidth() const { return m_expandedWidth; }
    int barHeight() const { return m_barHeight; }
    int railY() const { return m_railY; }
    QString edge() const { return m_edge; }

    bool settingsVisible() const { return m_settingsVisible; }

    // Invokable actions from QML
    Q_INVOKABLE void setSensorId(const QString &id);
    Q_INVOKABLE void setAutostart(bool enabled);
    Q_INVOKABLE void setUnit(const QString &unit);
    Q_INVOKABLE void setRailY(int y);
    Q_INVOKABLE void setBarHeight(int h);
    Q_INVOKABLE void setIdleWidth(int w);
    Q_INVOKABLE void setTemperatureRange(double minT, double maxT);

    Q_INVOKABLE void openSettings();
    Q_INVOKABLE void closeSettings();
    Q_INVOKABLE void quitApp();
    Q_INVOKABLE void refreshSensors();

signals:
    void temperatureChanged();
    void rangeChanged();
    void sensorChanged();
    void sensorsUpdated();
    void autostartChanged();
    void unitChanged();
    void appearanceChanged();
    void positionChanged();
    void settingsVisibilityChanged();

private slots:
    void pollTemperature();

private:
    void loadConfig();
    void saveConfig();
    void syncAutostartDesktopFile();
    QColor calculateGradientColor(double t) const;

    SensorManager m_sensorManager;
    QTimer m_pollTimer;

    QList<SensorInfo> m_sensors;
    QString m_sensorId;
    double m_currentTemp = 45.0; // In Celsius
    double m_minTemp = 30.0;
    double m_maxTemp = 100.0;

    bool m_autostart = true;
    QString m_unit = "C";
    int m_idleWidth = 5;
    int m_expandedWidth = 12;
    int m_barHeight = 380;
    int m_railY = 210;
    QString m_edge = "right";

    bool m_settingsVisible = false;
    QString m_configPath;
};
