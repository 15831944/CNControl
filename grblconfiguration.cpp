#include "grblconfiguration.h"
#include "ui_grblconfiguration.h"

GrblConfiguration::GrblConfiguration(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GrblConfiguration)
{
    ui->setupUi(this);
}

GrblConfiguration::~GrblConfiguration()
{
    delete ui;
}
