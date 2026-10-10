#include <QtTest>

#include <QImage>
#include <QPixmap>
#include <QTemporaryDir>

#include "markdownparser.h"
#include "previewview.h"

// A document with pictures used to lose them in two ways. SVG went through the
// qsvg image-format plugin, so a build without that plugin showed a "missing
// file" icon in place of every diagram; and a picture wider than the pane was
// clipped at its edge. PreviewView::loadResource now renders SVG itself and
// scales whatever is too wide.
class TestPreviewImages : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void svgIsRenderedAtItsOwnSizeWhenItFits();
    void wideSvgIsScaledToTheViewportKeepingItsProportions();
    void svgWithoutSvgSuffixIsStillRecognised();
    void brokenSvgFallsBackToTheRawData();
    void wideRasterImageIsScaledDown();
    void rasterImageThatFitsIsLeftAlone();
    void resizingThePaneFitsThePicturesAgain();
    void paragraphOfOnlyPicturesIsNotPaddedByTheLineHeight();

private:
    void writeSvg(const QString &name, int width, int height) const;
    void writeRaster(const QString &name, int width, int height) const;

    static QImage asImage(const QVariant &resource);
    static QImage cachedImage(const PreviewView &preview, const QString &name);

    // A pane that is wide enough to be fitted to, with the folder of test
    // pictures already registered.
    void prepare(PreviewView &preview, int width) const;

    QTemporaryDir m_dir;
};

void TestPreviewImages::initTestCase()
{
    QVERIFY(m_dir.isValid());

    writeSvg(QStringLiteral("small.svg"), 300, 150);
    writeSvg(QStringLiteral("wide.svg"), 2000, 1000);
    writeRaster(QStringLiteral("wide.png"), 2000, 500);
    writeRaster(QStringLiteral("small.png"), 200, 100);

    // Same SVG under a name that gives nothing away.
    QFile source(m_dir.filePath(QStringLiteral("small.svg")));
    QVERIFY(source.copy(m_dir.filePath(QStringLiteral("picture.bin"))));

    QFile broken(m_dir.filePath(QStringLiteral("broken.svg")));
    QVERIFY(broken.open(QIODevice::WriteOnly));
    broken.write("this is not a drawing");
}

void TestPreviewImages::writeSvg(const QString &name, int width, int height) const
{
    QFile file(m_dir.filePath(name));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QStringLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %1 %2\" width=\"%1\" height=\"%2\">"
        "<rect x=\"0\" y=\"0\" width=\"%1\" height=\"%2\" fill=\"#2563eb\"/></svg>")
        .arg(width).arg(height).toUtf8());
}

void TestPreviewImages::writeRaster(const QString &name, int width, int height) const
{
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::red);
    QVERIFY(image.save(m_dir.filePath(name)));
}

QImage TestPreviewImages::asImage(const QVariant &resource)
{
    if (resource.type() == QVariant::Image)
        return qvariant_cast<QImage>(resource);
    if (resource.type() == QVariant::Pixmap)
        return qvariant_cast<QPixmap>(resource).toImage();
    return QImage();
}

QImage TestPreviewImages::cachedImage(const PreviewView &preview, const QString &name)
{
    return asImage(preview.document()->resource(QTextDocument::ImageResource, QUrl(name)));
}

void TestPreviewImages::prepare(PreviewView &preview, int width) const
{
    preview.setDocumentDirectory(m_dir.path());
    preview.resize(width, 600);
}

void TestPreviewImages::svgIsRenderedAtItsOwnSizeWhenItFits()
{
    PreviewView preview;
    prepare(preview, 900);

    const QVariant resource = preview.loadResource(QTextDocument::ImageResource,
                                                   QUrl(QStringLiteral("small.svg")));

    // A QImage, not the raw bytes: the bytes would have been handed to the
    // image-format plugin, which is exactly what must not be needed.
    QCOMPARE(resource.type(), QVariant::Image);
    const QImage image = qvariant_cast<QImage>(resource);
    QCOMPARE(image.size(), QSize(300, 150));
    QCOMPARE(image.pixelColor(150, 75), QColor(QStringLiteral("#2563eb")));
}

void TestPreviewImages::wideSvgIsScaledToTheViewportKeepingItsProportions()
{
    PreviewView preview;
    prepare(preview, 700);

    const QImage image = asImage(preview.loadResource(QTextDocument::ImageResource,
                                                      QUrl(QStringLiteral("wide.svg"))));

    QVERIFY(!image.isNull());
    QVERIFY2(image.width() < 700,
             qPrintable(QStringLiteral("width %1 must leave room for the margins and scroll bar")
                            .arg(image.width())));
    QVERIFY(image.width() > 500);
    QCOMPARE(image.height(), qRound(image.width() / 2.0));
}

void TestPreviewImages::svgWithoutSvgSuffixIsStillRecognised()
{
    PreviewView preview;
    prepare(preview, 900);

    const QImage image = asImage(preview.loadResource(QTextDocument::ImageResource,
                                                      QUrl(QStringLiteral("picture.bin"))));

    QCOMPARE(image.size(), QSize(300, 150));
}

void TestPreviewImages::brokenSvgFallsBackToTheRawData()
{
    PreviewView preview;
    prepare(preview, 900);

    const QVariant resource = preview.loadResource(QTextDocument::ImageResource,
                                                   QUrl(QStringLiteral("broken.svg")));

    // Nothing to draw, so the original bytes go back to QTextDocument, which
    // shows its usual placeholder rather than the preview failing outright.
    QCOMPARE(resource.type(), QVariant::ByteArray);
}

void TestPreviewImages::wideRasterImageIsScaledDown()
{
    PreviewView preview;
    prepare(preview, 700);

    const QImage image = asImage(preview.loadResource(QTextDocument::ImageResource,
                                                      QUrl(QStringLiteral("wide.png"))));

    QVERIFY(!image.isNull());
    QVERIFY(image.width() < 700);
    QVERIFY(image.width() > 500);
    QCOMPARE(image.height(), qRound(image.width() / 4.0));
}

void TestPreviewImages::rasterImageThatFitsIsLeftAlone()
{
    PreviewView preview;
    prepare(preview, 900);

    const QVariant resource = preview.loadResource(QTextDocument::ImageResource,
                                                   QUrl(QStringLiteral("small.png")));

    QCOMPARE(resource.type(), QVariant::ByteArray);
}

void TestPreviewImages::resizingThePaneFitsThePicturesAgain()
{
    PreviewView preview;
    prepare(preview, 700);
    preview.show();
    QVERIFY(QTest::qWaitForWindowExposed(&preview));

    preview.setHtmlPreserveScroll(QStringLiteral("<p><img src=\"wide.svg\" /></p>\n"));
    preview.document()->size();   // lays the document out, which loads the picture

    const int narrow = cachedImage(preview, QStringLiteral("wide.svg")).width();
    QVERIFY(narrow > 0);
    QVERIFY(narrow < 700);

    preview.resize(1300, 600);
    QTRY_VERIFY2(cachedImage(preview, QStringLiteral("wide.svg")).width() > narrow + 300,
                 qPrintable(QStringLiteral("still %1 wide after the pane grew")
                                .arg(cachedImage(preview, QStringLiteral("wide.svg")).width())));

    preview.resize(500, 600);
    QTRY_VERIFY(cachedImage(preview, QStringLiteral("wide.svg")).width() < narrow);
}

void TestPreviewImages::paragraphOfOnlyPicturesIsNotPaddedByTheLineHeight()
{
    QFile css(QStringLiteral(":/preview-light.css"));
    QVERIFY(css.open(QIODevice::ReadOnly | QIODevice::Text));

    PreviewView preview;
    prepare(preview, 900);
    preview.setTheme(QString::fromUtf8(css.readAll()), Qt::white, Qt::black);

    // The widget is never shown, and a document that is not shown is not laid
    // out either: its height would read as 0 without a width to lay out to.
    preview.document()->setTextWidth(800);

    // What the stylesheet alone does to a picture's paragraph.
    preview.setHtmlPreserveScroll(
        QStringLiteral("<p><img src=\"small.png\" /></p>\n<p>after</p>\n"));
    const qreal padded = preview.document()->size().height();

    // What the parser emits for the same Markdown.
    preview.setHtmlPreserveScroll(
        MarkdownParser().toHtml(QStringLiteral("![x](small.png)\n\nafter")));
    const qreal tight = preview.document()->size().height();

    // small.png is 100 px tall, and 150% of that is 50 px of empty space.
    QVERIFY2(padded - tight > 40,
             qPrintable(QStringLiteral("padded %1, tight %2").arg(padded).arg(tight)));
}

QTEST_MAIN(TestPreviewImages)

#include "tst_previewimages.moc"
