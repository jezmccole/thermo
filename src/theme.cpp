#include "theme.h"

#include <QColor>
#include <QGuiApplication>
#include <QFont>
#include <QXmlStreamReader>
#include <algorithm>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <cmath>

namespace {
// Read only scalar theme data. Never evaluate theme-provided code.
QMap<QString, QString> readTokens(const QString &path) {
    QMap<QString, QString> values;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return values;
    QString section;
    const QRegularExpression assignment(R"re(^\s*([\w-]+)\s*=\s*(?:"([^"]*)"|'([^']*)'|([^#\s]+)))re");
    const QRegularExpression heading(R"(^\s*\[([\w-]+)\]\s*(?:#.*)?$)");
    for (const auto &line : QString::fromUtf8(file.readAll()).split('\n')) {
        const auto h = heading.match(line);
        if (h.hasMatch()) { section = h.captured(1) + "."; continue; }
        const auto a = assignment.match(line);
        if (a.hasMatch()) values[section + a.captured(1)] = !a.captured(2).isNull() ? a.captured(2) : !a.captured(3).isNull() ? a.captured(3) : a.captured(4);
    }
    return values;
}
QColor tokenColor(const QString &value, QColor fallback) {
    if (QRegularExpression("^#[0-9a-fA-F]{6}$").match(value).hasMatch()) return QColor(value);
    // Gradient borders use their first stop as a solid compatible outline.
    const auto match = QRegularExpression(R"(rgba\(([0-9a-fA-F]{6})[0-9a-fA-F]{2}\))").match(value);
    return match.hasMatch() ? QColor("#" + match.captured(1)) : fallback;
}
double luminance(QColor color) {
    auto linear=[](double x){return x <= .04045 ? x / 12.92 : std::pow((x + .055) / 1.055, 2.4);};
    return .2126*linear(color.redF()) + .7152*linear(color.greenF()) + .0722*linear(color.blueF());
}
QColor readable(QColor color, QColor background, double minimum = 4.5) {
    const double a=luminance(color), b=luminance(background);
    if ((std::max(a,b)+.05)/(std::min(a,b)+.05) >= minimum) return color;
    return (b+.05)/.05 >= 1.05/(b+.05) ? QColor("black") : QColor("white");
}
}

Theme::Theme(const QString &directory, QObject *parent, const QString &hyprctl)
    : QObject(parent),
      m_directory(directory.isEmpty() ? QDir::homePath() + "/.local/state/omarchy/current" : directory) {
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(80);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_debounce.start(); });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_debounce.start(); });
    connect(&m_debounce, &QTimer::timeout, this, &Theme::reload);

    m_hyprctl = !hyprctl.isEmpty() ? hyprctl
              : directory.isEmpty() && !qEnvironmentVariableIsEmpty("HYPRLAND_INSTANCE_SIGNATURE")
                  ? QStandardPaths::findExecutable("hyprctl") : QString();
    m_roundingTimeout.setSingleShot(true);
    m_roundingTimeout.setInterval(500);
    connect(&m_roundingTimeout, &QTimer::timeout, &m_roundingProcess, &QProcess::kill);
    connect(&m_roundingProcess, &QProcess::errorOccurred, this, [this] { m_roundingTimeout.stop(); });
    connect(&m_roundingProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        m_roundingTimeout.stop();
        const auto value = QJsonDocument::fromJson(m_roundingProcess.readAllStandardOutput()).object().value("int");
        if (code != 0 || status != QProcess::NormalExit || !value.isDouble() || value.toInt(-1) < 0)
            return;
        if (value.toInt() != m_radius) {
            m_radius = value.toInt();
            emit changed();
        }
    });
    m_roundingPoll.setInterval(1000);
    connect(&m_roundingPoll, &QTimer::timeout, this, &Theme::refreshRounding);
    if (!m_hyprctl.isEmpty())
        m_roundingPoll.start();
    reload();
}

Theme::~Theme() {
    if (m_roundingProcess.state() != QProcess::NotRunning) {
        m_roundingProcess.kill();
        m_roundingProcess.waitForFinished(1000);
    }
}

void Theme::refreshRounding() {
    if (m_hyprctl.isEmpty() || m_roundingProcess.state() != QProcess::NotRunning)
        return;
    m_roundingProcess.start(m_hyprctl, {"-j", "getoption", "decoration:rounding"});
    m_roundingTimeout.start();
}

QString Theme::readAccent(const QString &path) {
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        const QRegularExpression expression(R"re(^\s*accent\s*=\s*["'](#[0-9a-fA-F]{6})["'])re",
                                            QRegularExpression::MultilineOption);
        const auto match = expression.match(QString::fromUtf8(file.readAll()));
        if (match.hasMatch())
            return match.captured(1);
    }
    return "#34D399";
}

QString Theme::accentForeground() const {
    const QColor c(m_accent);
    auto linear = [](double v) { return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4); };
    const double luminance = .2126 * linear(c.redF()) + .7152 * linear(c.greenF()) + .0722 * linear(c.blueF());
    return luminance > .179 ? "black" : "white";
}

void Theme::reload() {
    refreshRounding();
    const auto paths = m_watcher.files() + m_watcher.directories();
    if (!paths.isEmpty())
        m_watcher.removePaths(paths);

    QString ancestor = m_directory;
    while (!QFileInfo::exists(ancestor) && ancestor != "/")
        ancestor = QFileInfo(ancestor).absolutePath();
    QStringList candidates{ancestor, QFileInfo(ancestor).absolutePath(), m_directory,
                           m_directory + "/theme", m_directory + "/theme/colors.toml", m_directory + "/theme/shell.toml",
                           QDir::homePath() + "/.config/fontconfig", QDir::homePath() + "/.config/fontconfig/fonts.conf"};
    candidates.removeDuplicates();
    for (const auto &path : candidates)
        if (QFileInfo::exists(path))
            m_watcher.addPath(path);

    const auto colors = readTokens(m_directory + "/theme/colors.toml");
    const auto shell = readTokens(m_directory + "/theme/shell.toml");
    const QColor background = tokenColor(shell.value("popups.background"), tokenColor(colors.value("background"), QColor("#0e1116")));
    const bool light = luminance(background) > .4;
    const QColor foreground = readable(tokenColor(shell.value("popups.text"), tokenColor(colors.value("foreground"), QColor("#f0f6fc"))), background);
    const QColor muted = readable(tokenColor(colors.value("muted"), QColor("#94a3b8")), background);
    const QColor accent = readable(tokenColor(colors.value("accent"), QColor("#34d399")), background, 3.0);
    QString borderValue = shell.value("popups.border");
    if (shell.contains(borderValue)) borderValue = shell.value(borderValue);
    const QColor border = readable(tokenColor(borderValue, tokenColor(colors.value("active_border_color"), accent)), background, 3.0);
    bool sizeOk = false;
    const int size = shell.value("font.base-size", "12").toInt(&sizeOk);
    QString family = QGuiApplication::font().family();
    QFile fonts(QDir::homePath() + "/.config/fontconfig/fonts.conf");
    if (fonts.open(QIODevice::ReadOnly)) {
        QXmlStreamReader xml(&fonts);
        bool preferred = false;
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement() && xml.name() == "prefer") preferred = true;
            if (xml.isEndElement() && xml.name() == "prefer") preferred = false;
            if (xml.isStartElement() && xml.name() == "family" && preferred) { family = xml.readElementText(); break; }
        }
    }
    const auto requestedFamily = shell.value("font.family").trimmed();
    if (!requestedFamily.isEmpty()) family = requestedFamily;
    const QColor panel = light ? background.darker(105) : background.lighter(135);
    const QVariantMap tokens{{"background", background.name()}, {"panel", panel.name()},
        {"foreground", foreground.name()}, {"muted", muted.name()}, {"accent", accent.name()},
        {"border", border.name()}, {"light", light}, {"baseSize", sizeOk ? std::clamp(size, 10, 18) : 12},
        {"fontFamily", family}};
    const bool changedTokens = tokens != m_tokens || accent.name() != m_accent;
    m_tokens = tokens;
    m_accent = accent.name();
    if (changedTokens) emit changed();
}
