#include "port.h"

Port::Port() {};
Port::~Port() {};

bool Port::setProperty(const char *prop, QVariant &val)
{
    return QObject::setProperty(prop,val);
}

