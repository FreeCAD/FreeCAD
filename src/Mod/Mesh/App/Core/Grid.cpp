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
# include <algorithm>
#endif

#include "Grid.h"
#include "Iterator.h"

#include "MeshKernel.h"
#include "Algorithm.h"
#include "Tools.h"

using namespace MeshCore;

MeshGrid::MeshGrid (const MeshKernel &rclM)
: _pclMesh(&rclM),
  _ulCtElements(0),
  _ulCtGridsX(0), _ulCtGridsY(0), _ulCtGridsZ(0),
  _fGridLenX(0.0f), _fGridLenY(0.0f), _fGridLenZ(0.0f),
  _fMinX(0.0f), _fMinY(0.0f), _fMinZ(0.0f)
{
}

MeshGrid::MeshGrid (void)
: _pclMesh(NULL),
  _ulCtElements(0),
  _ulCtGridsX(MESH_CT_GRID), _ulCtGridsY(MESH_CT_GRID), _ulCtGridsZ(MESH_CT_GRID),
  _fGridLenX(0.0f), _fGridLenY(0.0f), _fGridLenZ(0.0f),
  _fMinX(0.0f), _fMinY(0.0f), _fMinZ(0.0f)
{
}

void MeshGrid::Attach (const MeshKernel &rclM)
{
  _pclMesh = &rclM;
  RebuildGrid();
}

void MeshGrid::Clear (void)
{
  _aulGrid.clear();
  _pclMesh = NULL;  
}

void MeshGrid::Rebuild (unsigned long ulX, unsigned long ulY, unsigned long ulZ)
{
  _ulCtGridsX = ulX;
  _ulCtGridsY = ulY;
  _ulCtGridsZ = ulZ;
  _ulCtElements = HasElements();
  RebuildGrid();
}

void MeshGrid::Rebuild (unsigned long ulPerGrid, unsigned long ulMaxGrid)
{
  _ulCtElements = HasElements();
  CalculateGridLength(ulPerGrid, ulMaxGrid);
  RebuildGrid();
}

void MeshGrid::Rebuild (int iCtGridPerAxis)
{
  _ulCtElements = HasElements();
  CalculateGridLength(iCtGridPerAxis);
  RebuildGrid();
}

void MeshGrid::InitGrid (void)
{
  assert(_pclMesh != NULL);

  unsigned long i, j;

  // Grid Laengen berechnen wenn nicht initialisiert
  //
  if ((_ulCtGridsX == 0) || (_ulCtGridsX == 0) || (_ulCtGridsX == 0))
    CalculateGridLength(MESH_CT_GRID, MESH_MAX_GRIDS);

  // Grid Laengen und Offset bestimmen
  //
  {
  Base::BoundBox3f clBBMesh = _pclMesh->GetBoundBox();

  float fLengthX = clBBMesh.LengthX(); 
  float fLengthY = clBBMesh.LengthY();
  float fLengthZ = clBBMesh.LengthZ();

  {
    // Offset fGridLen/2
    //
    _fGridLenX = (1.0f + fLengthX) / float(_ulCtGridsX);
    _fMinX = clBBMesh.MinX - 0.5f;
  }

  {
    _fGridLenY = (1.0f + fLengthY) / float(_ulCtGridsY);
    _fMinY = clBBMesh.MinY - 0.5f;
  }

  {
    _fGridLenZ = (1.0f + fLengthZ) / float(_ulCtGridsZ);
    _fMinZ = clBBMesh.MinZ - 0.5f;
  }
  }

  // Daten-Struktur anlegen
  _aulGrid.clear();
  _aulGrid.resize(_ulCtGridsX);
  for (i = 0; i < _ulCtGridsX; i++)
  {
    _aulGrid[i].resize(_ulCtGridsY);
    for (j = 0; j < _ulCtGridsY; j++)
      _aulGrid[i][j].resize(_ulCtGridsZ);
  }
}

unsigned long MeshGrid::Inside (const Base::BoundBox3f &rclBB, std::vector<unsigned long> &raulElements,
                                bool bDelDoubles) const
{
  unsigned long i, j, k, ulMinX, ulMinY, ulMinZ,  ulMaxX, ulMaxY, ulMaxZ;
  
  raulElements.clear();

  // Grid-Boxen zur naehreren Auswahl
  Position(Base::Vector3f(rclBB.MinX, rclBB.MinY, rclBB.MinZ), ulMinX, ulMinY, ulMinZ);
  Position(Base::Vector3f(rclBB.MaxX, rclBB.MaxY, rclBB.MaxZ), ulMaxX, ulMaxY, ulMaxZ);

  for (i = ulMinX; i <= ulMaxX; i++)
  {
    for (j = ulMinY; j <= ulMaxY; j++)
    {
      for (k = ulMinZ; k <= ulMaxZ; k++)
      {
        raulElements.insert(raulElements.end(), _aulGrid[i][j][k].begin(), _aulGrid[i][j][k].end());
      }
    }
  }  

  if (bDelDoubles == true)
  {
    // doppelte Nennungen entfernen
    std::sort(raulElements.begin(), raulElements.end());
    raulElements.erase(std::unique(raulElements.begin(), raulElements.end()), raulElements.end());  
  }

  return raulElements.size();
}

unsigned long MeshGrid::Inside (const Base::BoundBox3f &rclBB, std::vector<unsigned long> &raulElements,
                                const Base::Vector3f &rclOrg, float fMaxDist, bool bDelDoubles) const
{
  unsigned long i, j, k, ulMinX, ulMinY, ulMinZ,  ulMaxX, ulMaxY, ulMaxZ;
  float  fGridDiag  = GetBoundBox(0, 0, 0).CalcDiagonalLength();
  float  fMinDistP2 = (fGridDiag * fGridDiag) + (fMaxDist * fMaxDist);

  raulElements.clear();

  // Grid-Boxen zur naehreren Auswahl
  Position(Base::Vector3f(rclBB.MinX, rclBB.MinY, rclBB.MinZ), ulMinX, ulMinY, ulMinZ);
  Position(Base::Vector3f(rclBB.MaxX, rclBB.MaxY, rclBB.MaxZ), ulMaxX, ulMaxY, ulMaxZ);

  for (i = ulMinX; i <= ulMaxX; i++)
  {
    for (j = ulMinY; j <= ulMaxY; j++)
    {
      for (k = ulMinZ; k <= ulMaxZ; k++)
      {
        if (Base::DistanceP2(GetBoundBox(i, j, k).CalcCenter(), rclOrg) < fMinDistP2)
          raulElements.insert(raulElements.end(), _aulGrid[i][j][k].begin(), _aulGrid[i][j][k].end());
      }
    }
  }  

  if (bDelDoubles == true)
  {
    // doppelte Nennungen entfernen
    std::sort(raulElements.begin(), raulElements.end());
    raulElements.erase(std::unique(raulElements.begin(), raulElements.end()), raulElements.end());  
  }

  return raulElements.size();
}

unsigned long MeshGrid::Inside (const Base::BoundBox3f &rclBB, std::set<unsigned long> &raulElements) const
{
  unsigned long i, j, k, ulMinX, ulMinY, ulMinZ,  ulMaxX, ulMaxY, ulMaxZ;
  
  raulElements.clear();

  // Grid-Boxen zur naehreren Auswahl
  Position(Base::Vector3f(rclBB.MinX, rclBB.MinY, rclBB.MinZ), ulMinX, ulMinY, ulMinZ);
  Position(Base::Vector3f(rclBB.MaxX, rclBB.MaxY, rclBB.MaxZ), ulMaxX, ulMaxY, ulMaxZ);

  for (i = ulMinX; i <= ulMaxX; i++)
  {
    for (j = ulMinY; j <= ulMaxY; j++)
    {
      for (k = ulMinZ; k <= ulMaxZ; k++)
      {
        raulElements.insert(_aulGrid[i][j][k].begin(), _aulGrid[i][j][k].end());
      }
    }
  }  

  return raulElements.size();
}

bool MeshGrid::CheckPosition (const Base::Vector3f &rclPoint, unsigned long &rulX, unsigned long &rulY, unsigned long &rulZ) const
{
  rulX = (unsigned long)((rclPoint.x - _fMinX) / _fGridLenX);
  rulY = (unsigned long)((rclPoint.y - _fMinY) / _fGridLenY);
  rulZ = (unsigned long)((rclPoint.z - _fMinZ) / _fGridLenZ);

  if ( (rulX < _ulCtGridsX) && (rulY < _ulCtGridsY) && (rulZ < _ulCtGridsZ) )
    return true;

  return false;
}

void MeshGrid::Position (const Base::Vector3f &rclPoint, unsigned long &rulX, unsigned long &rulY, unsigned long &rulZ) const
{
  if (rclPoint.x <= _fMinX)
    rulX = 0;
  else
    rulX = std::min<unsigned long>((unsigned long)((rclPoint.x - _fMinX) / _fGridLenX), _ulCtGridsX - 1);

  if (rclPoint.y <= _fMinY)
    rulY = 0;
  else
    rulY = std::min<unsigned long>((unsigned long)((rclPoint.y - _fMinY) / _fGridLenY), _ulCtGridsY - 1);

  if (rclPoint.z <= _fMinZ)
    rulZ = 0;
  else
    rulZ = std::min<unsigned long>((unsigned long)((rclPoint.z - _fMinZ) / _fGridLenZ), _ulCtGridsZ - 1);
}

void MeshGrid::CalculateGridLength (unsigned long ulCtGrid, unsigned long ulMaxGrids)
{
  // Grid Laengen bzw. Anzahl der Grids pro Dimension berechnen
  // pro Grid sollen ca. 10 (?!?!) Facets liegen
  // bzw. max Grids sollten 10000 nicht ueberschreiten
  Base::BoundBox3f clBBMeshEnlarged = _pclMesh->GetBoundBox();
  float fVolElem;

  if (_ulCtElements > (ulMaxGrids * ulCtGrid))
    fVolElem = (clBBMeshEnlarged.LengthX() * clBBMeshEnlarged.LengthY() * clBBMeshEnlarged.LengthZ()) / float(ulMaxGrids * ulCtGrid);
  else
    fVolElem = (clBBMeshEnlarged.LengthX() * clBBMeshEnlarged.LengthY() * clBBMeshEnlarged.LengthZ()) / float(_ulCtElements);

  float fVol     = fVolElem * float(ulCtGrid);
  float fGridLen = float(pow((float)fVol,(float) 1.0f / 3.0f));

  _ulCtGridsX = std::max<unsigned long>((unsigned long)(clBBMeshEnlarged.LengthX() / fGridLen), 1);
  _ulCtGridsY = std::max<unsigned long>((unsigned long)(clBBMeshEnlarged.LengthY() / fGridLen), 1);
  _ulCtGridsZ = std::max<unsigned long>((unsigned long)(clBBMeshEnlarged.LengthZ() / fGridLen), 1);
}

void MeshGrid::CalculateGridLength (int iCtGridPerAxis)
{
  if (iCtGridPerAxis<=0)
  {
    CalculateGridLength(MESH_CT_GRID, MESH_MAX_GRIDS);
    return;
  }

  // Grid Laengen bzw. Anzahl der Grids pro Dimension berechnen
  // pro Grid sollen ca. 10 (?!?!) Facets liegen
  // bzw. max Grids sollten 10000 nicht ueberschreiten
  Base::BoundBox3f clBBMesh = _pclMesh->GetBoundBox();

  float fLenghtX = clBBMesh.LengthX();
  float fLenghtY = clBBMesh.LengthY();
  float fLenghtZ = clBBMesh.LengthZ();

  float fLenghtD = clBBMesh.CalcDiagonalLength();

  float fLengthTol = 0.05f * fLenghtD;

  bool bLenghtXisZero = (fLenghtX <= fLengthTol);
  bool bLenghtYisZero = (fLenghtY <= fLengthTol);
  bool bLenghtZisZero = (fLenghtZ <= fLengthTol);

  int iFlag  = 0;

  int iMaxGrids = 1;

  if (bLenghtXisZero)  
    iFlag += 1; 
  else
    iMaxGrids *= iCtGridPerAxis;

  if (bLenghtYisZero) 
    iFlag += 2;
  else
    iMaxGrids *= iCtGridPerAxis;

  if (bLenghtZisZero)
    iFlag += 4; 
  else
    iMaxGrids *= iCtGridPerAxis;
  
  unsigned long ulGridsFacets =   10;

  float fFactorVolumen = 40.0;
  float fFactorArea    = 10.0;

  switch (iFlag)
  {
  case 0:
    {
      float fVolumen = fLenghtX * fLenghtY * fLenghtZ;

      float fVolumenGrid = (fVolumen * ulGridsFacets) / (fFactorVolumen * _ulCtElements);

      if ((fVolumenGrid * iMaxGrids) < fVolumen)
        fVolumenGrid = fVolumen / (float)iMaxGrids;

      float fLengthGrid = float(pow((float)fVolumenGrid,(float) 1.0f / 3.0f));

      _ulCtGridsX = std::max<unsigned long>((unsigned long)(fLenghtX / fLengthGrid), 1);
      _ulCtGridsY = std::max<unsigned long>((unsigned long)(fLenghtY / fLengthGrid), 1);
      _ulCtGridsZ = std::max<unsigned long>((unsigned long)(fLenghtZ / fLengthGrid), 1);
      
    } break;
  case 1:
    {
      _ulCtGridsX = 1; // bLenghtXisZero
      
      float fArea = fLenghtY * fLenghtZ;

      float fAreaGrid = (fArea * ulGridsFacets) / (fFactorArea * _ulCtElements);

      if ((fAreaGrid * iMaxGrids) < fArea)
        fAreaGrid = fArea / (float)iMaxGrids;

      float fLengthGrid = float(sqrt(fAreaGrid));

      _ulCtGridsY = std::max<unsigned long>((unsigned long)(fLenghtY / fLengthGrid), 1);
      _ulCtGridsZ = std::max<unsigned long>((unsigned long)(fLenghtZ / fLengthGrid), 1);
    } break;
  case 2:
    {
      _ulCtGridsY = 1; // bLenghtYisZero
  
      float fArea = fLenghtX * fLenghtZ;

      float fAreaGrid = (fArea * ulGridsFacets) / (fFactorArea * _ulCtElements);

      if ((fAreaGrid * iMaxGrids) < fArea)
        fAreaGrid = fArea / (float)iMaxGrids;

      float fLengthGrid = float(sqrt(fAreaGrid));

      _ulCtGridsX = std::max<unsigned long>((unsigned long)(fLenghtX / fLengthGrid), 1);
      _ulCtGridsZ = std::max<unsigned long>((unsigned long)(fLenghtZ / fLengthGrid), 1);
    } break;
  case 3:
    {
      _ulCtGridsX = 1; // bLenghtXisZero
      _ulCtGridsY = 1; // bLenghtYisZero
      _ulCtGridsZ = iMaxGrids; // bLenghtYisZero
    } break;
  case 4:
    {
      _ulCtGridsZ = 1; // bLenghtZisZero
      
      float fArea = fLenghtX * fLenghtY;

      float fAreaGrid = (fArea * ulGridsFacets) / (fFactorArea * _ulCtElements);

      if ((fAreaGrid * iMaxGrids) < fArea)
        fAreaGrid = fArea / (float)iMaxGrids;

      float fLengthGrid = float(sqrt(fAreaGrid));

      _ulCtGridsX = std::max<unsigned long>((unsigned long)(fLenghtX / fLengthGrid), 1);
      _ulCtGridsY = std::max<unsigned long>((unsigned long)(fLenghtY / fLengthGrid), 1);
    } break;
  case 5:
    {
      _ulCtGridsX = 1; // bLenghtXisZero
      _ulCtGridsZ = 1; // bLenghtZisZero
      _ulCtGridsY = iMaxGrids; // bLenghtYisZero
    } break;
  case 6:
    {
      _ulCtGridsY = 1; // bLenghtYisZero
      _ulCtGridsZ = 1; // bLenghtZisZero
      _ulCtGridsX = iMaxGrids; // bLenghtYisZero
    } break;
  case 7:
    {
      _ulCtGridsX = iMaxGrids; // bLenghtXisZero
      _ulCtGridsY = iMaxGrids; // bLenghtYisZero
      _ulCtGridsZ = iMaxGrids; // bLenghtZisZero
    } break;
  }
}

void MeshGrid::SearchNearestFromPoint (const Base::Vector3f &rclPt, std::set<unsigned long> &raclInd) const
{
  raclInd.clear();
  Base::BoundBox3f  clBB = GetBoundBox();

  if (clBB.IsInBox(rclPt) == true)
  { // Punkt liegt innerhalb
    unsigned long ulX, ulY, ulZ;
    Position(rclPt, ulX, ulY, ulZ);
    //int nX = ulX, nY = ulY, nZ = ulZ;
    unsigned long ulMaxLevel = std::max<unsigned long>(_ulCtGridsX, std::max<unsigned long>(_ulCtGridsY, _ulCtGridsZ));
    unsigned long ulLevel = 0;
    while (raclInd.empty() && ulLevel <= ulMaxLevel)
      GetHull(ulX, ulY, ulZ, ulLevel++, raclInd);
    GetHull(ulX, ulY, ulZ, ulLevel, raclInd);
  }
  else
  { // Punkt ausserhalb
    Base::BoundBox3f::SIDE tSide = clBB.GetSideFromRay(rclPt, clBB.CalcCenter() - rclPt);
    switch (tSide)
    {
      case Base::BoundBox3f::RIGHT:
      {
        unsigned long nX = 0;
        while (raclInd.empty() && nX < _ulCtGridsX)
        {
          for (unsigned long i = 0; i < _ulCtGridsY; i++)
          {
            for (unsigned long j = 0; j < _ulCtGridsZ; j++)
              raclInd.insert(_aulGrid[nX][i][j].begin(), _aulGrid[nX][i][j].end());
          }
          nX++;
        }
        break;
      }
      case Base::BoundBox3f::LEFT:
      {
        int nX = _ulCtGridsX - 1;
        while (raclInd.empty() && nX >= 0)
        {
          for (unsigned long i = 0; i < _ulCtGridsY; i++)
          {
            for (unsigned long j = 0; j < _ulCtGridsZ; j++)
              raclInd.insert(_aulGrid[nX][i][j].begin(), _aulGrid[nX][i][j].end());
          }
          nX++;
        }
        break;
      }
      case Base::BoundBox3f::TOP:
      {
        unsigned long nY = 0;
        while (raclInd.empty() && nY < _ulCtGridsY)
        {
          for (unsigned long i = 0; i < _ulCtGridsX; i++)
          {
            for (unsigned long j = 0; j < _ulCtGridsZ; j++)
              raclInd.insert(_aulGrid[i][nY][j].begin(), _aulGrid[i][nY][j].end());
          }
          nY++;
        }
        break;
      }
      case Base::BoundBox3f::BOTTOM:
      {
        int nY = _ulCtGridsY - 1;
        while (raclInd.empty() && nY >= 0)
        {
          for (unsigned long i = 0; i < _ulCtGridsX; i++)
          {
            for (unsigned long j = 0; j < _ulCtGridsZ; j++)
              raclInd.insert(_aulGrid[i][nY][j].begin(), _aulGrid[i][nY][j].end());
          }
          nY--;
        }
        break;
      }
      case Base::BoundBox3f::BACK:
      {
        unsigned long nZ = 0;
        while (raclInd.empty() && nZ < _ulCtGridsZ)
        {
          for (unsigned long i = 0; i < _ulCtGridsX; i++)
          {
            for (unsigned long j = 0; j < _ulCtGridsY; j++)
              raclInd.insert(_aulGrid[i][j][nZ].begin(), _aulGrid[i][j][nZ].end());
          }
          nZ++;
        }
        break;
      }
      case Base::BoundBox3f::FRONT:
      {
        int nZ = _ulCtGridsZ - 1;
        while (raclInd.empty() && nZ >= 0)
        {
          for (unsigned long i = 0; i < _ulCtGridsX; i++)
          {
            for (unsigned long j = 0; j < _ulCtGridsY; j++)
              raclInd.insert(_aulGrid[i][j][nZ].begin(), _aulGrid[i][j][nZ].end());
          }
          nZ--;
        }
        break;
      }

      default:
        break;
    }
  }
}

void MeshGrid::GetHull (unsigned long ulX, unsigned long ulY, unsigned long ulZ, 
                        unsigned long ulDistance, std::set<unsigned long> &raclInd) const
{
  int nX1 = std::max<int>(0, int(ulX) - int(ulDistance));
  int nY1 = std::max<int>(0, int(ulY) - int(ulDistance));
  int nZ1 = std::max<int>(0, int(ulZ) - int(ulDistance));
  int nX2 = std::min<int>(int(_ulCtGridsX) - 1, int(ulX) + int(ulDistance));
  int nY2 = std::min<int>(int(_ulCtGridsY) - 1, int(ulY) + int(ulDistance));
  int nZ2 = std::min<int>(int(_ulCtGridsZ) - 1, int(ulZ) + int(ulDistance));

  int i, j;

  // top plane
  for (i = nX1; i <= nX2; i++)
  {
    for (j = nY1; j <= nY2; j++)
      GetElements(i, j, nZ1, raclInd);
  }
  // bottom plane
  for (i = nX1; i <= nX2; i++)
  {
    for (j = nY1; j <= nY2; j++)
      GetElements(i, j, nZ2, raclInd);
  }
  // left plane
  for (i = nY1; i <= nY2; i++)
  {
    for (j = (nZ1+1); j <= (nZ2-1); j++)
      GetElements(nX1, i, j, raclInd);
  }
  // right plane
  for (i = nY1; i <= nY2; i++)
  {
    for (j = (nZ1+1); j <= (nZ2-1); j++)
      GetElements(nX2, i, j, raclInd);
  }
  // front plane
  for (i = (nX1+1); i <= (nX2-1); i++)
  {
    for (j = (nZ1+1); j <= (nZ2-1); j++)
      GetElements(i, nY1, j, raclInd);
  }
  // back plane
  for (i = (nX1+1); i <= (nX2-1); i++)
  {
    for (j = (nZ1+1); j <= (nZ2-1); j++)
      GetElements(i, nY2, j, raclInd);
  }
}

unsigned long MeshGrid::GetElements (unsigned long ulX, unsigned long ulY, unsigned long ulZ,  
                                     std::set<unsigned long> &raclInd) const
{
  const std::set<unsigned long> &rclSet = _aulGrid[ulX][ulY][ulZ];
  if (rclSet.size() > 0)
  {
    raclInd.insert(rclSet.begin(), rclSet.end());
    return rclSet.size();
  }

  return 0;
}

unsigned long MeshGrid::GetElements(const Base::Vector3f &rclPoint, std::vector<unsigned long>& aulFacets) const
{
  unsigned long ulX, ulY, ulZ;
  if (!CheckPosition(rclPoint, ulX, ulY, ulZ))
    return 0;

  aulFacets.resize(_aulGrid[ulX][ulY][ulZ].size());

  std::copy(_aulGrid[ulX][ulY][ulZ].begin(), _aulGrid[ulX][ulY][ulZ].end(), aulFacets.begin());
  return aulFacets.size();
}

unsigned long MeshGrid::GetIndexToPosition(unsigned long ulX, unsigned long ulY, unsigned long ulZ) const
{
  if ( !CheckPos(ulX, ulY, ulZ) )
    return ULONG_MAX;
  return (ulZ * _ulCtGridsY + ulY) * _ulCtGridsX + ulX;
}

bool MeshGrid::GetPositionToIndex(unsigned long id, unsigned long& ulX, unsigned long& ulY, unsigned long& ulZ) const
{
  ulX = id % _ulCtGridsX;
  ulY = (id/_ulCtGridsX)%_ulCtGridsY;
  ulZ = id/(_ulCtGridsX*_ulCtGridsY);

  if ( !CheckPos(ulX, ulY, ulZ) )
  {
    ulX = ULONG_MAX;
    ulY = ULONG_MAX;
    ulZ = ULONG_MAX;
    return false;
  }

  return true;
}

// ----------------------------------------------------------------

MeshFacetGrid::MeshFacetGrid (const MeshKernel &rclM)
: MeshGrid(rclM)
{
  RebuildGrid();
}

MeshFacetGrid::MeshFacetGrid (const MeshKernel &rclM, int iCtGridPerAxis)
: MeshGrid(rclM)
{
  Rebuild(iCtGridPerAxis);
}

MeshFacetGrid::MeshFacetGrid (const MeshKernel &rclM, unsigned long ulX, unsigned long ulY, unsigned long ulZ)
: MeshGrid(rclM)
{
  Rebuild(ulX, ulY, ulZ);
}

MeshFacetGrid::MeshFacetGrid (const MeshKernel &rclM, float fGridLen)
: MeshGrid(rclM)
{
  Base::BoundBox3f clBBMesh = _pclMesh->GetBoundBox();
  Rebuild(std::max<unsigned long>((unsigned long)(clBBMesh.LengthX() / fGridLen), 1),
          std::max<unsigned long>((unsigned long)(clBBMesh.LengthY() / fGridLen), 1),
          std::max<unsigned long>((unsigned long)(clBBMesh.LengthZ() / fGridLen), 1));
}

void MeshFacetGrid::Validate (const MeshKernel &rclMesh)
{
  if (_pclMesh != &rclMesh)
    Attach(rclMesh);
  else if (rclMesh.CountFacets() != _ulCtElements)
    RebuildGrid();
}

void MeshFacetGrid::Validate (void)
{
  if (_pclMesh == NULL)
    return;

  if (_pclMesh->CountFacets() != _ulCtElements)
    RebuildGrid();
}

bool MeshFacetGrid::Verify() const
{
  if ( !_pclMesh ) 
    return false; // no mesh attached
  if (_pclMesh->CountFacets() != _ulCtElements)
    return false; // not up-to-date

  MeshGridIterator it(*this);
  MeshFacetIterator cF(*_pclMesh);
  for ( it.Init(); it.More(); it.Next() )
  {
    std::vector<unsigned long> aulElements;
    it.GetElements( aulElements );
    for ( std::vector<unsigned long>::iterator itF = aulElements.begin(); itF != aulElements.end(); ++itF )
    {
      cF.Set( *itF );
      if ( cF->IntersectBoundingBox( it.GetBoundBox() ) == false )
        return false; // no intersection between facet although the facet is in grid
    }
  }

  return true;
}

void MeshFacetGrid::RebuildGrid (void)
{
  _ulCtElements = _pclMesh->CountFacets();

  InitGrid();
 
  // Daten-Struktur fuellen
  MeshFacetIterator clFIter(*_pclMesh);

  unsigned long i = 0;
  for (clFIter.Init(); clFIter.More(); clFIter.Next())
  {
//    AddFacet(*clFIter, i++, 2.0f);
    AddFacet(*clFIter, i++);
  }

}

unsigned long MeshFacetGrid::SearchNearestFromPoint (const Base::Vector3f &rclPt) const
{
  unsigned long ulFacetInd = ULONG_MAX;
  float fMinDist    = FLOAT_MAX;
  Base::BoundBox3f  clBB = GetBoundBox();

  if (clBB.IsInBox(rclPt) == true)
  { // Punkt liegt innerhalb
    unsigned long ulX, ulY, ulZ;
    Position(rclPt, ulX, ulY, ulZ);
    float fMinGridDist = std::min<float>(std::min<float>(_fGridLenX, _fGridLenY), _fGridLenZ);
    unsigned long ulDistance = 0;
    while (fMinDist > (fMinGridDist * float(ulDistance)))
    {
      SearchNearestFacetInHull(ulX, ulY, ulZ, ulDistance, rclPt, ulFacetInd, fMinDist);
      ulDistance++;
    }
    SearchNearestFacetInHull(ulX, ulY, ulZ, ulDistance, rclPt, ulFacetInd, fMinDist);
  }
  else
  { // Punkt ausserhalb
    Base::BoundBox3f::SIDE tSide = clBB.GetSideFromRay(rclPt, clBB.CalcCenter() - rclPt);
    switch (tSide)
    {
      case Base::BoundBox3f::RIGHT:
      {
        int nX = 0;
        while ((fMinDist > ((clBB.MinX - rclPt.x) + ((float)(nX) * _fGridLenX))) && ((unsigned long)(nX) < _ulCtGridsX))
        {
          for (unsigned long i = 0; i < _ulCtGridsY; i++)
          {
            for (unsigned long j = 0; j < _ulCtGridsZ; j++)
              SearchNearestFacetInGrid(nX, i, j, rclPt, fMinDist, ulFacetInd);
          }
          nX++;
        }
        break;
      }
      case Base::BoundBox3f::LEFT:
      {
        int nX = _ulCtGridsX - 1;
        while ((fMinDist > ((rclPt.x - clBB.MinX) + (float(nX) * _fGridLenX))) && (nX >= 0))
        {
          for (unsigned long i = 0; i < _ulCtGridsY; i++)
          {
            for (unsigned long j = 0; j < _ulCtGridsZ; j++)
              SearchNearestFacetInGrid(nX, i, j, rclPt, fMinDist, ulFacetInd);
          }
          nX--;
        }
        break;
      }
      case Base::BoundBox3f::TOP:
      {
        int nY = 0;
        while ((fMinDist > ((clBB.MinY - rclPt.y) + ((float)(nY) * _fGridLenY))) && ((unsigned long)(nY) < _ulCtGridsY))
        {
          for (unsigned long i = 0; i < _ulCtGridsX; i++)
          {
            for (unsigned long j = 0; j < _ulCtGridsZ; j++)
              SearchNearestFacetInGrid(i, nY, j, rclPt, fMinDist, ulFacetInd);
          }
          nY++;
        }
        break;
      }
      case Base::BoundBox3f::BOTTOM:
      {
        int nY = _ulCtGridsY - 1;
        while ((fMinDist > ((rclPt.y - clBB.MinY) + (float(nY) * _fGridLenY))) && (nY >= 0))
        {
          for (unsigned long i = 0; i < _ulCtGridsX; i++)
          {
            for (unsigned long j = 0; j < _ulCtGridsZ; j++)
              SearchNearestFacetInGrid(i, nY, j, rclPt, fMinDist, ulFacetInd);
          }
          nY--;
        }
        break;
      }
      case Base::BoundBox3f::BACK:
      {
        int nZ = 0;
        while ((fMinDist > ((clBB.MinZ - rclPt.z) + ((float)(nZ) * _fGridLenZ))) && ((unsigned long)(nZ) < _ulCtGridsZ))
        {
          for (unsigned long i = 0; i < _ulCtGridsX; i++)
          {
            for (unsigned long j = 0; j < _ulCtGridsY; j++)
              SearchNearestFacetInGrid(i, j, nZ, rclPt, fMinDist, ulFacetInd);
          }
          nZ++;
        }
        break;
      }
      case Base::BoundBox3f::FRONT:
      {
        int nZ = _ulCtGridsZ - 1;
        while ((fMinDist > ((rclPt.z - clBB.MinZ) + ((float)(nZ) * _fGridLenZ))) && (nZ >= 0))
        {
          for (unsigned long i = 0; i < _ulCtGridsX; i++)
          {
            for (unsigned long j = 0; j < _ulCtGridsY; j++)
              SearchNearestFacetInGrid(i, j, nZ, rclPt, fMinDist, ulFacetInd);
          }
          nZ--;
        }
        break;
      }
      default:
        break;
    }
  }

  return ulFacetInd;
}

unsigned long MeshFacetGrid::SearchNearestFromPoint (const Base::Vector3f &rclPt, float fMaxSearchArea) const
{
  std::vector<unsigned long> aulFacets;
  unsigned long ulFacetInd = ULONG_MAX;
  float fMinDist   = fMaxSearchArea;

  MeshAlgorithm clFTool(*_pclMesh);

  Base::BoundBox3f clBB(rclPt.x - fMaxSearchArea, rclPt.y - fMaxSearchArea, rclPt.z - fMaxSearchArea,
           rclPt.x + fMaxSearchArea, rclPt.y + fMaxSearchArea, rclPt.z + fMaxSearchArea);

  Inside(clBB, aulFacets, rclPt, fMaxSearchArea, true);

  for (std::vector<unsigned long>::const_iterator pI = aulFacets.begin(); pI != aulFacets.end(); pI++)
  {    
    float fDist;

    if (clFTool.Distance(rclPt, *pI, fMinDist, fDist) == true)
    {
      fMinDist   = fDist;
      ulFacetInd = *pI;
    }
  }

  return ulFacetInd;
}

void MeshFacetGrid::SearchNearestFacetInHull (unsigned long ulX, unsigned long ulY, unsigned long ulZ, 
                                              unsigned long ulDistance, const Base::Vector3f &rclPt,
                                              unsigned long &rulFacetInd, float &rfMinDist) const
{
  int nX1 = std::max<int>(0, int(ulX) - int(ulDistance));
  int nY1 = std::max<int>(0, int(ulY) - int(ulDistance));
  int nZ1 = std::max<int>(0, int(ulZ) - int(ulDistance));
  int nX2 = std::min<int>(int(_ulCtGridsX) - 1, int(ulX) + int(ulDistance));
  int nY2 = std::min<int>(int(_ulCtGridsY) - 1, int(ulY) + int(ulDistance));
  int nZ2 = std::min<int>(int(_ulCtGridsZ) - 1, int(ulZ) + int(ulDistance));

  int i, j;

  // top plane
  for (i = nX1; i <= nX2; i++)
  {
    for (j = nY1; j <= nY2; j++)
      SearchNearestFacetInGrid(i, j, nZ1, rclPt, rfMinDist, rulFacetInd);
  }
  // bottom plane
  for (i = nX1; i <= nX2; i++)
  {
    for (j = nY1; j <= nY2; j++)
      SearchNearestFacetInGrid(i, j, nZ2, rclPt, rfMinDist, rulFacetInd);
  }
  // left plane
  for (i = nY1; i <= nY2; i++)
  {
    for (j = (nZ1+1); j <= (nZ2-1); j++)
      SearchNearestFacetInGrid(nX1, i, j, rclPt, rfMinDist, rulFacetInd);
  }
  // right plane
  for (i = nY1; i <= nY2; i++)
  {
    for (j = (nZ1+1); j <= (nZ2-1); j++)
      SearchNearestFacetInGrid(nX2, i, j, rclPt, rfMinDist, rulFacetInd);
  }
  // front plane
  for (i = (nX1+1); i <= (nX2-1); i++)
  {
    for (j = (nZ1+1); j <= (nZ2-1); j++)
      SearchNearestFacetInGrid(i, nY1, j, rclPt, rfMinDist, rulFacetInd);
  }
  // back plane
  for (i = (nX1+1); i <= (nX2-1); i++)
  {
    for (j = (nZ1+1); j <= (nZ2-1); j++)
      SearchNearestFacetInGrid(i, nY2, j, rclPt, rfMinDist, rulFacetInd);
  }
}

void MeshFacetGrid::SearchNearestFacetInGrid(unsigned long ulX, unsigned long ulY, unsigned long ulZ, 
                                             const Base::Vector3f &rclPt, float &rfMinDist,
                                             unsigned long &rulFacetInd) const
{
  const std::set<unsigned long> &rclSet = _aulGrid[ulX][ulY][ulZ];
  for (std::set<unsigned long>::const_iterator pI = rclSet.begin(); pI != rclSet.end(); pI++)
  {
    float fDist = _pclMesh->GetFacet(*pI).DistanceToPoint(rclPt);
    if (fDist < rfMinDist)
    {
      rfMinDist   = fDist;
      rulFacetInd = *pI;
    }
  }
}

//----------------------------------------------------------------------------

MeshPointGrid::MeshPointGrid (const MeshKernel &rclM) 
 : MeshGrid(rclM) 
{
  RebuildGrid();
}

MeshPointGrid::MeshPointGrid (void)
 : MeshGrid() 
{
}

MeshPointGrid::MeshPointGrid (const MeshKernel &rclM, unsigned long ulX, unsigned long ulY, unsigned long ulZ)
 : MeshGrid(rclM) 
{
  Rebuild(ulX, ulY, ulZ);
}

MeshPointGrid::MeshPointGrid (const MeshKernel &rclM, int   iCtGridPerAxis)
  : MeshGrid(rclM) 
{
  Rebuild(iCtGridPerAxis);
}

MeshPointGrid::MeshPointGrid (const MeshKernel &rclM, float fGridLen)
  : MeshGrid(rclM)
{
  Base::BoundBox3f clBBMesh = _pclMesh->GetBoundBox();
  Rebuild(std::max<unsigned long>((unsigned long)(clBBMesh.LengthX() / fGridLen), 1),
          std::max<unsigned long>((unsigned long)(clBBMesh.LengthY() / fGridLen), 1),
          std::max<unsigned long>((unsigned long)(clBBMesh.LengthZ() / fGridLen), 1));
}

void MeshPointGrid::AddPoint (const MeshPoint &rclPt, unsigned long ulPtIndex, float fEpsilon)
{
  unsigned long ulX, ulY, ulZ;
  Pos(Base::Vector3f(rclPt.x, rclPt.y, rclPt.z), ulX, ulY, ulZ);
  if ( (ulX < _ulCtGridsX) && (ulY < _ulCtGridsY) && (ulZ < _ulCtGridsZ) )
    _aulGrid[ulX][ulY][ulZ].insert(ulPtIndex);
}

void MeshPointGrid::Validate (const MeshKernel &rclMesh)
{
  if (_pclMesh != &rclMesh)
    Attach(rclMesh);
  else if (rclMesh.CountPoints() != _ulCtElements)
    RebuildGrid();
}

void MeshPointGrid::Validate (void)
{
  if (_pclMesh == NULL)
    return;

  if (_pclMesh->CountPoints() != _ulCtElements)
    RebuildGrid();
}

bool MeshPointGrid::Verify() const
{
  if ( !_pclMesh ) 
    return false; // no mesh attached
  if (_pclMesh->CountFacets() != _ulCtElements)
    return false; // not up-to-date

  MeshGridIterator it(*this);
  MeshPointIterator cP(*_pclMesh);
  for ( it.Init(); it.More(); it.Next() )
  {
    std::vector<unsigned long> aulElements;
    it.GetElements( aulElements );
    for ( std::vector<unsigned long>::iterator itP = aulElements.begin(); itP != aulElements.end(); ++itP )
    {
      cP.Set( *itP );
      if ( it.GetBoundBox().IsInBox( *cP ) == false )
        return false; // point doesn't lie inside the grid element
    }
  }

  return true;
}

void MeshPointGrid::RebuildGrid (void)
{
  _ulCtElements = _pclMesh->CountPoints();

  InitGrid();
 
  // Daten-Struktur fuellen

  MeshPointIterator cPIter(*_pclMesh);

  unsigned long i = 0;
  for (cPIter.Init(); cPIter.More(); cPIter.Next())
  {
    AddPoint(*cPIter, i++);
  }
}

void MeshPointGrid::Pos (const Base::Vector3f &rclPoint, unsigned long &rulX, unsigned long &rulY, unsigned long &rulZ) const
{
  rulX = (unsigned long)((rclPoint.x - _fMinX) / _fGridLenX);
  rulY = (unsigned long)((rclPoint.y - _fMinY) / _fGridLenY);
  rulZ = (unsigned long)((rclPoint.z - _fMinZ) / _fGridLenZ);
}

unsigned long MeshPointGrid::FindElements (const Base::Vector3f &rclPoint, std::set<unsigned long>& aulElements) const
{
  unsigned long ulX, ulY, ulZ;
  Pos(rclPoint, ulX, ulY, ulZ);

  // check if the given point is inside the grid structure
  if ( (ulX < _ulCtGridsX) && (ulY < _ulCtGridsY) && (ulZ < _ulCtGridsZ) )
  {
    return GetElements(ulX, ulY, ulZ, aulElements);
  }

  return 0;
}

// ----------------------------------------------------------------

MeshGridIterator::MeshGridIterator (const MeshGrid &rclG)
: _rclGrid(rclG),
  _ulX(0), _ulY(0), _ulZ(0),
  _clPt(0.0f, 0.0f, 0.0f), _clDir(0.0f, 0.0f, 0.0f),
  _bValidRay(false),
  _fMaxSearchArea (FLOAT_MAX)

{
}

bool MeshGridIterator::InitOnRay (const Base::Vector3f &rclPt, const Base::Vector3f &rclDir, float fMaxSearchArea,
                                  std::vector<unsigned long> &raulElements)
{
  bool ret = InitOnRay (rclPt, rclDir, raulElements);
  _fMaxSearchArea = fMaxSearchArea;
  return ret;
}

bool MeshGridIterator::InitOnRay (const Base::Vector3f &rclPt, const Base::Vector3f &rclDir,
                                  std::vector<unsigned long> &raulElements)
{
  // needed in NextOnRay() to avoid an infinite loop
  _cSearchPositions.clear();

  _fMaxSearchArea = FLOAT_MAX;

  raulElements.clear();        

  _clPt      = rclPt;
  _clDir     = rclDir;
  _bValidRay = false;

  // liegt Punkt innerhalb globalen BB
  if ((_rclGrid.GetBoundBox().IsInBox(rclPt)) == true)
  {  // Voxel bestimmen, indem der Startpunkt liegt
    _rclGrid.Position(rclPt, _ulX, _ulY, _ulZ);
    raulElements.insert(raulElements.end(), _rclGrid._aulGrid[_ulX][_ulY][_ulZ].begin(), _rclGrid._aulGrid[_ulX][_ulY][_ulZ].end());
    _bValidRay = true;
  }
  else
  { // Startpunkt ausserhalb
    Base::Vector3f cP0, cP1;
    if (_rclGrid.GetBoundBox().IntersectWithLine(rclPt, rclDir, cP0, cP1) == true)
    {  // naechsten Punkt bestimmen
      if ((cP0 - rclPt).Length() < (cP1 - rclPt).Length())
        _rclGrid.Position(cP0, _ulX, _ulY, _ulZ);
      else
        _rclGrid.Position(cP1, _ulX, _ulY, _ulZ);

      raulElements.insert(raulElements.end(), _rclGrid._aulGrid[_ulX][_ulY][_ulZ].begin(), _rclGrid._aulGrid[_ulX][_ulY][_ulZ].end());
      _bValidRay = true;
    }
  }

  return _bValidRay;
}

bool MeshGridIterator::NextOnRay (std::vector<unsigned long> &raulElements)
{
  if (_bValidRay == false)
    return false;  // nicht initialisiert oder Strahl ausgetreten
  
  raulElements.clear();

  Base::Vector3f clIntersectPoint;

  // naechstes anliegende BB auf dem Suchstrahl suchen
  Base::BoundBox3f::SIDE tSide = _rclGrid.GetBoundBox(_ulX, _ulY, _ulZ).GetSideFromRay(_clPt, _clDir, clIntersectPoint);

  // Suchbereich
  //
  if ((_clPt-clIntersectPoint).Length() > _fMaxSearchArea)
  {
      _bValidRay = false;
  }
  else
  {
    switch (tSide)
    {
    case Base::BoundBox3f::LEFT:   _ulX--;  break;
    case Base::BoundBox3f::RIGHT:  _ulX++;  break;
    case Base::BoundBox3f::BOTTOM: _ulY--;  break;
    case Base::BoundBox3f::TOP:    _ulY++;  break;
    case Base::BoundBox3f::FRONT:  _ulZ--;  break;
    case Base::BoundBox3f::BACK:   _ulZ++;  break;

    default:
    case Base::BoundBox3f::INVALID:
      _bValidRay = false;
      break;
    }

    GridElement pos(_ulX, _ulY, _ulZ);
    if ( _cSearchPositions.find( pos ) != _cSearchPositions.end() )
      _bValidRay = false; // grid element already visited => result from GetSideFromRay invalid
  }

  if ((_bValidRay == true) && (_rclGrid.CheckPos(_ulX, _ulY, _ulZ) == true))
  {
    GridElement pos(_ulX, _ulY, _ulZ); _cSearchPositions.insert(pos);
    raulElements.insert(raulElements.end(), _rclGrid._aulGrid[_ulX][_ulY][_ulZ].begin(), _rclGrid._aulGrid[_ulX][_ulY][_ulZ].end()); 
  }
  else
    _bValidRay = false;  // Strahl ausgetreten

  return _bValidRay;
}
