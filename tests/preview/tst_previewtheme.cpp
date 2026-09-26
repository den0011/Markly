#include <QtTest>

#include <QFile>
#include <QTextCursor>

#include "previewview.h"

// Switching the preview theme used to be a no-op: setHtmlPreserveScroll skips
// input identical to what it already holds, so re-applying the same HTML after
// a stylesheet change never re-parsed the document, and the widget palette was
// never touched at all. These tests pin both halves down.
class TestPreviewTheme : public QObject
{
    Q_OBJECT

private slots:
    void styleSheetReachesAlreadyRenderedText();
    void paletteFollowsTheTheme();
    void switchingBackRestoresTheLightTheme();

private:
    static QString styleSheet(const QString &resource);
    static QColor firstCharacterColour(const PreviewView &preview);

    static void applyLight(PreviewView &preview);
    static void applyDark(PreviewView &preview);

    static const char *kLightCss;
    static const char *kDarkCss;
};

void TestPreviewTheme::applyLight(PreviewView &preview)
{
    preview.setTheme(styleSheet(QLatin1String(kLightCss)),
                     QColor(QStringLiteral("#ffffff")),
                     QColor(QStringLiteral("#24292f")));
}

void TestPreviewTheme::applyDark(PreviewView &preview)
{
    preview.setTheme(styleSheet(QLatin1String(kDarkCss)),
                     QColor(QStringLiteral("#0d1117")),
                     QColor(QStringLiteral("#d1d7e0")));
}

const char *TestPreviewTheme::kLightCss = ":/preview-light.css";
const char *TestPreviewTheme::kDarkCss = ":/preview-dark.css";

QString TestPreviewTheme::styleSheet(const QString &resource)
{
    QFile file(resource);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(file.readAll());
}

QColor TestPreviewTheme::firstCharacterColour(const PreviewView &preview)
{
    QTextCursor cursor(preview.document());
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
    return cursor.charFormat().foreground().color();
}

void TestPreviewTheme::styleSheetReachesAlreadyRenderedText()
{
    QVERIFY2(!styleSheet(QLatin1String(kLightCss)).isEmpty(),
             "the stylesheets must be reachable through the resource system");

    PreviewView preview;
    applyLight(preview);

    // Probed through a heading rather than plain text: Qt paints body text with
    // the palette and never writes `body { color }` into the character format,
    // so a paragraph would report black under either theme.
    preview.setHtmlPreserveScroll(QStringLiteral("<h1>Title</h1>\n"));
    const QColor light = firstCharacterColour(preview);

    // The content does not change, only the theme — this is exactly the case
    // that silently did nothing.
    applyDark(preview);
    const QColor dark = firstCharacterColour(preview);

    QVERIFY2(light != dark,
             qPrintable(QStringLiteral("light=%1 dark=%2").arg(light.name(), dark.name())));
    QCOMPARE(light, QColor(QStringLiteral("#1f2328")));
    QCOMPARE(dark, QColor(QStringLiteral("#f0f6fc")));
}

void TestPreviewTheme::paletteFollowsTheTheme()
{
    PreviewView preview;
    preview.setHtmlPreserveScroll(QStringLiteral("<p>hello</p>\n"));
    applyDark(preview);

    // This is what actually colours body text and the area behind it: the
    // viewport would otherwise stay white under the dark stylesheet.
    QCOMPARE(preview.palette().color(QPalette::Base), QColor(QStringLiteral("#0d1117")));
    QCOMPARE(preview.palette().color(QPalette::Text), QColor(QStringLiteral("#d1d7e0")));
}

void TestPreviewTheme::switchingBackRestoresTheLightTheme()
{
    PreviewView preview;
    preview.setHtmlPreserveScroll(QStringLiteral("<h1>Title</h1>\n"));

    applyDark(preview);
    applyLight(preview);

    QCOMPARE(firstCharacterColour(preview), QColor(QStringLiteral("#1f2328")));
    QCOMPARE(preview.palette().color(QPalette::Base), QColor(QStringLiteral("#ffffff")));
}

QTEST_MAIN(TestPreviewTheme)

#include "tst_previewtheme.moc"
