#include "grblconfiguration.h"
#include "ui_grblconfiguration.h"

#include <QMap>
#include <QDebug>

GrblConfiguration::GrblConfiguration(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GrblConfiguration)
{
    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(0);
}

GrblConfiguration::~GrblConfiguration()
{
    delete ui;
}

#define xFlagMask (1<<2)
#define yFlagMask (1<<1)
#define zFlagMask (1<<0)
#define noFlagMask 0

//void GrblConfiguration::setConfiguration(QMap<uint, double> config, quint64 infos)
void GrblConfiguration::setConfiguration(Grbl *grbl)
{
    for( auto key : grbl->config.keys() )
    {
        QString val = grbl->config[key];
        uint valUInt = val.toUInt();
        qDebug()<<"key="<<key<<",val="<<val;

        switch (key)
        {
        case Grbl::ConfigType::configStepPulse: ui->stepPulseLineEdit->setText( val ); break;
        case Grbl::ConfigType::configStepIdleDelay: ui->stepIdleDelayLineEdit->setText( val ); break;
        case Grbl::ConfigType::configStepPortInvert:
            ui->xStepPortInvertCheckBox->setChecked( valUInt & xFlagMask );
            ui->yStepPortInvertCheckBox->setChecked( valUInt & yFlagMask );
            ui->zStepPortInvertCheckBox->setChecked( valUInt & zFlagMask );
            break;
        case Grbl::ConfigType::configDirPortInvert:
            ui->xDirPortInvertCheckBox->setChecked( valUInt & xFlagMask );
            ui->yDirPortInvertCheckBox->setChecked( valUInt & yFlagMask );
            ui->zDirPortInvertCheckBox->setChecked( valUInt & zFlagMask );
            break;
        case Grbl::ConfigType::configStepEnableInvert: ui->stepEnableInvertCheckBox->setChecked( val == '1' ); break;
        case Grbl::ConfigType::configLimitPinInvert: ui->limitPinsInvertCheckBox->setChecked( val == '1' ); break;
        case Grbl::ConfigType::configProbePinInvert: ui->probePinInvertCheckBox->setChecked( val == '1' ); break;

        case Grbl::ConfigType::configStatusReport:
            // Warning : no verification of index
            ui->statusReportComboBox->setCurrentIndex( val.toInt() );
            // Mask
            break;
        case Grbl::ConfigType::configJunctionDeviation: ui->junctionDeviationLineEdit->setText( val ); break;
        case Grbl::ConfigType::configArcTolerance: ui->arcToleranceLineEdit->setText( val ); break;

            // Warning : no verification of index
        case Grbl::ConfigType::configReportInches: ui->reportInchesComboBox->setCurrentIndex( val.toInt() ); break;
        case Grbl::ConfigType::configSoftLimits: ui->softLimitsCheckBox->setChecked( val == '1' ); break;
        case Grbl::ConfigType::configHardLimits: ui->hardLimitsCheckBox->setChecked( val == '1' ); break;

        case Grbl::ConfigType::configHomingCycle: ui->homingCycleCheckBox->setChecked( val == '1' ); break;
        case Grbl::ConfigType::configHomingDirInvert:
            ui->xHomingDirInvertCheckBox->setChecked( valUInt & xFlagMask );
            ui->yHomingDirInvertCheckBox->setChecked( valUInt & yFlagMask );
            ui->zHomingDirInvertCheckBox->setChecked( valUInt & zFlagMask );
            break;
        case Grbl::ConfigType::configHomingFeed: ui->homingFeedLineEdit->setText( val ); break;
        case Grbl::ConfigType::configHomingSeek: ui->homingSeekLineEdit->setText( val ); break;
        case Grbl::ConfigType::configHomingDebounce: ui->homingDebounceLineEdit->setText( val ); break;
        case Grbl::ConfigType::configHomingPullOff: ui->homingPullofflineEdit->setText( val ); break;

        case Grbl::ConfigType::configMaxSpindleSpeed: ui->spindleSpeedMaxLineEdit->setText( val ); break;
        case Grbl::ConfigType::configMinSpindleSpeed: ui->spindleSpeedMinLineEdit->setText( val ); break;

        case Grbl::ConfigType::configLaserMode:
            ui->laserModeCheckBox->setEnabled( grbl->hasFeature( Grbl::InfoFlags::flagHasLaserMode) );
            ui->laserModeCheckBox->setChecked( val == 1 );
            break;

        case Grbl::ConfigType::configXSteps: ui->xStepsLineEdit->setText( val ); break;
        case Grbl::ConfigType::configYSteps: ui->yStepsLineEdit->setText( val ); break;
        case Grbl::ConfigType::configZSteps: ui->zStepsLineEdit->setText( val ); break;

        case Grbl::ConfigType::configXMaxRate: ui->xMaxRateLineEdit->setText( val ); break;
        case Grbl::ConfigType::configYMaxRate: ui->yMaxRateLineEdit->setText( val ); break;
        case Grbl::ConfigType::configZMaxRate: ui->zMaxRateLineEdit->setText( val ); break;

        case Grbl::ConfigType::configXAcceleration: ui->xAccelerationLineEdit->setText( val ); break;
        case Grbl::ConfigType::configYAcceleration: ui->yAccelerationLineEdit->setText( val ); break;
        case Grbl::ConfigType::configZAcceleration: ui->zAccelerationLineEdit->setText( val ); break;

        case Grbl::ConfigType::configXMaxTravel: ui->xMaxTravelLineEdit->setText( val ); break;
        case Grbl::ConfigType::configYMaxTravel: ui->yMaxTravelLineEdit->setText( val ); break;
        case Grbl::ConfigType::configZMaxTravel: ui->zMaxTravelLineEdit->setText( val ); break;

        case Grbl::ConfigType::configStartingBlock0:
            ui->startingBlock0LineEdit->setText( val ); break;
        case Grbl::ConfigType::configStartingBlock1:
            ui->startingBlock1LineEdit->setText( val ); break;

        default: qDebug() << "GrblConfiguration::setConfiguration: Key unknown " << key;
        }
    }
}

bool GrblConfiguration::getConfiguration(Grbl *grbl)
{
    grbl->config[ Grbl::ConfigType::configStepPulse         ] = ui->stepPulseLineEdit->text();
    grbl->config[ Grbl::ConfigType::configStepIdleDelay     ] = ui->stepIdleDelayLineEdit->text();
    grbl->config[ Grbl::ConfigType::configStepPortInvert    ] = QString("%1").arg(
                                                                (ui->xStepPortInvertCheckBox->isChecked()?xFlagMask:noFlagMask) |
                                                                (ui->yStepPortInvertCheckBox->isChecked()?yFlagMask:noFlagMask) |
                                                                (ui->zStepPortInvertCheckBox->isChecked()?zFlagMask:noFlagMask) );
    grbl->config[ Grbl::ConfigType::configDirPortInvert     ] = QString("%1").arg(
                                                                (ui->xDirPortInvertCheckBox->isChecked()?xFlagMask:noFlagMask) |
                                                                (ui->yDirPortInvertCheckBox->isChecked()?yFlagMask:noFlagMask) |
                                                                (ui->zDirPortInvertCheckBox->isChecked()?zFlagMask:noFlagMask) );
    grbl->config[ Grbl::ConfigType::configStepEnableInvert  ] = ui->stepEnableInvertCheckBox->isChecked()?'1':'0';
    grbl->config[ Grbl::ConfigType::configLimitPinInvert    ] = ui->limitPinsInvertCheckBox->isChecked()?'1':'0';
    grbl->config[ Grbl::ConfigType::configProbePinInvert    ] = ui->probePinInvertCheckBox->isChecked()?'1':'0';
    grbl->config[ Grbl::ConfigType::configStatusReport      ] = QString("%1").arg(ui->statusReportComboBox->currentIndex());
    grbl->config[ Grbl::ConfigType::configJunctionDeviation ] = ui->junctionDeviationLineEdit->text();
    grbl->config[ Grbl::ConfigType::configArcTolerance      ] = ui->arcToleranceLineEdit->text();
    grbl->config[ Grbl::ConfigType::configReportInches      ] = QString("%1").arg(ui->reportInchesComboBox->currentIndex());
    grbl->config[ Grbl::ConfigType::configSoftLimits        ] = ui->softLimitsCheckBox->isChecked()?'1':'0';
    grbl->config[ Grbl::ConfigType::configHardLimits        ] = ui->hardLimitsCheckBox->isChecked()?'1':'0';
    grbl->config[ Grbl::ConfigType::configHomingCycle       ] = ui->homingCycleCheckBox->isChecked()?'1':'0';
    grbl->config[ Grbl::ConfigType::configHomingDirInvert   ] = QString("%1").arg(
                                                                (ui->xHomingDirInvertCheckBox->isChecked()?xFlagMask:noFlagMask) |
                                                                (ui->yHomingDirInvertCheckBox->isChecked()?yFlagMask:noFlagMask) |
                                                                (ui->zHomingDirInvertCheckBox->isChecked()?zFlagMask:noFlagMask) );
    grbl->config[ Grbl::ConfigType::configHomingFeed        ] = ui->homingFeedLineEdit->text();
    grbl->config[ Grbl::ConfigType::configHomingSeek        ] = ui->homingSeekLineEdit->text();
    grbl->config[ Grbl::ConfigType::configHomingDebounce    ] = ui->homingDebounceLineEdit->text();
    grbl->config[ Grbl::ConfigType::configHomingPullOff     ] = ui->homingPullofflineEdit->text();
    grbl->config[ Grbl::ConfigType::configMaxSpindleSpeed   ] = ui->spindleSpeedMaxLineEdit->text();
    grbl->config[ Grbl::ConfigType::configMinSpindleSpeed   ] = ui->spindleSpeedMinLineEdit->text();

    grbl->config[ Grbl::ConfigType::configLaserMode         ] = ui->laserModeCheckBox->isChecked()?1.0:0.0;

    grbl->config[ Grbl::ConfigType::configXSteps            ] = ui->xStepsLineEdit->text();
    grbl->config[ Grbl::ConfigType::configYSteps            ] = ui->yStepsLineEdit->text();
    grbl->config[ Grbl::ConfigType::configZSteps            ] = ui->zStepsLineEdit->text();

    grbl->config[ Grbl::ConfigType::configXMaxRate          ] = ui->xMaxRateLineEdit->text();
    grbl->config[ Grbl::ConfigType::configYMaxRate          ] = ui->yMaxRateLineEdit->text();
    grbl->config[ Grbl::ConfigType::configZMaxRate          ] = ui->zMaxRateLineEdit->text();

    grbl->config[ Grbl::ConfigType::configXAcceleration     ] = ui->xAccelerationLineEdit->text();
    grbl->config[ Grbl::ConfigType::configYAcceleration     ] = ui->yAccelerationLineEdit->text();
    grbl->config[ Grbl::ConfigType::configZAcceleration     ] = ui->zAccelerationLineEdit->text();

    grbl->config[ Grbl::ConfigType::configXMaxTravel        ] = ui->xMaxTravelLineEdit->text();
    grbl->config[ Grbl::ConfigType::configYMaxTravel        ] = ui->yMaxTravelLineEdit->text();
    grbl->config[ Grbl::ConfigType::configZMaxTravel        ] = ui->zMaxTravelLineEdit->text();

    // Include starting blocks in config
    grbl->config[ Grbl::ConfigType::configStartingBlock0    ] = ui->startingBlock0LineEdit->text();
    grbl->config[ Grbl::ConfigType::configStartingBlock1    ] = ui->startingBlock1LineEdit->text();

    return true;
}
