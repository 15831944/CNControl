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
            jog,
            run,
            move,
            Last
        };
    };

public:
    GCode();

    bool parse(QStringList &gcode);

    float getMinX() { return minX; }
    float getMinY() { return minY; }
    float getMinZ() { return minZ; }

    float getMaxX() { return maxX; }
    float getMaxY() { return maxY; }
    float getMaxZ() { return maxZ; }

    float getSizeX() { return maxX - minX; }
    float getSizeY() { return maxY - minY; }
    float getSizeZ() { return maxZ - minZ; }

    QList<QVector3D> &getPoints()  { return points; }
    QList<int>       &getMotions() { return motions; }
protected:
    quint64 features;
    float minX, maxX;
    float minY, maxY;
    float minZ, maxZ;

    QList<QVector3D> points;
    QList<int>       motions;
};

#endif // GCODE_H
