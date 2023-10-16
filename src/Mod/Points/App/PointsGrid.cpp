/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "PointsGrid.h"


using namespace Points;

PointsGrid::PointsGrid(const PointKernel& rclM)
    : _pclPoints(&rclM)
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
{
    RebuildGrid();
}

PointsGrid::PointsGrid()
    : _pclPoints(nullptr)
    , _ulCtElements(0)
    , _ulCtGridsX(POINTS_CT_GRID)
    , _ulCtGridsY(POINTS_CT_GRID)
    , _ulCtGridsZ(POINTS_CT_GRID)
    , _fGridLenX(0.0f)
    , _fGridLenY(0.0f)
    , _fGridLenZ(0.0f)
    , _fMinX(0.0f)
    , _fMinY(0.0f)
    , _fMinZ(0.0f)
{}

PointsGrid::PointsGrid(const PointKernel& rclM,
                       unsigned long ulX,
                       unsigned long ulY,
                       unsigned long ulZ)
    : _pclPoints(&rclM)
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
{
    Rebuild(ulX, ulY, ulZ);
}

PointsGrid::PointsGrid(const PointKernel& rclM, int iCtGridPerAxis)
    : _pclPoints(&rclM)
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
{
    Rebuild(iCtGridPerAxis);
}

PointsGrid::PointsGrid(const PointKernel& rclM, double fGridLen)
    : _pclPoints(&rclM)
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
{
    Base::BoundBox3d clBBPts;  // = _pclPoints->GetBoundBox();
    for (const auto& pnt : *_pclPoints) {
        clBBPts.Add(pnt);
    }
    Rebuild(std::max<unsigned long>((unsigned long)(clBBPts.LengthX() / fGridLen), 1),
            std::max<unsigned long>((unsigned long)(clBBPts.LengthY() / fGridLen), 1),
            std::max<unsigned long>((unsigned long)(clBBPts.LengthZ() / fGridLen), 1));
}

void PointsGrid::Attach(const PointKernel& rclM)
{
    _pclPoints = &rclM;
    RebuildGrid();
}

void PointsGrid::Clear()
{
    _aulGrid.clear();
    _pclPoints = nullptr;
}

void PointsGrid::Rebuild(unsigned long ulX, unsigned long ulY, unsigned long ulZ)
{
    _ulCtGridsX = ulX;
    _ulCtGridsY = ulY;
    _ulCtGridsZ = ulZ;
    _ulCtElements = HasElements();
    RebuildGrid();
}

void PointsGrid::Rebuild(unsigned long ulPerGrid, unsigned long ulMaxGrid)
{
    _ulCtElements = HasElements();
    CalculateGridLength(ulPerGrid, ulMaxGrid);
    RebuildGrid();
}

void PointsGrid::Rebuild(int iCtGridPerAxis)
{
    _ulCtElements = HasElements();
    CalculateGridLength(iCtGridPerAxis);
    RebuildGrid();
}

void PointsGrid::InitGrid()
{
    assert(_pclPoints);

    unsigned long i, j;

    // Calculate grid lengths if not initialized
    //
    if ((_ulCtGridsX == 0) || (_ulCtGridsY == 0) || (_ulCtGridsZ == 0)) {
        CalculateGridLength(POINTS_CT_GRID, POINTS_MAX_GRIDS);
    }

    // Determine the grid length and offset
    //
    {
        Base::BoundBox3d clBBPts;  // = _pclPoints->GetBoundBox();
        for (const auto& pnt : *_pclPoints) {
            clBBPts.Add(pnt);
        }

        double fLengthX = clBBPts.LengthX();
        double fLengthY = clBBPts.LengthY();
        double fLengthZ = clBBPts.LengthZ();

        {
            // Offset fGridLen/2
            //
            unsigned long num = _ulCtGridsX;
            if (num == 0) {
                num = 1;
            }
            _fGridLenX = (1.0f + fLengthX) / double(num);
            _fMinX = clBBPts.MinX - 0.5f;
        }

        {
            unsigned long num = _ulCtGridsY;
            if (num == 0) {
                num = 1;
            }
            _fGridLenY = (1.0f + fLengthY) / double(num);
            _fMinY = clBBPts.MinY - 0.5f;
        }

        {
            unsigned long num = _ulCtGridsZ;
            if (num == 0) {
                num = 1;
            }
            _fGridLenZ = (1.0f + fLengthZ) / double(num);
            _fMinZ = clBBPts.MinZ - 0.5f;
        }
    }

    // Create data structure
    _aulGrid.clear();
    _aulGrid.resize(_ulCtGridsX);
    for (i = 0; i < _ulCtGridsX; i++) {
        _aulGrid[i].resize(_ulCtGridsY);
        for (j = 0; j < _ulCtGridsY; j++) {
            _aulGrid[i][j].resize(_ulCtGridsZ);
        }
    }
}

unsigned long PointsGrid::InSide(const Base::BoundBox3d& rclBB,
                                 std::vector<unsigned long>& raulElements,
                                 bool bDelDoubles) const
{
    unsigned long i, j, k, ulMinX, ulMinY, ulMinZ, ulMaxX, ulMaxY, ulMaxZ;

    raulElements.clear();

    // Grid boxes for a more detailed selection
    Position(Base::Vector3d(rclBB.MinX, rclBB.MinY, rclBB.MinZ), ulMinX, ulMinY, ulMinZ);
    Position(Base::Vector3d(rclBB.MaxX, rclBB.MaxY, rclBB.MaxZ), ulMaxX, ulMaxY, ulMaxZ);

    for (i = ulMinX; i <= ulMaxX; i++) {
        for (j = ulMinY; j <= ulMaxY; j++) {
            for (k = ulMinZ; k <= ulMaxZ; k++) {
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

unsigned long PointsGrid::InSide(const Base::BoundBox3d& rclBB,
                                 std::vector<unsigned long>& raulElements,
                                 const Base::Vector3d& rclOrg,
                                 double fMaxDist,
                                 bool bDelDoubles) const
{
    unsigned long i, j, k, ulMinX, ulMinY, ulMinZ, ulMaxX, ulMaxY, ulMaxZ;
    double fGridDiag = GetBoundBox(0, 0, 0).CalcDiagonalLength();
    double fMinDistP2 = (fGridDiag * fGridDiag) + (fMaxDist * fMaxDist);

    raulElements.clear();

    // Grid boxes for a more detailed selection
    Position(Base::Vector3d(rclBB.MinX, rclBB.MinY, rclBB.MinZ), ulMinX, ulMinY, ulMinZ);
    Position(Base::Vector3d(rclBB.MaxX, rclBB.MaxY, rclBB.MaxZ), ulMaxX, ulMaxY, ulMaxZ);

    for (i = ulMinX; i <= ulMaxX; i++) {
        for (j = ulMinY; j <= ulMaxY; j++) {
            for (k = ulMinZ; k <= ulMaxZ; k++) {
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

unsigned long PointsGrid::InSide(const Base::BoundBox3d& rclBB,
                                 std::set<unsigned long>& raulElements) const
{
    unsigned long i, j, k, ulMinX, ulMinY, ulMinZ, ulMaxX, ulMaxY, ulMaxZ;

    raulElements.clear();

    // Grid boxes for a more detailed selection
    Position(Base::Vector3d(rclBB.MinX, rclBB.MinY, rclBB.MinZ), ulMinX, ulMinY, ulMinZ);
    Position(Base::Vector3d(rclBB.MaxX, rclBB.MaxY, rclBB.MaxZ), ulMaxX, ulMaxY, ulMaxZ);

    for (i = ulMinX; i <= ulMaxX; i++) {
        for (j = ulMinY; j <= ulMaxY; j++) {
            for (k = ulMinZ; k <= ulMaxZ; k++) {
                raulElements.insert(_aulGrid[i][j][k].begin(), _aulGrid[i][j][k].end());
            }
        }
    }

    return raulElements.size();
}

void PointsGrid::Position(const Base::Vector3d& rclPoint,
                          unsigned long& rulX,
                          unsigned long& rulY,
                          unsigned long& rulZ) const
{
    if (rclPoint.x <= _fMinX) {
        rulX = 0;
    }
    else {
        rulX = std::min<unsigned long>((unsigned long)((rclPoint.x - _fMinX) / _fGridLenX),
                                       _ulCtGridsX - 1);
    }

    if (rclPoint.y <= _fMinY) {
        rulY = 0;
    }
    else {
        rulY = std::min<unsigned long>((unsigned long)((rclPoint.y - _fMinY) / _fGridLenY),
                                       _ulCtGridsY - 1);
    }

    if (rclPoint.z <= _fMinZ) {
        rulZ = 0;
    }
    else {
        rulZ = std::min<unsigned long>((unsigned long)((rclPoint.z - _fMinZ) / _fGridLenZ),
                                       _ulCtGridsZ - 1);
    }
}

void PointsGrid::CalculateGridLength(unsigned long ulCtGrid, unsigned long ulMaxGrids)
{
    // Calculate grid lengths or number of grids per dimension
    // There should be about 10 (?!?!) facets per grid
    // or max grids should not exceed 10000
    Base::BoundBox3d clBBPtsEnlarged;  // = _pclPoints->GetBoundBox();
    for (const auto& pnt : *_pclPoints) {
        clBBPtsEnlarged.Add(pnt);
    }
    double fVolElem;

    if (_ulCtElements > (ulMaxGrids * ulCtGrid)) {
        fVolElem =
            (clBBPtsEnlarged.LengthX() * clBBPtsEnlarged.LengthY() * clBBPtsEnlarged.LengthZ())
            / float(ulMaxGrids * ulCtGrid);
    }
    else {
        fVolElem =
            (clBBPtsEnlarged.LengthX() * clBBPtsEnlarged.LengthY() * clBBPtsEnlarged.LengthZ())
            / float(_ulCtElements);
    }

    double fVol = fVolElem * float(ulCtGrid);
    double fGridLen = float(pow((float)fVol, (float)1.0f / 3.0f));

    if (fGridLen > 0) {
        _ulCtGridsX =
            std::max<unsigned long>((unsigned long)(clBBPtsEnlarged.LengthX() / fGridLen), 1);
        _ulCtGridsY =
            std::max<unsigned long>((unsigned long)(clBBPtsEnlarged.LengthY() / fGridLen), 1);
        _ulCtGridsZ =
            std::max<unsigned long>((unsigned long)(clBBPtsEnlarged.LengthZ() / fGridLen), 1);
    }
    else {
        // Degenerated grid
        _ulCtGridsX = 1;
        _ulCtGridsY = 1;
        _ulCtGridsZ = 1;
    }
}

void PointsGrid::CalculateGridLength(int iCtGridPerAxis)
{
    if (iCtGridPerAxis <= 0) {
        CalculateGridLength(POINTS_CT_GRID, POINTS_MAX_GRIDS);
        return;
    }

    // Calculate grid lengths or number of grids per dimension
    // There should be about 10 (?!?!) facets per grid
    // or max grids should not exceed 10000
    Base::BoundBox3d clBBPts;  // = _pclPoints->GetBoundBox();
    for (const auto& pnt : *_pclPoints) {
        clBBPts.Add(pnt);
    }

    double fLenghtX = clBBPts.LengthX();
    double fLenghtY = clBBPts.LengthY();
    double fLenghtZ = clBBPts.LengthZ();

    double fLenghtD = clBBPts.CalcDiagonalLength();

    double fLengthTol = 0.05f * fLenghtD;

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

    double fFactorVolumen = 40.0;
    double fFactorArea = 10.0;

    switch (iFlag) {
        case 0: {
            double fVolumen = fLenghtX * fLenghtY * fLenghtZ;

            double fVolumenGrid = (fVolumen * ulGridsFacets) / (fFactorVolumen * _ulCtElements);

            if ((fVolumenGrid * iMaxGrids) < fVolumen) {
                fVolumenGrid = fVolumen / (float)iMaxGrids;
            }

            double fLengthGrid = float(pow((float)fVolumenGrid, (float)1.0f / 3.0f));

            _ulCtGridsX = std::max<unsigned long>((unsigned long)(fLenghtX / fLengthGrid), 1);
            _ulCtGridsY = std::max<unsigned long>((unsigned long)(fLenghtY / fLengthGrid), 1);
            _ulCtGridsZ = std::max<unsigned long>((unsigned long)(fLenghtZ / fLengthGrid), 1);

        } break;
        case 1: {
            _ulCtGridsX = 1;  // bLenghtXisZero

            double fArea = fLenghtY * fLenghtZ;

            double fAreaGrid = (fArea * ulGridsFacets) / (fFactorArea * _ulCtElements);

            if ((fAreaGrid * iMaxGrids) < fArea) {
                fAreaGrid = fArea / (double)iMaxGrids;
            }

            double fLengthGrid = double(sqrt(fAreaGrid));

            _ulCtGridsY = std::max<unsigned long>((unsigned long)(fLenghtY / fLengthGrid), 1);
            _ulCtGridsZ = std::max<unsigned long>((unsigned long)(fLenghtZ / fLengthGrid), 1);
        } break;
        case 2: {
            _ulCtGridsY = 1;  // bLenghtYisZero

            double fArea = fLenghtX * fLenghtZ;

            double fAreaGrid = (fArea * ulGridsFacets) / (fFactorArea * _ulCtElements);

            if ((fAreaGrid * iMaxGrids) < fArea) {
                fAreaGrid = fArea / (double)iMaxGrids;
            }

            double fLengthGrid = double(sqrt(fAreaGrid));

            _ulCtGridsX = std::max<unsigned long>((unsigned long)(fLenghtX / fLengthGrid), 1);
            _ulCtGridsZ = std::max<unsigned long>((unsigned long)(fLenghtZ / fLengthGrid), 1);
        } break;
        case 3: {
            _ulCtGridsX = 1;          // bLenghtXisZero
            _ulCtGridsY = 1;          // bLenghtYisZero
            _ulCtGridsZ = iMaxGrids;  // bLenghtYisZero
        } break;
        case 4: {
            _ulCtGridsZ = 1;  // bLenghtZisZero

            double fArea = fLenghtX * fLenghtY;

            double fAreaGrid = (fArea * ulGridsFacets) / (fFactorArea * _ulCtElements);

            if ((fAreaGrid * iMaxGrids) < fArea) {
                fAreaGrid = fArea / (float)iMaxGrids;
            }

            double fLengthGrid = double(sqrt(fAreaGrid));

            _ulCtGridsX = std::max<unsigned long>((unsigned long)(fLenghtX / fLengthGrid), 1);
            _ulCtGridsY = std::max<unsigned long>((unsigned long)(fLenghtY / fLengthGrid), 1);
        } break;
        case 5: {
            _ulCtGridsX = 1;          // bLenghtXisZero
            _ulCtGridsZ = 1;          // bLenghtZisZero
            _ulCtGridsY = iMaxGrids;  // bLenghtYisZero
        } break;
        case 6: {
            _ulCtGridsY = 1;          // bLenghtYisZero
            _ulCtGridsZ = 1;          // bLenghtZisZero
            _ulCtGridsX = iMaxGrids;  // bLenghtYisZero
        } break;
        case 7: {
            _ulCtGridsX = iMaxGrids;  // bLenghtXisZero
            _ulCtGridsY = iMaxGrids;  // bLenghtYisZero
            _ulCtGridsZ = iMaxGrids;  // bLenghtZisZero
        } break;
    }
}

void PointsGrid::SearchNearestFromPoint(const Base::Vector3d& rclPt,
                                        std::set<unsigned long>& raclInd) const
{
    raclInd.clear();
    Base::BoundBox3d clBB = GetBoundBox();

    if (clBB.IsInBox(rclPt)) {  // Point lies within
        unsigned long ulX, ulY, ulZ;
        Position(rclPt, ulX, ulY, ulZ);
        // int nX = ulX, nY = ulY, nZ = ulZ;
        unsigned long ulLevel = 0;
        while (raclInd.empty()) {
            GetHull(ulX, ulY, ulZ, ulLevel++, raclInd);
        }
        GetHull(ulX, ulY, ulZ, ulLevel, raclInd);
    }
    else {  // Point outside
        Base::BoundBox3d::SIDE tSide = clBB.GetSideFromRay(rclPt, clBB.GetCenter() - rclPt);
        switch (tSide) {
            case Base::BoundBox3d::RIGHT: {
                int nX = 0;
                while (raclInd.empty()) {
                    for (unsigned long i = 0; i < _ulCtGridsY; i++) {
                        for (unsigned long j = 0; j < _ulCtGridsZ; j++) {
                            raclInd.insert(_aulGrid[nX][i][j].begin(), _aulGrid[nX][i][j].end());
                        }
                    }
                    nX++;
                }
                break;
            }
            case Base::BoundBox3d::LEFT: {
                int nX = _ulCtGridsX - 1;
                while (raclInd.empty()) {
                    for (unsigned long i = 0; i < _ulCtGridsY; i++) {
                        for (unsigned long j = 0; j < _ulCtGridsZ; j++) {
                            raclInd.insert(_aulGrid[nX][i][j].begin(), _aulGrid[nX][i][j].end());
                        }
                    }
                    nX++;
                }
                break;
            }
            case Base::BoundBox3d::TOP: {
                int nY = 0;
                while (raclInd.empty()) {
                    for (unsigned long i = 0; i < _ulCtGridsX; i++) {
                        for (unsigned long j = 0; j < _ulCtGridsZ; j++) {
                            raclInd.insert(_aulGrid[i][nY][j].begin(), _aulGrid[i][nY][j].end());
                        }
                    }
                    nY++;
                }
                break;
            }
            case Base::BoundBox3d::BOTTOM: {
                int nY = _ulCtGridsY - 1;
                while (raclInd.empty()) {
                    for (unsigned long i = 0; i < _ulCtGridsX; i++) {
                        for (unsigned long j = 0; j < _ulCtGridsZ; j++) {
                            raclInd.insert(_aulGrid[i][nY][j].begin(), _aulGrid[i][nY][j].end());
                        }
                    }
                    nY--;
                }
                break;
            }
            case Base::BoundBox3d::BACK: {
                int nZ = 0;
                while (raclInd.empty()) {
                    for (unsigned long i = 0; i < _ulCtGridsX; i++) {
                        for (unsigned long j = 0; j < _ulCtGridsY; j++) {
                            raclInd.insert(_aulGrid[i][j][nZ].begin(), _aulGrid[i][j][nZ].end());
                        }
                    }
                    nZ++;
                }
                break;
            }
            case Base::BoundBox3d::FRONT: {
                int nZ = _ulCtGridsZ - 1;
                while (raclInd.empty()) {
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

void PointsGrid::GetHull(unsigned long ulX,
                         unsigned long ulY,
                         unsigned long ulZ,
                         unsigned long ulDistance,
                         std::set<unsigned long>& raclInd) const
{
    int nX1 = std::max<int>(0, int(ulX) - int(ulDistance));
    int nY1 = std::max<int>(0, int(ulY) - int(ulDistance));
    int nZ1 = std::max<int>(0, int(ulZ) - int(ulDistance));
    int nX2 = std::min<int>(int(_ulCtGridsX) - 1, int(ulX) + int(ulDistance));
    int nY2 = std::min<int>(int(_ulCtGridsY) - 1, int(ulY) + int(ulDistance));
    int nZ2 = std::min<int>(int(_ulCtGridsZ) - 1, int(ulZ) + int(ulDistance));

    int i, j;

    // top plane
    for (i = nX1; i <= nX2; i++) {
        for (j = nY1; j <= nY2; j++) {
            GetElements(i, j, nZ1, raclInd);
        }
    }
    // bottom plane
    for (i = nX1; i <= nX2; i++) {
        for (j = nY1; j <= nY2; j++) {
            GetElements(i, j, nZ2, raclInd);
        }
    }
    // left plane
    for (i = nY1; i <= nY2; i++) {
        for (j = (nZ1 + 1); j <= (nZ2 - 1); j++) {
            GetElements(nX1, i, j, raclInd);
        }
    }
    // right plane
    for (i = nY1; i <= nY2; i++) {
        for (j = (nZ1 + 1); j <= (nZ2 - 1); j++) {
            GetElements(nX2, i, j, raclInd);
        }
    }
    // front plane
    for (i = (nX1 + 1); i <= (nX2 - 1); i++) {
        for (j = (nZ1 + 1); j <= (nZ2 - 1); j++) {
            GetElements(i, nY1, j, raclInd);
        }
    }
    // back plane
    for (i = (nX1 + 1); i <= (nX2 - 1); i++) {
        for (j = (nZ1 + 1); j <= (nZ2 - 1); j++) {
            GetElements(i, nY2, j, raclInd);
        }
    }
}

unsigned long PointsGrid::GetElements(unsigned long ulX,
                                      unsigned long ulY,
                                      unsigned long ulZ,
                                      std::set<unsigned long>& raclInd) const
{
    const std::set<unsigned long>& rclSet = _aulGrid[ulX][ulY][ulZ];
    if (!rclSet.empty()) {
        raclInd.insert(rclSet.begin(), rclSet.end());
        return rclSet.size();
    }

    return 0;
}

void PointsGrid::AddPoint(const Base::Vector3d& rclPt, unsigned long ulPtIndex, float /*fEpsilon*/)
{
    unsigned long ulX, ulY, ulZ;
    Pos(Base::Vector3d(rclPt.x, rclPt.y, rclPt.z), ulX, ulY, ulZ);
    if ((ulX < _ulCtGridsX) && (ulY < _ulCtGridsY) && (ulZ < _ulCtGridsZ)) {
        _aulGrid[ulX][ulY][ulZ].insert(ulPtIndex);
    }
}

void PointsGrid::Validate(const PointKernel& rclPoints)
{
    if (_pclPoints != &rclPoints) {
        Attach(rclPoints);
    }
    else if (rclPoints.size() != _ulCtElements) {
        RebuildGrid();
    }
}

void PointsGrid::Validate()
{
    if (!_pclPoints) {
        return;
    }

    if (_pclPoints->size() != _ulCtElements) {
        RebuildGrid();
    }
}

bool PointsGrid::Verify() const
{
    if (!_pclPoints) {
        return false;  // no point cloud attached
    }
    if (_pclPoints->size() != _ulCtElements) {
        return false;  // not up-to-date
    }

    PointsGridIterator it(*this);
    for (it.Init(); it.More(); it.Next()) {
        std::vector<unsigned long> aulElements;
        it.GetElements(aulElements);
        for (unsigned long element : aulElements) {
            const Base::Vector3d& cP = _pclPoints->getPoint(element);
            if (!it.GetBoundBox().IsInBox(cP)) {
                return false;  // point doesn't lie inside the grid element
            }
        }
    }

    return true;
}

void PointsGrid::RebuildGrid()
{
    _ulCtElements = _pclPoints->size();

    InitGrid();

    // Fill data structure

    unsigned long i = 0;
    for (const auto& pnt : *_pclPoints) {
        AddPoint(pnt, i++);
    }
}

void PointsGrid::Pos(const Base::Vector3d& rclPoint,
                     unsigned long& rulX,
                     unsigned long& rulY,
                     unsigned long& rulZ) const
{
    rulX = (unsigned long)((rclPoint.x - _fMinX) / _fGridLenX);
    rulY = (unsigned long)((rclPoint.y - _fMinY) / _fGridLenY);
    rulZ = (unsigned long)((rclPoint.z - _fMinZ) / _fGridLenZ);
}

unsigned long PointsGrid::FindElements(const Base::Vector3d& rclPoint,
                                       std::set<unsigned long>& aulElements) const
{
    unsigned long ulX, ulY, ulZ;
    Pos(rclPoint, ulX, ulY, ulZ);

    // check if the given point is inside the grid structure
    if ((ulX < _ulCtGridsX) && (ulY < _ulCtGridsY) && (ulZ < _ulCtGridsZ)) {
        return GetElements(ulX, ulY, ulZ, aulElements);
    }

    return 0;
}

// ----------------------------------------------------------------

PointsGridIterator::PointsGridIterator(const PointsGrid& rclG)
    : _rclGrid(rclG)
    , _clPt(0.0f, 0.0f, 0.0f)
    , _clDir(0.0f, 0.0f, 0.0f)
{}

bool PointsGridIterator::InitOnRay(const Base::Vector3d& rclPt,
                                   const Base::Vector3d& rclDir,
                                   float fMaxSearchArea,
                                   std::vector<unsigned long>& raulElements)
{
    bool ret = InitOnRay(rclPt, rclDir, raulElements);
    _fMaxSearchArea = fMaxSearchArea;
    return ret;
}

bool PointsGridIterator::InitOnRay(const Base::Vector3d& rclPt,
                                   const Base::Vector3d& rclDir,
                                   std::vector<unsigned long>& raulElements)
{
    // needed in NextOnRay() to avoid an infinite loop
    _cSearchPositions.clear();

    _fMaxSearchArea = FLOAT_MAX;

    raulElements.clear();

    _clPt = rclPt;
    _clDir = rclDir;
    _bValidRay = false;

    // point lies within global BB
    if (_rclGrid.GetBoundBox().IsInBox(rclPt)) {  // determine the voxel by the starting point
        _rclGrid.Position(rclPt, _ulX, _ulY, _ulZ);
        raulElements.insert(raulElements.end(),
                            _rclGrid._aulGrid[_ulX][_ulY][_ulZ].begin(),
                            _rclGrid._aulGrid[_ulX][_ulY][_ulZ].end());
        _bValidRay = true;
    }
    else {  // StartPoint outside
        Base::Vector3d cP0, cP1;
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

bool PointsGridIterator::NextOnRay(std::vector<unsigned long>& raulElements)
{
    if (!_bValidRay) {
        return false;  // not initialized or beam exited
    }

    raulElements.clear();

    Base::Vector3d clIntersectPoint;

    // Look for the next adjacent BB on the search beam
    Base::BoundBox3d::SIDE tSide =
        _rclGrid.GetBoundBox(_ulX, _ulY, _ulZ).GetSideFromRay(_clPt, _clDir, clIntersectPoint);

    // Search area
    //
    if ((_clPt - clIntersectPoint).Length() > _fMaxSearchArea) {
        _bValidRay = false;
    }
    else {
        switch (tSide) {
            case Base::BoundBox3d::LEFT:
                _ulX--;
                break;
            case Base::BoundBox3d::RIGHT:
                _ulX++;
                break;
            case Base::BoundBox3d::BOTTOM:
                _ulY--;
                break;
            case Base::BoundBox3d::TOP:
                _ulY++;
                break;
            case Base::BoundBox3d::FRONT:
                _ulZ--;
                break;
            case Base::BoundBox3d::BACK:
                _ulZ++;
                break;

            default:
            case Base::BoundBox3d::INVALID:
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
        _bValidRay = false;  // ray exited
    }

    return _bValidRay;
}
