/***************************************************************************
 *   Copyright (c) 2005 Imetric 3D GmbH                                    *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef BASE_TOOLS2D_H
#define BASE_TOOLS2D_H


#include <algorithm>
#include <cmath>
#include <cfloat>
#include <stdio.h>
#include <list>
#include <vector>

#include "Vector3D.h"

namespace Base {

class Vector2d;
class BoundBox2d;
class Line2d;
class Polygon2d;

/**
 * The vector class for 2D calculations.
 */
class BaseExport Vector2d
{
public:
  double x, y;

  inline Vector2d (void);
  inline Vector2d (float x, float y);
  inline Vector2d (double x, double y);
  inline Vector2d (const Vector2d &rclVct);

  // methods
  inline double Length (void) const;
  inline double Distance(const Vector2d&) const;
  inline bool IsEqual(const Vector2d&, double tolerance) const;

  // operators
  inline Vector2d& operator= (const Vector2d &rclVct);
  inline double    operator* (const Vector2d &rclVct) const;
  inline bool      operator== (const Vector2d &rclVct) const;
  inline Vector2d  operator+ (const Vector2d &rclVct) const;
  inline Vector2d  operator- (const Vector2d &rclVct) const;
  inline Vector2d  operator/ (double c) const;

  inline void Set (double fPX, double fPY);
  inline void Scale (double fS);
  inline void Normalize (void);
  double GetAngle (const Vector2d &rclVect) const;
  void  ProjectToLine (const Vector2d &rclPt, const Vector2d &rclLine);
};

/** BoundBox2d ********************************************/

/**
 * Two dimensional bounding box.
 */
class BaseExport BoundBox2d
{
public:
  double MinX, MinY, MaxX, MaxY;
  
  inline BoundBox2d (void);
  inline BoundBox2d (const BoundBox2d &rclBB);
  inline BoundBox2d (double fX1, double fY1, double fX2, double fY2);
  inline bool IsValid (void);
  inline bool IsEqual(const BoundBox2d&, double tolerance) const;
  
  // operators
  inline BoundBox2d& operator= (const BoundBox2d& rclBB);
  inline bool operator== (const BoundBox2d& rclBB) const;
  bool Intersect(const Line2d &rclLine) const;
  bool Intersect(const BoundBox2d &rclBB) const;
  bool Intersect(const Polygon2d &rclPoly) const;
  inline void Add(const Vector2d &rclVct);

  void SetVoid (void) { MinX = MinY = DOUBLE_MAX; MaxX = MaxY = -DOUBLE_MAX; }

  // misc
  bool Contains (const Vector2d &rclV) const;
};

/** Line2d ********************************************/

/**
 * 2D line class.
 */
class BaseExport Line2d
{
public:
  Vector2d clV1, clV2;

  Line2d (void) {}
  inline Line2d (const Line2d &rclLine);
  inline Line2d (const Vector2d &rclV1, const Vector2d &rclV2);

  // methods
  inline double Length (void) const;
  BoundBox2d CalcBoundBox (void) const;

  // operators
  inline Line2d& operator= (const Line2d& rclLine);
  inline bool operator== (const Line2d& rclLine) const;

  // misc
  inline bool Contains (const Vector2d &rclV) const;
  bool Intersect (const Line2d& rclLine, Vector2d &rclV) const;
  bool Intersect (const Vector2d &rclV, double eps) const;
  bool IntersectAndContain (const Line2d& rclLine, Vector2d &rclV) const;
  Vector2d FromPos (double fDistance) const;
};

/** Polygon2d ********************************************/

/**
 * 2D polygon class.
 */
class BaseExport Polygon2d
{
public:
  Polygon2d (void) {}
  inline Polygon2d (const Polygon2d &rclPoly);
  virtual ~Polygon2d () {}

  inline Polygon2d& operator = (const Polygon2d &rclP);

  // admin-interface
  inline size_t GetCtVectors (void) const;
  inline bool Add (const Vector2d &rclVct);
  inline Vector2d& operator[] (size_t ulNdx) const;
  inline Vector2d& At (size_t ulNdx) const;
  inline bool Delete (size_t ulNdx);
  inline void  DeleteAll (void);    

  // misc
  BoundBox2d CalcBoundBox (void) const;
  bool Contains (const Vector2d &rclV) const;
  void Intersect (const Polygon2d &rclPolygon, std::list<Polygon2d> &rclResultPolygonList) const;
  bool Intersect (const Vector2d &rclV, double eps) const;

private:
  std::vector<Vector2d> _aclVct;
};

/** INLINES ********************************************/

inline Vector2d::Vector2d (void)
: x(0.0), y(0.0)
{
}

inline Vector2d::Vector2d (float x, float y)
: x(x), y(y)
{
}

inline Vector2d::Vector2d (double x, double y)
: x(x), y(y)
{
}

inline Vector2d::Vector2d (const Vector2d &rclVct)
: x(rclVct.x), y(rclVct.y)
{
}

inline double Vector2d::Length (void) const
{
  return sqrt ((x * x) + (y * y));
}

inline double Vector2d::Distance(const Vector2d& v) const
{
  double dx = (x - v.x);
  double dy = (y - v.y);
  return sqrt(dx*dx + dy*dy);
}

inline bool Vector2d::IsEqual(const Vector2d& v, double tolerance) const
{
  return Distance (v) <= tolerance;
}

inline Vector2d& Vector2d::operator= (const Vector2d &rclVct)
{
  x = rclVct.x;
  y = rclVct.y;
  return *this;
}

inline bool Vector2d::operator== (const Vector2d &rclVct) const
{
  return (x == rclVct.x) && (y == rclVct.y);
}

inline Vector2d Vector2d::operator+ (const Vector2d &rclVct) const
{
  return Vector2d(x + rclVct.x, y + rclVct.y);
}

inline Vector2d Vector2d::operator- (const Vector2d &rclVct) const
{
  return Vector2d(x - rclVct.x, y - rclVct.y);
}

inline double Vector2d::operator* (const Vector2d &rclVct) const
{
  return (x * rclVct.x) + (y * rclVct.y);
}

inline Vector2d Vector2d::operator/ (double c) const
{
  return Vector2d(x / c, y / c);
}

inline void Vector2d::Scale (double fS)
{
  x *= fS;
  y *= fS;
}

inline void Vector2d::Normalize (void)
{
  double fLen = Length();
  if (fLen != 0.0f)
  {
    x /= fLen;
    y /= fLen;
  }
}

inline void Vector2d::Set (double fPX, double fPY)
{
  x = fPX;
  y = fPY;
}

inline Polygon2d::Polygon2d (const Polygon2d &rclPoly)
{
  *this = rclPoly;
}

inline Polygon2d& Polygon2d::operator = (const Polygon2d &rclP)
{
  _aclVct = rclP._aclVct;
  return *this;
}

inline void Polygon2d::DeleteAll (void)
{
  _aclVct.clear();
}

inline size_t Polygon2d::GetCtVectors (void) const
{
  return _aclVct.size ();
}

inline bool Polygon2d::Add (const Vector2d &rclVct)
{
  _aclVct.push_back (rclVct);
  return true;
}

inline bool Polygon2d::Delete (size_t ulNdx)
{
  if ( ulNdx < _aclVct.size() )
  {
    std::vector<Vector2d>::iterator it = _aclVct.begin() + ulNdx;
    _aclVct.erase ( it );
    return true;
  }

  return false;
}

inline Vector2d& Polygon2d::operator[] (size_t ulNdx) const
{
  return (Vector2d&) _aclVct[ulNdx];
}

inline Vector2d& Polygon2d::At (size_t ulNdx) const
{
  return (Vector2d&) _aclVct[ulNdx];
}


inline Line2d::Line2d (const Line2d &rclLine)
    : clV1 (rclLine.clV1),
      clV2 (rclLine.clV2)
{
}

inline Line2d::Line2d (const Vector2d &rclV1, const Vector2d &rclV2)
    : clV1 (rclV1), clV2 (rclV2)
{
}

inline double Line2d::Length (void) const
{
  return (clV2 - clV1).Length ();
}

inline Line2d& Line2d::operator= (const Line2d& rclLine)
{
  clV1 = rclLine.clV1;
  clV2 = rclLine.clV2;
  return *this;
}

inline bool Line2d::operator== (const Line2d& rclLine) const
{
  return (clV1 == rclLine.clV1) && (clV2 == rclLine.clV2);
}

inline bool Line2d::Contains (const Vector2d &rclV) const
{
  return CalcBoundBox ().Contains (rclV);
}

inline BoundBox2d::BoundBox2d (void)
{
  MinX = MinY = DOUBLE_MAX;
  MaxX = MaxY = - DOUBLE_MAX;
}

inline BoundBox2d::BoundBox2d (const BoundBox2d &rclBB)
    : MinX (rclBB.MinX),
      MinY (rclBB.MinY),
      MaxX (rclBB.MaxX),
      MaxY (rclBB.MaxY)
{
}

inline BoundBox2d::BoundBox2d (double fX1, double fY1, double fX2, double fY2)
{
    MinX = std::min<double>( fX1, fX2 );
    MaxX = std::max<double>( fX1, fX2 );
    MinY = std::min<double>( fY1, fY2 );
    MaxY = std::max<double>( fY1, fY2 );
}

inline bool BoundBox2d::IsValid (void)
{
  return (MaxX >= MinX) && (MaxY >= MinY);
}

inline bool BoundBox2d::IsEqual(const BoundBox2d& b, double tolerance) const
{
  return Vector2d(MinX,MinY).IsEqual(Vector2d(b.MinX,b.MinY), tolerance) &&
         Vector2d(MaxX,MaxY).IsEqual(Vector2d(b.MaxX,b.MaxY), tolerance);
}

inline BoundBox2d& BoundBox2d::operator= (const BoundBox2d& rclBB)
{
  MinX = rclBB.MinX;
  MinY = rclBB.MinY;
  MaxX = rclBB.MaxX;
  MaxY = rclBB.MaxY;
  return *this;
}

inline bool BoundBox2d::operator== (const BoundBox2d& rclBB) const
{
  return (MinX == rclBB.MinX) &&
         (MinY == rclBB.MinY) &&
         (MaxX == rclBB.MaxX) &&
         (MaxY == rclBB.MaxY);
}

inline void BoundBox2d::Add(const Vector2d &rclVct)
{
  MinX = std::min<double>(MinX, rclVct.x);
  MinY = std::min<double>(MinY, rclVct.y);
  MaxX = std::max<double>(MaxX, rclVct.x);
  MaxY = std::max<double>(MaxY, rclVct.y);
}

} // namespace Base

#endif // BASE_TOOLS2D_H


