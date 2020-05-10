#include "glineedit.h"

void GLineEdit::focusOutEvent(QFocusEvent *event)
{
    emit focusOut(event);
}

void GLineEdit::focusInEvent(QFocusEvent *event)
{
    emit focusIn(event);
}

