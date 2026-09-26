#include "translationmanager.h"

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>

namespace {

const char *kLanguageKey = "ui/language";

} // namespace

TranslationManager &TranslationManager::instance()
{
    static TranslationManager manager;
    return manager;
}

bool TranslationManager::setLanguage(const QString &code)
{
    if (code == m_current)
        return true;

    if (code == sourceLanguage()) {
        uninstallAll();
        m_current = code;
        return true;
    }

    QTranslator candidate;
    if (!candidate.load(QStringLiteral("markly_%1").arg(code), QStringLiteral(":/i18n")))
        return false;

    uninstallAll();

    m_appTranslator.load(QStringLiteral("markly_%1").arg(code), QStringLiteral(":/i18n"));
    QCoreApplication::installTranslator(&m_appTranslator);

    // Qt's own catalog is optional: without it the app is still translated,
    // only the standard dialog buttons stay in English.
    if (m_qtTranslator.load(QStringLiteral("qtbase_%1").arg(code),
                            QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        QCoreApplication::installTranslator(&m_qtTranslator);
    }

    m_current = code;
    return true;
}

void TranslationManager::uninstallAll()
{
    QCoreApplication::removeTranslator(&m_appTranslator);
    QCoreApplication::removeTranslator(&m_qtTranslator);
}

QString TranslationManager::preferredLanguage()
{
    QSettings settings;
    const QString stored = settings.value(QLatin1String(kLanguageKey)).toString();
    if (!stored.isEmpty())
        return stored;

    const QString system = QLocale::system().name().section(QLatin1Char('_'), 0, 0);
    return system == QLatin1String("ru") ? system : sourceLanguage();
}

void TranslationManager::rememberLanguage(const QString &code)
{
    QSettings settings;
    settings.setValue(QLatin1String(kLanguageKey), code);
}
