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


#ifndef BASE_BOUNDBOX_H
#define BASE_BOUNDBOX_H

#include "Vector3D.h"
#include "Matrix.h"
#include "ViewProj.h"
#include "Tools2D.h"

// Checks if point K lies on the ray [A,B[
#define IS_ON_RAY(A,B,K)  (((A) <= (K)) && ((B) > (K)))

namespace Base {

class ViewProjMethod;

/** The 3D bounding box class. */
template <class _Precision>
class BoundBox3
{
public:
  typedef _Precision num_type;
  typedef float_traits<num_type> traits_type;

  /**  Public attributes */
  //@{
  _Precision MinX;
  _Precision MinY;
  _Precision MinZ;
  _Precision MaxX;
  _Precision MaxY;
  _Precision MaxZ;
  //@}
  
  /** Builds box from pairs of x,y,z values. */
  inline explicit BoundBox3 ( _Precision fMinX =  FLOAT_MAX, _Precision fMinY =  FLOAT_MAX,
                              _Precision fMinZ =  FLOAT_MAX, _Precision fMaxX = -FLOAT_MAX,
                              _Precision fMaxY = -FLOAT_MAX, _Precision fMaxZ = -FLOAT_MAX );
  BoundBox3 (const BoundBox3<_Precision> &rcBB) { *this = rcBB; }
  /** Builds box from an array of points. */
  inline BoundBox3 (const Vector3<_Precision> *pclVect, unsigned long ulCt);

  /** Defines a bounding box around the center \a rcCnt with the 
   * distances \a fDistance in each coordinate.
   */
  BoundBox3 (const Vector3<_Precision> &rcCnt, _Precision fDistance);
  ~BoundBox3 ();
 
  /// Assignment operator 
  inline  BoundBox3<_Precision>& operator = (const BoundBox3<_Precision> &rcBound);
 
  /** Methods for intersection, cuttíng and union of bounding boxes */
  //@{
  /** Checks for intersection. */
  inline bool operator && (const BoundBox3<_Precision> &rcBB) const;
  /** Checks for intersection. */
  inline bool operator && (const BoundBox2D &rcBB) const;
  /** Computes the intersection between two bounding boxes.
   * The result is also a bounding box.
   */
  BoundBox3<_Precision> operator & (const BoundBox3<_Precision> &rcBB) const;
  /** Appends the point to the box. The box can grow but not shrink. */
  inline  BoundBox3<_Precision>& operator &= (const Vector3<_Precision> &rclVect);
  /** The union of two bounding boxes. */
  BoundBox3<_Precision> operator | (const BoundBox3<_Precision> &rcBB) const;
  /** Appends the point to the box. The box can grow but not shrink. 
   * This method does the same as the &= operator unless that it nothing returns.
   */
  inline  void Add (const Vector3<_Precision> &rclVect);
  /** Appends the bounding box to this box. The box can grow but not shrink. */
  inline  void Add (const BoundBox3<_Precision> &rcBB);
  //@}
 
  /** Test methods */
  //@{
  /** Checks if this point lies inside the box. */
  inline bool IsInBox (const Vector3<_Precision> &rcVct) const;
  /** Checks if this 3D box lies inside the box. */
  inline bool IsInBox (const BoundBox3<_Precision> &rcBB) const;
  /** Checks if this 2D box lies inside the box. */
  inline bool IsInBox (const BoundBox2D &rcbb) const;
  /** Checks whether the bounding box is valid. */
  bool IsValid (void) const;
  //@}

  enum OCTANT {OCT_LDB = 0, OCT_RDB, OCT_LUB, OCT_RUB,
               OCT_LDF,     OCT_RDF, OCT_LUF, OCT_RUF};
  bool GetOctantFromVector (const Vector3<_Precision> &rclVct, OCTANT &rclOctant) const;
  BoundBox3<_Precision> CalcOctant (typename BoundBox3<_Precision>::OCTANT Octant) const;

  enum SIDE { LEFT =0, RIGHT=1, TOP=2, BOTTOM=3, FRONT=4, BACK=5, INVALID=255 };

  /**
   * Returns the corner point \a usPoint.
   * 0: front,bottom,left    1: front,bottom,right
   * 2: front,top,right      3: front,top,left
   * 4: back,bottom,left     5: back,bottom,right 
   * 6: back,top,right       7: back,top,left 
   */
  inline Vector3<_Precision> CalcPoint (unsigned short usPoint) const;
  /** Returns the plane of the given side. */
  void CalcPlane (unsigned short usPlane, Vector3<_Precision>& rBase, Vector3<_Precision>& rNormal ) const;
  /** Calculates the two points of an edge.
   * 0. edge P0-P1      1. edge P1-P2      2. edge P2-P3
   * 3. edge P3-P0      4. edge P4-P5      5. edge P5-P6
   * 6. edge P6-P7      7. edge P7-P4      8. edge P0-P4
   * 9. edge P1-P5     10. edge P2-P6     11. edge P3-P7 
   */
  bool  CalcDistance (unsigned short usEdge, Vector3<_Precision>& rcP0, Vector3<_Precision>& rcP1) const;
  /** Intersection point of an inner search ray with the bounding box, built of
   * the base \a rcVct and the direction \a rcVctDir. \a rcVct must lie inside the
   * bounding box.
   */
  Vector3<_Precision> IntersectionPoint (const Vector3<_Precision> &rcVct, const Vector3<_Precision> &rcVctDir) const;
  /** Checks for intersection with line incl. search tolerance. */      
  bool IsCutLine ( const Vector3<_Precision>& rcBase, const Vector3<_Precision>& rcDir, _Precision fTolerance = 0.0f) const;
  /** Checks if this plane specified by (point,normal) cuts this box. */
  inline bool IsCutPlane (const Vector3<_Precision> &rclBase, const Vector3<_Precision> &rclNormal) const;
  /** Computes the intersection points of line and bounding box. */
  bool IntersectWithLine (const Vector3<_Precision>& rcBase, const Vector3<_Precision>& rcDir, Vector3<_Precision>& rcP0, Vector3<_Precision>& rcP1) const;
  /** Computes the intersection point of line and a plane of the bounding box. */
  bool IntersectPlaneWithLine (unsigned short usSide, const Vector3<_Precision>& rcBase, const Vector3<_Precision>& rcDir, 
                               Vector3<_Precision>& rcP0) const;
  /** Returns the side of the bounding box the ray exits. */
  typename BoundBox3<_Precision>::SIDE GetSideFromRay (const Vector3<_Precision> &rclPt, const Vector3<_Precision> &rclDir) const;
  /** Returns the side of the bounding box the ray exits. */
  typename BoundBox3<_Precision>::SIDE GetSideFromRay (const Vector3<_Precision> &rclPt, const Vector3<_Precision> &rclDir, Vector3<_Precision>& rcInt) const;
  
  /**
   * Searches for the nearest point of the bounding box. 
   * \note Point must be inside the bounding box.
   */
  Vector3<_Precision> NearestPoint (const Vector3<_Precision> &rclPt) const;
  /** Projects the box onto a plane and returns a 2D box. */
  BoundBox2D ProjectBox(const ViewProjMethod *rclP) const;
  BoundBox3<_Precision> Transformed(const Matrix4D& mat) const;

  /** Returns the center.of the box. */
  inline Vector3<_Precision> CalcCenter (void) const;
  inline _Precision CalcDiagonalLength (void) const;
  void Flush (void);
  
  /** Enlarges the box with factor \a fLen. */
  inline void Enlarge (_Precision fLen);   
  /** Shrinks the box with factor \a fLen. */
  inline void Shrink  (_Precision fLen);   

  /** Calculates expansion in x-direction. */
  inline _Precision LengthX (void) const;
  /** Calculates expansion in y-direction. */
  inline _Precision LengthY (void) const;
  /** Calculates expansion in z-direction. */
  inline _Precision LengthZ (void) const;
  /** Moves in x-direction. */
  inline void MoveX (_Precision f);
  /** Moves in y-direction. */
  inline void MoveY (_Precision f);
  /** Moves in z-direction. */
  inline void MoveZ (_Precision f);
  /** Scales in x-direction. */
  inline void ScaleX (_Precision f);
  /** Scales in y-direction. */
  inline void ScaleY (_Precision f);
  /** Scales in z-direction. */
  inline void ScaleZ (_Precision f);
 
  /** Prints the values to stdout. */
  void Print (void);
};


template <class _Precision>
inline BoundBox3<_Precision>::BoundBox3 (const Vector3<_Precision> &rcVector, _Precision fDistance)
{
  MinX = rcVector.x - fDistance; 
  MaxX = rcVector.x + fDistance;
  MinY = rcVector.y - fDistance; 
  MaxY = rcVector.y + fDistance;
  MinZ = rcVector.z - fDistance; 
  MaxZ = rcVector.z + fDistance;
}

template <class _Precision>
inline BoundBox3<_Precision>::~BoundBox3 ()
{
}

template <class _Precision>
inline BoundBox3<_Precision> BoundBox3<_Precision>::operator & (const BoundBox3<_Precision> &rcBB) const
{
  BoundBox3<_Precision> cBBRes;

  cBBRes.MinX = std::max<_Precision> (MinX, rcBB.MinX);
  cBBRes.MaxX = std::min<_Precision> (MaxX, rcBB.MaxX);
  cBBRes.MinY = std::max<_Precision> (MinY, rcBB.MinY);
  cBBRes.MaxY = std::min<_Precision> (MaxY, rcBB.MaxY);
  cBBRes.MinZ = std::max<_Precision> (MinZ, rcBB.MinZ);
  cBBRes.MaxZ = std::min<_Precision> (MaxZ, rcBB.MaxZ);

  return cBBRes;
}

template <class _Precision>
inline BoundBox3<_Precision> BoundBox3<_Precision>::operator | (const BoundBox3<_Precision> &rcBB) const
{
  BoundBox3<_Precision> cBBRes;

  cBBRes.MinX = std::min<_Precision> (MinX, rcBB.MinX);
  cBBRes.MaxX = std::max<_Precision> (MaxX, rcBB.MaxX);
  cBBRes.MinY = std::min<_Precision> (MinY, rcBB.MinY);
  cBBRes.MaxY = std::max<_Precision> (MaxY, rcBB.MaxY);
  cBBRes.MinZ = std::min<_Precision> (MinZ, rcBB.MinZ);
  cBBRes.MaxZ = std::max<_Precision> (MaxZ, rcBB.MaxZ);

  return cBBRes;
}

template <class _Precision>
inline  void BoundBox3<_Precision>::Add (const Vector3<_Precision> &rclVect)
{
  this->MinX = std::min<_Precision>(this->MinX, rclVect.x);
  this->MinY = std::min<_Precision>(this->MinY, rclVect.y);
  this->MinZ = std::min<_Precision>(this->MinZ, rclVect.z);
  this->MaxX = std::max<_Precision>(this->MaxX, rclVect.x);
  this->MaxY = std::max<_Precision>(this->MaxY, rclVect.y);
  this->MaxZ = std::max<_Precision>(this->MaxZ, rclVect.z);
}

template <class _Precision>
inline  void BoundBox3<_Precision>::Add (const BoundBox3<_Precision> &rcBB)
{
  this->MinX = std::min<_Precision> (this->MinX, rcBB.MinX);
  this->MaxX = std::max<_Precision> (this->MaxX, rcBB.MaxX);
  this->MinY = std::min<_Precision> (this->MinY, rcBB.MinY);
  this->MaxY = std::max<_Precision> (this->MaxY, rcBB.MaxY);
  this->MinZ = std::min<_Precision> (this->MinZ, rcBB.MinZ);
  this->MaxZ = std::max<_Precision> (this->MaxZ, rcBB.MaxZ);
}

template <class _Precision>
inline bool BoundBox3<_Precision>::IsCutLine ( const Vector3<_Precision>& rcBase, const Vector3<_Precision>& rcDir, _Precision fTolerance) const
{
  _Precision fDist; 
 
  // zuerst nur grobe und schnelle Pruefung, indem der
  // Abstand der Linie zum Mittelpunkt der BB berechnet wird
  // und mit der maximalen Diagonalenlaenge + fTolerance
  // verglichen wird.
 
  // Distanz zwischen Mittelpunkt und Linie
  fDist = (rcDir % (CalcCenter() - rcBase)).Length() / rcDir.Length();

  if (fDist > (CalcDiagonalLength() + fTolerance))
    return false;
  else // hier genauerer Test
  {
    unsigned char i;
    Vector3<_Precision>  clVectRes;
   
    // schneide jede Seitenflaeche mit der Linie
    for (i = 0; i < 6; i++)
    {
     
      if (IntersectPlaneWithLine(i, rcBase, rcDir, clVectRes) == true)
      {
        // pruefe, ob Schnittpunkt innerhalb BB-Grenzen + Toleranz
        switch (i)
        {
          case 0 :  // linke und rechte Ebene
          case 1 :
            if ((IS_ON_RAY (MinY - fTolerance, MaxY + fTolerance, clVectRes.y) &&
                 IS_ON_RAY (MinZ - fTolerance, MaxZ + fTolerance, clVectRes.z)) == true) 
              return true;                     
            break;                    
          case 2 :  // obere und untere Ebene
          case 3 :
            if ((IS_ON_RAY (MinX - fTolerance, MaxX + fTolerance, clVectRes.x) &&
                 IS_ON_RAY (MinZ - fTolerance, MaxZ + fTolerance, clVectRes.z))== true) 
              return true;                     
            break;                    
          case 4 :  // vordere und hintere Ebene
          case 5 :
            if ((IS_ON_RAY (MinX - fTolerance, MaxX + fTolerance, clVectRes.x) &&
                 IS_ON_RAY (MinY - fTolerance, MaxY + fTolerance, clVectRes.y)) == true) 
              return true;                     
            break;                    
        }
      }
    }    
  }
  
  return false;
} 

template <class _Precision>
inline bool BoundBox3<_Precision>::IsValid (void) const
{
  return ((MinX <= MaxX) && (MinY <= MaxY) && (MinZ <= MaxZ));
}

#define HALF(A,B)  ((A)+((B-A)/2))

template <class _Precision>
inline bool BoundBox3<_Precision>::GetOctantFromVector (const Vector3<_Precision> &rclVct, OCTANT &rclOctant) const
{
  if (!IsInBox (rclVct))
    return false;
    
  unsigned short usNdx = 0;
  if (IS_ON_RAY (HALF (MinX, MaxX), MaxX, rclVct.x))  // left/RIGHT
    usNdx |= 1;
  if (IS_ON_RAY (HALF (MinY, MaxY), MaxY, rclVct.y))  // down/UP
    usNdx |= 2;
  if (IS_ON_RAY (HALF (MinZ, MaxZ), MaxZ, rclVct.z))  // back/FRONT
    usNdx |= 4;
  rclOctant = (OCTANT) usNdx;
  return true;
}

template <class _Precision>
inline BoundBox3<_Precision> BoundBox3<_Precision>::CalcOctant (typename BoundBox3< _Precision >::OCTANT Octant) const
{
  BoundBox3<_Precision> cOct (*this);

  switch (Octant)
  {
    case OCT_LDB:
      cOct.MaxX = HALF (cOct.MinX, cOct.MaxX);
      cOct.MaxY = HALF (cOct.MinY, cOct.MaxY);
      cOct.MaxZ = HALF (cOct.MinZ, cOct.MaxZ);
      break;

    case OCT_RDB:
      cOct.MinX = HALF (cOct.MinX, cOct.MaxX);
      cOct.MaxY = HALF (cOct.MinY, cOct.MaxY);
      cOct.MaxZ = HALF (cOct.MinZ, cOct.MaxZ);
      break;

    case OCT_LUB:
      cOct.MaxX = HALF (cOct.MinX, cOct.MaxX);
      cOct.MinY = HALF (cOct.MinY, cOct.MaxY);
      cOct.MaxZ = HALF (cOct.MinZ, cOct.MaxZ);
      break;

    case OCT_RUB:
      cOct.MinX = HALF (cOct.MinX, cOct.MaxX);
      cOct.MinY = HALF (cOct.MinY, cOct.MaxY);
      cOct.MaxZ = HALF (cOct.MinZ, cOct.MaxZ);
      break;

    case OCT_LDF:
      cOct.MaxX = HALF (cOct.MinX, cOct.MaxX);
      cOct.MaxY = HALF (cOct.MinY, cOct.MaxY);
      cOct.MinZ = HALF (cOct.MinZ, cOct.MaxZ);
      break;

    case OCT_RDF:
      cOct.MinX = HALF (cOct.MinX, cOct.MaxX);
      cOct.MaxY = HALF (cOct.MinY, cOct.MaxY);
      cOct.MinZ = HALF (cOct.MinZ, cOct.MaxZ);
      break;

    case OCT_LUF:
      cOct.MaxX = HALF (cOct.MinX, cOct.MaxX);
      cOct.MinY = HALF (cOct.MinY, cOct.MaxY);
      cOct.MinZ = HALF (cOct.MinZ, cOct.MaxZ);
      break;

    case OCT_RUF:
      cOct.MinX = HALF (cOct.MinX, cOct.MaxX);
      cOct.MinY = HALF (cOct.MinY, cOct.MaxY);
      cOct.MinZ = HALF (cOct.MinZ, cOct.MaxZ);
      break;
  }
  return cOct;
}

#undef HALF

template <class _Precision>
inline void BoundBox3<_Precision>::CalcPlane (unsigned short usPlane, Vector3<_Precision>& rBase, Vector3<_Precision>& rNormal ) const
{
  switch (usPlane)
  { 
  //links
  case 0:
    rBase.Set(MinX, MinY, MaxZ);
    rNormal.Set(1.0f, 0.0f, 0.0f);
    break;

  // rechts
  case 1:                                   
    rBase.Set(MaxX, MinY, MaxZ);
    rNormal.Set(1.0f, 0.0f, 0.0f);
    break;

  // oben
  case 2:
    rBase.Set(MinX, MaxY, MaxZ);
    rNormal.Set(0.0f, 1.0f, 0.0f);
    break;

  // unten
  case 3:
    rBase.Set(MinX, MinY, MaxZ);
    rNormal.Set(0.0f, 1.0f, 0.0f);
    break;

  // vorne
  case 4:
    rBase.Set(MinX, MinY, MaxZ);
    rNormal.Set(0.0f, 0.0f, 1.0f);
    break;

  // hinten
  default:
    rBase.Set(MinX, MinY, MinZ);
    rNormal.Set(0.0f, 0.0f, 1.0f);
    break;
  }
}

template <class _Precision>
inline bool BoundBox3<_Precision>::CalcDistance (unsigned short usEdge, Vector3<_Precision>& rcP0, Vector3<_Precision>& rcP1) const
{
  switch (usEdge)
  {
  case  0: 
    rcP0 = CalcPoint(0);
    rcP1 = CalcPoint(1);
    break;
  case  1: 
    rcP0 = CalcPoint(1);
    rcP1 = CalcPoint(2);
    break;
  case  2: 
    rcP0 = CalcPoint(2);
    rcP1 = CalcPoint(3);
    break;
  case  3: 
    rcP0 = CalcPoint(3);
    rcP1 = CalcPoint(0);
    break;
  case  4: 
    rcP0 = CalcPoint(4);
    rcP1 = CalcPoint(5);
    break;
  case  5: 
    rcP0 = CalcPoint(5);
    rcP1 = CalcPoint(6);
    break;
  case  6: 
    rcP0 = CalcPoint(6);
    rcP1 = CalcPoint(7);
    break;
  case  7: 
    rcP0 = CalcPoint(7);
    rcP1 = CalcPoint(4);
    break;
  case  8: 
    rcP0 = CalcPoint(0);
    rcP1 = CalcPoint(4);
    break;
  case  9: 
    rcP0 = CalcPoint(1);
    rcP1 = CalcPoint(5);
    break;
  case 10: 
    rcP0 = CalcPoint(2);
    rcP1 = CalcPoint(6);
    break;
  case 11: 
    rcP0 = CalcPoint(3);
    rcP1 = CalcPoint(7);
    break;
  default: 
    return false; // undefined
  }

  return true;
}

template <class _Precision>
inline Vector3<_Precision> BoundBox3<_Precision>::IntersectionPoint (const Vector3<_Precision> &rcVct, const Vector3<_Precision> &rcVctDir) const
{
  BoundBox3<_Precision> cCmpBound(*this);
  bool rc;
  unsigned short i;
  Vector3<_Precision>   cVctRes;

  // Vergleichs-BB um REEN_EPS vergroessern
  cCmpBound.MaxX += traits_type::epsilon();
  cCmpBound.MaxY += traits_type::epsilon();
  cCmpBound.MaxZ += traits_type::epsilon();
  cCmpBound.MinX -= traits_type::epsilon();
  cCmpBound.MinY -= traits_type::epsilon();
  cCmpBound.MinZ -= traits_type::epsilon();

  // Liegt Punkt innerhalb ?
  if (cCmpBound.IsInBox (rcVct))
  {
    // Seitenebenen testen
    for (i = 0, rc = false; (i < 6) && (!rc); i++)
    {
      rc = IntersectPlaneWithLine( i, rcVct, rcVctDir, cVctRes );
      if(!cCmpBound.IsInBox(cVctRes)) 
        rc = false;
      if (rc == true )
      {
        // Liegt Schnittpunkt in gesuchter Richtung
        // oder wurde gegenueberliegende Seite gefunden ?
        // -> Skalarprodukt beider Richtungsvektoren > 0 (Winkel < 90)
        rc = ((cVctRes - rcVct) * rcVctDir) > 0.0F;
      }
    }
  }
  else
    rc = false;

  // Schnittpunkt zurueckgeben
  return cVctRes;
}

template <class _Precision>
inline bool BoundBox3<_Precision>::IntersectPlaneWithLine (unsigned short usSide, const Vector3<_Precision>& rcBase,
                                         const Vector3<_Precision>& rcDir, Vector3<_Precision>& rcP0) const
{
  _Precision k;
  Vector3<_Precision> cBase, cNormal;
  Vector3<_Precision>  cDir(rcDir);
  CalcPlane(usSide, cBase, cNormal);

  if ((cNormal * cDir) == 0.0f) 
    return false;  // no point of intersection
  else
  {
    k = (cNormal * (cBase - rcBase)) / (cNormal * cDir);
    cDir.Scale(k, k, k);
    rcP0 = rcBase + cDir;

    return true;
  }
}

template <class _Precision>
inline bool BoundBox3<_Precision>::IntersectWithLine ( const Vector3<_Precision> & rcBase, const Vector3<_Precision>& rcDir,
                                     Vector3<_Precision>& rcP0, Vector3<_Precision>& rcP1 ) const
{
  Vector3<_Precision>  clVectRes, clVect[6];
  unsigned short i, j;
  
  j = 0;
  // schneide jede Seitenflaeche mit der Linie
  for (i = 0; i < 6; i++)
  {

    if ( IntersectPlaneWithLine(i, rcBase, rcDir, clVectRes ) )
    {
      // pruefe, ob Schnittpunkt innerhalb BB-Grenzen
      switch (i)
      {
        case 0 :  // linke und rechte Ebene
        case 1 :
          if ((IS_ON_RAY(MinY, MaxY, clVectRes.y) &&
               IS_ON_RAY(MinZ, MaxZ, clVectRes.z)) == true)
          {
            clVect[j] = clVectRes;
            j++;
          }                                 
          break;                    
        case 2 :  // obere und untere Ebene
        case 3 :
          if ((IS_ON_RAY(MinX, MaxX, clVectRes.x) &&
               IS_ON_RAY(MinZ, MaxZ, clVectRes.z))== true)
          {
            clVect[j] = clVectRes;
            j++;                     
          }
          break;                    
        case 4 :  // vordere und hintere Ebene
        case 5 :
          if ((IS_ON_RAY(MinX, MaxX, clVectRes.x) &&
               IS_ON_RAY(MinY, MaxY, clVectRes.y)) == true) 
          {
            clVect[j] = clVectRes;
            j++;
          }        
          break;                    
      }
    }
  }
  
  if (j == 2)  
  {
    rcP0 = clVect[0];
    rcP1 = clVect[1];
    return true;
  }
  else if (j > 2)  // suche 2 unterschiedliche Schnittpunkte
  {
    for (i = 1; i < j; i++)
    {
      if (clVect[i] != clVect[0])
      {
        rcP0 = clVect[0];
        rcP1 = clVect[i];
        return true;
      }
    }
  }
  
  return false; 
}

template <class _Precision>
inline typename BoundBox3<_Precision>::SIDE BoundBox3<_Precision>::GetSideFromRay (const Vector3<_Precision> &rclPt, const Vector3<_Precision> &rclDir) const
{
  Vector3<_Precision> cIntersection;
  return GetSideFromRay( rclPt, rclDir, cIntersection);
}

template <class _Precision>
inline typename BoundBox3<_Precision>::SIDE BoundBox3<_Precision>::GetSideFromRay (const Vector3<_Precision> &rclPt, const Vector3<_Precision> &rclDir,
                                                                                   Vector3<_Precision>& rcInt) const
{
  Vector3<_Precision> cP0, cP1;
  if ( IntersectWithLine(rclPt, rclDir, cP0, cP1) == false )
    return INVALID;

  Vector3<_Precision>  cOut;
  // same orientation
  if ( (cP1-cP0)*rclDir > 0.0f )
    cOut = cP1;
  else
    cOut = cP0;

  rcInt = cOut;

  _Precision fMax = 1.0e-3f;
  SIDE  tSide = INVALID;

  if (fabs(cOut.x - MinX) < fMax)      // linke Ebene
  {
    fMax = _Precision(fabs(cOut.x - MinX));
    tSide = LEFT;
  }

  if (fabs(cOut.x - MaxX) < fMax) // rechte Ebene
  {
    fMax = _Precision(fabs(cOut.x - MaxX));
    tSide = RIGHT;
  }

  if (fabs(cOut.y - MinY) < fMax) // untere Ebene
  {
    fMax = _Precision(fabs(cOut.y - MinY));
    tSide = BOTTOM;
  }

  if (fabs(cOut.y - MaxY) < fMax) // obere Ebene
  {
    fMax = _Precision(fabs(cOut.y - MaxY));
    tSide = TOP;
  }

  if (fabs(cOut.z - MinZ) < fMax) // vordere Ebene
  { 
    fMax = _Precision(fabs(cOut.z - MinZ));
    tSide = FRONT;
  }

  if (fabs(cOut.z - MaxZ) < fMax) // hintere Ebene
  {
    fMax = _Precision(fabs(cOut.z - MaxZ));
    tSide = BACK;
  }

  return tSide;
}

template <class _Precision>
inline void BoundBox3<_Precision>::Flush (void)
{
  MinX = MinY = MinZ =  FLOAT_MAX;
  MaxX = MaxY = MaxZ = -FLOAT_MAX;
}

template <class _Precision>
inline void BoundBox3<_Precision>::Print (void)
{
  printf ("X1 : %5.2f   Y1 : %5.2f   Z1 : %5.2f\n", MinX, MinY, MinZ);
  printf ("X2 : %5.2f   Y2 : %5.2f   Z2 : %5.2f\n", MaxX, MaxY, MaxZ);
}

template <class _Precision>
inline BoundBox2D BoundBox3<_Precision>::ProjectBox(const ViewProjMethod *pclP) const
{
  BoundBox2D  clBB2D;

  clBB2D.SetVoid();

  for (int i = 0; i < 8; i++)
  {
    Vector3<_Precision> clTrsPt = (*pclP)(CalcPoint(i));
    clBB2D &= Vector2D(clTrsPt.x, clTrsPt.y);
  }

  return clBB2D;
}

template <class _Precision>
inline BoundBox3<_Precision> BoundBox3<_Precision>::Transformed(const Matrix4D& mat) const
{
  BoundBox3<_Precision> bbox;
  for (int i=0; i<8; i++)
      bbox.Add(mat * CalcPoint(i));
  return bbox;
}

template <class _Precision>
inline Vector3<_Precision> BoundBox3<_Precision>::NearestPoint (const Vector3<_Precision> &rclPt) const
{
  // Suche naechsten Punkt auf der BB, !!! Punkt MUSS innerhalb BB liegen !!!
  _Precision fMinDist = FLOAT_MAX;
  Vector3<_Precision> cBase, cNormal, clRet;

  for (int i = 0; i < 6; i++)
  {
    Vector3<_Precision> clTemp = rclPt;
    CalcPlane(i, cBase, cNormal);
    clTemp.ProjToPlane(cBase, cNormal);
    _Precision fDist = (clTemp - rclPt).Length();
    if (fDist < fMinDist)
    {
      fMinDist = fDist;
      clRet = clTemp; 
    }
  }

  return clRet;
}


template <class _Precision>
inline BoundBox3<_Precision>::BoundBox3 (_Precision fMinX, _Precision fMinY, _Precision fMinZ,
                                 _Precision fMaxX, _Precision fMaxY, _Precision fMaxZ)
: MinX(fMinX), MinY(fMinY), MinZ(fMinZ),
  MaxX(fMaxX), MaxY(fMaxY), MaxZ(fMaxZ)
{
}

template <class _Precision>
inline BoundBox3<_Precision>::BoundBox3 (const Vector3<_Precision> *pclVect, unsigned long ulCt)
: MinX(FLOAT_MAX),  MinY(FLOAT_MAX),  MinZ(FLOAT_MAX),
  MaxX(-FLOAT_MAX), MaxY(-FLOAT_MAX), MaxZ(-FLOAT_MAX)
{
  const Vector3<_Precision>  *pI, *pEnd = pclVect + ulCt;
  for (pI = pclVect; pI < pEnd; pI++)
  {
    MinX = std::min<_Precision>(MinX, pI->x);
    MinY = std::min<_Precision>(MinY, pI->y);
    MinZ = std::min<_Precision>(MinZ, pI->z);
    MaxX = std::max<_Precision>(MaxX, pI->x);
    MaxY = std::max<_Precision>(MaxY, pI->y);
    MaxZ = std::max<_Precision>(MaxZ, pI->z);
  }
}

template <class _Precision>
inline  BoundBox3<_Precision>& BoundBox3<_Precision>::operator = (const BoundBox3<_Precision> &rcBound)
{
  MinX = rcBound.MinX;
  MinY = rcBound.MinY;
  MinZ = rcBound.MinZ;
  MaxX = rcBound.MaxX;
  MaxY = rcBound.MaxY;
  MaxZ = rcBound.MaxZ;
  return *this;
}

template <class _Precision>
inline  BoundBox3<_Precision>& BoundBox3<_Precision>::operator &= (const Vector3<_Precision> &rclVect)
{
  MinX = std::min<_Precision>(MinX, rclVect.x);
  MinY = std::min<_Precision>(MinY, rclVect.y);
  MinZ = std::min<_Precision>(MinZ, rclVect.z);
  MaxX = std::max<_Precision>(MaxX, rclVect.x);
  MaxY = std::max<_Precision>(MaxY, rclVect.y);
  MaxZ = std::max<_Precision>(MaxZ, rclVect.z);
  return *this;
}

template <class _Precision>
inline bool BoundBox3<_Precision>::operator && (const BoundBox3<_Precision> &rcBB) const
{
  return  (IS_ON_RAY (MinX, MaxX, rcBB.MinX)        ||
           IS_ON_RAY (MinX, MaxX, rcBB.MaxX)        ||
           IS_ON_RAY (rcBB.MinX,  rcBB.MaxX, MinX)  ||
           IS_ON_RAY (rcBB.MinX,  rcBB.MaxX, MaxX)) &&
          (IS_ON_RAY (MinY, MaxY, rcBB.MinY)        ||
           IS_ON_RAY (MinY, MaxY, rcBB.MaxY)        ||
           IS_ON_RAY (rcBB.MinY,  rcBB.MaxY, MinY)  ||
           IS_ON_RAY (rcBB.MinY,  rcBB.MaxY, MaxY)) &&
          (IS_ON_RAY (MinZ, MaxZ, rcBB.MinZ)        ||
           IS_ON_RAY (MinZ, MaxZ, rcBB.MaxZ)        ||
           IS_ON_RAY (rcBB.MinZ,  rcBB.MaxZ, MinZ)  ||
           IS_ON_RAY (rcBB.MinZ,  rcBB.MaxZ, MaxZ));
}

template <class _Precision>
inline bool BoundBox3<_Precision>::operator && (const BoundBox2D &rcBB) const
{
  return  (IS_ON_RAY (MinX, MaxX, rcBB.fMinX)         ||
           IS_ON_RAY (MinX, MaxX, rcBB.fMaxX)         ||
           IS_ON_RAY (rcBB.fMinX,  rcBB.fMaxX, MinX)  ||
           IS_ON_RAY (rcBB.fMinX,  rcBB.fMaxX, MaxX)) &&
          (IS_ON_RAY (MinY, MaxY, rcBB.fMinY)         ||
           IS_ON_RAY (MinY, MaxY, rcBB.fMaxY)         ||
           IS_ON_RAY (rcBB.fMinY,  rcBB.fMaxY, MinY)  ||
           IS_ON_RAY (rcBB.fMinY,  rcBB.fMaxY, MaxY) );
}

template <class _Precision>
inline Vector3<_Precision> BoundBox3<_Precision>::CalcPoint (unsigned short usPoint) const
{
  switch (usPoint)
  {
    case 0: return Vector3<_Precision>(MinX, MinY, MaxZ);
    case 1: return Vector3<_Precision>(MaxX, MinY, MaxZ);
    case 2: return Vector3<_Precision>(MaxX, MaxY, MaxZ);
    case 3: return Vector3<_Precision>(MinX, MaxY, MaxZ);
    case 4: return Vector3<_Precision>(MinX, MinY, MinZ);
    case 5: return Vector3<_Precision>(MaxX, MinY, MinZ);
    case 6: return Vector3<_Precision>(MaxX, MaxY, MinZ);
    case 7: return Vector3<_Precision>(MinX, MaxY, MinZ);
  }

  return Vector3<_Precision>();
}

template <class _Precision>
inline Vector3<_Precision> BoundBox3<_Precision>::CalcCenter (void) const
{
  return Vector3<_Precision>(MinX + (MaxX - MinX) / 2.0f,
                  MinY + (MaxY - MinY) / 2.0f,
                  MinZ + (MaxZ - MinZ) / 2.0f);
}

template <class _Precision>
inline _Precision BoundBox3<_Precision>::CalcDiagonalLength (void) const
{
  return (_Precision)sqrt (((MaxX - MinX) * (MaxX - MinX)) +
                      ((MaxY - MinY) * (MaxY - MinY)) +
                      ((MaxZ - MinZ) * (MaxZ - MinZ)));
}

template <class _Precision>
inline bool BoundBox3<_Precision>::IsCutPlane (const Vector3<_Precision> &rclBase, const Vector3<_Precision> &rclNormal) const
{
  if (fabs(CalcCenter().DistanceToPlane(rclBase, rclNormal)) < CalcDiagonalLength())
  {
    _Precision fD = CalcPoint(0).DistanceToPlane(rclBase, rclNormal);
    for (unsigned short i = 1; i < 8; i++)
    {
      if ((CalcPoint(i).DistanceToPlane(rclBase, rclNormal) * fD) < 0.0f)
        return true;
    }
  }
  return false;
}

template <class _Precision>
inline void BoundBox3<_Precision>::Enlarge (_Precision fLen)
{
  MinX -= fLen; MinY -= fLen; MinZ -= fLen;
  MaxX += fLen; MaxY += fLen; MaxZ += fLen;
}

template <class _Precision>
inline void BoundBox3<_Precision>::Shrink (_Precision fLen)
{
  MinX += fLen; MinY += fLen; MinZ += fLen;
  MaxX -= fLen; MaxY -= fLen; MaxZ -= fLen;
}

template <class _Precision>
inline bool BoundBox3<_Precision>::IsInBox (const Vector3<_Precision> &rcVct) const
{
  return (IS_ON_RAY (MinX, MaxX, rcVct.x) &&
          IS_ON_RAY (MinY, MaxY, rcVct.y) &&
          IS_ON_RAY (MinZ, MaxZ, rcVct.z));
}

template <class _Precision>
inline bool BoundBox3<_Precision>::IsInBox (const BoundBox3<_Precision> &rcBB) const
{
  return (IS_ON_RAY (MinX, MaxX, rcBB.MinX) &&
          IS_ON_RAY (MinX, MaxX, rcBB.MaxX) &&
          IS_ON_RAY (MinY, MaxY, rcBB.MinY) &&
          IS_ON_RAY (MinY, MaxY, rcBB.MaxY) &&
          IS_ON_RAY (MinZ, MaxZ, rcBB.MinZ) &&
          IS_ON_RAY (MinZ, MaxZ, rcBB.MaxZ));
}

template <class _Precision>
inline bool BoundBox3<_Precision>::IsInBox (const BoundBox2D &rcBB) const
{
   return ( IS_ON_RAY (MinX, MaxX, rcBB.fMinX) &&
            IS_ON_RAY (MinX, MaxX, rcBB.fMaxX) &&
            IS_ON_RAY (MinY, MaxY, rcBB.fMinY) &&
            IS_ON_RAY (MinY, MaxY, rcBB.fMaxY) );
}

template <class _Precision>
inline _Precision BoundBox3<_Precision>::LengthX (void) const
{
  return MaxX - MinX;
}

template <class _Precision>
inline _Precision BoundBox3<_Precision>::LengthY (void) const
{
  return MaxY - MinY;
}

template <class _Precision>
inline _Precision BoundBox3<_Precision>::LengthZ (void) const
{
  return MaxZ - MinZ;
}

template <class _Precision>
inline void BoundBox3<_Precision>::MoveX (_Precision f)
{
  MinX += f; MaxX += f;
}

template <class _Precision>
inline void BoundBox3<_Precision>::MoveY (_Precision f)
{
  MinY += f; MaxY += f;
}

template <class _Precision>
inline void BoundBox3<_Precision>::MoveZ (_Precision f)
{
  MinZ += f; MaxZ += f;
}

template <class _Precision>
inline void BoundBox3<_Precision>::ScaleX (_Precision f)
{
  MinX *= f; MaxX *= f;
}

template <class _Precision>
inline void BoundBox3<_Precision>::ScaleY (_Precision f)
{
  MinY *= f; MaxY *= f;
}

template <class _Precision>
inline void BoundBox3<_Precision>::ScaleZ (_Precision f)
{
  MinZ *= f; MaxZ *= f;
}

typedef BoundBox3<float> BoundBox3f;
typedef BoundBox3<double> BoundBox3d;

} // namespace Base


#endif  // BASE_BOUNDBOX_H

