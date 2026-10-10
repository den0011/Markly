#ifndef PREVIEWVIEW_H
#define PREVIEWVIEW_H

#include <QTextBrowser>

class QTimer;

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

    // Pictures are decoded here rather than by QTextDocument, for two reasons.
    // SVG is rendered with QSvgRenderer, so it no longer depends on the qsvg
    // image-format plugin being deployed next to the executable (without it
    // every SVG turns into a "missing file" icon). And a picture wider than the
    // pane is scaled down to fit: QTextDocument would clip it instead.
    QVariant loadResource(int type, const QUrl &name) override;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void openExternalLink(const QUrl &url);
    void refitImages();

private:
    void applyZoom();

    // Width a picture may occupy, or 0 while the pane is too narrow (or not yet
    // laid out) for a limit to mean anything.
    int availableImageWidth() const;

    // Width the widest picture ends up at under `limit`.
    int fittedWidth(int limit) const;

    QString m_css;
    QString m_html;
    int m_zoomSteps = 0;
    int m_baseFontSize;

    QTimer *m_refitTimer;
    int m_widestImage = 0;
    int m_appliedLimit = 0;
};

#endif // PREVIEWVIEW_H
