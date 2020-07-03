#include "gcodehighlighter.h"

GCodeHighlighter::GCodeHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    lineNumberFormat.setForeground(Qt::cyan);
//    lineNumberFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("[Nn]\\d*");
    rule.format = lineNumberFormat;
    highlightingRules.append(rule);

    gCodeFormat.setForeground(Qt::red);
    gCodeFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("[Gg]\\d*");
    rule.format = gCodeFormat;
    highlightingRules.append(rule);

    mCodeFormat.setForeground(Qt::blue);
//    mCodeFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("[Mm]\\d*");
    rule.format = mCodeFormat;
    highlightingRules.append(rule);

    coordFormat.setForeground(Qt::darkMagenta);
//    coordFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("[XIAxia][-+]?\\d*(\\.\\d*)?");
    rule.format = coordFormat;
    highlightingRules.append(rule);

    coordFormat.setForeground(Qt::darkCyan);
//    coordFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("[YJByjb][-+]?\\d*(\\.\\d*)?");
    rule.format = coordFormat;
    highlightingRules.append(rule);

    coordFormat.setForeground(Qt::darkYellow);
//    coordFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("[ZKCzkc][-+]?\\d*(\\.\\d*)?");
    rule.format = coordFormat;
    highlightingRules.append(rule);

    feedFormat.setForeground(Qt::magenta);
//    feedFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("[Ff]\\d*(\\.\\d*)?");
    rule.format = feedFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression("[Ss]\\d*(\\.\\d*)?");
    rule.format = feedFormat;
    highlightingRules.append(rule);

    commentFormat.setForeground(Qt::darkGreen);
//    commentFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("^\\(.*\\)");
    rule.format = commentFormat;
    highlightingRules.append(rule);

    commentFormat.setForeground(Qt::lightGray);
    commentFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression(";.*");
    rule.format = commentFormat;
    highlightingRules.append(rule);
}

void GCodeHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : qAsConst(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

//    setCurrentBlockState(0);

//    int startIndex = 0;
//    if (previousBlockState() != 1)
//        startIndex = text.indexOf(commentStartExpression);

//    while (startIndex >= 0)
//    {
//        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
//        int endIndex = match.capturedStart();
//        int commentLength = 0;
//        if (endIndex == -1) {
//            setCurrentBlockState(1);
//            commentLength = text.length() - startIndex;
//        } else {
//            commentLength = endIndex - startIndex
//                          + match.capturedLength();
//        }
//        //setFormat(startIndex, commentLength, multiLineCommentFormat);
//        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
//    }
}
