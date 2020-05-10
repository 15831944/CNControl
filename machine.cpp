#include "machine.h"
#include "logger.h"

#include <QtDebug>
Machine::Machine(Port *port)
{
    infos = switches = actioners = 0;

    state = StateType::stateUnknown;

    errorCode = alarmCode = holdCode = doorCode = 0;

    machineCoordinates = {0,0,0};
    workingCoordinates = {0,0,0};

    blockBuffer = blockBufferMax = 0;
    rxBuffer = rxBufferMax = 0;
    feedRate = spindleSpeed = 0;

    workingOffset = {0,0,0};

    lineNumber = 0;
    fOverride = rOverride = spindleSpeedOverride = 0;

    this->port = port;

};

bool Machine::isState( int type ) { return state == type; };
int Machine::getState() { return state; };
const QString &Machine::getStateString() { return status.at(state); };

quint64 Machine::getInfos() { return infos; };
bool Machine::hasInfo( int flag ) { return (infos & bit(flag)); };

quint64 Machine::getSwitches() { return switches; };
bool Machine::hasSwitch(int type) { return bitIsSet(switches, type); };

quint64 Machine::getActions() { return actioners; };
bool Machine::hasAction(int type) { return bitIsSet(actioners, type); };

Machine::CoordinatesType Machine::getMachineCoordinates() { return machineCoordinates; };
Machine::CoordinatesType Machine::getWorkingCoordinates() { return workingCoordinates; };

int Machine::getBlockBuffer() { return blockBuffer; };
int Machine::getRXBuffer() { return rxBuffer; };

int Machine::getBlockBufferMax() { return blockBufferMax; };
int Machine::getRXBufferMax() { return rxBufferMax; };

int Machine::getFeedRate() { return feedRate; };
int Machine::getSpindleSpeed() { return spindleSpeed; };

Machine::CoordinatesType Machine::getWorkingOffset() { return workingOffset; };
int Machine::getLineNumber() { return lineNumber; };

int Machine::getErrorCode() { return errorCode; };
int Machine::getAlarmCode() { return alarmCode; };
int Machine::getHoldCode() { return holdCode; };
int Machine::getDoorCode() { return doorCode; };

int Machine::getFOverride() { return fOverride; };
int Machine::getROverride() { return rOverride; };
int Machine::getSpindleSpeedOverride() { return spindleSpeedOverride; };

const QString &Machine::getErrorString(int errorCode) { return errors.at(errorCode); };

const QString &Machine::getLastLine() { return lastLine; };

bool Machine::sendGCode(QString gcode, bool withNewline, bool noLog)
{
    if (!port) return false;


    if ((gcode == '!') || (gcode == '~') || (gcode == '?'))
        withNewline = false;

//    QByteArray cmd = gcode.toLocal8Bit();
    if (withNewline) gcode += "\n";

    if (!noLog)
        qDebug() << "Machine : Sending command " << gcode;

    return port->write(gcode.toLocal8Bit()) == gcode.size();
}

