# Markly

A **simple**, *fast* Markdown editor and reader with live preview.

## Inline formatting

Regular text with **bold**, *italic*, ***both***, ~~strikethrough~~ and
`inline code`. Escapes work too: \*not italic\*, and a literal backtick: \`.

Links: [Qt documentation](https://doc.qt.io/) and an autolink <https://commonmark.org>.

A hard line break follows this sentence.  
This line started after two trailing spaces.

## Lists

- First bullet
- Second bullet
  - Nested item
  - Another nested item
- Third bullet

1. Step one
2. Step two
3. Step three

### Task list

- [x] Write the Markdown parser
- [x] Add the live preview
- [ ] Add a document outline
- [ ] Add find and replace

## Blockquote

> Markdown is intended to be as easy-to-read and easy-to-write as is feasible.
> Readability is emphasized above all else.

## Code

Indented code:

    printf("hello, world\n");

Fenced code with a language tag:

```cpp
QString MarkdownParser::toHtml(const QString &markdown) const
{
    QString body;
    parseBlocks(markdown.split(QRegularExpression("\r\n|\n|\r")), 0, -1, &body);
    return body;
}
```

## Table

| Feature        | Shortcut       | Status |
|:---------------|:--------------:|-------:|
| Bold           | Ctrl+B         | done   |
| Italic         | Ctrl+I         | done   |
| Link           | Ctrl+K         | done   |
| Split view     | Ctrl+D         | done   |
| Find / Replace | Ctrl+F         | todo   |

## Headings

Setext H1
=========

Setext H2
---------

#### Level four
##### Level five
###### Level six

---

That thematic break above ends the showcase.
