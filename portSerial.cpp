#include "portSerial.h"

#include <QByteArray>
#include <QDebug>

PortSerial::PortSerial() : Port ()
{
    setSpeed();
    setDataBits();
    setFlowControl();
    setParity();
    setStopBits();

    qDebug() << "SerialPort : Port initialized.";
}

PortSerial::~PortSerial()
{
    close();
    qDebug() << "SerialPort : Port deleted.";
}

QList<QSerialPortInfo> PortSerial::list;

QStringList PortSerial::getDevices()
{
    QSerialPortInfo info;
    QStringList infos;

    list = info.availablePorts();

    infos.clear();
    for (int i=0; i < list.size(); i++)
        infos.append( list.at(i).portName() );

    return infos;
}

QSerialPortInfo PortSerial::getDeviceInfo( QString portName )
{
    for (QSerialPortInfo item : list)
    {
        if (item.portName() == portName)
        {
            info = item;
            break;
        }
    }
    return info;
}

void PortSerial::setDeviceInfo(QSerialPortInfo info)
{
    serial.setPort(info);
}

bool PortSerial::setDevice(QString &portName)
{
    QSerialPortInfo deviceInfo = getDeviceInfo( portName );
    setDeviceInfo(deviceInfo);
    return true;
}

bool PortSerial::setOptions(qint32 speed,
          QSerialPort::DataBits dataBits,
          QSerialPort::FlowControl flowControl,
          QSerialPort::Parity parity,
          QSerialPort::StopBits stopBits )
{
    if (!setSpeed(speed)) return false;
    if (!setDataBits(dataBits)) return false;
    if (!setFlowControl(flowControl)) return false;
    if (!setParity(parity)) return false;
    if (!setStopBits(stopBits)) return false;
    return true;
};

bool PortSerial::setSpeed(qint32 speed)
{
    //int i = serial.setBaudRate(speed, QSerialPort::AllDirections);
    if(!serial.setBaudRate(speed, QSerialPort::AllDirections))
        return true;
    if (serial.error() == QSerialPort::NoError)
        return true;

    qDebug() << "SerialPort : setSpeed : " << serial.errorString().toUtf8().data();
    return false;
}

bool PortSerial::setDataBits(QSerialPort::DataBits dataBits)
{
    if(!serial.setDataBits(dataBits))
        return true;
    if (serial.error() == QSerialPort::NoError)
        return true;

    qDebug() << "SerialPort : setDataBits : " << serial.errorString().toUtf8().data();
    return false;
};

bool PortSerial::setFlowControl(QSerialPort::FlowControl flowControl)
{
    if(!serial.setFlowControl(flowControl))
        return true;
    if (serial.error() == QSerialPort::NoError)
        return true;

    qDebug() << "SerialPort : setFlowControl : " << serial.errorString().toUtf8().data();
    return false;
};

bool PortSerial::setParity(QSerialPort::Parity parity)
{
    if(!serial.setParity(parity))
        return true;
    if (serial.error() == QSerialPort::NoError)
        return true;

    qDebug() << "SerialPort : setParity : " << serial.errorString().toUtf8().data();
    return false;
};

bool PortSerial::setStopBits(QSerialPort::StopBits stopBits)
{
    if(!serial.setStopBits(stopBits))
        return true;
    if (serial.error() == QSerialPort::NoError)
        return true;

    qDebug() << "SerialPort : setStopBits : " << serial.errorString().toUtf8().data();
    return false;
};

bool PortSerial::setProperty(const char *prop, QVariant &val)
{
    bool res;
    if (!strcmp(prop, "speed")) setSpeed(val.toInt(&res));
    else if (!strcmp(prop, "dataBits")) setDataBits(static_cast<QSerialPort::DataBits>(val.toInt(&res)));
    else if (!strcmp(prop, "flowControl")) setFlowControl(static_cast<QSerialPort::FlowControl>(val.toInt(&res)));
    else if (!strcmp(prop, "parity")) setParity(static_cast<QSerialPort::Parity>(val.toInt(&res)));
    else if (!strcmp(prop, "stopBits")) setStopBits(static_cast<QSerialPort::StopBits>(val.toInt(&res)));
    else res = QObject::setProperty(prop, val);
    return res;
}

bool PortSerial::isOpen()
{
    return serial.isOpen();
};

bool PortSerial::open()
{
    if (serial.open(QIODevice::ReadWrite))
    {
        serial.flush();
        connect(&serial, &QIODevice::readyRead, this, &PortSerial::readyReadSlot);
        qDebug() << "SerialPort : Port opened.";

        return true;
    }

    qDebug() << serial.errorString();
    return false;
}

void PortSerial::close()
{
    serial.close();
    qDebug() << "SerialPort : Port closed.";
}

bool PortSerial::flush()
{
    return serial.flush();
};

qint64 PortSerial::write(const QByteArray &byteArray)
{
    qint64 res = serial.write(byteArray);
    serial.flush();
    return res;
}

QString PortSerial::errorString()
{
    return serial.errorString();
}

void PortSerial::readyReadSlot()
{
    while (!serial.atEnd()) {
        QByteArray data = serial.readLine();
        buffer.append(data.trimmed());

        if (data.contains('\n'))
        {
            //qDebug() << "SerialPort::readLine: End of line (" << buffer << ")";
            emit lineAvailable(  buffer );
            buffer.clear();
        }
    }
}
