#ifndef GRBLCONFIGURATION_H
#define GRBLCONFIGURATION_H

#include <QDialog>
#include "grbl.h"

namespace Ui {
class GrblConfiguration;
}

class GrblConfiguration : public QDialog
{
    Q_OBJECT

public:
    explicit GrblConfiguration(QWidget *parent = nullptr);
    ~GrblConfiguration();

    void setConfiguration(Grbl *grbl);
    bool getConfiguration(Grbl *grbl);

private:
    Ui::GrblConfiguration *ui;
};

#endif // GRBLCONFIGURATION_H
