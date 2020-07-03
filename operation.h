#ifndef OPERATION_H
#define OPERATION_H

#include <QDialog>

namespace Ui {
class Operation;
}

class Operation : public QDialog
{
    Q_OBJECT

public:
    explicit Operation(QWidget *parent = nullptr);
    ~Operation();

private:
    Ui::Operation *ui;
};

#endif // OPERATION_H
