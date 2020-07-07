#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QString>
#include <QStringList>
#include <QSerialPort>
#include <QWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>

#include "machineGrbl.h"
#include "QFocusLineEdit"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (!loadConfiguration())
        defaultConfiguration();

    QWidget::setWindowTitle(QString("%1 %2").arg(PROGRAM_NAME).arg(PROGRAM_VERSION));
    //setLogWidget( ui->logListWidget );

    // CoolantMist may not be enabled, we'll see later in infos ($I)
    ui->coolantMistPushButton->setEnabled(false);

    ui->xLimitSwitchPushButton->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->yLimitSwitchPushButton->setAttribute(Qt::WA_TransparentForMouseEvents,true);
    ui->zLimitSwitchPushButton->setAttribute(Qt::WA_TransparentForMouseEvents,true);
    ui->probeSwitchPushButton->setAttribute(Qt::WA_TransparentForMouseEvents,true);
    ui->doorSwitchPushButton->setAttribute(Qt::WA_TransparentForMouseEvents,true);
    ui->feedHoldSwitchPushButton->setAttribute(Qt::WA_TransparentForMouseEvents,true);
    ui->cycleStartSwitchPushButton->setAttribute(Qt::WA_TransparentForMouseEvents,true);
    ui->resetSwitchPushButton->setAttribute(Qt::WA_TransparentForMouseEvents,true);

    ui->spindleRateProgressBar->setMaximum(255);

    // editingFinished seems to be emitted when Alt is pressed.
    connect( ui->commandComboBox->lineEdit(), &QFocusLineEdit::editingFinished, this, &MainWindow::onGcodeChanged);

    gcodeParser = new GCode();
    machine = nullptr;

    on_jogIntervalSlider_valueChanged( 3 );
    gcodeIndex = 0;

    this->onPortsUpdate();
    connect( &portsTimer, SIGNAL(timeout()), this, SLOT(onPortsUpdate()) );
    portsTimer.start(5000);

    setUIDisconnected();
}

MainWindow::~MainWindow()
{
    if (machine) delete machine;
    delete ui;
}

bool MainWindow::machineOk()
{
    if (!machine) // Security
    {
        QMessageBox::information(this, tr("Machine Error"), tr("No machine connected."));
        return false;
    }
    return true;
}

//bool MainWindow::portOk()
//{
//    if (!port) // Security
//    {
//        QMessageBox::information(this, tr("Port Error"), tr("No port available."));
//        return false;
//    }
//    return true;
//}

void MainWindow::setUIConnected()
{
    ui->statePushButton->setEnabled(true);
    ui->resetToolButton->setEnabled(true);

    ui->connectPushButton->setText(tr("Disconnect","Disconnect button"));
    ui->connectPushButton->setEnabled(true);

    ui->devicesComboBox->setEnabled(false);
    ui->switchesGroupBox->setEnabled(true);
    ui->commandComboBox->setEnabled(true);

    ui->runToolButton->setEnabled(true);
    ui->stepToolButton->setEnabled(true);
    ui->stopToolButton->setEnabled(false);

    ui->xZeroToolButton->setEnabled(true);
    ui->xMachineLineEdit->setEnabled(true);
    ui->xWorkingLineEdit->setEnabled(true);

    ui->yZeroToolButton->setEnabled(true);
    ui->yMachineLineEdit->setEnabled(true);
    ui->yWorkingLineEdit->setEnabled(true);

    ui->zZeroToolButton->setEnabled(true);
    ui->zMachineLineEdit->setEnabled(true);
    ui->zWorkingLineEdit->setEnabled(true);

    movingMachine = movingWorking = false;
    onCoordinatesUpdated();
}

void MainWindow::setUIDisconnected()
{
    ui->statePushButton->setEnabled(false);
    ui->statePushButton->setIcon(QIcon(":/images/leds/led_white.png"));
    ui->statePushButton->setText(tr("Unknown"));
    ui->resetToolButton->setEnabled(false);

    ui->connectPushButton->setText(tr("Connect", "Connect button"));
    ui->connectPushButton->setEnabled(true);

    ui->devicesComboBox->setEnabled(true);
    ui->coordsGroupBox->setEnabled(false);
    ui->jogGroupBox->setEnabled(false);
    ui->actionGroupBox->setEnabled(false);
    ui->switchesGroupBox->setEnabled(false);
    ui->commandComboBox->setEnabled(false);

    ui->runToolButton->setEnabled(false);
    ui->stepToolButton->setEnabled(false);
    ui->stopToolButton->setEnabled(false);

    ui->xZeroToolButton->setEnabled(false);
    ui->xMachineLineEdit->setEnabled(false);
    ui->xWorkingLineEdit->setEnabled(false);
    ui->xMachineLineEdit->setText("-");
    ui->xWorkingLineEdit->setText("-");

    ui->yZeroToolButton->setEnabled(false);
    ui->yMachineLineEdit->setEnabled(false);
    ui->yWorkingLineEdit->setEnabled(false);
    ui->yMachineLineEdit->setText("-");
    ui->yWorkingLineEdit->setText("-");

    ui->zZeroToolButton->setEnabled(false);
    ui->zMachineLineEdit->setEnabled(false);
    ui->zWorkingLineEdit->setEnabled(false);
    ui->zMachineLineEdit->setText("-");
    ui->zWorkingLineEdit->setText("-");

    ui->blockBufferValue->setText("-");
    ui->rxBufferValue->setText("-");

    movingMachine = movingWorking = false;
}

void MainWindow::setUISleeping()
{
    ui->commandComboBox->setEnabled(false);

    ui->runToolButton->setEnabled(false);
    ui->stepToolButton->setEnabled(false);
    ui->stopToolButton->setEnabled(false);

    ui->xZeroToolButton->setEnabled(false);
    ui->xMachineLineEdit->setEnabled(false);
    ui->xWorkingLineEdit->setEnabled(false);

    ui->yZeroToolButton->setEnabled(true);
    ui->yMachineLineEdit->setEnabled(true);
    ui->yWorkingLineEdit->setEnabled(true);

    ui->zZeroToolButton->setEnabled(true);
    ui->zMachineLineEdit->setEnabled(true);
    ui->zWorkingLineEdit->setEnabled(true);

}


#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCborValue>
#include <QCborMap>

void MainWindow::defaultConfiguration()
{
    // This is the default configuration
    config["a"] = true;
    config["b"] = 0;
    config["c"] = "Bonjour";

    config["machines"] = QJsonArray();
}

bool MainWindow::loadConfiguration()
{
    QString configName = "config.json";
    SaveFormat saveFormat = SaveFormat::Json ;

//    QFile loadFile(saveFormat == Json
//            ? QStringLiteral("save.json")
//            : QStringLiteral("save.dat"));
    QFile configFile(configName);

    if (!configFile.open(QIODevice::ReadOnly)) {
        qWarning("MainWindow::saveConfiguration: Couldn't open save file.");
        return false;
    }

    QByteArray configData = configFile.readAll();

    QJsonDocument configDoc(saveFormat == Json
            ? QJsonDocument::fromJson(configData)
            : QJsonDocument(QCborValue::fromCbor(configData).toMap().toJsonObject()));

    config = configDoc.object();
    qDebug() << "MainWindow::saveConfiguration(" << configName << ") done.";
    return true;
}

bool MainWindow::saveConfiguration()
{
    QString configName = "config.json";
    SaveFormat saveFormat = SaveFormat::Json ;

//    QFile saveFile(saveFormat == Json
//           ? QStringLiteral("save.json")
//           : QStringLiteral("save.dat"));
    QFile configFile( configName );

    if (!configFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    configFile.write(saveFormat == SaveFormat::Json
        ? QJsonDocument(config).toJson()
        : QCborValue::fromJsonValue(config).toCbor());

    return true;
}

void MainWindow::homing()
{
    if (!machineOk()) return; // security
    if (!machine->ask(Machine::CommandType::commandHoming))
        qDebug() << "MainWindow::homing: Can't ask for homing.";
}

bool MainWindow::newFile()
{
    if (ui->gcodeCodeEditor->isWindowModified())
        saveFile();

    ui->gcodeCodeEditor->clear();
    return true;
}

void MainWindow::openFile(QString fileName)
{
    if (ui->gcodeCodeEditor->isWindowModified())
        saveFile();

    if (fileName.isEmpty())
    {
        fileName = QFileDialog::getOpenFileName(nullptr,
                tr("Open Gcode File"), "",
                tr("gcode files (*.tap *.nc *.gcode);;All Files (*)"));
    }

    if (fileName.isNull()) return;
    QFile file( fileName );

    if (file.open(QFile::ReadOnly | QFile::Text))
    {
        ui->gcodeCodeEditor->setPlainText(file.readAll());

        if (gcodeParser)
        {
            QStringList lines = ui->gcodeCodeEditor->toPlainText().split("\n");
            gcodeParser->parse( lines );

            ui->linesNbLabel->setText( QString().setNum( lines.size()) );
            ui->pointsNbLabel->setText( QString().setNum(gcodeParser->getPoints().size()) );
            ui->visualizer->setGCode( gcodeParser );

            QVector3D size = gcodeParser->getSize();
            ui->gCodeSizeInfo->setText( QString("%1 / %2 mm")
                        .arg( QString().sprintf("%4.2f", double(size.x())) )
                        .arg( QString().sprintf("%4.2f", double(size.y())) )
                        );

            QVector3D minPoint = gcodeParser->getMin();
            ui->gCodeZeroInfo->setText( QString("%1 / %2 mm")
                        .arg( QString().sprintf("%4.2f",  - double(minPoint.x())) )
                        .arg( QString().sprintf("%4.2f",  - double(minPoint.y())) )
                        );

        }
        gcodeIndex = 0;
    }
    else QMessageBox::critical(this,"Error",QString("Can't read file %1").arg( fileName ));
}

bool MainWindow::saveFile()
{
    if (QMessageBox::question(this,tr("Save Gcode file", "Save dialog caption"), tr("Do you want to save current file ?")) == QMessageBox::Yes)
        return false;
    return true;
}

void MainWindow::closeFile()
{

}

void MainWindow::on_actionNew_triggered()
{
    newFile();
}

void MainWindow::on_actionOpen_triggered()
{
    //openFile("/home/yann/Téléchargements/Adaptateur trou.gcode");
    //return;
    openFile();
}



//----------------------------------------------------------------------------------------------------
#include <QSerialPortInfo>
void MainWindow::onPortsUpdate(void)
{
    QStringList devicesList = PortSerial::getDevices();
    std::sort( devicesList.begin(), devicesList.end());

    // Prevoir de mettre j à 0 si la liste ne contient pas <Select port>
    int i=0, j=1; // ignore first item in list : <Select port>
    int selectedIndex = 0;

#define L1Finished() (i >= devicesList.size())
#define L2Finished() (j >= ui->devicesComboBox->count())
    while ( !L1Finished() || !L2Finished() )
    {
        if (L2Finished())
        {
            ui->devicesComboBox->addItem( devicesList.at(i) );
            ui->statusbar->showMessage(QString(tr("Port %1 added")).arg(devicesList.at(i)), 2000);
            qDebug() << "Port" << devicesList.at(i).toUtf8().data() << "added.";
            if (!selectedIndex) selectedIndex = j;
            i++; j++;
        }
        else if (L1Finished() || (ui->devicesComboBox->itemText(j) > devicesList.at(i)))
        {
            QString device = ui->devicesComboBox->itemText(j);
            if (ui->devicesComboBox->currentIndex() == j)
            {
                ui->devicesComboBox->setCurrentIndex(0);
                if (machine)
                    machine->close();
            }
            ui->devicesComboBox->removeItem(j);
            ui->statusbar->showMessage(QString(tr("Port %1 removed")).arg(device), 2000);
            qDebug() << "Port" << device.toUtf8().data() << "removed.";
        }
        else if (ui->devicesComboBox->itemText(j) < devicesList.at(i))
        {
            ui->devicesComboBox->insertItem(j, devicesList.at(i) );
            ui->statusbar->showMessage(QString(tr("Port %1 inserted")).arg(devicesList.at(i)), 2000);
            qDebug() << "Port" << devicesList.at(i).toUtf8().data() << "inserted.";
            if (!selectedIndex) selectedIndex = j;
            i++;
        }
        else
        {
            i++; j++;
        }
    }
    if (selectedIndex && (ui->devicesComboBox->currentIndex() == 0))
        ui->devicesComboBox->setCurrentIndex(selectedIndex);
}

void MainWindow::openMachine()
{
    setUIDisconnected();

    if (machine)
    {
        closeMachine();
        delete machine;
        machine = nullptr;
        return;
    }
    ui->connectPushButton->setEnabled(false);

    QString portName = ui->devicesComboBox->itemText( ui->devicesComboBox->currentIndex() );
    try {
        qDebug() << "Connecting to" << portName.toUtf8().data();
        ui->statusbar->showMessage(tr("Connecting to machine.", "StatusBar message"));

        machine = new MachineGrbl(this);
        machine->openMachine(portName);

//        connect( machine, SIGNAL(error(Port::PortError)), this, SLOT(onPortError(Port::PortError)));
        connect( machine, SIGNAL(error(int)), this, SLOT(onMachineError(int)) );
        connect( machine, SIGNAL(alarm(int)), this, SLOT(onMachineAlarm(int)) );

        connect( machine, SIGNAL(statusUpdated()), this, SLOT(onStatusUpdated()) );

        connect( machine, SIGNAL(infoReceived(QString)), this, SLOT(onMachineLog(QString)) );
        connect( machine, SIGNAL(commandSent(QString)), this, SLOT(onMachineLog(QString)) );

        connect( machine, SIGNAL(versionUpdated()), this, SLOT(onVersionUpdated()) );
        connect( machine, SIGNAL(stateUpdated()), this, SLOT(onStateUpdated()) );
        connect( machine, SIGNAL(lineNumberUpdated()), this, SLOT(onLineNumberUpdated()) );
        connect( machine, SIGNAL(coordinatesUpdated()), this, SLOT(onCoordinatesUpdated()) );
        connect( machine, SIGNAL(switchesUpdated()), this, SLOT(onSwitchesUpdated()) );
        connect( machine, SIGNAL(actionersUpdated()), this, SLOT(onActionersUpdated()) );
        connect( machine, SIGNAL(ratesUpdated()), this, SLOT(onRatesUpdated()) );
        connect( machine, SIGNAL(buffersUpdated()), this, SLOT(onBuffersUpdated()) );

        connect( machine, SIGNAL(infoUpdated()), this, SLOT(onInfoUpdated()) );

        setUIConnected();
        qDebug() << "Connected to" << portName.toUtf8().data();
        ui->statusbar->showMessage(tr("Connected to machine.", "StatusBar message"));
    } catch (machineConnectException &) {
        qDebug() << "Error connecting to " << portName.toUtf8();
        QMessageBox::critical(this,tr("Connection Error", "Error dialog caption"),
            tr("Unable to connect to %1").arg(portName));
        closeMachine();
    }
}

//void MainWindow::onPortError(Port::PortError error)
//{
//    if (!port) return; // security
//    qDebug() << "Port error : " << error;
//    if (error == Port::ResourceError) {
//        if (port)
//            QMessageBox::critical(this,
//                                  tr("Critical Error", "Port error dialog caption"),
//                                  port->errorString());
//        closePort();
//    }
//    else {
//        if (port)
//            QMessageBox::warning(this, tr("Critical Error", "Port warning dialog caption"), port->errorString());
//    }
//}

void MainWindow::closeMachine()
{
    if (machine)
    {
        delete machine;
        machine = nullptr;
    }

    setUIDisconnected();
}

void MainWindow::onMachineLog( QString line )
{
    if (line == "?")
    {
        return;
    }
    else if ((line.size() == 1) && (line[0].toLatin1() < 0))
    {
        line = QString("0x%1").arg(line[0].toLatin1());
    }
    else if (line.startsWith("<"))
    {
        if (!ui->statusPushButton->isChecked()) return;
    }

    ui->logTextEdit->append( line.trimmed() );
    if (ui->logTextEdit->document()->lineCount() > 200)
    {
        QTextCursor cursor = ui->logTextEdit->textCursor();

        cursor.movePosition(QTextCursor::Start);
        cursor.select(QTextCursor::LineUnderCursor);
        cursor.removeSelectedText();
        cursor.deleteChar();
    }
}

//----------------------------------------------------------------------------------------------------0
void MainWindow::onMachineError(int error)
{
    QMessageBox::critical(this, machine->getErrorMessages(error).shortMessage,
                          machine->getErrorMessages(error).longMessage );
//    QMessageBox::critical(this, QString(tr("Machine Error %1", "Machine error dialog title")).arg( error ),
//                          QString("%2\n\n%3")
//                          .arg( machine->getErrorMessages(error).shortMessage )
//                          .arg( machine->getErrorMessages(error).longMessage )
//                          );
}

void MainWindow::onMachineAlarm(int alarm)
{
    QMessageBox::critical(this, machine->getAlarmMessages(alarm).shortMessage,
                          machine->getAlarmMessages(alarm).longMessage );
//    QMessageBox::critical(this, QString(tr("Machine Alarm %1", "Machine alarm dialog title")).arg( alarm ),
//                          QString("%2\n\n%3")
//                          .arg( machine->getAlarmMessages(alarm).shortMessage )
//                          .arg( machine->getAlarmMessages(alarm).longMessage )
//                          );
    machine->ask(Machine::CommandType::commandReset);
}

//----------------------------------------------------------------------------------------------------

void MainWindow::onStateUpdated()
{
    static Qt::FocusPolicy focusPolicy;

    if (!machineOk()) return; // security

    // Display status in coordinatesBox
    QString stateMessage = machine->getStateMessages( machine->getState() );
    ui->statePushButton->setText( stateMessage );

    if (ui->statePushButton->isChecked())
        ui->statePushButton->setChecked(false);

    QPixmap pixmap;
    QIcon icon;
    switch(machine->getState())
    {
    case Machine::StateType::stateUnknown:
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_white.png"));
        ui->coordsGroupBox->setEnabled(false);
        ui->jogGroupBox->setEnabled(false);
        ui->actionGroupBox->setEnabled(false);

        ui->statePushButton->setToolTip( tr("Machine state is unknown") );

        break;
    case Machine::StateType::stateIdle:
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_green.png"));
        ui->coordsGroupBox->setEnabled(true);
        ui->coordsGroupBox->setAttribute(Qt::WA_TransparentForMouseEvents, false);
        ui->coordsGroupBox->setFocusPolicy(focusPolicy);
        ui->jogGroupBox->setEnabled(true);
        ui->actionGroupBox->setEnabled(true);
        ui->runToolButton->setEnabled(true);
        ui->stepToolButton->setEnabled(true);
        ui->statePushButton->setToolTip( tr("Pause machine") );

        uncheckJogButtons();
        movingMachine = movingWorking = false;
        break;
    case Machine::StateType::stateHold:
        if (doResetOnHold && machine->getHoldCode() == 0)
        {
            machine->ask(Machine::CommandType::commandReset);
            doResetOnHold = false;
        }
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_orange.png"));
        //ui->coordsGroupBox->setEnabled(false);
        ui->jogGroupBox->setEnabled(false);
        ui->actionGroupBox->setEnabled(true);

        ui->statePushButton->setChecked(true);
        ui->statePushButton->setToolTip( tr("Resume machine") );

        break;
    case Machine::StateType::stateJog:
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_yellow.png"));
//        ui->coordsGroupBox->setEnabled(false);
        focusPolicy = ui->coordsGroupBox->focusPolicy();
        ui->coordsGroupBox->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        ui->coordsGroupBox->setFocusPolicy(Qt::NoFocus);

        ui->xWorkingLineEdit->clearFocus();
        ui->yWorkingLineEdit->clearFocus();
        ui->zWorkingLineEdit->clearFocus();
        ui->xMachineLineEdit->clearFocus();
        ui->yMachineLineEdit->clearFocus();
        ui->zMachineLineEdit->clearFocus();

        ui->jogGroupBox->setEnabled(true);
        ui->actionGroupBox->setEnabled(true);
        break;
    case Machine::StateType::stateRun:
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_green.png"));
//        ui->coordsGroupBox->setEnabled(true);
        focusPolicy = ui->coordsGroupBox->focusPolicy();
        ui->coordsGroupBox->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        ui->coordsGroupBox->setFocusPolicy(Qt::NoFocus);

        ui->xWorkingLineEdit->clearFocus();
        ui->yWorkingLineEdit->clearFocus();
        ui->zWorkingLineEdit->clearFocus();
        ui->xMachineLineEdit->clearFocus();
        ui->yMachineLineEdit->clearFocus();
        ui->zMachineLineEdit->clearFocus();

        ui->jogGroupBox->setEnabled(false);
        ui->actionGroupBox->setEnabled(true);
        break;
    case Machine::StateType::stateDoor:
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_orange.png"));
        ui->coordsGroupBox->setEnabled(false);
        ui->jogGroupBox->setEnabled(false);
        ui->actionGroupBox->setEnabled(false);
        break;
    case Machine::StateType::stateHome:
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_violet.png"));
        ui->coordsGroupBox->setEnabled(false);
        ui->jogGroupBox->setEnabled(false);
        ui->actionGroupBox->setEnabled(false);
        break;
    case Machine::StateType::stateAlarm:
    {
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_red.png"));
        ui->coordsGroupBox->setEnabled(false);
        ui->jogGroupBox->setEnabled(false);
        ui->actionGroupBox->setEnabled(false);
        if (!ui->statePushButton->isChecked())
            ui->statePushButton->setChecked(true);

        int alarm = machine->getAlarmCode();
        stateMessage += QString(" %1").arg( alarm );
        ui->statePushButton->setToolTip( QString("%2\n\n%3")
                                         .arg( machine->getAlarmMessages(alarm).shortMessage )
                                         .arg( machine->getAlarmMessages(alarm).longMessage ) );
        stopGcode();
        break;
    }
    case Machine::StateType::stateCheck:
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_blue.png"));
        ui->coordsGroupBox->setEnabled(false);
        ui->jogGroupBox->setEnabled(false);
        ui->actionGroupBox->setEnabled(false);

        ui->statePushButton->setToolTip( tr("Resume normal operations") );

        break;
    case Machine::StateType::stateSleep:
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_black.png"));
        ui->coordsGroupBox->setEnabled(false);
        ui->jogGroupBox->setEnabled(false);
        ui->actionGroupBox->setEnabled(false);
        setUISleeping();

        ui->statePushButton->setToolTip( tr("Wake-up machine") );

        break;
    }

}

void MainWindow::onLineNumberUpdated()
{
    if (!machineOk()) return; // security
    if ( machine->hasInfo( Machine::InfoFlags::flagHasLineNumber ))
    {
        ui->gcodeCodeEditor->setCurrentLine( machine->getLineNumber() );
        ui->lineNbLabel->setText( QString("%1 / %2")
                                  .arg(machine->getLineNumber())
                                  .arg(gcode.size())
                                  );
        //ui->infoLabel->setText( QString("%1").arg(machine->getLineNumber()) );
        ui->gcodeExecutedProgressBar->setValue( machine->getLineNumber() );
    }
}

void MainWindow::onCoordinatesUpdated()
{
    if (!machineOk()) return; // security

    //ui->infoLabel->setText( QString().setNum( machine->getInfos(), 2));

    if (machine->hasInfo( Machine::InfoFlags::flagHasWorkingCoords ))
    {
        QVector3D coords = machine->getWorkingCoordinates();

        if (!ui->xWorkingLineEdit->isModified() )
        {
            ui->xWorkingLineEdit->setText( QString().sprintf("%+03.3f", double(coords.x()) ));
//            if (ui->xWorkingLineEdit == focusWidget())
//                ui->xWorkingLineEdit->selectAll();
        }

        if (!ui->yWorkingLineEdit->isModified())
        {
            ui->yWorkingLineEdit->setText( QString().sprintf("%+03.3f", double(coords.y()) ));
//            if (ui->yWorkingLineEdit == focusWidget())
//                ui->yWorkingLineEdit->selectAll();
        }

        if (!ui->zWorkingLineEdit->isModified())
        {
            ui->zWorkingLineEdit->setText( QString().sprintf("%+03.3f", double(coords.z()) ));
//            if (ui->zWorkingLineEdit == focusWidget())
//                ui->zWorkingLineEdit->selectAll();
        }
    }

    if (machine->hasInfo( Machine::InfoFlags::flagHasMachineCoords ))
    {
        QVector3D coords = machine->getMachineCoordinates();

        if (!ui->xMachineLineEdit->isModified())
        {
            ui->xMachineLineEdit->setText( QString().sprintf("%+03.3f", double(coords.x()) ));
//            if (ui->xMachineLineEdit == focusWidget())
//                ui->xMachineLineEdit->selectAll();
        }

        if (!ui->yMachineLineEdit->isModified())
        {
            ui->yMachineLineEdit->setText( QString().sprintf("%+03.3f", double(coords.y()) ));
//            if (ui->yMachineLineEdit == focusWidget())
//                ui->yMachineLineEdit->selectAll();
        }

        if (!ui->zMachineLineEdit->isModified())
        {
            ui->zMachineLineEdit->setText( QString().sprintf("%+03.3f", double(coords.z()) ));
//            if (ui->zMachineLineEdit == focusWidget())
//                ui->zMachineLineEdit->selectAll();
        }
    }
}

void MainWindow::onSwitchesUpdated()
{
    if (!machineOk()) return; // security
    if (machine->hasInfo( Machine::InfoFlags::flagHasSwitches ))
    {
        ui->xLimitSwitchPushButton->setChecked( machine->hasSwitch( Machine::SwitchFlags::switchLimitX ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
        ui->yLimitSwitchPushButton->setChecked( machine->hasSwitch( Machine::SwitchFlags::switchLimitY ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
        ui->zLimitSwitchPushButton->setChecked( machine->hasSwitch( Machine::SwitchFlags::switchLimitZ ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
        ui->probeSwitchPushButton->setChecked( machine->hasSwitch( Machine::SwitchFlags::switchProbe ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
        ui->doorSwitchPushButton->setChecked( machine->hasSwitch( Machine::SwitchFlags::switchDoor ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
        ui->feedHoldSwitchPushButton->setChecked( machine->hasSwitch( Machine::SwitchFlags::switchFeedHold) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
        ui->cycleStartSwitchPushButton->setChecked( machine->hasSwitch( Machine::SwitchFlags::switchCycleStart) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
        ui->resetSwitchPushButton->setChecked( machine->hasSwitch( Machine::SwitchFlags::switchReset ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
    }
}

void MainWindow::onActionersUpdated()
{
    if (!machineOk()) return; // security
    if (machine->hasInfo( Machine::InfoFlags::flagHasActioners ))
    {
        ui->spindlePushButton->setChecked( machine->hasAction( Machine::ActionerFlags::actionSpindle ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
        ui->coolantFloodPushButton->setChecked( machine->hasAction( MachineGrbl::ActionerFlags::actionCoolantFlood ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
        ui->coolantMistPushButton->setChecked( machine->hasAction( MachineGrbl::ActionerFlags::actionCoolantMist ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
    }
}

void MainWindow::onRatesUpdated()
{
    if (!machineOk()) return; // security
    if ( machine->hasInfo( Machine::InfoFlags::flagHasFeedRate ))
    {
        int feedRate = machine->getFeedRate();
        if (ui->feedRateProgressBar->maximum() < feedRate)
            ui->feedRateProgressBar->setMaximum( feedRate);
        ui->feedRateProgressBar->setValue( feedRate );
        ui->feedRateProgressBar->setEnabled(true);
        ui->feedRateValue->setText( QString("%1").arg( feedRate ));
    }
    else
    {
        ui->spindleRateValue->setText( "-" );
        ui->feedRateProgressBar->setEnabled(false);
    }

    if ( machine->hasInfo( Machine::InfoFlags::flagHasSpindleSpeed ))
    {
        ui->spindleRateValue->setText( QString("%1").arg( machine->getSpindleSpeed() ));
        ui->spindleRateProgressBar->setValue( machine->getFeedRate() );
        ui->spindleRateProgressBar->setEnabled(true);
    }
    else
    {
        ui->spindleRateValue->setText( "-" );
        ui->spindleRateProgressBar->setEnabled(false);
    }
}

void MainWindow::onBuffersUpdated()
{
    if (!machineOk()) return; // security
    if ( machine->hasInfo( Machine::InfoFlags::flagHasBuffer ))
    {
        int blockBuffer = machine->getBlockBuffer();
        int blockBufferMax = machine->getBlockBufferMax();

        if (blockBufferMax < blockBuffer) blockBufferMax = blockBuffer;
        ui->blockBufferValue->setText( QString( tr("%1 / %2", "blockBuffer format") ).arg(blockBuffer).arg(blockBufferMax) );
        ui->blockBufferProgressBar->setMaximum( blockBufferMax );
        ui->blockBufferProgressBar->setValue( blockBufferMax - blockBuffer );

        int rxBuffer = machine->getRXBuffer();
        int rxBufferMax = machine->getRXBufferMax();

        if (rxBufferMax < rxBuffer) rxBufferMax = rxBuffer;
        ui->rxBufferValue->setText( QString( tr("%1 / %2","rxBuffer format") ).arg(rxBuffer).arg(rxBufferMax) );
        ui->rxBufferProgressBar->setMaximum( rxBufferMax );
        ui->rxBufferProgressBar->setValue( rxBufferMax - rxBuffer );
    }
    else
    {
        ui->blockBufferValue->setText( "-" );
        ui->rxBufferValue->setText( "-" );
    }
}

void MainWindow::onStatusUpdated()
{
    if (!machineOk()) return; // security

    // Display status in statusBar for debug ???
    ui->statusbar->showMessage( machine->getLastLine(), 250);
}

void MainWindow::onGcodeChanged()
{
    if (!machineOk()) return; // security
    QByteArray gcode = ui->commandComboBox->currentText().toUtf8();
    if (!gcode.isEmpty())
    {
        if (machine->sendCommand(gcode))
        {
            ui->commandComboBox->addItem( gcode );
            ui->commandComboBox->clearEditText();
        }
    }
}

void MainWindow::onVersionUpdated()
{
    setWindowTitle( QString("%1 %2 (Machine %3)").arg(PROGRAM_NAME).arg(PROGRAM_VERSION).arg(machine->getMachineVersion()));
    qDebug() << "MainWindow::onVersionUpdated()";
}

void MainWindow::onInfoUpdated()
{
    if (machine)
        // Check if CoolantMist is enabled in machine, and enable it in the UI
        if (machine->hasFeature(MachineGrbl::FeatureFlags::flagHasCoolantMist))
            ui->coolantMistPushButton->setEnabled(true);
}

//----------------------------------------------------------------------------------------------------
void MainWindow::checkGcode()
{
    if (machine)
    {
        ui->runToolButton->setEnabled(false);
        ui->stepToolButton->setEnabled(false);
        ui->stopToolButton->setEnabled(false);

        gcodeIndex = 0;
        gcode = ui->gcodeCodeEditor->toPlainText().split("\n", QString::KeepEmptyParts, Qt::CaseInsensitive);

        ui->gcodeExecutedProgressBar->setValue(0);
        ui->gcodeExecutedProgressBar->setMaximum( gcode.size() );

        if (machine->isState( MachineGrbl::StateType::stateIdle))
        {
            machine->ask(MachineGrbl::CommandType::commandCheck);
            connect(machine, SIGNAL(commandExecuted()), this, SLOT(onCommandExecuted()));
            sendNextGCode();
        }
    }
}

void MainWindow::runGcode(bool step)
{
    if (!machineOk()) return; // security

    if (gcodeIndex == 0)
    {
        gcode = ui->gcodeCodeEditor->toPlainText().split("\n", QString::KeepEmptyParts, Qt::CaseInsensitive);

        ui->gcodeExecutedProgressBar->setValue(0);
        ui->gcodeExecutedProgressBar->setMaximum( gcode.size() );

        if (!step)
        {
            machine->ask(MachineGrbl::CommandType::commandOverrideSpindle, MachineGrbl::SubCommandType::commandReset);
            machine->ask(MachineGrbl::CommandType::commandOverrideFeed, MachineGrbl::SubCommandType::commandReset);
            machine->ask(MachineGrbl::CommandType::commandOverrideRapid, MachineGrbl::SubCommandType::commandReset);
            // machine->ask(Grbl::CommandType::commandOverrideCoolantMistToggle);
            // machine->ask(Grbl::CommandType::commandOverrideCoolantMistToggle);

        }
        connect(machine, SIGNAL(commandExecuted()), this, SLOT(onCommandExecuted()));
    }

    stepCommand = step;

    // This pauses immediately, but seems weird !!!
    if (step)
        machine->ask(Machine::CommandType::commandPause, true);
    else
        machine->ask(Machine::CommandType::commandPause, false);

    ui->runToolButton->setEnabled(step);
    ui->stepToolButton->setEnabled(true);
    ui->stopToolButton->setEnabled(true);

    sendNextGCode();
}

void MainWindow::pauseGcode()
{
    qDebug() << "MainWindow::pauseGcode";
}

void MainWindow::stopGcode()
{
    if (machine)
    {
        disconnect(machine, SIGNAL(commandExecuted()), this, SLOT(onCommandExecuted()));
        machine->ask(Machine::CommandType::commandPause, true);
        doResetOnHold = true;

        if (machine->isState(Machine::StateType::stateCheck))
            machine->ask(MachineGrbl::CommandType::commandCheck);
    }

    gcodeIndex = 0;

    ui->runToolButton->setEnabled(true);
    ui->stepToolButton->setEnabled(true);
    ui->stopToolButton->setEnabled(false);

    ui->gcodeExecutedProgressBar->setValue(0);
    ui->lineNbLabel->setText( QString() );
}

void MainWindow::onCommandExecuted()
{
    qDebug() << "MainWindow::onCommandExecuted() stepCommand=" << stepCommand;

    if (stepCommand)
    {
        ui->actionStep->setEnabled(true);
    }
    else sendNextGCode();
}

void MainWindow::sendNextGCode()
{
    if (!machineOk()) return; // security
    if (gcodeIndex < gcode.size())
    {
        machine->sendCommand( QString("N%1%2").arg(gcodeIndex+1).arg(gcode.at(gcodeIndex)).toUtf8() );
        gcodeIndex++;
    }

    if (gcodeIndex >= gcode.size())
    {
        disconnect(machine, SIGNAL(commandExecuted()), this, SLOT(onCommandExecuted()));
        ui->gcodeExecutedProgressBar->setValue(gcode.size());
        ui->lineNbLabel->setText( QString() );
        stopGcode();
    }
};

void MainWindow::resetMachine()
{
    if (!machineOk()) return; // security
    ui->statusbar->showMessage(tr("Resetting machine.", "StatusBar message"));
    machine->ask(Machine::CommandType::commandReset);
}

void MainWindow::uncheckJogButtons()
{
    ui->xMinusToolButton->setChecked(false);
    ui->xPlusToolButton->setChecked(false);
    ui->yMinusToolButton->setChecked(false);
    ui->yPlusToolButton->setChecked(false);
    ui->zMinusToolButton->setChecked(false);
    ui->zPlusToolButton->setChecked(false);

    ui->zeroMachineToolButton->setChecked(false);
    ui->zeroWorkingToolButton->setChecked(false);

    ui->zSafeToolButton->setChecked(false);
    ui->cancelJogToolButton->setChecked(false);
}

//----------------------------------------------------------------------------------------------------
void MainWindow::on_xWorkingLineEdit_focusIn()
{
    ui->xWorkingLineEdit->selectAll();
}

void MainWindow::on_xWorkingLineEdit_focusOut(QFocusEvent*)
{
    // When leaving LineEdit without validating, cancel modification
    ui->xWorkingLineEdit->setModified(false);
}

void MainWindow::on_xWorkingLineEdit_returnPressed()
{
    if (!machineOk()) return; // security
    if (!movingMachine) // Do not do Machine and Working moves at the same time.
    {
        double x = ui->xWorkingLineEdit->text().toDouble();
        if (machine->moveToX(x, 1000 /* feed */, true /* job */))
            movingWorking = true;
    }
    ui->xWorkingLineEdit->setModified(false);
}

void MainWindow::on_yWorkingLineEdit_focusIn()
{
    ui->yWorkingLineEdit->selectAll();
}

void MainWindow::on_yWorkingLineEdit_focusOut(QFocusEvent*)
{
    // When leaving LineEdit without validating, cancel modification
    ui->yWorkingLineEdit->setModified(false);
}

void MainWindow::on_yWorkingLineEdit_returnPressed()
{
    if (!machineOk()) return; // security
    if (!movingMachine) // Do not do Machine and Working moves at the same time.
    {
        double y = ui->yWorkingLineEdit->text().toDouble();
        if (machine->moveToY(y, 1000 /* feed */, true /* job */))
            movingWorking = true;
    }
    ui->yWorkingLineEdit->setModified(false);
}

void MainWindow::on_zWorkingLineEdit_focusIn()
{
    ui->zWorkingLineEdit->selectAll();
}

void MainWindow::on_zWorkingLineEdit_focusOut(QFocusEvent*)
{
    // When leaving LineEdit without validating, cancel modification
    ui->zWorkingLineEdit->setModified(false);
}

void MainWindow::on_zWorkingLineEdit_returnPressed()
{
    if (!machineOk()) return; // security
    if (!movingMachine) // Do not do Machine and Working moves at the same time.
    {
        double z = ui->zWorkingLineEdit->text().toDouble();
        if (machine->moveToZ(z, 1000 /* feed */, true /* job */))
            movingWorking = true;
    }
    ui->zWorkingLineEdit->setModified(false);
}

void MainWindow::on_xMachineLineEdit_focusIn()
{
    ui->xMachineLineEdit->selectAll();
}

void MainWindow::on_xMachineLineEdit_focusOut(QFocusEvent*)
{
    // When leaving LineEdit without validating, cancel modification
    ui->xMachineLineEdit->setModified(false);
}

void MainWindow::on_xMachineLineEdit_returnPressed()
{
    if (!machineOk()) return; // security
    if (!movingWorking) // Do not do Machine and Working moves at the same time.
    {
        double x = ui->xMachineLineEdit->text().toDouble() - double(machine->getWorkingOffset().x());
        if (machine->moveToX(x, 1000 /* feed */, true /* job */, true /* machine */))
            movingMachine = true;
    }
    ui->xMachineLineEdit->setModified(false);
}

void MainWindow::on_yMachineLineEdit_focusIn()
{
    ui->yMachineLineEdit->selectAll();
}

void MainWindow::on_yMachineLineEdit_focusOut(QFocusEvent*)
{
    // When leaving LineEdit without validating, cancel modification
    ui->yMachineLineEdit->setModified(false);
}

void MainWindow::on_yMachineLineEdit_returnPressed()
{
    if (!machineOk()) return; // security
    if (!movingWorking) // Do not do Machine and Working moves at the same time.
    {
        double y = ui->yMachineLineEdit->text().toDouble() - double(machine->getWorkingOffset().y());
        if (machine->moveToY(y, 1000 /* feed */, true /* job */, true /* machine */))
            movingMachine = true;
    }
    ui->yMachineLineEdit->setModified(false);
}

void MainWindow::on_zMachineLineEdit_focusIn()
{
    ui->zMachineLineEdit->selectAll();
}

void MainWindow::on_zMachineLineEdit_focusOut(QFocusEvent*)
{
    // When leaving LineEdit without validating, cancel modification
    ui->zMachineLineEdit->setModified(false);
}

void MainWindow::on_zMachineLineEdit_returnPressed()
{
    if (!machineOk()) return; // security
    if (!movingWorking) // Do not do Machine and Working moves at the same time.
    {
        double z = ui->zMachineLineEdit->text().toDouble() - double(machine->getWorkingOffset().z());
        if (machine->moveToZ(z, 1000 /* feed */, true /* job */, true /* machine */))
            movingMachine = true;
    }
    ui->zMachineLineEdit->setModified(false);
}

//----------------------------------------------------------------------------------------------------
void MainWindow::on_connectPushButton_clicked()
{
    qDebug() << "MainWindow::on_connectPushButton_clicked()";
    openMachine();
}

void MainWindow::on_spindlePushButton_clicked(bool checked)
{
    if (!machineOk()) return; // security

    // Note : If machine is IDLE, send a command, if not, override
    // All this should be put into the machine !!!
    if (machine->getState() == Machine::StateType::stateIdle)
    {
        if (checked)
        {
            if ( machine->sendCommand("M3") )
                ui->spindlePushButton->setChecked(true);
        }
        else {
            if ( machine->sendCommand("M5") )
                ui->spindlePushButton->setChecked(false);
        }
    }
    else
    {
        if (machine->isState(Machine::StateType::stateHold))
        {
            qDebug() << "MainWindow::on_spindlePushButton_clicked: Sending spindle override";
            machine->ask(MachineGrbl::CommandType::commandOverrideSpindle, MachineGrbl::SubCommandType::commandStop);
            ui->spindlePushButton->setChecked(false);
        }
        ui->spindlePushButton->setChecked(false);
    }
}

void MainWindow::on_coolantFloodPushButton_clicked(bool checked)
{
    if (!machineOk()) return; // security

    // Note : If machine is IDLE, send a command, if not, override
    // All this should be put into the machine !!!
    if (machine->getState() == Machine::StateType::stateIdle)
    {
        if (checked)
            machine->sendCommand("M8");
        else {
            machine->sendCommand("M9");
            if (ui->coolantMistPushButton->isChecked())
                machine->sendCommand("M7");
        }
    }
    else
    {
        qDebug() << "MainWindow::on_coolantFloodPushButton_clicked: Sending flood coolant override";
        machine->ask(MachineGrbl::CommandType::commandOverrideCoolantFloodToggle);
    }
}

void MainWindow::on_coolantMistPushButton_clicked(bool checked)
{
    if (!machineOk()) return; // security

    // Note : If machine is IDLE, send a command, if not, use override
    // All this should be put into the machine !!!
    if (machine->getState() == Machine::StateType::stateIdle)
    {
        if (checked)
        {
            machine->sendCommand("M7");
        }
        else {
            machine->sendCommand("M9");
            if (ui->coolantFloodPushButton->isChecked())
                machine->sendCommand("M8");
        }
    }
    else
    {
        qDebug() << "MainWindow::on_coolantMistPushButton_clicked: Sending mist coolant override";
        machine->ask(MachineGrbl::CommandType::commandOverrideCoolantMistToggle);
    }
}

void MainWindow::on_statePushButton_clicked(bool checked)
{
    if (!machineOk()) return; // security
    if (machine->getState() == Machine::StateType::stateIdle)
        machine->ask(Machine::CommandType::commandPause, checked);
    if (machine->getState() == Machine::StateType::stateRun)
        machine->ask(Machine::CommandType::commandPause, checked);
    else if (machine->getState() == Machine::StateType::stateHold)
        machine->ask(Machine::CommandType::commandPause, checked);
    else if (machine->getState() == Machine::StateType::stateAlarm)
        machine->ask(Machine::CommandType::commandUnlock);
    else if (machine->getState() == Machine::StateType::stateSleep)
    {
        // Get out of sleep
        if (!machine->ask(Machine::CommandType::commandReset))
            qDebug() << "MainWindow::on_statePushButton_clicked(): Can't send wake up command";
            //setUIConnected();
    }
}

void MainWindow::on_actionReset_triggered()
{
    resetMachine();
}

void MainWindow::on_xZeroToolButton_clicked()
{
    if (!machineOk()) return; // security
    machine->setXWorkingZero();
}

void MainWindow::on_yZeroToolButton_clicked()
{
    if (!machineOk()) return; // security
    machine->setYWorkingZero();
}

void MainWindow::on_zZeroToolButton_clicked()
{
    if (!machineOk()) return; // security
    machine->setZWorkingZero();
}

void MainWindow::on_actionConfig_triggered()
{
    if (!machineOk()) return; // security
    machine->openConfiguration();
}

void MainWindow::on_resetToolButton_clicked()
{
    resetMachine();
}

void MainWindow::on_cancelJogToolButton_clicked()
{
    if (!machineOk()) return; // security
    if (machine->getState() == Machine::StateType::stateJog)
        machine->ask(MachineGrbl::CommandType::commandJogCancel);
    else ui->cancelJogToolButton->setChecked(false);
}

//-----------------------------------------------------------------------------------------
void MainWindow::on_xMinusToolButton_clicked()
{
    if (!machineOk()) return; // security
    if (!machine->moveToX(-jogInterval, 1000 /* feed */, true /* jog */, false /* machine */, true /* incremental */))
        qDebug() << "MainWindow::on_xMinusToolButton_clicked(): Can't send command.";
}

void MainWindow::on_xPlusToolButton_clicked()
{
    if (!machineOk()) return; // security
    if (!machine->moveToX(jogInterval, 1000 /* feed */, true /* jog */, false /* machine */, true /* incremental */))
        qDebug() << "MainWindow::on_xPlusToolButton_clicked(): Can't send command.";
}

void MainWindow::on_yMinusToolButton_clicked()
{
    if (!machineOk()) return; // security
    if (!machine->moveToY(-jogInterval, 1000 /* feed */, true /* jog */, false /* machine */, true /* incremental */))
        qDebug() << "MainWindow::on_yMinusToolButton_clicked(): Can't send command.";
}

void MainWindow::on_yPlusToolButton_clicked()
{
    if (!machineOk()) return; // security
    if (!machine->moveToY(jogInterval, 1000 /* feed */, true /* jog */, false /* machine */, true /* incremental */))
        qDebug() << "MainWindow::on_yPlusToolButton_clicked(): Can't send command.";
}

void MainWindow::on_zMinusToolButton_clicked()
{
    if (!machineOk()) return; // security
    if (!machine->moveToZ(-jogInterval, 1000 /* feed */, true /* jog */, false /* machine */, true /* incremental */))
        qDebug() << "MainWindow::on_zMinusToolButton_clicked(): Can't send command.";
}

void MainWindow::on_zPlusToolButton_clicked()
{
    if (!machineOk()) return; // security
    if (!machine->moveToZ(jogInterval, 1000 /* feed */, true /* jog */, false /* machine */, true /* incremental */))
        qDebug() << "MainWindow::on_zPlusToolButton_clicked(): Can't send command.";
}

void MainWindow::on_zSafeToolButton_clicked()
{
    if (!machineOk()) return; // security
    machine->moveToZ(2, 1000 /* feed */, true /* jog */, false /* machine */, false /* incremental */);
}

//-----------------------------------------------------------------------------------------
void MainWindow::on_jogIntervalSlider_valueChanged(int value)
{
    switch(value)
    {
    case 0:
        jogInterval = 0.01;
        break;
    case 1:
        jogInterval = 0.1;
        break;
    case 2:
        jogInterval = 1;
        break;
    case 3:
        jogInterval = 10;
        break;
    case 4:
        jogInterval = 100;
        break;
    default:
        jogInterval = 10;
    }
    ui->jogIntervalLineEdit->setText( QString("%1").arg(jogInterval));
}

void MainWindow::on_runToolButton_clicked()
{
    runGcode();
}

void MainWindow::on_stepToolButton_clicked()
{
    runGcode(true);
}

void MainWindow::on_stopToolButton_clicked()
{
    stopGcode();
    if (!machineOk()) return; // security
    // resetMachine();
}

#define zSafe 5
//void MainWindow::zeroWorking()
void MainWindow::on_zeroWorkingToolButton_clicked()
{
    if (!machineOk()) return; // security

    QVector3D coords = machine->getWorkingCoordinates();
    if (coords.z() < zSafe)
    {
        if (!machine->moveToZ(zSafe, 1000 /* feed */, true /* jog */, false /* machine */, false /* incremental */))
//        if (!machine->sendCommand("$J=G90Z5F1000")) // how much zSafe whould be ?
        {
            qDebug() << "MainWindow::zeroWorking: Can't execute zSafe command.";
            // there should be a QMessage box here ?
            return;
        }
    }

    if (!machine->moveToXY(0, 0, 1000 /* feed */, true /* jog */, false /* machine */, false /* incremental */))
//    if (!machine->sendCommand("$J=G90X0Y0F1000"))
        qDebug() << "MainWindow::zeroWorking: Can't execute xyZero command.";
}

//void MainWindow::on_zeroWorkingToolButton_clicked()
//{
//    zeroWorking();
//}

//void MainWindow::zeroMachine()
void MainWindow::on_zeroMachineToolButton_clicked()
{
    if (!machineOk()) return; // security
    QVector3D coords = machine->getMachineCoordinates();
    if (coords.z() < zSafe)
    {
        if (!machine->moveToZ(zSafe, 1000 /* feed */, true /* jog */, false /* machine */, false /* incremental */))
//        if (machine->sendCommand("$J=G90G53Z0F1000"))
        {
            qDebug() << "MainWindow::zeroMachine: Can't execute zSafe command.";
            // there should be a QMessage box here ?
            ui->zeroMachineToolButton->setChecked(false);
            return;
        }
    }

    if ((coords.x() != 0) || (coords.y() != 0))
    {
        if (!machine->moveToXY(0, 0, 1000 /* feed */, true /* jog */, true /* machine */, false /* incremental */))
//        if (machine->sendCommand("$J=G90G53X0Y0F1000"))
        {
            qDebug() << "MainWindow::zeroMachine: Can't execute xyZero command.";
            return;
        }
        if (!machine->moveToZ(0, 1000 /* feed */, true /* jog */, true /* machine */, false /* incremental */))
        {
            qDebug() << "MainWindow::zeroMachine: Can't execute zZero command.";
            return;
        }
    }
    else
    {
        ui->zeroMachineToolButton->setChecked(false);
    }
}

//void MainWindow::on_homeMachineToolButton_clicked()
//{
//    zeroMachine();
//}


void MainWindow::on_homingToolButton_clicked()
{
    homing();
}


void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, PROGRAM_NAME,
                       QString("%1 %2").arg(PROGRAM_NAME).arg(PROGRAM_VERSION)
                       + "\n" +
                       QString("Yann LE GUENNEC <yann@leguennec.net>")
                       );
}


void MainWindow::on_gCodeExecutionSlider_valueChanged(int value)
{
    ui->visualizer->setExecution(value);
}

void MainWindow::on_topViewToolButton_clicked()
{
    ui->visualizer->setRotation(QVector3D( -1440.0f , 0.0f, -2880.0f));
}

void MainWindow::on_isometricViewToolButton_clicked()
{
    ui->visualizer->setRotation(QVector3D( -600.0f, 0.0f, -2200.0f ));
}
void MainWindow::on_pushButton_clicked()
{
    ui->logTextEdit->clear();
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (index == 2)
        ui->visualizer->update();
}
