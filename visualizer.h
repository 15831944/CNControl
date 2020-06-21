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
    void setExecution(int perc);

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

    float  distance = -15.0;

    QVector3D rotation = { -600.0f, 0.0f, -2200.0f };
    QVector3D center={0.0f, 0.0f, -1.0f};
    QVector3D plateSize={ 6, 6, 1 };
    float     plateInterval = 0.5f;

    GCode *gcode;
    int perc;
};

#endif // VISUALIZER_H
