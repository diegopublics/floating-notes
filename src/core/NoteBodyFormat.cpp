#include "NoteBodyFormat.h"

#include <QTextDocument>

namespace {
const QString RichTextPrefix = QStringLiteral("<!-- Floating Notes rich text -->\n");

QTextDocument::MarkdownFeatures markdownFeatures()
{
    QTextDocument::MarkdownFeatures features = QTextDocument::MarkdownDialectGitHub;
    features |= QTextDocument::MarkdownNoHTML;
    return features;
}
}

namespace NoteBodyFormat {

bool isRichText(const QString &storage)
{
    return storage.startsWith(RichTextPrefix);
}

void loadInto(QTextDocument *document, const QString &storage)
{
    if (isRichText(storage)) {
        document->setHtml(storage.mid(RichTextPrefix.size()));
        return;
    }
    document->setMarkdown(storage, markdownFeatures());
}

QString toMarkdown(const QString &storage)
{
    QTextDocument document;
    loadInto(&document, storage);
    return document.toMarkdown(markdownFeatures());
}

QString toPlainText(const QString &storage)
{
    QTextDocument document;
    loadInto(&document, storage);
    return document.toPlainText();
}

QString toStorage(const QTextDocument *document)
{
    return RichTextPrefix + document->toHtml();
}

}
