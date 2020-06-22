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
            jogMove,
            rapidMove,
            feedMove,
            clockwiseArcMove,
            counterClockwiseArcMove,
            Last
        };
    };

public:
    GCode();

    bool parse(QStringList &gcode);

    QVector3D getMin() { return minPoint; }
    QVector3D getMax() { return maxPoint; }
    QVector3D getSize() { return maxPoint - minPoint; }

    // This method is inspired from Grbl 1.1h mc_arc function from motion_control.c
    void mc_arc(QVector3D &target, QVector3D &position, QVector3D &offset,
                       float radius, int motion);

//    float getMinX() { return minX; }
//    float getMinY() { return minY; }
//    float getMinZ() { return minZ; }

//    float getMaxX() { return maxX; }
//    float getMaxY() { return maxY; }
//    float getMaxZ() { return maxZ; }

//    float getSizeX() { return maxX - minX; }
//    float getSizeY() { return maxY - minY; }
//    float getSizeZ() { return maxZ - minZ; }

    QList<QVector3D> &getPoints()  { return points; }
    QList<int>       &getMotions() { return motions; }
protected:
    quint64 features;

    QVector3D center, minPoint, maxPoint;
    QList<QVector3D> points;
    QList<int>       motions;
};

#endif // GCODE_H
