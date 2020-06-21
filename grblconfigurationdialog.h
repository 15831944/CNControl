#ifndef GRBLCONFIGURATIONDIALOG_H
#define GRBLCONFIGURATIONDIALOG_H

#include <QDialog>
#include "grbl.h"

namespace Ui {
class GrblConfigurationDialog;
}

class GrblConfigurationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GrblConfigurationDialog(QWidget *parent = nullptr);
    ~GrblConfigurationDialog();

    void setConfiguration(Grbl *grbl);
    bool getConfiguration(Grbl *grbl);

private:
    Ui::GrblConfigurationDialog *ui;
};

#endif // GRBLCONFIGURATION_H
