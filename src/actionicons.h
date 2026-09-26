#ifndef ACTIONICONS_H
#define ACTIONICONS_H

#include <QIcon>

class QPalette;

// Toolbar and menu icons, painted at load time instead of shipped as image
// files.
//
// Two reasons for that: the letter icons (B, I, S, H1..H3) come out as real
// typography in the application font rather than as hand-traced outlines, and
// every icon takes its colour from the palette, so a dark system theme does not
// leave a toolbar of black-on-black glyphs.
namespace ActionIcons {

enum class Kind {
    New,
    Open,
    Save,
    SaveAs,
    Print,
    PrintPreview,
    ExportHtml,
    ExportPdf,
    Exit,
    Undo,
    Redo,
    Cut,
    Copy,
    Paste,
    Bold,
    Italic,
    Strikethrough,
    InlineCode,
    Heading,
    Heading1,
    Heading2,
    Heading3,
    Heading4,
    BulletList,
    NumberedList,
    Blockquote,
    Link,
    Image,
    Table,
    EditorOnly,
    SplitView,
    PreviewOnly,
    DarkPreview,
    LineNumbers,
    WordWrap,
    SyncScroll,
    ZoomIn,
    ZoomOut,
    ResetZoom,
    About
};

// Rendered at several sizes so the style can pick one without rescaling.
QIcon icon(Kind kind, const QPalette &palette);

} // namespace ActionIcons

#endif // ACTIONICONS_H
