#include <QDir>
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
private slots:
    void initTestCase() {
        QVERIFY(configDir.isValid());
        qputenv("XDG_CONFIG_HOME", configDir.path().toUtf8());
    }

    void compactDefaultsAndColorEndpoints() {
        Backend backend;
        QCOMPARE(backend.barHeight(), 127);
        backend.setBarHeight(127);
        QCOMPARE(backend.barHeight(), 127);
        const double temperature = backend.currentTemperature();
        backend.setTemperatureRange(temperature, temperature + 100);
        QCOMPARE(backend.currentColor(), QColor("#38bdf8"));
        backend.setTemperatureRange(temperature - 100, temperature);
        QCOMPARE(backend.currentColor(), QColor("#dc2626"));
    }

    void sensorManagerFormatsAppleSmcLabels() {
        QCOMPARE(SensorManager::formatAppleSmcLabel("TC0P"), QString("Central Processor Proximity (TC0P)"));
        QCOMPARE(SensorManager::formatAppleSmcLabel("TC0C"), QString("CPU Core 0 (TC0C)"));
        QCOMPARE(SensorManager::formatAppleSmcLabel("TA0P"), QString("Ambient Proximity (TA0P)"));
        QCOMPARE(SensorManager::formatAppleSmcLabel("TM0P"), QString("Memory Proximity (TM0P)"));
    }

    void sensorManagerScansSensors() {
        SensorManager sm;
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
        Backend backend;
        backend.setTemperatureRange(20.0, 100.0);
        QCOMPARE(backend.minTemperature(), 20.0);
        QCOMPARE(backend.maxTemperature(), 100.0);

        // Normalized value should always be within [0.0, 1.0]
        QVERIFY(backend.normalizedTemperature() >= 0.0);
        QVERIFY(backend.normalizedTemperature() <= 1.0);
    }

    void backendColorGradients() {
        Backend backend;
        backend.setTemperatureRange(0.0, 100.0);

        // Every sensor reading produces a valid display color.
        QColor col = backend.currentColor();
        QVERIFY(col.isValid());
        QVERIFY(col.red() >= 0 && col.red() <= 255);
        QVERIFY(col.green() >= 0 && col.green() <= 255);
        QVERIFY(col.blue() >= 0 && col.blue() <= 255);
    }

    void backendAppearanceAndPositioning() {
        Backend backend;
        backend.setIdleWidth(6);
        QCOMPARE(backend.idleWidth(), 6);

        backend.setBarHeight(420);
        QCOMPARE(backend.barHeight(), 420);

        backend.setRailY(180);
        QCOMPARE(backend.railY(), 180);
    }

    void themeFallsBack() {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#34D399"));
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
