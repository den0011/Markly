#include "previewview.h"

#include <QDesktopServices>
#include <QDir>
#include <QPalette>
#include <QScrollBar>
#include <QUrl>

PreviewView::PreviewView(QWidget *parent)
    : QTextBrowser(parent)
    , m_baseFontSize(11)
{
    setOpenLinks(false);
    setOpenExternalLinks(false);
    setFrameShape(QFrame::NoFrame);
    document()->setDocumentMargin(20);

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

void PreviewView::openExternalLink(const QUrl &url)
{
    if (url.isRelative() && url.path().isEmpty() && !url.fragment().isEmpty()) {
        scrollToAnchor(url.fragment());
        return;
    }

    QDesktopServices::openUrl(document()->baseUrl().resolved(url));
}
