
#define LOGGER_H
#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QListWidget>
#include "singleton.h"

#define sendLog(msg) Logger::Instance().Log(msg)
#define setLogWidget(widget) Logger::Instance().SetLogWidget(widget)
#define setLogLines(lines) Logger::Instance().SetLogLines(lines)

class Logger : public Singleton<Logger>
{
    QListWidget *logWidget;
    qint64 logLines;

    Logger();
public:
    void SetLogWidget(QListWidget *logger);
    void SetLogLines(qint64 logLines);

    void Log(QString msg);

    friend class Singleton<Logger>;
};

#endif // LOGGER_H
