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
#include <cstdio>
#include <list>
#include <vector>

#include "Vector3D.h"
#ifndef FC_GLOBAL_H
# include <FCGlobal.h>
#endif


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

  inline Vector2d();
  inline Vector2d(float x, float y);
  inline Vector2d(double x, double y);
  inline Vector2d(const Vector2d&) = default;
  inline Vector2d(Vector2d&&) = default;
  ~Vector2d() = default;

  // operators
  inline Vector2d& operator=  (const Vector2d&) = default;
  inline Vector2d& operator=  (Vector2d&&) = default;
  inline bool      operator== (const Vector2d &v) const;
  inline Vector2d  operator+  () const;
  inline Vector2d  operator+  (const Vector2d &v) const;
  inline Vector2d& operator+= (const Vector2d &v);
  inline Vector2d  operator-  () const;
  inline Vector2d  operator-  (const Vector2d &v) const;
  inline Vector2d& operator-= (const Vector2d &v);
  inline Vector2d  operator*  (double c) const;
  inline Vector2d& operator*= (double c);
  inline double    operator*  (const Vector2d &v) const;
  inline Vector2d  operator/  (double c) const;
  inline Vector2d& operator/= (double c);

  // methods
  inline bool IsNull(double tolerance = 0.0) const;
  inline double Length() const;
  inline double Angle() const;
  inline double Sqr() const;

  inline Vector2d& Set(double x, double y);
  inline Vector2d& Negate();
  inline Vector2d& Scale(double factor);
  inline Vector2d& Rotate(double angle);
  inline Vector2d& Normalize();

  inline Vector2d Perpendicular(bool clockwise = false) const;
  static inline Vector2d FromPolar(double r, double fi);

  inline double Distance(const Vector2d &v) const;
  inline bool IsEqual(const Vector2d &v, double tolerance = 0.0) const;

  double GetAngle(const Vector2d &v) const;
  void  ProjectToLine(const Vector2d &point, const Vector2d &line);
};

/** BoundBox2d ********************************************/

/**
 * Two dimensional bounding box.
 */
class BaseExport BoundBox2d
{
public:
  double MinX, MinY, MaxX, MaxY;

  inline BoundBox2d ();
  inline BoundBox2d (const BoundBox2d&) = default;
  inline BoundBox2d (BoundBox2d&&) = default;
  inline BoundBox2d (double fX1, double fY1, double fX2, double fY2);
  ~BoundBox2d() = default;
  inline bool IsValid ();
  inline bool IsEqual(const BoundBox2d&, double tolerance) const;

  // operators
  inline BoundBox2d& operator= (const BoundBox2d&) = default;
  inline BoundBox2d& operator= (BoundBox2d&&) = default;
  inline bool operator== (const BoundBox2d& rclBB) const;

  // methods
  inline double Width() const;
  inline double Height() const;
  inline bool Contains(const Vector2d &v) const;
  inline bool Contains(const Vector2d &v, double tolerance) const;
  inline Vector2d GetCenter() const;

  inline void SetVoid();
  inline void Add(const Vector2d &v);

  bool Intersect(const Line2d &rclLine) const;
  bool Intersect(const BoundBox2d &rclBB) const;
  bool Intersect(const Polygon2d &rclPoly) const;
};

/** Line2d ********************************************/

/**
 * 2D line class.
 */
class BaseExport Line2d
{
public:
  Vector2d clV1, clV2;

  Line2d () = default;
  ~Line2d () = default;
  inline Line2d (const Line2d&) = default;
  inline Line2d (Line2d&&) = default;
  inline Line2d (const Vector2d &rclV1, const Vector2d &rclV2);

  // methods
  inline double Length () const;
  BoundBox2d CalcBoundBox () const;

  // operators
  inline Line2d& operator= (const Line2d&) = default;
  inline Line2d& operator= (Line2d&&) = default;
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
  Polygon2d () = default;
  inline Polygon2d (const Polygon2d&) = default;
  inline Polygon2d (Polygon2d&&) = default;
  ~Polygon2d () = default;

  inline Polygon2d& operator = (const Polygon2d &rclP);
  inline Polygon2d& operator = (Polygon2d &&rclP);

  // admin-interface
  inline size_t GetCtVectors () const;
  inline bool Add (const Vector2d &rclVct);
  inline const Vector2d& operator[] (size_t ulNdx) const;
  inline const Vector2d& At (size_t ulNdx) const;
  inline Vector2d& operator[] (size_t ulNdx);
  inline Vector2d& At (size_t ulNdx);
  inline bool Delete (size_t ulNdx);
  inline void  DeleteAll ();

  // misc
  BoundBox2d CalcBoundBox () const;
  bool Contains (const Vector2d &rclV) const;
  void Intersect (const Polygon2d &rclPolygon, std::list<Polygon2d> &rclResultPolygonList) const;
  bool Intersect (const Polygon2d &rclPolygon) const;
  bool Intersect (const Vector2d &rclV, double eps) const;

private:
  std::vector<Vector2d> _aclVct;
};

/** INLINES ********************************************/

inline Vector2d::Vector2d()
: x(0.0), y(0.0)
{
}

inline Vector2d::Vector2d(float x, float y)
: x(double(x)), y(double(y))
{
}

inline Vector2d::Vector2d(double x, double y)
: x(x), y(y)
{
}

inline bool Vector2d::operator== (const Vector2d &v) const
{
  return (x == v.x) && (y == v.y);
}

inline Vector2d Vector2d::operator+ () const
{
  return {x, y};
}

inline Vector2d Vector2d::operator+ (const Vector2d &v) const
{
  return {x + v.x, y + v.y};
}

inline Vector2d& Vector2d::operator+= (const Vector2d &v)
{
  x += v.x;
  y += v.y;
  return *this;
}

inline Vector2d Vector2d::operator- () const
{
  return {-x, -y};
}

inline Vector2d Vector2d::operator- (const Vector2d &v) const
{
  return {x - v.x, y - v.y};
}

inline Vector2d& Vector2d::operator-= (const Vector2d &v)
{
  x -= v.x;
  y -= v.y;
  return *this;
}

inline Vector2d Vector2d::operator* (double c) const
{
  return {c * x, c * y};
}

inline Vector2d& Vector2d::operator*= (double c)
{
  x *= c;
  y *= c;
  return *this;
}

inline double Vector2d::operator* (const Vector2d &v) const
{
  return x * v.x + y * v.y;
}

inline Vector2d operator* (double c, const Vector2d &v)
{
  return {c * v.x, c * v.y};
}

inline Vector2d Vector2d::operator/ (double c) const
{
  return {x / c, y / c};
}

inline Vector2d& Vector2d::operator/= (double c)
{
  x /= c;
  y /= c;
  return *this;
}

inline bool Vector2d::IsNull(double tolerance) const
{
  return x * x + y * y <= tolerance * tolerance;
}

inline double Vector2d::Length() const
{
  return sqrt(x * x + y * y);
}

inline double Vector2d::Angle() const
{
  return atan2(y, x);
}

inline double Vector2d::Sqr() const
{
  return x * x + y * y;
}

inline Vector2d& Vector2d::Set(double x, double y)
{
  this->x = x;
  this->y = y;
  return *this;
}

inline Vector2d& Vector2d::Negate()
{
  x = -x;
  y = -y;
  return *this;
}

inline Vector2d& Vector2d::Scale(double factor)
{
  x *= factor;
  y *= factor;
  return *this;
}

inline Vector2d& Vector2d::Rotate(double angle)
{
  auto tmp_x = x;
  x = x * cos(angle) - y * sin(angle);
  y = tmp_x * sin(angle) + y * cos(angle);
  return *this;
}

inline Vector2d& Vector2d::Normalize()
{
  double length = Length();
  if (length > 0.0)
  {
    x /= length;
    y /= length;
  }

  return *this;
}

inline Vector2d Vector2d::Perpendicular(bool clockwise) const
{
  return clockwise ? Vector2d(y, -x) : Vector2d(-y, x);
}

inline Vector2d Vector2d::FromPolar(double r, double fi)
{
  return {r * cos(fi), r * sin(fi)};
}

inline double Vector2d::Distance(const Vector2d& v) const
{
  double dx = (x - v.x);
  double dy = (y - v.y);

  return sqrt(dx * dx + dy * dy);
}

inline bool Vector2d::IsEqual(const Vector2d& v, double tolerance) const
{
  return Distance(v) <= tolerance;
}

// ========================================

inline Polygon2d& Polygon2d::operator = (const Polygon2d &rclP) = default;

inline Polygon2d& Polygon2d::operator = (Polygon2d &&rclP) = default;

inline void Polygon2d::DeleteAll ()
{
  _aclVct.clear();
}

inline size_t Polygon2d::GetCtVectors () const
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
    std::vector<Vector2d>::iterator it = _aclVct.begin();
    std::advance(it, ulNdx);
    _aclVct.erase(it);
    return true;
  }

  return false;
}

inline const Vector2d& Polygon2d::operator[] (size_t ulNdx) const
{
  return _aclVct[ulNdx];
}

inline const Vector2d& Polygon2d::At (size_t ulNdx) const
{
  return _aclVct.at(ulNdx);
}

inline Vector2d& Polygon2d::operator[] (size_t ulNdx)
{
  return _aclVct[ulNdx];
}

inline Vector2d& Polygon2d::At (size_t ulNdx)
{
  return _aclVct.at(ulNdx);
}


inline Line2d::Line2d (const Vector2d &rclV1, const Vector2d &rclV2)
    : clV1 (rclV1), clV2 (rclV2)
{
}

inline double Line2d::Length () const
{
  return (clV2 - clV1).Length ();
}

inline bool Line2d::operator== (const Line2d& rclLine) const
{
  return (clV1 == rclLine.clV1) && (clV2 == rclLine.clV2);
}

inline bool Line2d::Contains (const Vector2d &rclV) const
{
  return CalcBoundBox ().Contains (rclV);
}

inline BoundBox2d::BoundBox2d ()
{
  MinX = MinY = DOUBLE_MAX;
  MaxX = MaxY = - DOUBLE_MAX;
}

inline BoundBox2d::BoundBox2d (double fX1, double fY1, double fX2, double fY2)
    : MinX(std::min<double>(fX1, fX2))
    , MinY(std::min<double>(fY1, fY2))
    , MaxX(std::max<double>(fX1, fX2))
    , MaxY(std::max<double>(fY1, fY2))
{
}

inline bool BoundBox2d::IsValid ()
{
  return (MaxX >= MinX) && (MaxY >= MinY);
}

inline bool BoundBox2d::IsEqual(const BoundBox2d& b, double tolerance) const
{
  return Vector2d(MinX, MinY).IsEqual(Vector2d(b.MinX, b.MinY), tolerance) &&
         Vector2d(MaxX, MaxY).IsEqual(Vector2d(b.MaxX, b.MaxY), tolerance);
}

inline bool BoundBox2d::operator== (const BoundBox2d& rclBB) const
{
  return (MinX == rclBB.MinX) &&
         (MinY == rclBB.MinY) &&
         (MaxX == rclBB.MaxX) &&
         (MaxY == rclBB.MaxY);
}

inline double BoundBox2d::Width() const
{
  return MaxX - MinX;
}

inline double BoundBox2d::Height() const
{
  return MaxY - MinY;
}

inline bool BoundBox2d::Contains(const Vector2d &v) const
{
  return v.x >= MinX && v.x <= MaxX
      && v.y >= MinY && v.y <= MaxY;
}

inline bool BoundBox2d::Contains(const Vector2d &v, double tolerance) const
{
  return v.x >= MinX - tolerance && v.x <= MaxX + tolerance
      && v.y >= MinY - tolerance && v.y <= MaxY + tolerance;
}

inline Vector2d BoundBox2d::GetCenter() const
{
  return {(MinX + MaxX) * 0.5, (MinY + MaxY) * 0.5};
}

inline void BoundBox2d::SetVoid()
{
  MinX = MinY = DOUBLE_MAX;
  MaxX = MaxY = -DOUBLE_MAX;
}

inline void BoundBox2d::Add(const Vector2d &v)
{
  MinX = std::min<double>(MinX, v.x);
  MinY = std::min<double>(MinY, v.y);
  MaxX = std::max<double>(MaxX, v.x);
  MaxY = std::max<double>(MaxY, v.y);
}

} // namespace Base

#endif // BASE_TOOLS2D_H
