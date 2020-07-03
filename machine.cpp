#include "machine.h"
#include "ui_machine.h"
#include <QtDebug>
#include <QJsonDocument>

Machine::Machine(QWidget *parent) :
    QDialog(parent),
    uiMachine(new Ui::Machine)
{
    uiMachine->setupUi(this);


    features = infos = switches = actioners = 0;

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

//    this->port = port;
}

Machine::~Machine()
{
    delete uiMachine;
}

QMap<QString, Machine*> Machine::machines;
void Machine::registerMachine(Machine *machine)
{
    QString type = machine->getMachineType();
    if ( ! machines.contains( type ) )
    {
        qDebug() << "Machine::registerMachine(" << type << ")";
        machines[ type ] = machine;
    }
}

QJsonObject Machine::toJsonObject()
{
    QJsonObject json;
    json["type"] = "machine";
    json["zSafe"] = 2;
    json["center.x"] = 310;
    json["center.y"] = 360;
    json["center.z"] = 50;

    return json;
}

QString Machine::toJson()
{
    return QJsonDocument( toJsonObject() ).toJson();
}

void Machine::openConfiguration(QWidget *parent)
{

}

QString Machine::getMachineType()
{
    return machineType;
}

QString Machine::getMachineName()
{
    return machineName;
}


QString Machine::getMachineVersion()
{
    QString version = machineName;

    if (!version.isEmpty()) version += " ";
    version += machineVersion;

    if (!version.isEmpty()) version += ".";
    version += machineBuild;

    return version;
}
bool Machine::isState( int type ) { return state == type; };
int Machine::getState() { return state; };

const QString Machine::getStateMessages(int state) { return stateMessages.value(state, QString()); };
const Machine::ErrorMessageType Machine::getErrorMessages(int error) { return errorMessages.value(error, Machine::ErrorMessageType()); };
const Machine::AlarmMessageType Machine::getAlarmMessages(int alarm) { return alarmMessages.value(alarm, Machine::AlarmMessageType()); };
const Machine::BuildOptionMessageType Machine::getBuildOptionsMessages(char option) { return buildOptionMessages.value(option, BuildOptionMessageType()); };
const Machine::SettingMessageType Machine::getSettingMessages(int setting) { return settingMessages.value(setting, SettingMessageType()); };

quint64 Machine::getFeatures() { return features; };
bool Machine::hasFeature( int flag ) { return bitIsSet(features, flag); };

quint64 Machine::getInfos() { return infos; };
bool Machine::hasInfo( int flag ) { return bitIsSet(infos, flag); };

quint64 Machine::getSwitches() { return switches; };
bool Machine::hasSwitch(int flag) { return bitIsSet(switches, flag); };

quint64 Machine::getActions() { return actioners; };
bool Machine::hasAction(int flag) { return bitIsSet(actioners, flag); };

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

const QString &Machine::getLastLine() { return lastLine; };

bool Machine::sendCommand(QString gcode, bool withNewline, bool noLog)
{
    if (!port) return false;

    if ((gcode == '!') || (gcode == '~') || (gcode == '?'))
        withNewline = false;

    if (withNewline) gcode += "\n";

    if (!noLog)
    {
        if (gcode[0] <= 127)
            qDebug() << "Machine::sendCommand(" << gcode << ")";
        else
            qDebug() << "Machine::sendCommand(" << QString().setNum( static_cast<char>(gcode[0].toLatin1()), 16) << ")";
    }

    emit commandSent(gcode);
    return port->write(gcode.toLocal8Bit()) == gcode.size();
}
