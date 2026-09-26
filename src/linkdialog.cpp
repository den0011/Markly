#include "linkdialog.h"
#include "ui_linkdialog.h"

#include "markdownsnippets.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QPushButton>

LinkDialog::LinkDialog(const QString &documentPath, QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::LinkDialog)
    , m_documentPath(documentPath)
{
    m_ui->setupUi(this);

    // Nothing to be relative to until the document has been saved somewhere.
    m_ui->relativeCheck->setEnabled(!documentPath.isEmpty());
    if (documentPath.isEmpty())
        m_ui->relativeCheck->setChecked(false);

    connect(m_ui->browseButton, &QToolButton::clicked, this, &LinkDialog::browse);
    connect(m_ui->textEdit, &QLineEdit::textChanged, this, &LinkDialog::updatePreview);
    connect(m_ui->targetEdit, &QLineEdit::textChanged, this, &LinkDialog::updatePreview);
    connect(m_ui->titleEdit, &QLineEdit::textChanged, this, &LinkDialog::updatePreview);
    connect(m_ui->relativeCheck, &QCheckBox::toggled, this, &LinkDialog::updatePreview);

    updatePreview();
}

LinkDialog::~LinkDialog() = default;

void LinkDialog::setLinkText(const QString &text)
{
    m_ui->textEdit->setText(text);
    updatePreview();
}

void LinkDialog::setTarget(const QString &target)
{
    m_ui->targetEdit->setText(target);
    updatePreview();
}

QString LinkDialog::markdown() const
{
    const QString target = MarkdownSnippets::linkTarget(
        m_ui->targetEdit->text(), m_documentPath, m_ui->relativeCheck->isChecked());

    QString text = m_ui->textEdit->text();
    if (text.isEmpty())
        text = target;

    return MarkdownSnippets::link(text, target, m_ui->titleEdit->text());
}

void LinkDialog::browse()
{
    const QString start = m_documentPath.isEmpty()
        ? QString()
        : QFileInfo(m_documentPath).absolutePath();

    const QString path = QFileDialog::getOpenFileName(this, tr("Choose a File to Link To"), start);
    if (path.isEmpty())
        return;

    m_ui->targetEdit->setText(QDir::toNativeSeparators(path));
    if (m_ui->textEdit->text().isEmpty())
        m_ui->textEdit->setText(QFileInfo(path).fileName());
}

void LinkDialog::updatePreview()
{
    const bool ready = !m_ui->targetEdit->text().trimmed().isEmpty();

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ready);
    m_ui->previewLabel->setText(ready ? markdown() : QString());
}
