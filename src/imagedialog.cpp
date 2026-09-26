#include "imagedialog.h"
#include "ui_imagedialog.h"

#include "markdownsnippets.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QPixmap>
#include <QPushButton>

ImageDialog::ImageDialog(const QString &documentPath, QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::ImageDialog)
    , m_documentPath(documentPath)
{
    m_ui->setupUi(this);

    m_ui->relativeCheck->setEnabled(!documentPath.isEmpty());
    if (documentPath.isEmpty())
        m_ui->relativeCheck->setChecked(false);

    connect(m_ui->browseButton, &QToolButton::clicked, this, &ImageDialog::browse);
    connect(m_ui->altEdit, &QLineEdit::textChanged, this, &ImageDialog::updatePreview);
    connect(m_ui->targetEdit, &QLineEdit::textChanged, this, &ImageDialog::updatePreview);
    connect(m_ui->titleEdit, &QLineEdit::textChanged, this, &ImageDialog::updatePreview);
    connect(m_ui->relativeCheck, &QCheckBox::toggled, this, &ImageDialog::updatePreview);

    updatePreview();
}

ImageDialog::~ImageDialog() = default;

void ImageDialog::setAltText(const QString &text)
{
    m_ui->altEdit->setText(text);
    updatePreview();
}

void ImageDialog::setTarget(const QString &target)
{
    m_ui->targetEdit->setText(target);
    updatePreview();
}

QString ImageDialog::markdown() const
{
    const QString target = MarkdownSnippets::linkTarget(
        m_ui->targetEdit->text(), m_documentPath, m_ui->relativeCheck->isChecked());

    return MarkdownSnippets::image(m_ui->altEdit->text(), target, m_ui->titleEdit->text());
}

void ImageDialog::browse()
{
    const QString start = m_documentPath.isEmpty()
        ? QString()
        : QFileInfo(m_documentPath).absolutePath();

    QStringList patterns;
    const QList<QByteArray> formats = QImageReader::supportedImageFormats();
    for (const QByteArray &format : formats)
        patterns << QStringLiteral("*.%1").arg(QString::fromLatin1(format));

    const QString filter = tr("Images (%1);;All Files (*)").arg(patterns.join(QLatin1Char(' ')));

    const QString path = QFileDialog::getOpenFileName(this, tr("Choose an Image"), start, filter);
    if (path.isEmpty())
        return;

    m_ui->targetEdit->setText(QDir::toNativeSeparators(path));
    if (m_ui->altEdit->text().isEmpty())
        m_ui->altEdit->setText(QFileInfo(path).completeBaseName());
}

void ImageDialog::updatePreview()
{
    const QString target = m_ui->targetEdit->text().trimmed();
    const bool ready = !target.isEmpty();

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ready);
    m_ui->previewLabel->setText(ready ? markdown() : QString());
    showThumbnail(target);
}

void ImageDialog::showThumbnail(const QString &target)
{
    // Only local files are previewed; fetching a remote image would turn typing
    // a URL into a series of network requests.
    const QPixmap image(target);
    if (target.isEmpty() || image.isNull()) {
        m_ui->thumbnailLabel->setPixmap(QPixmap());
        m_ui->thumbnailLabel->setText(target.isEmpty() ? tr("No image selected")
                                                       : tr("No preview available"));
        return;
    }

    m_ui->thumbnailLabel->setText(QString());
    m_ui->thumbnailLabel->setPixmap(
        image.scaled(m_ui->thumbnailLabel->size() - QSize(8, 8),
                     Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
