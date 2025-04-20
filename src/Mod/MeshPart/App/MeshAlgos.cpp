/***************************************************************************
 *   Copyright (c) 2008 Juergen Riegel <juergen.riegel@web.de>             *
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
#ifdef FC_OS_LINUX
#include <unistd.h>
#endif
#endif

#include <Base/Builder3D.h>
#include <Base/Console.h>
#include <Mod/Mesh/App/Core/Evaluation.h>
#include <Mod/Mesh/App/Core/Iterator.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/TopoAlgorithm.h>

#include "MeshAlgos.h"


using namespace MeshPart;
using namespace MeshCore;


void MeshAlgos::offset(MeshCore::MeshKernel* Mesh, float fSize)
{
    std::vector<Base::Vector3f> normals = Mesh->CalcVertexNormals();

    unsigned int i = 0;
    // go through all the Vertex normals
    for (std::vector<Base::Vector3f>::iterator It = normals.begin(); It != normals.end();
         ++It, i++) {
        // and move each mesh point in the normal direction
        Mesh->MovePoint(i, It->Normalize() * fSize);
    }
    Mesh->RecalcBoundBox();
}


void MeshAlgos::offsetSpecial2(MeshCore::MeshKernel* Mesh, float fSize)
{
    Base::Builder3D builder;
    std::vector<Base::Vector3f> PointNormals = Mesh->CalcVertexNormals();
    std::vector<Base::Vector3f> FaceNormals;
    std::set<MeshCore::FacetIndex> flipped;

    MeshFacetIterator it(*Mesh);
    for (it.Init(); it.More(); it.Next()) {
        FaceNormals.push_back(it->GetNormal().Normalize());
    }

    unsigned int i = 0;

    // go through all the Vertex normals
    for (std::vector<Base::Vector3f>::iterator It = PointNormals.begin(); It != PointNormals.end();
         ++It, i++) {
        Base::Line3f line {Mesh->GetPoint(i), Mesh->GetPoint(i) + It->Normalize() * fSize};
        Base::DrawStyle drawStyle;
        builder.addNode(Base::LineItem {line, drawStyle});
        // and move each mesh point in the normal direction
        Mesh->MovePoint(i, It->Normalize() * fSize);
    }
    Mesh->RecalcBoundBox();

    MeshTopoAlgorithm alg(*Mesh);

    for (int l = 0; l < 1; l++) {
        for (it.Init(), i = 0; it.More(); it.Next(), i++) {
            if (it->IsFlag(MeshFacet::INVALID)) {
                continue;
            }
            // calculate the angle between them
            float angle = acos((FaceNormals[i] * it->GetNormal())
                               / (it->GetNormal().Length() * FaceNormals[i].Length()));
            if (angle > 1.6) {
                Base::DrawStyle drawStyle;
                drawStyle.pointSize = 4.0F;
                Base::PointItem item {it->GetGravityPoint(),
                                      drawStyle,
                                      Base::ColorRGB {1.0F, 0.0F, 0.0F}};
                builder.addNode(item);
                flipped.insert(it.Position());
            }
        }

        if (flipped.empty()) {
            break;
        }

        for (MeshCore::FacetIndex It : flipped) {
            alg.CollapseFacet(It);
        }
        flipped.clear();
    }

    alg.Cleanup();

    // search for intersected facets
    MeshCore::MeshEvalSelfIntersection eval(*Mesh);
    std::vector<std::pair<MeshCore::FacetIndex, MeshCore::FacetIndex>> faces;
    eval.GetIntersections(faces);


    builder.saveToLog();
}

void MeshAlgos::offsetSpecial(MeshCore::MeshKernel* Mesh, float fSize, float zmax, float zmin)
{
    std::vector<Base::Vector3f> normals = Mesh->CalcVertexNormals();

    unsigned int i = 0;
    // go through all the Vertex normals
    for (std::vector<Base::Vector3f>::iterator It = normals.begin(); It != normals.end();
         ++It, i++) {
        Base::Vector3f Pnt = Mesh->GetPoint(i);

        if (Pnt.z < zmax && Pnt.z > zmin) {
            Pnt.z = 0;
            Mesh->MovePoint(i, Pnt.Normalize() * fSize);
        }
        else {
            // and move each mesh point in the normal direction
            Mesh->MovePoint(i, It->Normalize() * fSize);
        }
    }
}

#include <BRep_Tool.hxx>
#include <GeomAPI_IntCS.hxx>
#include <GeomLProp_CLProps.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Plane.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>

void MeshAlgos::cutByShape(const TopoDS_Shape& aShape,
                           const MeshCore::MeshKernel* pMesh,
                           MeshCore::MeshKernel* pToolMesh)
{
    CurveProjectorWithToolMesh Project(aShape, *pMesh, *pToolMesh);
}


void MeshAlgos::cutByCurve(MeshCore::MeshKernel* pMesh,
                           const std::vector<CurveProjector::FaceSplitEdge>& vSplitEdges)
{
    MeshTopoAlgorithm cTopAlg(*pMesh);

    for (const auto& it : vSplitEdges) {
        cTopAlg.SplitFacet(it.ulFaceIndex, it.p1, it.p2);
    }
}

class _VertexCompare
{
public:
    bool operator()(const TopoDS_Vertex& rclV1, const TopoDS_Vertex& rclV2) const
    {
        if (rclV1.IsSame(rclV2) == Standard_True) {
            return false;
        }

        gp_XYZ clP1 = BRep_Tool::Pnt(rclV1).XYZ();
        gp_XYZ clP2 = BRep_Tool::Pnt(rclV2).XYZ();

        if (fabs(clP1.X() - clP2.X()) < dE) {
            if (fabs(clP1.Y() - clP2.Y()) < dE) {
                return clP1.Z() < clP2.Z();
            }
            else {
                return clP1.Y() < clP2.Y();
            }
        }
        else {
            return clP1.X() < clP2.X();
        }
    }

    double dE = 1.0e-5;
};


void MeshAlgos::LoftOnCurve(MeshCore::MeshKernel& ResultMesh,
                            const TopoDS_Shape& Shape,
                            const std::vector<Base::Vector3f>& poly,
                            const Base::Vector3f& up,
                            float MaxSize)
{
    TopExp_Explorer Ex;
    Standard_Real fBegin, fEnd;
    std::vector<MeshGeomFacet> cVAry;
    std::map<TopoDS_Vertex, std::vector<Base::Vector3f>, _VertexCompare> ConnectMap;

    for (Ex.Init(Shape, TopAbs_EDGE); Ex.More(); Ex.Next()) {
        // get the edge and the belonging Vertexes
        TopoDS_Edge Edge = (TopoDS_Edge&)Ex.Current();
        TopoDS_Vertex V1, V2;
        TopExp::Vertices(Edge, V1, V2);
        bool bBegin = false, bEnd = false;
        // getting the geometric curve and the interval
        GeomLProp_CLProps prop(BRep_Tool::Curve(Edge, fBegin, fEnd), 1, 0.0000000001);
        int res = int((fEnd - fBegin) / MaxSize);
        // do at least 2 segments
        if (res < 2) {
            res = 2;
        }
        gp_Dir Tangent;

        std::vector<Base::Vector3f> prePoint(poly.size());
        std::vector<Base::Vector3f> actPoint(poly.size());

        // checking if there is already a end to connect
        if (ConnectMap.find(V1) != ConnectMap.end()) {
            bBegin = true;
            prePoint = ConnectMap[V1];
        }

        if (ConnectMap.find(V2) != ConnectMap.end()) {
            bEnd = true;
        }

        for (long i = 0; i < res; i++) {

            // get point and tangent at the position, up is fix for the moment
            prop.SetParameter(fBegin + ((fEnd - fBegin) * float(i)) / float(res - 1));
            prop.Tangent(Tangent);
            Base::Vector3f Tng((float)Tangent.X(), (float)Tangent.Y(), (float)Tangent.Z());
            Base::Vector3f Ptn((float)prop.Value().X(),
                               (float)prop.Value().Y(),
                               (float)prop.Value().Z());
            Base::Vector3f Up(up);
            // normalize and calc the third vector of the plane coordinatesystem
            Tng.Normalize();
            Up.Normalize();
            Base::Vector3f Third(Tng % Up);

            unsigned int l = 0;
            std::vector<Base::Vector3f>::const_iterator It;

            // got through the profile
            for (It = poly.begin(); It != poly.end(); ++It, l++) {
                actPoint[l] = ((Third * It->x) + (Up * It->y) + (Tng * It->z) + Ptn);
            }

            if (i == res - 1 && !bEnd) {
                // remember the last row to connect to a otger edge with the same vertex
                ConnectMap[V2] = actPoint;
            }

            if (i == 1 && bBegin) {
                // using the end of an other edge as start
                prePoint = ConnectMap[V1];
            }

            if (i == 0 && !bBegin) {
                // remember the first row for connection to a edge with the same vertex
                ConnectMap[V1] = actPoint;
            }

            if (i)  // not the first row or something to connect to
            {
                for (l = 0; l < actPoint.size(); l++) {
                    if (l)  // not first point in row
                    {
                        if (i == res - 1 && bEnd) {  // if last row and a end to connect
                            actPoint = ConnectMap[V2];
                        }

                        Base::Vector3f p1 = prePoint[l - 1], p2 = actPoint[l - 1], p3 = prePoint[l],
                                       p4 = actPoint[l];

                        cVAry.emplace_back(p1, p2, p3);
                        cVAry.emplace_back(p3, p2, p4);
                    }
                }
            }

            prePoint = actPoint;
        }
    }

    ResultMesh.AddFacets(cVAry);
}
