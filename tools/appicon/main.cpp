// Draws the application icon and writes resources/markly.ico.
//
// The icon is painted rather than kept as a binary blob nobody can edit, for
// the same reason the toolbar icons are: the shapes stay reviewable in a diff,
// and every size is rendered at its own scale instead of being downsampled from
// one large bitmap, which is what makes 16 px legible.
//
//   qmake ../../tools/appicon/appicon.pro && make
//   ./release/appicon <path-to-resources-dir>
//
// Pass --sheet <file.png> as well to get a contact sheet for reviewing.

#include <QBuffer>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QVector>

namespace {

// All sizes Windows asks for, smallest first.
const QVector<int> kSizes = { 16, 20, 24, 32, 48, 64, 128, 256 };

const QColor kBackgroundTop(0x3B, 0x82, 0xF6);
const QColor kBackgroundBottom(0x1D, 0x4E, 0xD8);
const QColor kForeground(0xFF, 0xFF, 0xFF);

// The mark is laid out in a 100x100 box and scaled, so the numbers below are
// percentages in disguise.
const qreal kCanvas = 100.0;

QFont fittedFont(const QString &text, const QRectF &box)
{
    QFont font(QStringLiteral("Segoe UI"));
    font.setStyleHint(QFont::SansSerif);
    font.setBold(true);
    font.setPixelSize(int(box.height()));

    const QRectF bounds = QFontMetricsF(font).tightBoundingRect(text);
    if (bounds.width() > 0.0 && bounds.height() > 0.0) {
        const qreal scale = qMin(box.width() / bounds.width(),
                                 box.height() / bounds.height());
        font.setPixelSize(qMax(1, int(font.pixelSize() * scale)));
    }

    return font;
}

void paintMark(QPainter *painter, int pixelSize)
{
    // Rounded badge. The corner radius shrinks at small sizes: a 22% radius
    // that looks right at 256 px eats the whole glyph at 16 px.
    const qreal radius = pixelSize <= 24 ? 14.0 : 22.0;

    QLinearGradient gradient(QPointF(0, 0), QPointF(0, kCanvas));
    gradient.setColorAt(0.0, kBackgroundTop);
    gradient.setColorAt(1.0, kBackgroundBottom);

    painter->setPen(Qt::NoPen);
    painter->setBrush(gradient);
    painter->drawRoundedRect(QRectF(2, 2, 96, 96), radius, radius);

    // "M" for Markly, with the down caret that says Markdown. The mark keeps a
    // margin of roughly a sixth of the badge on each side; filling the badge
    // edge to edge reads as cramped once the icon is shown large.
    painter->setPen(kForeground);
    const QRectF letterBox(18, 30, 36, 40);
    painter->setFont(fittedFont(QStringLiteral("M"), letterBox));
    painter->drawText(letterBox, Qt::AlignCenter, QStringLiteral("M"));

    painter->setPen(Qt::NoPen);
    painter->setBrush(kForeground);

    QPainterPath caret;
    caret.moveTo(66, 30);
    caret.lineTo(76, 30);
    caret.lineTo(76, 55);
    caret.lineTo(82, 55);
    caret.lineTo(71, 70);
    caret.lineTo(60, 55);
    caret.lineTo(66, 55);
    caret.closeSubpath();
    painter->drawPath(caret);
}

QImage renderIcon(int pixelSize)
{
    QImage image(pixelSize, pixelSize, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.scale(pixelSize / kCanvas, pixelSize / kCanvas);
    paintMark(&painter, pixelSize);

    return image;
}

QByteArray toPng(const QImage &image)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return bytes;
}

// Writes a PNG-compressed .ico. Qt cannot write this format, and the container
// is simple enough that pulling in a dependency for it would be worse.
bool writeIco(const QString &path, const QVector<QImage> &images)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);

    out << quint16(0) << quint16(1) << quint16(images.size());

    QVector<QByteArray> payloads;
    payloads.reserve(images.size());
    for (const QImage &image : images)
        payloads.append(toPng(image));

    // Directory entries are fixed width, so the first payload starts right
    // after all of them.
    quint32 offset = 6 + 16 * quint32(images.size());
    for (int i = 0; i < images.size(); ++i) {
        const int size = images.at(i).width();

        out << quint8(size >= 256 ? 0 : size)   // 0 means 256 in this format
            << quint8(size >= 256 ? 0 : size)
            << quint8(0)                        // palette size, none
            << quint8(0)                        // reserved
            << quint16(1)                       // colour planes
            << quint16(32)                      // bits per pixel
            << quint32(payloads.at(i).size())
            << offset;

        offset += quint32(payloads.at(i).size());
    }

    for (const QByteArray &payload : payloads)
        file.write(payload);

    return true;
}

void writeSheet(const QString &path, const QVector<QImage> &images)
{
    const int cell = 140;
    QImage sheet(cell * images.size(), cell + 24, QImage::Format_ARGB32);
    sheet.fill(QColor(0xF4, 0xF4, 0xF6));

    QPainter painter(&sheet);
    for (int i = 0; i < images.size(); ++i) {
        const QImage &image = images.at(i);
        const int x = i * cell + (cell - qMin(image.width(), 128)) / 2;

        painter.drawImage(x, (cell - qMin(image.height(), 128)) / 2,
                          image.width() > 128
                              ? image.scaled(128, 128, Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation)
                              : image);

        painter.setPen(QColor(0x66, 0x66, 0x70));
        painter.drawText(QRect(i * cell, cell, cell, 24), Qt::AlignCenter,
                         QStringLiteral("%1 px").arg(image.width()));
    }
    painter.end();

    sheet.save(path);
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    const QStringList args = app.arguments();
    if (args.size() < 2) {
        qWarning("usage: appicon <resources-dir> [--sheet <file.png>]");
        return 2;
    }

    QVector<QImage> images;
    for (const int size : kSizes)
        images.append(renderIcon(size));

    const QDir resources(args.at(1));

    const QString icoPath = resources.filePath(QStringLiteral("markly.ico"));
    if (!writeIco(icoPath, images)) {
        qWarning("cannot write %s", qPrintable(icoPath));
        return 1;
    }

    // The same mark as a PNG, because .ico does not render in a README.
    if (!images.last().save(resources.filePath(QStringLiteral("markly.png")))) {
        qWarning("cannot write markly.png");
        return 1;
    }

    const int sheetIndex = args.indexOf(QStringLiteral("--sheet"));
    if (sheetIndex > 0 && sheetIndex + 1 < args.size())
        writeSheet(args.at(sheetIndex + 1), images);

    return 0;
}
