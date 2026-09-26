#include "actionicons.h"

#include <QApplication>
#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmap>
#include <QVector>

#include <functional>

namespace {

// Every icon is drawn in a 100x100 logical box and scaled to the requested
// pixel size, so the shapes below can use round numbers.
const qreal kCanvas = 100.0;

const int kSizes[] = { 16, 20, 24, 32, 48 };

QPen strokePen(const QColor &color, qreal width)
{
    QPen pen(color);
    pen.setWidthF(width);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    return pen;
}

// Largest font that keeps `text` inside `box`.
QFont fittedFont(const QString &text, const QRectF &box,
                 bool bold, bool italic, bool strikeOut)
{
    QFont font = QApplication::font();
    font.setBold(bold);
    font.setItalic(italic);
    font.setStrikeOut(strikeOut);
    font.setPixelSize(int(box.height()));

    const QRectF bounds = QFontMetricsF(font).tightBoundingRect(text);
    if (bounds.width() > 0.0 && bounds.height() > 0.0) {
        const qreal scale = qMin(box.width() / bounds.width(),
                                 box.height() / bounds.height());
        font.setPixelSize(qMax(1, int(font.pixelSize() * scale)));
    }

    return font;
}

void drawGlyph(QPainter *painter, const QString &text, const QColor &color,
               bool bold, bool italic = false, bool strikeOut = false)
{
    const QRectF box(12, 16, 76, 68);

    painter->setPen(color);
    painter->setFont(fittedFont(text, box, bold, italic, strikeOut));
    painter->drawText(QRectF(0, 0, kCanvas, kCanvas), Qt::AlignCenter, text);
}

// Three stacked rows of text, with `bullet` drawn in the left margin.
void drawList(QPainter *painter, const QColor &color, qreal textStart,
              const std::function<void(QPainter *, const QPointF &, int)> &bullet)
{
    const qreal rows[] = { 24, 50, 76 };

    painter->setPen(strokePen(color, 9));
    for (int i = 0; i < 3; ++i) {
        painter->drawLine(QPointF(textStart, rows[i]), QPointF(88, rows[i]));
        bullet(painter, QPointF(18, rows[i]), i);
    }
}

// QFont::setStrikeOut is not honoured when the painter carries a scale, so the
// bar is drawn by hand.
void paintStrikethrough(QPainter *painter, const QColor &color)
{
    drawGlyph(painter, QStringLiteral("S"), color, true);

    painter->setPen(strokePen(color, 9));
    painter->drawLine(QPointF(14, 50), QPointF(86, 50));
}

void paintNew(QPainter *painter, const QColor &color)
{
    QPainterPath page;
    page.moveTo(22, 8);
    page.lineTo(60, 8);
    page.lineTo(80, 30);
    page.lineTo(80, 92);
    page.lineTo(22, 92);
    page.closeSubpath();

    painter->setPen(strokePen(color, 8));
    painter->drawPath(page);
    painter->drawPolyline(QPolygonF() << QPointF(58, 10) << QPointF(58, 32)
                                      << QPointF(78, 32));
}

void paintOpen(QPainter *painter, const QColor &color)
{
    painter->setPen(strokePen(color, 8));
    painter->drawPolyline(QPolygonF()
                          << QPointF(10, 82) << QPointF(10, 22) << QPointF(40, 22)
                          << QPointF(50, 36) << QPointF(84, 36));
    painter->drawPolyline(QPolygonF()
                          << QPointF(10, 82) << QPointF(26, 50) << QPointF(94, 50)
                          << QPointF(78, 82) << QPointF(10, 82));
}

void paintSave(QPainter *painter, const QColor &color)
{
    painter->setPen(strokePen(color, 8));
    painter->drawRect(QRectF(12, 12, 76, 76));
    painter->drawRect(QRectF(32, 12, 36, 26));
    painter->drawRect(QRectF(28, 56, 44, 32));
}

void paintPrint(QPainter *painter, const QColor &color)
{
    painter->setPen(strokePen(color, 8));
    painter->drawRect(QRectF(28, 6, 44, 24));
    painter->drawRect(QRectF(12, 30, 76, 36));
    painter->drawRect(QRectF(26, 64, 48, 28));

    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawEllipse(QPointF(70, 40), 5, 5);
    painter->setBrush(Qt::NoBrush);
}

// The printer body, shrunk into the top-left quadrant so a magnifier glass
// fits alongside it without the two overlapping into a blur.
void paintPrintPreview(QPainter *painter, const QColor &color)
{
    painter->save();
    painter->translate(-6, -6);
    painter->scale(0.66, 0.66);
    paintPrint(painter, color);
    painter->restore();

    painter->setPen(strokePen(color, 8));
    painter->drawEllipse(QPointF(66, 66), 20, 20);
    painter->drawLine(QPointF(80, 80), QPointF(94, 94));
}

void paintInlineCode(QPainter *painter, const QColor &color)
{
    painter->setPen(strokePen(color, 10));
    painter->drawPolyline(QPolygonF() << QPointF(36, 26) << QPointF(12, 50)
                                      << QPointF(36, 74));
    painter->drawPolyline(QPolygonF() << QPointF(64, 26) << QPointF(88, 50)
                                      << QPointF(64, 74));
}

// A pair of opening quotation marks: a bar-plus-lines version reads as just
// another list icon next to the two real ones.
void paintBlockquote(QPainter *painter, const QColor &color)
{
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);

    // Ball on top with the tail descending, i.e. the comma shape. Mirroring it
    // into an opening quote was tried and reads as a teardrop at icon sizes.
    for (const qreal offset : { 0.0, 46.0 }) {
        QPainterPath mark;
        mark.addEllipse(QPointF(26 + offset, 38), 16, 16);

        QPainterPath tail;
        tail.moveTo(26 + offset, 44);
        tail.lineTo(42 + offset, 44);
        tail.lineTo(24 + offset, 78);
        tail.closeSubpath();

        painter->drawPath(mark.united(tail));
    }

    painter->setBrush(Qt::NoBrush);
}

void paintLink(QPainter *painter, const QColor &color)
{
    painter->setPen(strokePen(color, 9));

    // Two capsules meeting at the middle, drawn along the diagonal.
    painter->save();
    painter->translate(50, 50);
    painter->rotate(-45);

    QPainterPath left;
    left.addRoundedRect(QRectF(-44, -15, 42, 30), 15, 15);
    QPainterPath right;
    right.addRoundedRect(QRectF(2, -15, 42, 30), 15, 15);

    painter->drawPath(left);
    painter->drawPath(right);
    painter->drawLine(QPointF(-14, 0), QPointF(14, 0));
    painter->restore();
}

void paintPanes(QPainter *painter, const QColor &color,
                bool fillLeft, bool fillRight, bool divider)
{
    const QRectF frame(10, 18, 80, 64);

    painter->setPen(strokePen(color, 8));
    painter->drawRect(frame);

    QColor fill = color;
    fill.setAlphaF(0.35);

    if (fillLeft)
        painter->fillRect(QRectF(14, 22, 32, 56), fill);
    if (fillRight)
        painter->fillRect(QRectF(54, 22, 32, 56), fill);
    if (divider)
        painter->drawLine(QPointF(50, 18), QPointF(50, 82));
}

// Arrow head centred on `tip`, pointing along `angle` degrees.
void drawArrowHead(QPainter *painter, const QPointF &tip, qreal angle, qreal size)
{
    const QColor color = painter->pen().color();

    painter->save();
    painter->translate(tip);
    painter->rotate(angle);
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);

    QPainterPath head;
    head.moveTo(0, 0);
    head.lineTo(-size, -size * 0.62);
    head.lineTo(-size, size * 0.62);
    head.closeSubpath();

    painter->drawPath(head);
    painter->restore();
}

// Undo and Redo differ only in which way the arc runs.
void paintUndoRedo(QPainter *painter, const QColor &color, bool mirrored)
{
    painter->save();
    if (mirrored) {
        painter->translate(kCanvas, 0);
        painter->scale(-1, 1);
    }

    painter->setPen(strokePen(color, 9));
    painter->setBrush(Qt::NoBrush);

    // Top half of a circle, left to right, with the tail dropping on the right.
    // A cubic was tried first and gave a lopsided arc that the head missed.
    const QRectF box(24, 30, 56, 56);
    QPainterPath arc;
    arc.arcMoveTo(box, 180);
    arc.arcTo(box, 180, -180);
    arc.lineTo(80, 82);
    painter->drawPath(arc);

    // The head has to be clearly wider than the 9-unit stroke, otherwise it
    // merges into the arc and reads as a blob.
    painter->setPen(color);
    drawArrowHead(painter, QPointF(24, 84), 90, 26);
    painter->restore();
}

void paintExportHtml(QPainter *painter, const QColor &color)
{
    painter->setPen(strokePen(color, 8));
    painter->drawPolyline(QPolygonF()
                          << QPointF(54, 12) << QPointF(20, 12) << QPointF(20, 88)
                          << QPointF(76, 88) << QPointF(76, 56));

    painter->setPen(strokePen(color, 9));
    painter->drawLine(QPointF(48, 44), QPointF(88, 44));
    painter->setPen(color);
    drawArrowHead(painter, QPointF(92, 44), 0, 18);
}

// A page with a folded corner, stamped "PDF" rather than fitted with an
// arrow: the file format is the point, not the direction of the export.
void paintExportPdf(QPainter *painter, const QColor &color)
{
    painter->setPen(strokePen(color, 8));
    painter->drawPolyline(QPolygonF()
                          << QPointF(54, 8) << QPointF(24, 8) << QPointF(24, 92)
                          << QPointF(76, 92) << QPointF(76, 30) << QPointF(54, 8));
    painter->drawPolyline(QPolygonF()
                          << QPointF(54, 8) << QPointF(54, 30) << QPointF(76, 30));

    const QRectF box(28, 50, 44, 28);
    painter->setPen(color);
    painter->setFont(fittedFont(QStringLiteral("PDF"), box, true, false, false));
    painter->drawText(box, Qt::AlignCenter, QStringLiteral("PDF"));
}

void paintExit(QPainter *painter, const QColor &color)
{
    painter->setPen(strokePen(color, 8));
    painter->drawPolyline(QPolygonF()
                          << QPointF(52, 14) << QPointF(16, 14) << QPointF(16, 86)
                          << QPointF(52, 86));

    painter->setPen(strokePen(color, 9));
    painter->drawLine(QPointF(44, 50), QPointF(84, 50));
    painter->setPen(color);
    drawArrowHead(painter, QPointF(90, 50), 0, 18);
}

void paintSaveAs(QPainter *painter, const QColor &color)
{
    painter->setPen(strokePen(color, 8));
    painter->drawRect(QRectF(12, 12, 62, 62));
    painter->drawRect(QRectF(28, 12, 30, 22));

    // Pencil, marking this as "save under a different name".
    painter->setPen(strokePen(color, 8));
    painter->drawPolyline(QPolygonF()
                          << QPointF(58, 92) << QPointF(62, 74) << QPointF(90, 46)
                          << QPointF(96, 52) << QPointF(68, 80) << QPointF(58, 92));
}

void paintCut(QPainter *painter, const QColor &color)
{
    painter->setPen(strokePen(color, 8));
    painter->drawLine(QPointF(26, 10), QPointF(66, 62));
    painter->drawLine(QPointF(74, 10), QPointF(34, 62));

    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(QPointF(26, 78), 13, 13);
    painter->drawEllipse(QPointF(74, 78), 13, 13);
}

void paintCopy(QPainter *painter, const QColor &color)
{
    painter->setPen(strokePen(color, 8));
    painter->drawRect(QRectF(12, 12, 48, 56));
    painter->drawRect(QRectF(40, 32, 48, 56));
}

void paintPaste(QPainter *painter, const QColor &color)
{
    painter->setPen(strokePen(color, 8));
    painter->drawRect(QRectF(18, 20, 64, 70));
    painter->drawRect(QRectF(36, 8, 28, 20));
    painter->setPen(strokePen(color, 7));
    painter->drawLine(QPointF(32, 50), QPointF(68, 50));
    painter->drawLine(QPointF(32, 68), QPointF(68, 68));
}

void paintMagnifier(QPainter *painter, const QColor &color, int sign)
{
    painter->setPen(strokePen(color, 9));
    painter->drawEllipse(QPointF(44, 42), 30, 30);
    painter->drawLine(QPointF(66, 64), QPointF(90, 88));

    if (sign != 0) {
        painter->drawLine(QPointF(30, 42), QPointF(58, 42));
        if (sign > 0)
            painter->drawLine(QPointF(44, 28), QPointF(44, 56));
    }
}

// A crescent, cut out of a full disc rather than drawn as an arc.
void paintDarkPreview(QPainter *painter, const QColor &color)
{
    QPainterPath disc;
    disc.addEllipse(QPointF(50, 50), 36, 36);

    QPainterPath bite;
    bite.addEllipse(QPointF(68, 36), 32, 32);

    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawPath(disc.subtracted(bite));
    painter->setBrush(Qt::NoBrush);
}

// A gutter of numbers beside text, which is what the setting actually shows.
void paintLineNumbers(QPainter *painter, const QColor &color)
{
    const qreal rows[] = { 26, 50, 74 };

    painter->setPen(strokePen(color, 8));
    painter->drawLine(QPointF(36, 12), QPointF(36, 88));

    for (int i = 0; i < 3; ++i) {
        const QString number = QString::number(i + 1);
        const QRectF box(6, rows[i] - 9, 22, 18);
        painter->setPen(color);
        painter->setFont(fittedFont(number, box, true, false, false));
        painter->drawText(box, Qt::AlignCenter, number);

        painter->setPen(strokePen(color, 8));
        painter->drawLine(QPointF(48, rows[i]), QPointF(90, rows[i]));
    }
}

void paintWordWrap(QPainter *painter, const QColor &color)
{
    painter->setPen(strokePen(color, 9));
    painter->drawLine(QPointF(12, 24), QPointF(88, 24));

    // Second line runs into a hook that turns back, i.e. wraps.
    painter->drawLine(QPointF(12, 52), QPointF(70, 52));
    QPainterPath hook;
    hook.moveTo(70, 52);
    hook.cubicTo(90, 52, 90, 78, 66, 78);
    painter->drawPath(hook);
    painter->drawLine(QPointF(66, 78), QPointF(40, 78));

    painter->setPen(color);
    drawArrowHead(painter, QPointF(32, 78), 180, 16);
}

void paintSyncScroll(QPainter *painter, const QColor &color)
{
    painter->setPen(strokePen(color, 8));
    painter->drawRect(QRectF(10, 16, 80, 68));
    painter->drawLine(QPointF(50, 16), QPointF(50, 84));

    painter->setPen(strokePen(color, 8));
    for (const qreal x : { 30.0, 70.0 }) {
        painter->drawLine(QPointF(x, 36), QPointF(x, 64));
        painter->setPen(color);
        drawArrowHead(painter, QPointF(x, 70), 90, 13);
        drawArrowHead(painter, QPointF(x, 30), -90, 13);
        painter->setPen(strokePen(color, 8));
    }
}

// Frame with a sun and a hill: the usual shorthand for a picture.
void paintImage(QPainter *painter, const QColor &color)
{
    painter->setPen(strokePen(color, 8));
    painter->drawRect(QRectF(10, 20, 80, 60));

    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawEllipse(QPointF(32, 40), 8, 8);

    QPainterPath hill;
    hill.moveTo(16, 76);
    hill.lineTo(42, 50);
    hill.lineTo(60, 68);
    hill.lineTo(72, 56);
    hill.lineTo(84, 76);
    hill.closeSubpath();
    painter->drawPath(hill);

    painter->setBrush(Qt::NoBrush);
}

void paintTable(QPainter *painter, const QColor &color)
{
    const QRectF frame(10, 20, 80, 60);

    painter->setPen(strokePen(color, 8));
    painter->drawRect(frame);

    // Heavier header rule, the way a table actually reads.
    painter->drawLine(QPointF(10, 40), QPointF(90, 40));

    painter->setPen(strokePen(color, 6));
    painter->drawLine(QPointF(10, 60), QPointF(90, 60));
    painter->drawLine(QPointF(37, 20), QPointF(37, 80));
    painter->drawLine(QPointF(63, 20), QPointF(63, 80));
}

void paintAbout(QPainter *painter, const QColor &color)
{
    painter->setPen(strokePen(color, 8));
    painter->drawEllipse(QPointF(50, 50), 38, 38);

    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawEllipse(QPointF(50, 28), 6, 6);
    painter->drawRoundedRect(QRectF(45, 42, 10, 32), 5, 5);
    painter->setBrush(Qt::NoBrush);
}

QPixmap renderPixmap(ActionIcons::Kind kind, const QColor &color, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.scale(size / kCanvas, size / kCanvas);
    painter.setBrush(Qt::NoBrush);

    using Kind = ActionIcons::Kind;
    switch (kind) {
    case Kind::New:           paintNew(&painter, color); break;
    case Kind::Open:          paintOpen(&painter, color); break;
    case Kind::Save:          paintSave(&painter, color); break;
    case Kind::SaveAs:        paintSaveAs(&painter, color); break;
    case Kind::Print:         paintPrint(&painter, color); break;
    case Kind::PrintPreview:  paintPrintPreview(&painter, color); break;
    case Kind::ExportHtml:    paintExportHtml(&painter, color); break;
    case Kind::ExportPdf:     paintExportPdf(&painter, color); break;
    case Kind::Exit:          paintExit(&painter, color); break;
    case Kind::Undo:          paintUndoRedo(&painter, color, false); break;
    case Kind::Redo:          paintUndoRedo(&painter, color, true); break;
    case Kind::Cut:           paintCut(&painter, color); break;
    case Kind::Copy:          paintCopy(&painter, color); break;
    case Kind::Paste:         paintPaste(&painter, color); break;
    case Kind::DarkPreview:   paintDarkPreview(&painter, color); break;
    case Kind::LineNumbers:   paintLineNumbers(&painter, color); break;
    case Kind::WordWrap:      paintWordWrap(&painter, color); break;
    case Kind::SyncScroll:    paintSyncScroll(&painter, color); break;
    case Kind::ZoomIn:        paintMagnifier(&painter, color, 1); break;
    case Kind::ZoomOut:       paintMagnifier(&painter, color, -1); break;
    case Kind::ResetZoom:     paintMagnifier(&painter, color, 0); break;
    case Kind::About:         paintAbout(&painter, color); break;
    case Kind::Bold:          drawGlyph(&painter, QStringLiteral("B"), color, true); break;
    case Kind::Italic:        drawGlyph(&painter, QStringLiteral("I"), color, false, true); break;
    case Kind::Strikethrough: paintStrikethrough(&painter, color); break;
    case Kind::InlineCode:    paintInlineCode(&painter, color); break;
    case Kind::Heading:       drawGlyph(&painter, QStringLiteral("H"), color, true); break;
    case Kind::Heading1:      drawGlyph(&painter, QStringLiteral("H1"), color, true); break;
    case Kind::Heading2:      drawGlyph(&painter, QStringLiteral("H2"), color, true); break;
    case Kind::Heading3:      drawGlyph(&painter, QStringLiteral("H3"), color, true); break;
    case Kind::Heading4:      drawGlyph(&painter, QStringLiteral("H4"), color, true); break;
    case Kind::Blockquote:    paintBlockquote(&painter, color); break;
    case Kind::Link:          paintLink(&painter, color); break;
    case Kind::Image:         paintImage(&painter, color); break;
    case Kind::Table:         paintTable(&painter, color); break;
    case Kind::EditorOnly:    paintPanes(&painter, color, true, false, false); break;
    case Kind::SplitView:     paintPanes(&painter, color, false, false, true); break;
    case Kind::PreviewOnly:   paintPanes(&painter, color, false, true, false); break;

    case Kind::BulletList:
        drawList(&painter, color, 42, [&color](QPainter *p, const QPointF &at, int) {
            p->save();
            p->setPen(Qt::NoPen);
            p->setBrush(color);
            p->drawEllipse(at, 7, 7);
            p->restore();
        });
        break;

    case Kind::NumberedList:
        // Digits need more room than bullets and must not crowd the rules,
        // hence the narrower glyph box and the later text start.
        drawList(&painter, color, 50, [&color](QPainter *p, const QPointF &at, int row) {
            const QString number = QString::number(row + 1);
            const QRectF box(at.x() - 11, at.y() - 9, 22, 18);
            p->save();
            p->setPen(color);
            p->setFont(fittedFont(number, box, true, false, false));
            p->drawText(box, Qt::AlignCenter, number);
            p->restore();
        });
        break;
    }

    return pixmap;
}

} // namespace

namespace ActionIcons {

QIcon icon(Kind kind, const QPalette &palette)
{
    const QColor color = palette.color(QPalette::ButtonText);

    QIcon result;
    for (const int size : kSizes)
        result.addPixmap(renderPixmap(kind, color, size));

    return result;
}

} // namespace ActionIcons
