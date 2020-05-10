#ifndef GLINEEDIT_H
#define GLINEEDIT_H

#include <QLineEdit>
#include <QFocusEvent>

// Replace QLineEdit with two signals focusIn and focusOut

class GLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    GLineEdit(const QString &contents, QWidget *parent = nullptr) : QLineEdit(contents, parent) {}
    GLineEdit(QWidget *parent = nullptr) : QLineEdit(parent) {}

    virtual void focusOutEvent(QFocusEvent *event);
    virtual void focusInEvent(QFocusEvent *event);
public slots:

signals:
    void focusOut(QFocusEvent *event);
    void focusIn(QFocusEvent *event);
};

#endif // GLINEEDIT_H
