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

double Vector2D::GetAngle (const Vector2D &rclVect) const
{
  double fDivid, fNum;
  
  fDivid = Length() * rclVect.Length();
 
  if ((fDivid < -1e-10) || (fDivid > 1e-10))
  {
    fNum = (*this * rclVect) / fDivid;
    if (fNum < -1)
      return F_PI;
    else
      if (fNum > 1)
        return 0.0;
      else
        return acos(fNum);
  }
  else
    return -FLOAT_MAX; // division by zero
}

void Vector2D::ProjToLine (const Vector2D &rclPt, const Vector2D &rclLine)
{
  double l  = rclLine.Length();
  double t1 = (rclPt * rclLine) / l;
  Vector2D clNormal = rclLine;
  clNormal.Normalize();
  clNormal.Scale(t1);  
  *this = clNormal;
}

/********************************************************/
/** BOUNDBOX2D ********************************************/

bool BoundBox2D::operator|| (const Line2D &rclLine) const
{
  Line2D clThisLine;
  Vector2D clVct;

  // first line 
  clThisLine.clV1.fX = fMinX;
  clThisLine.clV1.fY = fMinY;
  clThisLine.clV2.fX = fMaxX;
  clThisLine.clV2.fY = fMinY;
  if (clThisLine.IntersectAndContain (rclLine, clVct)) 
    return true;

  // second line
  clThisLine.clV1 = clThisLine.clV2;
  clThisLine.clV2.fX = fMaxX;
  clThisLine.clV2.fY = fMaxY;
  if (clThisLine.IntersectAndContain (rclLine, clVct)) 
    return true;

  // third line
  clThisLine.clV1 = clThisLine.clV2;
  clThisLine.clV2.fX = fMinX;
  clThisLine.clV2.fY = fMaxY;
  if (clThisLine.IntersectAndContain (rclLine, clVct)) 
    return true;

  // fourth line
  clThisLine.clV1 = clThisLine.clV2;
  clThisLine.clV2.fX = fMinX;
  clThisLine.clV2.fY = fMinY;
  if (clThisLine.IntersectAndContain (rclLine, clVct)) 
    return true;

  return false;
}

bool BoundBox2D::operator|| (const BoundBox2D &rclBB) const
{
//// compare bb2-points to this
//if (Contains (Vector2D (rclBB.fMinX, rclBB.fMinY))) return TRUE;
//if (Contains (Vector2D (rclBB.fMaxX, rclBB.fMinY))) return TRUE;
//if (Contains (Vector2D (rclBB.fMaxX, rclBB.fMaxY))) return TRUE;
//if (Contains (Vector2D (rclBB.fMinX, rclBB.fMaxY))) return TRUE;
//
//// compare this-points to bb2
//if (rclBB.Contains (Vector2D (fMinX, fMinY))) return TRUE;
//if (rclBB.Contains (Vector2D (fMaxX, fMinY))) return TRUE;
//if (rclBB.Contains (Vector2D (fMaxX, fMaxY))) return TRUE;
//if (rclBB.Contains (Vector2D (fMinX, fMaxY))) return TRUE;

  if (fMinX       < rclBB.fMaxX  &&
      rclBB.fMinX < fMaxX        &&
      fMinY       < rclBB.fMaxY  &&
      rclBB.fMinY < fMaxY         )
      return true;
  else // no intersection
      return false;
}

bool BoundBox2D::operator|| (const Polygon2D &rclPoly) const
{
  unsigned long i;
  Line2D clLine;
  
  // points contained in boundbox
  for (i = 0; i < rclPoly.GetCtVectors(); i++)
    if (Contains (rclPoly[i]))
      return true;    /***** RETURN INTERSECTION *********/

  // points contained in polygon
  if (rclPoly.Contains (Vector2D (fMinX, fMinY)) ||
      rclPoly.Contains (Vector2D (fMaxX, fMinY)) ||
      rclPoly.Contains (Vector2D (fMaxX, fMaxY)) ||
      rclPoly.Contains (Vector2D (fMinX, fMaxY)))
    return true;    /***** RETURN INTERSECTION *********/
      
  // test intersections of bound-lines
  if (rclPoly.GetCtVectors() < 3) return false;
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
    if (*this || clLine) 
      return true;    /***** RETURN INTERSECTION *********/
  }
  // no intersection
  return false;
}

bool BoundBox2D::Contains (const Vector2D &rclV) const
{
  return
    (rclV.fX >= fMinX) && (rclV.fX <= fMaxX) &&
    (rclV.fY >= fMinY) && (rclV.fY <= fMaxY);
}

/********************************************************/
/** LINE2D **********************************************/

BoundBox2D Line2D::CalcBoundBox (void) const
{
  BoundBox2D clBB;
  clBB.fMinX = std::min<double> (clV1.fX, clV2.fX);
  clBB.fMinY = std::min<double> (clV1.fY, clV2.fY);
  clBB.fMaxX = std::max<double> (clV1.fX, clV2.fX);
  clBB.fMaxY = std::max<double> (clV1.fY, clV2.fY);
  return clBB;
}

bool Line2D::Intersect (const Line2D& rclLine, Vector2D &rclV) const
{
  double m1, m2, b1, b2;
  
  // calc coefficients
  if (fabs (clV2.fX - clV1.fX) > 1e-10)
    m1 = (clV2.fY - clV1.fY) / (clV2.fX - clV1.fX);
  else
    m1 = FLOAT_MAX;
  if (fabs (rclLine.clV2.fX - rclLine.clV1.fX) > 1e-10)
    m2 = (rclLine.clV2.fY - rclLine.clV1.fY) / (rclLine.clV2.fX - rclLine.clV1.fX);
  else
    m2 = FLOAT_MAX;
  if (m1 == m2)     /****** RETURN ERR (parallel lines) *************/
    return false;
  
  b1 = clV1.fY - m1 * clV1.fX;
  b2 = rclLine.clV1.fY - m2 * rclLine.clV1.fX;

  // calc intersection
  if (m1 == FLOAT_MAX)
  {
    rclV.fX = clV1.fX;
    rclV.fY = m2 * rclV.fX + b2;
  }
  else
  if (m2 == FLOAT_MAX)
  {
    rclV.fX = rclLine.clV1.fX;
    rclV.fY = m1 * rclV.fX + b1;
  }
  else
  {
    rclV.fX = (b2 - b1) / (m1 - m2);
    rclV.fY = m1 * rclV.fX + b1;  
  }
  
  return true;    /*** RETURN TRUE (intersection) **********/
}

Vector2D Line2D::FromPos (double fDistance) const
{
  Vector2D clDir(clV2 - clV1);
  clDir.Normalize();
  return Vector2D(clV1.fX + (clDir.fX * fDistance), clV1.fY + (clDir.fY * fDistance));
}

bool Line2D::IntersectAndContain (const Line2D& rclLine, Vector2D &rclV) const
{
  bool rc = Intersect (rclLine, rclV);
  if (rc)
    rc = Contains (rclV) && rclLine.Contains (rclV);
  return rc;
}

/********************************************************/
/** POLYGON2D ********************************************/

BoundBox2D Polygon2D::CalcBoundBox (void) const
{
  unsigned long i;
  BoundBox2D clBB;
  for (i = 0; i < _aclVct.size(); i++)
  {
    clBB.fMinX = std::min<double> (clBB.fMinX, _aclVct[i].fX);
    clBB.fMinY = std::min<double> (clBB.fMinY, _aclVct[i].fY);
    clBB.fMaxX = std::max<double> (clBB.fMaxX, _aclVct[i].fX);
    clBB.fMaxY = std::max<double> (clBB.fMaxY, _aclVct[i].fY);
  }
  return clBB;  
}

static short _CalcTorsion (double *pfLine, double fX, double fY)
{
  short sQuad[2], i;
  double fResX;

  // Klassifizierung der beiden Polygonpunkte in Quadranten
  for (i = 0; i < 2; i++)
  {
    if (pfLine[i * 2] <= fX)
      sQuad[i] = (pfLine[i * 2 + 1] > fY) ? 0 : 3;
    else
      sQuad[i] = (pfLine[i * 2 + 1] > fY) ? 1 : 2;
  }

  // Abbruch bei Linienpunkten innerhalb eines Quadranten
  // Abbruch bei nichtschneidenden Linienpunkten
  if (abs (sQuad[0] - sQuad[1]) <= 1) return 0;

  // Beide Punkte links von ulX
  if (abs (sQuad[0] - sQuad[1]) == 3)
    return (sQuad[0] == 0) ? 1 : -1;

  // Restfaelle : Quadrantendifferenz von 2
  // mathematischer Test auf Schnitt
  fResX = pfLine[0] + (fY - pfLine[1]) /
                ((pfLine[3] - pfLine[1]) / (pfLine[2] - pfLine[0]));
  if (fResX < fX)

    // oben/unten oder unten/oben
    return (sQuad[0] <= 1) ? 1 : -1;

  // Verbleibende Faelle ?
  return 0;
}

bool Polygon2D::Contains (const Vector2D &rclV) const
{
  // Ermittelt mit dem Verfahren der Windungszahl, ob ein Punkt innerhalb 
  // eines Polygonzugs enthalten ist.
  // Summe aller Windungszahlen gibt an, ob ja oder nein.
  double pfTmp[4];
  unsigned long i;
  short sTorsion = 0;

  // Fehlercheck
  if (GetCtVectors() < 3)  return false;

  // fuer alle Polygon-Linien
  for (i = 0; i < GetCtVectors(); i++)
  {
    // Linienstruktur belegen
    if (i == GetCtVectors() - 1)
    {
      // Polygon automatisch schliessen
      pfTmp[0] = _aclVct[i].fX;
      pfTmp[1] = _aclVct[i].fY;
      pfTmp[2] = _aclVct[0].fX;
      pfTmp[3] = _aclVct[0].fY;
    }
    else
    {
      // uebernehmen Punkt i und i+1
      pfTmp[0] = _aclVct[i].fX;
      pfTmp[1] = _aclVct[i].fY;
      pfTmp[2] = _aclVct[i + 1].fX;
      pfTmp[3] = _aclVct[i + 1].fY;
    }

    // Schnitt-Test durchfuehren und Windungszaehler berechnen
    sTorsion += _CalcTorsion (pfTmp, rclV.fX, rclV.fY);
  }

  // Windungszaehler auswerten
  return sTorsion != 0;
}

void Polygon2D::Intersect (const Polygon2D &rclPolygon, std::list<Polygon2D> &rclResultPolygonList) const
{
  // trimmen des uebergebenen Polygons mit dem aktuellen, Ergebnis ist eine Liste von Polygonen (Untermenge des uebergebenen Polygons)
  // das eigene (Trim-) Polygon ist geschlossen
  //
  if ((rclPolygon.GetCtVectors() < 2) || (GetCtVectors() < 2))
    return;

  // position of first points (in or out of polygon)
  bool bInner = Contains(rclPolygon[0]);

  Polygon2D clResultPolygon;
  if (bInner == true)  // add first point if inner trim-polygon
    clResultPolygon.Add(rclPolygon[0]);

  // for each polygon segment
  size_t ulPolyCt = rclPolygon.GetCtVectors();
  size_t ulTrimCt = GetCtVectors();
  for (size_t ulVec = 0; ulVec < (ulPolyCt-1); ulVec++)
  {
    Vector2D clPt0 = rclPolygon[ulVec];
    Vector2D clPt1 = rclPolygon[ulVec+1];
    Line2D clLine(clPt0, clPt1);

    // try to intersect with each line of the trim-polygon
    std::set<double> afIntersections;  // set of intersections (sorted by line parameter)
    Vector2D clTrimPt2;  // second line point
    for (size_t i = 0; i < ulTrimCt; i++)
    {
      clTrimPt2 = At((i + 1) % ulTrimCt);
      Line2D clToTrimLine(At(i), clTrimPt2);
   
      Vector2D clV;
      if (clLine.IntersectAndContain(clToTrimLine, clV) == true)
      {
        // save line parameter of intersection point
        double fDist = (clV - clPt0).Length();
        afIntersections.insert(fDist);
      }
    }

    if (afIntersections.size() > 0)  // intersections founded
    {
      for (std::set<double>::iterator pF = afIntersections.begin(); pF != afIntersections.end(); pF++)
      {
        // intersection point
        Vector2D clPtIS = clLine.FromPos(*pF);
        if (bInner == true)
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

      if (bInner == true) // add line end point if inside
        clResultPolygon.Add(clPt1);
    }
    else
    {  // no intersections, add line (means second point of it) if inside trim-polygon
      if (bInner == true)
        clResultPolygon.Add(clPt1);
    }
  }  

  // add last segment
  if (clResultPolygon.GetCtVectors() > 0)
    rclResultPolygonList.push_back(clResultPolygon);
}

