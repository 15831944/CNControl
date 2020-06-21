#ifndef GCODEPARSER_H
#define GCODEPARSER_H

#include <QStringList>
#include "bits.h"

class GCodeParser
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

    class TypeType
    {
    public:
        enum {
            run,
            jog,
            Last
        };
    };

    typedef struct CoordinatesType
    {
        double x;
        double y;
        double z;

        int type;

        bool operator==(CoordinatesType coords)
        {
            return (coords.x == x) && (coords.y == y) && (coords.z == z);
        }
        bool operator!=(CoordinatesType coords)
        {
            return !(*this == coords);
        }
    } CoordinatesType;

public:
    GCodeParser();

    bool parse(QStringList &gcode);

    double getMinX() { return minX; }
    double getMinY() { return minY; }
    double getMinZ() { return minZ; }

    double getMaxX() { return maxX; }
    double getMaxY() { return maxY; }
    double getMaxZ() { return maxZ; }

protected:
    quint64 features;
    double minX, maxX;
    double minY, maxY;
    double minZ, maxZ;

    QList<CoordinatesType> points;

};

#endif // GCODEPARSER_H
