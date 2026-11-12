#include <QCommandLineParser>
#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>
#include <iostream>
#include <unistd.h>

#include "backend.h"
#include "sensormanager.h"
#include "theme.h"
#include "translator.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("thermo");
    app.setApplicationVersion("0.1.0");
    app.setDesktopFileName("thermo");
    app.setWindowIcon(QIcon::fromTheme("thermo"));

    QCommandLineParser parser;
    parser.setApplicationDescription("Modern ambient desktop edge thermometer for MacBook");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption backgroundOpt(QStringList() << "b" << "background" << "autostart",
                                     "Run thermometer in background without showing settings");
    parser.addOption(backgroundOpt);

    QCommandLineOption settingsOpt(QStringList() << "s" << "settings",
                                   "Open settings window");
    parser.addOption(settingsOpt);

    QCommandLineOption jsonOpt(QStringList() << "j" << "json",
                               "Output current sensor readings as JSON and exit");
    parser.addOption(jsonOpt);

    parser.addPositionalArgument("action", "Action to perform: status, settings, quit");
    parser.process(app);

    const QStringList posArgs = parser.positionalArguments();
    const QString action = posArgs.isEmpty() ? QString() : posArgs.first().toLower();

    // Fast-path: standalone --json or status request without GUI
    if (parser.isSet(jsonOpt) || action == "status") {
        SensorManager sm;
        const auto sensors = sm.scanSensors();
        QJsonObject root;
        root["defaultSensor"] = sm.defaultSensorId();
        QJsonObject sensorObj;
        for (const auto &s : sensors) {
            QJsonObject item;
            item["label"] = s.label;
            item["driver"] = s.driver;
            item["tempC"] = s.currentTemp;
            item["isCpu"] = s.isCpu;
            sensorObj[s.id] = item;
        }
        root["sensors"] = sensorObj;
        std::cout << QJsonDocument(root).toJson(QJsonDocument::Indented).toStdString() << std::endl;
        return 0;
    }

    const QString socketName = QString("thermo-ipc-%1").arg(getuid());

    // Check if another instance is already running
    QLocalSocket clientSocket;
    clientSocket.connectToServer(socketName);
    if (clientSocket.waitForConnected(250)) {
        if (action == "quit") {
            clientSocket.write("QUIT\n");
            clientSocket.flush();
            clientSocket.waitForBytesWritten(500);
            return 0;
        }

        // Send show settings to existing instance
        clientSocket.write("SHOW_SETTINGS\n");
        clientSocket.flush();
        clientSocket.waitForBytesWritten(500);
        return 0;
    }

    // No existing instance; we are the primary server
    QLocalServer::removeServer(socketName);
    QLocalServer server;
    if (!server.listen(socketName)) {
        qWarning("Could not start local IPC server: %s", qPrintable(server.errorString()));
    }

    // App should not quit when the settings dialog is closed
    app.setQuitOnLastWindowClosed(false);
    QQuickStyle::setStyle("Material");

    Translator translator;
    app.installTranslator(&translator);

    Theme theme;
    Backend backend;

    // Connect IPC server
    QObject::connect(&server, &QLocalServer::newConnection, [&server, &backend, &app] {
        while (QLocalSocket *client = server.nextPendingConnection()) {
            QObject::connect(client, &QLocalSocket::readyRead, [client, &backend, &app] {
                const QString msg = QString::fromUtf8(client->readAll()).trimmed();
                if (msg == "SHOW_SETTINGS") {
                    backend.openSettings();
                } else if (msg == "QUIT") {
                    backend.quitApp();
                }
            });
        }
    });

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("backend", &backend);

    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;

    // Determine initial settings window visibility:
    // If started via launcher or with --settings, show settings window.
    // If started via --background / --autostart, keep settings closed.
    if (parser.isSet(settingsOpt) || action == "settings") {
        backend.openSettings();
    } else if (!parser.isSet(backgroundOpt)) {
        // First interactive launch opens settings so user can see it right away
        backend.openSettings();
    }

    return app.exec();
}
