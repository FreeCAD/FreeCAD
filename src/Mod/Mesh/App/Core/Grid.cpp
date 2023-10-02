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
#include <algorithm>
#endif

#include "Algorithm.h"
#include "Grid.h"
#include "Iterator.h"
#include "MeshKernel.h"


using namespace MeshCore;

MeshGrid::MeshGrid(const MeshKernel& rclM)
    : _pclMesh(&rclM)
    , _ulCtElements(0)
    , _ulCtGridsX(0)
    , _ulCtGridsY(0)
    , _ulCtGridsZ(0)
    , _fGridLenX(0.0f)
    , _fGridLenY(0.0f)
    , _fGridLenZ(0.0f)
    , _fMinX(0.0f)
    , _fMinY(0.0f)
    , _fMinZ(0.0f)
{}

MeshGrid::MeshGrid()
    : _pclMesh(nullptr)
    , _ulCtElements(0)
    , _ulCtGridsX(MESH_CT_GRID)
    , _ulCtGridsY(MESH_CT_GRID)
    , _ulCtGridsZ(MESH_CT_GRID)
    , _fGridLenX(0.0f)
    , _fGridLenY(0.0f)
    , _fGridLenZ(0.0f)
    , _fMinX(0.0f)
    , _fMinY(0.0f)
    , _fMinZ(0.0f)
{}

void MeshGrid::Attach(const MeshKernel& rclM)
{
    _pclMesh = &rclM;
    RebuildGrid();
}

void MeshGrid::Clear()
{
    _aulGrid.clear();
    _pclMesh = nullptr;
}

void MeshGrid::Rebuild(unsigned long ulX, unsigned long ulY, unsigned long ulZ)
{
    _ulCtGridsX = ulX;
    _ulCtGridsY = ulY;
    _ulCtGridsZ = ulZ;
    _ulCtElements = HasElements();
    RebuildGrid();
}

void MeshGrid::Rebuild(unsigned long ulPerGrid, unsigned long ulMaxGrid)
{
    _ulCtElements = HasElements();
    CalculateGridLength(ulPerGrid, ulMaxGrid);
    RebuildGrid();
}

void MeshGrid::Rebuild(int iCtGridPerAxis)
{
    _ulCtElements = HasElements();
    CalculateGridLength(iCtGridPerAxis);
    RebuildGrid();
}

void MeshGrid::InitGrid()
{
    assert(_pclMesh);

    // Calculate grid length if not initialised
    //
    if ((_ulCtGridsX == 0) || (_ulCtGridsY == 0) || (_ulCtGridsZ == 0)) {
        CalculateGridLength(MESH_CT_GRID, MESH_MAX_GRIDS);
    }

    // Determine grid length and offset
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

    // Create data structure
    _aulGrid.clear();
    _aulGrid.resize(_ulCtGridsX);
    for (unsigned long i = 0; i < _ulCtGridsX; i++) {
        _aulGrid[i].resize(_ulCtGridsY);
        for (unsigned long j = 0; j < _ulCtGridsY; j++) {
            _aulGrid[i][j].resize(_ulCtGridsZ);
        }
    }
}

unsigned long MeshGrid::Inside(const Base::BoundBox3f& rclBB,
                               std::vector<ElementIndex>& raulElements,
                               bool bDelDoubles) const
{
    unsigned long ulMinX {}, ulMinY {}, ulMinZ {}, ulMaxX {}, ulMaxY {}, ulMaxZ {};

    raulElements.clear();

    // Grid boxes for a more detailed selection
    Position(Base::Vector3f(rclBB.MinX, rclBB.MinY, rclBB.MinZ), ulMinX, ulMinY, ulMinZ);
    Position(Base::Vector3f(rclBB.MaxX, rclBB.MaxY, rclBB.MaxZ), ulMaxX, ulMaxY, ulMaxZ);

    for (auto i = ulMinX; i <= ulMaxX; i++) {
        for (auto j = ulMinY; j <= ulMaxY; j++) {
            for (auto k = ulMinZ; k <= ulMaxZ; k++) {
                raulElements.insert(raulElements.end(),
                                    _aulGrid[i][j][k].begin(),
                                    _aulGrid[i][j][k].end());
            }
        }
    }

    if (bDelDoubles) {
        // remove duplicate mentions
        std::sort(raulElements.begin(), raulElements.end());
        raulElements.erase(std::unique(raulElements.begin(), raulElements.end()),
                           raulElements.end());
    }

    return raulElements.size();
}

unsigned long MeshGrid::Inside(const Base::BoundBox3f& rclBB,
                               std::vector<ElementIndex>& raulElements,
                               const Base::Vector3f& rclOrg,
                               float fMaxDist,
                               bool bDelDoubles) const
{
    unsigned long ulMinX {}, ulMinY {}, ulMinZ {}, ulMaxX {}, ulMaxY {}, ulMaxZ {};
    float fGridDiag = GetBoundBox(0, 0, 0).CalcDiagonalLength();
    float fMinDistP2 = (fGridDiag * fGridDiag) + (fMaxDist * fMaxDist);

    raulElements.clear();

    // Grid boxes for a more detailed selection
    Position(Base::Vector3f(rclBB.MinX, rclBB.MinY, rclBB.MinZ), ulMinX, ulMinY, ulMinZ);
    Position(Base::Vector3f(rclBB.MaxX, rclBB.MaxY, rclBB.MaxZ), ulMaxX, ulMaxY, ulMaxZ);

    for (auto i = ulMinX; i <= ulMaxX; i++) {
        for (auto j = ulMinY; j <= ulMaxY; j++) {
            for (auto k = ulMinZ; k <= ulMaxZ; k++) {
                if (Base::DistanceP2(GetBoundBox(i, j, k).GetCenter(), rclOrg) < fMinDistP2) {
                    raulElements.insert(raulElements.end(),
                                        _aulGrid[i][j][k].begin(),
                                        _aulGrid[i][j][k].end());
                }
            }
        }
    }

    if (bDelDoubles) {
        // remove duplicate mentions
        std::sort(raulElements.begin(), raulElements.end());
        raulElements.erase(std::unique(raulElements.begin(), raulElements.end()),
                           raulElements.end());
    }

    return raulElements.size();
}

unsigned long MeshGrid::Inside(const Base::BoundBox3f& rclBB,
                               std::set<ElementIndex>& raulElements) const
{
    unsigned long ulMinX {}, ulMinY {}, ulMinZ {}, ulMaxX {}, ulMaxY {}, ulMaxZ {};

    raulElements.clear();

    // Grid boxes for a more detailed selection
    Position(Base::Vector3f(rclBB.MinX, rclBB.MinY, rclBB.MinZ), ulMinX, ulMinY, ulMinZ);
    Position(Base::Vector3f(rclBB.MaxX, rclBB.MaxY, rclBB.MaxZ), ulMaxX, ulMaxY, ulMaxZ);

    for (auto i = ulMinX; i <= ulMaxX; i++) {
        for (auto j = ulMinY; j <= ulMaxY; j++) {
            for (auto k = ulMinZ; k <= ulMaxZ; k++) {
                raulElements.insert(_aulGrid[i][j][k].begin(), _aulGrid[i][j][k].end());
            }
        }
    }

    return raulElements.size();
}

bool MeshGrid::CheckPosition(const Base::Vector3f& rclPoint,
                             unsigned long& rulX,
                             unsigned long& rulY,
                             unsigned long& rulZ) const
{
    rulX = static_cast<unsigned long>((rclPoint.x - _fMinX) / _fGridLenX);
    rulY = static_cast<unsigned long>((rclPoint.y - _fMinY) / _fGridLenY);
    rulZ = static_cast<unsigned long>((rclPoint.z - _fMinZ) / _fGridLenZ);

    if ((rulX < _ulCtGridsX) && (rulY < _ulCtGridsY) && (rulZ < _ulCtGridsZ)) {
        return true;
    }

    return false;
}

void MeshGrid::Position(const Base::Vector3f& rclPoint,
                        unsigned long& rulX,
                        unsigned long& rulY,
                        unsigned long& rulZ) const
{
    if (rclPoint.x <= _fMinX) {
        rulX = 0;
    }
    else {
        rulX =
            std::min<unsigned long>(static_cast<unsigned long>((rclPoint.x - _fMinX) / _fGridLenX),
                                    _ulCtGridsX - 1);
    }

    if (rclPoint.y <= _fMinY) {
        rulY = 0;
    }
    else {
        rulY =
            std::min<unsigned long>(static_cast<unsigned long>((rclPoint.y - _fMinY) / _fGridLenY),
                                    _ulCtGridsY - 1);
    }

    if (rclPoint.z <= _fMinZ) {
        rulZ = 0;
    }
    else {
        rulZ =
            std::min<unsigned long>(static_cast<unsigned long>((rclPoint.z - _fMinZ) / _fGridLenZ),
                                    _ulCtGridsZ - 1);
    }
}

void MeshGrid::CalculateGridLength(unsigned long ulCtGrid, unsigned long ulMaxGrids)
{
    // Calculate grid lengths or number of grids per dimension
    // There should be about 10 (?!?!) facets per grid
    // respectively max grids should not exceed 10000
    Base::BoundBox3f clBBMeshEnlarged = _pclMesh->GetBoundBox();
    float fGridLen = 0;

    float fLenX = clBBMeshEnlarged.LengthX();
    float fLenY = clBBMeshEnlarged.LengthY();
    float fLenZ = clBBMeshEnlarged.LengthZ();

    float fVolume = fLenX * fLenY * fLenZ;
    if (fVolume > 0.0f) {
        float fVolElem {};
        if (_ulCtElements > (ulMaxGrids * ulCtGrid)) {
            fVolElem = (fLenX * fLenY * fLenZ) / float(ulMaxGrids * ulCtGrid);
        }
        else {
            fVolElem = (fLenX * fLenY * fLenZ) / float(_ulCtElements);
        }

        float fVol = fVolElem * float(ulCtGrid);
        fGridLen = float(pow(fVol, 1.0f / 3.0f));
    }
    else {
        // Planar bounding box
        float fArea = fLenX * fLenY + fLenX * fLenZ + fLenY * fLenZ;
        float fAreaElem {};
        if (_ulCtElements > (ulMaxGrids * ulCtGrid)) {
            fAreaElem = fArea / float(ulMaxGrids * ulCtGrid);
        }
        else {
            fAreaElem = fArea / float(_ulCtElements);
        }

        float fRepresentativeArea = fAreaElem * static_cast<float>(ulCtGrid);
        fGridLen = sqrt(fRepresentativeArea);
    }

    if (fGridLen > 0) {
        _ulCtGridsX = std::max<unsigned long>(static_cast<unsigned long>(fLenX / fGridLen), 1);
        _ulCtGridsY = std::max<unsigned long>(static_cast<unsigned long>(fLenY / fGridLen), 1);
        _ulCtGridsZ = std::max<unsigned long>(static_cast<unsigned long>(fLenZ / fGridLen), 1);
    }
    else {
        // Degenerated grid
        _ulCtGridsX = 1;
        _ulCtGridsY = 1;
        _ulCtGridsZ = 1;
    }
}

void MeshGrid::CalculateGridLength(int iCtGridPerAxis)
{
    if (iCtGridPerAxis <= 0) {
        CalculateGridLength(MESH_CT_GRID, MESH_MAX_GRIDS);
        return;
    }

    // Calculate grid lengths or number of grids per dimension
    // There should be about 10 (?!?!) facets per grid
    // respectively max grids should not exceed 10000
    Base::BoundBox3f clBBMesh = _pclMesh->GetBoundBox();

    float fLenghtX = clBBMesh.LengthX();
    float fLenghtY = clBBMesh.LengthY();
    float fLenghtZ = clBBMesh.LengthZ();

    float fLenghtD = clBBMesh.CalcDiagonalLength();

    float fLengthTol = 0.05f * fLenghtD;

    bool bLenghtXisZero = (fLenghtX <= fLengthTol);
    bool bLenghtYisZero = (fLenghtY <= fLengthTol);
    bool bLenghtZisZero = (fLenghtZ <= fLengthTol);

    int iFlag = 0;

    int iMaxGrids = 1;

    if (bLenghtXisZero) {
        iFlag += 1;
    }
    else {
        iMaxGrids *= iCtGridPerAxis;
    }

    if (bLenghtYisZero) {
        iFlag += 2;
    }
    else {
        iMaxGrids *= iCtGridPerAxis;
    }

    if (bLenghtZisZero) {
        iFlag += 4;
    }
    else {
        iMaxGrids *= iCtGridPerAxis;
    }

    unsigned long ulGridsFacets = 10;

    float fFactorVolumen = 40.0;
    float fFactorArea = 10.0;

    switch (iFlag) {
        case 0: {
            float fVolumen = fLenghtX * fLenghtY * fLenghtZ;

            float fVolumenGrid = (fVolumen * ulGridsFacets) / (fFactorVolumen * _ulCtElements);

            if ((fVolumenGrid * iMaxGrids) < fVolumen) {
                fVolumenGrid = fVolumen / static_cast<float>(iMaxGrids);
            }

            float fLengthGrid = pow(fVolumenGrid, 1.0f / 3.0f);

            _ulCtGridsX =
                std::max<unsigned long>(static_cast<unsigned long>(fLenghtX / fLengthGrid), 1);
            _ulCtGridsY =
                std::max<unsigned long>(static_cast<unsigned long>(fLenghtY / fLengthGrid), 1);
            _ulCtGridsZ =
                std::max<unsigned long>(static_cast<unsigned long>(fLenghtZ / fLengthGrid), 1);

        } break;
        case 1: {
            _ulCtGridsX = 1;  // bLenghtXisZero

            float fArea = fLenghtY * fLenghtZ;

            float fAreaGrid = (fArea * ulGridsFacets) / (fFactorArea * _ulCtElements);

            if ((fAreaGrid * iMaxGrids) < fArea) {
                fAreaGrid = fArea / static_cast<float>(iMaxGrids);
            }

            float fLengthGrid = float(sqrt(fAreaGrid));

            _ulCtGridsY =
                std::max<unsigned long>(static_cast<unsigned long>(fLenghtY / fLengthGrid), 1);
            _ulCtGridsZ =
                std::max<unsigned long>(static_cast<unsigned long>(fLenghtZ / fLengthGrid), 1);
        } break;
        case 2: {
            _ulCtGridsY = 1;  // bLenghtYisZero

            float fArea = fLenghtX * fLenghtZ;

            float fAreaGrid = (fArea * ulGridsFacets) / (fFactorArea * _ulCtElements);

            if ((fAreaGrid * iMaxGrids) < fArea) {
                fAreaGrid = fArea / static_cast<float>(iMaxGrids);
            }

            float fLengthGrid = float(sqrt(fAreaGrid));

            _ulCtGridsX =
                std::max<unsigned long>(static_cast<unsigned long>(fLenghtX / fLengthGrid), 1);
            _ulCtGridsZ =
                std::max<unsigned long>(static_cast<unsigned long>(fLenghtZ / fLengthGrid), 1);
        } break;
        case 3: {
            _ulCtGridsX = 1;                                      // bLenghtXisZero
            _ulCtGridsY = 1;                                      // bLenghtYisZero
            _ulCtGridsZ = static_cast<unsigned long>(iMaxGrids);  // bLenghtYisZero
        } break;
        case 4: {
            _ulCtGridsZ = 1;  // bLenghtZisZero

            float fArea = fLenghtX * fLenghtY;

            float fAreaGrid = (fArea * ulGridsFacets) / (fFactorArea * _ulCtElements);

            if ((fAreaGrid * iMaxGrids) < fArea) {
                fAreaGrid = fArea / static_cast<float>(iMaxGrids);
            }

            float fLengthGrid = float(sqrt(fAreaGrid));

            _ulCtGridsX =
                std::max<unsigned long>(static_cast<unsigned long>(fLenghtX / fLengthGrid), 1);
            _ulCtGridsY =
                std::max<unsigned long>(static_cast<unsigned long>(fLenghtY / fLengthGrid), 1);
        } break;
        case 5: {
            _ulCtGridsX = 1;                                      // bLenghtXisZero
            _ulCtGridsZ = 1;                                      // bLenghtZisZero
            _ulCtGridsY = static_cast<unsigned long>(iMaxGrids);  // bLenghtYisZero
        } break;
        case 6: {
            _ulCtGridsY = 1;                                      // bLenghtYisZero
            _ulCtGridsZ = 1;                                      // bLenghtZisZero
            _ulCtGridsX = static_cast<unsigned long>(iMaxGrids);  // bLenghtYisZero
        } break;
        case 7: {
            _ulCtGridsX = static_cast<unsigned long>(iMaxGrids);  // bLenghtXisZero
            _ulCtGridsY = static_cast<unsigned long>(iMaxGrids);  // bLenghtYisZero
            _ulCtGridsZ = static_cast<unsigned long>(iMaxGrids);  // bLenghtZisZero
        } break;
    }
}

void MeshGrid::SearchNearestFromPoint(const Base::Vector3f& rclPt,
                                      std::set<ElementIndex>& raclInd) const
{
    raclInd.clear();
    Base::BoundBox3f clBB = GetBoundBox();

    if (clBB.IsInBox(rclPt)) {  // Point lies within
        unsigned long ulX {}, ulY {}, ulZ {};
        Position(rclPt, ulX, ulY, ulZ);
        // int nX = ulX, nY = ulY, nZ = ulZ;
        unsigned long ulMaxLevel =
            std::max<unsigned long>(_ulCtGridsX, std::max<unsigned long>(_ulCtGridsY, _ulCtGridsZ));
        unsigned long ulLevel = 0;
        while (raclInd.empty() && ulLevel <= ulMaxLevel) {
            GetHull(ulX, ulY, ulZ, ulLevel++, raclInd);
        }
        GetHull(ulX, ulY, ulZ, ulLevel, raclInd);
    }
    else {  // Point outside
        Base::BoundBox3f::SIDE tSide = clBB.GetSideFromRay(rclPt, clBB.GetCenter() - rclPt);
        switch (tSide) {
            case Base::BoundBox3f::RIGHT: {
                unsigned long nX = 0;
                while (raclInd.empty() && nX < _ulCtGridsX) {
                    for (unsigned long i = 0; i < _ulCtGridsY; i++) {
                        for (unsigned long j = 0; j < _ulCtGridsZ; j++) {
                            raclInd.insert(_aulGrid[nX][i][j].begin(), _aulGrid[nX][i][j].end());
                        }
                    }
                    nX++;
                }
                break;
            }
            case Base::BoundBox3f::LEFT: {
                unsigned long nX = _ulCtGridsX - 1;
                while (raclInd.empty() && nX < _ulCtGridsX) {
                    for (unsigned long i = 0; i < _ulCtGridsY; i++) {
                        for (unsigned long j = 0; j < _ulCtGridsZ; j++) {
                            raclInd.insert(_aulGrid[nX][i][j].begin(), _aulGrid[nX][i][j].end());
                        }
                    }
                    nX++;
                }
                break;
            }
            case Base::BoundBox3f::TOP: {
                unsigned long nY = 0;
                while (raclInd.empty() && nY < _ulCtGridsY) {
                    for (unsigned long i = 0; i < _ulCtGridsX; i++) {
                        for (unsigned long j = 0; j < _ulCtGridsZ; j++) {
                            raclInd.insert(_aulGrid[i][nY][j].begin(), _aulGrid[i][nY][j].end());
                        }
                    }
                    nY++;
                }
                break;
            }
            case Base::BoundBox3f::BOTTOM: {
                unsigned long nY = _ulCtGridsY - 1;
                while (raclInd.empty() && nY < _ulCtGridsY) {
                    for (unsigned long i = 0; i < _ulCtGridsX; i++) {
                        for (unsigned long j = 0; j < _ulCtGridsZ; j++) {
                            raclInd.insert(_aulGrid[i][nY][j].begin(), _aulGrid[i][nY][j].end());
                        }
                    }
                    nY--;
                }
                break;
            }
            case Base::BoundBox3f::BACK: {
                unsigned long nZ = 0;
                while (raclInd.empty() && nZ < _ulCtGridsZ) {
                    for (unsigned long i = 0; i < _ulCtGridsX; i++) {
                        for (unsigned long j = 0; j < _ulCtGridsY; j++) {
                            raclInd.insert(_aulGrid[i][j][nZ].begin(), _aulGrid[i][j][nZ].end());
                        }
                    }
                    nZ++;
                }
                break;
            }
            case Base::BoundBox3f::FRONT: {
                unsigned long nZ = _ulCtGridsZ - 1;
                while (raclInd.empty() && nZ < _ulCtGridsZ) {
                    for (unsigned long i = 0; i < _ulCtGridsX; i++) {
                        for (unsigned long j = 0; j < _ulCtGridsY; j++) {
                            raclInd.insert(_aulGrid[i][j][nZ].begin(), _aulGrid[i][j][nZ].end());
                        }
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

void MeshGrid::GetHull(unsigned long ulX,
                       unsigned long ulY,
                       unsigned long ulZ,
                       unsigned long ulDistance,
                       std::set<ElementIndex>& raclInd) const
{
    int nX1 = std::max<int>(0, int(ulX) - int(ulDistance));
    int nY1 = std::max<int>(0, int(ulY) - int(ulDistance));
    int nZ1 = std::max<int>(0, int(ulZ) - int(ulDistance));
    int nX2 = std::min<int>(int(_ulCtGridsX) - 1, int(ulX) + int(ulDistance));
    int nY2 = std::min<int>(int(_ulCtGridsY) - 1, int(ulY) + int(ulDistance));
    int nZ2 = std::min<int>(int(_ulCtGridsZ) - 1, int(ulZ) + int(ulDistance));

    // top plane
    for (int i = nX1; i <= nX2; i++) {
        for (int j = nY1; j <= nY2; j++) {
            GetElements(static_cast<unsigned long>(i),
                        static_cast<unsigned long>(j),
                        static_cast<unsigned long>(nZ1),
                        raclInd);
        }
    }
    // bottom plane
    for (int i = nX1; i <= nX2; i++) {
        for (int j = nY1; j <= nY2; j++) {
            GetElements(static_cast<unsigned long>(i),
                        static_cast<unsigned long>(j),
                        static_cast<unsigned long>(nZ2),
                        raclInd);
        }
    }
    // left plane
    for (int i = nY1; i <= nY2; i++) {
        for (int j = (nZ1 + 1); j <= (nZ2 - 1); j++) {
            GetElements(static_cast<unsigned long>(nX1),
                        static_cast<unsigned long>(i),
                        static_cast<unsigned long>(j),
                        raclInd);
        }
    }
    // right plane
    for (int i = nY1; i <= nY2; i++) {
        for (int j = (nZ1 + 1); j <= (nZ2 - 1); j++) {
            GetElements(static_cast<unsigned long>(nX2),
                        static_cast<unsigned long>(i),
                        static_cast<unsigned long>(j),
                        raclInd);
        }
    }
    // front plane
    for (int i = (nX1 + 1); i <= (nX2 - 1); i++) {
        for (int j = (nZ1 + 1); j <= (nZ2 - 1); j++) {
            GetElements(static_cast<unsigned long>(i),
                        static_cast<unsigned long>(nY1),
                        static_cast<unsigned long>(j),
                        raclInd);
        }
    }
    // back plane
    for (int i = (nX1 + 1); i <= (nX2 - 1); i++) {
        for (int j = (nZ1 + 1); j <= (nZ2 - 1); j++) {
            GetElements(static_cast<unsigned long>(i),
                        static_cast<unsigned long>(nY2),
                        static_cast<unsigned long>(j),
                        raclInd);
        }
    }
}

unsigned long MeshGrid::GetElements(unsigned long ulX,
                                    unsigned long ulY,
                                    unsigned long ulZ,
                                    std::set<ElementIndex>& raclInd) const
{
    const std::set<ElementIndex>& rclSet = _aulGrid[ulX][ulY][ulZ];
    if (!rclSet.empty()) {
        raclInd.insert(rclSet.begin(), rclSet.end());
        return rclSet.size();
    }

    return 0;
}

unsigned long MeshGrid::GetElements(const Base::Vector3f& rclPoint,
                                    std::vector<ElementIndex>& aulFacets) const
{
    unsigned long ulX {}, ulY {}, ulZ {};
    if (!CheckPosition(rclPoint, ulX, ulY, ulZ)) {
        return 0;
    }

    aulFacets.resize(_aulGrid[ulX][ulY][ulZ].size());

    std::copy(_aulGrid[ulX][ulY][ulZ].begin(), _aulGrid[ulX][ulY][ulZ].end(), aulFacets.begin());
    return aulFacets.size();
}

unsigned long
MeshGrid::GetIndexToPosition(unsigned long ulX, unsigned long ulY, unsigned long ulZ) const
{
    if (!CheckPos(ulX, ulY, ulZ)) {
        return ULONG_MAX;
    }
    return (ulZ * _ulCtGridsY + ulY) * _ulCtGridsX + ulX;
}

bool MeshGrid::GetPositionToIndex(unsigned long id,
                                  unsigned long& ulX,
                                  unsigned long& ulY,
                                  unsigned long& ulZ) const
{
    ulX = id % _ulCtGridsX;
    ulY = (id / _ulCtGridsX) % _ulCtGridsY;
    ulZ = id / (_ulCtGridsX * _ulCtGridsY);

    if (!CheckPos(ulX, ulY, ulZ)) {
        ulX = ULONG_MAX;
        ulY = ULONG_MAX;
        ulZ = ULONG_MAX;
        return false;
    }

    return true;
}

// ----------------------------------------------------------------

MeshFacetGrid::MeshFacetGrid(const MeshKernel& rclM)
    : MeshGrid(rclM)
{
    MeshFacetGrid::RebuildGrid();
}

MeshFacetGrid::MeshFacetGrid(const MeshKernel& rclM, int iCtGridPerAxis)
    : MeshGrid(rclM)
{
    Rebuild(iCtGridPerAxis);
}

MeshFacetGrid::MeshFacetGrid(const MeshKernel& rclM,
                             unsigned long ulX,
                             unsigned long ulY,
                             unsigned long ulZ)
    : MeshGrid(rclM)
{
    Rebuild(ulX, ulY, ulZ);
}

MeshFacetGrid::MeshFacetGrid(const MeshKernel& rclM, float fGridLen)
    : MeshGrid(rclM)
{
    Base::BoundBox3f clBBMesh = _pclMesh->GetBoundBox();
    Rebuild(std::max<unsigned long>(static_cast<unsigned long>(clBBMesh.LengthX() / fGridLen), 1),
            std::max<unsigned long>(static_cast<unsigned long>(clBBMesh.LengthY() / fGridLen), 1),
            std::max<unsigned long>(static_cast<unsigned long>(clBBMesh.LengthZ() / fGridLen), 1));
}

void MeshFacetGrid::Validate(const MeshKernel& rclMesh)
{
    if (_pclMesh != &rclMesh) {
        Attach(rclMesh);
    }
    else if (rclMesh.CountFacets() != _ulCtElements) {
        RebuildGrid();
    }
}

void MeshFacetGrid::Validate()
{
    if (!_pclMesh) {
        return;
    }

    if (_pclMesh->CountFacets() != _ulCtElements) {
        RebuildGrid();
    }
}

bool MeshFacetGrid::Verify() const
{
    if (!_pclMesh) {
        return false;  // no mesh attached
    }
    if (_pclMesh->CountFacets() != _ulCtElements) {
        return false;  // not up-to-date
    }

    MeshGridIterator it(*this);
    MeshFacetIterator cF(*_pclMesh);
    for (it.Init(); it.More(); it.Next()) {
        std::vector<ElementIndex> aulElements;
        it.GetElements(aulElements);
        for (ElementIndex element : aulElements) {
            cF.Set(element);
            if (!cF->IntersectBoundingBox(it.GetBoundBox())) {
                return false;  // no intersection between facet although the facet is in grid
            }
        }
    }

    return true;
}

void MeshFacetGrid::RebuildGrid()
{
    _ulCtElements = _pclMesh->CountFacets();

    InitGrid();

    // Fill data structure
    MeshFacetIterator clFIter(*_pclMesh);

    unsigned long i = 0;
    for (clFIter.Init(); clFIter.More(); clFIter.Next()) {
        //    AddFacet(*clFIter, i++, 2.0f);
        AddFacet(*clFIter, i++);
    }
}

unsigned long MeshFacetGrid::SearchNearestFromPoint(const Base::Vector3f& rclPt) const
{
    ElementIndex ulFacetInd = ELEMENT_INDEX_MAX;
    float fMinDist = FLOAT_MAX;
    Base::BoundBox3f clBB = GetBoundBox();

    if (clBB.IsInBox(rclPt)) {  // Point lies within
        unsigned long ulX {}, ulY {}, ulZ {};
        Position(rclPt, ulX, ulY, ulZ);
        float fMinGridDist = std::min<float>(std::min<float>(_fGridLenX, _fGridLenY), _fGridLenZ);
        unsigned long ulDistance = 0;
        while (fMinDist > (fMinGridDist * float(ulDistance))) {
            SearchNearestFacetInHull(ulX, ulY, ulZ, ulDistance, rclPt, ulFacetInd, fMinDist);
            ulDistance++;
        }
        SearchNearestFacetInHull(ulX, ulY, ulZ, ulDistance, rclPt, ulFacetInd, fMinDist);
    }
    else {  // Point outside
        Base::BoundBox3f::SIDE tSide = clBB.GetSideFromRay(rclPt, clBB.GetCenter() - rclPt);
        switch (tSide) {
            case Base::BoundBox3f::RIGHT: {
                unsigned long nX = 0;
                while ((fMinDist > ((clBB.MinX - rclPt.x) + (float(nX) * _fGridLenX)))
                       && (nX < _ulCtGridsX)) {
                    for (unsigned long i = 0; i < _ulCtGridsY; i++) {
                        for (unsigned long j = 0; j < _ulCtGridsZ; j++) {
                            SearchNearestFacetInGrid(nX, i, j, rclPt, fMinDist, ulFacetInd);
                        }
                    }
                    nX++;
                }
                break;
            }
            case Base::BoundBox3f::LEFT: {
                unsigned long nX = _ulCtGridsX - 1;
                while ((fMinDist > ((rclPt.x - clBB.MinX) + (float(nX) * _fGridLenX)))
                       && (nX < _ulCtGridsX)) {
                    for (unsigned long i = 0; i < _ulCtGridsY; i++) {
                        for (unsigned long j = 0; j < _ulCtGridsZ; j++) {
                            SearchNearestFacetInGrid(nX, i, j, rclPt, fMinDist, ulFacetInd);
                        }
                    }
                    nX--;
                }
                break;
            }
            case Base::BoundBox3f::TOP: {
                unsigned long nY = 0;
                while ((fMinDist > ((clBB.MinY - rclPt.y) + (float(nY) * _fGridLenY)))
                       && (nY < _ulCtGridsY)) {
                    for (unsigned long i = 0; i < _ulCtGridsX; i++) {
                        for (unsigned long j = 0; j < _ulCtGridsZ; j++) {
                            SearchNearestFacetInGrid(i, nY, j, rclPt, fMinDist, ulFacetInd);
                        }
                    }
                    nY++;
                }
                break;
            }
            case Base::BoundBox3f::BOTTOM: {
                unsigned long nY = _ulCtGridsY - 1;
                while ((fMinDist > ((rclPt.y - clBB.MinY) + (float(nY) * _fGridLenY)))
                       && (nY < _ulCtGridsY)) {
                    for (unsigned long i = 0; i < _ulCtGridsX; i++) {
                        for (unsigned long j = 0; j < _ulCtGridsZ; j++) {
                            SearchNearestFacetInGrid(i, nY, j, rclPt, fMinDist, ulFacetInd);
                        }
                    }
                    nY--;
                }
                break;
            }
            case Base::BoundBox3f::BACK: {
                unsigned long nZ = 0;
                while ((fMinDist > ((clBB.MinZ - rclPt.z) + (float(nZ) * _fGridLenZ)))
                       && (nZ < _ulCtGridsZ)) {
                    for (unsigned long i = 0; i < _ulCtGridsX; i++) {
                        for (unsigned long j = 0; j < _ulCtGridsY; j++) {
                            SearchNearestFacetInGrid(i, j, nZ, rclPt, fMinDist, ulFacetInd);
                        }
                    }
                    nZ++;
                }
                break;
            }
            case Base::BoundBox3f::FRONT: {
                unsigned long nZ = _ulCtGridsZ - 1;
                while ((fMinDist > ((rclPt.z - clBB.MinZ) + (float(nZ) * _fGridLenZ)))
                       && (nZ < _ulCtGridsZ)) {
                    for (unsigned long i = 0; i < _ulCtGridsX; i++) {
                        for (unsigned long j = 0; j < _ulCtGridsY; j++) {
                            SearchNearestFacetInGrid(i, j, nZ, rclPt, fMinDist, ulFacetInd);
                        }
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

unsigned long MeshFacetGrid::SearchNearestFromPoint(const Base::Vector3f& rclPt,
                                                    float fMaxSearchArea) const
{
    std::vector<ElementIndex> aulFacets;
    ElementIndex ulFacetInd = ELEMENT_INDEX_MAX;
    float fMinDist = fMaxSearchArea;

    MeshAlgorithm clFTool(*_pclMesh);

    Base::BoundBox3f clBB(rclPt.x - fMaxSearchArea,
                          rclPt.y - fMaxSearchArea,
                          rclPt.z - fMaxSearchArea,
                          rclPt.x + fMaxSearchArea,
                          rclPt.y + fMaxSearchArea,
                          rclPt.z + fMaxSearchArea);

    Inside(clBB, aulFacets, rclPt, fMaxSearchArea, true);

    for (ElementIndex facet : aulFacets) {
        float fDist {};

        if (clFTool.Distance(rclPt, facet, fMinDist, fDist)) {
            fMinDist = fDist;
            ulFacetInd = facet;
        }
    }

    return ulFacetInd;
}

void MeshFacetGrid::SearchNearestFacetInHull(unsigned long ulX,
                                             unsigned long ulY,
                                             unsigned long ulZ,
                                             unsigned long ulDistance,
                                             const Base::Vector3f& rclPt,
                                             ElementIndex& rulFacetInd,
                                             float& rfMinDist) const
{
    int nX1 = std::max<int>(0, int(ulX) - int(ulDistance));
    int nY1 = std::max<int>(0, int(ulY) - int(ulDistance));
    int nZ1 = std::max<int>(0, int(ulZ) - int(ulDistance));
    int nX2 = std::min<int>(int(_ulCtGridsX) - 1, int(ulX) + int(ulDistance));
    int nY2 = std::min<int>(int(_ulCtGridsY) - 1, int(ulY) + int(ulDistance));
    int nZ2 = std::min<int>(int(_ulCtGridsZ) - 1, int(ulZ) + int(ulDistance));

    // top plane
    for (int i = nX1; i <= nX2; i++) {
        for (int j = nY1; j <= nY2; j++) {
            SearchNearestFacetInGrid(static_cast<unsigned long>(i),
                                     static_cast<unsigned long>(j),
                                     static_cast<unsigned long>(nZ1),
                                     rclPt,
                                     rfMinDist,
                                     rulFacetInd);
        }
    }
    // bottom plane
    for (int i = nX1; i <= nX2; i++) {
        for (int j = nY1; j <= nY2; j++) {
            SearchNearestFacetInGrid(static_cast<unsigned long>(i),
                                     static_cast<unsigned long>(j),
                                     static_cast<unsigned long>(nZ2),
                                     rclPt,
                                     rfMinDist,
                                     rulFacetInd);
        }
    }
    // left plane
    for (int i = nY1; i <= nY2; i++) {
        for (int j = (nZ1 + 1); j <= (nZ2 - 1); j++) {
            SearchNearestFacetInGrid(static_cast<unsigned long>(nX1),
                                     static_cast<unsigned long>(i),
                                     static_cast<unsigned long>(j),
                                     rclPt,
                                     rfMinDist,
                                     rulFacetInd);
        }
    }
    // right plane
    for (int i = nY1; i <= nY2; i++) {
        for (int j = (nZ1 + 1); j <= (nZ2 - 1); j++) {
            SearchNearestFacetInGrid(static_cast<unsigned long>(nX2),
                                     static_cast<unsigned long>(i),
                                     static_cast<unsigned long>(j),
                                     rclPt,
                                     rfMinDist,
                                     rulFacetInd);
        }
    }
    // front plane
    for (int i = (nX1 + 1); i <= (nX2 - 1); i++) {
        for (int j = (nZ1 + 1); j <= (nZ2 - 1); j++) {
            SearchNearestFacetInGrid(static_cast<unsigned long>(i),
                                     static_cast<unsigned long>(nY1),
                                     static_cast<unsigned long>(j),
                                     rclPt,
                                     rfMinDist,
                                     rulFacetInd);
        }
    }
    // back plane
    for (int i = (nX1 + 1); i <= (nX2 - 1); i++) {
        for (int j = (nZ1 + 1); j <= (nZ2 - 1); j++) {
            SearchNearestFacetInGrid(static_cast<unsigned long>(i),
                                     static_cast<unsigned long>(nY2),
                                     static_cast<unsigned long>(j),
                                     rclPt,
                                     rfMinDist,
                                     rulFacetInd);
        }
    }
}

void MeshFacetGrid::SearchNearestFacetInGrid(unsigned long ulX,
                                             unsigned long ulY,
                                             unsigned long ulZ,
                                             const Base::Vector3f& rclPt,
                                             float& rfMinDist,
                                             ElementIndex& rulFacetInd) const
{
    const std::set<ElementIndex>& rclSet = _aulGrid[ulX][ulY][ulZ];
    for (ElementIndex pI : rclSet) {
        float fDist = _pclMesh->GetFacet(pI).DistanceToPoint(rclPt);
        if (fDist < rfMinDist) {
            rfMinDist = fDist;
            rulFacetInd = pI;
        }
    }
}

//----------------------------------------------------------------------------

MeshPointGrid::MeshPointGrid(const MeshKernel& rclM)
    : MeshGrid(rclM)
{
    MeshPointGrid::RebuildGrid();
}

MeshPointGrid::MeshPointGrid()
    : MeshGrid()
{}

MeshPointGrid::MeshPointGrid(const MeshKernel& rclM,
                             unsigned long ulX,
                             unsigned long ulY,
                             unsigned long ulZ)
    : MeshGrid(rclM)
{
    Rebuild(ulX, ulY, ulZ);
}

MeshPointGrid::MeshPointGrid(const MeshKernel& rclM, int iCtGridPerAxis)
    : MeshGrid(rclM)
{
    Rebuild(iCtGridPerAxis);
}

MeshPointGrid::MeshPointGrid(const MeshKernel& rclM, float fGridLen)
    : MeshGrid(rclM)
{
    Base::BoundBox3f clBBMesh = _pclMesh->GetBoundBox();
    Rebuild(std::max<unsigned long>(static_cast<unsigned long>(clBBMesh.LengthX() / fGridLen), 1),
            std::max<unsigned long>(static_cast<unsigned long>(clBBMesh.LengthY() / fGridLen), 1),
            std::max<unsigned long>(static_cast<unsigned long>(clBBMesh.LengthZ() / fGridLen), 1));
}

void MeshPointGrid::AddPoint(const MeshPoint& rclPt, ElementIndex ulPtIndex, float fEpsilon)
{
    (void)fEpsilon;
    unsigned long ulX {}, ulY {}, ulZ {};
    Pos(Base::Vector3f(rclPt.x, rclPt.y, rclPt.z), ulX, ulY, ulZ);
    if ((ulX < _ulCtGridsX) && (ulY < _ulCtGridsY) && (ulZ < _ulCtGridsZ)) {
        _aulGrid[ulX][ulY][ulZ].insert(ulPtIndex);
    }
}

void MeshPointGrid::Validate(const MeshKernel& rclMesh)
{
    if (_pclMesh != &rclMesh) {
        Attach(rclMesh);
    }
    else if (rclMesh.CountPoints() != _ulCtElements) {
        RebuildGrid();
    }
}

void MeshPointGrid::Validate()
{
    if (!_pclMesh) {
        return;
    }

    if (_pclMesh->CountPoints() != _ulCtElements) {
        RebuildGrid();
    }
}

bool MeshPointGrid::Verify() const
{
    if (!_pclMesh) {
        return false;  // no mesh attached
    }
    if (_pclMesh->CountFacets() != _ulCtElements) {
        return false;  // not up-to-date
    }

    MeshGridIterator it(*this);
    MeshPointIterator cP(*_pclMesh);
    for (it.Init(); it.More(); it.Next()) {
        std::vector<ElementIndex> aulElements;
        it.GetElements(aulElements);
        for (ElementIndex element : aulElements) {
            cP.Set(element);
            if (!it.GetBoundBox().IsInBox(*cP)) {
                return false;  // point doesn't lie inside the grid element
            }
        }
    }

    return true;
}

void MeshPointGrid::RebuildGrid()
{
    _ulCtElements = _pclMesh->CountPoints();

    InitGrid();

    // Fill data structure

    MeshPointIterator cPIter(*_pclMesh);

    unsigned long i = 0;
    for (cPIter.Init(); cPIter.More(); cPIter.Next()) {
        AddPoint(*cPIter, i++);
    }
}

void MeshPointGrid::Pos(const Base::Vector3f& rclPoint,
                        unsigned long& rulX,
                        unsigned long& rulY,
                        unsigned long& rulZ) const
{
    rulX = static_cast<unsigned long>((rclPoint.x - _fMinX) / _fGridLenX);
    rulY = static_cast<unsigned long>((rclPoint.y - _fMinY) / _fGridLenY);
    rulZ = static_cast<unsigned long>((rclPoint.z - _fMinZ) / _fGridLenZ);
}

unsigned long MeshPointGrid::FindElements(const Base::Vector3f& rclPoint,
                                          std::set<ElementIndex>& aulElements) const
{
    unsigned long ulX {}, ulY {}, ulZ {};
    Pos(rclPoint, ulX, ulY, ulZ);

    // check if the given point is inside the grid structure
    if ((ulX < _ulCtGridsX) && (ulY < _ulCtGridsY) && (ulZ < _ulCtGridsZ)) {
        return GetElements(ulX, ulY, ulZ, aulElements);
    }

    return 0;
}

// ----------------------------------------------------------------

MeshGridIterator::MeshGridIterator(const MeshGrid& rclG)
    : _rclGrid(rclG)
    , _clPt(0.0f, 0.0f, 0.0f)
    , _clDir(0.0f, 0.0f, 0.0f)
{}

bool MeshGridIterator::InitOnRay(const Base::Vector3f& rclPt,
                                 const Base::Vector3f& rclDir,
                                 float fMaxSearchArea,
                                 std::vector<ElementIndex>& raulElements)
{
    bool ret = InitOnRay(rclPt, rclDir, raulElements);
    _fMaxSearchArea = fMaxSearchArea;
    return ret;
}

bool MeshGridIterator::InitOnRay(const Base::Vector3f& rclPt,
                                 const Base::Vector3f& rclDir,
                                 std::vector<ElementIndex>& raulElements)
{
    // needed in NextOnRay() to avoid an infinite loop
    _cSearchPositions.clear();

    _fMaxSearchArea = FLOAT_MAX;

    raulElements.clear();

    _clPt = rclPt;
    _clDir = rclDir;
    _bValidRay = false;

    // point lies within global BB
    if (_rclGrid.GetBoundBox().IsInBox(rclPt)) {  // Determine the voxel by the starting point
        _rclGrid.Position(rclPt, _ulX, _ulY, _ulZ);
        raulElements.insert(raulElements.end(),
                            _rclGrid._aulGrid[_ulX][_ulY][_ulZ].begin(),
                            _rclGrid._aulGrid[_ulX][_ulY][_ulZ].end());
        _bValidRay = true;
    }
    else {  // Start point outside
        Base::Vector3f cP0, cP1;
        if (_rclGrid.GetBoundBox().IntersectWithLine(rclPt,
                                                     rclDir,
                                                     cP0,
                                                     cP1)) {  // determine the next point
            if ((cP0 - rclPt).Length() < (cP1 - rclPt).Length()) {
                _rclGrid.Position(cP0, _ulX, _ulY, _ulZ);
            }
            else {
                _rclGrid.Position(cP1, _ulX, _ulY, _ulZ);
            }

            raulElements.insert(raulElements.end(),
                                _rclGrid._aulGrid[_ulX][_ulY][_ulZ].begin(),
                                _rclGrid._aulGrid[_ulX][_ulY][_ulZ].end());
            _bValidRay = true;
        }
    }

    return _bValidRay;
}

bool MeshGridIterator::NextOnRay(std::vector<ElementIndex>& raulElements)
{
    if (!_bValidRay) {
        return false;  // not initialized or beam exited
    }

    raulElements.clear();

    Base::Vector3f clIntersectPoint;

    // Look for the next adjacent BB on the search beam
    Base::BoundBox3f::SIDE tSide =
        _rclGrid.GetBoundBox(_ulX, _ulY, _ulZ).GetSideFromRay(_clPt, _clDir, clIntersectPoint);

    // Search area
    //
    if ((_clPt - clIntersectPoint).Length() > _fMaxSearchArea) {
        _bValidRay = false;
    }
    else {
        switch (tSide) {
            case Base::BoundBox3f::LEFT:
                _ulX--;
                break;
            case Base::BoundBox3f::RIGHT:
                _ulX++;
                break;
            case Base::BoundBox3f::BOTTOM:
                _ulY--;
                break;
            case Base::BoundBox3f::TOP:
                _ulY++;
                break;
            case Base::BoundBox3f::FRONT:
                _ulZ--;
                break;
            case Base::BoundBox3f::BACK:
                _ulZ++;
                break;

            default:
            case Base::BoundBox3f::INVALID:
                _bValidRay = false;
                break;
        }

        GridElement pos(_ulX, _ulY, _ulZ);
        if (_cSearchPositions.find(pos) != _cSearchPositions.end()) {
            _bValidRay =
                false;  // grid element already visited => result from GetSideFromRay invalid
        }
    }

    if (_bValidRay && _rclGrid.CheckPos(_ulX, _ulY, _ulZ)) {
        GridElement pos(_ulX, _ulY, _ulZ);
        _cSearchPositions.insert(pos);
        raulElements.insert(raulElements.end(),
                            _rclGrid._aulGrid[_ulX][_ulY][_ulZ].begin(),
                            _rclGrid._aulGrid[_ulX][_ulY][_ulZ].end());
    }
    else {
        _bValidRay = false;  // Beam leaked
    }

    return _bValidRay;
}
