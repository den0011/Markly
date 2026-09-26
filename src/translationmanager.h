#ifndef TRANSLATIONMANAGER_H
#define TRANSLATIONMANAGER_H

#include <QString>
#include <QTranslator>

// Installs and swaps the application's translation catalogs at runtime.
//
// English is the source language and therefore has no catalog: selecting it
// simply uninstalls the translators. Other languages load Markly's own catalog
// from the embedded `:/i18n` resource, plus Qt's own catalog (for standard
// dialog buttons and file dialogs) from the Qt installation when present.
//
// Installing a translator makes Qt post a LanguageChange event to every widget,
// which MainWindow handles by calling retranslateUi().
class TranslationManager
{
public:
    static TranslationManager &instance();

    static QString sourceLanguage() { return QStringLiteral("en"); }

    // ISO 639-1 code of the active language.
    QString currentLanguage() const { return m_current; }

    // Returns false when the catalog for `code` is missing; the previous
    // language stays active in that case.
    bool setLanguage(const QString &code);

    // Language remembered in QSettings, falling back to the system locale and
    // then to the source language.
    static QString preferredLanguage();
    static void rememberLanguage(const QString &code);

private:
    TranslationManager() = default;

    void uninstallAll();

    QTranslator m_appTranslator;
    QTranslator m_qtTranslator;
    QString m_current = sourceLanguage();
};

#endif // TRANSLATIONMANAGER_H
