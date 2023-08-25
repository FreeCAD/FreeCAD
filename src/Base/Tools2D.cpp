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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <cstdlib>
# include <set>
#endif

#include "Tools2D.h"
#include "Vector3D.h"

using namespace Base;

double Vector2d::GetAngle (const Vector2d &rclVect) const
{
  double fDivid = 0.0, fNum = 0.0;

  fDivid = Length() * rclVect.Length();

  if ((fDivid < -1e-10) || (fDivid > 1e-10))
  {
    fNum = (*this * rclVect) / fDivid;
    if (fNum < -1)
      return D_PI;
    else
      if (fNum > 1)
        return 0.0;
      else
        return acos(fNum);
  }
  else
    return -FLOAT_MAX; // division by zero
}

void Vector2d::ProjectToLine (const Vector2d &rclPt, const Vector2d &rclLine)
{
  double l  = rclLine.Length();
  double t1 = (rclPt * rclLine) / l;
  Vector2d clNormal = rclLine;
  clNormal.Normalize();
  clNormal.Scale(t1);
  *this = clNormal;
}

/********************************************************/
/** BOUNDBOX2d ********************************************/

bool BoundBox2d::Intersect(const Line2d &rclLine) const
{
  Line2d clThisLine;
  Vector2d clVct;

  // first line
  clThisLine.clV1.x = MinX;
  clThisLine.clV1.y = MinY;
  clThisLine.clV2.x = MaxX;
  clThisLine.clV2.y = MinY;
  if (clThisLine.IntersectAndContain (rclLine, clVct))
    return true;

  // second line
  clThisLine.clV1 = clThisLine.clV2;
  clThisLine.clV2.x = MaxX;
  clThisLine.clV2.y = MaxY;
  if (clThisLine.IntersectAndContain (rclLine, clVct))
    return true;

  // third line
  clThisLine.clV1 = clThisLine.clV2;
  clThisLine.clV2.x = MinX;
  clThisLine.clV2.y = MaxY;
  if (clThisLine.IntersectAndContain (rclLine, clVct))
    return true;

  // fourth line
  clThisLine.clV1 = clThisLine.clV2;
  clThisLine.clV2.x = MinX;
  clThisLine.clV2.y = MinY;
  if (clThisLine.IntersectAndContain (rclLine, clVct))
    return true;

  return false;
}

bool BoundBox2d::Intersect(const BoundBox2d &rclBB) const
{
  if (MinX       < rclBB.MaxX  &&
      rclBB.MinX < MaxX        &&
      MinY       < rclBB.MaxY  &&
      rclBB.MinY < MaxY         )
      return true;
  else // no intersection
      return false;
}

bool BoundBox2d::Intersect(const Polygon2d &rclPoly) const
{
  unsigned long i = 0;
  Line2d clLine;

  // points contained in boundbox
  for (i = 0; i < rclPoly.GetCtVectors(); i++)
    if (Contains (rclPoly[i]))
      return true;    /***** RETURN INTERSECTION *********/

  // points contained in polygon
  if (rclPoly.Contains (Vector2d (MinX, MinY)) ||
      rclPoly.Contains (Vector2d (MaxX, MinY)) ||
      rclPoly.Contains (Vector2d (MaxX, MaxY)) ||
      rclPoly.Contains (Vector2d (MinX, MaxY)))
    return true;    /***** RETURN INTERSECTION *********/

  // test intersections of bound-lines
  if (rclPoly.GetCtVectors() < 3)
      return false;
  for (i = 0; i < rclPoly.GetCtVectors(); i++)
  {
    if (i == rclPoly.GetCtVectors() - 1)
    {
      clLine.clV1 = rclPoly[i];
      clLine.clV2 = rclPoly[0];
    }
    else
    {
      clLine.clV1 = rclPoly[i];
      clLine.clV2 = rclPoly[i + 1];
    }
    if (Intersect(clLine))
      return true;    /***** RETURN INTERSECTION *********/
  }
  // no intersection
  return false;
}

/********************************************************/
/** LINE2D **********************************************/

BoundBox2d Line2d::CalcBoundBox () const
{
  BoundBox2d clBB;
  clBB.MinX = std::min<double> (clV1.x, clV2.x);
  clBB.MinY = std::min<double> (clV1.y, clV2.y);
  clBB.MaxX = std::max<double> (clV1.x, clV2.x);
  clBB.MaxY = std::max<double> (clV1.y, clV2.y);
  return clBB;
}

bool Line2d::Intersect (const Line2d& rclLine, Vector2d &rclV) const
{
  double m1 = 0.0, m2 = 0.0, b1 = 0.0, b2 = 0.0;

  // calc coefficients
  if (fabs (clV2.x - clV1.x) > 1e-10)
    m1 = (clV2.y - clV1.y) / (clV2.x - clV1.x);
  else
    m1 = DOUBLE_MAX;
  if (fabs (rclLine.clV2.x - rclLine.clV1.x) > 1e-10)
    m2 = (rclLine.clV2.y - rclLine.clV1.y) / (rclLine.clV2.x - rclLine.clV1.x);
  else
    m2 = DOUBLE_MAX;
  if (m1 == m2)     /****** RETURN ERR (parallel lines) *************/
    return false;

  b1 = clV1.y - m1 * clV1.x;
  b2 = rclLine.clV1.y - m2 * rclLine.clV1.x;

  // calc intersection
  if (m1 == DOUBLE_MAX)
  {
    rclV.x = clV1.x;
    rclV.y = m2 * rclV.x + b2;
  }
  else
  if (m2 == DOUBLE_MAX)
  {
    rclV.x = rclLine.clV1.x;
    rclV.y = m1 * rclV.x + b1;
  }
  else
  {
    rclV.x = (b2 - b1) / (m1 - m2);
    rclV.y = m1 * rclV.x + b1;
  }

  return true;    /*** RETURN true (intersection) **********/
}

bool Line2d::Intersect (const Vector2d &rclV, double eps) const
{
    double dxc = rclV.x - clV1.x;
    double dyc = rclV.y - clV1.y;

    double dxl = clV2.x - clV1.x;
    double dyl = clV2.y - clV1.y;

    double cross = dxc * dyl - dyc * dxl;

    // is point on the infinite line?
    if (fabs(cross) > eps)
        return false;

    // point is on line but it is also between V1 and V2?
    double dot = dxc * dxl + dyc * dyl;
    double len = dxl * dxl + dyl * dyl;
    if (dot < -eps || dot > len + eps)
        return false;
    return true;
}

Vector2d Line2d::FromPos (double fDistance) const
{
  Vector2d clDir(clV2 - clV1);
  clDir.Normalize();
  return {clV1.x + (clDir.x * fDistance), clV1.y + (clDir.y * fDistance)};
}

bool Line2d::IntersectAndContain (const Line2d& rclLine, Vector2d &rclV) const
{
  bool rc = Intersect (rclLine, rclV);
  if (rc)
    rc = Contains (rclV) && rclLine.Contains (rclV);
  return rc;
}

/********************************************************/
/** POLYGON2d ********************************************/

BoundBox2d Polygon2d::CalcBoundBox () const
{
  unsigned long i = 0;
  BoundBox2d clBB;
  for (i = 0; i < _aclVct.size(); i++)
  {
    clBB.MinX = std::min<double> (clBB.MinX, _aclVct[i].x);
    clBB.MinY = std::min<double> (clBB.MinY, _aclVct[i].y);
    clBB.MaxX = std::max<double> (clBB.MaxX, _aclVct[i].x);
    clBB.MaxY = std::max<double> (clBB.MaxY, _aclVct[i].y);
  }
  return clBB;
}

static short _CalcTorsion (double *pfLine, double fX, double fY)
{
  int sQuad[2], i = 0; // Changing this from short to int allows the compiler to inline this function
  double fResX = 0.0;

  // Classification of both polygon points into quadrants
  for (i = 0; i < 2; i++)
  {
    if (pfLine[i * 2] <= fX)
      sQuad[i] = (pfLine[i * 2 + 1] > fY) ? 0 : 3;
    else
      sQuad[i] = (pfLine[i * 2 + 1] > fY) ? 1 : 2;
  }

  // Abort at line points within a quadrant
  // Abort at non-intersecting line points
  if (abs (sQuad[0] - sQuad[1]) <= 1)
      return 0;

  // Both points to the left of ulX
  if (abs (sQuad[0] - sQuad[1]) == 3)
    return (sQuad[0] == 0) ? 1 : -1;

  // Remaining cases: Quadrant difference from 2
  // mathematical tests on intersection
  fResX = pfLine[0] + (fY - pfLine[1]) /
                ((pfLine[3] - pfLine[1]) / (pfLine[2] - pfLine[0]));
  if (fResX < fX)

    // up/down or down/up
    return (sQuad[0] <= 1) ? 1 : -1;

  // Remaining cases?
  return 0;
}

bool Polygon2d::Contains (const Vector2d &rclV) const
{
  // Using the number of turns method, determines
  // whether a point is contained within a polygon.
  // The sum of all turns indicates whether yes or no.
  double pfTmp[4];
  unsigned long i = 0;
  short sTorsion = 0;

  // Error check
  if (GetCtVectors() < 3)
      return false;

  // for all polygon lines
  for (i = 0; i < GetCtVectors(); i++)
  {
    // Evidence of line structure
    if (i == GetCtVectors() - 1)
    {
      // Close polygon automatically
      pfTmp[0] = _aclVct[i].x;
      pfTmp[1] = _aclVct[i].y;
      pfTmp[2] = _aclVct[0].x;
      pfTmp[3] = _aclVct[0].y;
    }
    else
    {
      // accept point i and i+1
      pfTmp[0] = _aclVct[i].x;
      pfTmp[1] = _aclVct[i].y;
      pfTmp[2] = _aclVct[i + 1].x;
      pfTmp[3] = _aclVct[i + 1].y;
    }

    // Carry out a cut test and calculate the turn counter
    sTorsion += _CalcTorsion (pfTmp, rclV.x, rclV.y);
  }

  // Evaluate turn counter
  return sTorsion != 0;
}

void Polygon2d::Intersect (const Polygon2d &rclPolygon, std::list<Polygon2d> &rclResultPolygonList) const
{
  // trim the passed polygon with the current one, the result is a list of polygons (subset of the passed polygon)
  // your own (trim) polygon is closed
  //
  if ((rclPolygon.GetCtVectors() < 2) || (GetCtVectors() < 2))
    return;

  // position of first points (in or out of polygon)
  bool bInner = Contains(rclPolygon[0]);

  Polygon2d clResultPolygon;
  if (bInner)  // add first point if inner trim-polygon
    clResultPolygon.Add(rclPolygon[0]);

  // for each polygon segment
  size_t ulPolyCt = rclPolygon.GetCtVectors();
  size_t ulTrimCt = GetCtVectors();
  for (size_t ulVec = 0; ulVec < (ulPolyCt-1); ulVec++)
  {
    Vector2d clPt0 = rclPolygon[ulVec];
    Vector2d clPt1 = rclPolygon[ulVec+1];
    Line2d clLine(clPt0, clPt1);

    // try to intersect with each line of the trim-polygon
    std::set<double> afIntersections;  // set of intersections (sorted by line parameter)
    Vector2d clTrimPt2;  // second line point
    for (size_t i = 0; i < ulTrimCt; i++)
    {
      clTrimPt2 = At((i + 1) % ulTrimCt);
      Line2d clToTrimLine(At(i), clTrimPt2);

      Vector2d clV;
      if (clLine.IntersectAndContain(clToTrimLine, clV))
      {
        // save line parameter of intersection point
        double fDist = (clV - clPt0).Length();
        afIntersections.insert(fDist);
      }
    }

    if (afIntersections.size() > 0)  // intersections founded
    {
      for (double it : afIntersections)
      {
        // intersection point
        Vector2d clPtIS = clLine.FromPos(it);
        if (bInner)
        {
          clResultPolygon.Add(clPtIS);
          rclResultPolygonList.push_back(clResultPolygon);
          clResultPolygon.DeleteAll();
          bInner = false;
        }
        else
        {
          clResultPolygon.Add(clPtIS);
          bInner = true;
        }
      }

      if (bInner) // add line end point if inside
        clResultPolygon.Add(clPt1);
    }
    else
    {  // no intersections, add line (means second point of it) if inside trim-polygon
      if (bInner)
        clResultPolygon.Add(clPt1);
    }
  }

  // add last segment
  if (clResultPolygon.GetCtVectors() > 0)
    rclResultPolygonList.push_back(clResultPolygon);
}

bool Polygon2d::Intersect (const Polygon2d &other) const {
    if (other.GetCtVectors()<2 || GetCtVectors() < 2)
        return false;

    for (auto &v : _aclVct) {
        if (other.Contains(v))
            return true;
    }

    if (Contains(other[0]))
        return true;

    for (size_t j=1; j<other.GetCtVectors(); ++j) {
        auto &v0 = other[j-1];
        auto &v1 = other[j];

        if (Contains(v1))
            return true;

        Line2d line(v0, v1);
        for (size_t i=0; i<GetCtVectors(); ++i) {
            Line2d line2(At(i), At((i+1)%GetCtVectors()));
            Vector2d v;
            if (line.IntersectAndContain(line2, v))
                return true;
        }
    }
    return false;
}


bool Polygon2d::Intersect (const Vector2d &rclV, double eps) const
{
    if (_aclVct.size() < 2)
        return false;

    size_t numPts = GetCtVectors();
    for (size_t i = 0; i < numPts; i++) {
        Vector2d clPt0 = (*this)[i];
        Vector2d clPt1 = (*this)[(i+1)%numPts];
        Line2d clLine(clPt0, clPt1);
        if (clLine.Intersect(rclV, eps))
            return true;
    }

    return false;
}
