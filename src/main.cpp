#include <QCommandLineParser>
#include <QDir>
#include <QElapsedTimer>
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

    parser.addPositionalArgument("action", "Action to perform: status, settings, inspect, close, toggle, quit");
    parser.process(app);

    const QStringList posArgs = parser.positionalArguments();
    const QString action = posArgs.isEmpty() ? QString() : posArgs.first().toLower();

    // Fast-path: standalone --json or status request without GUI
    if (parser.isSet(jsonOpt) || action == "status") {
        SensorManager sm;
        sm.scanSensors();
        std::cout << QJsonDocument(sm.snapshotJson()).toJson(QJsonDocument::Indented).toStdString() << std::endl;
        return 0;
    }

    const QString socketName = QDir::temp().filePath(QString("thermo-ipc-%1").arg(getuid()));

    // Check if another instance is already running
    QLocalSocket clientSocket;
    clientSocket.connectToServer(socketName);
    if (clientSocket.waitForConnected(250)) {
        if (action == "inspect") {
            clientSocket.write("INSPECT\n");
        } else if (action == "quit") {
            clientSocket.write("QUIT\n");
        } else if (action == "close") {
            clientSocket.write("CLOSE_SETTINGS\n");
        } else if (action == "toggle") {
            clientSocket.write("TOGGLE_SETTINGS\n");
        } else if (parser.isSet(backgroundOpt) || action == "background") {
            // Already running in background; exit quietly
            return 0;
        } else {
            // Send show settings to existing instance
            clientSocket.write("SHOW_SETTINGS\n");
        }
        clientSocket.flush();
        clientSocket.waitForBytesWritten(500);
        // Keep the connection alive until the server has received the command.
        QElapsedTimer acknowledgement;
        acknowledgement.start();
        while (!clientSocket.canReadLine()) {
            const int remaining = 1000 - acknowledgement.elapsed();
            if (remaining <= 0 || !clientSocket.waitForReadyRead(remaining)) return 1;
        }
        return clientSocket.readLine().trimmed() == "OK" ? 0 : 1;
    }

    // Control commands must not start a new application when none is running.
    if (action == "quit" || action == "close" || action == "toggle")
        return 0;

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
    QObject::connect(&server, &QLocalServer::newConnection, [&server, &backend] {
        while (QLocalSocket *client = server.nextPendingConnection()) {
            const auto dispatch = [client, &backend] {
                while (client->canReadLine()) {
                    const QString msg = QString::fromUtf8(client->readLine()).trimmed();
                    client->write("OK\n");
                    client->flush();
                    if (msg == "INSPECT") backend.inspect();
                    else if (msg == "SHOW_SETTINGS") backend.openSettings();
                    else if (msg == "CLOSE_SETTINGS") backend.closeSettings();
                    else if (msg == "TOGGLE_SETTINGS") {
                        if (backend.settingsVisible()) backend.closeSettings();
                        else backend.openSettings();
                    } else if (msg == "QUIT") backend.quitApp();
                }
            };
            QObject::connect(client, &QLocalSocket::readyRead, dispatch);
            QObject::connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
            // Data may already have arrived before readyRead was connected.
            dispatch();
            if (client->state() == QLocalSocket::UnconnectedState) client->deleteLater();
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
    if (action == "inspect") {
        backend.inspect();
    } else if (parser.isSet(settingsOpt) || action == "settings") {
        backend.openSettings();
    } else if (!parser.isSet(backgroundOpt)) {
        // First interactive launch opens settings so user can see it right away
        backend.openSettings();
    }

    return app.exec();
}
