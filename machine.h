#ifndef MACHINE_H
#define MACHINE_H

#include <QDialog>
#include <QStringList>
#include <QMap>

#include "port.h"
#include "bits.h"

class Machine : public QObject
{    
    Q_OBJECT

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

    typedef struct CoordinatesType
    {
        double x;
        double y;
        double z;
        bool operator==(CoordinatesType coords)
        {
            return (coords.x == x) && (coords.y == y) && (coords.z == z);
        }
        bool operator!=(CoordinatesType coords)
        {
            return !(*this == coords);
        }
    } CoordinatesType;

protected:

    QString machineName;
    QString machineVersion;
    QString machineBuild;

    int state;

    quint64 features;
    quint64 infos;
    quint64 switches;
    quint64 actioners;

    int errorCode, alarmCode, holdCode, doorCode;

    CoordinatesType machineCoordinates;
    CoordinatesType workingCoordinates;

    int blockBuffer, rxBuffer;
    int blockBufferMax, rxBufferMax;
    int feedRate, spindleSpeed;

    CoordinatesType workingOffset;

    int lineNumber;
    int fOverride, rOverride, spindleSpeedOverride;

    QStringList errors, states;
    QString lastLine;

    QMap<uint, QString> config;

    Port *port;
public:
    Machine(Port *port);
    virtual ~Machine() {}

    virtual void openConfiguration(QWidget *parent)=0;

    virtual bool sendCommand(QString gcode, bool withNewline = true, bool noLog = false);
    virtual bool ask(int commandCode, int commandArg = 0, bool noLog = false) = 0;

    virtual bool isState( int type );
    virtual int getState();
    virtual const QString &getStateString();

    virtual QString getMachineVersion();

    virtual quint64 getFeatures();
    virtual bool hasFeature( int flag );

    virtual quint64 getInfos();
    virtual bool hasInfo( int flag );

    virtual quint64 getSwitches();
    virtual bool hasSwitch( int flag );

    virtual quint64 getActions();
    virtual bool hasAction( int flag );

    virtual CoordinatesType getMachineCoordinates();
    virtual CoordinatesType getWorkingCoordinates();

    virtual int getBlockBuffer();
    virtual int getRXBuffer();
    virtual int getBlockBufferMax();
    virtual int getRXBufferMax();

    virtual int getFeedRate();
    virtual int getSpindleSpeed();
    virtual CoordinatesType getWorkingOffset();
    virtual int getLineNumber();
//    virtual ??? getOverride();
    virtual int getErrorCode();
    virtual int getAlarmCode();
    virtual int getHoldCode();
    virtual int getDoorCode();

    virtual int getFOverride();
    virtual int getROverride();
    virtual int getSpindleSpeedOverride();

    virtual const QString &getErrorString(int error);

    virtual const QString &getLastLine();

    virtual void setXWorkingZero()=0;
    virtual void setYWorkingZero()=0;
    virtual void setZWorkingZero()=0;

public slots:
    virtual void parse(QString &line)=0;

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
    void commandSent(QString &gcode); // When a command is send to machine

};

#endif // MACHINE_H
