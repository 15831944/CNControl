#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QKeyEvent>
#include <QTimer>

#include "port_serial.h"
#include "machine.h"
#include "highlighter.h"

#define PROGRAM_NAME "ControlNCenter"
#define PROGRAM_VERSION "ControlNCenter v1.0"

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

//    const QString lastError();

public slots:
    void about();
    bool newFile();
    //void openFile();
    void openFile(QString filename = QString());
    bool saveFile();
    void closeFile();

private slots:
    void onVersionUpdated();

    void onStateUpdated();
    void onLineNumberUpdated();
    void onCoordinatesUpdated();
    void onSwitchesUpdated();
    void onActionsUpdated();
    void onRatesUpdated();
    void onBuffersUpdated();


    void onStatusUpdated();
    void gcodeChanged();    

    void updatePorts();
    void infoUpdated();

    void onPortError(Port::PortError error);
    void onMachineError(int error);

    void runGcode(bool step = false);
    void pauseGcode();
    void stopGcode();
    void commandExecuted();

    // Automatically connected :
    void on_connectPushButton_clicked();

    void on_xWorkingLineEdit_focusOut(QFocusEvent*);
    void on_yWorkingLineEdit_focusOut(QFocusEvent*);
    void on_zWorkingLineEdit_focusOut(QFocusEvent*);

    void on_xWorkingLineEdit_returnPressed();
    void on_yWorkingLineEdit_returnPressed();
    void on_zWorkingLineEdit_returnPressed();

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

    void on_actionNew_triggered();

    void on_actionRun_triggered();
    void on_actionStep_triggered();
    void on_actionPause_triggered();
    void on_actionStop_triggered();

    void on_homePushButton_clicked();

    void on_actionConfig_triggered();

    void on_ZeroPushButton_clicked();

    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    QTimer  portsTimer;

    Highlighter *highlighter;

    QStringList devicesList;
    Machine *machine;
    Port *port;

    QStringList gcode;
    int gcodeIndex;
    bool stepCommand;

    bool moveMachine, moveWorking;
};

#endif // MAINWINDOW_H
