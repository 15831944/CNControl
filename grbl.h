#ifndef GRBL_H
#define GRBL_H

#define Grbl_version V1.0

#include <QTimer>
#include <QMap>
#include "port_serial.h"
#include "machine.h"

#define CMD_CONFIG     "$$"
#define CMD_INFOS      "$I"
#define CMD_GC         "$G"
#define CMD_GXX        "$#"
#define CMD_UNLOCK     "$X"
#define CMD_HOME       "$H"
#define CMD_STARTBLOCK "$N"
#define CMD_CHECK      "$C"

class Grbl : public Machine
{
    Q_OBJECT

    QTimer  statusTimer;

    QMap<uint, CoordinatesType> GxxConfig;
    CoordinatesType prbCoords;
    double TLOValue;

public:
    class StateType : public Machine::StateType
    {
//    public:
//        enum
//        {
//            Three = Machine::StateType::Last,
//            Four,
//            Five
//        };
    };

    class FeatureFlags : public Machine::FeatureFlags
    {
    public:
        enum {
            flagAskStatus = Machine::FeatureFlags::Last,
            flagWaitForReset,
            flagHasCoolantMist ,
            flagHasVariableSpindle,
            flagHasLaserMode,
            Last
        };
    };

    class InfoFlags : public Machine::InfoFlags
    {
    public:
        enum {
            flagHasConfig = Machine::InfoFlags::Last,
            flagHasStartingBlocks,
            flagHasLaserMode,
            flagTLO,
            flagPRB,
            flagGXX,
            flagGC,
            Last
        };
    };

    class ActionerFlags : public Machine::ActionerFlags
    {
    public:
        enum {
            actionSpindleVariable = Machine::InfoFlags::Last,
            actionCoolantFlood,
            actionCoolantMist,
            Last
        };
    };

    class SubCommandType
    {
    public:
        enum {
            commandReset,
            commandCoarsePlus,
            commandCoarseMinus,
            commandFinePlus,
            commandFineMinus,
            commandToggle,
            commandLow,
            commandMedium,
            commandStop,
            Last
        };
    };

    class CommandType : public Machine::CommandType
    {
    public:
        enum {
            commandStartBlock = Machine::CommandType::Last,
            commandDebugReport,
            commandJogCancel,
            commandSaftyDoor,
            commandCheck,

            commandOverrideFeed,
            commandOverrideRapid,
            commandOverrideSpindle,

            commandOverrideCoolantFloodToggle,
            commandOverrideCoolantMistToggle,

            Last
        };
    };

    class ConfigType : public Machine::ConfigType
    {
    public:
        enum {
            configStepPulse,            //  0
            configStepIdleDelay,        //  1
            configStepPortInvert,       //  2
            configDirPortInvert,        //  3
            configStepEnableInvert,     //  4
            configLimitPinInvert,       //  5
            configProbePinInvert,       //  6

            configStatusReport = 10,    // 10
            configJunctionDeviation,    // 11
            configArcTolerance,         // 12
            configReportInches,         // 13

            configSoftLimits = 20,      // 20
            configHardLimits,           // 21
            configHomingCycle,          // 22
            configHomingDirInvert,      // 23
            configHomingFeed,           // 24
            configHomingSeek,           // 25
            configHomingDebounce,       // 26
            configHomingPullOff,        // 27

            configMaxSpindleSpeed = 30, // 30
            configMinSpindleSpeed,      // 31
            configLaserMode,            // 32

            configXSteps = 100,         // 100
            configYSteps,               // 101
            configZSteps,               // 102

            configXMaxRate = 110,       // 110
            configYMaxRate,             // 111
            configZMaxRate,             // 112

            configXAcceleration = 120,  // 120
            configYAcceleration,        // 121
            configZAcceleration,        // 122

            configXMaxTravel = 130,     // 130
            configYMaxTravel,           // 131
            configZMaxTravel,           // 132
            configEnd,

            // Include starting blocks in config
            configStartingBlock0 = 500,
            configStartingBlock1,
            configStartingBlockEnd,
            Last
        };
    };

    class SwitchFlags : public Machine::SwitchFlags
    {
    public:
        enum {
            switchDoor,
            Last
        };
    };

    Grbl(Port *port);
    virtual ~Grbl();

    virtual QJsonObject toJsonObject();
    virtual QString toJson();

    virtual bool ask(int command, int arg = 0, bool noLog = false);

    void readErrorsMessages();
    void readAlarmsMessages();
    void readBuildOptionsMessages();
    void readSettingsMessages();

    virtual void setXWorkingZero();
    virtual void setYWorkingZero();
    virtual void setZWorkingZero();

protected:
    virtual void parseStatus(QString &);
    virtual void parseInfo(QString &line);
    virtual void parseConfig(QString &line);

public slots:
    void parse(QString &line);

private slots:
    void timeout();
    virtual void openConfiguration(QWidget *parent=nullptr);
    virtual void writeConfiguration(bool start = false);

signals:

    friend class GrblConfigurationDialog;
};

#endif // GRBL_H
