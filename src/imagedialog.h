#ifndef IMAGEDIALOG_H
#define IMAGEDIALOG_H

#include <QDialog>
#include <QScopedPointer>

QT_BEGIN_NAMESPACE
namespace Ui { class ImageDialog; }
QT_END_NAMESPACE

// Collects the pieces of a Markdown image, with a thumbnail so that picking the
// wrong file is obvious before it lands in the document.
class ImageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImageDialog(const QString &documentPath, QWidget *parent = nullptr);
    ~ImageDialog() override;

    void setAltText(const QString &text);
    void setTarget(const QString &target);

    QString markdown() const;

private slots:
    void browse();
    void updatePreview();

private:
    void showThumbnail(const QString &target);

    QScopedPointer<Ui::ImageDialog> m_ui;
    QString m_documentPath;
};

#endif // IMAGEDIALOG_H
