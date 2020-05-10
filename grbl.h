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
    public:
        enum
        {
            Three = Machine::StateType::Last,
            Four,
            Five
        };
    };

    class InfoFlags : public Machine::InfoFlags
    {
    public:
        enum {
            flagHasCoolantMist = Machine::InfoFlags::Last,
            flagHasConfig,
            flagTLO,
            flagPRB,
            flagGXX,
            flagGC,
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
            commandDebugReport = Machine::CommandType::Last,
            commandJogCancel,
            commandSaftyDoor,

            commandOverrideFeed,
            commandOverrideRapid,
            commandOverrideSpindle,

            commandOverrideCoolantFloodToggle,
            commandOverrideCoolantMistToggle,

            Last
        };
    };

    class SwitchFlags : public Machine::SwitchFlags
    {
    public:
        enum {
            switchDoor,
        };
    };


    Grbl(Port *port);
    virtual ~Grbl();

    virtual void OpenConfiguration(QWidget *parent);

    virtual QMap<uint, double> getConfig();
    virtual bool ask(int command, int arg = 0, bool noLog = false);

protected:
    virtual void parseStatus(QString &);
    virtual void parseInfo(QString &line);
    virtual void parseConfig(QString &line);

public slots:
    void parse(QString &line);

private slots:
    void timeout();

signals:

};

#endif // GRBL_H
