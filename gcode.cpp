#include "gcode.h"

#include <QDebug>

GCode::GCode()
{
}

#include <cmath>
double hypot_f(double x, double y) { return double(sqrt(x*x + y*y)); };
#define MAX_INT_DIGITS 8

bool GCode::parse(QString &gcode)
{
    QStringList lines = gcode.split("\n", QString::KeepEmptyParts, Qt::CaseInsensitive);
    return parse(lines);
}

bool GCode::parse(QStringList &gcode)
{
    int unit = UnitType::millimeters;
    int mode = ModeType::absolute;

    this->gcode = gcode;

    // We start at zero for each coordinates
    Point point;
    point.coords = {0, 0, 0};
    point.motion = MotionType::noMove;

    //QVector3D point {0,0,0};
    QVector3D lastPoint = {0,0,0};
    quint64 words = 0;
    quint64 features = 0;
    int motion;
    bool inComment = false;

    QString row;
    QChar letter;

    points.clear();
    //motions.clear();

    // Prévoir une maniere de sortir de la boucle en cas de commande M2 ou M30
    for (int nRow = 0; nRow < gcode.size(); ++nRow)
    {
        row =  gcode.at(nRow).trimmed();
        inComment = false;

        motion = MotionType::noMove;
        words = 0;

        // qDebug() << "Line:" << nRow;

        while (!row.isEmpty())
        {
            letter = row.at(0).toUpper();
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

                float value = valueRow.toFloat();
                int intValue = valueRow.toInt();

                row = row.right( row.size() - valueRow.size() );

                switch (letter.toLatin1())
                {
                case 'G':
                    switch (intValue)
                    {
                    case 0:
                        motion = MotionType::rapidMove;
                        break;
                    case 1:
                        motion = MotionType::feedMove;
                        break;
                    case 2:
                        motion = MotionType::clockwiseArcMove;
                        break;
                    case 3:
                        motion = MotionType::counterClockwiseArcMove;
                        break;
                    case 17:
                        // Plane XY : The only one we handle !!!
                        break;
                    case 20:
                        unit = UnitType::inches;
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
                    case 94:
                        // Mode unités par minute. The only one we handle
                        break;

                    default:
                        qDebug() << "G" << intValue << " command not supported.";
                    }
                    break;
                case 'M':
                    switch (intValue)
                    {
                    case 2:
                    case 30:
                        // End program
                        // Prévoir une manière de sortir...
                        break;
                    default:
                        qDebug() << "M" << intValue << " command not supported.";
                    }
                    break;
                default:
                    if (motion>=0)
                    {
                        switch (letter.toLatin1())
                        {

                        case 'X':
                            if (mode == ModeType::absolute) point.coords.setX(0);
                            point.coords.setX ( point.coords.x() + value );
                            bitSet(words, WordFlags::flagHasX);

                            if (!bitIsSet(features, FeatureFlags::flagHasMinX) || (minPoint.x() > point.coords.x()))
                            {
                                minPoint.setX( point.coords.x() );
                                bitSet(features, FeatureFlags::flagHasMinX);
                            }
                            if (!bitIsSet(features, FeatureFlags::flagHasMaxX) || (maxPoint.x() < point.coords.x()))
                            {
                                maxPoint.setX( point.coords.x() );
                                bitSet(features, FeatureFlags::flagHasMaxX);
                            }
                            break;

                        case 'Y':
                            if (mode == ModeType::absolute) point.coords.setY(0);
                            point.coords.setY( point.coords.y() + value);
                            bitSet(words, WordFlags::flagHasY);

                            if (!bitIsSet(features, FeatureFlags::flagHasMinY) || (minPoint.y() > point.coords.y()))
                            {
                                minPoint.setY( point.coords.y() );
                                bitSet(features, FeatureFlags::flagHasMinY);
                            }
                            if (!bitIsSet(features, FeatureFlags::flagHasMaxY) || (maxPoint.y() < point.coords.y()))
                            {
                                maxPoint.setY( point.coords.y() );
                                bitSet(features, FeatureFlags::flagHasMaxY);
                            }
                            break;

                        case 'Z':
                            if (mode == ModeType::absolute) point.coords.setZ(0);
                            point.coords.setZ( point.coords.z() + value);
                            bitSet(words, WordFlags::flagHasZ);

                            if (!bitIsSet(features, FeatureFlags::flagHasMinZ) || (minPoint.z() > point.coords.z()))
                            {
                                minPoint.setZ( point.coords.z() );
                                bitSet(features, FeatureFlags::flagHasMinZ);
                            }
                            if (!bitIsSet(features, FeatureFlags::flagHasMaxZ) || (maxPoint.z() < point.coords.z()))
                            {
                                maxPoint.setZ( point.coords.z() );
                                bitSet(features, FeatureFlags::flagHasMaxZ);
                            }
                            break;

                        case 'I':
                            // Not sure that mode tell if center is absolute
                            //if (mode == ModeType::absolute)
                                center.setX( value );
                            //else
                            //    center.setX( lastPoint.x() - value );

                            bitSet(words, WordFlags::flagHasI);
                            break;

                        case 'J':
                            // Not sure that mode tell if center is absolute
                            //if (mode == ModeType::absolute)
                                center.setY( value );
                            //else
                            //    center.setY( lastPoint.y() - value );

                            bitSet(words, WordFlags::flagHasJ);
                            break;

                        case 'K':
                            // Not sure that mode tell if center is absolute
                            //if (mode == ModeType::absolute)
                                center.setZ( value );
                            //else
                            //    center.setZ( lastPoint.z() - value );

                            bitSet(words, WordFlags::flagHasK);
                            break;
                        }

                    }
                    else
                        qDebug() << letter.toLatin1() << intValue << " command not supported.";
                } // switch
            } // if
        } // while - End of row parsing

        if (motion>=0)
        {
            double radius;

            switch(motion)
            {
            case MotionType::feedMove:

                if (bitIsClear(words, WordFlags::flagHasX) &&
                    bitIsClear(words, WordFlags::flagHasY))
                    // This is a move in Z only, make it a rapid move
                    motion = MotionType::rapidMove;
                if (point.coords.z() > 0)
                    // When z is above zero, make it a rapidMove
                    motion = MotionType::rapidMove;

                point.motion = motion;
                points.append( point );
                break;

            case MotionType::rapidMove:

                point.motion = motion;
                points.append( point );
                break;

            case MotionType::clockwiseArcMove:
            case MotionType::counterClockwiseArcMove:

//                qDebug() << "center: " << center;

                radius = hypot_f(double(center.x()), double(center.y()));
                mc_arc( point.coords, lastPoint, center, radius, motion, nRow);

                break;
            }

            lastPoint = point.coords;
        } // End of row
    } // End of gCode

    qDebug() << "Min  : " << getBoxMin();
    qDebug() << "Max  : " << getBoxMax();
    qDebug() << "Size : " << getBoxSize();
    //assert( motions.size() == points.size() );
    return true;
};

#include <cmath>

#define N_ARC_CORRECTION 12
#define ARC_ANGULAR_TRAVEL_EPSILON 5E-7
#define ARC_TOLERANCE 0.002

// The next method is inspired from Grbl 1.1h mc_arc function from motion_control.c
/*
  motion_control.c - high level interface for issuing motion commands
  Part of Grbl

  Copyright (c) 2011-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/
void GCode::mc_arc(QVector3D &target, QVector3D &position, QVector3D &offset,
                   double radius, int motion, int nRow)
{
  Point point;
  point.motion = motion;
  point.line = nRow;

  double center_axis0 = double(position.x()) + double(offset.x());
  double center_axis1 = double(position.y()) + double(offset.y());

  double r_axis0 = -double(offset.x());  // Radius vector from center to current location
  double r_axis1 = -double(offset.y());
  double rt_axis0 = double(target.x()) - center_axis0;
  double rt_axis1 = double(target.y()) - center_axis1;

  // CCW angle between position and target from circle center. Only one atan2() trig computation required.
  double angular_travel = atan2( r_axis0 * rt_axis1 -
                                 r_axis1 * rt_axis0,
                                 r_axis0 * rt_axis0 +
                                 r_axis1 * rt_axis1);

  // Correct atan2 output per direction
  if (motion == MotionType::clockwiseArcMove) {
    if (angular_travel >= -ARC_ANGULAR_TRAVEL_EPSILON) { angular_travel -= 2.0*M_PI; }
  } else {
    if (angular_travel <= ARC_ANGULAR_TRAVEL_EPSILON) { angular_travel += 2.0*M_PI; }
  }

  // NOTE: Segment end points are on the arc, which can lead to the arc diameter being smaller by up to
  // (2x) settings.arc_tolerance. For 99% of users, this is just fine. If a different arc segment fit
  // is desired, i.e. least-squares, midpoint on arc, just change the mm_per_arc_segment calculation.
  // For the intended uses of Grbl, this value shouldn't exceed 2000 for the strictest of cases.
  uint segments = uint(floor(fabs(0.5*angular_travel*radius)/
                          sqrt(ARC_TOLERANCE*(2*radius - ARC_TOLERANCE)) ));

  if (segments) {
    // Multiply inverse feed_rate to compensate for the fact that this movement is approximated
    // by a number of discrete segments. The inverse feed_rate should be correct for the sum of
    // all segments.
//    if (pl_data->condition & PL_COND_FLAG_INVERSE_TIME) {
//      pl_data->feed_rate *= segments;
//      bit_false(pl_data->condition,PL_COND_FLAG_INVERSE_TIME); // Force as feed absolute mode over arc segments.
//    }

    double theta_per_segment = angular_travel / segments;
    double linear_per_segment = ( double(target.z()) - double(position.z()) ) / segments;

    /* Vector rotation by transformation matrix: r is the original vector, r_T is the rotated vector,
       and phi is the angle of rotation. Solution approach by Jens Geisler.
           r_T = [cos(phi) -sin(phi);
                  sin(phi)  cos(phi] * r ;

       For arc generation, the center of the circle is the axis of rotation and the radius vector is
       defined from the circle center to the initial position. Each line segment is formed by successive
       vector rotations. Single precision values can accumulate error greater than tool precision in rare
       cases. So, exact arc path correction is implemented. This approach avoids the problem of too many very
       expensive trig operations [sin(),cos(),tan()] which can take 100-200 usec each to compute.

       Small angle approximation may be used to reduce computation overhead further. A third-order approximation
       (second order sin() has too much error) holds for most, if not, all CNC applications. Note that this
       approximation will begin to accumulate a numerical drift error when theta_per_segment is greater than
       ~0.25 rad(14 deg) AND the approximation is successively used without correction several dozen times. This
       scenario is extremely unlikely, since segment lengths and theta_per_segment are automatically generated
       and scaled by the arc tolerance setting. Only a very large arc tolerance setting, unrealistic for CNC
       applications, would cause this numerical drift error. However, it is best to set N_ARC_CORRECTION from a
       low of ~4 to a high of ~20 or so to avoid trig operations while keeping arc generation accurate.

       This approximation also allows mc_arc to immediately insert a line segment into the planner
       without the initial overhead of computing cos() or sin(). By the time the arc needs to be applied
       a correction, the planner should have caught up to the lag caused by the initial mc_arc overhead.
       This is important when there are successive arc motions.
    */
    // Computes: cos_T = 1 - theta_per_segment^2/2, sin_T = theta_per_segment - theta_per_segment^3/6) in ~52usec
    double cos_T = 2.0 - theta_per_segment * theta_per_segment;
    double sin_T = theta_per_segment * 0.16666667 * (cos_T + 4.0);
    cos_T *= 0.5;

    double sin_Ti;
    double cos_Ti;
    double r_axisi;
    uint16_t i;
    uint8_t count = 0;

    for (i = 1; i < segments; i++) { // Increment (segments-1).

      if (count < N_ARC_CORRECTION) {
        // Apply vector rotation matrix. ~40 usec
        r_axisi = r_axis0 * sin_T + r_axis1 * cos_T;
        r_axis0 = r_axis0 * cos_T - r_axis1 * sin_T;
        r_axis1 = r_axisi;
        count++;
      } else {
        // Arc correction to radius vector. Computed only every N_ARC_CORRECTION increments. ~375 usec
        // Compute exact location by applying transformation matrix from initial radius vector(=-offset).
        cos_Ti = cos(i * theta_per_segment);
        sin_Ti = sin(i * theta_per_segment);
        r_axis0 = -double(offset.x()) * cos_Ti + double(offset.y()) * sin_Ti;
        r_axis1 = -double(offset.x()) * sin_Ti - double(offset.y()) * cos_Ti;
        count = 0;
      }

      // Update arc_target location
      point.coords.setX( float(center_axis0 + r_axis0) );
      point.coords.setY( float(center_axis1 + r_axis1) );
      point.coords.setZ( position.z() + float(linear_per_segment));

      // Add point to our list
      points.append(point);
    }
  }

  // Ensure last segment arrives at target location.
  point.coords = target;
  points.append(point);
}
