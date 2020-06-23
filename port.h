#ifndef PORT_H
#define PORT_H

#include <QObject>
#include <QStringList>

class Port : public QObject
{
    Q_OBJECT
public:
    enum PortError {
        NoError,
        DeviceNotFoundError,
        PermissionError,
        OpenError,
        NotOpenError,
        ParityError,
        FramingError,
        BreakConditionError,
        WriteError,
        ReadError,
        ResourceError,
        UnsupportedOperationError,
        TimeoutError,
        UnknownError
    };

    Port();
    virtual ~Port();

    virtual bool isOpen() = 0;
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool flush() = 0;
    virtual QStringList getDevices() = 0;
    virtual bool setDevice(QString &portName) = 0;

    virtual bool setProperty(const char *prop, QVariant &val);
    virtual qint64 	write(const QByteArray &) = 0;

    virtual QString errorString() = 0;

signals:
    void lineAvailable(QString &line);
    void error(Port::PortError error);
};

#endif // PORT_H
