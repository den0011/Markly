#ifndef LINKDIALOG_H
#define LINKDIALOG_H

#include <QDialog>
#include <QScopedPointer>

QT_BEGIN_NAMESPACE
namespace Ui { class LinkDialog; }
QT_END_NAMESPACE

// Collects the pieces of a Markdown link. The target may be a web address or a
// file on this machine; `documentPath` is what a relative path is measured
// against, and may be empty for an unsaved document.
class LinkDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LinkDialog(const QString &documentPath, QWidget *parent = nullptr);
    ~LinkDialog() override;

    void setLinkText(const QString &text);
    void setTarget(const QString &target);

    QString markdown() const;

private slots:
    void browse();
    void updatePreview();

private:
    QScopedPointer<Ui::LinkDialog> m_ui;
    QString m_documentPath;
};

#endif // LINKDIALOG_H
