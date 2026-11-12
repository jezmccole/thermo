#include "backend.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <algorithm>
#include <cmath>

Backend::Backend(QObject *parent)
    : QObject(parent),
      m_configPath(QDir::homePath() + "/.config/thermo/config.json") {

    m_sensors = m_sensorManager.scanSensors();
    loadConfig();

    if (m_sensorId.isEmpty()) {
        m_sensorId = m_sensorManager.defaultSensorId();
    }

    // Initial temperature read
    m_currentTemp = m_sensorManager.readSensor(m_sensorId);

    // Initial autostart setup
    syncAutostartDesktopFile();

    // Set up polling timer (1.5 seconds)
    connect(&m_pollTimer, &QTimer::timeout, this, &Backend::pollTemperature);
    m_pollTimer.setInterval(1500);
    m_pollTimer.start();
}

Backend::~Backend() {
    saveConfig();
}

double Backend::normalizedTemperature() const {
    if (m_maxTemp <= m_minTemp)
        return 0.5;
    double t = (m_currentTemp - m_minTemp) / (m_maxTemp - m_minTemp);
    return std::clamp(t, 0.0, 1.0);
}

QString Backend::formattedTemperature() const {
    if (m_unit == "F") {
        double f = (m_currentTemp * 9.0 / 5.0) + 32.0;
        return QString("%1°F").arg(QString::number(f, 'f', 1));
    }
    return QString("%1°C").arg(QString::number(m_currentTemp, 'f', 1));
}

QColor Backend::currentColor() const {
    return calculateGradientColor(normalizedTemperature());
}

QColor Backend::calculateGradientColor(double t) const {
    // Heat gradient:
    // 0.0 -> Cool Emerald Green rgb(16, 185, 129) #10B981
    // 0.4 -> Lime / Chartreuse rgb(132, 204, 22) #84CC16
    // 0.65 -> Warm Burnt Amber / Copper rgb(245, 158, 11) #F59E0B
    // 0.85 -> Burnt Vermilion rgb(234, 88, 12) #EA580C
    // 1.0 -> Deep Crimson Red rgb(220, 38, 38) #DC2626

    t = std::clamp(t, 0.0, 1.0);

    struct ColorStop {
        double pos;
        int r, g, b;
    };

    static const ColorStop stops[] = {
        { 0.00,  16, 185, 129 }, // Emerald green
        { 0.35, 132, 204,  22 }, // Lime green
        { 0.60, 245, 158,  11 }, // Burnt amber
        { 0.82, 234,  88,  12 }, // Burnt orange-red
        { 1.00, 220,  38,  38 }  // Crimson red
    };

    for (int i = 0; i < 4; ++i) {
        if (t >= stops[i].pos && t <= stops[i + 1].pos) {
            double range = stops[i + 1].pos - stops[i].pos;
            double factor = (range > 0.0) ? (t - stops[i].pos) / range : 0.0;

            int r = std::round(stops[i].r + factor * (stops[i + 1].r - stops[i].r));
            int g = std::round(stops[i].g + factor * (stops[i + 1].g - stops[i].g));
            int b = std::round(stops[i].b + factor * (stops[i + 1].b - stops[i].b));

            return QColor(std::clamp(r, 0, 255), std::clamp(g, 0, 255), std::clamp(b, 0, 255));
        }
    }

    return QColor(220, 38, 38);
}

QString Backend::sensorLabel() const {
    return m_sensorManager.sensorLabel(m_sensorId);
}

QVariantList Backend::availableSensors() const {
    QVariantList list;
    for (const SensorInfo &s : m_sensors) {
        QVariantMap map;
        map["id"] = s.id;
        map["label"] = s.label;
        map["driver"] = s.driver;
        map["isCpu"] = s.isCpu;
        map["isDefault"] = s.isDefault;

        if (m_unit == "F") {
            double f = (s.currentTemp * 9.0 / 5.0) + 32.0;
            map["tempString"] = QString("%1°F").arg(QString::number(f, 'f', 1));
        } else {
            map["tempString"] = QString("%1°C").arg(QString::number(s.currentTemp, 'f', 1));
        }

        map["temp"] = s.currentTemp;
        list.append(map);
    }
    return list;
}

void Backend::pollTemperature() {
    double newTemp = m_sensorManager.readSensor(m_sensorId);
    if (std::abs(newTemp - m_currentTemp) > 0.05) {
        m_currentTemp = newTemp;
        emit temperatureChanged();
    }
}

void Backend::refreshSensors() {
    m_sensors = m_sensorManager.scanSensors();
    emit sensorsUpdated();
    pollTemperature();
}

void Backend::setSensorId(const QString &id) {
    if (m_sensorId != id) {
        m_sensorId = id;
        m_currentTemp = m_sensorManager.readSensor(m_sensorId);
        emit sensorChanged();
        emit temperatureChanged();
        saveConfig();
    }
}

void Backend::setAutostart(bool enabled) {
    if (m_autostart != enabled) {
        m_autostart = enabled;
        syncAutostartDesktopFile();
        emit autostartChanged();
        saveConfig();
    }
}

void Backend::setUnit(const QString &unit) {
    if (m_unit != unit) {
        m_unit = unit;
        emit unitChanged();
        emit temperatureChanged();
        emit sensorsUpdated();
        saveConfig();
    }
}

void Backend::setRailY(int y) {
    if (m_railY != y) {
        m_railY = y;
        emit positionChanged();
        saveConfig();
    }
}

void Backend::setBarHeight(int h) {
    if (m_barHeight != h) {
        m_barHeight = std::clamp(h, 150, 750);
        emit appearanceChanged();
        saveConfig();
    }
}

void Backend::setIdleWidth(int w) {
    if (m_idleWidth != w) {
        m_idleWidth = std::clamp(w, 2, 12);
        emit appearanceChanged();
        saveConfig();
    }
}

void Backend::setTemperatureRange(double minT, double maxT) {
    if (maxT > minT) {
        m_minTemp = minT;
        m_maxTemp = maxT;
        emit rangeChanged();
        emit temperatureChanged();
        saveConfig();
    }
}

void Backend::openSettings() {
    if (!m_settingsVisible) {
        m_settingsVisible = true;
        refreshSensors();
        emit settingsVisibilityChanged();
    }
}

void Backend::closeSettings() {
    if (m_settingsVisible) {
        m_settingsVisible = false;
        emit settingsVisibilityChanged();
    }
}

void Backend::quitApp() {
    saveConfig();
    QCoreApplication::quit();
}

void Backend::syncAutostartDesktopFile() {
    QString autostartDir = QDir::homePath() + "/.config/autostart";
    QString desktopPath = autostartDir + "/thermo.desktop";

    if (m_autostart) {
        QDir().mkpath(autostartDir);
        QFile file(desktopPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n"
                << "Type=Application\n"
                << "Name=Thermo\n"
                << "Comment=Desktop Edge Thermometer\n"
                << "Exec=thermo --background\n"
                << "Icon=thermo\n"
                << "Terminal=false\n"
                << "Categories=Utility;\n"
                << "X-GNOME-Autostart-enabled=true\n";
        }
    } else {
        if (QFile::exists(desktopPath)) {
            QFile::remove(desktopPath);
        }
    }
}

void Backend::loadConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    if (obj.contains("sensorId"))
        m_sensorId = obj.value("sensorId").toString();
    if (obj.contains("autostart"))
        m_autostart = obj.value("autostart").toBool();
    if (obj.contains("unit"))
        m_unit = obj.value("unit").toString();
    if (obj.contains("minTemp"))
        m_minTemp = obj.value("minTemp").toDouble(30.0);
    if (obj.contains("maxTemp"))
        m_maxTemp = obj.value("maxTemp").toDouble(100.0);
    if (obj.contains("idleWidth"))
        m_idleWidth = obj.value("idleWidth").toInt(5);
    if (obj.contains("expandedWidth"))
        m_expandedWidth = obj.value("expandedWidth").toInt(12);
    if (obj.contains("barHeight"))
        m_barHeight = obj.value("barHeight").toInt(380);
    if (obj.contains("railY"))
        m_railY = obj.value("railY").toInt(210);
    if (obj.contains("edge"))
        m_edge = obj.value("edge").toString("right");
}

void Backend::saveConfig() {
    QFileInfo info(m_configPath);
    QDir().mkpath(info.dir().absolutePath());

    QJsonObject obj;
    obj["sensorId"] = m_sensorId;
    obj["autostart"] = m_autostart;
    obj["unit"] = m_unit;
    obj["minTemp"] = m_minTemp;
    obj["maxTemp"] = m_maxTemp;
    obj["idleWidth"] = m_idleWidth;
    obj["expandedWidth"] = m_expandedWidth;
    obj["barHeight"] = m_barHeight;
    obj["railY"] = m_railY;
    obj["edge"] = m_edge;

    QFile file(m_configPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    }
}
