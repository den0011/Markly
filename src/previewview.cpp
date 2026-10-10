#include "previewview.h"

#include <QBuffer>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QPalette>
#include <QScrollBar>
#include <QSvgRenderer>
#include <QTimer>
#include <QUrl>

namespace {

// Below this the pane is either not laid out yet or too small for scaling
// pictures to it to be anything but harmful.
const int kMinImageLimit = 120;

const int kRefitDelayMs = 150;

bool looksLikeSvg(const QUrl &name, const QByteArray &data)
{
    const QString suffix = QFileInfo(name.path()).suffix().toLower();
    if (suffix == QLatin1String("svg") || suffix == QLatin1String("svgz"))
        return true;

    return data.left(1024).contains("<svg");
}

} // namespace

PreviewView::PreviewView(QWidget *parent)
    : QTextBrowser(parent)
    , m_baseFontSize(11)
    , m_refitTimer(new QTimer(this))
{
    setOpenLinks(false);
    setOpenExternalLinks(false);
    setFrameShape(QFrame::NoFrame);
    document()->setDocumentMargin(20);

    // Resizing the pane changes how wide a picture may be, and a picture is only
    // scaled when it is loaded, so it has to be loaded again. Doing that on every
    // resize event would re-parse the whole document while the splitter is
    // being dragged.
    m_refitTimer->setSingleShot(true);
    m_refitTimer->setInterval(kRefitDelayMs);
    connect(m_refitTimer, &QTimer::timeout, this, &PreviewView::refitImages);

    connect(this, &QTextBrowser::anchorClicked, this, &PreviewView::openExternalLink);
}

void PreviewView::setTheme(const QString &css, const QColor &background, const QColor &text)
{
    m_css = css;
    document()->setDefaultStyleSheet(css);

    QPalette themed = palette();
    themed.setColor(QPalette::Base, background);
    themed.setColor(QPalette::Text, text);
    setPalette(themed);

    // QTextDocument resolves the default stylesheet while it parses, so content
    // that is already laid out keeps the old colours. Re-setting the same HTML
    // is what makes a theme switch visible, and setHtmlPreserveScroll skips
    // identical input, so the cached copy is cleared first.
    if (!m_html.isEmpty()) {
        const QString html = m_html;
        m_html.clear();
        setHtmlPreserveScroll(html);
    }
}

void PreviewView::setHtmlPreserveScroll(const QString &html)
{
    if (html == m_html)
        return;

    const double ratio = scrollRatio();
    m_html = html;
    m_widestImage = 0;
    setHtml(html);
    applyZoom();
    setScrollRatio(ratio);
}

void PreviewView::setDocumentDirectory(const QString &path)
{
    if (path.isEmpty()) {
        document()->setBaseUrl(QUrl());
        setSearchPaths(QStringList());
        return;
    }

    document()->setBaseUrl(QUrl::fromLocalFile(QDir(path).absolutePath() + QLatin1Char('/')));
    setSearchPaths(QStringList() << path);
}

double PreviewView::scrollRatio() const
{
    const QScrollBar *bar = verticalScrollBar();
    if (bar->maximum() <= bar->minimum())
        return 0.0;
    return double(bar->value() - bar->minimum()) / double(bar->maximum() - bar->minimum());
}

void PreviewView::setScrollRatio(double ratio)
{
    QScrollBar *bar = verticalScrollBar();
    const int range = bar->maximum() - bar->minimum();
    if (range <= 0)
        return;
    bar->setValue(bar->minimum() + qRound(ratio * range));
}

void PreviewView::zoomIn()
{
    if (m_zoomSteps < 10) {
        ++m_zoomSteps;
        applyZoom();
    }
}

void PreviewView::zoomOut()
{
    if (m_zoomSteps > -5) {
        --m_zoomSteps;
        applyZoom();
    }
}

void PreviewView::resetZoom()
{
    m_zoomSteps = 0;
    applyZoom();
}

void PreviewView::applyZoom()
{
    QFont font = document()->defaultFont();
    font.setPointSize(qMax(6, m_baseFontSize + m_zoomSteps));
    document()->setDefaultFont(font);
}

int PreviewView::availableImageWidth() const
{
    // The scroll bar's width is reserved even while it is hidden. Otherwise a
    // picture that makes the document tall enough to need the bar would narrow
    // the viewport, shrink the picture, shorten the document, hide the bar again
    // and so on, round and round.
    const int chrome = verticalScrollBar()->sizeHint().width()
        + 2 * qRound(document()->documentMargin());
    const int width = this->width() - chrome;

    return width >= kMinImageLimit ? width : 0;
}

int PreviewView::fittedWidth(int limit) const
{
    return limit > 0 ? qMin(limit, m_widestImage) : m_widestImage;
}

QVariant PreviewView::loadResource(int type, const QUrl &name)
{
    const QVariant resource = QTextBrowser::loadResource(type, name);
    if (type != QTextDocument::ImageResource || resource.type() != QVariant::ByteArray)
        return resource;

    const QByteArray data = resource.toByteArray();
    const int limit = availableImageWidth();
    m_appliedLimit = limit;

    if (looksLikeSvg(name, data)) {
        QSvgRenderer renderer(data);
        const QSize natural = renderer.defaultSize();
        if (!renderer.isValid() || natural.isEmpty())
            return resource;

        m_widestImage = qMax(m_widestImage, natural.width());

        QSize size = natural;
        if (limit > 0 && natural.width() > limit) {
            size = QSize(limit, qMax(1, qRound(natural.height() * double(limit) / natural.width())));
        }

        // Rendered at its final size rather than scaled afterwards, so lines
        // and text stay sharp.
        QImage image(size, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        renderer.render(&painter);
        return image;
    }

    // Raster pictures that already fit are left to QTextDocument.
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QIODevice::ReadOnly);
    const QSize natural = QImageReader(&buffer).size();
    if (natural.isEmpty())
        return resource;

    m_widestImage = qMax(m_widestImage, natural.width());
    if (limit <= 0 || natural.width() <= limit)
        return resource;

    const QImage image = QImage::fromData(data);
    if (image.isNull())
        return resource;

    return image.scaledToWidth(limit, Qt::SmoothTransformation);
}

void PreviewView::resizeEvent(QResizeEvent *event)
{
    QTextBrowser::resizeEvent(event);

    if (m_widestImage > 0
        && fittedWidth(availableImageWidth()) != fittedWidth(m_appliedLimit))
        m_refitTimer->start();
}

void PreviewView::refitImages()
{
    if (m_html.isEmpty())
        return;

    // Same trick as setTheme: setHtmlPreserveScroll ignores identical input.
    const QString html = m_html;
    m_html.clear();
    setHtmlPreserveScroll(html);
}

void PreviewView::openExternalLink(const QUrl &url)
{
    if (url.isRelative() && url.path().isEmpty() && !url.fragment().isEmpty()) {
        scrollToAnchor(url.fragment());
        return;
    }

    QDesktopServices::openUrl(document()->baseUrl().resolved(url));
}
