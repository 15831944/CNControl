#ifndef PortSerial_H
#define PortSerial_H

#include <QSerialPort>
#include <QSerialPortInfo>

#include "port.h"

#define DEFAULT_SPEED       115200
#define DEFAULT_DATABITS    QSerialPort::Data8
#define DEFAULT_FLOWCONTROL QSerialPort::NoFlowControl
#define DEFAULT_PARITY      QSerialPort::NoParity
#define DEFAULT_STOPBITS    QSerialPort::OneStop

class PortSerial : public Port
{
    Q_OBJECT

    QSerialPortInfo info;
    QSerialPort serial;
    static QList<QSerialPortInfo> list;

    QString buffer;
public:
    PortSerial();
    virtual ~PortSerial();

    QSerialPortInfo getDeviceInfo( QString portName );
    void setDeviceInfo(QSerialPortInfo info);

    virtual bool setDevice(QString &portName);

    bool setOptions(qint32 speed = DEFAULT_SPEED,
              QSerialPort::DataBits dataBits = DEFAULT_DATABITS,
              QSerialPort::FlowControl flowControl = DEFAULT_FLOWCONTROL,
              QSerialPort::Parity parity = DEFAULT_PARITY,
              QSerialPort::StopBits stopBits = DEFAULT_STOPBITS );

    bool setSpeed(qint32 speed = DEFAULT_SPEED);
    bool setDataBits(QSerialPort::DataBits dataBits = DEFAULT_DATABITS);
    bool setFlowControl(QSerialPort::FlowControl flowControl = DEFAULT_FLOWCONTROL);
    bool setParity(QSerialPort::Parity parity = DEFAULT_PARITY);
    bool setStopBits(QSerialPort::StopBits stopBits = DEFAULT_STOPBITS);

    virtual bool isOpen();
    virtual bool open();
    virtual void close();
    virtual bool flush();
    static QStringList getDevices(void);
    virtual bool setProperty(const char *prop, QVariant &val);
    // prop IN ( 'speed', 'dataBits', 'flowControl', 'parity', 'stopBits' )
    virtual qint64 	write(const QByteArray &byteArray);

    virtual QString errorString();

private slots:
//    void readLine();
    void readyReadSlot();

signals:
    void lineAvailable(QString &line);
};

#endif // PortSerial_H
