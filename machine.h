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

    class InfoFlags
    {
    public:
        enum {
            flagWaitForReset,
            flagHasMachineCoords,
            flagHasWorkingCoords,
            flagHasBuffer,
            flagHasFeedRate,
            flagHasSpindleSpeed,
            flagHasWorkingOffset,
            flagHasLineNumber,
            flagHasSwitches,
            flagHasOverride,
            flagHasActions,
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
            commandHome,
            commandStatus,
            commandConfig,
            commandInfos,
            commandPause,
            commandFeedHold,
            commandCycleStart,
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

    typedef struct
    {
        double x;
        double y;
        double z;
    } CoordinatesType;

protected:

    int state;

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

    QStringList errors, status;
    QString lastLine;

    QMap<uint, double> config;

    Port *port;
public:
    Machine(Port *port);
    virtual ~Machine() {}

    virtual void OpenConfiguration(QWidget *parent)=0;

    virtual bool sendGCode(QString gcode, bool withNewline = true, bool noLog = false);
    virtual bool ask(int commandCode, int commandArg = 0, bool noLog = false) = 0;

    virtual bool isState( int type );
    virtual int getState();
    virtual const QString &getStateString();

    virtual quint64 getInfos();
    virtual bool hasInfo( int type );

    virtual quint64 getSwitches();
    virtual bool hasSwitch( int type );

    virtual quint64 getActions();
    virtual bool hasAction( int type );

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

public slots:
    virtual void parse(QString &line)=0;

signals:
    void stateChanged();
    void titleUpdated();
    void statusUpdated();
    void configUpdated();
    void infoUpdated();
    void error(int errorCode);
    void commandExecuted();
    void command(int commandCode);

};

#endif // MACHINE_H
