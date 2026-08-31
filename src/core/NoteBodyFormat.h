#pragma once

#include <QString>

class QTextDocument;

namespace NoteBodyFormat {

bool isRichText(const QString &storage);
void loadInto(QTextDocument *document, const QString &storage);
QString toMarkdown(const QString &storage);
QString toPlainText(const QString &storage);
QString toStorage(const QTextDocument *document);

}
