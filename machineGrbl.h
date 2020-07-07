#ifndef MACHINEGRBL_H
#define MACHINEGRBL_H

#include <QTimer>
#include <QMap>

#include "machine.h"
#include "portSerial.h"

namespace Ui {
class MachineGrbl;
}

class MachineGrbl : public Machine, public SingletonFactory<MachineGrbl>
{
    Q_OBJECT

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
//            flagAskStatus ,
            flagWaitForReset= Machine::FeatureFlags::Last,
            flagHasCoolantMist ,
            flagHasVariableSpindle,
            flagHasLaserMode,
            flagHasLineNumbers,
            flagHasCoreXY,
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

//    explicit MachineGrbl(QJsonObject &configMachine, QWidget *parent = nullptr);
    explicit MachineGrbl(QWidget *parent = nullptr);
    ~MachineGrbl();

    virtual void openMachine(QString portName);
    virtual void closeMachine();

    virtual bool moveToX(double x, double feed, bool jog=false, bool machine=false, bool absolute=false);
    virtual bool moveToY(double y, double feed, bool jog=false, bool machine=false, bool absolute=false);
    virtual bool moveToXY(double x, double y, double feed, bool jog=false, bool machine=false, bool absolute=false);
    virtual bool moveToZ(double z, double feed, bool jog=false, bool machine=false, bool absolute=false);
    virtual bool moveToXYZ(double x, double y, double z, double feed, bool jog=false, bool machine=false, bool absolute=false);
    virtual bool moveTo(QVector3D &poin, double feed, bool jog, bool machine=false, bool absolute=false);
    virtual bool stopMove();

//    virtual QJsonObject toJsonObject();
//    virtual QString toJson();

    void setConfiguration();
    bool getConfiguration();

    virtual bool ask(int command, int arg = 0, bool noLog = false);

    void loadErrorsMessages();
    void loadAlarmsMessages();
    void loadBuildOptionsMessages();
    void loadSettingsMessages();
    void loadStatesMessages();

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
    virtual int openConfiguration();
    virtual void writeConfiguration();

private:
    Ui::MachineGrbl *ui;

    QTimer  statusTimer;
//    PortSerial serial;


    QMap<uint, QVector3D> GxxConfig;
    QVector3D prbCoords;
    double TLOValue;


};

#endif // MACHINEGRBL_H
