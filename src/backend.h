#pragma once

#include <QColor>
#include <QDateTime>
#include <limits>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QWindow>
#include <QVariantList>

#include "sensormanager.h"

class Backend : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString temperatureStatus READ temperatureStatus NOTIFY temperatureChanged)
    Q_PROPERTY(QString temperatureWarning READ temperatureWarning NOTIFY temperatureChanged)
    Q_PROPERTY(QString lastReadingAt READ lastReadingAt NOTIFY temperatureChanged)
    Q_PROPERTY(double currentTemperature READ currentTemperature NOTIFY temperatureChanged)
    Q_PROPERTY(double minTemperature READ minTemperature NOTIFY rangeChanged)
    Q_PROPERTY(double maxTemperature READ maxTemperature NOTIFY rangeChanged)
    Q_PROPERTY(double normalizedTemperature READ normalizedTemperature NOTIFY temperatureChanged)
    Q_PROPERTY(QString formattedTemperature READ formattedTemperature NOTIFY temperatureChanged)
    Q_PROPERTY(QColor currentColor READ currentColor NOTIFY temperatureChanged)
    Q_PROPERTY(QColor outlineColor READ outlineColor CONSTANT)
    Q_PROPERTY(QColor coldColor READ coldColor CONSTANT)

    Q_PROPERTY(QString sensorId READ sensorId NOTIFY sensorChanged)
    Q_PROPERTY(QString sensorLabel READ sensorLabel NOTIFY sensorsUpdated)
    Q_PROPERTY(QVariantList availableSensors READ availableSensors NOTIFY sensorsUpdated)
    Q_PROPERTY(QVariantList fanSpeeds READ fanSpeeds NOTIFY fanSpeedsChanged)
    Q_PROPERTY(int sensorCount READ sensorCount NOTIFY sensorsUpdated)

    Q_PROPERTY(QString appearanceMode READ appearanceMode NOTIFY appearanceChanged)
    Q_PROPERTY(bool fanRailEnabled READ fanRailEnabled NOTIFY appearanceChanged)
    Q_PROPERTY(double fanLevel READ fanLevel NOTIFY fanSpeedsChanged)
    Q_PROPERTY(bool fanLevelAvailable READ fanLevelAvailable NOTIFY fanSpeedsChanged)
    Q_PROPERTY(QString fanRailDescription READ fanRailDescription NOTIFY fanSpeedsChanged)
    Q_PROPERTY(bool reducedMotion READ reducedMotion NOTIFY appearanceChanged)
    Q_PROPERTY(bool inspecting READ inspecting NOTIFY inspectingChanged)
    Q_PROPERTY(bool autostart READ autostart NOTIFY autostartChanged)
    Q_PROPERTY(QString unit READ unit NOTIFY unitChanged)
    Q_PROPERTY(int idleWidth READ idleWidth NOTIFY appearanceChanged)
    Q_PROPERTY(int expandedWidth READ expandedWidth NOTIFY appearanceChanged)
    Q_PROPERTY(int barHeight READ barHeight NOTIFY appearanceChanged)
    Q_PROPERTY(int railY READ railY NOTIFY positionChanged)
    Q_PROPERTY(QString edge READ edge NOTIFY placementChanged)
    Q_PROPERTY(QString screenName READ screenName NOTIFY placementChanged)
    Q_PROPERTY(QVariantList availableScreens READ availableScreens NOTIFY screensChanged)
    Q_PROPERTY(QString screenWarning READ screenWarning NOTIFY screensChanged)

    Q_PROPERTY(bool settingsVisible READ settingsVisible NOTIFY settingsVisibilityChanged)

public:
    explicit Backend(QObject *parent = nullptr, const QString &hwmonRoot = "/sys/class/hwmon");
    ~Backend() override;

    QString temperatureStatus() const { return m_temperatureStatus; }
    QString temperatureWarning() const;
    QString lastReadingAt() const { return m_lastReadingAt.toString(Qt::ISODate); }
    double currentTemperature() const { return m_currentTemp; }
    double minTemperature() const { return m_minTemp; }
    double maxTemperature() const { return m_maxTemp; }
    double normalizedTemperature() const;
    QString formattedTemperature() const;
    QColor currentColor() const;
    QColor outlineColor() const { return QColor(56, 189, 248); } // #38bdf8 Ice Cold Blue
    QColor coldColor() const { return QColor(56, 189, 248); }

    QString sensorId() const { return m_sensorId; }
    QString sensorLabel() const;
    QVariantList availableSensors() const;
    QVariantList fanSpeeds() const { return m_fanSpeeds; }
    int sensorCount() const { return m_sensors.size(); }

    QString appearanceMode() const { return m_appearanceMode; }
    bool fanRailEnabled() const { return m_fanRailEnabled; }
    double fanLevel() const;
    bool fanLevelAvailable() const;
    QString fanRailDescription() const;
    bool reducedMotion() const { return m_reducedMotion; }
    bool inspecting() const { return m_inspectTimer.isActive(); }
    bool autostart() const { return m_autostart; }
    QString unit() const { return m_unit; }
    int idleWidth() const { return m_idleWidth; }
    int expandedWidth() const { return m_expandedWidth; }
    int barHeight() const { return m_barHeight; }
    int railY() const { return m_railY; }
    QString edge() const { return m_edge; }
    QString screenName() const { return m_screenName; }
    QVariantList availableScreens() const;
    QString screenWarning() const;

    bool settingsVisible() const { return m_settingsVisible; }

    // Invokable actions from QML
    Q_INVOKABLE void setSensorId(const QString &id);
    Q_INVOKABLE void setAppearanceMode(const QString &mode);
    Q_INVOKABLE void setFanRailEnabled(bool enabled);
    Q_INVOKABLE void setAutostart(bool enabled);
    Q_INVOKABLE void setUnit(const QString &unit);
    Q_INVOKABLE void setRailY(int y);
    Q_INVOKABLE void setEdge(const QString &edge);
    Q_INVOKABLE void setScreenName(const QString &name);
    Q_INVOKABLE void configureDesktopWindow(QWindow *window);
    Q_INVOKABLE void setBarHeight(int h);
    Q_INVOKABLE void setIdleWidth(int w);
    Q_INVOKABLE void setTemperatureRange(double minT, double maxT);

    Q_INVOKABLE void setReducedMotion(bool enabled);
    Q_INVOKABLE void inspect();
    Q_INVOKABLE void openSettings();
    Q_INVOKABLE void closeSettings();
    Q_INVOKABLE void quitApp();
    Q_INVOKABLE void refreshSensors();
    Q_INVOKABLE void setInputRegion(QWindow *window, const QVariantList &regions);

signals:
    void temperatureChanged();
    void rangeChanged();
    void sensorChanged();
    void sensorsUpdated();
    void fanSpeedsChanged();
    void autostartChanged();
    void unitChanged();
    void appearanceChanged();
    void positionChanged();
    void placementChanged();
    void screensChanged();
    void settingsVisibilityChanged();
    void settingsRequested();
    void inspectingChanged();

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
    QVariantList m_fanSpeeds;
    QString m_sensorId;
    double m_currentTemp = std::numeric_limits<double>::quiet_NaN();
    QString m_temperatureStatus = "unavailable";
    QString m_temperatureError;
    QDateTime m_lastReadingAt;
    QString m_hwmonRoot;
    double m_minTemp = 30.0;
    double m_maxTemp = 100.0;

    QString m_appearanceMode = "omarchy";
    bool m_fanRailEnabled = false;
    bool m_reducedMotion = false;
    QTimer m_inspectTimer;
    bool m_autostart = true;
    QString m_unit = "C";
    int m_idleWidth = 6;
    int m_expandedWidth = 6;
    int m_barHeight = 127;
    int m_railY = 210;
    QString m_edge = "right";
    QString m_screenName;

    bool m_settingsVisible = false;
    QString m_configPath;
};
