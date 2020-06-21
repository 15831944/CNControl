#include "gcodeparser.h"

#include <QDebug>

GCodeParser::GCodeParser()
{
    qDebug() << "GCodeParser::GCodeParser()";
}

#define MAX_INT_DIGITS 8
bool GCodeParser::parse(QStringList &gcode)
{
    int unit = UnitType::millimeters;
    int mode = ModeType::absolute;
    features = 0;

    // We start at zero for each coordinates
    double x=0, y=0, z=0;
    points.clear();

    for (int nRow = 0; nRow < gcode.size(); ++nRow)
    {
        QString row =  gcode.at(nRow).trimmed();
        bool inComment = false;

        bool motionCommand = false;
        bool jogCommand = false;

        // qDebug() << "Line:" << nRow;

        while (!row.isEmpty())
        {
            QChar letter = row.at(0).toUpper();
            row = row.right( row.size()-1 );

            if (inComment)
            {
                if (letter == ')')
                    inComment=false;
            }
            else if (letter.isSpace())
            {
                // Ignore spaces
            }
            else if (letter == '(')
            {
                inComment = true;
            }
            else if (letter == ';')
            {
                row = "";
            }
            else if ((letter >= 'A') && (letter <= 'Z'))
            {
                QString valueRow;
                valueRow.clear();

                // Get numerical value
                int nChar=0;
                while ( (nChar < row.size()) && (nChar < MAX_INT_DIGITS) &&
                        (row.at(nChar).isDigit() || row.at(nChar) == '-' || row.at(nChar) == '.'))
                    valueRow += row.at(nChar++);

                double value = valueRow.toDouble();
                int intValue = valueRow.toInt();

                row = row.right( row.size() - valueRow.size() );

                switch (letter.toLatin1())
                {
                case 'G':
                    switch (intValue)
                    {
                    case 0:
                        jogCommand = true;
                        break;
                    case 1:
                        motionCommand = true;
                        break;
                    case 20:
                        unit = UnitType::millimeters;
                        break;
                    case 21:
                        unit = UnitType::millimeters;
                        break;
                    case 90:
                        mode = ModeType::absolute;
                        break;
                    case 91:
                        mode = ModeType::incremental;
                        break;

                    default:
                        qDebug() << "G" << intValue << " command not supported.";
                    }
                    break;
                case 'M':
                    switch (intValue)
                    {
                    case 2:
                        // End program
                        break;
                    default:
                        qDebug() << "M" << intValue << " command not supported.";
                    }
                    break;
                default:
                    if (motionCommand || jogCommand)
                    {
                        switch (letter.toLatin1())
                        {
                        case 'X':
                            if (mode == ModeType::absolute) x = 0;
                            x += value;

                            if (!bitIsSet(features, FeatureFlags::flagHasMinX) || (minX > x))
                            {
                                minX = x;
                                bitSet(features, FeatureFlags::flagHasMinX);
                            }
                            if (!bitIsSet(features, FeatureFlags::flagHasMaxX) || (maxX < x))
                            {
                                maxX = x;
                                bitSet(features, FeatureFlags::flagHasMaxX);
                            }
                            break;
                        case 'Y':
                            if (mode == ModeType::absolute) y = 0;
                            y += value;

                            if (!bitIsSet(features, FeatureFlags::flagHasMinY) || (minY > y))
                            {
                                minY = y;
                                bitSet(features, FeatureFlags::flagHasMinY);
                            }
                            if (!bitIsSet(features, FeatureFlags::flagHasMaxY) || (maxY < y))
                            {
                                maxY = y;
                                bitSet(features, FeatureFlags::flagHasMaxY);
                            }
                            break;
                        case 'Z':
                            if (mode == ModeType::absolute) z = 0;
                            z += value;

                            if (!bitIsSet(features, FeatureFlags::flagHasMinZ) || (minZ > z))
                            {
                                minZ = z;
                                bitSet(features, FeatureFlags::flagHasMinZ);
                            }
                            if (!bitIsSet(features, FeatureFlags::flagHasMaxZ) || (maxZ < z))
                            {
                                maxZ = z;
                                bitSet(features, FeatureFlags::flagHasMaxZ);
                            }
                            break;
                        }
                    }
                    else
                        qDebug() << letter.toLatin1() << intValue << " command not supported.";
                }
            }
        } // End of row parsing

        CoordinatesType point = { x, y, z };
        points.append( point );
        //qDebug() << "Point: " << x << "," << y << "," << z;

    } // End of gCode

//    qDebug() << "End of gCode";
    qDebug() << "Min  : " << minX << "," << minY << "," << minZ;
    qDebug() << "Max  : " << maxX << "," << maxY << "," << maxZ;
    qDebug() << "Size : " << (maxX - minX) << "," << (maxY-minY) << "," << (maxZ-minZ);
    return true;
};
