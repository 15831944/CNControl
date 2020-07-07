#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QDialog>

namespace Ui {
class Configuration;
}

class Configuration : public QDialog
{
    Q_OBJECT

public:
    explicit Configuration(QWidget *parent = nullptr);
    ~Configuration();

private:
    Ui::Configuration *ui;
};

#endif // CONFIGURATION_H
