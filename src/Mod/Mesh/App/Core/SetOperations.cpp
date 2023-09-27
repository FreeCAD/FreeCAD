/***************************************************************************
 *   Copyright (c) 2005 Berthold Grupp                                     *
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
#include <fstream>
#include <ios>
#endif

#include <Base/Builder3D.h>
#include <Base/Sequencer.h>

#include "Algorithm.h"
#include "Builder.h"
#include "Definitions.h"
#include "Elements.h"
#include "Grid.h"
#include "Iterator.h"
#include "SetOperations.h"
#include "Triangulation.h"
#include "Visitor.h"


using namespace Base;
using namespace MeshCore;


SetOperations::SetOperations(const MeshKernel& cutMesh1,
                             const MeshKernel& cutMesh2,
                             MeshKernel& result,
                             OperationType opType,
                             float minDistanceToPoint)
    : _cutMesh0(cutMesh1)
    , _cutMesh1(cutMesh2)
    , _resultMesh(result)
    , _operationType(opType)
    , _minDistanceToPoint(minDistanceToPoint)
{}

void SetOperations::Do()
{
    _minDistanceToPoint = 0.000001f;
    float saveMinMeshDistance = MeshDefinitions::_fMinPointDistance;
    MeshDefinitions::SetMinPointDistance(0.000001f);

    //  Base::Sequencer().start("set operation", 5);

    // _builder.clear();

    // Base::Sequencer().next();
    std::set<FacetIndex> facetsCuttingEdge0, facetsCuttingEdge1;
    Cut(facetsCuttingEdge0, facetsCuttingEdge1);

    // no intersection curve of the meshes found
    if (facetsCuttingEdge0.empty() || facetsCuttingEdge1.empty()) {
        switch (_operationType) {
            case Union: {
                _resultMesh = _cutMesh0;
                _resultMesh.Merge(_cutMesh1);
            } break;
            case Intersect: {
                _resultMesh.Clear();
            } break;
            case Difference:
            case Inner:
            case Outer: {
                _resultMesh = _cutMesh0;
            } break;
            default: {
                _resultMesh.Clear();
                break;
            }
        }

        MeshDefinitions::SetMinPointDistance(saveMinMeshDistance);
        return;
    }

    for (auto i = 0UL; i < _cutMesh0.CountFacets(); i++) {
        if (facetsCuttingEdge0.find(i) == facetsCuttingEdge0.end()) {
            _newMeshFacets[0].push_back(_cutMesh0.GetFacet(i));
        }
    }

    for (auto i = 0UL; i < _cutMesh1.CountFacets(); i++) {
        if (facetsCuttingEdge1.find(i) == facetsCuttingEdge1.end()) {
            _newMeshFacets[1].push_back(_cutMesh1.GetFacet(i));
        }
    }

    // Base::Sequencer().next();
    TriangulateMesh(_cutMesh0, 0);

    // Base::Sequencer().next();
    TriangulateMesh(_cutMesh1, 1);

    float mult0 {}, mult1 {};
    switch (_operationType) {
        case Union:
            mult0 = -1.0f;
            mult1 = -1.0f;
            break;
        case Intersect:
            mult0 = 1.0f;
            mult1 = 1.0f;
            break;
        case Difference:
            mult0 = -1.0f;
            mult1 = 1.0f;
            break;
        case Inner:
            mult0 = 1.0f;
            mult1 = 0.0f;
            break;
        case Outer:
            mult0 = -1.0f;
            mult1 = 0.0f;
            break;
        default:
            mult0 = 0.0f;
            mult1 = 0.0f;
            break;
    }

    // Base::Sequencer().next();
    CollectFacets(0, mult0);
    // Base::Sequencer().next();
    CollectFacets(1, mult1);

    std::vector<MeshGeomFacet> facets;

    std::vector<MeshGeomFacet>::iterator itf;
    for (itf = _facetsOf[0].begin(); itf != _facetsOf[0].end(); ++itf) {
        if (_operationType == Difference) {  // toggle normal
            std::swap(itf->_aclPoints[0], itf->_aclPoints[1]);
            itf->CalcNormal();
        }

        facets.push_back(*itf);
    }

    for (itf = _facetsOf[1].begin(); itf != _facetsOf[1].end(); ++itf) {
        facets.push_back(*itf);
    }

    _resultMesh = facets;

    // Base::Sequencer().stop();
    // _builder.saveToFile("c:/temp/vdbg.iv");

    MeshDefinitions::SetMinPointDistance(saveMinMeshDistance);
}

void SetOperations::Cut(std::set<FacetIndex>& facetsCuttingEdge0,
                        std::set<FacetIndex>& facetsCuttingEdge1)
{
    MeshFacetGrid grid1(_cutMesh0, 20);
    MeshFacetGrid grid2(_cutMesh1, 20);

    unsigned long ctGx1 {}, ctGy1 {}, ctGz1 {};
    grid1.GetCtGrids(ctGx1, ctGy1, ctGz1);

    for (auto gx1 = 0UL; gx1 < ctGx1; gx1++) {
        for (auto gy1 = 0UL; gy1 < ctGy1; gy1++) {
            for (auto gz1 = 0UL; gz1 < ctGz1; gz1++) {
                if (grid1.GetCtElements(gx1, gy1, gz1) > 0) {
                    std::vector<FacetIndex> vecFacets2;
                    grid2.Inside(grid1.GetBoundBox(gx1, gy1, gz1), vecFacets2);

                    if (!vecFacets2.empty()) {
                        std::set<FacetIndex> vecFacets1;
                        grid1.GetElements(gx1, gy1, gz1, vecFacets1);

                        std::set<FacetIndex>::iterator it1;
                        for (it1 = vecFacets1.begin(); it1 != vecFacets1.end(); ++it1) {
                            FacetIndex fidx1 = *it1;
                            MeshGeomFacet f1 = _cutMesh0.GetFacet(*it1);

                            std::vector<FacetIndex>::iterator it2;
                            for (it2 = vecFacets2.begin(); it2 != vecFacets2.end(); ++it2) {
                                FacetIndex fidx2 = *it2;
                                MeshGeomFacet f2 = _cutMesh1.GetFacet(fidx2);

                                MeshPoint p0, p1;

                                int isect = f1.IntersectWithFacet(f2, p0, p1);
                                if (isect > 0) {
                                    // optimize cut line if distance to nearest point is too small
                                    float minDist1 = _minDistanceToPoint,
                                          minDist2 = _minDistanceToPoint;
                                    MeshPoint np0 = p0, np1 = p1;
                                    for (int i = 0; i < 3; i++)  // NOLINT
                                    {
                                        float d1 = (f1._aclPoints[i] - p0).Length();
                                        float d2 = (f1._aclPoints[i] - p1).Length();
                                        if (d1 < minDist1) {
                                            minDist1 = d1;
                                            np0 = f1._aclPoints[i];
                                        }
                                        if (d2 < minDist2) {
                                            minDist2 = d2;
                                            p1 = f1._aclPoints[i];
                                        }
                                    }  // for (int i = 0; i < 3; i++)

                                    // optimize cut line if distance to nearest point is too small
                                    for (int i = 0; i < 3; i++)  // NOLINT
                                    {
                                        float d1 = (f2._aclPoints[i] - p0).Length();
                                        float d2 = (f2._aclPoints[i] - p1).Length();
                                        if (d1 < minDist1) {
                                            minDist1 = d1;
                                            np0 = f2._aclPoints[i];
                                        }
                                        if (d2 < minDist2) {
                                            minDist2 = d2;
                                            np1 = f2._aclPoints[i];
                                        }
                                    }  // for (int i = 0; i < 3; i++)

                                    MeshPoint mp0 = np0;
                                    MeshPoint mp1 = np1;

                                    if (mp0 != mp1) {
                                        facetsCuttingEdge0.insert(fidx1);
                                        facetsCuttingEdge1.insert(fidx2);

                                        _cutPoints.insert(mp0);
                                        _cutPoints.insert(mp1);

                                        std::pair<std::set<MeshPoint>::iterator, bool> pit0 =
                                            _cutPoints.insert(mp0);
                                        std::pair<std::set<MeshPoint>::iterator, bool> pit1 =
                                            _cutPoints.insert(mp1);

                                        _edges[Edge(mp0, mp1)] = EdgeInfo();

                                        _facet2points[0][fidx1].push_back(pit0.first);
                                        _facet2points[0][fidx1].push_back(pit1.first);
                                        _facet2points[1][fidx2].push_back(pit0.first);
                                        _facet2points[1][fidx2].push_back(pit1.first);
                                    }
                                    else {
                                        std::pair<std::set<MeshPoint>::iterator, bool> pit =
                                            _cutPoints.insert(mp0);

                                        // do not insert a facet when only one corner point cuts the
                                        // edge if (!((mp0 == f1._aclPoints[0]) || (mp0 ==
                                        // f1._aclPoints[1]) || (mp0 == f1._aclPoints[2])))
                                        {
                                            facetsCuttingEdge0.insert(fidx1);
                                            _facet2points[0][fidx1].push_back(pit.first);
                                        }

                                        // if (!((mp0 == f2._aclPoints[0]) || (mp0 ==
                                        // f2._aclPoints[1]) || (mp0 == f2._aclPoints[2])))
                                        {
                                            facetsCuttingEdge1.insert(fidx2);
                                            _facet2points[1][fidx2].push_back(pit.first);
                                        }
                                    }

                                }  // if (f1.IntersectWithFacet(f2, p0, p1))
                            }      // for (it2 = vecFacets2.begin(); it2 != vecFacets2.end(); ++it2)
                        }          // for (it1 = vecFacets1.begin(); it1 != vecFacets1.end(); ++it1)
                    }              // if (vecFacets2.size() > 0)
                }                  // if (grid1.GetCtElements(gx1, gy1, gz1) > 0)
            }                      // for (gz1 = 0; gz1 < ctGz1; gz1++)
        }                          // for (gy1 = 0; gy1 < ctGy1; gy1++)
    }                              // for (gx1 = 0; gx1 < ctGx1; gx1++)
}

void SetOperations::TriangulateMesh(const MeshKernel& cutMesh, int side)
{
    // Triangulate Mesh
    std::map<FacetIndex, std::list<std::set<MeshPoint>::iterator>>::iterator it1;
    for (it1 = _facet2points[side].begin(); it1 != _facet2points[side].end(); ++it1) {
        std::vector<Vector3f> points;
        std::set<MeshPoint> pointsSet;

        FacetIndex fidx = it1->first;
        MeshGeomFacet f = cutMesh.GetFacet(fidx);

        // if (side == 1)
        //     _builder.addSingleTriangle(f._aclPoints[0], f._aclPoints[1], f._aclPoints[2], 3, 0,
        //     1, 1);

        // facet corner points
        // const MeshFacet& mf = cutMesh._aclFacetArray[fidx];
        for (int i = 0; i < 3; i++)  // NOLINT
        {
            pointsSet.insert(f._aclPoints[i]);
            points.push_back(f._aclPoints[i]);
        }

        // triangulated facets
        std::list<std::set<MeshPoint>::iterator>::iterator it2;
        for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2) {
            if (pointsSet.find(*(*it2)) == pointsSet.end()) {
                pointsSet.insert(*(*it2));
                points.push_back(*(*it2));
            }
        }

        Vector3f normal = f.GetNormal();
        Vector3f base = points[0];
        Vector3f dirX = points[1] - points[0];
        dirX.Normalize();
        Vector3f dirY = dirX % normal;

        // project points to 2D plane
        std::vector<Vector3f>::iterator it;
        std::vector<Vector3f> vertices;
        for (it = points.begin(); it != points.end(); ++it) {
            Vector3f pv = *it;
            pv.TransformToCoordinateSystem(base, dirX, dirY);
            vertices.push_back(pv);
        }

        DelaunayTriangulator tria;
        tria.SetPolygon(vertices);
        tria.TriangulatePolygon();

        std::vector<MeshFacet> facets = tria.GetFacets();
        for (auto& it : facets) {
            if ((it._aulPoints[0] == it._aulPoints[1]) || (it._aulPoints[1] == it._aulPoints[2])
                || (it._aulPoints[2] == it._aulPoints[0])) {  // two same triangle corner points
                continue;
            }

            MeshGeomFacet facet(points[it._aulPoints[0]],
                                points[it._aulPoints[1]],
                                points[it._aulPoints[2]]);

            // if (side == 1)
            //  _builder.addSingleTriangle(facet._aclPoints[0], facet._aclPoints[1],
            //  facet._aclPoints[2], true, 3, 0, 1, 1);

            // if (facet.Area() < 0.0001f)
            //{ // too small facet
            //   continue;
            // }

            float dist0 =
                facet._aclPoints[0].DistanceToLine(facet._aclPoints[1],
                                                   facet._aclPoints[1] - facet._aclPoints[2]);
            float dist1 =
                facet._aclPoints[1].DistanceToLine(facet._aclPoints[0],
                                                   facet._aclPoints[0] - facet._aclPoints[2]);
            float dist2 =
                facet._aclPoints[2].DistanceToLine(facet._aclPoints[0],
                                                   facet._aclPoints[0] - facet._aclPoints[1]);

            if ((dist0 < _minDistanceToPoint) || (dist1 < _minDistanceToPoint)
                || (dist2 < _minDistanceToPoint)) {
                continue;
            }

            // dist0 = (facet._aclPoints[0] - facet._aclPoints[1]).Length();
            // dist1 = (facet._aclPoints[1] - facet._aclPoints[2]).Length();
            // dist2 = (facet._aclPoints[2] - facet._aclPoints[3]).Length();

            // if ((dist0 < _minDistanceToPoint) || (dist1 < _minDistanceToPoint) || (dist2 <
            // _minDistanceToPoint))
            //{
            //   continue;
            // }

            facet.CalcNormal();
            if ((facet.GetNormal() * f.GetNormal()) < 0.0f) {  // adjust normal
                std::swap(facet._aclPoints[0], facet._aclPoints[1]);
                facet.CalcNormal();
            }


            for (int j = 0; j < 3; j++) {
                std::map<Edge, EdgeInfo>::iterator eit =
                    _edges.find(Edge(facet._aclPoints[j], facet._aclPoints[(j + 1) % 3]));

                if (eit != _edges.end()) {

                    if (eit->second.fcounter[side] < 2) {
                        // if (side == 0)
                        //    _builder.addSingleTriangle(facet._aclPoints[0], facet._aclPoints[1],
                        //    facet._aclPoints[2], true, 3, 0, 1, 1);

                        eit->second.facet[side] = fidx;
                        eit->second.facets[side][eit->second.fcounter[side]] = facet;
                        eit->second.fcounter[side]++;
                        facet.SetFlag(
                            MeshFacet::MARKED);  // set all facets connected to an edge: MARKED
                    }
                }
            }

            _newMeshFacets[side].push_back(facet);

        }  // for (i = 0; i < (out->numberoftriangles * 3); i += 3)
    }      // for (it1 = _facet2points[side].begin(); it1 != _facet2points[side].end(); ++it1)
}

void SetOperations::CollectFacets(int side, float mult)
{
    // float distSave = MeshDefinitions::_fMinPointDistance;
    // MeshDefinitions::SetMinPointDistance(1.0e-4f);

    MeshKernel mesh;
    MeshBuilder mb(mesh);
    mb.Initialize(_newMeshFacets[side].size());
    std::vector<MeshGeomFacet>::iterator it;
    for (it = _newMeshFacets[side].begin(); it != _newMeshFacets[side].end(); ++it) {
        // if (it->IsFlag(MeshFacet::MARKED))
        //{
        //   _builder.addSingleTriangle(it->_aclPoints[0], it->_aclPoints[1], it->_aclPoints[2],
        //   true, 3.0, 0.0, 1.0, 1.0);
        // }
        mb.AddFacet(*it, true);
    }
    mb.Finish();

    MeshAlgorithm algo(mesh);
    algo.ResetFacetFlag(static_cast<MeshFacet::TFlagType>(MeshFacet::VISIT | MeshFacet::TMP0));

    // bool hasFacetsNotVisited = true; // until facets not visited
    // search for facet not visited
    MeshFacetArray::_TConstIterator itf;
    const MeshFacetArray& rFacets = mesh.GetFacets();
    for (itf = rFacets.begin(); itf != rFacets.end(); ++itf) {
        if (!itf->IsFlag(MeshFacet::VISIT)) {  // Facet found, visit neighbours
            std::vector<FacetIndex> facets;
            facets.push_back(itf - rFacets.begin());  // add seed facet
            CollectFacetVisitor visitor(mesh, facets, _edges, side, mult, _builder);
            mesh.VisitNeighbourFacets(visitor, itf - rFacets.begin());

            if (visitor._addFacets == 0) {  // mark all facets to add it to the result
                algo.SetFacetsFlag(facets, MeshFacet::TMP0);
            }
        }
    }

    // add all facets to the result vector
    for (itf = rFacets.begin(); itf != rFacets.end(); ++itf) {
        if (itf->IsFlag(MeshFacet::TMP0)) {
            _facetsOf[side].push_back(mesh.GetFacet(*itf));
        }
    }

    // MeshDefinitions::SetMinPointDistance(distSave);
}

SetOperations::CollectFacetVisitor::CollectFacetVisitor(const MeshKernel& mesh,
                                                        std::vector<FacetIndex>& facets,
                                                        std::map<Edge, EdgeInfo>& edges,
                                                        int side,
                                                        float mult,
                                                        Base::Builder3D& builder)
    : _facets(facets)
    , _mesh(mesh)
    , _edges(edges)
    , _side(side)
    , _mult(mult)
    , _builder(builder)
{}

bool SetOperations::CollectFacetVisitor::Visit(const MeshFacet& rclFacet,
                                               const MeshFacet& rclFrom,
                                               FacetIndex ulFInd,
                                               unsigned long ulLevel)
{
    (void)rclFacet;
    (void)rclFrom;
    (void)ulLevel;
    _facets.push_back(ulFInd);
    return true;
}

// static int matchCounter = 0;
bool SetOperations::CollectFacetVisitor::AllowVisit(const MeshFacet& rclFacet,
                                                    const MeshFacet& rclFrom,
                                                    FacetIndex ulFInd,
                                                    unsigned long ulLevel,
                                                    unsigned short neighbourIndex)
{
    (void)ulFInd;
    (void)ulLevel;
    if (rclFacet.IsFlag(MeshFacet::MARKED) && rclFrom.IsFlag(MeshFacet::MARKED)) {
        // facet connected to an edge
        PointIndex pt0 = rclFrom._aulPoints[neighbourIndex],
                   pt1 = rclFrom._aulPoints[(neighbourIndex + 1) % 3];
        Edge edge(_mesh.GetPoint(pt0), _mesh.GetPoint(pt1));

        std::map<Edge, EdgeInfo>::iterator it = _edges.find(edge);

        if (it != _edges.end()) {
            if (_addFacets == -1) {
                // determine if the facets should add or not only once
                MeshGeomFacet facet = _mesh.GetFacet(rclFrom);  // triangulated facet
                MeshGeomFacet facetOther =
                    it->second
                        .facets[1 - _side][0];  // triangulated facet from same edge and other mesh
                Vector3f normalOther = facetOther.GetNormal();
                // Vector3f normal = facet.GetNormal();

                Vector3f edgeDir = it->first.pt1 - it->first.pt2;
                Vector3f ocDir = (edgeDir % (facet.GetGravityPoint() - it->first.pt1)) % edgeDir;
                ocDir.Normalize();
                Vector3f ocDirOther =
                    (edgeDir % (facetOther.GetGravityPoint() - it->first.pt1)) % edgeDir;
                ocDirOther.Normalize();

                // Vector3f dir = ocDir % normal;
                // Vector3f dirOther = ocDirOther % normalOther;

                bool match = ((ocDir * normalOther) * _mult) < 0.0f;

                // if (matchCounter == 1)
                //{
                //   // _builder.addSingleArrow(it->second.pt1, it->second.pt1 + edgeDir, 3,
                //   0.0, 1.0, 0.0);

                //  _builder.addSingleTriangle(facet._aclPoints[0], facet._aclPoints[1],
                //  facet._aclPoints[2], true, 3.0, 1.0, 0.0, 0.0);
                //  // _builder.addSingleArrow(facet.GetGravityPoint(), facet.GetGravityPoint() +
                //  ocDir, 3, 1.0, 0.0, 0.0); _builder.addSingleArrow(facet.GetGravityPoint(),
                //  facet.GetGravityPoint() + normal, 3, 1.0, 0.5, 0.0);
                //  // _builder.addSingleArrow(facet.GetGravityPoint(), facet.GetGravityPoint() +
                //  dir, 3, 1.0, 1.0, 0.0);

                //  _builder.addSingleTriangle(facetOther._aclPoints[0], facetOther._aclPoints[1],
                //  facetOther._aclPoints[2], true, 3.0, 0.0, 0.0, 1.0);
                //  // _builder.addSingleArrow(facetOther.GetGravityPoint(),
                //  facetOther.GetGravityPoint() + ocDirOther, 3, 0.0, 0.0, 1.0);
                //  _builder.addSingleArrow(facetOther.GetGravityPoint(),
                //  facetOther.GetGravityPoint() + normalOther, 3, 0.0, 0.5, 1.0);
                //  // _builder.addSingleArrow(facetOther.GetGravityPoint(),
                //  facetOther.GetGravityPoint() + dirOther, 3, 0.0, 1.0, 1.0);

                //}

                // float scalar = dir * dirOther * _mult;
                // bool match = scalar > 0.0f;


                // MeshPoint pt0 = it->first.pt1;
                // MeshPoint pt1 = it->first.pt2;

                // int i, n0 = -1, n1 = -1, m0 = -1, m1 = -1;
                // for (i = 0; i < 3; i++)
                //{
                //   if ((n0 == -1) && (facet._aclPoints[i] == pt0))
                //     n0 = i;
                //   if ((n1 == -1) && (facet._aclPoints[i] == pt1))
                //     n1 = i;
                //   if ((m0 == -1) && (facetOther._aclPoints[i] == pt0))
                //     m0 = i;
                //   if ((m1 == -1) && (facetOther._aclPoints[i] == pt1))
                //     m1 = i;
                // }

                // if ((n0 != -1) && (n1 != -1) && (m0 != -1) && (m1 != -1))
                //{
                //   bool orient_n = n1 > n0;
                //   bool orient_m = m1 > m0;

                //  Vector3f dirN = facet._aclPoints[n1] - facet._aclPoints[n0];
                //  Vector3f dirM = facetOther._aclPoints[m1] - facetOther._aclPoints[m0];

                //  if (matchCounter == 1)
                //  {
                //    _builder.addSingleArrow(facet.GetGravityPoint(), facet.GetGravityPoint() +
                //    dirN, 3, 1.0, 1.0, 0.0); _builder.addSingleArrow(facetOther.GetGravityPoint(),
                //    facetOther.GetGravityPoint() + dirM, 3, 0.0, 1.0, 1.0);
                //  }

                //  if (_mult > 0.0)
                //    match = orient_n == orient_m;
                //  else
                //    match = orient_n != orient_m;
                //}

                if (match) {
                    _addFacets = 0;
                }
                else {
                    _addFacets = 1;
                }

                // matchCounter++;
            }

            return false;
        }
    }

    return true;
}

// ----------------------------------------------------------------------------

bool MeshIntersection::hasIntersection() const
{
    Base::BoundBox3f bbox1 = kernel1.GetBoundBox();
    Base::BoundBox3f bbox2 = kernel2.GetBoundBox();
    if (!(bbox1 && bbox2)) {
        return false;
    }

    if (testIntersection(kernel1, kernel2)) {
        return true;
    }

    return false;
}

void MeshIntersection::getIntersection(std::list<MeshIntersection::Tuple>& intsct) const
{
    const MeshKernel& k1 = kernel1;
    const MeshKernel& k2 = kernel2;

    // Contains bounding boxes for every facet of 'k1'
    std::vector<Base::BoundBox3f> boxes1;
    MeshFacetIterator cMFI1(k1);
    for (cMFI1.Begin(); cMFI1.More(); cMFI1.Next()) {
        boxes1.push_back((*cMFI1).GetBoundBox());
    }

    // Contains bounding boxes for every facet of 'k2'
    std::vector<Base::BoundBox3f> boxes2;
    MeshFacetIterator cMFI2(k2);
    for (cMFI2.Begin(); cMFI2.More(); cMFI2.Next()) {
        boxes2.push_back((*cMFI2).GetBoundBox());
    }

    // Splits the mesh using grid for speeding up the calculation
    MeshFacetGrid cMeshFacetGrid(k1);

    const MeshFacetArray& rFaces2 = k2.GetFacets();
    Base::SequencerLauncher seq("Checking for intersections...", rFaces2.size());
    int index = 0;
    MeshGeomFacet facet1, facet2;
    Base::Vector3f pt1, pt2;

    // Iterate over the facets of the 2nd mesh and find the grid elements of the 1st mesh
    for (MeshFacetArray::_TConstIterator it = rFaces2.begin(); it != rFaces2.end(); ++it, index++) {
        seq.next();
        std::vector<FacetIndex> elements;
        cMeshFacetGrid.Inside(boxes2[index], elements, true);

        cMFI2.Set(index);
        facet2 = *cMFI2;

        for (FacetIndex element : elements) {
            if (boxes2[index] && boxes1[element]) {
                cMFI1.Set(element);
                facet1 = *cMFI1;
                int ret = facet1.IntersectWithFacet(facet2, pt1, pt2);
                if (ret == 2) {
                    Tuple d;
                    d.p1 = pt1;
                    d.p2 = pt2;
                    d.f1 = element;
                    d.f2 = index;
                    intsct.push_back(d);
                }
            }
        }
    }
}

bool MeshIntersection::testIntersection(const MeshKernel& k1, const MeshKernel& k2)
{
    // Contains bounding boxes for every facet of 'k1'
    std::vector<Base::BoundBox3f> boxes1;
    MeshFacetIterator cMFI1(k1);
    for (cMFI1.Begin(); cMFI1.More(); cMFI1.Next()) {
        boxes1.push_back((*cMFI1).GetBoundBox());
    }

    // Contains bounding boxes for every facet of 'k2'
    std::vector<Base::BoundBox3f> boxes2;
    MeshFacetIterator cMFI2(k2);
    for (cMFI2.Begin(); cMFI2.More(); cMFI2.Next()) {
        boxes2.push_back((*cMFI2).GetBoundBox());
    }

    // Splits the mesh using grid for speeding up the calculation
    MeshFacetGrid cMeshFacetGrid(k1);

    const MeshFacetArray& rFaces2 = k2.GetFacets();
    Base::SequencerLauncher seq("Checking for intersections...", rFaces2.size());
    int index = 0;
    MeshGeomFacet facet1, facet2;
    Base::Vector3f pt1, pt2;

    // Iterate over the facets of the 2nd mesh and find the grid elements of the 1st mesh
    for (MeshFacetArray::_TConstIterator it = rFaces2.begin(); it != rFaces2.end(); ++it, index++) {
        seq.next();
        std::vector<FacetIndex> elements;
        cMeshFacetGrid.Inside(boxes2[index], elements, true);

        cMFI2.Set(index);
        facet2 = *cMFI2;

        for (FacetIndex element : elements) {
            if (boxes2[index] && boxes1[element]) {
                cMFI1.Set(element);
                facet1 = *cMFI1;
                int ret = facet1.IntersectWithFacet(facet2, pt1, pt2);
                if (ret == 2) {
                    // abort after the first detected self-intersection
                    return true;
                }
            }
        }
    }

    return false;
}

void MeshIntersection::connectLines(bool onlyclosed,
                                    const std::list<MeshIntersection::Tuple>& rdata,
                                    std::list<std::list<MeshIntersection::Triple>>& lines)
{
    float fMinEps = minDistance * minDistance;

    std::list<Tuple> data = rdata;
    while (!data.empty()) {
        std::list<Tuple>::iterator pF;
        std::list<Triple> newPoly;

        // add first line and delete from the list
        Triple front, back;
        front.f1 = data.begin()->f1;
        front.f2 = data.begin()->f2;
        front.p = data.begin()->p1;  // current start point of the polyline
        back.f1 = data.begin()->f1;
        back.f2 = data.begin()->f2;
        back.p = data.begin()->p2;  // current end point of the polyline
        newPoly.push_back(front);
        newPoly.push_back(back);
        data.erase(data.begin());

        // search for the next line on the begin/end of the polyline and add it
        std::list<Tuple>::iterator pFront, pEnd;
        bool bFoundLine {};
        do {
            float fFrontMin = fMinEps, fEndMin = fMinEps;
            bool bFrontFirst = false, bEndFirst = false;

            pFront = data.end();
            pEnd = data.end();
            bFoundLine = false;

            for (pF = data.begin(); pF != data.end(); ++pF) {
                if (Base::DistanceP2(front.p, pF->p1) < fFrontMin) {
                    fFrontMin = Base::DistanceP2(front.p, pF->p1);
                    pFront = pF;
                    bFrontFirst = true;
                }
                else if (Base::DistanceP2(back.p, pF->p1) < fEndMin) {
                    fEndMin = Base::DistanceP2(back.p, pF->p1);
                    pEnd = pF;
                    bEndFirst = true;
                }
                else if (Base::DistanceP2(front.p, pF->p2) < fFrontMin) {
                    fFrontMin = Base::DistanceP2(front.p, pF->p2);
                    pFront = pF;
                    bFrontFirst = false;
                }
                else if (Base::DistanceP2(back.p, pF->p2) < fEndMin) {
                    fEndMin = Base::DistanceP2(back.p, pF->p2);
                    pEnd = pF;
                    bEndFirst = false;
                }

                if (fFrontMin == 0.0f || fEndMin == 0.0f) {
                    break;
                }
            }

            if (pFront != data.end()) {
                bFoundLine = true;
                if (bFrontFirst) {
                    front.f1 = pFront->f1;
                    front.f2 = pFront->f2;
                    front.p = pFront->p2;
                    newPoly.push_front(front);
                }
                else {
                    front.f1 = pFront->f1;
                    front.f2 = pFront->f2;
                    front.p = pFront->p1;
                    newPoly.push_front(front);
                }

                data.erase(pFront);
            }

            if (pEnd != data.end()) {
                bFoundLine = true;
                if (bEndFirst) {
                    back.f1 = pEnd->f1;
                    back.f2 = pEnd->f2;
                    back.p = pEnd->p2;
                    newPoly.push_back(back);
                }
                else {
                    back.f1 = pEnd->f1;
                    back.f2 = pEnd->f2;
                    back.p = pEnd->p1;
                    newPoly.push_back(back);
                }

                data.erase(pEnd);
            }
        } while (bFoundLine);

        if (onlyclosed) {
            if (newPoly.size() > 2
                && Base::DistanceP2(newPoly.front().p, newPoly.back().p) < fMinEps) {
                lines.push_back(newPoly);
            }
        }
        else {
            lines.push_back(newPoly);
        }
    }
}
