#include <string.h>
#include "grbl.h"

#include <QDebug>
#include "logger.h"

Grbl::Grbl(Port *port) :
    Machine(port)
{
    // Initialize status list
    status << "Unknown" <<
            "Idle"      <<
            "Run"       <<
            "Hold"      <<
            "Jog"       <<
            "Home"      <<
            "Alarm"     <<
            "Check"     <<
            "Door"      <<
            "Sleep";

    // Initialize error list
    errors << "Ok"                               <<    // 0
              "Command letter missing"           <<    // 1
              "Bad number format"                <<    // 2
              "Invalid statement"                <<    // 3
              "Negative value"                   <<    // 4
              "Setting disabled"                 <<    // 5
              "Setting step pulse min"           <<    // 6
              "Setting read fail"                <<    // 7
              "Idle error"                       <<    // 8
              "System GC lock"                   <<    // 9
              "Soft limit error"                 <<    // 10
              "Overflow"                         <<    // 11
              "Max step rate exceeded"           <<    // 12
              "Check door"                       <<    // 13
              "Line length exceeded"             <<    // 14
              "Travel exceeded"                  <<    // 15
              "Invalid Jog command"              <<    // 16
              "Setting disabled (laser)"         <<    // 17
              "Error 18"                         <<    // 18
              "Error 19"                         <<    // 19
              "Gcode: Unsupported command"       <<    // 20
              "Gcode: Modal group violation"     <<    // 21
              "Gcode: Undefined feed rate"       <<    // 22
              "Gcode: Command value not integer" <<    // 23
              "Gcode: Axis command conflict"     <<    // 24
              "Gcode: Word repeated"             <<    // 25
              "Gcode: No axis words"             <<    // 26
              "Gcode: Invalid line number"       <<    // 27
              "Gcode: Value word missing"        <<    // 28
              "Gcode: Unsupported coords system" <<    // 29
              "Gcode: G53 invalid motion mode"   <<    // 30
              "Gcode: Axis word exists"          <<    // 31
              "Gcode: No axis word in plane"     <<    // 32
              "Gcode: Invalid target"            <<    // 33
              "Gcode: Arc radius error"          <<    // 34
              "Gcode: No offsets in plane"       <<    // 35
              "Gcode: Unused words"              <<    // 36
              "Gcode: G43 dynamic axis error"    <<    // 37
              "Gcode: Max value exceeded";             // 38

    infos = bit(InfoFlags::flagWaitForReset) | bit(InfoFlags::flagHasSwitches);

    connect( port, SIGNAL(lineAvailable(QString&)), this, SLOT(parse(QString&)));
    connect( &statusTimer, &QTimer::timeout, this, &Grbl::timeout);
    // does not work : connect( &statusTimer, SIGNAL(timeout()), this, SLOT(askStatus()));

    statusTimer.start(200);
    qDebug() << "Grbl : machine initialized.";

    ask(CommandType::commandInfos);
}

Grbl::~Grbl()
{
    statusTimer.stop();
    qDebug() << "Grbl : machine deleted.";
}

#include "grblconfiguration.h"
void Grbl::OpenConfiguration(QWidget *parent)
{
    GrblConfiguration grblConfig(parent);
}

QMap<uint, double> Grbl::getConfig() { return config; };

void Grbl::timeout()
{
    ask(CommandType::commandStatus, 0, true);
}

bool Grbl::ask(int commandCode, int commandArg, bool noLog)
{
    QString cmd;
    bool newLine = true;

    switch (commandCode)
    {
    case CommandType::commandReset:
        cmd = CMD_RESET;
        infos = bit(InfoFlags::flagWaitForReset);
        switches = 0;
        actioners = 0;
        newLine = false;
        break;

    case CommandType::commandStatus:
        cmd = static_cast<char>(CMD_STATUS_REPORT);
        newLine = false;
        break;

    case CommandType::commandUnlock:
        cmd = CMD_UNLOCK;
        break;

    case CommandType::commandHome:
        cmd = CMD_HOME;
        break;

    case CommandType::commandConfig:
        cmd = CMD_CONFIG;
        break;

    case CommandType::commandInfos:
        cmd = CMD_INFOS;
        break;

    case CommandType::commandPause:
        if (commandArg)
            cmd = static_cast<char>(CMD_FEED_HOLD);
        else
            cmd = static_cast<char>(CMD_CYCLE_START);
        newLine = false;
        break;

    case CommandType::commandFeedHold:
        cmd = static_cast<char>(CMD_FEED_HOLD);
        newLine = false;
        break;

    case CommandType::commandCycleStart:
        cmd = static_cast<char>(CMD_CYCLE_START);
        newLine = false;
        break;

    case CommandType::commandDebugReport:
        cmd = static_cast<char>(CMD_DEBUG_REPORT);
        newLine = false;
        break;

    case CommandType::commandJogCancel:
        cmd = static_cast<char>(CMD_JOG_CANCEL);
        newLine = false;
        break;

    case CommandType::commandSaftyDoor:
        cmd = static_cast<char>(CMD_SAFETY_DOOR);
        newLine = false;
        break;

    case CommandType::commandOverrideFeed:
        switch(commandArg)
        {
        case SubCommandType::commandReset:
            cmd = static_cast<char>(CMD_FEED_OVR_RESET);
            break;
        case SubCommandType::commandCoarsePlus:
            cmd = static_cast<char>(CMD_FEED_OVR_COARSE_PLUS);
            break;
        case SubCommandType::commandCoarseMinus:
            cmd = static_cast<char>(CMD_FEED_OVR_COARSE_MINUS);
            break;
        case SubCommandType::commandFinePlus:
            cmd = static_cast<char>(CMD_FEED_OVR_FINE_PLUS);
            break;
        case SubCommandType::commandFineMinus:
            cmd = static_cast<char>(CMD_FEED_OVR_FINE_MINUS);
            break;
        default:
            qDebug() << "Grbl error : commandOverrideFeed has no subcommand " << commandArg;
        }
        newLine = false;
        break;

    case CommandType::commandOverrideRapid:
        switch(commandArg)
        {
        case SubCommandType::commandReset:
            cmd = static_cast<char>(CMD_RAPID_OVR_RESET);
            break;
        case SubCommandType::commandLow:
            cmd = static_cast<char>(CMD_RAPID_OVR_LOW);
            break;
        case SubCommandType::commandMedium:
            cmd = static_cast<char>(CMD_RAPID_OVR_MEDIUM);
            break;
        default:
            qDebug() << "Grbl error : commandOverrideRapid has no subcommand " << commandArg;
        }
        newLine = false;
        break;

    case CommandType::commandOverrideSpindle:
        switch(commandArg)
        {
        case SubCommandType::commandReset:
            cmd = static_cast<char>(CMD_SPINDLE_OVR_RESET);
            break;
        case SubCommandType::commandCoarsePlus:
            cmd = static_cast<char>(CMD_SPINDLE_OVR_COARSE_PLUS);
            break;
        case SubCommandType::commandCoarseMinus:
            cmd = static_cast<char>(CMD_SPINDLE_OVR_COARSE_MINUS);
            break;
        case SubCommandType::commandFinePlus:
            cmd = static_cast<char>(CMD_SPINDLE_OVR_FINE_PLUS);
            break;
        case SubCommandType::commandFineMinus:
            cmd = static_cast<char>(CMD_SPINDLE_OVR_FINE_MINUS);
            break;
        case SubCommandType::commandStop:
            cmd = static_cast<char>(CMD_SPINDLE_OVR_STOP);
            break;
        default:
            qDebug() << "Grbl error : commandOverrideSpindle has no subcommand " << commandArg;
        }
        newLine = false;
        break;

    case CommandType::commandOverrideCoolantFloodToggle:
        cmd = static_cast<char>(CMD_COOLANT_FLOOD_OVR_TOGGLE);
        newLine = false;
        break;

    case CommandType::commandOverrideCoolantMistToggle:
        cmd = static_cast<char>(CMD_COOLANT_MIST_OVR_TOGGLE);
        newLine = false;
        break;
    }

    if (sendGCode(cmd, newLine, noLog))
    {
        emit command(commandCode);
        return true;
    }
    return false;
}

void Grbl::parse(QString &line)
{
    lastLine = line;
    //qDebug() << "flagWaitForReset2:" << bitIsSet(infos,flagWaitForReset);
    //qDebug() << "line" << bitIsSet(infos, flagWaitForReset) << ": " << line;

    // Parse message
    if (line.startsWith("Grbl"))
    {
        QStringList blocks = line.split(" ", QString::KeepEmptyParts, Qt::CaseInsensitive);
        qDebug() << blocks.at(0).toUtf8() << " " << blocks.at(1).toUtf8();

        // Problem : When clicking on reset switch, multiple reset occurs.
        //           Informations are asked multiple times (4 times).
        //           It works, but that take plenty of time
        ask(CommandType::commandInfos);

        emit titleUpdated();
    }
    else if (line.startsWith("ok"))
    {
        bitClear(infos, InfoFlags::flagHasError);
        bitClear(infos, InfoFlags::flagWaitForReset);
        emit commandExecuted();
    }
    else if (line.startsWith("error:"))
    {
        QString reste = line.right( line.size() - 6 );
        errorCode = reste.toInt();
        qDebug() << QString("Grbl error %1 : %2 ").arg( errorCode ).arg( errors[ errorCode ] ).toUtf8().data();
        emit error(errorCode);
    }
    else if (line.startsWith('[')) // This is an information
    {
        parseInfo(line);
        emit infoUpdated();
    }
    else if (line.startsWith('<') ) // This is status
    {
        parseStatus(line);
        emit statusUpdated();
    }
    else if (line.startsWith('$') ) // This is config
    {
        parseConfig(line);
        emit configUpdated();
    }
    else if (!line.isEmpty()) qDebug() << QString("Grbl error: unknown line '%1'.").arg(line).toUtf8().data();
}


void Grbl::parseInfo(QString &line)
{
    QString block = line.right(line.size()-1); // remove '[' char
    block = block.left(block.size()-1); // remove ']' char
    qDebug() << line;

    if (block.startsWith("TLO:"))
    {
        block = block.right( block.size() - 4 );
        TLOValue = block.toDouble();
        qDebug() << "Grbl TLO found.";
        bitSet(infos, InfoFlags::flagTLO);
    }
    else if (block.startsWith("PRB:"))
    {
        block = block.right( block.size() - 4 );
        QStringList vals;
        vals = block.split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
        if ( vals.size() == 3 )
        {
            prbCoords.x = vals.at(0).toDouble();
            prbCoords.y = vals.at(1).toDouble();
            prbCoords.z = vals.at(2).toDouble();
            qDebug() << "Grbl PRB found.";
            bitSet(infos, InfoFlags::flagPRB);
        }
        else qDebug() << "Grbl PRB: incorrect format: " << block;
    }
    else if (block.startsWith("GC:"))
    {
        block = block.right( block.size() - 3 );
        QStringList blocks;
        blocks = block.split(" ", QString::KeepEmptyParts, Qt::CaseInsensitive);
        for (int i=0; i<blocks.size(); i++)
        {
            QString command = blocks.at(i);
            if (command == "G20") bitClear(infos, InfoFlags::flagIsMilimeters);
            if (command == "G21") bitSet(infos, InfoFlags::flagIsMilimeters );

            if (command == "G90") bitClear(infos, InfoFlags::flagIsAbsolute);
            if (command == "G91") bitSet(infos, InfoFlags::flagIsAbsolute );

            // The rest is not really relevant
        }
        qDebug() << "Grbl GC found.";
        bitSet(infos, InfoFlags::flagGC);
    }
    else if (block.startsWith("VER:"))
    {
        block = block.right( block.size() - 4 );
        // What to do with that ?
        qDebug() << "Grbl VER found.";
    }
    else if (block.startsWith("OPT:"))
    {
        block = block.right( block.size() - 4 );
        QStringList vals;
        vals = block.split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
        if ( vals.size() == 3 )
        {
            // These values are possible, but we don't care about them for now.
//            if (vals.at(0).contains('V')); // variable spindle
//            if (vals.at(0).contains('N')); // line numbers

            if (vals.at(0).contains('M'))
                bitSet(infos, InfoFlags::flagHasCoolantMist);

            // These values are possible, but we don't care about them for now.
//            if (vals.at(0).contains('C')); // coreXY
//            if (vals.at(0).contains('P')); // Parking
//            if (vals.at(0).contains('Z')); // Home force set origin
//            if (vals.at(0).contains('H')); // Home single axis command
//            if (vals.at(0).contains('T')); // Limits two switches on axes
//            if (vals.at(0).contains('A')); // Allow feed override during probe cycle
//            if (vals.at(0).contains('D')); // Spindle dir as enable pin
//            if (vals.at(0).contains('0')); // Enable off with zero speed
//            if (vals.at(0).contains('S')); // Software debounce
//            if (vals.at(0).contains('R')); // Parking override control
//            if (vals.at(0).contains('L')); // Homing init lock
//            if (vals.at(0).contains('+')); // Safety Door input pin
//            if (vals.at(0).contains('*')); // RESTORE_EEPROM_WIPE_ALL
//            if (vals.at(0).contains('$')); // RESTORE_EEPROM_DEFAULT_SETTINGS
//            if (vals.at(0).contains('#')); // RESTORE_EEPROM_CLEAR_PARAMETERS
//            if (vals.at(0).contains('I')); // BUILD_INFO_WRITE_COMMAND
//            if (vals.at(0).contains('E')); // BUFFER_SYNC_DURING_EEPROM_WRITE
//            if (vals.at(0).contains('W')); // BUFFER_SYNC_DURING_WCO_CHANGE
//            if (vals.at(0).contains('2')); // Dual axis

            blockBufferMax = vals.at(1).toInt();
            rxBufferMax = vals.at(2).toInt();

            qDebug() << "Grbl OPT found.";
        }
        else qDebug() << "Grbl OPT: incorrect format: " << block;
    }
    else if (block.startsWith('G'))
    {
        block = block.right( block.size() - 1 );
        QStringList blocks, vals;
        blocks = block.split(":", QString::KeepEmptyParts, Qt::CaseInsensitive);
        if (blocks.size() == 2 ) // Security
        {
            uint gCode = blocks.at(0).toUInt();

            vals = blocks.at(1).split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
            if ( vals.size() == 3 )
            {
                CoordinatesType coords;
                coords.x = vals.at(0).toDouble();
                coords.y = vals.at(1).toDouble();
                coords.z = vals.at(2).toDouble();

                GxxConfig[ gCode ] = coords;
                qDebug() << "Grbl G" << gCode << " found.";
                bitSet(infos, InfoFlags::flagGXX);
            }
            else qDebug() << "Grbl G" << gCode << " incorrect coordinates: " << block;
        }
        else qDebug() << "Grbl GXX: incorrect format: " << block;
    }
}

#define flagKeep ( bit(InfoFlags::flagHasWorkingOffset) | bit(InfoFlags::flagHasSwitches) | bit(InfoFlags::flagWaitForReset) )
void Grbl::parseStatus(QString &line)
{
    infos &= flagKeep;
    switches = 0;

    QString buffer = line.right(line.size()-1); // remove '<' char
    buffer = buffer.left(buffer.size()-1); // remove '>' char

    QStringList blocks = buffer.split("|", QString::KeepEmptyParts, Qt::CaseInsensitive);

    QString block = blocks.at(0);
    int newstate = StateType::stateUnknown;

    if ( block == "Idle" ) newstate = StateType::stateIdle;
    if ( block == "Run" ) newstate = StateType::stateRun;
    if ( block.startsWith("Hold:") )
    {
        block = block.right( block.size() - 5 );
        holdCode = block.toInt();
        newstate = StateType::stateHold;
    }
    if ( block == "Jog" ) newstate = StateType::stateJog;
    if ( block == "Home" ) newstate = StateType::stateHome;
    if ( block == "Alarm") newstate = StateType::stateAlarm;
    if ( block == "Check" ) newstate = StateType::stateCheck;
    if ( block.startsWith("Door:"))
    {
        block = block.right( block.size() - 5 );
        doorCode = block.toInt();
        newstate = StateType::stateDoor;
    }

    if ( block == "Sleep" ) newstate = StateType::stateSleep;

    if (state != newstate)
    {
        state = newstate;
        emit stateChanged();
    }

    for (int i=1; i < blocks.size(); i++)
    {
     QStringList vals;
     block = blocks.at(i);
     if ( block.startsWith("WPos:") ) // Working Position
     {
        block = block.right( block.size() - 5 );
        vals = block.split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
        if ( vals.size() == 3 )
        {
            workingCoordinates.x = vals.at(0).toDouble();
            workingCoordinates.y = vals.at(1).toDouble();
            workingCoordinates.z = vals.at(2).toDouble();
            bitSet(infos, InfoFlags::flagHasWorkingCoords);
        } else qDebug() << "Grbl statusError : " << block;

     } else if ( block.startsWith("MPos:") ) // Machine Position
     {
        block = block.right( block.size() - 5 );
        vals = block.split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
        if ( vals.size() == 3 )
        {
            machineCoordinates.x = vals.at(0).toDouble();
            machineCoordinates.y = vals.at(1).toDouble();
            machineCoordinates.z = vals.at(2).toDouble();
            bitSet(infos, InfoFlags::flagHasMachineCoords);
        } else qDebug() << "Grbl statusError : " << block;

     } else if ( block.startsWith("Bf:" ) ) // Buffer State
     {
        block = block.right( block.size() - 3 );
        vals = block.split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
        if ( vals.size() == 2 )
        {
            blockBuffer = vals.at(0).toInt();
            rxBuffer = vals.at(1).toInt();
            bitSet(infos, InfoFlags::flagHasBuffer);
        } else qDebug() << "Grbl statusError : " << block;

     } else if ( block.startsWith("Ln:") ) // Line Numbers
     {
        block = block.right( block.size() - 3 );
        vals = block.split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
        if ( vals.size() == 1 )
        {
            lineNumber = vals.at(0).toInt();
            bitSet(infos, InfoFlags::flagHasLineNumber);
        } else qDebug() << "Grbl statusError : " << block;

     } else if ( block.startsWith("FS:") ) // FeedRate & SpindleSpeed
     {
        block = block.right( block.size() - 3 );
        vals = block.split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
        if ( vals.size() == 2 )
        {
            feedRate = vals.at(0).toInt();
            spindleSpeed = vals.at(1).toInt();
            bitSet(infos, InfoFlags::flagHasFeedRate);
            bitSet(infos, InfoFlags::flagHasSpindleSpeed);
        } else qDebug() << "Grbl statusError : " << block;

     } else if ( block.startsWith("F:") ) // FeedRate alone
     {
        block = block.right( block.size() - 2 );
        feedRate = block.toInt();
        bitSet(infos, InfoFlags::flagHasFeedRate);

     } else if ( block.startsWith("Pn:") ) // Pin states
     {
        block = block.right( block.size() - 3 );
        if (block.contains('P', Qt::CaseInsensitive))
            bitSet(switches, SwitchFlags::switchProbe);

        if (block.contains('X', Qt::CaseInsensitive))
            bitSet(switches, SwitchFlags::switchLimitX);
        if (block.contains('Y', Qt::CaseInsensitive))
            bitSet(switches, SwitchFlags::switchLimitY);
        if (block.contains('Z', Qt::CaseInsensitive))
            bitSet(switches, SwitchFlags::switchLimitZ);

        if (block.contains('D', Qt::CaseInsensitive))
            bitSet(switches, SwitchFlags::switchDoor);
        if (block.contains('R', Qt::CaseInsensitive))
            bitSet(switches, SwitchFlags::switchReset);
        if (block.contains('H', Qt::CaseInsensitive))
            bitSet(switches, SwitchFlags::switchFeedHold);
        if (block.contains('S', Qt::CaseInsensitive))
            bitSet(switches, SwitchFlags::switchCycleStart);

        bitSet(infos, InfoFlags::flagHasSwitches);

     } else if ( block.startsWith("WCO:") ) // Working Coord Offset
     {
        block = block.right( block.size() - 4 );
        vals = block.split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
        if ( vals.size() == 3 )
        {
            workingOffset.x = vals.at(0).toDouble();
            workingOffset.y = vals.at(1).toDouble();
            workingOffset.z = vals.at(2).toDouble();
            bitSet(infos, InfoFlags::flagHasWorkingOffset);
        } else qDebug() << "Grbl statusError : " << block;

     } else if ( block.startsWith("Ov:") ) // Overrides
     {
        block = block.right( block.size() - 3 );
        vals = block.split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
        if ( vals.size() == 3 )
        {
            fOverride = vals.at(0).toInt();
            rOverride = vals.at(1).toInt();
            spindleSpeedOverride = vals.at(2).toInt();
            bitSet(infos, InfoFlags::flagHasOverride);
            // When Ov: is present, actions are following if any.
            bitSet(infos, InfoFlags::flagHasActions);
            actioners = 0;
         } else qDebug() << "Grbl statusError : " << block;

    } else if ( block.startsWith("A:") ) // Overrides
    {
        block = block.right( block.size() - 2 );
        if (block.contains("SS", Qt::CaseInsensitive))
        {
            bitSet(actioners, ActionerFlags::actionSpindleVariable);
            bitSet(actioners, ActionerFlags::actionSpindle);
        }
        else if (block.contains("SC", Qt::CaseInsensitive))
        {
            bitSet(actioners, ActionerFlags::actionSpindleVariable);
            bitSet(actioners, ActionerFlags::actionSpindle);
            bitSet(actioners, ActionerFlags::actionSpindleCounterClockwise);
        }
        else
        {
            if (block.contains('S', Qt::CaseInsensitive))
                bitSet(actioners, ActionerFlags::actionSpindle);

            if (block.contains('C', Qt::CaseInsensitive))
            {
                bitSet(actioners, ActionerFlags::actionSpindle);
                bitSet(actioners, ActionerFlags::actionSpindleCounterClockwise);
            }
        }

        if (block.contains('F', Qt::CaseInsensitive))
        {
            bitSet(actioners, ActionerFlags::actionCoolant);
            bitSet(actioners, ActionerFlags::actionCoolantFlood);
        }
        if (block.contains('M', Qt::CaseInsensitive))
        {
            bitSet(actioners, ActionerFlags::actionCoolant);
            bitSet(actioners, ActionerFlags::actionCoolantMist);
        }

        } else qDebug() << "Grbl statusError : " << block;
    }

    // Compute coordinates not given in status
    if (bitIsSet(infos, InfoFlags::flagHasWorkingOffset) && bitIsClear(infos, InfoFlags::flagHasMachineCoords))
    {
        machineCoordinates.x = workingCoordinates.x + workingOffset.x;
        machineCoordinates.y = workingCoordinates.y + workingOffset.y;
        machineCoordinates.z = workingCoordinates.z + workingOffset.z;
        bitSet(infos, InfoFlags::flagHasMachineCoords);
    }

    if (bitIsSet(infos, InfoFlags::flagHasMachineCoords) && bitIsClear(infos, InfoFlags::flagHasWorkingCoords))
    {
        workingCoordinates.z = machineCoordinates.x - workingOffset.x;
        workingCoordinates.y = machineCoordinates.y - workingOffset.y;
        workingCoordinates.z = machineCoordinates.z - workingOffset.z;
        bitSet(infos, InfoFlags::flagHasWorkingCoords);
    }
}

void Grbl::parseConfig(QString &line)
{
    if (line.startsWith('$'))
    {
        QString block = line.right(line.size() - 1);
        QStringList vals = block.split("=", QString::KeepEmptyParts, Qt::CaseInsensitive);
        if ( vals.size() == 2 )
        {
            config[ vals.at(0).toUInt() ] = vals.at(1).toDouble();
            bitSet(infos, Grbl::InfoFlags::flagHasConfig);
        }
        else qDebug() << "Grbl statusError : " << block;
    }
}
