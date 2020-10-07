#ifndef GCODE_H
#define GCODE_H

#include <QStringList>
#include <QVector3D>
#include "bits.h"

class GCode
{
    class FeatureFlags
    {
    public:
        enum {
            flagHasMinX,
            flagHasMaxX,
            flagHasMinY,
            flagHasMaxY,
            flagHasMinZ,
            flagHasMaxZ,
            Last
        };
    };

    class WordFlags
    {
    public:
        enum {
            flagHasX,
            flagHasY,
            flagHasZ,
            flagHasI,
            flagHasJ,
            flagHasK,
            Last
        };
    };

    class ModeType
    {
    public:
        enum {
            incremental,
            absolute,
            Last
        };
    };

    class UnitType
    {
    public:
        enum {
            inches,
            millimeters,
            Last
        };
    };
public:
    class MotionType
    {
    public:
        enum {
            noMove,
            jogMove,
            rapidMove,
            feedMove,
            clockwiseArcMove,
            counterClockwiseArcMove,
            Last
        };
    };

    struct Point
    {
        QVector3D coords;
        int motion;
        int line;
    };

public:
    GCode();

    bool parse(QString &gcode);
    bool parse(QStringList &gcode);

    QVector3D getBoxMin() { return minPoint; }
    QVector3D getBoxMax() { return maxPoint; }
    QVector3D getBoxSize() { return maxPoint - minPoint; }

    QStringList getLines() { return gcode; }
    int getSize() {return gcode.size(); }

    // This method is inspired from Grbl 1.1h mc_arc function from motion_control.c
    // Many thanks to the Grbl team.
    void mc_arc(QVector3D &target, QVector3D &position, QVector3D &offset,
                       double radius, int motion, int nRow);

    QList<Point> &getPoints()  { return points; }
protected:
    QVector3D center, minPoint, maxPoint;

    QStringList gcode;
    QList<Point> points;
};

#endif // GCODE_H
