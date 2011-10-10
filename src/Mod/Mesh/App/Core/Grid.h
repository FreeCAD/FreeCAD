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


#ifndef MESH_GRID_H
#define MESH_GRID_H

#include <set>

#include "MeshKernel.h"
#include <Base/Vector3D.h>
#include <Base/BoundBox.h>

#define  MESH_CT_GRID          256     // Default value for number of elements per grid
#define  MESH_MAX_GRIDS        100000  // Default value for maximum number of grids
#define  MESH_CT_GRID_PER_AXIS 20


namespace MeshCore {

class MeshKernel;
class MeshGeomFacet;
class MeshGrid;

//#define MESHGRID_BBOX_EXTENSION 1.0e-3f
#define MESHGRID_BBOX_EXTENSION 10.0f

/**
 * The MeshGrid allows to divide a global mesh object into smaller regions
 * of elements (e.g. facets, points or edges) depending on the resolution
 * of the grid. All grid elements in the grid structure have the same size.
 *
 * Grids can be used within algorithms to avoid to iterate through all elements,
 * so grids can speed up algorithms dramatically.
 */
class MeshExport MeshGrid
{
protected:
  /** @name Construction */
  //@{
  /// Construction
  MeshGrid (const MeshKernel &rclM);
  /// Construction
  MeshGrid (void);
  //@}

public:
  /// Destruction
  virtual ~MeshGrid (void) { }

public:
  /** Attaches the mesh kernel to this grid, an already attached mesh gets detached. The grid gets rebuilt 
   * automatically. */
  virtual void Attach (const MeshKernel &rclM);
  /** Rebuilds the grid structure. */
  virtual void Rebuild (unsigned long ulPerGrid = MESH_CT_GRID, unsigned long ulMaxGrid = MESH_MAX_GRIDS);
  /** Rebuilds the grid structure. */
  virtual void Rebuild (int iCtGridPerAxis = MESH_CT_GRID_PER_AXIS);
  /** Rebuilds the grid structure. */
  virtual void Rebuild (unsigned long ulX, unsigned long ulY, unsigned long ulZ);

  /** @name Search */
  //@{
  /** Searches for elements lying in the intersection area of the grid and the bounding box. */
  virtual unsigned long Inside (const Base::BoundBox3f &rclBB, std::vector<unsigned long> &raulElements, bool bDelDoubles = true) const;
  /** Searches for elements lying in the intersection area of the grid and the bounding box. */
  virtual unsigned long Inside (const Base::BoundBox3f &rclBB, std::set<unsigned long> &raulElementss) const;
  /** Searches for elements lying in the intersection area of the grid and the bounding box. */
  virtual unsigned long Inside (const Base::BoundBox3f &rclBB, std::vector<unsigned long> &raulElements,
                                const Base::Vector3f &rclOrg, float fMaxDist, bool bDelDoubles = true) const;
  /** Searches for the nearest grids that contain elements from a point, the result are grid indices. */
  void SearchNearestFromPoint (const Base::Vector3f &rclPt, std::set<unsigned long> &rclInd) const;
  //@}

  /** @name Getters */
  //@{
  /** Returns the indices of the elements in the given grid. */
  unsigned long GetElements (unsigned long ulX, unsigned long ulY, unsigned long ulZ,  std::set<unsigned long> &raclInd) const;
  unsigned long GetElements (const Base::Vector3f &rclPoint, std::vector<unsigned long>& aulFacets) const;
  //@}

  /** Returns the lengths of the grid elements in x,y and z direction. */
  virtual void  GetGridLengths (float &rfLenX, float &rfLenY, float &rfLenZ) const
  { rfLenX = _fGridLenX; rfLenY = _fGridLenY; rfLenZ = _fGridLenZ; }
  /** Returns the number of grid elements in x,y and z direction. */
  virtual void GetCtGrids (unsigned long &rulX, unsigned long &rulY, unsigned long &rulZ) const
  { rulX = _ulCtGridsX;  rulY = _ulCtGridsY;  rulZ = _ulCtGridsZ; }

  /** @name Boundings */
  //@{
  /** Returns the bounding box of a given grid element. */
  inline Base::BoundBox3f  GetBoundBox (unsigned long ulX, unsigned long ulY, unsigned long ulZ) const;
  /** Returns the bounding box of the whole. */
  inline Base::BoundBox3f  GetBoundBox     (void) const;
  /** Returns an extended bounding box of the mesh object. */
  inline Base::BoundBox3f  GetMeshBoundBox (void) const;
  //@}
  /** Returns an index for the given grid position. If the specified triple is not a valid grid position ULONG_MAX is returned. 
   * If the index is valid than its value is between zero and the number of grid elements. For each different grid position
   * a different index is returned.
   */
  unsigned long GetIndexToPosition(unsigned long ulX, unsigned long ulY, unsigned long ulZ) const;
  /** Returns the grid position to the given index. If the index is equal to or higher than the number of grid elements false is returned
   * and the triple is set to ULONG_MAX. 
   */
  bool GetPositionToIndex(unsigned long id, unsigned long& ulX, unsigned long& ulY, unsigned long& ulZ) const;
  /** Returns the number of elements in a given grid. */
  unsigned long GetCtElements(unsigned long ulX, unsigned long ulY, unsigned long ulZ) const
  { return _aulGrid[ulX][ulY][ulZ].size(); }
  /** Validates the grid structure and rebuilds it if needed. Must be implemented in sub-classes. */
  virtual void Validate (const MeshKernel &rclM) = 0;
  /** Verifies the grid structure and returns false if inconsistencies are found. */
  virtual bool Verify() const = 0;
  /** Checks whether the point is inside the grid. In case it is inside true is returned with the grid position,
   * otherwise false is returned and the grid position is undefined.
   */
  bool CheckPosition (const Base::Vector3f &rclPoint, unsigned long &rulX, unsigned long &rulY, unsigned long &rulZ) const;
  /** Returns the indices of the grid this point lies in. If the point is outside the grid the indices of 
   * the nearest grid element are taken.*/
  virtual void Position (const Base::Vector3f &rclPoint, unsigned long &rulX, unsigned long &rulY, unsigned long &rulZ) const;
  /** Checks if this is a valid grid position. */
  inline bool CheckPos (unsigned long ulX, unsigned long ulY, unsigned long ulZ) const;
  /** Get the indices of all elements lying in the grids around a given grid with distance \a ulDistance. */
  void GetHull (unsigned long ulX, unsigned long ulY, unsigned long ulZ, unsigned long ulDistance, std::set<unsigned long> &raclInd) const;

protected:
  /** Initializes the size of the internal structure. */
  virtual void InitGrid (void);
  /** Deletes the grid structure. */
  virtual void Clear (void);
  /** Calculates the grid length dependent on maximum number of grids. */
  virtual void CalculateGridLength (unsigned long ulCtGrid, unsigned long ulMaxGrids);
  /** Calculates the grid length dependent on the number of grids per axis. */
  virtual void CalculateGridLength (int    iCtGridPerAxis);
  /** Rebuilds the grid structure. Must be implemented in sub-classes. */
  virtual void RebuildGrid (void) = 0;
  /** Returns the number of stored elements. Must be implemented in sub-classes. */
  virtual unsigned long HasElements (void) const = 0;

protected:
  std::vector<std::vector<std::vector<std::set<unsigned long> > > >  _aulGrid;   /**< Grid data structure. */
  const MeshKernel* _pclMesh;     /**< The mesh kernel. */
  unsigned long     _ulCtElements;/**< Number of grid elements for validation issues. */
  unsigned long     _ulCtGridsX;  /**< Number of grid elements in z. */
  unsigned long     _ulCtGridsY;  /**< Number of grid elements in z. */
  unsigned long     _ulCtGridsZ;  /**< Number of grid elements in z. */
  float             _fGridLenX;   /**< Length of grid elements in x. */
  float             _fGridLenY;   /**< Length of grid elements in y. */
  float             _fGridLenZ;   /**< Length of grid elements in z. */
  float             _fMinX;       /**< Grid null position in x. */
  float             _fMinY;       /**< Grid null position in y. */ 
  float             _fMinZ;       /**< Grid null position in z. */

  // friends
  friend class MeshGridIterator;
};

/**
 * Special grid class that stores facet indices of the mesh object
 * in its grids.
 */
class MeshExport MeshFacetGrid: public MeshGrid
{
public:
  /** @name Construction */
  //@{
  /// Construction
  MeshFacetGrid (const MeshKernel &rclM);
  /// Construction
  MeshFacetGrid (void) : MeshGrid() { }
  /// Construction
  MeshFacetGrid (const MeshKernel &rclM, unsigned long ulX, unsigned long ulY, unsigned long ulZ);
  /// Construction
  MeshFacetGrid (const MeshKernel &rclM, int   iCtGridPerAxis);
  /// Construction
  MeshFacetGrid (const MeshKernel &rclM, float fGridLen);
  /// Destruction
  virtual ~MeshFacetGrid (void) { }
  //@}
 
  /** @name Search */
  //@{
  /** Searches for the nearest facet from a point. */
  unsigned long SearchNearestFromPoint (const Base::Vector3f &rclPt) const;
  /** Searches for the nearest facet from a point with the maximum search area. */
  unsigned long SearchNearestFromPoint (const Base::Vector3f &rclPt, float fMaxSearchArea) const;
  /** Searches for the nearest facet in a given grid element and returns the facet index and the actual distance. */
  void SearchNearestFacetInGrid(unsigned long ulX, unsigned long ulY, unsigned long ulZ, const Base::Vector3f &rclPt,
                                float &rfMinDist, unsigned long &rulFacetInd) const;
  /** Does basically the same as the method above unless that grid neighbours upto the order of \a ulDistance
   * are introduced into the search. */
  void SearchNearestFacetInHull (unsigned long ulX, unsigned long ulY, unsigned long ulZ, unsigned long ulDistance, 
                                 const Base::Vector3f &rclPt, unsigned long &rulFacetInd, float &rfMinDist) const;
  //@}

  /** Validates the grid structure and rebuilds it if needed. */
  virtual void Validate (const MeshKernel &rclM);
  /** Validates the grid structure and rebuilds it if needed. */
  virtual void Validate (void);
  /** Verifies the grid structure and returns false if inconsistencies are found. */
  virtual bool Verify() const;

protected:
  /** Returns the grid numbers to the given point \a rclPoint. */
  inline void Pos (const Base::Vector3f &rclPoint, unsigned long &rulX, unsigned long &rulY, unsigned long &rulZ) const;
  /** Returns the grid numbers to the given point \a rclPoint. */
  inline void PosWithCheck (const Base::Vector3f &rclPoint, unsigned long &rulX, unsigned long &rulY, unsigned long &rulZ) const;
  /** Adds a new facet element to the grid structure. \a rclFacet is the geometric facet and \a ulFacetIndex 
   * the corresponding index in the mesh kernel. The facet is added to each grid element that intersects 
   * the facet. */
  inline void AddFacet (const MeshGeomFacet &rclFacet, unsigned long ulFacetIndex, float fEpsilon = 0.0f);
  /** Returns the number of stored elements. */
  unsigned long HasElements (void) const
  { return _pclMesh->CountFacets(); }
  /** Rebuilds the grid structure. */
  virtual void RebuildGrid (void);
};

/**
 * Special grid class that stores point indices of the mesh object
 * in its grids.
 */
class MeshExport MeshPointGrid: public MeshGrid
{
public:
  /** @name Construction */
  //@{
  /// Construction
  MeshPointGrid (void);
  /// Construction
  MeshPointGrid (const MeshKernel &rclM);
  /// Construction
  MeshPointGrid (const MeshKernel &rclM, int   iCtGridPerAxis);
  /// Construction
  MeshPointGrid (const MeshKernel &rclM, float fGridLen);
  /// Construction
  MeshPointGrid (const MeshKernel &rclM, unsigned long ulX, unsigned long ulY, unsigned long ulZ);
  /// Destruction
  virtual ~MeshPointGrid (void) {}
  //@}

  /** Finds all points that lie in the same grid as the point \a rclPoint. */
  unsigned long FindElements(const Base::Vector3f &rclPoint, std::set<unsigned long>& aulElements) const;
  /** Validates the grid structure and rebuilds it if needed. */
  virtual void Validate (const MeshKernel &rclM);
  /** Validates the grid structure and rebuilds it if needed. */
  virtual void Validate (void);
  /** Verifies the grid structure and returns false if inconsistencies are found. */
  virtual bool Verify() const;

protected:
  /** Adds a new point element to the grid structure. \a rclPt is the geometric point and \a ulPtIndex 
   * the corresponding index in the mesh kernel. */
  void AddPoint (const MeshPoint &rclPt, unsigned long ulPtIndex, float fEpsilon = 0.0f);
  /** Returns the grid numbers to the given point \a rclPoint. */
  void Pos(const Base::Vector3f &rclPoint, unsigned long &rulX, unsigned long &rulY, unsigned long &rulZ) const;
  /** Returns the number of stored elements. */
  unsigned long HasElements (void) const
  { return _pclMesh->CountPoints(); }
  /** Rebuilds the grid structure. */
  virtual void RebuildGrid (void);
};

/**
 * The MeshGridIterator class provides an interface to walk through
 * all grid elements of a mesh grid.
 */
class MeshExport MeshGridIterator
{
public:
  /// Construction
  MeshGridIterator (const MeshGrid &rclG);
  /** Returns the bounding box of the current grid element. */
  Base::BoundBox3f GetBoundBox (void) const
  { return _rclGrid.GetBoundBox(_ulX, _ulY, _ulZ); }
  /** Returns indices of the elements in the current grid. */
  void GetElements (std::vector<unsigned long> &raulElements) const
  {
    raulElements.insert(raulElements.end(), _rclGrid._aulGrid[_ulX][_ulY][_ulZ].begin(), _rclGrid._aulGrid[_ulX][_ulY][_ulZ].end());
  }
  /** Returns the number of elements in the current grid. */
  unsigned long GetCtElements() const
  {
    return _rclGrid.GetCtElements(_ulX, _ulY, _ulZ);
  }
  /** @name Iteration */
  //@{
  /** Sets the iterator to the first element*/
  void  Init (void)
  { _ulX = _ulY = _ulZ = 0; }
  /** Checks if the iterator has not yet reached the end position. */  
  bool  More (void) const
  { return (_ulZ < _rclGrid._ulCtGridsZ); }
  /** Go to the next grid. */
  void  Next (void)
  {
    if (++_ulX >= (_rclGrid._ulCtGridsX)) _ulX = 0; else return;
    if (++_ulY >= (_rclGrid._ulCtGridsY)) { _ulY = 0; _ulZ++; } else return;
  }
  //@}

  /** @name Tests with rays */
  //@{
  /** Searches for facets around the ray. */
  bool InitOnRay (const Base::Vector3f &rclPt, const Base::Vector3f &rclDir, std::vector<unsigned long> &raulElements);
  /** Searches for facets around the ray. */
  bool InitOnRay (const Base::Vector3f &rclPt, const Base::Vector3f &rclDir, float fMaxSearchArea, std::vector<unsigned long> &raulElements);
  /** Searches for facets around the ray. */
  bool NextOnRay (std::vector<unsigned long> &raulElements);
  //@}
  
  /** Returns the grid number of the current position. */
  void GetGridPos (unsigned long &rulX, unsigned long &rulY, unsigned long &rulZ) const
  { rulX = _ulX; rulY = _ulY; rulZ = _ulZ; }

protected:
  const MeshGrid& _rclGrid; /**< The mesh kernel. */
  unsigned long   _ulX;     /**< Number of grids in x. */
  unsigned long   _ulY;     /**< Number of grids in y. */
  unsigned long   _ulZ;     /**< Number of grids in z. */ 
  Base::Vector3f        _clPt;    /**< Base point of search ray. */
  Base::Vector3f        _clDir;   /**< Direction of search ray. */
  bool            _bValidRay; /**< Search ray ok? */
  float           _fMaxSearchArea;
  /** Checks if a grid position is already visited by NextOnRay(). */
  struct GridElement 
  {
    GridElement( unsigned long x, unsigned long y, unsigned long z)
    { this->x = x; this->y = y; this->z = z; }
    bool operator < (const GridElement& pos) const
    {
      if ( x == pos.x)
      { if ( y == pos.y) return z < pos.z; else return y < pos.y; }
      else 
      { return x < pos.x; }
    }
  private:
    unsigned long x,y,z;
  };
  std::set<GridElement> _cSearchPositions;
};

// --------------------------------------------------------------

inline Base::BoundBox3f  MeshGrid::GetBoundBox (unsigned long ulX, unsigned long ulY, unsigned long ulZ) const
{
  float fX, fY, fZ;
  
  fX = _fMinX + (float(ulX) * _fGridLenX);
  fY = _fMinY + (float(ulY) * _fGridLenY);
  fZ = _fMinZ + (float(ulZ) * _fGridLenZ);

  return Base::BoundBox3f(fX, fY, fZ, fX + _fGridLenX, fY + _fGridLenY, fZ + _fGridLenZ);
}

inline Base::BoundBox3f  MeshGrid::GetBoundBox (void) const
{
  return Base::BoundBox3f(_fMinX, _fMinY, _fMinZ, _fMinX + (_fGridLenX * float(_ulCtGridsX)),
                    _fMinY + (_fGridLenY * float(_ulCtGridsY)), _fMinZ + (_fGridLenZ * float(_ulCtGridsZ)));
}

inline Base::BoundBox3f  MeshGrid::GetMeshBoundBox (void) const
{
  Base::BoundBox3f clBBenlarged = _pclMesh->GetBoundBox();
  clBBenlarged.Enlarge(MESHGRID_BBOX_EXTENSION);

  return clBBenlarged;
}

inline bool MeshGrid::CheckPos (unsigned long ulX, unsigned long ulY, unsigned long ulZ) const
{
  return ((ulX < _ulCtGridsX) && (ulY < _ulCtGridsY) && (ulZ < _ulCtGridsZ));
}

// --------------------------------------------------------------

inline void MeshFacetGrid::Pos (const Base::Vector3f &rclPoint, unsigned long &rulX, unsigned long &rulY, unsigned long &rulZ) const
{
  rulX = (unsigned long)((rclPoint.x - _fMinX) / _fGridLenX);
  rulY = (unsigned long)((rclPoint.y - _fMinY) / _fGridLenY);
  rulZ = (unsigned long)((rclPoint.z - _fMinZ) / _fGridLenZ);

  assert((rulX < _ulCtGridsX) && (rulY < _ulCtGridsY) && (rulZ < _ulCtGridsZ));
}

inline void MeshFacetGrid::PosWithCheck (const Base::Vector3f &rclPoint, unsigned long &rulX, unsigned long &rulY, unsigned long &rulZ) const
{
  if ( rclPoint.x < _fMinX)
    rulX = 0;
  else
  {
    rulX = (unsigned long)((rclPoint.x - _fMinX) / _fGridLenX);
    if (rulX >= _ulCtGridsX)
      rulX = (_ulCtGridsX-1);
  }

  if ( rclPoint.y < _fMinY)
    rulY = 0;
  else
  {
    rulY = (unsigned long)((rclPoint.y - _fMinY) / _fGridLenY);
    if (rulY >= _ulCtGridsY)
      rulY = (_ulCtGridsY-1);
  }

  if ( rclPoint.z < _fMinZ)
    rulZ = 0;
  else
  {
    rulZ = (unsigned long)((rclPoint.z - _fMinZ) / _fGridLenZ);
    if (rulZ >= _ulCtGridsZ)
      rulZ = (_ulCtGridsZ-1);
  }

  assert((rulX < _ulCtGridsX) && (rulY < _ulCtGridsY) && (rulZ < _ulCtGridsZ));
}

inline void MeshFacetGrid::AddFacet (const MeshGeomFacet &rclFacet, unsigned long ulFacetIndex, float fEpsilon)
{
#if 0
  unsigned long  i, ulX, ulY, ulZ, ulX1, ulY1, ulZ1, ulX2, ulY2, ulZ2;
  
  ulX1 = ulY1 = ulZ1 = ULONG_MAX;
  ulX2 = ulY2 = ulZ2 = 0;

  for (i = 0; i < 3; i++)
  {
    Pos(rclFacet._aclPoints[i], ulX, ulY, ulZ);
    _aulGrid[ulX][ulY][ulZ].insert(ulFacetIndex);
    ulX1 = RSmin<unsigned long>(ulX1, ulX); ulY1 = RSmin<unsigned long>(ulY1, ulY); ulZ1 = RSmin<unsigned long>(ulZ1, ulZ);
    ulX2 = RSmax<unsigned long>(ulX2, ulX); ulY2 = RSmax<unsigned long>(ulY2, ulY); ulZ2 = RSmax<unsigned long>(ulZ2, ulZ);
  }
  
  // falls Facet ueber mehrere BB reicht
  if ((ulX1 < ulX2) || (ulY1 < ulY2) || (ulZ1 < ulZ2))
  {
    for (ulX = ulX1; ulX <= ulX2; ulX++)
    {
      for (ulY = ulY1; ulY <= ulY2; ulY++)
      {
        for (ulZ = ulZ1; ulZ <= ulZ2; ulZ++)
        {
          if (CMeshFacetFunc::BBoxContainFacet(GetBoundBox(ulX, ulY, ulZ), rclFacet) == TRUE)
            _aulGrid[ulX][ulY][ulZ].insert(ulFacetIndex);
        }
      }
    }
  }
#else
  unsigned long ulX, ulY, ulZ;

  unsigned long ulX1, ulY1, ulZ1, ulX2, ulY2, ulZ2;

  Base::BoundBox3f clBB;

  clBB &= rclFacet._aclPoints[0];
  clBB &= rclFacet._aclPoints[1];
  clBB &= rclFacet._aclPoints[2];

  //float fDiagonalLength = clBB.CalcDiagonalLength();

  //clBB.Enlarge(fEpsilon*fDiagonalLength);

  Pos(Base::Vector3f(clBB.MinX,clBB.MinY,clBB.MinZ), ulX1, ulY1, ulZ1);
  Pos(Base::Vector3f(clBB.MaxX,clBB.MaxY,clBB.MaxZ), ulX2, ulY2, ulZ2);
  
  /*
  if (ulX1 > 0) ulX1--;
  if (ulY1 > 0) ulY1--;
  if (ulZ1 > 0) ulZ1--;
  
  if (ulX2 < (_ulCtGridsX-1)) ulX2++;
  if (ulY2 < (_ulCtGridsY-1)) ulY2++;
  if (ulZ2 < (_ulCtGridsZ-1)) ulZ2++;
  */

  // falls Facet ueber mehrere BB reicht
  if ((ulX1 < ulX2) || (ulY1 < ulY2) || (ulZ1 < ulZ2))
  {
    for (ulX = ulX1; ulX <= ulX2; ulX++)
    {
      for (ulY = ulY1; ulY <= ulY2; ulY++)
      {
        for (ulZ = ulZ1; ulZ <= ulZ2; ulZ++)
        {
          if ( rclFacet.IntersectBoundingBox( GetBoundBox(ulX, ulY, ulZ) ) )
            _aulGrid[ulX][ulY][ulZ].insert(ulFacetIndex);
        }
      }
    }
  }
  else
    _aulGrid[ulX1][ulY1][ulZ1].insert(ulFacetIndex);

#endif
}

} // namespace MeshCore

#endif // MESH_GRID_H
