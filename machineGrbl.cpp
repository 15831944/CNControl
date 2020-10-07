
#include <QDebug>
#include <QIcon>
#include <QMessageBox>
#include <QCsvFile>
#include <QJsonDocument>

//#include "ui_grbl.h"
#include "ui_machineGrbl.h"

#include "grbl_config.h"
#include "machine.h"
#include "machineGrbl.h"

#define CMD_CONFIG     "$$"
#define CMD_INFOS      "$I"
#define CMD_GC         "$G"
#define CMD_GXX        "$#"
#define CMD_UNLOCK     "$X"
#define CMD_HOME       "$H"
#define CMD_STARTBLOCK "$N"
#define CMD_CHECK      "$C"

MachineGrbl::MachineGrbl(QWidget *parent) :
//    MachineGrbl::MachineGrbl(QJsonObject &configMachine, QWidget *parent) :
//    Machine(configMachine, parent),
    Machine(parent),
    ui(new Ui::MachineGrbl)
{
    machineType = "Grbl";

    ui->setupUi( this );
    setMachineConfigurationWidget( ui->tabWidget );

    loadErrorsMessages();
    loadAlarmsMessages();
    loadBuildOptionsMessages();
    loadSettingsMessages();
    loadStatesMessages();

    port = nullptr;

    qDebug() << "MachineGrbl::MachineGrbl: machine initialized.";
}

MachineGrbl::~MachineGrbl()
{

    closeMachine();
    delete ui;

    qDebug() << "MachineGrbl::~MachineGrbl: machine deleted.";
}

#include "portSerial.h"
void MachineGrbl::openMachine(QString portName)
{
    PortSerial *serial = new PortSerial();
//    serial->setSpeed( 38400 );
    serial->setSpeed( 115200 );
    serial->setDevice( portName );

    if (!serial->open())
        throw machineConnectException(serial->errorString());

    port = serial;
//    features = bit(FeatureFlags::flagAskStatus) |
//               bit(FeatureFlags::flagName);
    features = 0;

    connect( port, SIGNAL(lineAvailable(QString&)), this, SLOT(parse(QString&)));
    connect( &statusTimer, SIGNAL(timeout()), this, SLOT(timeout()));

    // Start by asking informations about version
//    ask(CommandType::commandInfos);
    ask(CommandType::commandReset);

    // And ask for status on a 5Hz basis (maximum recommended by Grbl)
    //statusTimer.start(200);
}

void MachineGrbl::closeMachine()
{
    statusTimer.stop();
    disconnect( &statusTimer, SIGNAL(timeout()), this, SLOT(timeout()));

    if (port)
    {
        disconnect( port, SIGNAL(lineAvailable(QString&)), this, SLOT(parse(QString&)));
        delete port;
        port = nullptr;
    }
}

bool MachineGrbl::moveToX(double x, double feed, bool jog, bool machine, bool absolute)
{
    return sendCommand(
                QString("%1%2%3X%4F%5")
                    .arg( jog?"$j=":"G0" )
                    .arg( absolute?"G91":"G90" )
                    .arg( machine?"G53":"" )
                    .arg( x )
                    .arg( feed )
                .toUtf8()
                );
}

bool MachineGrbl::moveToY(double y, double feed, bool jog, bool machine, bool absolute)
{
    return sendCommand(
                QString("%1%2%3Y%4F%5")
                    .arg( jog?"$j=":"G0" )
                    .arg( absolute?"G91":"G90" )
                    .arg( machine?"G53":"" )
                    .arg( y )
                    .arg( feed )
                .toUtf8()
                );
}

bool MachineGrbl::moveToXY(double x, double y, double feed, bool jog, bool machine, bool absolute)
{
    return sendCommand(
                QString("%1%2%3X%4Y%5F%6")
                    .arg( jog?"$j=":"G0" )
                    .arg( absolute?"G91":"G90" )
                    .arg( machine?"G53":"" )
                    .arg( x )
                    .arg( y )
                    .arg( feed )
                .toUtf8()
                );
}

bool MachineGrbl::moveToZ(double z, double feed, bool jog, bool machine, bool absolute)
{
    return sendCommand(
                QString("%1%2%3Z%4F%5")
                    .arg( jog?"$j=":"G0" )
                    .arg( absolute?"G91":"G90" )
                    .arg( machine?"G53":"" )
                    .arg( z )
                    .arg( feed )
                .toUtf8()
                );
}

bool MachineGrbl::moveToXYZ(double x, double y, double z, double feed, bool jog, bool machine, bool absolute)
{
    return sendCommand(
                QString("%1%2%3X%4Y%5Z%6F%7")
                    .arg( jog?"$j=":"G0" )
                    .arg( absolute?"G91":"G90" )
                    .arg( machine?"G53":"" )
                    .arg( x )
                    .arg( y )
                    .arg( z )
                    .arg( feed )
                .toUtf8()
                );
}

bool MachineGrbl::moveTo(QVector3D &point, double feed, bool jog, bool machine, bool absolute)
{
    return sendCommand(
                QString("%1%2%3X%4Y%5Z%6F%7")
                    .arg( jog?"$j=":"G0" )
                    .arg( absolute?"G91":"G90" )
                    .arg( machine?"G53":"" )
                    .arg( point.x() )
                    .arg( point.y() )
                    .arg( point.z() )
                    .arg( feed )
                .toUtf8()
                );
}

bool MachineGrbl::stopMove()
{
    switch (state)
    {
    case MachineGrbl::StateType::stateRun:
        // Que faire pour stopper le mouvement en plein travail ?
        // On pourrait faire pause ?
        break;
    case MachineGrbl::StateType::stateJog:
        ask(MachineGrbl::CommandType::commandJogCancel);
        break;
    default:
        return false;
    }
    return true;
}

void MachineGrbl::loadErrorsMessages()
{
    if (!errorMessages.isEmpty()) return;

    QCsvFile file("./csv/error_codes_en_US.csv");
    if(!file.open(QFile::ReadOnly |
                  QFile::Text))
    {
        qDebug() << QString("MachineGrbl::readErrorsMessages: Could not open file %1.").arg(file.fileName());
        return;
    }

    QStringList row;
    while (file.readLine(&row))
    {
        ErrorMessageType message;
        message.errorCode = row.at(0).toInt();
        message.shortMessage = row.at(1);
        message.longMessage = row.at(2);

        if (message.errorCode)
            errorMessages[ message.errorCode ] = message;
    }
    qDebug() << "MachineGrbl::readErrorsMessages: Messages read " << errorMessages.size();
};

void MachineGrbl::loadAlarmsMessages()
{
    if (!alarmMessages.isEmpty()) return;

    QCsvFile file("./csv/alarm_codes_en_US.csv");
    if(!file.open(QFile::ReadOnly |
                  QFile::Text))
    {
        qDebug() << QString("MachineGrbl::readAlarmsMessages: Could not open file %1.").arg(file.fileName());
        return;
    }

    QStringList row;
    while (file.readLine(&row))
    {
        AlarmMessageType message;
        message.alarmCode = row.at(0).toInt();
        message.shortMessage = row.at(1);
        message.longMessage = row.at(2);

        if (message.alarmCode)
            alarmMessages[ message.alarmCode ] = message;
    }
    qDebug() << "MachineGrbl::readAlarmsMessages: Messages read " << alarmMessages.size();

};

void MachineGrbl::loadBuildOptionsMessages()
{
    if (!buildOptionMessages.isEmpty()) return;

    QCsvFile file("./csv/build_option_codes_en_US.csv");
    if(!file.open(QFile::ReadOnly |
                  QFile::Text))
    {
        qDebug() << QString("MachineGrbl::readBuildOptionsMessages: Could not open file %1.").arg(file.fileName());
        return;
    }

    QStringList row;
    while (file.readLine(&row))
    {
        BuildOptionMessageType message;
        message.buildOptionCode = row.at(0).at(0).toLatin1();
        message.description = row.at(1);

        buildOptionMessages[ message.buildOptionCode ] = message;
    }
    qDebug() << "MachineGrbl::readBuildOptionsMessages: Messages read " << buildOptionMessages.size();
};

void MachineGrbl::loadSettingsMessages()
{
    if (!settingMessages.isEmpty()) return;

    QCsvFile file("./csv/setting_codes_en_US.csv");
    if(!file.open(QFile::ReadOnly |
                  QFile::Text))
    {
        qDebug() << QString("MachineGrbl::readSettingsMessages: Could not open file %1.").arg(file.fileName());
        return;
    }

    QStringList row;
    while (file.readLine(&row))
    {
        SettingMessageType message;
        message.settingCode = row.at(0).toInt();
        message.setting = row.at(1);
        message.unit = row.at(2);
        message.description = row.at(3);

        if (message.settingCode)
            settingMessages[ message.settingCode ] = message;
    }
    qDebug() << "MachineGrbl::readSettingsMessages: Messages read " << settingMessages.size();
};

void MachineGrbl::loadStatesMessages()
{
    if (!stateMessages.isEmpty()) return;

    stateMessages.insert( StateType::stateUnknown, tr("Unknown","Grbl state"));
    stateMessages.insert( StateType::stateIdle,    tr("Idle","Grbl state"));
    stateMessages.insert( StateType::stateRun,     tr("Run","Grbl state"));
    stateMessages.insert( StateType::stateHold,    tr("Hold","Grbl state"));
    stateMessages.insert( StateType::stateJog,     tr("Jog","Grbl state"));
    stateMessages.insert( StateType::stateHome,    tr("Home","Grbl state"));
    stateMessages.insert( StateType::stateAlarm,   tr("Alarm","Grbl state"));
    stateMessages.insert( StateType::stateCheck,   tr("Check","Grbl state"));
    stateMessages.insert( StateType::stateDoor,    tr("Door","Grbl state"));
    stateMessages.insert( StateType::stateSleep,   tr("Sleep","Grbl state"));

    qDebug() << "MachineGrbl::loadStatesMessages: Messages read " << stateMessages.size();
}

int MachineGrbl::openConfiguration()
{
//    static QWidget *widget;
//    static QMessageBox *waitMessage;
    static int configState = 0;

    if (!isState(StateType::stateIdle) && !isState(StateType::stateAlarm))
    {
        qDebug() << "MachineGrbl::openConfiguration: Error : machine is not Idle or Alarm";
        QMessageBox::information(nullptr, "Error", "Machine is not Idle or Alarm.");
    }
    else
    {
        switch (configState)
        {
        case 0: // First step : Ask for configuration
            configState++;

            // We have to choose between bitClear or Timer.stop(), not both
//            bitClear(features, FeatureFlags::flagAskStatus);
            statusTimer.stop();

            connect( this, SIGNAL(commandExecuted()), this, SLOT(openConfiguration()));
            //config = QJsonObject();

            ask(MachineGrbl::CommandType::commandConfig);
            qDebug() << "MachineGrbl::openConfiguration: Asking for configuration.";

//        waitMessage = new QMessageBox( parent );
//        //waitMessage->setAttribute( Qt::WA_DeleteOnClose ); //makes sure the msgbox is deleted automatically when closed
//        waitMessage->setStandardButtons( QMessageBox::NoButton );
//        waitMessage->setWindowTitle( tr("Retreiving Grbl Conguration") );
//        waitMessage->setText( tr("Please wait while retreiving configuration...") );
//        waitMessage->setModal( true ); // if you want it non-modal
//        waitMessage->open( this, nullptr);
            break;
        case 1: // Second step : Ask for starting blocks
            configState++;
            ask(MachineGrbl::CommandType::commandStartBlock);
            qDebug() << "MachineGrbl::openConfiguration: Asking for starting blocks.";
            break;
        case 2: // Third step : open configuration dialog
            configState = 0;

            disconnect( this, SIGNAL(commandExecuted()), this, SLOT(openConfiguration()));
            qDebug() << "MachineGrbl::openConfiguration: Got configuration and starting blocks.";

//            bitSet(features, FeatureFlags::flagAskStatus);
            statusTimer.start();

            setConfiguration();
            int result = Machine::openConfiguration();

            if (result == QDialog::Accepted)
            {
                if (getConfiguration())
                    writeConfiguration();
            }

            return result;
        }
    }
    return 0;
}

void MachineGrbl::writeConfiguration()
{
    static int index = 0;

    switch (index)
    {
    case 0: // First step :
//        bitClear(features, FeatureFlags::flagAskStatus);
        statusTimer.stop();

        connect( this, SIGNAL(commandExecuted()), this, SLOT(writeConfiguration()));
        qDebug() << "MachineGrbl::writeConfiguration: Start sending configuration.";
//        break; // No break here !!! we continue...
     [[clang::fallthrough]]; default:

        if ( index < config.size() )
        {
            // Warning, setting LaserMode($32) when disabled generate an error
            if ((index == MachineGrbl::ConfigType::configLaserMode) && !hasFeature( MachineGrbl::InfoFlags::flagHasLaserMode))
            {
                qDebug()<< "MachineGrbl::writeConfiguration: Ignoring LaserMode parameter";
                writeConfiguration();
            }
            else {

                QString val = config.value(index);
                QString cmd;
                switch (index)
                {
                case MachineGrbl::ConfigType::configStartingBlock0:
                case MachineGrbl::ConfigType::configStartingBlock1:
                    cmd = QString("$N%1=%2").arg(index - MachineGrbl::ConfigType::configStartingBlock0).arg(val);
                    break;
                default:
                    cmd = QString("$%1=%2").arg(index).arg(val);
                }

                qDebug() << "Grbl::writeConfiguration: " << cmd;

                if (!sendCommand(cmd))
                {
                    disconnect( this, SIGNAL(commandExecuted()), this, SLOT(writeConfiguration()));
//                    bitSet(features, FeatureFlags::flagAskStatus);
                    statusTimer.start();
                    index = 0;
                    qDebug() << "Grbl::writeConfiguration: Error sending configuration.";
                }
            }
        }
        else {
            disconnect( this, SIGNAL(commandExecuted()), this, SLOT(writeConfiguration()));
//            bitSet(features, FeatureFlags::flagAskStatus);
            statusTimer.start();
            index = 0;
            qDebug() << "Grbl::writeConfiguration: configuration set.";
        }
    }
}

#define xFlagMask (1<<2)
#define yFlagMask (1<<1)
#define zFlagMask (1<<0)
#define noFlagMask 0

void MachineGrbl::setConfiguration()
{
    for( int key : config.keys() )
    {
        QString val = config[key];
        uint valUInt = val.toUInt();
        qDebug()<<"key="<<key<<",val="<<val;

        switch (key)
        {
        case MachineGrbl::ConfigType::configStepPulse: ui->stepPulseLineEdit->setText( val ); break;
        case MachineGrbl::ConfigType::configStepIdleDelay: ui->stepIdleDelayLineEdit->setText( val ); break;
        case MachineGrbl::ConfigType::configStepPortInvert:
            ui->xStepPortInvertCheckBox->setChecked( valUInt & xFlagMask );
            ui->yStepPortInvertCheckBox->setChecked( valUInt & yFlagMask );
            ui->zStepPortInvertCheckBox->setChecked( valUInt & zFlagMask );
            break;
        case MachineGrbl::ConfigType::configDirPortInvert:
            ui->xDirPortInvertCheckBox->setChecked( valUInt & xFlagMask );
            ui->yDirPortInvertCheckBox->setChecked( valUInt & yFlagMask );
            ui->zDirPortInvertCheckBox->setChecked( valUInt & zFlagMask );
            break;
        case MachineGrbl::ConfigType::configStepEnableInvert: ui->stepEnableInvertCheckBox->setChecked( val == '1' ); break;
        case MachineGrbl::ConfigType::configLimitPinInvert: ui->limitPinsInvertCheckBox->setChecked( val == '1' ); break;
        case MachineGrbl::ConfigType::configProbePinInvert: ui->probePinInvertCheckBox->setChecked( val == '1' ); break;

        case MachineGrbl::ConfigType::configStatusReport:
            // Warning : no verification of index
            ui->statusReportComboBox->setCurrentIndex( val.toInt() );
            // Mask
            break;
        case MachineGrbl::ConfigType::configJunctionDeviation: ui->junctionDeviationLineEdit->setText( val ); break;
        case MachineGrbl::ConfigType::configArcTolerance: ui->arcToleranceLineEdit->setText( val ); break;

            // Warning : no verification of index
        case MachineGrbl::ConfigType::configReportInches: ui->reportInchesComboBox->setCurrentIndex( val.toInt() ); break;
        case MachineGrbl::ConfigType::configSoftLimits: ui->softLimitsCheckBox->setChecked( val == '1' ); break;
        case MachineGrbl::ConfigType::configHardLimits: ui->hardLimitsCheckBox->setChecked( val == '1' ); break;

        case MachineGrbl::ConfigType::configHomingCycle: ui->homingCycleCheckBox->setChecked( val == '1' ); break;
        case MachineGrbl::ConfigType::configHomingDirInvert:
            ui->xHomingDirInvertCheckBox->setChecked( valUInt & xFlagMask );
            ui->yHomingDirInvertCheckBox->setChecked( valUInt & yFlagMask );
            ui->zHomingDirInvertCheckBox->setChecked( valUInt & zFlagMask );
            break;
        case MachineGrbl::ConfigType::configHomingFeed: ui->homingFeedLineEdit->setText( val ); break;
        case MachineGrbl::ConfigType::configHomingSeek: ui->homingSeekLineEdit->setText( val ); break;
        case MachineGrbl::ConfigType::configHomingDebounce: ui->homingDebounceLineEdit->setText( val ); break;
        case MachineGrbl::ConfigType::configHomingPullOff: ui->homingPullofflineEdit->setText( val ); break;

        case MachineGrbl::ConfigType::configMaxSpindleSpeed: ui->spindleSpeedMaxLineEdit->setText( val ); break;
        case MachineGrbl::ConfigType::configMinSpindleSpeed: ui->spindleSpeedMinLineEdit->setText( val ); break;

        case MachineGrbl::ConfigType::configLaserMode:
            ui->laserModeCheckBox->setEnabled( hasFeature( MachineGrbl::InfoFlags::flagHasLaserMode) );
            ui->laserModeCheckBox->setChecked( val == 1 );
            break;

        case MachineGrbl::ConfigType::configXSteps: ui->xStepsLineEdit->setText( val ); break;
        case MachineGrbl::ConfigType::configYSteps: ui->yStepsLineEdit->setText( val ); break;
        case MachineGrbl::ConfigType::configZSteps: ui->zStepsLineEdit->setText( val ); break;

        case MachineGrbl::ConfigType::configXMaxRate: ui->xMaxRateLineEdit->setText( val ); break;
        case MachineGrbl::ConfigType::configYMaxRate: ui->yMaxRateLineEdit->setText( val ); break;
        case MachineGrbl::ConfigType::configZMaxRate: ui->zMaxRateLineEdit->setText( val ); break;

        case MachineGrbl::ConfigType::configXAcceleration: ui->xAccelerationLineEdit->setText( val ); break;
        case MachineGrbl::ConfigType::configYAcceleration: ui->yAccelerationLineEdit->setText( val ); break;
        case MachineGrbl::ConfigType::configZAcceleration: ui->zAccelerationLineEdit->setText( val ); break;

        case MachineGrbl::ConfigType::configXMaxTravel: ui->xMaxTravelLineEdit->setText( val ); break;
        case MachineGrbl::ConfigType::configYMaxTravel: ui->yMaxTravelLineEdit->setText( val ); break;
        case MachineGrbl::ConfigType::configZMaxTravel: ui->zMaxTravelLineEdit->setText( val ); break;

        case MachineGrbl::ConfigType::configStartingBlock0:
            ui->startingBlock0LineEdit->setText( val ); break;
        case MachineGrbl::ConfigType::configStartingBlock1:
            ui->startingBlock1LineEdit->setText( val ); break;

        default: qDebug() << "MachineGrbl::setConfiguration: Key unknown " << key;
        }
    }
}

bool MachineGrbl::getConfiguration()
{
//    config[ MachineGrbl::ConfigType::configStepPulse         ] = ui->stepPulseLineEdit->text();
//    config[ MachineGrbl::ConfigType::configStepIdleDelay     ] = ui->stepIdleDelayLineEdit->text();
//    config[ MachineGrbl::ConfigType::configStepPortInvert    ] = QString("%1").arg(
//                                                                (ui->xStepPortInvertCheckBox->isChecked()?xFlagMask:noFlagMask) |
//                                                                (ui->yStepPortInvertCheckBox->isChecked()?yFlagMask:noFlagMask) |
//                                                                (ui->zStepPortInvertCheckBox->isChecked()?zFlagMask:noFlagMask) );
//    config[ MachineGrbl::ConfigType::configDirPortInvert     ] = QString("%1").arg(
//                                                                (ui->xDirPortInvertCheckBox->isChecked()?xFlagMask:noFlagMask) |
//                                                                (ui->yDirPortInvertCheckBox->isChecked()?yFlagMask:noFlagMask) |
//                                                                (ui->zDirPortInvertCheckBox->isChecked()?zFlagMask:noFlagMask) );
//    config[ MachineGrbl::ConfigType::configStepEnableInvert  ] = ui->stepEnableInvertCheckBox->isChecked()?'1':'0';
//    config[ MachineGrbl::ConfigType::configLimitPinInvert    ] = ui->limitPinsInvertCheckBox->isChecked()?'1':'0';
//    config[ MachineGrbl::ConfigType::configProbePinInvert    ] = ui->probePinInvertCheckBox->isChecked()?'1':'0';
//    config[ MachineGrbl::ConfigType::configStatusReport      ] = QString("%1").arg(ui->statusReportComboBox->currentIndex());
//    config[ MachineGrbl::ConfigType::configJunctionDeviation ] = ui->junctionDeviationLineEdit->text();
//    config[ MachineGrbl::ConfigType::configArcTolerance      ] = ui->arcToleranceLineEdit->text();
//    config[ MachineGrbl::ConfigType::configReportInches      ] = QString("%1").arg(ui->reportInchesComboBox->currentIndex());
//    config[ MachineGrbl::ConfigType::configSoftLimits        ] = ui->softLimitsCheckBox->isChecked()?'1':'0';
//    config[ MachineGrbl::ConfigType::configHardLimits        ] = ui->hardLimitsCheckBox->isChecked()?'1':'0';
//    config[ MachineGrbl::ConfigType::configHomingCycle       ] = ui->homingCycleCheckBox->isChecked()?'1':'0';
//    config[ MachineGrbl::ConfigType::configHomingDirInvert   ] = QString("%1").arg(
//                                                                (ui->xHomingDirInvertCheckBox->isChecked()?xFlagMask:noFlagMask) |
//                                                                (ui->yHomingDirInvertCheckBox->isChecked()?yFlagMask:noFlagMask) |
//                                                                (ui->zHomingDirInvertCheckBox->isChecked()?zFlagMask:noFlagMask) );
//    config[ MachineGrbl::ConfigType::configHomingFeed        ] = ui->homingFeedLineEdit->text();
//    config[ MachineGrbl::ConfigType::configHomingSeek        ] = ui->homingSeekLineEdit->text();
//    config[ MachineGrbl::ConfigType::configHomingDebounce    ] = ui->homingDebounceLineEdit->text();
//    config[ MachineGrbl::ConfigType::configHomingPullOff     ] = ui->homingPullofflineEdit->text();
//    config[ MachineGrbl::ConfigType::configMaxSpindleSpeed   ] = ui->spindleSpeedMaxLineEdit->text();
//    config[ MachineGrbl::ConfigType::configMinSpindleSpeed   ] = ui->spindleSpeedMinLineEdit->text();

//    config[ MachineGrbl::ConfigType::configLaserMode         ] = ui->laserModeCheckBox->isChecked()?1.0:0.0;

//    config[ MachineGrbl::ConfigType::configXSteps            ] = ui->xStepsLineEdit->text();
//    config[ MachineGrbl::ConfigType::configYSteps            ] = ui->yStepsLineEdit->text();
//    config[ MachineGrbl::ConfigType::configZSteps            ] = ui->zStepsLineEdit->text();

//    config[ MachineGrbl::ConfigType::configXMaxRate          ] = ui->xMaxRateLineEdit->text();
//    config[ MachineGrbl::ConfigType::configYMaxRate          ] = ui->yMaxRateLineEdit->text();
//    config[ MachineGrbl::ConfigType::configZMaxRate          ] = ui->zMaxRateLineEdit->text();

//    config[ MachineGrbl::ConfigType::configXAcceleration     ] = ui->xAccelerationLineEdit->text();
//    config[ MachineGrbl::ConfigType::configYAcceleration     ] = ui->yAccelerationLineEdit->text();
//    config[ MachineGrbl::ConfigType::configZAcceleration     ] = ui->zAccelerationLineEdit->text();

//    config[ MachineGrbl::ConfigType::configXMaxTravel        ] = ui->xMaxTravelLineEdit->text();
//    config[ MachineGrbl::ConfigType::configYMaxTravel        ] = ui->yMaxTravelLineEdit->text();
//    config[ MachineGrbl::ConfigType::configZMaxTravel        ] = ui->zMaxTravelLineEdit->text();

//    // Include starting blocks in config
//    config[ MachineGrbl::ConfigType::configStartingBlock0    ] = ui->startingBlock0LineEdit->text();
//    config[ MachineGrbl::ConfigType::configStartingBlock1    ] = ui->startingBlock1LineEdit->text();

    return true;
}

// ----------------------------------------------------------------------------------
bool MachineGrbl::ask(int commandCode, int commandArg, bool noLog)
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

void MachineGrbl::timeout()
{
//    if (hasFeature( FeatureFlags::flagAskStatus))
        ask(CommandType::commandStatus, 0, true);
}

void MachineGrbl::parse(QString &line)
{
    lastLine = line;

    emit infoReceived(line);

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

        state = StateType::stateUnknown;

        // Problem : When clicking on reset switch, multiple reset occurs.
        //           Informations are asked multiple times (4 times).
        //           It works, but that take plenty of time
        ask(CommandType::commandInfos);
        statusTimer.start(200);

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
        qDebug() << QString("Grbl::parse: Error %1 : %2 ")
                    .arg( errorCode )
                    .arg( getErrorMessages( errorCode ).shortMessage ).toUtf8().data();
        emit error(errorCode);
    }
    else if (line.startsWith("ALARM:"))
    {
        QString block = line.right( line.size() - 6 );
        alarmCode = block.toInt();
        state = StateType::stateAlarm;
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
void MachineGrbl::parseInfo(QString &line)
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
            prbCoords.setX( vals.at(0).toFloat() );
            prbCoords.setY( vals.at(1).toFloat() );
            prbCoords.setZ( vals.at(2).toFloat() );
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

            if (vals.at(0).contains('M'))
                bitSet(features, FeatureFlags::flagHasCoolantMist);
            if (vals.at(0).contains('N'))
                bitSet(features, FeatureFlags::flagHasLineNumbers);
            if (vals.at(0).contains('C'))
                bitSet(features, FeatureFlags::flagHasCoreXY);

            // These values are possible, but we don't care about them for now.
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
                QVector3D coords;
                coords.setX( vals.at(0).toFloat() );
                coords.setY( vals.at(1).toFloat() );
                coords.setZ( vals.at(2).toFloat() );

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
void MachineGrbl::parseStatus(QString &line)
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
        int newHoldCode = block.toInt();

        // This is a hack to have a signal stateUpdated on holdCode change !!!
        // Maybe we should duplicate the if statement with "Hold:0" and "Hold:1"...
        if (newHoldCode != holdCode)
        {
            state = StateType::stateUnknown;
            holdCode = newHoldCode;
        }

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
                QVector3D coords;
                coords.setX( vals.at(0).toFloat() );
                coords.setY( vals.at(1).toFloat() );
                coords.setZ( vals.at(2).toFloat() );

                if ((workingCoordinates != coords) || first) changed = true;

                workingCoordinates = coords;
                bitSet(infos, InfoFlags::flagHasWorkingCoords);

                // Recompute machineCoordinates even if no change
                //      in case WorkingOffset changed
                if (bitIsSet(infos, InfoFlags::flagHasWorkingOffset))
                {
                    coords.setX( workingCoordinates.x() + workingOffset.x() );
                    coords.setY( workingCoordinates.y() + workingOffset.y() );
                    coords.setZ( workingCoordinates.z() + workingOffset.z() );
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
                QVector3D coords;
                coords.setX( vals.at(0).toFloat() );
                coords.setY( vals.at(1).toFloat() );
                coords.setZ( vals.at(2).toFloat() );

                changed = (workingCoordinates != coords);

                machineCoordinates = coords;
                bitSet(infos, InfoFlags::flagHasMachineCoords);

                // Recompute workingCoordinates even if no change
                //      in case WorkingOffset changed
                if (bitIsSet(infos, InfoFlags::flagHasWorkingOffset))
                {
                    coords.setX( machineCoordinates.x() - workingOffset.x() );
                    coords.setY( machineCoordinates.y() - workingOffset.y() );
                    coords.setZ( machineCoordinates.z() - workingOffset.z() );
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
                QVector3D coords;
                coords.setX( vals.at(0).toFloat() );
                coords.setY( vals.at(1).toFloat() );
                coords.setZ( vals.at(2).toFloat() );

                if (workingOffset != coords) changed = true;
                workingOffset = coords;

                if (bitIsClear(infos, InfoFlags::flagHasMachineCoords))
                {
                    coords.setX( workingCoordinates.x() + workingOffset.x() );
                    coords.setY( workingCoordinates.y() + workingOffset.y() );
                    coords.setZ( workingCoordinates.z() + workingOffset.z() );
                    changed = true;
                    machineCoordinates = coords;
                    bitSet(infos, InfoFlags::flagHasMachineCoords);
                }

                if (bitIsClear(infos, InfoFlags::flagHasWorkingCoords))
                {
                    coords.setX( machineCoordinates.x() - workingOffset.x() );
                    coords.setY( machineCoordinates.y() - workingOffset.y() );
                    coords.setZ( machineCoordinates.z() - workingOffset.z() );
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
    //if (switches != newSwitches)
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

void MachineGrbl::parseConfig(QString &line)
{
    if (line.startsWith("$N"))
    {
        QString block = line.right(line.size() - 2);
        QStringList vals = block.split("=", QString::KeepEmptyParts, Qt::CaseInsensitive);
        if (vals.size() == 2)
        {
//            QString key = QString().setNum( vals.at(0).toUInt() + ConfigType::configStartingBlock0 );
            int key = vals.at(0).toInt() + ConfigType::configStartingBlock0;
            QString val = vals.at(1);

            config[ key ] = val;
            bitSet(infos, MachineGrbl::InfoFlags::flagHasStartingBlocks);
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
            int key = vals.at(0).toInt();
            QString val = vals.at(1);
            config[ key ] = val;
            bitSet(infos, MachineGrbl::InfoFlags::flagHasConfig);
            qDebug() << "Getting config " << config.size() << ": $" << key << " = " << val;
        }
        else qDebug() << "Grbl statusError : " << block;
    }
}

void MachineGrbl::setXWorkingZero()
{
    if (sendCommand( "G10L20P1X0" ))
    {
        workingCoordinates.setX(0);
        bitClear(infos, Machine::InfoFlags::flagHasWorkingOffset);
    }
};

void MachineGrbl::setYWorkingZero()
{
    if (sendCommand( "G10L20P1Y0" ))
    {
        workingCoordinates.setY(0);
        bitClear(infos, Machine::InfoFlags::flagHasWorkingOffset);
    }
};

void MachineGrbl::setZWorkingZero()
{
    if (sendCommand( "G10L20P1Z0" ))
    {
        workingCoordinates.setZ(0);
        bitClear(infos, Machine::InfoFlags::flagHasWorkingOffset);
    }
};
