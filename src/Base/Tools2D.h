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


#include <math.h>
#include <stdio.h>
#include <list>
#include <vector>

#include "Vector3D.h"

namespace Base {

class Vector2D;
class BoundBox2D;
class Line2D;
class Polygon2D;

/**
 * The vector class for 2D calculations.
 */
class BaseExport Vector2D
{
public:
  double fX, fY;

  inline Vector2D (void);
  inline Vector2D (float x, float y);
  inline Vector2D (double x, double y);
  inline Vector2D (const Vector2D &rclVct);

  // methods
  inline double Length (void) const;

  // operators
  inline Vector2D& operator= (const Vector2D &rclVct);
  inline double    operator* (const Vector2D &rclVct) const;
  inline bool      operator== (const Vector2D &rclVct) const;
  inline Vector2D  operator+ (const Vector2D &rclVct) const;
  inline Vector2D  operator- (const Vector2D &rclVct) const;

  inline void Set (double fPX, double fPY);
  inline void Scale (double fS);
  inline void Normalize (void);
  double GetAngle (const Vector2D &rclVect) const;
  void  ProjToLine (const Vector2D &rclPt, const Vector2D &rclLine);
};

/** BoundBox2D ********************************************/

/**
 * Two dimensional bounding box.
 */
class BaseExport BoundBox2D
{
public:
  double fMinX, fMinY, fMaxX, fMaxY;
  
  inline BoundBox2D (void);
  inline BoundBox2D (const BoundBox2D &rclBB);
  inline BoundBox2D (double fX1, double fY1, double fX2, double fY2);
  inline bool IsValid (void);
  
  // operators
  inline BoundBox2D& operator= (const BoundBox2D& rclBB);
  inline bool operator== (const BoundBox2D& rclBB) const;
  bool operator|| (const Line2D &rclLine) const;
  bool operator|| (const BoundBox2D &rclBB) const;
  bool operator|| (const Polygon2D &rclPoly) const;
  inline void operator &= (const Vector2D &rclVct);

  void SetVoid (void) { fMinX = fMinY = DOUBLE_MAX; fMaxX = fMaxY = -DOUBLE_MAX; }

  // misc
  bool Contains (const Vector2D &rclV) const;
};

/** Line2D ********************************************/

/**
 * 2D line class.
 */
class BaseExport Line2D
{
public:
  Vector2D clV1, clV2;

  Line2D (void) {}
  inline Line2D (const Line2D &rclLine);
  inline Line2D (const Vector2D &rclV1, const Vector2D &rclV2);

  // methods
  inline double Length (void) const;
  BoundBox2D CalcBoundBox (void) const;

  // operators
  inline Line2D& operator= (const Line2D& rclLine);
  inline bool operator== (const Line2D& rclLine) const;

  // misc
  inline bool Contains (const Vector2D &rclV) const;
  bool Intersect (const Line2D& rclLine, Vector2D &rclV) const;
  bool IntersectAndContain (const Line2D& rclLine, Vector2D &rclV) const;
  Vector2D FromPos (double fDistance) const;
};

/** Polygon2D ********************************************/

/**
 * 2D polygon class.
 */
class BaseExport Polygon2D
{
public:
  Polygon2D (void) {}
  inline Polygon2D (const Polygon2D &rclPoly);
  virtual ~Polygon2D () {}  

  inline Polygon2D& operator = (const Polygon2D &rclP);

  // admin-interface
  inline size_t GetCtVectors (void) const;
  inline bool Add (const Vector2D &rclVct);
  inline Vector2D& operator[] (size_t ulNdx) const;
  inline Vector2D& At (size_t ulNdx) const;
  inline bool Delete (size_t ulNdx);
  inline void  DeleteAll (void);    

  // misc
  BoundBox2D CalcBoundBox (void) const;
  bool Contains (const Vector2D &rclV) const;
  void  Intersect (const Polygon2D &rclPolygon, std::list<Polygon2D> &rclResultPolygonList) const;

private:
  std::vector<Vector2D> _aclVct;
};

/** INLINES ********************************************/

inline void BoundBox2D::operator &= (const Vector2D &rclVct)
{
  fMinX = std::min<double>(fMinX, rclVct.fX);
  fMinY = std::min<double>(fMinY, rclVct.fY);
  fMaxX = std::max<double>(fMaxX, rclVct.fX);
  fMaxY = std::max<double>(fMaxY, rclVct.fY);
}
  
inline Vector2D::Vector2D (void)
: fX(0.0), fY(0.0)
{
}

inline Vector2D::Vector2D (float x, float y) 
: fX (x), fY (y)
{
}

inline Vector2D::Vector2D (double x, double y) 
: fX(x),
  fY(y)
{
}

inline Vector2D::Vector2D (const Vector2D &rclVct)
: fX(rclVct.fX), fY(rclVct.fY)
{
}

inline double Vector2D::Length (void) const
{
  return sqrt ((fX * fX) + (fY * fY));
}

inline Vector2D& Vector2D::operator= (const Vector2D &rclVct)
{
  fX = rclVct.fX;
  fY = rclVct.fY;
  return *this;
}

inline bool Vector2D::operator== (const Vector2D &rclVct) const
{
  return (fX == rclVct.fX) && (fY == rclVct.fY);
}

inline Vector2D Vector2D::operator+ (const Vector2D &rclVct) const
{
  return Vector2D(fX + rclVct.fX, fY + rclVct.fY);
}

inline Vector2D Vector2D::operator- (const Vector2D &rclVct) const
{
  return Vector2D(fX - rclVct.fX, fY - rclVct.fY);
}

inline double Vector2D::operator* (const Vector2D &rclVct) const
{
  return (fX * rclVct.fX) + (fY * rclVct.fY);
}

inline void Vector2D::Scale (double fS)
{
  fX *= fS;
  fY *= fS;
}

inline void Vector2D::Normalize (void)
{
  double fLen = Length();
  if (fLen != 0.0f)
  {
    fX /= fLen;
    fY /= fLen;
  }
}

inline void Vector2D::Set (double fPX, double fPY)
{
  fX = fPX;
  fY = fPY;
}

inline Polygon2D::Polygon2D (const Polygon2D &rclPoly)
{
  *this = rclPoly;
}

inline Polygon2D& Polygon2D::operator = (const Polygon2D &rclP)
{
  _aclVct = rclP._aclVct;
  return *this;
}

inline void Polygon2D::DeleteAll (void)
{
  _aclVct.clear();
}

inline size_t Polygon2D::GetCtVectors (void) const
{
  return _aclVct.size ();
}

inline bool Polygon2D::Add (const Vector2D &rclVct)
{
  _aclVct.push_back (rclVct);
  return true;
}

inline bool Polygon2D::Delete (size_t ulNdx)
{
  if ( ulNdx < _aclVct.size() )
  {
    std::vector<Vector2D>::iterator it = _aclVct.begin() + ulNdx;
    _aclVct.erase ( it );
    return true;
  }

  return false;
}

inline Vector2D& Polygon2D::operator[] (size_t ulNdx) const
{
  return (Vector2D&) _aclVct[ulNdx];
}

inline Vector2D& Polygon2D::At (size_t ulNdx) const
{
  return (Vector2D&) _aclVct[ulNdx];
}


inline Line2D::Line2D (const Line2D &rclLine)
    : clV1 (rclLine.clV1),
      clV2 (rclLine.clV2)
{
}

inline Line2D::Line2D (const Vector2D &rclV1, const Vector2D &rclV2)
    : clV1 (rclV1), clV2 (rclV2)
{
}

inline double Line2D::Length (void) const
{
  return (clV2 - clV1).Length ();
}

inline Line2D& Line2D::operator= (const Line2D& rclLine)
{
  clV1 = rclLine.clV1;
  clV2 = rclLine.clV2;
  return *this;
}

inline bool Line2D::operator== (const Line2D& rclLine) const
{
  return (clV1 == rclLine.clV1) && (clV2 == rclLine.clV2);
}

inline bool Line2D::Contains (const Vector2D &rclV) const
{
  return CalcBoundBox ().Contains (rclV);
}

inline BoundBox2D::BoundBox2D (void)
{
  fMinX = fMinY = DOUBLE_MAX;
  fMaxX = fMaxY = - DOUBLE_MAX;
}

inline BoundBox2D::BoundBox2D (const BoundBox2D &rclBB)
    : fMinX (rclBB.fMinX),
      fMinY (rclBB.fMinY),
      fMaxX (rclBB.fMaxX),
      fMaxY (rclBB.fMaxY)
{
}

inline BoundBox2D::BoundBox2D (double fX1, double fY1, double fX2, double fY2)
{
    fMinX = std::min<double>( fX1, fX2 );
    fMaxX = std::max<double>( fX1, fX2 );
    fMinY = std::min<double>( fY1, fY2 );
    fMaxY = std::max<double>( fY1, fY2 );
}

inline bool BoundBox2D::IsValid (void)
{
  return (fMaxX >= fMinX) && (fMaxY >= fMinY);
}

inline BoundBox2D& BoundBox2D::operator= (const BoundBox2D& rclBB)
{
  fMinX = rclBB.fMinX;
  fMinY = rclBB.fMinY;
  fMaxX = rclBB.fMaxX;
  fMaxY = rclBB.fMaxY;
  return *this;
}

inline bool BoundBox2D::operator== (const BoundBox2D& rclBB) const
{
  return 
      (fMinX == rclBB.fMinX) &&
      (fMinY == rclBB.fMinY) &&
      (fMaxX == rclBB.fMaxX) &&
      (fMaxY == rclBB.fMaxY);
}

} // namespace Base

#endif // BASE_TOOLS2D_H


