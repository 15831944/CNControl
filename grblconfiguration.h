#ifndef GRBLCONFIGURATION_H
#define GRBLCONFIGURATION_H

#include <QDialog>

namespace Ui {
class GrblConfiguration;
}

class GrblConfiguration : public QDialog
{
    Q_OBJECT

public:
    explicit GrblConfiguration(QWidget *parent = nullptr);
    ~GrblConfiguration();

private:
    Ui::GrblConfiguration *ui;
};

#endif // GRBLCONFIGURATION_H
