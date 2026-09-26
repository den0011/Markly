#ifndef PREVIEWVIEW_H
#define PREVIEWVIEW_H

#include <QTextBrowser>

// Renders the HTML produced by MarkdownParser. Re-rendering happens on every
// keystroke (debounced by the main window), so setHtmlPreserveScroll keeps the
// viewport anchored instead of jumping back to the top.
class PreviewView : public QTextBrowser
{
    Q_OBJECT

public:
    explicit PreviewView(QWidget *parent = nullptr);

    // Applies the stylesheet together with the matching widget colours. Both
    // are needed: the CSS reaches the text, while the area around it — the
    // viewport and the document margin — comes from the widget palette.
    void setTheme(const QString &css, const QColor &background, const QColor &text);

    QString styleSheetSource() const { return m_css; }

    void setHtmlPreserveScroll(const QString &html);

    // Relative image and link paths are resolved against the document's folder.
    void setDocumentDirectory(const QString &path);

    void zoomIn();
    void zoomOut();
    void resetZoom();

    // 0.0 at the top, 1.0 at the bottom. Used for scroll syncing.
    double scrollRatio() const;
    void setScrollRatio(double ratio);

private slots:
    void openExternalLink(const QUrl &url);

private:
    void applyZoom();

    QString m_css;
    QString m_html;
    int m_zoomSteps = 0;
    int m_baseFontSize;
};

#endif // PREVIEWVIEW_H
