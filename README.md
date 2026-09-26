<img src="resources/markly.png" width="96" align="left" alt="Markly" />

# Markly

**A simple, fast Markdown editor and reader with live preview.**
Built with Qt 5 Widgets. No third-party dependencies — the Markdown parser,
the syntax highlighting and every toolbar icon are part of the source.

<br clear="left" />

## Features

- **Split view** — Markdown source on the left, rendered preview on the right.
  Switch to editor-only or preview-only at any time.
- **Live preview**, debounced so typing stays responsive on large documents, with
  scrolling synchronised between the two panes.
- **Editor niceties** — syntax highlighting, a line-number gutter, current-line
  highlight, and list continuation on Enter that understands numbered and task
  lists.
- **Insert dialogs** for links, images and tables. They take a web address or a
  local file, can write the path relative to the document, and show the Markdown
  they will produce before you accept it.
- **Light and dark preview themes.**
- **Drag and drop** — drop a `.md` or `.txt` file anywhere on the window to open
  it.
- **Print, print preview and export** to a standalone HTML file, or to PDF —
  always in the light theme, so a dark preview never sends dark-on-dark text to
  paper.
- **Find and replace** in the editor, with case-sensitive and whole-word
  matching.
- **English and Russian UI**, switchable at runtime from *View → Language*.
- Recent files and session state — window geometry, view mode, and every toggle
  — are remembered between runs.

### Keyboard shortcuts

| | | | |
|:---|:---|:---|:---|
| New | `Ctrl+N` | Bold | `Ctrl+B` |
| Open | `Ctrl+O` | Italic | `Ctrl+I` |
| Save | `Ctrl+S` | Strikethrough | `Ctrl+Shift+X` |
| Save As | `Ctrl+Shift+S` | Inline code | ``Ctrl+` `` |
| Print | `Ctrl+P` | Heading 1–4 | `Ctrl+1` … `Ctrl+4` |
| Find/Replace | `Ctrl+F` | Bullet list | `Ctrl+Shift+U` |
| Quit | `Ctrl+Q` | Numbered list | `Ctrl+Shift+O` |
| Editor only | `Ctrl+E` | Blockquote | `Ctrl+Shift+Q` |
| Split view | `Ctrl+D` | Link / Image / Table | `Ctrl+K` / `Ctrl+Shift+I` / `Ctrl+Shift+T` |
| Preview only | `Ctrl+R` | | |
| Zoom preview | `Ctrl+±`, `Ctrl+0` | | |

### Supported Markdown

ATX and setext headings, fenced and indented code blocks, blockquotes,
ordered / unordered / task lists including nesting, GFM tables, thematic breaks,
links, autolinks, images, emphasis, strong, strikethrough, inline code, hard
line breaks and backslash escapes. `samples/showcase.md` exercises all of it.

Inline HTML is passed through for a whitelist of formatting tags that Qt's rich
text engine actually renders — `<small>`, `<sup>`, `<kbd>`, `<span>`, `<br>` and
friends; see `isPassThroughTag` in [src/markdownparser.cpp](src/markdownparser.cpp).
HTML comments are dropped. Everything else, block level markup and `<script>`
included, stays visible as literal text: silently swallowing markup the renderer
cannot show would hide the author's own content from them.

## Building

Requires **Qt 5.13 or newer** and a C++14 compiler. Developed and tested with
Qt 5.13.0 and MinGW 7.3 (64-bit) on Windows 10. There is no platform-specific
code beyond the `win32`-guarded icon resource, but other platforms are untested.

```sh
mkdir build && cd build
qmake ../Markly.pro
make                # mingw32-make on Windows
```

The executable lands in `build/release/Markly.exe`, or `build/Markly` elsewhere.
It takes an optional file to open:

```sh
./Markly ../samples/showcase.md
```

### Tests

`tests/` is a subdirs project of QtTest suites. They are what makes this
codebase safe to change: the parser, the snippet generation and the trickier
widget wiring are all covered.

| Suite | Covers |
|:------|:-------|
| `parser/` | Markdown to HTML conversion |
| `snippets/` | The Markdown the insert dialogs generate |
| `preview/` | Preview theme switching |
| `dialogs/` | Insert dialog wiring |
| `gui/` | The window: file drops, toolbar wiring |

```sh
mkdir build-tests && cd build-tests
qmake ../tests/tests.pro
make
./parser/release/parsertest      # and the other four
```

### Regenerating the application icon

The icon is painted in code rather than kept as an opaque binary, so its shape
stays reviewable in a diff and every size is rendered at its own scale instead
of being downsampled from one bitmap — which is what keeps 16 px legible.

```sh
mkdir build-tools && cd build-tools
qmake ../tools/appicon/appicon.pro
make
./release/appicon ../resources          # writes markly.ico and markly.png
```

## Project layout

| Path | Responsibility |
|:-----|:---------------|
| `ui/*.ui` | Window and dialog layouts, actions, menus, toolbar |
| `src/markdownparser.cpp` | Markdown to HTML conversion |
| `src/markdownsnippets.cpp` | Markdown the insert dialogs generate |
| `src/markdownhighlighter.cpp` | Per-line highlighting of the source pane |
| `src/editor.cpp` | Source pane: gutter, list continuation, format helpers |
| `src/previewview.cpp` | Preview pane: theming, zoom, scroll anchoring |
| `src/actionicons.cpp` | Painted toolbar and menu icons |
| `src/translationmanager.cpp` | Loading and swapping translation catalogs |
| `src/finddialog.cpp` | Non-modal find/replace bar |
| `src/mainwindow.cpp` | Actions, file handling, settings, printing |
| `tools/appicon/` | Generator for the application icon |

---

# Implementation notes

The rest of this file is for anyone changing the code. Each section records a
Qt behaviour that is easy to get wrong and was got wrong at least once here.

## Why there is a Markdown parser in the repository

Qt 5.13 has no `QTextDocument::setMarkdown` — that arrived in 5.14 — and
QtWebEngine is not available for MinGW builds. So `src/markdownparser.cpp`
converts Markdown into the HTML subset that Qt's rich text engine understands,
and a `QTextBrowser` renders the result.

That subset is also the main constraint on the preview stylesheets in
`resources/`: Qt supports only part of CSS 2.1, so declarations stay limited to
colours, fonts, margins and simple borders.

## Designer forms, and what cannot live in them

The window and the three dialogs are Qt Designer forms. `ui/mainwindow.ui`
defines the widget tree, every `QAction` with its text and shortcut, the menus,
the toolbar and the status bar. `Editor` and `PreviewView` are promoted widgets,
so the layout stays editable in Designer.

Only what Designer cannot express lives in C++: the exclusive view-mode
`QActionGroup`, the status bar labels, the dynamic *Open Recent* entries, the
heading drop-down, and all signal wiring.

The heading levels are one toolbar button with a menu rather than four buttons.
Designer can neither attach a menu to a `QAction` nor reach the `QToolButton`
the toolbar builds for it, so `MainWindow` does both. The levels stay in
*Format → Heading* as well, which is where their `Ctrl+1`–`Ctrl+4` shortcuts get
their scope — taking them off the toolbar alone would have silently cost them
that.

## The preview theme

*View → Dark Preview* has to change two things, and missing either one leaves it
looking broken:

- the **stylesheet**, which colours the text. `QTextDocument` resolves the
  default stylesheet while it parses, so content that is already laid out keeps
  the old colours — the HTML has to be set again for a switch to take effect.
- the **widget palette**, which colours everything else. `QTextBrowser` takes
  its viewport background from `QPalette::Base`, never from CSS `body`, so the
  stylesheet alone would put light text on a white page.

`PreviewView::setTheme` does both. The colours passed to it must match
`body { background-color }` and `body { color }` in the stylesheet they sit next
to in `MainWindow`'s theme table.

Worth knowing when probing this in tests: Qt does not write `body { color }`
into the character format, so plain paragraph text reports black under either
theme. Probe a heading or inline code instead.

## Printing, print preview and PDF export

All three build their own `QTextDocument` in `MainWindow::preparePrintDocument`
rather than reusing the on-screen preview: the preview may be in the dark theme,
and printing whatever the screen shows would send dark-on-dark text to paper (or
to the PDF). The document is always built with the light stylesheet, regardless
of *View → Dark Preview*.

PDF export is `QPrinter::PdfFormat` with an output file name instead of a print
dialog — `QTextDocument::print()` does not care which one it is handed.

## Find and replace

`FindDialog` is non-modal — closing it while a search is mid-flight, or leaving
it open while editing, has to keep working — and one instance is kept alive for
the window's session so the search term and option checkboxes are not lost
between openings.

Replace relies on a property of `QTextCursor` that is easy to miss: inserting
text through a cursor obtained from `QPlainTextEdit::textCursor()` edits the
live document, and every other cursor attached to that document — including the
widget's own internal one — has its position adjusted automatically. That is
what lets `replaceAll()` call `find()` again immediately after each replacement
without manually re-synchronising the widget's cursor.

## Dropping files onto the window

Qt hands a drag to the innermost widget that accepts drops and does *not* retry
the parent when that widget rejects it. Both text panes' viewports accept drops
on their own, so `setAcceptDrops(true)` on the window alone would only work over
its frame. `MainWindow` therefore filters the two viewports' events and claims
the drag before they see it — but only when the payload is a file it can open,
so dragging text around inside the editor keeps working.

The file is opened from a zero-delay timer rather than inside the drop handler:
the drag source stays blocked until the handler returns, and the unsaved-changes
prompt must not run while it is.

## Insert dialogs

`LinkDialog`, `ImageDialog` and `TableDialog` are deliberately thin. Everything
that can be got wrong — escaping labels, encoding targets, laying out a padded
pipe table, and deciding whether a picked file becomes a relative path or an
absolute `file://` URL — lives in `src/markdownsnippets.cpp`, which has no UI
and is unit tested.

A local file is written relative to the document's folder when the option is
ticked. It is unavailable for an unsaved document, which has no folder to be
relative to, and it is skipped when the file and the document sit on different
Windows drives, where `QDir::relativeFilePath` returns nonsense rather than
failing.

## Icons

`src/actionicons.cpp` paints the toolbar and menu icons with `QPainter` instead
of loading image files. The letter icons (B, I, S, H1–H4) come out as real
typography in the application font rather than as hand-traced outlines, and each
icon is drawn in `QPalette::ButtonText`, so a dark system theme does not leave a
toolbar of black-on-black glyphs. `MainWindow` repaints them on
`QEvent::PaletteChange`.

Shapes are authored in a 100×100 logical box and rendered at 16, 20, 24, 32 and
48 px, so the style picks a size instead of rescaling one.

Two things that cost a round of rework, kept here so they are not rediscovered:
`QFont::setStrikeOut` is not honoured when the painter carries a scale, so the
strikethrough icon draws its bar by hand; and an arrow head narrower than the
stroke it terminates merges into the line and reads as a blob.

## Translations

English is the source language and has no catalog; every other language lives in
`translations/markly_<code>.ts`. `CONFIG += lrelease embed_translations` makes
qmake compile them to `.qm` at build time and embed them under `:/i18n`, so the
binary ships standalone.

Switching language installs a new `QTranslator`, which makes Qt post a
`LanguageChange` event; `MainWindow::changeEvent` calls `retranslateUi()` and
rebuilds the strings that are assembled at runtime — recent files, window title,
status bar. The choice is remembered in `QSettings`; on first run the system
locale decides.

Qt's own catalog (`qtbase_<code>.qm`, for standard dialog buttons) is loaded
from the Qt installation when available. It is optional; a missing one only
leaves those buttons in English.

To update the catalog after changing any UI string:

```sh
lupdate -no-obsolete Markly.pro
linguist translations/markly_ru.ts
```

`lupdate` only sees string literals passed directly to `tr()` — a
`tr(someVariable)` is silently skipped, and the string ends up untranslatable
with no warning.

### Adding a language

1. Add `translations/markly_<code>.ts` to `TRANSLATIONS` in `Markly.pro`
2. Run `lupdate` and translate
3. Add a checkable action to the `Language` menu in `ui/mainwindow.ui`
4. Register it in `MainWindow::connectLanguageActions()`
