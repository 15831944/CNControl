#include "logger.h"
#ifdef XXX
Logger::Logger()
{
    logLines = 50;
    logWidget = nullptr;
}

void Logger::SetLogWidget(QListWidget *logWidget)
{
    this->logWidget = logWidget;
    logWidget->setAutoScroll(false);
}

void Logger::SetLogLines(qint64 logLines)
{
    this->logLines = logLines;
}

#include <QDateTime>
void Logger::Log(QString msg)
{
    if (!logWidget) return;

    QString log;
    QDateTime date = QDateTime::currentDateTime();
    log = date.toString(Qt::RFC2822Date);
    log += ": ";
    log += msg;

    logWidget->addItem(log);

    // Remove old messages
    while (logWidget->count() > logLines)
        logWidget->removeItemWidget(logWidget->takeItem(0));

    // Set view at the end
    int item = logWidget->count();
    logWidget->scrollToItem( logWidget->itemAt(0, item - 1), QAbstractItemView::EnsureVisible );
}
#endif
