#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QKeyEvent>
#include <QTimer>

#include "port_serial.h"
#include "gcode.h"
#include "machine.h"
#include "highlighter.h"

#define PROGRAM_NAME "CNControl"
#define PROGRAM_VERSION "v1.0"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool machineOk();
    bool portOk();


    void setUIConnected();
    void setUIDisconnected();
    void setUISleeping();
    //bool eventFilter(QObject *obj, QEvent *event);

    void openPort();
    void closePort();

    bool gcodeSend(QString gcode);
    void sendNextGCode();

    void resetMachine();
    void uncheckJogButtons();
    void zeroWorking();
    void zeroMachine();

public slots:
    bool newFile();
    //void openFile();
    void openFile(QString filename = QString());
    bool saveFile();
    void closeFile();

    void homing();

    void checkGcode();
    void runGcode(bool step = false);
    void pauseGcode();
    void stopGcode();

private slots:
    void onVersionUpdated();

    void onStateUpdated();
    void onLineNumberUpdated();
    void onCoordinatesUpdated();
    void onSwitchesUpdated();
    void onActionersUpdated();
    void onRatesUpdated();
    void onBuffersUpdated();
    void onInfoUpdated();
    void onStatusUpdated();
    void onGcodeChanged();
    void onPortsUpdate();

    void onPortError(Port::PortError error);
    void onMachineError(int error);
    void onMachineAlarm(int alarm);

    void onCommandExecuted();

    // Automatically connected :
    void on_connectPushButton_clicked();

    void on_xWorkingLineEdit_focusIn();
    void on_yWorkingLineEdit_focusIn();
    void on_zWorkingLineEdit_focusIn();

    void on_xWorkingLineEdit_focusOut(QFocusEvent*);
    void on_yWorkingLineEdit_focusOut(QFocusEvent*);
    void on_zWorkingLineEdit_focusOut(QFocusEvent*);

    void on_xWorkingLineEdit_returnPressed();
    void on_yWorkingLineEdit_returnPressed();
    void on_zWorkingLineEdit_returnPressed();

    void on_xMachineLineEdit_focusIn();
    void on_yMachineLineEdit_focusIn();
    void on_zMachineLineEdit_focusIn();

    void on_xMachineLineEdit_focusOut(QFocusEvent*);
    void on_yMachineLineEdit_focusOut(QFocusEvent*);
    void on_zMachineLineEdit_focusOut(QFocusEvent*);

    void on_xMachineLineEdit_returnPressed();
    void on_yMachineLineEdit_returnPressed();
    void on_zMachineLineEdit_returnPressed();

    void on_actionOpen_triggered();
    void on_actionReset_triggered();

    void on_statePushButton_clicked(bool checked);
    void on_spindlePushButton_clicked(bool checked);
    void on_coolantFloodPushButton_clicked(bool checked);
    void on_coolantMistPushButton_clicked(bool checked);

    void on_xZeroToolButton_clicked();
    void on_yZeroToolButton_clicked();
    void on_zZeroToolButton_clicked();

    void on_actionConfig_triggered();

    void on_xMinusToolButton_clicked();
    void on_xPlusToolButton_clicked();
    void on_yMinusToolButton_clicked();
    void on_yPlusToolButton_clicked();
    void on_zPlusToolButton_clicked();
    void on_zMinusToolButton_clicked();
    void on_zSafeToolButton_clicked();

    void on_resetToolButton_clicked();
    void on_cancelJogToolButton_clicked();

    void on_runToolButton_clicked();
    void on_stepToolButton_clicked();

    void on_stopToolButton_clicked();
    void on_actionNew_triggered();

    void on_homeWorkingToolButton_clicked();
    void on_homeMachineToolButton_clicked();

    void on_homingToolButton_clicked();

    void on_actionAbout_triggered();

    void on_jogIntervalSlider_valueChanged(int value);

    void on_gCodeExecutionSlider_valueChanged(int value);

private:
    Ui::MainWindow *ui;
    QTimer  portsTimer;

    Highlighter *highlighter;

    QStringList devicesList;
    Machine *machine;
    Port *port;
    double jogInterval;

    GCode *gcodeParser;

    QStringList gcode;
    int gcodeIndex;
    bool stepCommand, doResetOnHold;

    bool moveMachine, moveWorking;
};

#endif // MAINWINDOW_H
