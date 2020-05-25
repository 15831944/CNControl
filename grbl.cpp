#include <string.h>
#include "grbl.h"

#include <QDebug>

Grbl::Grbl(Port *device) :
    Machine(device)
{
    machineName = "GRBL";

    // Initialize status list
    states << tr("Unknown","Grbl state") <<
              tr("Idle","Grbl state")    <<
              tr("Run","Grbl state")     <<
              tr("Hold","Grbl state")    <<
              tr("Jog","Grbl state")     <<
              tr("Home","Grbl state")    <<
              tr("Alarm","Grbl state")   <<
              tr("Check","Grbl state")   <<
              tr("Door","Grbl state")    <<
              tr("Sleep","Grbl state");

    // Initialize error list
    errors << tr("Ok","Grbl Error Message")                               <<    // 0
              tr("Command letter missing","Grbl Error Message")           <<    // 1
              tr("Bad number format","Grbl Error Message")                <<    // 2
              tr("Invalid statement","Grbl Error Message")                <<    // 3
              tr("Negative value","Grbl Error Message")                   <<    // 4
              tr("Setting disabled","Grbl Error Message")                 <<    // 5
              tr("Setting step pulse min","Grbl Error Message")           <<    // 6
              tr("Setting read fail","Grbl Error Message")                <<    // 7
              tr("Idle error","Grbl Error Message")                       <<    // 8
              tr("System GC lock","Grbl Error Message")                   <<    // 9
              tr("Soft limit error","Grbl Error Message")                 <<    // 10
              tr("Overflow","Grbl Error Message")                         <<    // 11
              tr("Max step rate exceeded","Grbl Error Message")           <<    // 12
              tr("Check door","Grbl Error Message")                       <<    // 13
              tr("Line length exceeded","Grbl Error Message")             <<    // 14
              tr("Travel exceeded","Grbl Error Message")                  <<    // 15
              tr("Invalid Jog command","Grbl Error Message")              <<    // 16
              tr("Setting disabled (laser)","Grbl Error Message")         <<    // 17
              tr("Error 18","Grbl Error Message")                         <<    // 18
              tr("Error 19","Grbl Error Message")                         <<    // 19
              tr("Gcode: Unsupported command","Grbl Error Message")       <<    // 20
              tr("Gcode: Modal group violation","Grbl Error Message")     <<    // 21
              tr("Gcode: Undefined feed rate","Grbl Error Message")       <<    // 22
              tr("Gcode: Command value not integer","Grbl Error Message") <<    // 23
              tr("Gcode: Axis command conflict","Grbl Error Message")     <<    // 24
              tr("Gcode: Word repeated","Grbl Error Message")             <<    // 25
              tr("Gcode: No axis words","Grbl Error Message")             <<    // 26
              tr("Gcode: Invalid line number","Grbl Error Message")       <<    // 27
              tr("Gcode: Value word missing","Grbl Error Message")        <<    // 28
              tr("Gcode: Unsupported coords system","Grbl Error Message") <<    // 29
              tr("Gcode: G53 invalid motion mode","Grbl Error Message")   <<    // 30
              tr("Gcode: Axis word exists","Grbl Error Message")          <<    // 31
              tr("Gcode: No axis word in plane","Grbl Error Message")     <<    // 32
              tr("Gcode: Invalid target","Grbl Error Message")            <<    // 33
              tr("Gcode: Arc radius error","Grbl Error Message")          <<    // 34
              tr("Gcode: No offsets in plane","Grbl Error Message")       <<    // 35
              tr("Gcode: Unused words","Grbl Error Message")              <<    // 36
              tr("Gcode: G43 dynamic axis error","Grbl Error Message")    <<    // 37
              tr("Gcode: Max value exceeded","Grbl Error Message");             // 38

    features = bit(FeatureFlags::flagAskStatus) |
               bit(FeatureFlags::flagName);

//    bitSet(infos, Machine::InfoFlags::flagHasWorkingCoords);
//    bitSet(infos, Machine::InfoFlags::flagHasMachineCoords);

    // Get command line from port
    connect( port, SIGNAL(lineAvailable(QString&)), this, SLOT(parse(QString&)));

    // Start by asking informations for version
    ask(CommandType::commandInfos);

    // Ask for status regularely (5Hz)
    connect( &statusTimer, &QTimer::timeout, this, &Grbl::timeout);
    statusTimer.start(200);

    qDebug() << "Grbl : machine initialized.";
}

Grbl::~Grbl()
{
    statusTimer.stop();
    qDebug() << "Grbl : machine deleted.";
}

#include <QIcon>
#include <QMessageBox>
#include "grblconfiguration.h"
//#define hasConfig (config.size() >= Grbl::ConfigType::configEnd)
//#define hasStartingBlocks (config.size() >= Grbl::ConfigType::configStartingBlockEnd)

void Grbl::openConfiguration(QWidget *parent)
{
    static QWidget *widget;
    static QMessageBox *waitMessage;

    static int timeoutTries = 0;

    if (!isState(StateType::stateIdle) && !isState(StateType::stateAlarm))
    {
        qDebug() << "Grbl::openConfiguration: Error : machine is not Idle or Alarm";
        QMessageBox::information(parent, "Error", "Machine is not Idle or Alarm.");

    }
    else if (parent)
    {
        // First call, ask for configuration
        widget = parent;
        timeoutTries = 10;

        config.clear();
        bitClear(features, FeatureFlags::flagAskStatus);
        connect( this, SIGNAL(commandExecuted()), this, SLOT(openConfiguration()));

        ask(Grbl::CommandType::commandConfig);
        qDebug() << "Grbl::openConfiguration: Asking for configuration.";

//        waitMessage = new QMessageBox( parent );
//        //waitMessage->setAttribute( Qt::WA_DeleteOnClose ); //makes sure the msgbox is deleted automatically when closed
//        waitMessage->setStandardButtons( QMessageBox::NoButton );
//        waitMessage->setWindowTitle( tr("Retreiving Grbl Conguration") );
//        waitMessage->setText( tr("Please wait while retreiving configuration...") );
//        waitMessage->setModal( true ); // if you want it non-modal
//        waitMessage->open( this, nullptr);
    }
    else
    {
        if (!hasInfo( InfoFlags::flagHasStartingBlocks))
        {
            ask(Grbl::CommandType::commandStartBlock);
            qDebug() << "Grbl::openConfiguration: Asking for starting blocks.";
            return;
        }

        // Call when command executed
        delete waitMessage;
        disconnect( this, SIGNAL(commandExecuted()), this, SLOT(openConfiguration()));

        qDebug() << "Grbl::openConfiguration: Got configuration and starting blocks.";
        bitSet(features, FeatureFlags::flagAskStatus);

        // Open the configuration Dialog
        GrblConfiguration grblConfig(widget);
        // Use config to populate configuration dialog
        grblConfig.setConfiguration(this);
        grblConfig.exec();
//        grblConfig.open(this,nullptr);
        if (grblConfig.result() == QDialog::Accepted)
        {
            if (grblConfig.getConfiguration(this))
                writeConfiguration(true);
        }
        else bitSet(features, FeatureFlags::flagAskStatus);
    }
}

void Grbl::writeConfiguration(bool start)
{
    static int index = 0;

    if (start)
    {
        // First call, initialize
//        if (grblConfig.getConfiguration(this))
//        {
            index = 0;
            bitClear(features, FeatureFlags::flagAskStatus);
            connect( this, SIGNAL(commandExecuted()), this, SLOT(writeConfiguration()));
            qDebug() << "Grbl::writeConfiguration: Start sending configuration.";
//        }
//        else qDebug << "Grbl::writeConfiguration: Error getting configuration.";
    }

    if ( index < config.size() )
    {
        uint key = config.keys().at(index++);

        // Warning, setting LaserMode($32) when disabled generate an error
        if ((key == Grbl::ConfigType::configLaserMode) && !hasFeature( Grbl::InfoFlags::flagHasLaserMode))
        {
            //qDebug()<< "Grbl::writeConfiguration: Ignoring LaserMode";
            writeConfiguration();
        }
        else {
            QString val = config[key];
            QString cmd;
            switch (key)
            {
            case Grbl::ConfigType::configStartingBlock0:
            case Grbl::ConfigType::configStartingBlock1:
                cmd = QString("$N%1=%2").arg(key - Grbl::ConfigType::configStartingBlock0).arg(val);
                break;
            default:
                cmd = QString("$%1=%2").arg(key).arg(val);
            }
            qDebug() << "Grbl::writeConfiguration: " << cmd;
            if (!sendCommand(cmd))
            {
                disconnect( this, SIGNAL(commandExecuted()), this, SLOT(writeConfiguration()));
                qDebug() << "Grbl::writeConfiguration: Error sending configuration.";
            }
        }
    }
    else {
        disconnect( this, SIGNAL(commandExecuted()), this, SLOT(writeConfiguration()));
        qDebug() << "Grbl::writeConfiguration: configuration set.";
        bitSet(features, FeatureFlags::flagAskStatus);
    }
}

// ----------------------------------------------------------------------------------
bool Grbl::ask(int commandCode, int commandArg, bool noLog)
{
    QString cmd;
    bool newLine = true;

    switch (commandCode)
    {
    case CommandType::commandReset:
        cmd = CMD_RESET;
        switches = 0;
        actioners = 0;
        newLine = false;
        // Grbl resets working offset on reset
        bitClear(infos, InfoFlags::flagHasWorkingOffset);
        break;

    case CommandType::commandStatus:
        cmd = static_cast<char>(CMD_STATUS_REPORT);
        newLine = false;
        break;

    case CommandType::commandUnlock:
        cmd = CMD_UNLOCK;
        break;

    case CommandType::commandHoming:
        cmd = CMD_HOME;
        break;

    case CommandType::commandConfig:
        cmd = CMD_CONFIG;
        break;

    case CommandType::commandInfos:
        cmd = CMD_INFOS;
        break;

    case CommandType::commandStartBlock:
        cmd = CMD_STARTBLOCK;
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

    case CommandType::commandCheck:
        cmd = CMD_CHECK;
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

    if (cmd.isEmpty())
    {
        qDebug() << "Grbl::ask: Command is empty.";
        return false;
    }

    return sendCommand(cmd, newLine, noLog);
}

void Grbl::timeout()
{
    if (hasFeature( FeatureFlags::flagAskStatus))
        ask(CommandType::commandStatus, 0, true);
}

void Grbl::parse(QString &line)
{
    lastLine = line;

    //qDebug() << "line=" << line;
    // Parse message
    if (line.startsWith("Grbl"))
    {
        QStringList blocks = line.split(" ", QString::KeepEmptyParts, Qt::CaseInsensitive);

        machineName = blocks.at(0);
        qDebug() << "Grbl::parse: Machine name " << machineName;
        bitSet(features, FeatureFlags::flagName);

        machineVersion = blocks.at(1);
        qDebug() << "Grbl::parse: Partial version " << machineVersion;
        bitSet(features, FeatureFlags::flagVersion);
        emit versionUpdated();

        // Problem : When clicking on reset switch, multiple reset occurs.
        //           Informations are asked multiple times (4 times).
        //           It works, but that take plenty of time
        ask(CommandType::commandInfos);

        emit resetDone();
    }
    else if (line.startsWith("ok"))
    {
        bitClear(infos, InfoFlags::flagHasError);
        //qDebug() << "Grbl::parse: Command executed.";
        emit commandExecuted();
    }
    else if (line.startsWith("error:"))
    {
        QString block = line.right( line.size() - 6 );
        errorCode = block.toInt();
        qDebug() << QString("Grbl::parse: Error %1 : %2 ").arg( errorCode ).arg( errors[ errorCode ] ).toUtf8().data();
        emit error(errorCode);
    }
    else if (line.startsWith("ALARM:"))
    {
        QString block = line.right( line.size() - 6 );
        alarmCode = block.toInt();
        //state = StateType::stateAlarm;
        emit alarm(alarmCode);
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
    else if (line.startsWith('>') ) // This is status
    {
        // This is a line execution, probably starting blocks.
        // Do nothing about that.
    }    else if (line.startsWith('$') ) // This is config
    {
        parseConfig(line);
        emit configUpdated();
    }
    else if (!line.isEmpty()) qDebug() << QString("Grbl error: unknown line '%1'.").arg(line).toUtf8().data();
}

// ----------------------------------------------------------------------------------
void Grbl::parseInfo(QString &line)
{
    QString block = line.right(line.size()-1); // remove '[' char
    block = block.left(block.size()-1); // remove ']' char

    if (block.startsWith("TLO:"))
    {
        block = block.right( block.size() - 4 );
        TLOValue = block.toDouble();
        //qDebug() << "Grbl TLO found.";
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
            //qDebug() << "Grbl PRB found.";
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

            // The rest is not really relevant for now.
        }
        //qDebug() << "Grbl GC found.";
        bitSet(infos, InfoFlags::flagGC);
    }
    else if (block.startsWith("VER:"))
    {
        block = block.right( block.size() - 4 );
        QStringList vals;
        vals = block.split(":", QString::KeepEmptyParts, Qt::CaseInsensitive);
        if ( vals.size() == 2 )
        {
            machineVersion = vals.at(0);
            qDebug() << "Grbl::parse: Complete version" << machineVersion;
            bitSet(features, FeatureFlags::flagVersion);
            emit versionUpdated();
        }
    }
    else if (block.startsWith("OPT:"))
    {
        block = block.right( block.size() - 4 );
        QStringList vals;
        vals = block.split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
        if ( vals.size() == 3 )
        {
            if (vals.at(0).contains('V')) // variable spindle
            {
                bitSet(features, FeatureFlags::flagHasVariableSpindle);
                bitSet(features, FeatureFlags::flagHasLaserMode);
            }

            if (vals.at(0).contains('M')) // Mist coolant
                bitSet(features, FeatureFlags::flagHasCoolantMist);

            // These values are possible, but we don't care about them for now.
//            if (vals.at(0).contains('N')); // line numbers
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
                //qDebug() << "Grbl G" << gCode << " found.";
                bitSet(infos, InfoFlags::flagGXX);
            }
            else qDebug() << "Grbl G" << gCode << " incorrect coordinates: " << block;
        }
        else qDebug() << "Grbl GXX: incorrect format: " << block;
    }
}

// ----------------------------------------------------------------------------------
void Grbl::parseStatus(QString &line)
{
    static bool first = true; // This is used to emit coordinatesUpdated on first call to display coordinates.

    //qDebug() << "Grbl::parseStatus";
    bool changed = false;
    infos &= bit(InfoFlags::flagHasWorkingOffset)|bit(InfoFlags::flagHasMachineCoords); // WorkingOffset must be kept accross calls

    quint64 newActioners = 0;
    quint64 newSwitches = 0;
    bitSet(infos, InfoFlags::flagHasSwitches); // Switches are considered always presents, absence means released.

    // Handle status line
    QString buffer = line.right(line.size()-1); // remove '<' char
    buffer = buffer.left(buffer.size()-1); // remove '>' char

    // Split status on | (pipe)
    QStringList blocks = buffer.split("|", QString::KeepEmptyParts, Qt::CaseInsensitive);

    // Block 0 if the state of Grbl, we use newState to emit signal at the end when state changes.
    QString block = blocks.at(0);
    int newstate = state;

    if ( block == "Idle" ) newstate = StateType::stateIdle;
    if ( block == "Run" ) newstate = StateType::stateRun;
    if ( block.startsWith("Hold:") )
    {
        block = block.right( block.size() - 5 );
        holdCode = block.toInt();
        newstate = StateType::stateHold;
    }
    if ( block == "Jog" )
        newstate = StateType::stateJog;
    if ( block == "Home" )
        newstate = StateType::stateHome;
    if ( block == "Alarm")
    {
        newstate = StateType::stateAlarm;
    }
    if ( block == "Check" ) newstate = StateType::stateCheck;
    if ( block.startsWith("Door:"))
    {
        block = block.right( block.size() - 5 );
        doorCode = block.toInt();
        newstate = StateType::stateDoor;
    }

    if ( block == "Sleep" ) newstate = StateType::stateSleep;

    // If state has changed, keep state and emit signal
    if (state != newstate)
    {
        state = newstate;
        emit stateUpdated();
    }

    for (int i=1; i < blocks.size(); i++)
    {
        QStringList vals;
        block = blocks.at(i);
        changed = false;

        if ( block.startsWith("WPos:") ) // Working Position
        {
            block = block.right( block.size() - 5 );
            vals = block.split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
            if ( vals.size() == 3 )
            {
                CoordinatesType coords;
                coords.x = vals.at(0).toDouble();
                coords.y = vals.at(1).toDouble();
                coords.z = vals.at(2).toDouble();

                if ((workingCoordinates != coords) || first) changed = true;

                workingCoordinates = coords;
                bitSet(infos, InfoFlags::flagHasWorkingCoords);

                // Recompute machineCoordinates even if no change
                //      in case WorkingOffset changed
                if (bitIsSet(infos, InfoFlags::flagHasWorkingOffset))
                {
                    coords.x = workingCoordinates.x + workingOffset.x;
                    coords.y = workingCoordinates.y + workingOffset.y;
                    coords.z = workingCoordinates.z + workingOffset.z;
                    if ((machineCoordinates != coords) || first) changed = true;
                    machineCoordinates = coords;
                    bitSet(infos, InfoFlags::flagHasMachineCoords);
                }

                if (changed || first)
                    emit coordinatesUpdated();

            } else qDebug() << "Grbl statusError : " << block;

         } else if ( block.startsWith("MPos:") ) // Machine Position
         {
            block = block.right( block.size() - 5 );
            vals = block.split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
            if ( vals.size() == 3 )
            {
                CoordinatesType coords;
                coords.x = vals.at(0).toDouble();
                coords.y = vals.at(1).toDouble();
                coords.z = vals.at(2).toDouble();

                changed = (workingCoordinates != coords);

                machineCoordinates = coords;
                bitSet(infos, InfoFlags::flagHasMachineCoords);

                // Recompute workingCoordinates even if no change
                //      in case WorkingOffset changed
                if (bitIsSet(infos, InfoFlags::flagHasWorkingOffset))
                {
                    coords.z = machineCoordinates.x - workingOffset.x;
                    coords.y = machineCoordinates.y - workingOffset.y;
                    coords.z = machineCoordinates.z - workingOffset.z;
                    if ((workingCoordinates != coords) || first) changed = true;
                    workingCoordinates = coords;
                    bitSet(infos, InfoFlags::flagHasWorkingCoords);
                }

                if (changed || first)
                    emit coordinatesUpdated();

            } else qDebug() << "Grbl statusError : " << block;

         } else if ( block.startsWith("Bf:" ) ) // Buffer State
         {
            block = block.right( block.size() - 3 );
            vals = block.split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
            if ( vals.size() == 2 )
            {
                bitSet(infos, InfoFlags::flagHasBuffer);
                int val = vals.at(0).toInt();
                if (blockBuffer != val) changed = true;
                blockBuffer = val;

                val = vals.at(1).toInt();
                if (rxBuffer != val) changed = true;
                rxBuffer = val;

                if (changed)
                    emit buffersUpdated();

            } else qDebug() << "Grbl statusError : " << block;

         } else if ( block.startsWith("Ln:") ) // Line Numbers
         {
            block = block.right( block.size() - 3 );
            vals = block.split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
            if ( vals.size() == 1 )
            {
                bitSet(infos, InfoFlags::flagHasLineNumber);
                int val = vals.at(0).toInt();
                if (lineNumber != val) changed = true;
                lineNumber = val;
                if (changed)
                    emit lineNumberUpdated();

            } else qDebug() << "Grbl statusError : " << block;

         } else if ( block.startsWith("FS:") ) // FeedRate & SpindleSpeed
         {
            block = block.right( block.size() - 3 );
            vals = block.split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
            if ( vals.size() == 2 )
            {
                bitSet(infos, InfoFlags::flagHasFeedRate);
                int val = vals.at(0).toInt();
                if (feedRate != val) changed = true;
                feedRate = val;

                bitSet(infos, InfoFlags::flagHasSpindleSpeed);
                val = vals.at(1).toInt();
                if (spindleSpeed != val) changed = true;
                spindleSpeed = val;

                if (changed)
                    emit ratesUpdated();

            } else qDebug() << "Grbl statusError : " << block;

         } else if ( block.startsWith("F:") ) // FeedRate alone
         {
             bitSet(infos, InfoFlags::flagHasFeedRate);
             block = block.right( block.size() - 2 );
             int val = block.toInt();
             if (feedRate != val) changed = true;
             feedRate = val;

             if (changed)
                 emit ratesUpdated();

         } else if ( block.startsWith("Pn:") ) // Switches states
         {
            block = block.right( block.size() - 3 );
            if (block.contains('P', Qt::CaseInsensitive))
                bitSet(newSwitches, SwitchFlags::switchProbe);

            if (block.contains('X', Qt::CaseInsensitive))
                bitSet(newSwitches, SwitchFlags::switchLimitX);
            if (block.contains('Y', Qt::CaseInsensitive))
                bitSet(newSwitches, SwitchFlags::switchLimitY);
            if (block.contains('Z', Qt::CaseInsensitive))
                bitSet(newSwitches, SwitchFlags::switchLimitZ);

            if (block.contains('D', Qt::CaseInsensitive))
                bitSet(newSwitches, SwitchFlags::switchDoor);
            if (block.contains('R', Qt::CaseInsensitive))
                bitSet(newSwitches, SwitchFlags::switchReset);
            if (block.contains('H', Qt::CaseInsensitive))
                bitSet(newSwitches, SwitchFlags::switchFeedHold);
            if (block.contains('S', Qt::CaseInsensitive))
                bitSet(newSwitches, SwitchFlags::switchCycleStart);

         } else if ( block.startsWith("WCO:") ) // Working Coord Offset
         {
            bitSet(infos, InfoFlags::flagHasWorkingOffset);
            block = block.right( block.size() - 4 );
            vals = block.split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
            if ( vals.size() == 3 )
            {
                CoordinatesType coords;
                coords.x = vals.at(0).toDouble();
                coords.y = vals.at(1).toDouble();
                coords.z = vals.at(2).toDouble();
                if (workingOffset != coords) changed = true;
                workingOffset = coords;

                if (bitIsClear(infos, InfoFlags::flagHasMachineCoords))
                {
                    coords.x = workingCoordinates.x + workingOffset.x;
                    coords.y = workingCoordinates.y + workingOffset.y;
                    coords.z = workingCoordinates.z + workingOffset.z;
                    changed = true;
                    machineCoordinates = coords;
                    bitSet(infos, InfoFlags::flagHasMachineCoords);
                }

                if (bitIsClear(infos, InfoFlags::flagHasWorkingCoords))
                {
                    coords.z = machineCoordinates.x - workingOffset.x;
                    coords.y = machineCoordinates.y - workingOffset.y;
                    coords.z = machineCoordinates.z - workingOffset.z;
                    changed = true;
                    workingCoordinates = coords;
                    bitSet(infos, InfoFlags::flagHasWorkingCoords);
                }

                if (changed)
                    emit coordinatesUpdated();

            } else qDebug() << "Grbl statusError : " << block;

         } else if ( block.startsWith("Ov:") ) // Overrides
         {
            block = block.right( block.size() - 3 );
            vals = block.split(",", QString::KeepEmptyParts, Qt::CaseInsensitive);
            if ( vals.size() == 3 )
            {
                bitSet(infos, InfoFlags::flagHasOverride);
                fOverride = vals.at(0).toInt();
                rOverride = vals.at(1).toInt();
                spindleSpeedOverride = vals.at(2).toInt();

                // When Ov: is present, actions are following if any.
                bitSet(infos, InfoFlags::flagHasActioners);
                actioners = 0;
             } else qDebug() << "Grbl statusError : " << block;

        } else if ( block.startsWith("A:") ) // Action states
        {
            bitSet(infos, InfoFlags::flagHasActioners); // just in case

            block = block.right( block.size() - 2 );
            if (block.contains("SS", Qt::CaseInsensitive))
            {
                bitSet(newActioners, ActionerFlags::actionSpindleVariable);
                bitSet(newActioners, ActionerFlags::actionSpindle);
            }
            else if (block.contains("SC", Qt::CaseInsensitive))
            {
                bitSet(newActioners, ActionerFlags::actionSpindleVariable);
                bitSet(newActioners, ActionerFlags::actionSpindle);
                bitSet(newActioners, ActionerFlags::actionSpindleCounterClockwise);
            }
            else
            {
                if (block.contains('S', Qt::CaseInsensitive))
                    bitSet(newActioners, ActionerFlags::actionSpindle);

                if (block.contains('C', Qt::CaseInsensitive))
                {
                    bitSet(newActioners, ActionerFlags::actionSpindle);
                    bitSet(newActioners, ActionerFlags::actionSpindleCounterClockwise);
                }
            }

            if (block.contains('F', Qt::CaseInsensitive))
            {
                bitSet(newActioners, ActionerFlags::actionCoolant);
                bitSet(newActioners, ActionerFlags::actionCoolantFlood);
            }

            if (block.contains('M', Qt::CaseInsensitive))
            {
                bitSet(newActioners, ActionerFlags::actionCoolant);
                bitSet(newActioners, ActionerFlags::actionCoolantMist);
            }

        } else qDebug() << "Grbl statusError : " << block;
    }

    // This block is here as we must emit switchesUpdated event if no Pn: parameter is present
    if (switches != newSwitches)
    {
        switches = newSwitches;
        emit switchesUpdated();
    }

    if (bitIsSet(infos, Machine::InfoFlags::flagHasActioners) && (actioners != newActioners))
    {
        actioners = newActioners;
        emit actionersUpdated();
    }

    first = false;
}

void Grbl::parseConfig(QString &line)
{
    if (line.startsWith("$N"))
    {
        QString block = line.right(line.size() - 2);
        QStringList vals = block.split("=", QString::KeepEmptyParts, Qt::CaseInsensitive);
        if (vals.size() == 2)
        {
            uint key = vals.at(0).toUInt();
            QString val = vals.at(1);
            config[ key + ConfigType::configStartingBlock0 ] = val;
            bitSet(infos, Grbl::InfoFlags::flagHasStartingBlocks);
            qDebug() << "Getting startingBlock " << config.size() << ": $" << key << " = " << val;
        }
        else qDebug() << "Grbl statusError : " << block;
    }
    else if (line.startsWith('$'))
    {
        QString block = line.right(line.size() - 1);
        QStringList vals = block.split("=", QString::KeepEmptyParts, Qt::CaseInsensitive);
        if ( vals.size() == 2 )
        {
            uint key = vals.at(0).toUInt();
            QString val = vals.at(1);
            config[ key ] = val;
            bitSet(infos, Grbl::InfoFlags::flagHasConfig);
            qDebug() << "Getting config " << config.size() << ": $" << key << " = " << val;
        }
        else qDebug() << "Grbl statusError : " << block;
    }
}

void Grbl::setXWorkingZero()
{
    if (sendCommand( "G92X0" ))
    {
        workingCoordinates.x = 0;
        bitClear(infos, Machine::InfoFlags::flagHasWorkingOffset);
    }
};

void Grbl::setYWorkingZero()
{
    if (sendCommand( "G92Y0" ))
    {
        workingCoordinates.y = 0;
        bitClear(infos, Machine::InfoFlags::flagHasWorkingOffset);
    }
};

void Grbl::setZWorkingZero()
{
    if (sendCommand( "G92Z0" ))
    {
        workingCoordinates.z = 0;
        bitClear(infos, Machine::InfoFlags::flagHasWorkingOffset);
    }
};
