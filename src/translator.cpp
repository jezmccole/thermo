#include "translator.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>

Translator::Translator(const QString &languageFile, const QString &catalogDirectory, QObject *parent)
    : QTranslator(parent),
      m_languageFile(languageFile.isEmpty() ? QDir::homePath() + "/.config/omarchy/language" : languageFile),
      m_catalogDirectory(catalogDirectory.isEmpty() ? ":/i18n" : catalogDirectory) {
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &Translator::reload);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &Translator::reload);
    reload();
}

QString Translator::readLanguage(const QString &path) {
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (!line.isEmpty())
            return line;
    }
    return "en";
}

QString Translator::pluralCategory(const QString &language, int n) {
    const QString code = language.section('-', 0, 0);
    if (code == "ja" || code == "zh" || code == "ko" || code == "vi" || code == "th") return "other";
    if (code == "en" || code == "de" || code == "nl" || code == "sv" || code == "da" || code == "no")
        return n == 1 ? "one" : "other";
    if (code == "fr" || code == "pt") return n <= 1 ? "one" : "other";
    return n == 1 ? "one" : "other";
}

QLocale Translator::formatLocale(const QString &language) {
    QFile registry(qEnvironmentVariable("OMARCHY_PATH") + "/default/i18n/locales.json");
    if (registry.open(QIODevice::ReadOnly)) {
        const QString name = QJsonDocument::fromJson(registry.readAll()).object()
                                 .value(language).toObject().value("formatLocale").toString();
        if (!name.isEmpty())
            return QLocale(name);
    }
    return QLocale(language);
}

QString Translator::translate(const char *, const char *sourceText, const char *, int n) const {
    const QJsonValue value = m_strings.value(QString::fromUtf8(sourceText));
    if (value.isString())
        return value.toString();
    if (value.isObject()) {
        const QJsonObject forms = value.toObject();
        const QString form = forms.value(n < 0 ? "other" : pluralCategory(m_language, n)).toString();
        return form.isEmpty() ? forms.value("other").toString() : form;
    }
    return {};
}

void Translator::reload() {
    const auto paths = m_watcher.files() + m_watcher.directories();
    if (!paths.isEmpty())
        m_watcher.removePaths(paths);

    const QString directory = QFileInfo(m_languageFile).absolutePath();
    if (QFileInfo::exists(directory))
        m_watcher.addPath(directory);
    if (QFileInfo::exists(m_languageFile))
        m_watcher.addPath(m_languageFile);

    const QString language = readLanguage(m_languageFile);
    if (language == m_language && !m_strings.isEmpty())
        return;

    QHash<QString, QJsonValue> strings;
    QStringList codes{"en", language.section('-', 0, 0), language};
    codes.removeDuplicates();
    for (const auto &code : codes) {
        QFile catalog(m_catalogDirectory + "/" + code + ".json");
        if (!catalog.open(QIODevice::ReadOnly))
            continue;
        const QJsonObject object = QJsonDocument::fromJson(catalog.readAll()).object();
        for (auto it = object.begin(); it != object.end(); ++it)
            if ((it.value().isString() && !it.value().toString().isEmpty()) || it.value().isObject())
                strings.insert(it.key(), it.value());
    }

    m_language = language;
    m_strings = strings;

    const QLocale locale = formatLocale(language);
    QLocale::setDefault(locale);
    QGuiApplication::setLayoutDirection(locale.textDirection());
    emit changed();
}
