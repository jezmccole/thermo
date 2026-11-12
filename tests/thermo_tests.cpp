#include <QDir>
#include <QJsonDocument>
#include <QSaveFile>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <cmath>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "backend.h"
#include "sensormanager.h"
#include "theme.h"
#include "translator.h"

class ThermoTests : public QObject {
    Q_OBJECT

    QTemporaryDir configDir;
    QTemporaryDir hwmonRoot;
    void writeReading(const QString &root, const QString &name, const QByteArray &value) {
        QVERIFY(QDir().mkpath(root + "/hwmon0"));
        QFile file(root + "/hwmon0/" + name);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(value), value.size());
    }
    void setupCpu(const QString &root) {
        writeReading(root, "name", "coretemp\n");
        writeReading(root, "temp1_label", "Package id 0\n");
        writeReading(root, "temp1_input", "50000\n");
    }
private slots:
    void initTestCase() {
        QVERIFY(configDir.isValid());
        QVERIFY(hwmonRoot.isValid());
        setupCpu(hwmonRoot.path());
        qputenv("XDG_CONFIG_HOME", configDir.path().toUtf8());
    }

    void init() {
        QDir(configDir.path()).removeRecursively();
        QVERIFY(QDir().mkpath(configDir.path()));
    }

    void snapshotsExcludeInvalidValuesAndRecover() {
        QTemporaryDir root;
        setupCpu(root.path());
        SensorManager sm(nullptr, root.path());
        QCOMPARE(sm.readSensor("cpu_average").value(), 50.0);
        writeReading(root.path(), "temp1_input", "70000\n");
        sm.scanSensors();
        QCOMPARE(sm.readSensor("cpu_average").value(), 70.0);
        writeReading(root.path(), "temp2_label", "Core 0\n");
        writeReading(root.path(), "temp2_input", "0\n");
        sm.scanSensors();
        QCOMPARE(sm.readSensor("cpu_average").value(), 35.0);
        for (const QByteArray &value : {QByteArray("bad"), QByteArray("nan"), QByteArray("inf"), QByteArray("-127000"), QByteArray("141000"), QByteArray("")}) {
            writeReading(root.path(), "temp1_input", value);
            sm.scanSensors();
            QVERIFY(!sm.readSensor("coretemp:Package id 0"));
            QCOMPARE(sm.readSensor("cpu_average").value(), 0.0);
            const auto json = sm.snapshotJson()["sensors"].toObject()["coretemp:Package id 0"].toObject();
            QVERIFY(json["tempC"].isNull());
            QCOMPARE(json["status"].toString(), QString("unavailable"));
            QCOMPARE(json["error"].toString(), QString("read_failed"));
        }
        writeReading(root.path(), "temp1_input", "65000\n");
        writeReading(root.path(), "temp1_fault", "1\n");
        sm.scanSensors();
        QVERIFY(!sm.readSensor("coretemp:Package id 0"));
        writeReading(root.path(), "temp1_fault", "0\n");
        sm.scanSensors();
        QCOMPARE(sm.readSensor("coretemp:Package id 0").value(), 65.0);
        QVERIFY(QFile::remove(root.path() + "/hwmon0/temp1_input"));
        QVERIFY(QFile::remove(root.path() + "/hwmon0/temp2_input"));
        sm.scanSensors();
        QVERIFY(!sm.readSensor("cpu_average"));
        QVERIFY(!sm.readSensor("all_average"));
        QCOMPARE(sm.sensorError("coretemp:Package id 0"), QString("missing"));
        QVERIFY(!sm.readSensor("unknown"));
    }

    void backendFlagsFailureMissingAndRecovery() {
        QTemporaryDir root;
        setupCpu(root.path());
        Backend backend(nullptr, root.path());
        QCOMPARE(backend.temperatureStatus(), QString("live"));
        QCOMPARE(backend.currentTemperature(), 50.0);
        QVERIFY(!backend.lastReadingAt().isEmpty());
        writeReading(root.path(), "temp1_input", "bad");
        backend.refreshSensors();
        QCOMPARE(backend.temperatureStatus(), QString("stale"));
        QVERIFY(backend.temperatureWarning().contains("failed"));
        QCOMPARE(backend.currentTemperature(), 50.0);
        QCOMPARE(backend.currentColor(), QColor("#94a3b8"));
        QVERIFY(QFile::remove(root.path() + "/hwmon0/temp1_input"));
        backend.refreshSensors();
        QVERIFY(backend.temperatureWarning().contains("missing"));
        backend.setSensorId("unknown");
        QCOMPARE(backend.temperatureStatus(), QString("unavailable"));
        QVERIFY(std::isnan(backend.currentTemperature()));
        QCOMPARE(backend.formattedTemperature(), QString("Unavailable"));
        QCOMPARE(backend.normalizedTemperature(), 0.0);
        QVERIFY(backend.lastReadingAt().isEmpty());
        backend.setSensorId("coretemp:Package id 0");
        QCOMPARE(backend.temperatureStatus(), QString("unavailable"));
        writeReading(root.path(), "temp1_input", "63000\n");
        backend.refreshSensors();
        QCOMPARE(backend.temperatureStatus(), QString("live"));
        QCOMPARE(backend.currentTemperature(), 63.0);
        QVERIFY(backend.temperatureWarning().isEmpty());
        backend.setUnit("F");
        QCOMPARE(backend.formattedTemperature(), QString("145.4°F"));
        // Rediscovery tolerates hwmon numbering changes.
        QVERIFY(QDir(root.path()).rename("hwmon0", "hwmon7"));
        backend.refreshSensors();
        QCOMPARE(backend.temperatureStatus(), QString("live"));
        QCOMPARE(backend.currentTemperature(), 63.0);
    }

    void emptyHardwareStartsUnavailable() {
        QTemporaryDir root;
        Backend backend(nullptr, root.path());
        QCOMPARE(backend.temperatureStatus(), QString("unavailable"));
        QVERIFY(backend.temperatureWarning().contains("No valid"));
        QVERIFY(std::isnan(backend.currentTemperature()));
        SensorManager sm(nullptr, root.path());
        const auto json = sm.snapshotJson()["sensors"].toObject();
        QVERIFY(json["cpu_average"].toObject()["tempC"].isNull());
        QCOMPARE(json["cpu_average"].toObject()["error"].toString(), QString("no_valid_sources"));
    }

    void railStaysFixedAndFlagsReadFailures() {
        QTemporaryDir root;
        setupCpu(root.path());
        Backend backend(nullptr, root.path());
        Theme theme(root.path());
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        engine.rootContext()->setContextProperty("theme", &theme);
        engine.load(QUrl("qrc:/DesktopThermometer.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto window = engine.rootObjects().first();
        auto rail = window->findChild<QObject*>("temperatureRail");
        auto fill = window->findChild<QObject*>("temperatureFill");
        QVERIFY(rail); QVERIFY(fill);
        QTRY_VERIFY(rail->property("x").toDouble() > 0);
        const auto x = rail->property("x");
        const auto width = rail->property("width");
        QVERIFY(fill->property("visible").toBool());
        auto heatMarker = window->findChild<QObject*>("heatRailMarker");
        auto fanMarker = window->findChild<QObject*>("fanRailMarker");
        QVERIFY(heatMarker); QVERIFY(fanMarker);
        QVERIFY(!heatMarker->property("visible").toBool());
        QVERIFY(!fanMarker->property("visible").toBool());
        backend.setFanRailEnabled(true);
        QCoreApplication::processEvents();
        QVERIFY(heatMarker->property("visible").toBool());
        QVERIFY(fanMarker->property("visible").toBool());
        QCOMPARE(window->property("topHitMargin").toInt(), 20);
        QCOMPARE(rail->property("x"), x);
        QVERIFY(window->findChild<QObject*>("fanRail")->property("visible").toBool());
        backend.setAppearanceMode("classic");
        QCOMPARE(rail->property("x"), x);
        window->setProperty("isHovered", true);
        writeReading(root.path(), "temp1_input", "bad");
        backend.refreshSensors();
        QCoreApplication::processEvents();
        QVERIFY(!fill->property("visible").toBool());
        QCOMPARE(rail->property("x"), x);
        QCOMPARE(rail->property("width"), width);
        writeReading(root.path(), "temp1_input", "60000\n");
        backend.refreshSensors();
        QVERIFY(fill->property("visible").toBool());
        QCOMPARE(rail->property("x"), x);
    }

    void compactDefaultsAndColorEndpoints() {
        Backend backend(nullptr, hwmonRoot.path());
        QCOMPARE(backend.barHeight(), 127);
        backend.setBarHeight(127);
        QCOMPARE(backend.barHeight(), 127);
        const double temperature = backend.currentTemperature();
        backend.setTemperatureRange(temperature, temperature + 100);
        QCOMPARE(backend.currentColor(), QColor("#38bdf8"));
        backend.setTemperatureRange(temperature - 100, temperature);
        QCOMPARE(backend.currentColor(), QColor("#dc2626"));
    }

    void fanSpeedsHandleLabelsZeroAndInvalidReadings() {
        QTemporaryDir dir;
        QVERIFY(QDir().mkpath(dir.path() + "/hwmon0"));
        auto write = [&](const QString &name, const QByteArray &value) {
            QFile file(dir.path() + "/hwmon0/" + name);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(value);
        };
        write("fan1_input", "3443\n");
        write("fan1_label", "Right Side  \n");
        write("fan2_input", "0\n");
        write("fan3_input", "bad\n");
        write("fan4_input", "-10\n");
        auto fans = SensorManager::readFanSpeeds(dir.path());
        QCOMPARE(fans.size(), 4);
        QCOMPARE(fans[0].toMap()["label"].toString(), QString("Right Side"));
        QCOMPARE(fans[0].toMap()["rpm"].toInt(), 3443);
        QCOMPARE(fans[1].toMap()["label"].toString(), QString("Fan 2"));
        QCOMPARE(fans[1].toMap()["rpm"].toInt(), 0);
        QCOMPARE(fans[2].toMap()["rpm"].toInt(), -1);
        QCOMPARE(fans[3].toMap()["rpm"].toInt(), -1);
        QVERIFY(SensorManager::readFanSpeeds(dir.path() + "/missing").isEmpty());
    }

    void optionalFanRailUsesRealMaximumAndPersists() {
        QTemporaryDir root;
        setupCpu(root.path());
        writeReading(root.path(), "fan1_input", "3000");
        writeReading(root.path(), "fan1_max", "6000");
        {
            Backend backend(nullptr, root.path());
            QVERIFY(!backend.fanRailEnabled());
            backend.setFanRailEnabled(true);
            QVERIFY(backend.fanLevelAvailable());
            QCOMPARE(backend.fanLevel(), 0.5);
            writeReading(root.path(), "fan1_input", "0");
            backend.refreshSensors();
            QVERIFY(backend.fanLevelAvailable());
            QCOMPARE(backend.fanLevel(), 0.0);
            writeReading(root.path(), "fan1_max", "bad");
            backend.refreshSensors();
            QVERIFY(!backend.fanLevelAvailable());
            QVERIFY(backend.fanRailDescription().contains("Maximum RPM unavailable"));
            writeReading(root.path(), "fan1_fault", "1");
            backend.refreshSensors();
            QCOMPARE(backend.fanSpeeds()[0].toMap()["rpm"].toInt(), -1);
            QVERIFY(backend.fanRailDescription().contains("Reading unavailable"));
        }
        Backend restored(nullptr, root.path());
        QVERIFY(restored.fanRailEnabled());
    }

    void sensorManagerFormatsAppleSmcLabels() {
        QCOMPARE(SensorManager::formatAppleSmcLabel("TC0P"), QString("Central Processor Proximity (TC0P)"));
        QCOMPARE(SensorManager::formatAppleSmcLabel("TC0C"), QString("CPU Core 0 (TC0C)"));
        QCOMPARE(SensorManager::formatAppleSmcLabel("TA0P"), QString("Ambient Proximity (TA0P)"));
        QCOMPARE(SensorManager::formatAppleSmcLabel("TM0P"), QString("Memory Proximity (TM0P)"));
    }

    void sensorManagerScansSensors() {
        SensorManager sm(nullptr, hwmonRoot.path());
        const auto sensors = sm.scanSensors();
        QVERIFY(!sensors.isEmpty());

        bool foundCpuAvg = false;
        for (const auto &s : sensors) {
            if (s.id == "cpu_average")
                foundCpuAvg = true;
        }
        QVERIFY(foundCpuAvg);
        QVERIFY(!sm.defaultSensorId().isEmpty());
    }

    void backendTemperatureNormalization() {
        Backend backend(nullptr, hwmonRoot.path());
        backend.setTemperatureRange(20.0, 100.0);
        QCOMPARE(backend.minTemperature(), 20.0);
        QCOMPARE(backend.maxTemperature(), 100.0);

        // Normalized value should always be within [0.0, 1.0]
        QVERIFY(backend.normalizedTemperature() >= 0.0);
        QVERIFY(backend.normalizedTemperature() <= 1.0);
    }

    void backendColorGradients() {
        Backend backend(nullptr, hwmonRoot.path());
        backend.setTemperatureRange(0.0, 100.0);

        // Every sensor reading produces a valid display color.
        QColor col = backend.currentColor();
        QVERIFY(col.isValid());
        QVERIFY(col.red() >= 0 && col.red() <= 255);
        QVERIFY(col.green() >= 0 && col.green() <= 255);
        QVERIFY(col.blue() >= 0 && col.blue() <= 255);
    }

    void backendAppearanceAndPositioning() {
        Backend backend(nullptr, hwmonRoot.path());
        backend.setIdleWidth(6);
        QCOMPARE(backend.idleWidth(), 6);

        backend.setBarHeight(420);
        QCOMPARE(backend.barHeight(), 420);

        backend.setRailY(180);
        QCOMPARE(backend.railY(), 180);
    }

    void themeTokensSwitchLiveAndKeepContrast() {
        QTemporaryDir root;
        QVERIFY(QDir().mkpath(root.path() + "/theme"));
        auto write = [&](QString name, QByteArray value) {
            QSaveFile file(root.path() + "/theme/" + name);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(value);
            QVERIFY(file.commit());
        };
        write("colors.toml", "background = \"#ffffff\"\nforeground = \"#fffffe\"\nmuted = \"#eeeeee\"\naccent = \"#ffffff\"\n");
        write("shell.toml", "[font]\nbase-size = 16\nfamily = \"Sans Serif\"\n");
        Theme theme(root.path());
        QCOMPARE(theme.tokens()["background"].toString(), QString("#ffffff"));
        QCOMPARE(theme.tokens()["foreground"].toString(), QString("#000000"));
        QCOMPARE(theme.tokens()["muted"].toString(), QString("#000000"));
        QCOMPARE(theme.tokens()["baseSize"].toInt(), 16);
        QCOMPARE(theme.tokens()["fontFamily"].toString(), QString("Sans Serif"));
        QVERIFY(theme.tokens()["light"].toBool());
        write("colors.toml", "background = \"#101010\"\nforeground = \"#eeeeee\"\naccent = \"#38bdf8\"\n");
        QTRY_COMPARE(theme.tokens()["background"].toString(), QString("#101010"));
        QCOMPARE(theme.tokens()["foreground"].toString(), QString("#eeeeee"));
        QVERIFY(!theme.tokens()["light"].toBool());
        write("shell.toml", "[popups]\nbackground = \"#202020\"\ntext = \"#fafafa\"\nborder = \"hyprland.active-border\"\n[hyprland]\nactive-border = \"rgba(38bdf8ff) rgba(ff0000ff)\"\n[font]\nbase-size = 999\n");
        QTRY_COMPARE(theme.tokens()["background"].toString(), QString("#202020"));
        QCOMPARE(theme.tokens()["border"].toString(), QString("#38bdf8"));
        QCOMPARE(theme.tokens()["baseSize"].toInt(), 18);
        // Replacement of the entire active theme must also be detected.
        QVERIFY(QDir(root.path()).rename("theme", "old-theme"));
        QVERIFY(QDir().mkpath(root.path() + "/theme"));
        write("colors.toml", "background = \"#f0f0f0\"\n");
        QTRY_COMPARE(theme.tokens()["background"].toString(), QString("#f0f0f0"));
        QVERIFY(QFile::remove(root.path() + "/theme/colors.toml"));
        QTRY_COMPARE(theme.tokens()["background"].toString(), QString("#0e1116"));
    }

    void appearancePreferencePersistsAndRejectsUnknownModes() {
        {
            Backend backend(nullptr, hwmonRoot.path());
            QCOMPARE(backend.appearanceMode(), QString("omarchy"));
            backend.setAppearanceMode("classic");
            backend.setAppearanceMode("unknown");
            QCOMPARE(backend.appearanceMode(), QString("classic"));
        }
        Backend restored(nullptr, hwmonRoot.path());
        QCOMPARE(restored.appearanceMode(), QString("classic"));
    }

    void themeFallsBack() {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(QColor(theme.accent()), QColor("#34D399"));
        QCOMPARE(theme.accentForeground(), QString("black"));
    }

    void translatorFallsBackToEnglish() {
        QTemporaryDir dir;
        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("en"));
        QVERIFY(translator.translate("Main", "Thermo").isNull());
    }
};

QTEST_MAIN(ThermoTests)
#include "thermo_tests.moc"
