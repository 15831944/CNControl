#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "stdio.h"

#include <QString>
#include <QStringList>
#include <QSerialPort>
#include <QWidget>
#include <QMessageBox>
#include <QFileDialog>
//#include <QLineEdit>
#include <QDebug>

#include "grbl.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
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

    connect( ui->gcodeComboBox->lineEdit(), &QLineEdit::editingFinished, this, &MainWindow::gcodeChanged);

//    connect( ui->xWorkingLineEdit, SIGNAL(focusOut(QFocusEvent *)), this, SLOT(xWorkingLineEdit_focusOut(QFocusEvent *)));
//    connect( ui->yWorkingLineEdit, SIGNAL(focusOut(QFocusEvent *)), this, SLOT(yWorkingLineEdit_focusOut(QFocusEvent *)));
//    connect( ui->zWorkingLineEdit, SIGNAL(focusOut(QFocusEvent *)), this, SLOT(zWorkingLineEdit_focusOut(QFocusEvent *)));

//    connect( ui->xMachineLineEdit, SIGNAL(focusOut(QFocusEvent *)), this, SLOT(xMachineLineEdit_focusOut(QFocusEvent *)));
//    connect( ui->yMachineLineEdit, SIGNAL(focusOut(QFocusEvent *)), this, SLOT(yMachineLineEdit_focusOut(QFocusEvent *)));
//    connect( ui->zMachineLineEdit, SIGNAL(focusOut(QFocusEvent *)), this, SLOT(zMachineLineEdit_focusOut(QFocusEvent *)));

    SerialPort *serial = new SerialPort();
    serial->setSpeed( 38400 );
    port = serial;

    machine = nullptr;

    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(10);
    ui->gcodeCodeEditor->setFont(font);
    highlighter = new Highlighter(ui->gcodeCodeEditor->document());

    this->updatePorts();
    connect( &portsTimer, SIGNAL(timeout()), this, SLOT(updatePorts()) );
    portsTimer.start(5000);

    setUIDisconnected();
}

MainWindow::~MainWindow()
{
    delete machine;
    delete port;
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

bool MainWindow::portOk()
{
    if (!port) // Security
    {
        QMessageBox::information(this, tr("Port Error"), tr("No port available."));
        return false;
    }
    return true;
}

void MainWindow::setUIDisconnected()
{
    ui->statePushButton->setEnabled(false);

    ui->connectPushButton->setText(tr("Connect", "Connect button"));
    ui->connectPushButton->setEnabled(true);

    ui->devicesComboBox->setEnabled(true);
    ui->gcodeComboBox->setEnabled(false);

    ui->actionGridLayout->setEnabled(false);
    ui->coordsGroupBox->setEnabled(false);

    ui->xMachineLineEdit->setText("-");
    ui->yMachineLineEdit->setText("-");
    ui->zMachineLineEdit->setText("-");

    ui->xWorkingLineEdit->setText("-");
    ui->yWorkingLineEdit->setText("-");
    ui->zWorkingLineEdit->setText("-");

    ui->blockBufferValue->setText("-");
    ui->rxBufferValue->setText("-");

    ui->toolBar->setEnabled(false);

    moveMachine = moveWorking = false;
}

void MainWindow::setUIConnected()
{
    ui->statePushButton->setEnabled(true);

    ui->connectPushButton->setText(tr("Disconnect","Disconnect button"));
    ui->connectPushButton->setEnabled(true);

    ui->devicesComboBox->setEnabled(false);
    ui->gcodeComboBox->setEnabled(true);

    ui->actionGridLayout->setEnabled(true);
    ui->coordsGroupBox->setEnabled(true);

    ui->toolBar->setEnabled(true);

    moveMachine = moveWorking = false;
    onCoordinatesUpdated();
}

void MainWindow::setUISleeping()
{
    ui->actionGridLayout->setEnabled(false);
    ui->gcodeComboBox->setEnabled(false);
    ui->coordsGroupBox->setEnabled(false);
    ui->toolBar->setEnabled(false);
}

void MainWindow::about()
{
    QMessageBox::about(this, PROGRAM_NAME, PROGRAM_VERSION);
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
                tr("gcode files (*.tap;*.nc;*.gcode);;All Files (*)"));
    }

    QFile file( fileName );

    if (file.open(QFile::ReadOnly | QFile::Text))
    {
        ui->gcodeCodeEditor->setPlainText(file.readAll());
        ui->gcodeCodeEditor->setCurrentLine( 10);
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
    openFile("/home/yann/Téléchargements/Adaptateur trou.gcode");
    return;
    openFile();
}



//----------------------------------------------------------------------------------------------------
void MainWindow::updatePorts(void)
{
    if (!port) return; // security
    QStringList devicesList = port->getDevices();
    std::sort( devicesList.begin(), devicesList.end());

    // Prevoir de mettre j à 0 si la liste ne contient pas <Select port>
    int i=0, j=1; // ignore first item in list : <Select port>

#define L1Finished() (i >= devicesList.size())
#define L2Finished() (j >= ui->devicesComboBox->count())
    while ( !L1Finished() || !L2Finished() )
    {
        if (L2Finished())
        {
            ui->devicesComboBox->addItem( devicesList.at(i) );
            ui->statusBar->showMessage(QString(tr("Port %1 added")).arg(devicesList.at(i)), 2000);
            qDebug() << "Port" << devicesList.at(i).toUtf8().data() << "added.";
            i++; j++;
        }
        else if (L1Finished() || (ui->devicesComboBox->itemText(j) > devicesList.at(i)))
        {
            QString device = ui->devicesComboBox->itemText(j);
            if (ui->devicesComboBox->currentIndex() == j)
            {
                ui->devicesComboBox->setCurrentIndex(0);
                if (port->isOpen())
                    closePort();
            }
            ui->devicesComboBox->removeItem(j);
            ui->statusBar->showMessage(QString(tr("Port %1 removed")).arg(device), 2000);
            qDebug() << "Port" << device.toUtf8().data() << "removed.";
        }
        else if (ui->devicesComboBox->itemText(j) < devicesList.at(i))
        {
            ui->devicesComboBox->insertItem(j, devicesList.at(i) );
            ui->statusBar->showMessage(QString(tr("Port %1 inserted")).arg(devicesList.at(i)), 2000);
            qDebug() << "Port" << devicesList.at(i).toUtf8().data() << "inserted.";
            i++;
        }
        else
        {
            i++; j++;
        }
    }

}

void MainWindow::openPort()
{
    if (!port) return; // security
    setUIDisconnected();
    ui->connectPushButton->setEnabled(false);

    if (port->isOpen())
        closePort();
    else
    {
        QString portName = ui->devicesComboBox->itemText( ui->devicesComboBox->currentIndex() );
        port->setDevice( portName );

        if (port->open())
        {
            if (machine)
                delete machine;

            qDebug() << "Connected to" << portName.toUtf8().data();
            ui->statusBar->showMessage(tr("Starting machine.", "StatusBar message"));
            machine = new Grbl(port);

            connect( port, SIGNAL(error(Port::PortError)), this, SLOT(onPortError(Port::PortError)));
            connect( machine, SIGNAL(error(int)), this, SLOT(onMachineError(int)) );

            connect( machine, SIGNAL(statusUpdated()), this, SLOT(onStatusUpdated()) );

            connect( machine, SIGNAL(versionUpdated()), this, SLOT(onVersionUpdated()) );
            connect( machine, SIGNAL(stateUpdated()), this, SLOT(onStateUpdated()) );
            connect( machine, SIGNAL(lineNumberUpdated()), this, SLOT(onLineNumberUpdated()) );
            connect( machine, SIGNAL(coordinatesUpdated()), this, SLOT(onCoordinatesUpdated()) );
            connect( machine, SIGNAL(switchesUpdated()), this, SLOT(onSwitchesUpdated()) );
            connect( machine, SIGNAL(actionsUpdated()), this, SLOT(onActionsUpdated()) );
            connect( machine, SIGNAL(ratesUpdated()), this, SLOT(onRatesUpdated()) );
            connect( machine, SIGNAL(buffersUpdated()), this, SLOT(onBuffersUpdated()) );

            connect( machine, SIGNAL(infoUpdated()), this, SLOT(infoUpdated()) );

            setUIConnected();
        }
        else
        {
            qDebug() << "Error connecting to " << portName.toUtf8();
            QMessageBox::critical(this,tr("Connection Error","Error dialog caption"),
                QString(tr("Unable to connect to %1\n%2")).arg(portName).arg(port->errorString()));
            closePort();
        }
    }
}

void MainWindow::onPortError(Port::PortError error)
{
    if (!port) return; // security
    qDebug() << "Port error : " << error;
    if (error == Port::ResourceError) {
        if (port)
            QMessageBox::critical(this, tr("Critical Error", "Port error dialog caption"), port->errorString());
        closePort();
    }
    else {
        if (port)
            QMessageBox::warning(this, tr("Critical Error", "Port warning dialog caption"), port->errorString());
    }
}

void MainWindow::closePort()
{
    if (machine)
    {
        delete machine;
        machine = nullptr;
    }
    if (port)
        port->close();

    setUIDisconnected();
}

void MainWindow::onMachineError(int error)
{
    QString msg = QString(tr("Error %1: %2","Machine error message")).arg(error).arg( machine->getErrorString(error) );
    QMessageBox::critical(this, tr("Machine Error","Machine error dialog caption"), msg);
}

//----------------------------------------------------------------------------------------------------

void MainWindow::onStateUpdated()
{
    if (!machineOk()) return; // security

    // Display status in coordinatesBox
    ui->statePushButton->setText( machine->getStateString() );

    if (ui->statePushButton->isChecked())
        ui->statePushButton->setChecked(false);

    QPixmap pixmap;
    QIcon icon;
    switch(machine->getState())
    {
    case Machine::StateType::stateUnknown:
//        QPixmap pixmap("~/Images/icones/leds/led_white.png");
//        QIcon ButtonIcon(pixmap);
//        ui->statusPushButton->setIcon(ButtonIcon);
//        ui->statusPushButton->setIconSize(pixmap.rect().size());
        break;
    case Machine::StateType::stateIdle:
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_green.png"));
        moveMachine = moveWorking = false;
        break;
    case Machine::StateType::stateHold:
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_orange.png"));
        if (!ui->statePushButton->isChecked())
            ui->statePushButton->setChecked(true);
        break;
    case Machine::StateType::stateJog:
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_yellow.png"));
        break;
    case Machine::StateType::stateRun:
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_green.png"));
        break;
    case Machine::StateType::stateDoor:
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_orange.png"));
        break;
    case Machine::StateType::stateHome:
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_violet.png"));
        break;
    case Machine::StateType::stateAlarm:
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_red.png"));
        if (!ui->statePushButton->isChecked())
            ui->statePushButton->setChecked(true);
        break;
    case Machine::StateType::stateCheck:
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_blue.png"));
        break;
    case Machine::StateType::stateSleep:
        ui->statePushButton->setIcon(QIcon(":/images/leds/led_black.png"));
        setUISleeping();
        break;
    }

}

void MainWindow::onLineNumberUpdated()
{
    if (!machineOk()) return; // security
    if ( machine->hasInfo( Machine::InfoFlags::flagHasLineNumber ))
    {
        ui->gcodeCodeEditor->setCurrentLine( machine->getLineNumber() );
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
        Machine::CoordinatesType coords = machine->getWorkingCoordinates();

        if (!ui->xWorkingLineEdit->isModified() )
        {
            ui->xWorkingLineEdit->setText( QString().sprintf("%+03.3f", coords.x ));
            if (ui->xWorkingLineEdit == focusWidget())
                ui->xWorkingLineEdit->selectAll();
        }

        if (!ui->yWorkingLineEdit->isModified())
        {
            ui->yWorkingLineEdit->setText( QString().sprintf("%+03.3f", coords.y ));
            if (ui->yWorkingLineEdit == focusWidget())
                ui->yWorkingLineEdit->selectAll();
        }

        if (!ui->zWorkingLineEdit->isModified())
        {
            ui->zWorkingLineEdit->setText( QString().sprintf("%+03.3f", coords.z ));
            if (ui->zWorkingLineEdit == focusWidget())
                ui->zWorkingLineEdit->selectAll();
        }
    }

    if (machine->hasInfo( Machine::InfoFlags::flagHasMachineCoords ))
    {
        Machine::CoordinatesType coords = machine->getMachineCoordinates();

        if (!ui->xMachineLineEdit->isModified())
        {
            ui->xMachineLineEdit->setText( QString().sprintf("%+03.3f", coords.x ));
            if (ui->xMachineLineEdit == focusWidget())
                ui->xMachineLineEdit->selectAll();
        }

        if (!ui->yMachineLineEdit->isModified())
        {
            ui->yMachineLineEdit->setText( QString().sprintf("%+03.3f", coords.y ));
            if (ui->yMachineLineEdit == focusWidget())
                ui->yMachineLineEdit->selectAll();
        }

        if (!ui->zMachineLineEdit->isModified())
        {
            ui->zMachineLineEdit->setText( QString().sprintf("%+03.3f", coords.z ));
            if (ui->zMachineLineEdit == focusWidget())
                ui->zMachineLineEdit->selectAll();
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

void MainWindow::onActionsUpdated()
{
    if (!machineOk()) return; // security
    if (machine->hasInfo( Machine::InfoFlags::flagHasActions ))
    {
        ui->spindlePushButton->setChecked( machine->hasAction( Machine::ActionerFlags::actionSpindle ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
        ui->coolantFloodPushButton->setChecked( machine->hasAction( Grbl::ActionerFlags::actionCoolantFlood ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
        ui->coolantMistPushButton->setChecked( machine->hasAction( Grbl::ActionerFlags::actionCoolantMist ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
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
    ui->statusBar->showMessage( machine->getLastLine(), 250);
}

void MainWindow::gcodeChanged()
{
    if (!machineOk()) return; // security
    QByteArray gcode = ui->gcodeComboBox->currentText().toUtf8();
    if (!gcode.isEmpty())
    {
        if (machine->sendCommand(gcode))
        {
            ui->gcodeComboBox->addItem( gcode );
            ui->gcodeComboBox->clearEditText();
        }
    }
}

void MainWindow::onVersionUpdated()
{
    setWindowTitle( QString("GCodeSender (%1)").arg(machine->getMachineVersion()));
    qDebug() << "MainWindow::onVersionUpdated()";
}

void MainWindow::infoUpdated()
{
    if (machine)
        // Check if CoolantMist is enabled in machine, and enable it in the UI
        if (machine->hasFeature(Grbl::FeatureFlags::flagHasCoolantMist))
            ui->coolantMistPushButton->setEnabled(true);
}

//----------------------------------------------------------------------------------------------------
void MainWindow::runGcode(bool step)
{
    if (!machineOk()) return; // security

    //ui->gcodeCodeEditor->setReadOnly(true);
    ui->actionRun->setEnabled(false);
    ui->actionStep->setEnabled(step);
    ui->actionStop->setEnabled(true);
    ui->actionPause->setEnabled(!step);

    if (gcodeIndex == 0)
    {
        gcode = ui->gcodeCodeEditor->toPlainText().split("\n", QString::KeepEmptyParts, Qt::CaseInsensitive);

        ui->gcodeSentProgressBar->setValue(0);
        ui->gcodeSentProgressBar->setMaximum( gcode.size() );

        ui->gcodeExecutedProgressBar->setValue(0);
        ui->gcodeExecutedProgressBar->setMaximum( gcode.size() );

        if (machine)
            connect(machine, SIGNAL(commandExecuted()), this, SLOT(commandExecuted()));
        gcodeIndex = 0;

        if (!step)
        {
            machine->ask(Grbl::CommandType::commandOverrideSpindle, Grbl::SubCommandType::commandReset);
            machine->ask(Grbl::CommandType::commandOverrideFeed, Grbl::SubCommandType::commandReset);
            machine->ask(Grbl::CommandType::commandOverrideRapid, Grbl::SubCommandType::commandReset);
            machine->ask(Grbl::CommandType::commandOverrideCoolantMistToggle);
            machine->ask(Grbl::CommandType::commandOverrideCoolantMistToggle);

        }
    }

    stepCommand = step;
    sendNextGCode();
}

void MainWindow::pauseGcode()
{
    if (machine)
        disconnect(machine, SIGNAL(commandExecuted()), this, SLOT(commandExecuted()));
    //ui->gcodeCodeEditor->setReadOnly(false);
    ui->actionRun->setEnabled(true);
    ui->actionStep->setEnabled(true);
    ui->actionStop->setEnabled(false);
    ui->actionPause->setEnabled(false);
}

void MainWindow::stopGcode()
{
    if (machine)
        disconnect(machine, SIGNAL(commandExecuted()), this, SLOT(commandExecuted()));
    //ui->gcodeCodeEditor->setReadOnly(false);
    ui->actionRun->setEnabled(true);
    ui->actionStep->setEnabled(true);
    ui->actionStop->setEnabled(false);
    ui->actionPause->setEnabled(false);
    gcodeIndex = 0;
    ui->gcodeSentProgressBar->setValue(0);
}

void MainWindow::commandExecuted()
{
    qDebug() << "MainWindow::commandExecuted()";
    if (stepCommand)
    {
        ui->actionRun->setEnabled(true);
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
        ui->gcodeSentProgressBar->setValue(gcodeIndex);
        gcodeIndex++;
    }

    if (gcodeIndex >= gcode.size())
    {
        ui->gcodeSentProgressBar->setValue(gcode.size());
        ui->gcodeExecutedProgressBar->setValue(gcode.size());
        stopGcode();
    }
};

//----------------------------------------------------------------------------------------------------
void MainWindow::on_xWorkingLineEdit_focusOut(QFocusEvent*)
{
    // When leaving LineEdit without validating, cancel modification
    ui->xWorkingLineEdit->setModified(false);
}

void MainWindow::on_xWorkingLineEdit_returnPressed()
{
    if (!machineOk()) return; // security
    if (!moveMachine) // Do not do Machine and Working moves at the same time.
    {
        double x = ui->xWorkingLineEdit->text().toDouble();
        if (machine->sendCommand(QString("G0X%1").arg(x).toUtf8()))
            moveWorking = true;
    }
    ui->xWorkingLineEdit->setModified(false);
}

void MainWindow::on_yWorkingLineEdit_focusOut(QFocusEvent*)
{
    // When leaving LineEdit without validating, cancel modification
    ui->yWorkingLineEdit->setModified(false);
}

void MainWindow::on_yWorkingLineEdit_returnPressed()
{
    if (!machineOk()) return; // security
    if (!moveMachine) // Do not do Machine and Working moves at the same time.
    {
        double y = ui->yWorkingLineEdit->text().toDouble();
        if (machine->sendCommand(QString("G0Y%1").arg(y).toUtf8()))
            moveWorking = true;
    }
    ui->yWorkingLineEdit->setModified(false);
}

void MainWindow::on_zWorkingLineEdit_focusOut(QFocusEvent*)
{
    // When leaving LineEdit without validating, cancel modification
    ui->zWorkingLineEdit->setModified(false);
}

void MainWindow::on_zWorkingLineEdit_returnPressed()
{
    if (!machineOk()) return; // security
    if (!moveMachine) // Do not do Machine and Working moves at the same time.
    {
        double z = ui->zWorkingLineEdit->text().toDouble();
        if (machine->sendCommand(QString("G0Z%1").arg(z).toUtf8()))
            moveWorking = true;
    }
    ui->zWorkingLineEdit->setModified(false);
}

void MainWindow::on_xMachineLineEdit_focusOut(QFocusEvent*)
{
    // When leaving LineEdit without validating, cancel modification
    ui->xMachineLineEdit->setModified(false);
}

void MainWindow::on_xMachineLineEdit_returnPressed()
{
    if (!machineOk()) return; // security
    if (!moveWorking) // Do not do Machine and Working moves at the same time.
    {
        double x = ui->xMachineLineEdit->text().toDouble() - machine->getWorkingOffset().x;
        if (machine->sendCommand(QString("G0X%1").arg(x).toUtf8()))
            moveMachine = true;
    }
    ui->xMachineLineEdit->setModified(false);
}

void MainWindow::on_yMachineLineEdit_focusOut(QFocusEvent*)
{
    // When leaving LineEdit without validating, cancel modification
    ui->yMachineLineEdit->setModified(false);
}

void MainWindow::on_yMachineLineEdit_returnPressed()
{
    if (!machineOk()) return; // security
    if (!moveWorking) // Do not do Machine and Working moves at the same time.
    {
        double y = ui->yMachineLineEdit->text().toDouble() - machine->getWorkingOffset().y;
        if (machine->sendCommand(QString("G0Y%1").arg(y).toUtf8()))
            moveMachine = true;
    }
    ui->yMachineLineEdit->setModified(false);
}

void MainWindow::on_zMachineLineEdit_focusOut(QFocusEvent*)
{
    // When leaving LineEdit without validating, cancel modification
    ui->zMachineLineEdit->setModified(false);
}

void MainWindow::on_zMachineLineEdit_returnPressed()
{
    if (!machineOk()) return; // security
    if (!moveWorking) // Do not do Machine and Working moves at the same time.
    {
        double z = ui->zMachineLineEdit->text().toDouble() - machine->getWorkingOffset().z;
        if (machine->sendCommand(QString("G0Z%1").arg(z).toUtf8()))
            moveMachine = true;
    }
    ui->zMachineLineEdit->setModified(false);
}

//----------------------------------------------------------------------------------------------------
void MainWindow::on_connectPushButton_clicked()
{
    openPort();
}

void MainWindow::on_spindlePushButton_clicked(bool checked)
{
    if (!machineOk()) return; // security

    // Note : If machine is IDLE, send a command, if not, override
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
        machine->ask(Grbl::CommandType::commandOverrideSpindle, Grbl::SubCommandType::commandStop);
}

void MainWindow::on_coolantFloodPushButton_clicked(bool checked)
{
    if (!machineOk()) return; // security

    // Note : If machine is IDLE, send a command, if not, override
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
        machine->ask(Grbl::CommandType::commandOverrideCoolantFloodToggle);
}

void MainWindow::on_coolantMistPushButton_clicked(bool checked)
{
    if (!machineOk()) return; // security

    // Note : If machine is IDLE, send a command, if not, override
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
        machine->ask(Grbl::CommandType::commandOverrideCoolantMistToggle);
}

void MainWindow::on_statePushButton_clicked(bool checked)
{
    if (!machineOk()) return; // security
    if (machine->getState() == Machine::StateType::stateIdle)
        machine->ask(Machine::CommandType::commandPause, checked);
    else if (machine->getState() == Machine::StateType::stateHold)
        machine->ask(Machine::CommandType::commandPause, checked);
    else if (machine->getState() == Machine::StateType::stateAlarm)
        machine->ask(Machine::CommandType::commandUnlock);
    else if (machine->getState() == Machine::StateType::stateSleep)
    {
        // Get out of sleep
        if (machine->ask(Machine::CommandType::commandReset))
            setUIConnected();
    }
}

void MainWindow::on_actionReset_triggered()
{
    if (!machineOk()) return; // security
    ui->statusBar->showMessage(tr("Resetting machine.", "StatusBar message"));
    machine->ask(Machine::CommandType::commandReset);
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

void MainWindow::on_actionRun_triggered()
{
    runGcode();
}

void MainWindow::on_actionStep_triggered()
{
    runGcode(true);
}

void MainWindow::on_actionPause_triggered()
{
    pauseGcode();
}

void MainWindow::on_actionStop_triggered()
{
    stopGcode();
    if (!machineOk()) return; // security
    machine->ask(Machine::CommandType::commandReset);
}

void MainWindow::on_homePushButton_clicked()
{
    if (!machineOk()) return; // security
    if (machine->ask(Machine::CommandType::commandHome))
        ui->homePushButton->setChecked(false);
}

void MainWindow::on_actionConfig_triggered()
{
    if (!machineOk()) return; // security
    machine->openConfiguration(this);
}


void MainWindow::on_ZeroPushButton_clicked()
{
    if (!machineOk()) return; // security
    machine->sendCommand("G28");
}

void MainWindow::on_pushButton_clicked()
{
    if (!machineOk()) return; // security
    machine->sendCommand("G30");
}
