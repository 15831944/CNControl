#include "visualizer.h"
#include <QMouseEvent>
#include <QMatrix4x4>
#include <QPainter>

Visualizer::Visualizer(QWidget *parent) :
    QOpenGLWidget(parent)
{
    resize(1000, 800);
    gcode = nullptr;
    nbPoints = 0;
    time.start();
}

void Visualizer::initializeGL()
{
    // init OpenGL
    initializeOpenGLFunctions();

    // GL options
    glClearColor(0.98f, 0.98f, 0.98f, 1.0f);
    glEnable(GL_DEPTH_TEST);
//    glEnable(GL_LINE_SMOOTH);

    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable (GL_BLEND);

    glEnable (GL_LINE_SMOOTH);
    glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
}

#include <assert.h>

void Visualizer::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT, GL_LINE);

    // Model view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    eye.setY( distance );

    QMatrix4x4  modelview;
    modelview.lookAt(eye, center, up);
    modelview.rotate(rotation.x() / 16.0f, QVector3D(1.0f, 0.0f, 0.0f));
    modelview.rotate(rotation.y() / 16.0f, QVector3D(0.0f, 1.0f, 0.0f));
    modelview.rotate(rotation.z() / 16.0f, QVector3D(0.0f, 0.0f, 1.0f));
    glLoadMatrixf(modelview.constData());

    // Projection matrix
    glMatrixMode(GL_PROJECTION);
    QMatrix4x4  projection;
    projection.perspective(30.0f, 1.0f*width()/height(), 0.1f, 100.0f);
    glLoadMatrixf(projection.constData());

    paintBoard();
    paintGCode();
    paintStats();
    paintRepere(); // Repere is drown at the end, to stay visible et all time
}

void Visualizer::paintBoard()
{
    glBegin(GL_TRIANGLE_STRIP);
        glColor3f(0.6f, 0.6f, 0.6f);
        glVertex3f( -plateSize.x() / 2.0f, -plateSize.y() / 2.0f, -0.0001f);
        glVertex3f(  plateSize.x() / 2.0f, -plateSize.y() / 2.0f, -0.0001f);
        glVertex3f( -plateSize.x() / 2.0f,  plateSize.y() / 2.0f, -0.0001f);
        glVertex3f(  plateSize.x() / 2.0f,  plateSize.y() / 2.0f, -0.0001f);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
        glColor3f(0.3f, 0.3f, 0.3f);
        glVertex3f( -plateSize.x() / 2.0f, -plateSize.y() / 2.0f, 0.0f);
        glVertex3f( -plateSize.x() / 2.0f,  plateSize.y() / 2.0f, 0.0f);
        glVertex3f(  plateSize.x() / 2.0f, -plateSize.y() / 2.0f, 0.0f);
        glVertex3f(  plateSize.x() / 2.0f,  plateSize.y() / 2.0f, 0.0f);
    glEnd();

    // Draw board lines
    glBegin(GL_LINES);
        glColor3f(0.2f, 0.2f, 0.2f);
        for(float i= -plateSize.x() / 2.0f + plateInterval; i < plateSize.x() / 2.0f; i += plateInterval)
        {
            glVertex3f( -i, -plateSize.y() / 2.0f, 0.00001f);
            glVertex3f( -i,  plateSize.y() / 2.0f, 0.00001f);
        }
        for(float i= -plateSize.y() / 2.0f + plateInterval; i < plateSize.y() / 2.0f; i += plateInterval)
        {
            glVertex3f( -plateSize.x() / 2.0f, -i, 0.00001f);
            glVertex3f(  plateSize.x() / 2.0f, -i, 0.00001f);
        }
    glEnd();
}

void Visualizer::paintRepere()
{
    // Repère X : Red
    glBegin(GL_LINES);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(-1.0f, 0.0f, 0.0f);

        glVertex3f(-1.0f, 0.0f, 0.0f);
        glVertex3f(-0.90f, 0.05f, 0.05f);

        glVertex3f(-1.0f, 0.0f, 0.0f);
        glVertex3f(-0.90f, -0.05f, -0.05f);
    glEnd();

    // Repère Y : Blue
    glBegin(GL_LINES);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, -1.0f, 0.0f);

        glVertex3f(0.0f, -1.0f, 0.0f);
        glVertex3f(-0.05f, -0.90f, -0.05f);

        glVertex3f(0.0f, -1.0f, 0.0f);
        glVertex3f(0.05f, -0.90f, 0.05f);
    glEnd();

    // Repère Z : Green
    glBegin(GL_LINES);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 1.0f);

        glVertex3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.05f, -0.05f, 0.90f);

        glVertex3f(0.0f, 0.0f, 1.0f);
        glVertex3f(-0.05f, 0.05f, 0.90f);
    glEnd();
}

void Visualizer::paintGCode()
{
    if (gcode)
    {
        QVector3D lastPoint = {0,0,0};
        QList<GCode::Point> points = gcode->getPoints();

        int lastMotion = -1;
        float color = 0.9f;

        glBegin(GL_LINES);
            for( int i=0 ; i < nbPoints; i++)
            {
                QVector3D point = points.at(i).coords;
                QVector3D minPoint = gcode->getBoxMin();

                int motion = points.at(i).motion;

                // Convert X and Y from mm to cm, but keep Z bigger for visualization
                point.setX( point.x() / 100.0f );
                point.setY( point.y() / 100.0f );
                point.setZ( (point.z() - minPoint.z()) / 10.0f );

                if (motion != lastMotion)
                {
                    // Change color according to motion
                    switch(motion)
                    {
                    case GCode::MotionType::noMove:
                        glColor3f(0.8f, 0.8f, 0.8f);
                        break;
                    case GCode::MotionType::jogMove:
                        glColor3f(0.0f, color, 0.0f);
                        glLineWidth( 1.0f );
                        break;
                    case GCode::MotionType::feedMove:
                        glColor3f(color, color, 0.0f);
                        glLineWidth( 2.0f );
                        break;
                    case GCode::MotionType::rapidMove:
                        glColor3f(0.0f, 0.0f, color);
                        glLineWidth( 1.0f );
                        break;
                    case GCode::MotionType::clockwiseArcMove:
                        glColor3f(color, color / 2.0f, 0.0f);
                        glLineWidth( 2.0f );
                        break;
                    case GCode::MotionType::counterClockwiseArcMove:
                        glColor3f(color / 2.0f, color, 0.0f);
                        glLineWidth( 2.0f );
                        break;

                    default:
                        qDebug() << "Visualizer::paintGL: Point " << i << " motion is unknown : " << motion;
                    }
                }

                glVertex3f(lastPoint.x(), lastPoint.y(), lastPoint.z());
                glVertex3f(point.x(), point.y(), point.z());

                //qDebug() << "Point " << point.x() << ", " << point.y();
                lastPoint = point;
                lastMotion = motion;
            }
        glEnd();
    }
}

void Visualizer::paintStats()
{
    // FPS count
    ++frame_count;
    const auto elapsed = time.elapsed();
    if (elapsed >= 1000)
    {
        last_count = frame_count;
        frame_count = 0;
        time.restart();
    }

    // FPS display
    glPolygonMode(GL_FRONT, GL_FILL);
    QPainter painter(this);
//    painter.setPen(Qt::black);
//    if (gcode)
//        painter.drawText( QRectF(10.0, 10.0, 300.0, 100.0), QString("Drawn points: %1").arg(nbPoints));
//    painter.drawText(QRectF(10.0, 10.0, 300.0, 100.0), QString("FPS:%1, x=%2, y=%3, z=%4")
//                     .arg(last_count)
//                     .arg(rotation.x())
//                     .arg(rotation.y())
//                     .arg(rotation.z()));

//    if (gcode)
    painter.drawText(QRectF(10.0, 5.0, 300.0, 100.0), QString("distance=%1")
                     .arg(distance)
                     );

//    if (gcode)
//    {
//        painter.drawText(QRectF(10.0, 40.0, 300.0, 100.0), QString("p=%1")
//                     .arg(gcode->getPoints().size()) );
//    }

//    static QImage image(":/logo.png");

//    painter.drawImage(QPoint( this->width() - image.width() - 10 ,10), image);
}

void Visualizer::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    update();
}

void Visualizer::mousePressEvent(QMouseEvent *event)
{
    last_pos = event->pos();
}

void Visualizer::mouseMoveEvent(QMouseEvent *event)
{
    const auto dx = event->x() - last_pos.x();
    const auto dy = event->y() - last_pos.y();
    last_pos = event->pos();

    // Rotate board
    if (event->buttons() == Qt::LeftButton)
    {
        rotation.setZ( rotation.z() + dx * distance / 2.0f);
        rotation.setX( rotation.x() - dy * distance / 2.0f);
//        rotation.setZ( rotation.z() + dx * 10.0f);
//        rotation.setX( rotation.x() - dy * 10.0f);
    }
    // Change board position
    else if (event->buttons() == Qt::RightButton)
    {
        center += QVector3D( dx * distance / 1150.0f, 0, dy * distance / 1150.0f );
        eye += QVector3D( dx * distance / 1150.0f, 0, dy * distance / 1150.0f );
    }
    // Change zoom
    else if (event->buttons() == Qt::MidButton)
    {
        distance *= 1.0f - (1.0f * dy / 120.0f);
        if (distance < 0.2f) distance = 0.2f;
        if (distance > 50.0f) distance = 50.0f;
    }

    update();
}

void Visualizer::wheelEvent(QWheelEvent *event)
{
    distance *= 1.0f - (1.0f * event->delta() / 1200.0f);
    if (distance < 0.2f) distance = 0.2f;
    if (distance > 50.0f) distance = 50.0f;

    update();
}

void Visualizer::setGCode(GCode *gcode)
{
    this->gcode = gcode;
    // Here we must update the nbPoints var from the slider, but how ???
    nbPoints = gcode->getPoints().size();
    update();
}

void Visualizer::setNbPoints(int nbPoints)
{
    this->nbPoints = nbPoints;
    update();
}
