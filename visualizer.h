#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_2_0>
#include <QTime>
#include <QVector>
#include <QVector3D>
#include <gcode.h>

class Visualizer : public QOpenGLWidget, protected QOpenGLFunctions_2_0
{
public:
    Visualizer(QWidget *parent);

    void setGCode(GCode *gcode);
    void setNbPoints(int nbPoints);

    void setRotation(QVector3D rot) { rotation = rot; update(); }
    void setPosition(QVector3D pos) { center = pos; update(); }

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    void paintBoard();
    void paintRepere();
    void paintGCode();
    void paintStats();

    // Events
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:

    QTime time;
    size_t frame_count{};
    size_t last_count{};

    QPoint last_pos;

    float  distance = 15.0;

    QVector3D eye       = {0.0f,  -distance, 0.0f};
    QVector3D up        = {0.0f, 0.0f, 1.0f};
    QVector3D rotation  = { -1440.0f , 0.0f, -2880.0f };
    QVector3D center    = {0.0f, 0.0f, 0.0f};
    QVector3D plateSize = { 6, 6, 1 };
    float     plateInterval = 0.5f;

    GCode *gcode;
    int nbPoints;
};

#endif // VISUALIZER_H
