#include "port.h"

Port::Port() {};
Port::~Port() {};

bool Port::setProperty(const char *prop, QVariant &val)
{
    return QObject::setProperty(prop,val);
}

qint64 Port::write(const QByteArray &)
{
    return 0;
}
