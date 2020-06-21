#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <QtWidgets/QOpenGLWidget>
#include <QVector3D>

class Visualizer : QOpenGLWidget
{
public:
    Visualizer();

    void setPoints(QList<QVector3D> *list);
};

#endif // VISUALIZER_H
