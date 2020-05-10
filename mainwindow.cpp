#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QString>
#include <QStringList>
#include <QSerialPort>
#include <QFileDialog>
#include <QMessageBox>


#include "logger.h"
#include "grbl.h"

#include "stdio.h"

#include <QWidget>
#include <QLineEdit>
#include <QDebug>

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

    connect( ui->xWorkingLineEdit, SIGNAL(focusOut(QFocusEvent *)), this, SLOT(xWorkingLineEdit_focusOut(QFocusEvent *)));
    connect( ui->yWorkingLineEdit, SIGNAL(focusOut(QFocusEvent *)), this, SLOT(yWorkingLineEdit_focusOut(QFocusEvent *)));
    connect( ui->zWorkingLineEdit, SIGNAL(focusOut(QFocusEvent *)), this, SLOT(zWorkingLineEdit_focusOut(QFocusEvent *)));

    connect( ui->xMachineLineEdit, SIGNAL(focusOut(QFocusEvent *)), this, SLOT(xMachineLineEdit_focusOut(QFocusEvent *)));
    connect( ui->yMachineLineEdit, SIGNAL(focusOut(QFocusEvent *)), this, SLOT(yMachineLineEdit_focusOut(QFocusEvent *)));
    connect( ui->zMachineLineEdit, SIGNAL(focusOut(QFocusEvent *)), this, SLOT(zMachineLineEdit_focusOut(QFocusEvent *)));

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

    disconnectUi();
}

MainWindow::~MainWindow()
{
    delete machine;
    delete port;
    delete ui;
}

void MainWindow::disconnectUi(void)
{
    ui->connectPushButton->setText("Connect");
    ui->connectPushButton->setEnabled(true);

    ui->devicesComboBox->setEnabled(true);
    ui->gcodeComboBox->setEnabled(false);

    ui->switchesGroupBox->setEnabled(false);
    ui->actionGroupBox->setEnabled(false);
    ui->coordsGroupBox->setEnabled(false);

    ui->xMachineLineEdit->setText("-");
    ui->yMachineLineEdit->setText("-");
    ui->zMachineLineEdit->setText("-");

    ui->xWorkingLineEdit->setText("-");
    ui->yWorkingLineEdit->setText("-");
    ui->zWorkingLineEdit->setText("-");

    ui->blockBufferValue->setText("-");
    ui->rxBufferValue->setText("-");

    moveMachine = moveWorking = false;
}


void MainWindow::connectUi(void)
{
    ui->connectPushButton->setText("Disconnect");
    ui->connectPushButton->setEnabled(true);

    ui->devicesComboBox->setEnabled(false);
    ui->gcodeComboBox->setEnabled(true);

    ui->switchesGroupBox->setEnabled(true);
    ui->actionGroupBox->setEnabled(true);
    ui->coordsGroupBox->setEnabled(true);

    moveMachine = moveWorking = false;
}



void MainWindow::about()
{
    QMessageBox::about(this, PROGRAM_NAME, PROGRAM_VERSION);
}

bool MainWindow::newFile()
{
    if (ui->gcodeCodeEditor->isWindowModified())
        if (QMessageBox::question(this,"Save file", "Do you want to save current file ?") == QMessageBox::Yes)
            saveFile();

    ui->gcodeCodeEditor->clear();
    return true;
}

void MainWindow::openFile(QString fileName)
{
    if (ui->gcodeCodeEditor->isWindowModified())
        if (QMessageBox::question(this,"Save file", "Do you want to save current file ?") == QMessageBox::Yes)
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

void MainWindow::saveFile()
{

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
            ui->statusBar->showMessage(QString("Port %1 added").arg(devicesList.at(i)), 2000);
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
            ui->statusBar->showMessage(QString("Port %1 removed").arg(device), 2000);
            qDebug() << "Port" << device.toUtf8().data() << "removed.";
        }
        else if (ui->devicesComboBox->itemText(j) < devicesList.at(i))
        {
            ui->devicesComboBox->insertItem(j, devicesList.at(i) );
            ui->statusBar->showMessage(QString("Port %1 added").arg(devicesList.at(i)), 2000);
            qDebug() << "Port" << devicesList.at(i).toUtf8().data() << "added.";
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
    disconnectUi();
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
            ui->statusBar->showMessage("Starting machine.");
            machine = new Grbl(port);

            connect( machine, SIGNAL(statusUpdated()), this, SLOT(statusUpdated()) );
            connect( machine, SIGNAL(infoUpdated()), this, SLOT(infoUpdated()) );
            connect( machine, SIGNAL(error(int)), this, SLOT(handleMachineError(int)) );

            connect(port, SIGNAL(error(Port::PortError)), this, SLOT(handlePortError(Port::PortError)));

            connectUi();
        }
        else
        {
            qDebug() << "Error connecting to " << portName.toUtf8();
            QMessageBox::critical(this,"Connection Error", QString("Unable to connect to %1\n%2").arg(portName).arg(port->errorString()));
            closePort();
        }
    }
}

void MainWindow::handlePortError(Port::PortError error)
{
    if (!port) return; // security
    qDebug() << "Port error : " << error;
    if (error == Port::ResourceError) {
        if (port)
            QMessageBox::critical(this, tr("Critical Error"), port->errorString());
        closePort();
    }
    else {
        if (port)
            QMessageBox::critical(this, tr("Critical Error"), port->errorString());
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

    disconnectUi();
}

void MainWindow::handleMachineError(int error)
{
    QString msg = QString("Error %1: %2").arg(error).arg( machine->getErrorString(error) );
    QMessageBox::critical(this, "Machine Error", msg);
}

//----------------------------------------------------------------------------------------------------

void MainWindow::statusUpdated()
{
    if (!machine) return; // security
    //ui->infoLabel->setText( QString("%1").arg(machine->hasInfo( Machine::flagWaitForReset )) );

    // Display status in statusBar
    if (!machine->hasInfo( Machine::InfoFlags::flagWaitForReset ))
        ui->statusBar->showMessage( machine->getLastLine(), 250);

    if (machine->hasInfo(Machine::InfoFlags::flagHasLineNumber))
    {
        QTextCursor cursor;
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, machine->getLineNumber() );
        ui->gcodeCodeEditor->setTextCursor(cursor);
        ui->gcodeCodeEditor->centerCursor();

        ui->infoLabel->setText( QString("%1").arg(machine->getLineNumber()) );
        ui->gcodeExecutedProgressBar->setValue( machine->getLineNumber() );
    }

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
        ui->statePushButton->setIcon(QIcon(":/leds/images/leds/led_green.png"));
        moveMachine = moveWorking = false;
        break;
    case Machine::StateType::stateHold:
        ui->statePushButton->setIcon(QIcon(":/leds/images/leds/led_orange.png"));
        if (!ui->statePushButton->isChecked())
            ui->statePushButton->setChecked(true);
        break;
    case Machine::StateType::stateJog:
        ui->statePushButton->setIcon(QIcon(":/leds/images/leds/led_yellow.png"));
        break;
    case Machine::StateType::stateRun:
        ui->statePushButton->setIcon(QIcon(":/leds/images/leds/led_green.png"));
        break;
    case Machine::StateType::stateDoor:
        ui->statePushButton->setIcon(QIcon(":/leds/images/leds/led_orange.png"));
        break;
    case Machine::StateType::stateHome:
        ui->statePushButton->setIcon(QIcon(":/leds/images/leds/led_violet.png"));
        break;
    case Machine::StateType::stateAlarm:
        if (!ui->statePushButton->isChecked())
            ui->statePushButton->setChecked(true);
        break;
    case Machine::StateType::stateCheck:
        ui->statePushButton->setIcon(QIcon(":/leds/images/leds/led_blue.png"));
        break;
    case Machine::StateType::stateSleep:
        ui->statePushButton->setIcon(QIcon(":/leds/images/leds/led_black.png"));
        break;
    }

    // This is just for debug, not useful
    ui->xLabel->setText( ui->xWorkingLineEdit->isModified() ? "X": "x" );
    ui->yLabel->setText( ui->yWorkingLineEdit->isModified() ? "Y": "y" );
    ui->zLabel->setText( ui->zWorkingLineEdit->isModified() ? "Z": "z" );

    if (machine->hasInfo( Machine::InfoFlags::flagHasWorkingCoords ))
    {
        Machine::CoordinatesType coords = machine->getWorkingCoordinates();

        if (!ui->xWorkingLineEdit->isModified() )
        {
            ui->xWorkingLineEdit->setText( QString().sprintf( "%+03.3f", coords.x ));
            if (ui->xWorkingLineEdit == focusWidget())
                ui->xWorkingLineEdit->selectAll();
        }

        if (!ui->yWorkingLineEdit->isModified())
        {
            ui->yWorkingLineEdit->setText( QString().sprintf( "%+03.3f", coords.y ));
            if (ui->yWorkingLineEdit == focusWidget())
                ui->yWorkingLineEdit->selectAll();
        }

    //    if (ui->zWorkingLineEdit!=focusWidget())
        if (!ui->zWorkingLineEdit->isModified())
        {
            ui->zWorkingLineEdit->setText( QString().sprintf( "%+03.3f", coords.z ));
            if (ui->zWorkingLineEdit == focusWidget())
                ui->zWorkingLineEdit->selectAll();
        }
    }

    if (machine->hasInfo( Machine::InfoFlags::flagHasMachineCoords ))
    {
        Machine::CoordinatesType coords = machine->getMachineCoordinates();

        if (!ui->xMachineLineEdit->isModified())
        {
            ui->xMachineLineEdit->setText( QString().sprintf( "%+03.3f", coords.x ));
            if (ui->xMachineLineEdit == focusWidget())
                ui->xMachineLineEdit->selectAll();
        }

        if (!ui->yMachineLineEdit->isModified())
        {
            ui->yMachineLineEdit->setText( QString().sprintf( "%+03.3f", coords.y ));
            if (ui->yMachineLineEdit == focusWidget())
                ui->yMachineLineEdit->selectAll();
        }

        if (!ui->zMachineLineEdit->isModified())
        {
            ui->zMachineLineEdit->setText( QString().sprintf( "%+03.3f", coords.z ));
            if (ui->zMachineLineEdit == focusWidget())
                ui->zMachineLineEdit->selectAll();
        }
    }

    // Display switches
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

    // Display actions
    if (machine->hasInfo( Machine::InfoFlags::flagHasActions ))
    {
        ui->spindlePushButton->setChecked( machine->hasAction( Machine::ActionerFlags::actionSpindle ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
        ui->coolantFloodPushButton->setChecked( machine->hasAction( Grbl::ActionerFlags::actionCoolantFlood ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
        ui->coolantMistPushButton->setChecked( machine->hasAction( Grbl::ActionerFlags::actionCoolantMist ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
    }

    // Display Feed Rate
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

    // Display Spindle Speed
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

    // Display Line Number
    if ( machine->hasInfo( Machine::InfoFlags::flagHasLineNumber ))
    {
        ui->gcodeCodeEditor->setCurrentLine( machine->getLineNumber() );
    }

    // Display buffers
    if ( machine->hasInfo( Machine::InfoFlags::flagHasBuffer ))
    {
        int blockBuffer = machine->getBlockBuffer();
        int blockBufferMax = machine->getBlockBufferMax();

        if (blockBufferMax < blockBuffer) blockBufferMax = blockBuffer;
        ui->blockBufferValue->setText( QString("%1 / %2").arg(blockBuffer).arg(blockBufferMax) );
        ui->blockBufferProgressBar->setMaximum( blockBufferMax );
        ui->blockBufferProgressBar->setValue( blockBufferMax - blockBuffer );

        int rxBuffer = machine->getRXBuffer();
        int rxBufferMax = machine->getRXBufferMax();

        if (rxBufferMax < rxBuffer) rxBufferMax = rxBuffer;
        ui->rxBufferValue->setText( QString("%1 / %2").arg(rxBuffer).arg(rxBufferMax) );
        ui->rxBufferProgressBar->setMaximum( rxBufferMax );
        ui->rxBufferProgressBar->setValue( rxBufferMax - rxBuffer );
    }
    else
    {
        ui->blockBufferValue->setText( "-" );
        ui->rxBufferValue->setText( "-" );
    }
}

void MainWindow::gcodeChanged()
{
    if (!machine) return; // security
    QByteArray gcode = ui->gcodeComboBox->currentText().toUtf8();
    if (!gcode.isEmpty())
    {
        if (machine->sendGCode(gcode))
        {
            ui->gcodeComboBox->addItem( gcode );
            ui->gcodeComboBox->clearEditText();
        }
    }
}

void MainWindow::infoUpdated()
{
    if (machine)
        // Check if CoolantMist is enabled in machine, and enable it in the UI
        if (machine->hasInfo(Grbl::InfoFlags::flagHasCoolantMist))
            ui->coolantMistPushButton->setEnabled(true);
}

//----------------------------------------------------------------------------------------------------
void MainWindow::runGcode(bool step)
{
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
    if (!machine) return;
    if (gcodeIndex < gcode.size())
    {
        machine->sendGCode( QString("N%1%2").arg(gcodeIndex+1).arg(gcode.at(gcodeIndex)).toUtf8() );
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
void MainWindow::xWorkingLineEdit_focusOut(QFocusEvent*)
{
    ui->xWorkingLineEdit->setModified(false);
}

void MainWindow::on_xWorkingLineEdit_returnPressed()
{
    if (!machine) return; // security
    if (!moveMachine)
    {
        double x = ui->xWorkingLineEdit->text().toDouble();
        if (machine->sendGCode(QString("G0X%1").arg(x).toUtf8()))
        {
            moveWorking = true;
            //ui->label->setText("moveWorking set");
        }
    }
    ui->xWorkingLineEdit->setModified(false);
}

void MainWindow::yWorkingLineEdit_focusOut(QFocusEvent*)
{
    ui->yWorkingLineEdit->setModified(false);
}

void MainWindow::on_yWorkingLineEdit_returnPressed()
{
    if (!machine) return; // security
    if (!moveMachine)
    {
        double y = ui->yWorkingLineEdit->text().toDouble();
        if (machine->sendGCode(QString("G0Y%1").arg(y).toUtf8()))
        {
            moveWorking = true;
            //ui->label->setText("moveWorking set");
        }
    }
    ui->yWorkingLineEdit->setModified(false);
}

void MainWindow::zWorkingLineEdit_focusOut(QFocusEvent*)
{
    ui->zWorkingLineEdit->setModified(false);
}

void MainWindow::on_zWorkingLineEdit_returnPressed()
{
    if (!machine) return; // security
    if (!moveMachine)
    {
        double z = ui->zWorkingLineEdit->text().toDouble();
        if (machine->sendGCode(QString("G0Z%1").arg(z).toUtf8()))
        {
            moveWorking = true;
            //ui->label->setText("moveWorking set");
        }
    }
    ui->zWorkingLineEdit->setModified(false);
}

void MainWindow::xMachineLineEdit_focusOut(QFocusEvent*)
{
    ui->xMachineLineEdit->setModified(false);
}

void MainWindow::on_xMachineLineEdit_returnPressed()
{
    if (!machine) return; // security
    if (!moveWorking)
    {
        double x = ui->xMachineLineEdit->text().toDouble() - machine->getWorkingOffset().x;
        if (machine->sendGCode(QString("G0X%1").arg(x).toUtf8()))
        {
            moveMachine = true;
            //ui->label->setText("moveMachine set");
        }
    }
    ui->xMachineLineEdit->setModified(false);
}

void MainWindow::yMachineLineEdit_focusOut(QFocusEvent*)
{
    ui->yMachineLineEdit->setModified(false);
}

void MainWindow::on_yMachineLineEdit_returnPressed()
{
    if (!machine) return; // security
    if (!moveWorking)
    {
        double y = ui->yMachineLineEdit->text().toDouble() - machine->getWorkingOffset().y;
        if (machine->sendGCode(QString("G0Y%1").arg(y).toUtf8()))
        {
            moveMachine = true;
            //ui->label->setText("moveMachine set");
        }
    }
    ui->yMachineLineEdit->setModified(false);
}

void MainWindow::zMachineLineEdit_focusOut(QFocusEvent*)
{
    ui->zMachineLineEdit->setModified(false);
}

void MainWindow::on_zMachineLineEdit_returnPressed()
{
    if (!machine) return; // security
    if (!moveWorking)
    {
        double z = ui->zMachineLineEdit->text().toDouble() - machine->getWorkingOffset().z;
        if (machine->sendGCode(QString("G0Z%1").arg(z).toUtf8()))
        {
            moveMachine = true;
            //ui->label->setText("moveMachine set");
        }
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
    if (!machine) return; // security

    // Note : If machine is IDLE, send a command, if not, override
    if (machine->getState() == Machine::StateType::stateIdle)
    {
        if (checked)
        {
            if ( machine->sendGCode("M3") )
                ui->spindlePushButton->setChecked(true);
        }
        else {
            if ( machine->sendGCode("M5") )
                ui->spindlePushButton->setChecked(false);
        }
    }
    else
        machine->ask(Grbl::CommandType::commandOverrideSpindle, Grbl::SubCommandType::commandStop);
}

void MainWindow::on_coolantFloodPushButton_clicked(bool checked)
{
    if (!machine) return; // security

    // Note : If machine is IDLE, send a command, if not, override
    if (machine->getState() == Machine::StateType::stateIdle)
    {
        if (checked)
            machine->sendGCode("M8");
        else {
            machine->sendGCode("M9");
            if (ui->coolantMistPushButton->isChecked())
                machine->sendGCode("M7");
        }
    }
    else
        machine->ask(Grbl::CommandType::commandOverrideCoolantFloodToggle);
}

void MainWindow::on_coolantMistPushButton_clicked(bool checked)
{
    if (!machine) return; // security

    // Note : If machine is IDLE, send a command, if not, override
    if (machine->getState() == Machine::StateType::stateIdle)
    {
        if (checked)
        {
            machine->sendGCode("M7");
        }
        else {
            machine->sendGCode("M9");
            if (ui->coolantFloodPushButton->isChecked())
                machine->sendGCode("M8");
        }
    }
    else
        machine->ask(Grbl::CommandType::commandOverrideCoolantMistToggle);
}

void MainWindow::on_statePushButton_clicked(bool checked)
{
    if (!machine) return; // security
    if (machine->getState() == Machine::StateType::stateIdle)
        machine->ask(Machine::CommandType::commandPause, checked);
    else if (machine->getState() == Machine::StateType::stateHold)
        machine->ask(Machine::CommandType::commandPause, checked);
    else if (machine->getState() == Machine::StateType::stateAlarm)
        machine->ask(Machine::CommandType::commandUnlock);
}

void MainWindow::on_actionReset_triggered()
{
    if (!machine) return; // security
    ui->statusBar->showMessage("Resetting machine.");
    machine->ask(Machine::CommandType::commandReset);
}

void MainWindow::on_xZeroToolButton_clicked()
{
    if (!machine) return; // security
    machine->sendGCode( "G92X0" );
}

void MainWindow::on_yZeroToolButton_clicked()
{
    if (!machine) return; // security
    machine->sendGCode( "G92Y0" );
}

void MainWindow::on_zZeroToolButton_clicked()
{
    if (!machine) return; // security
    machine->sendGCode( "G92Z0" );
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
    if (machine)
        machine->ask(Machine::CommandType::commandReset);
}

void MainWindow::on_homePushButton_clicked()
{
    if (machine->ask(Machine::CommandType::commandHome))
        ui->homePushButton->setChecked(false);
}

void MainWindow::on_actionConfig_triggered()
{
    if (!machine) return; // Security
    machine->OpenConfiguration(this);
}
