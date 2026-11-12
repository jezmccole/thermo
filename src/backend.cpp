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

Backend::Backend(QObject *parent, const QString &hwmonRoot)
    : QObject(parent), m_sensorManager(nullptr, hwmonRoot), m_hwmonRoot(hwmonRoot),
      m_configPath(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/thermo/config.json") {

    m_sensors = m_sensorManager.scanSensors();
    loadConfig();

    if (m_sensorId.isEmpty()) {
        m_sensorId = m_sensorManager.defaultSensorId();
    }

    m_fanSpeeds = SensorManager::readFanSpeeds(m_hwmonRoot);

    // Initial temperature read
    pollTemperature();

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
    if (!std::isfinite(m_currentTemp)) return 0.0;
    if (m_maxTemp <= m_minTemp)
        return 0.5;
    double t = (m_currentTemp - m_minTemp) / (m_maxTemp - m_minTemp);
    return std::clamp(t, 0.0, 1.0);
}

QString Backend::formattedTemperature() const {
    if (!std::isfinite(m_currentTemp)) return tr("Unavailable");
    if (m_unit == "F") {
        double f = (m_currentTemp * 9.0 / 5.0) + 32.0;
        return QString("%1°F").arg(QString::number(f, 'f', 1));
    }
    return QString("%1°C").arg(QString::number(m_currentTemp, 'f', 1));
}

QColor Backend::currentColor() const {
    if (m_temperatureStatus != "live") return QColor("#94a3b8");
    return calculateGradientColor(normalizedTemperature());
}

QColor Backend::calculateGradientColor(double t) const {
    // Cold blue through a neutral midpoint to orange and hot red.
    t = std::clamp(t, 0.0, 1.0);

    struct ColorStop {
        double pos;
        int r, g, b;
    };

    static const ColorStop stops[] = {
        { 0.00, 56, 189, 248 },
        { 0.45, 148, 163, 184 },
        { 0.72, 234, 88, 12 },
        { 1.00, 220, 38, 38 }
    };

    for (int i = 0; i < 3; ++i) {
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

        map["status"] = s.currentTemp ? "live" : "unavailable";
        if (!s.currentTemp) {
            map["tempString"] = tr("Unavailable");
            map["temp"] = QVariant();
        } else {
            const double value = m_unit == "F" ? *s.currentTemp * 9 / 5 + 32 : *s.currentTemp;
            map["tempString"] = QString::number(value, 'f', 1) + (m_unit == "F" ? "°F" : "°C");
            map["temp"] = *s.currentTemp;
        }
        list.append(map);
    }
    return list;
}

QString Backend::temperatureWarning() const {
    if (m_temperatureStatus == "live") return {};
    QString warning = m_temperatureError == "missing" ? tr("Sensor missing")
        : m_temperatureError == "no_valid_sources" ? tr("No valid temperature sources")
        : tr("Sensor reading failed");
    if (m_temperatureStatus == "stale")
        warning += tr(" • Stale reading");
    return warning;
}

void Backend::pollTemperature() {
    const auto fans = SensorManager::readFanSpeeds(m_hwmonRoot);
    if (fans != m_fanSpeeds) {
        m_fanSpeeds = fans;
        emit fanSpeedsChanged();
    }
    m_sensors = m_sensorManager.scanSensors();
    // Preserve a disappeared selected channel in the selector, flagged unavailable.
    bool selectedFound = false;
    for (const auto &sensor : m_sensors) selectedFound |= sensor.id == m_sensorId;
    if (!selectedFound) {
        SensorInfo missing{};
        missing.id = m_sensorId;
        missing.label = m_sensorId + tr(" (missing)");
        m_sensors.append(missing);
    }
    emit sensorsUpdated();
    const auto reading = m_sensorManager.readSensor(m_sensorId);
    if (reading) {
        m_currentTemp = *reading;
        m_lastReadingAt = QDateTime::currentDateTimeUtc();
        m_temperatureStatus = "live";
        m_temperatureError.clear();
    } else {
        m_temperatureStatus = std::isfinite(m_currentTemp) ? "stale" : "unavailable";
        m_temperatureError = m_sensorManager.sensorError(m_sensorId);
    }
    emit temperatureChanged();
}

void Backend::refreshSensors() {
    pollTemperature();
}

void Backend::setSensorId(const QString &id) {
    if (m_sensorId != id) {
        m_sensorId = id;
        m_currentTemp = std::numeric_limits<double>::quiet_NaN();
        m_lastReadingAt = {};
        pollTemperature();
        emit sensorChanged();
        saveConfig();
    }
}

bool Backend::fanLevelAvailable() const {
    if (m_fanSpeeds.isEmpty()) return false;
    const auto fan = m_fanSpeeds.first().toMap();
    return fan["rpm"].toInt() >= 0 && fan["maxRpm"].toInt() > 0;
}

double Backend::fanLevel() const {
    if (!fanLevelAvailable()) return 0;
    const auto fan = m_fanSpeeds.first().toMap();
    return std::clamp(double(fan["rpm"].toInt()) / fan["maxRpm"].toInt(), 0.0, 1.0);
}

QString Backend::fanRailDescription() const {
    if (m_fanSpeeds.isEmpty()) return tr("Fan rail: no fan detected");
    const auto fan = m_fanSpeeds.first().toMap();
    if (fan["rpm"].toInt() < 0) return tr("Fan rail: %1 • Reading unavailable").arg(fan["label"].toString());
    if (fan["maxRpm"].toInt() <= 0) return tr("Fan rail: %1 • Maximum RPM unavailable").arg(fan["label"].toString());
    return tr("Fan rail: %1 • %2 / %3 RPM").arg(fan["label"].toString()).arg(fan["rpm"].toInt()).arg(fan["maxRpm"].toInt());
}

void Backend::setAppearanceMode(const QString &mode) {
    if ((mode != "omarchy" && mode != "classic") || mode == m_appearanceMode) return;
    m_appearanceMode = mode;
    emit appearanceChanged();
    saveConfig();
}

void Backend::setFanRailEnabled(bool enabled) {
    if (m_fanRailEnabled == enabled) return;
    m_fanRailEnabled = enabled;
    emit appearanceChanged();
    saveConfig();
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
        m_barHeight = std::clamp(h, 80, 750);
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
    QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/autostart";
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
    if (obj.value("appearanceMode").toString() == "classic") m_appearanceMode = "classic";
    m_fanRailEnabled = obj.value("fanRailEnabled").toBool(false);
    if (obj.contains("autostart"))
        m_autostart = obj.value("autostart").toBool();
    if (obj.contains("unit"))
        m_unit = obj.value("unit").toString();
    if (obj.contains("minTemp"))
        m_minTemp = obj.value("minTemp").toDouble(30.0);
    if (obj.contains("maxTemp"))
        m_maxTemp = obj.value("maxTemp").toDouble(100.0);
    if (obj.contains("idleWidth"))
        m_idleWidth = obj.value("idleWidth").toInt(6);
    if (obj.contains("expandedWidth"))
        m_expandedWidth = obj.value("expandedWidth").toInt(6);
    if (obj.contains("barHeight"))
        m_barHeight = obj.value("barHeight").toInt(127);
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
    obj["appearanceMode"] = m_appearanceMode;
    obj["fanRailEnabled"] = m_fanRailEnabled;
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

void Backend::setInputRegion(QWindow *window, const QRect &region) {
    if (window)
        window->setMask(QRegion(region));
}
