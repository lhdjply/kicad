/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef TRIGO_H
#define TRIGO_H

#include <cmath>
#include <math/vector2d.h>
#include <geometry/eda_angle.h>

/**
 * Test if \a aTestPoint is on line defined by \a aSegStart and \a aSegEnd.
 *
 * This function is faster than #TestSegmentHit() because \a aTestPoint  should be exactly on
 * the line.  This only works for horizontal, vertical, and 45 degree line segments.
 *
 * @param aSegStart The first point of the line segment.
 * @param aSegEnd The second point of the line segment.
 * @param aTestPoint The point to test.
 *
 * @return true if the point is on the line segment.
 */
bool IsPointOnSegment( const VECTOR2I& aSegStart, const VECTOR2I& aSegEnd,
                       const VECTOR2I& aTestPoint );

/**
 * Test if two lines intersect.
 *
 * @param a_p1_l1 The first point of the first line.
 * @param a_p2_l1 The second point of the first line.
 * @param a_p1_l2 The first point of the second line.
 * @param a_p2_l2 The second point of the second line.
 * @param aIntersectionPoint is filled with the intersection point if it exists
 * @return bool true if the two segments defined by four points intersect.
 * (i.e. if the 2 segments have at least a common point)
 */
bool SegmentIntersectsSegment( const VECTOR2I& a_p1_l1, const VECTOR2I& a_p2_l1,
                               const VECTOR2I& a_p1_l2, const VECTOR2I& a_p2_l2,
                               VECTOR2I* aIntersectionPoint = nullptr );

/**
 * Calculate the new point of coord coord pX, pY, for a rotation center 0, 0.
 */
void RotatePoint( int *pX, int *pY, const EDA_ANGLE& aAngle );

inline void RotatePoint( VECTOR2I& point, const EDA_ANGLE& aAngle )
{
    RotatePoint( &point.x, &point.y, aAngle );
}


/**
 * Calculate the new point of coord coord pX, pY, for a rotation center cx, cy.
 */
void RotatePoint( int *pX, int *pY, int cx, int cy, const EDA_ANGLE& aAngle );

inline void RotatePoint( VECTOR2I& point, const VECTOR2I& centre, const EDA_ANGLE& aAngle )
{
    RotatePoint( &point.x, &point.y, centre.x, centre.y, aAngle );
}


/**
 * Calculate the new coord point point for a rotation center 0, 0.
 */
void RotatePoint( double* pX, double* pY, const EDA_ANGLE& aAngle );

inline void RotatePoint( VECTOR2D& point, const EDA_ANGLE& aAngle )
{
    RotatePoint( &point.x, &point.y, aAngle );
}

void RotatePoint( double* pX, double* pY, double cx, double cy, const EDA_ANGLE& aAngle );

inline void RotatePoint( VECTOR2D& point, const VECTOR2D& aCenter, const EDA_ANGLE& aAngle )
{
    RotatePoint( &point.x, &point.y, aCenter.x, aCenter.y, aAngle );
}

/**
 * Determine the center of an arc or circle given three points on its circumference.
 *
 * @param aStart The starting point of the circle (equivalent to aEnd).
 * @param aMid The point on the arc, half-way between aStart and aEnd.
 * @param aEnd The ending point of the circle (equivalent to aStart).
 * @return The center of the circle.
 */
const VECTOR2I CalcArcCenter( const VECTOR2I& aStart, const VECTOR2I& aMid, const VECTOR2I& aEnd );
const VECTOR2D CalcArcCenter( const VECTOR2D& aStart, const VECTOR2D& aMid, const VECTOR2D& aEnd );
const VECTOR2D CalcArcCenter( const VECTOR2D& aStart, const VECTOR2D& aEnd,
                              const EDA_ANGLE& aAngle );

/**
 * Return the middle point of an arc, half-way between aStart and aEnd.
 *
 * There are two possible solutions which can be found by toggling aMinArcAngle. The behavior
 * is undefined for semicircles (i.e. 180 degree arcs).
 *
 * @param aStart The starting point of the arc (for calculating the radius).
 * @param aEnd The end point of the arc (for determining the arc angle).
 * @param aCenter The center point of the arc.
 * @param aMinArcAngle If true, returns the point that results in the smallest arc angle.
 * @return The middle point of the arc.
*/
const VECTOR2I CalcArcMid( const VECTOR2I& aStart, const VECTOR2I& aEnd, const VECTOR2I& aCenter,
                           bool aMinArcAngle = true );

inline double EuclideanNorm( const VECTOR2I& vector )
{
    // this is working with doubles
    return hypot( vector.x, vector.y );
}

/**
 * Compute the distance between a line and a reference point.
 *
 * Reference: http://mathworld.wolfram.com/Point-LineDistance2-Dimensional.html
 *
 * @param linePointA Point on line.
 * @param linePointB Point on line.
 * @param referencePoint Reference point.
 */
inline double DistanceLinePoint( const VECTOR2I& linePointA, const VECTOR2I& linePointB,
                                 const VECTOR2I& referencePoint )
{
    // Some of the multiple double casts are redundant. However in the previous definition
    // the cast was (implicitly) done too late, just before the division (EuclideanNorm gives
    // a double so from int it would be promoted); that means that the whole expression were
    // vulnerable to overflow during int multiplications
    return fabs( ( static_cast<double>( linePointB.x - linePointA.x ) *
                   static_cast<double>( linePointA.y - referencePoint.y ) -
                   static_cast<double>( linePointA.x  - referencePoint.x ) *
                   static_cast<double>( linePointB.y - linePointA.y) )
            / EuclideanNorm( linePointB - linePointA ) );
}

/**
 * Test if two points are near each other.
 *
 * @param pointA First point.
 * @param pointB Second point.
 * @param threshold The maximum distance.
 * @return true if \a pointA is within \a threshold of \a pointB otherwise false.
 */
inline bool HitTestPoints( const VECTOR2I& pointA, const VECTOR2I& pointB, double threshold )
{
    VECTOR2I vectorAB = pointB - pointA;

    // Compare the distances squared. The double is needed to avoid overflow during int
    // multiplication
    double sqdistance = (double)vectorAB.x * vectorAB.x + (double)vectorAB.y * vectorAB.y;

    return sqdistance < threshold * threshold;
}

/**
 * Test if \a aRefPoint is with \a aDistance on the line defined by \a aStart and \a aEnd..
 *
 * @param aRefPoint = reference point to test
 * @param aStart is the first end-point of the line segment
 * @param aEnd is the second end-point of the line segment
 * @param aDist = maximum distance for hit
*/
bool TestSegmentHit( const VECTOR2I& aRefPoint, const VECTOR2I& aStart, const VECTOR2I& aEnd,
                     int aDist );

/**
 * Return the length of a line segment defined by \a aPointA and \a aPointB.
 *
 * See also EuclideanNorm and Distance for the single vector or four scalar versions.
 *
 * @return Length of a line (as double)
 */
inline double GetLineLength( const VECTOR2I& aPointA, const VECTOR2I& aPointB )
{
    return hypot( (double) aPointA.x - aPointB.x, (double) aPointA.y - aPointB.y );
}

// These are the usual degrees <-> radians conversion routines
inline double DEG2RAD( double deg ) { return deg * M_PI / 180.0; }
inline double RAD2DEG( double rad ) { return rad * 180.0 / M_PI; }

// These are the same *but* work with the internal 'decidegrees' unit
inline double RAD2DECIDEG( double rad ) { return rad * 1800.0 / M_PI; }

/* These are templated over T (and not simply double) because Eeschema
   is still using int for angles in some place */

/**
 * Normalize angle to be in the 0.0 .. 360.0 range: angle is in 1/10 degrees.
 */
template <class T> inline T NormalizeAnglePos( T Angle )
{
    while( Angle < 0 )
        Angle += 3600;
    while( Angle >= 3600 )
        Angle -= 3600;
    return Angle;
}

template <class T> inline void NORMALIZE_ANGLE_POS( T& Angle )
{
    Angle = NormalizeAnglePos( Angle );
}


/**
 * Normalize angle to be in the -180.0 .. 180.0 range.
 */
template <class T> inline T NormalizeAngle180( T Angle )
{
    while( Angle <= -1800 )
        Angle += 3600;

    while( Angle > 1800 )
        Angle -= 3600;

    return Angle;
}

/**
 * Test if an arc from \a aStartAngle to \a aEndAngle crosses the positive X axis (0 degrees).
 *
 * Testing is performed in the quadrant 1 to quadrant 4 direction (counter-clockwise).
 *
 * @param aStartAngle The arc start angle in degrees.
 * @param aEndAngle The arc end angle in degrees.
 */
inline bool InterceptsPositiveX( double aStartAngle, double aEndAngle )
{
    double end = aEndAngle;

    if( aStartAngle > aEndAngle )
        end += 360.0;

    return aStartAngle < 360.0 && end > 360.0;
}

/**
 * Test if an arc from \a aStartAngle to \a aEndAngle crosses the negative X axis (180 degrees).
 *
 * Testing is performed in the quadrant 1 to quadrant 4 direction (counter-clockwise).
 *
 * @param aStartAngle The arc start angle in degrees.
 * @param aEndAngle The arc end angle in degrees.
 */
inline bool InterceptsNegativeX( double aStartAngle, double aEndAngle )
{
    double end = aEndAngle;

    if( aStartAngle > aEndAngle )
        end += 360.0;

    return aStartAngle < 180.0 && end > 180.0;
}

/**
 * Calculate the area of a parallelogram defined by \a aPointA, \a aPointB, and \a aPointC.
 *                     B ______________________
 *                      /                      /
 *                     /                      /
 *                    /______________________/
 *                   A                       C
 * The area of a parallelogram is the cross product of the vectors A->B and A->C.
 * The order of the vertices is not important, the result will be the same (modulo sign).
 */
template <class T>
inline typename VECTOR2<T>::extended_type
ParallelogramArea( const VECTOR2<T>& aPointA, const VECTOR2<T>& aPointB, const VECTOR2<T>& aPointC )
{
    VECTOR2<T> v1 = aPointB - aPointA;
    VECTOR2<T> v2 = aPointC - aPointA;
    return v1.Cross( v2 );
}

/**
 * Test if a point hits a line segment within a given distance.  This is a faster version of
 * TestSegmentHit() that does not calculate the distance.
*/
bool TestSegmentHitFast( const VECTOR2I& aRefPoint, const VECTOR2I& aStart, const VECTOR2I& aEnd,
                         int aDist );

#endif
