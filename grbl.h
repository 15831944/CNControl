#ifndef GRBL_H
#define GRBL_H

#define Grbl_version V1.0

#include <QTimer>
#include "port_serial.h"
#include "machine.h"

// From grbl config.h, must be synchronized.
// -8<----------------------------------------------------------------------------------
#define CMD_RESET 0x18 // ctrl-x.
#define CMD_STATUS_REPORT '?'
#define CMD_CYCLE_START '~'
#define CMD_FEED_HOLD '!'

// NOTE: All override realtime commands must be in the extended ASCII character set, starting
// at character value 128 (0x80) and up to 255 (0xFF). If the normal set of realtime commands,
// such as status reports, feed hold, reset, and cycle start, are moved to the extended set
// space, serial.c's RX ISR will need to be modified to accomodate the change.
// #define CMD_RESET 0x80
// #define CMD_STATUS_REPORT 0x81
// #define CMD_CYCLE_START 0x82
// #define CMD_FEED_HOLD 0x83
#define CMD_SAFETY_DOOR 0x84
#define CMD_JOG_CANCEL  0x85
#define CMD_DEBUG_REPORT 0x86 // Only when DEBUG enabled, sends debug report in '{}' braces.
#define CMD_FEED_OVR_RESET 0x90         // Restores feed override value to 100%.
#define CMD_FEED_OVR_COARSE_PLUS 0x91
#define CMD_FEED_OVR_COARSE_MINUS 0x92
#define CMD_FEED_OVR_FINE_PLUS  0x93
#define CMD_FEED_OVR_FINE_MINUS  0x94
#define CMD_RAPID_OVR_RESET 0x95        // Restores rapid override value to 100%.
#define CMD_RAPID_OVR_MEDIUM 0x96
#define CMD_RAPID_OVR_LOW 0x97
// #define CMD_RAPID_OVR_EXTRA_LOW 0x98 // *NOT SUPPORTED*
#define CMD_SPINDLE_OVR_RESET 0x99      // Restores spindle override value to 100%.
#define CMD_SPINDLE_OVR_COARSE_PLUS 0x9A
#define CMD_SPINDLE_OVR_COARSE_MINUS 0x9B
#define CMD_SPINDLE_OVR_FINE_PLUS 0x9C
#define CMD_SPINDLE_OVR_FINE_MINUS 0x9D
#define CMD_SPINDLE_OVR_STOP 0x9E
#define CMD_COOLANT_FLOOD_OVR_TOGGLE 0xA0
#define CMD_COOLANT_MIST_OVR_TOGGLE 0xA1
// -8<-----------------------------------------------------------------------------------

#define CMD_CONFIG "$$"
#define CMD_INFOS "$I"
#define CMD_GC "$G"
#define CMD_GXX "$#"
#define CMD_UNLOCK "$X"
#define CMD_HOME "$H"
#define CMD_STARTBLOCK "$N"
#define CMD_CHECK "$c"

#include <QMap>

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

    virtual bool ask(int command, int arg = 0, bool noLog = false);

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

    friend class GrblConfiguration;
};

#endif // GRBL_H
