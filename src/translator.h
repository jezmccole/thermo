#pragma once

#include <QFileSystemWatcher>
#include <QHash>
#include <QJsonValue>
#include <QLocale>
#include <QObject>
#include <QTranslator>

class Translator : public QTranslator {
    Q_OBJECT
    Q_PROPERTY(QString language READ language NOTIFY changed)

public:
    explicit Translator(const QString &languageFile = {}, const QString &catalogDirectory = {}, QObject *parent = nullptr);

    QString language() const { return m_language; }
    QString translate(const char *context, const char *sourceText, const char *disambiguation = nullptr, int n = -1) const override;

    static QString readLanguage(const QString &path);
    static QString pluralCategory(const QString &language, int n);
    static QLocale formatLocale(const QString &language);

signals:
    void changed();

private:
    void reload();

    QString m_languageFile;
    QString m_catalogDirectory;
    QString m_language = "en";
    QHash<QString, QJsonValue> m_strings;
    QFileSystemWatcher m_watcher;
};
