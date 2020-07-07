#ifndef MACHINE_H
#define MACHINE_H

#include <QDialog>
#include <QString>
#include <QList>
#include <QMap>
#include <QJsonObject>
#include <QVector3D>
#include <QTabWidget>

#include "port.h"
#include "bits.h"
#include "singletonFactory.h"

namespace Ui {
class Machine;
}

class Machine : public QDialog, public SingletonFactory<Machine>
{
    Q_OBJECT

protected:
    class ErrorMessageType
    {
    public:
        int errorCode;
        QString shortMessage;
        QString longMessage;
    };

    class AlarmMessageType
    {
    public:
        int alarmCode;
        QString shortMessage;
        QString longMessage;
    };

    class SettingMessageType
    {
    public:
        int settingCode;
        QString setting;
        QString unit;
        QString description;
    };

    class BuildOptionMessageType
    {
    public:
        char buildOptionCode;
        QString description;
    };

public:

    class StateType
    {
    public:
        enum {
            stateUnknown,
            stateIdle,
            stateRun,
            stateHold,
            stateJog,
            stateHome,
            stateAlarm,
            stateCheck,
            stateDoor,
            stateSleep,
            Last,
        };
    };

    class FeatureFlags
    {
    public:
        enum {
            flagName,
            flagVersion,
            flagBuild,
            Last
        };
    };

    class InfoFlags
    {
    public:
        enum {
            flagHasMachineCoords,
            flagHasWorkingCoords,
            flagHasBuffer,
            flagHasFeedRate,
            flagHasSpindleSpeed,
            flagHasWorkingOffset,
            flagHasLineNumber,
            flagHasSwitches,
            flagHasOverride,
            flagHasActioners,
            flagHasError,
            flagHasAlarm,

            flagIsMilimeters,
            flagIsAbsolute,
            Last
        };
    };

    class ActionerFlags
    {
    public:
        enum {
            actionSpindle,
            actionSpindleCounterClockwise,
            actionCoolant,
            Last
        };
    };

    class CommandType
    {
    public:
        enum {
            commandReset,
            commandUnlock,
            commandHoming,
            commandStatus,
            commandConfig,
            commandInfos,
            commandPause,
            commandFeedHold,
            commandCycleStart,
            commandHomeMachine,
            commandHomeWorking,
            Last
        };
    };

    class SwitchFlags
    {
    public:
        enum {
            switchLimitX,
            switchLimitY,
            switchLimitZ,
            switchProbe,
            switchDoor,
            switchFeedHold,
            switchCycleStart,
            switchReset,
            Last
        };
    };

    class ConfigType
    {
    public:
        enum {
            Last
        };
    };

//    typedef struct CoordinatesType
//    {
//        double x;
//        double y;
//        double z;
//        bool operator==(CoordinatesType coords)
//        {
//            return (coords.x == x) && (coords.y == y) && (coords.z == z);
//        }
//        bool operator!=(CoordinatesType coords)
//        {
//            return !(*this == coords);
//        }
//    } CoordinatesType;

protected:

    QString machineType;
    QString machineName;
    QString machineVersion;
    QString machineBuild;

    int state;

    quint64 features;
    quint64 infos;
    quint64 switches;
    quint64 actioners;

    int errorCode, alarmCode, holdCode, doorCode;

    QVector3D machineCoordinates;
    QVector3D workingCoordinates;
    QVector3D workingOffset;

    int blockBuffer, rxBuffer;
    int blockBufferMax, rxBufferMax;
    int feedRate, spindleSpeed;

    int lineNumber;
    int fOverride, rOverride, spindleSpeedOverride;

    QMap<int, ErrorMessageType> errorMessages;
    QMap<int, AlarmMessageType> alarmMessages;
    QMap<int, SettingMessageType> settingMessages;
    QMap<int, BuildOptionMessageType> buildOptionMessages;
    QMap<int, QString> stateMessages;

    QString lastLine;

    QMap<int, QString> config;
    //QJsonObject &config;

    Port *port;

public:
//    explicit Machine(QJsonObject &configMachine, QWidget *parent = nullptr);
    explicit Machine(QWidget *parent = nullptr);
    ~Machine();

    virtual void openMachine(QString portName)=0;
    virtual void closeMachine()=0;

//    virtual QJsonObject toJsonObject();
//    virtual QString toJson();

    virtual bool moveToX(double x, double feed, bool jog=false, bool machine=false, bool absolute=false)=0;
    virtual bool moveToY(double y, double feed, bool jog=false, bool machine=false, bool absolute=false)=0;
    virtual bool moveToXY(double x, double y, double feed, bool jog=false, bool machine=false, bool absolute=false)=0;
    virtual bool moveToZ(double z, double feed, bool jog=false, bool machine=false, bool absolute=false)=0;
    virtual bool moveToXYZ(double x, double y, double z, double feed, bool jog=false, bool machine=false, bool absolute=false)=0;
    virtual bool moveTo(QVector3D &poin, double feed, bool jog, bool machine=false, bool absolute=false)=0;
    virtual bool stopMove()=0;

    virtual bool sendCommand(QString gcode, bool withNewline = true, bool noLog = false);
    virtual bool ask(int commandCode, int commandArg = 0, bool noLog = false) = 0;

    void setMachineConfigurationWidget(QTabWidget *configTabWidget);

    // Message functions
    virtual const ErrorMessageType getErrorMessages(int error);
    virtual const AlarmMessageType getAlarmMessages(int alarm);
    virtual const BuildOptionMessageType getBuildOptionsMessages(char option);
    virtual const SettingMessageType getSettingMessages(int setting);
    virtual const QString getStateMessages(int state);

    virtual bool isState( int type );
    virtual int getState();

    virtual QString getMachineType();
    virtual QString getMachineName();
    virtual QString getMachineVersion();

    virtual quint64 getFeatures();
    virtual bool hasFeature( int flag );

    virtual quint64 getInfos();
    virtual bool hasInfo( int flag );

    virtual quint64 getSwitches();
    virtual bool hasSwitch( int flag );

    virtual quint64 getActions();
    virtual bool hasAction( int flag );

    virtual QVector3D getMachineCoordinates();
    virtual QVector3D getWorkingCoordinates();
    virtual QVector3D getWorkingOffset();

    virtual int getBlockBuffer();
    virtual int getRXBuffer();
    virtual int getBlockBufferMax();
    virtual int getRXBufferMax();

    virtual int getFeedRate();
    virtual int getSpindleSpeed();
    virtual int getLineNumber();
//    virtual ??? getOverride();
    virtual int getErrorCode();
    virtual int getAlarmCode();
    virtual int getHoldCode();
    virtual int getDoorCode();

    virtual int getFOverride();
    virtual int getROverride();
    virtual int getSpindleSpeedOverride();

    virtual const QString &getLastLine();

    virtual void setXWorkingZero()=0;
    virtual void setYWorkingZero()=0;
    virtual void setZWorkingZero()=0;

public slots:
    virtual void parse(QString &line)=0;
    virtual int openConfiguration();

signals:
    void versionUpdated(); // When name or version has been found or changed
    void statusUpdated(); // When status of machine has been received (changed or not)

    void stateUpdated();
    void resetDone();
    void lineNumberUpdated();
    void coordinatesUpdated();
    void switchesUpdated();
    void actionersUpdated();
    void ratesUpdated();
    void buffersUpdated();

    void configUpdated(); // When configuration has been received
    void infoUpdated();   // When information has been received
    void error(int errorCode); // When error has been received
    void alarm(int alarmCode); // When alarm has been received
    void commandExecuted();    // When command has been accepted (not necesserally executed !!!)

    void infoReceived(QString line); // When a command is received from the machine
    void commandSent(QString line); // When a command is send to machine

private:
    Ui::Machine *ui;
};

#include <QException>

class machineConnectException : public QException
{
public:
    void raise() const override { throw *this; }
    machineConnectException *clone() const override { return new machineConnectException(*this); }
};

#endif // MACHINE_H
