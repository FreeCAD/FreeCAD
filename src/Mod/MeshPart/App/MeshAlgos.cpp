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
    std::set<MeshCore::FacetIndex> fliped;

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
                fliped.insert(it.Position());
            }
        }

        // if there are no flipped triangles -> stop
        // int f =fliped.size();
        if (fliped.empty()) {
            break;
        }

        for (MeshCore::FacetIndex It : fliped) {
            alg.CollapseFacet(It);
        }
        fliped.clear();
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


void MeshAlgos::coarsen(MeshCore::MeshKernel* /*Mesh*/, float /*f*/)
{
#ifdef FC_USE_GTS
    GtsSurface* surface;

    // create a GTS surface
    surface = MeshAlgos::createGTSSurface(Mesh);

    Mesh->Clear();

    guint stop_number = 100000;
    gdouble fold = 3.1415 / 180.;

    gts_surface_coarsen(surface,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        (GtsStopFunc)gts_coarsen_stop_number,
                        &stop_number,
                        fold);

    // get the standard mesh
    fillMeshFromGTSSurface(Mesh, surface);
#endif
}


MeshCore::MeshKernel* MeshAlgos::boolean(MeshCore::MeshKernel* pMesh1,
                                         MeshCore::MeshKernel* /*pMesh2*/,
                                         MeshCore::MeshKernel* /*pResult*/,
                                         int /*Type*/)
{
#ifdef FC_USE_GTS
    GtsSurface *s1, *s2, *s3;
    GtsSurfaceInter* si;
    GNode *tree1, *tree2;
    gboolean check_self_intersection = false;
    gboolean closed = true, is_open1, is_open2;


    // create a GTS surface
    s1 = MeshAlgos::createGTSSurface(pMesh1);
    s2 = MeshAlgos::createGTSSurface(pMesh2);

    // clear the mesh (memory)
    // Mesh1.clear();
    // Mesh2.clear();

    /* check that the surfaces are orientable manifolds */
    if (!gts_surface_is_orientable(s1)) {
        gts_object_destroy(GTS_OBJECT(s1));
        gts_object_destroy(GTS_OBJECT(s2));
        throw std::runtime_error("surface 1 is not an orientable manifold\n");
    }
    if (!gts_surface_is_orientable(s2)) {
        gts_object_destroy(GTS_OBJECT(s1));
        gts_object_destroy(GTS_OBJECT(s2));
        throw std::runtime_error("surface 2 is not an orientable manifold\n");
    }

    /* check that the surfaces are not self-intersecting */
    if (check_self_intersection) {
        GtsSurface* self_intersects;

        self_intersects = gts_surface_is_self_intersecting(s1);
        if (self_intersects != NULL) {
            //      if (verbose)
            //        gts_surface_print_stats (self_intersects, stderr);
            //      gts_surface_write (self_intersects, stdout);
            gts_object_destroy(GTS_OBJECT(self_intersects));
            gts_object_destroy(GTS_OBJECT(s1));
            gts_object_destroy(GTS_OBJECT(s2));
            throw std::runtime_error("surface is self-intersecting\n");
        }
        self_intersects = gts_surface_is_self_intersecting(s2);
        if (self_intersects != NULL) {
            //      if (verbose)
            //        gts_surface_print_stats (self_intersects, stderr);
            //      gts_surface_write (self_intersects, stdout);
            gts_object_destroy(GTS_OBJECT(self_intersects));
            gts_object_destroy(GTS_OBJECT(s1));
            gts_object_destroy(GTS_OBJECT(s2));
            throw std::runtime_error("surface is self-intersecting\n");
        }
    }

    /* build bounding box tree for first surface */
    tree1 = gts_bb_tree_surface(s1);
    is_open1 = gts_surface_volume(s1) < 0. ? true : false;

    /* build bounding box tree for second surface */
    tree2 = gts_bb_tree_surface(s2);
    is_open2 = gts_surface_volume(s2) < 0. ? true : false;

    si = gts_surface_inter_new(gts_surface_inter_class(), s1, s2, tree1, tree2, is_open1, is_open2);
    g_assert(gts_surface_inter_check(si, &closed));
    if (!closed) {
        gts_object_destroy(GTS_OBJECT(s1));
        gts_object_destroy(GTS_OBJECT(s2));
        gts_bb_tree_destroy(tree1, true);
        gts_bb_tree_destroy(tree2, true);
        throw "the intersection of 1 and  2 is not a closed curve\n";
    }

    s3 = gts_surface_new(gts_surface_class(),
                         gts_face_class(),
                         gts_edge_class(),
                         gts_vertex_class());
    if (Type == 0) {  // union
        gts_surface_inter_boolean(si, s3, GTS_1_OUT_2);
        gts_surface_inter_boolean(si, s3, GTS_2_OUT_1);
    }
    else if (Type == 1) {  // inter
        gts_surface_inter_boolean(si, s3, GTS_1_IN_2);
        gts_surface_inter_boolean(si, s3, GTS_2_IN_1);
    }
    else if (Type == 2) {  // diff
        gts_surface_inter_boolean(si, s3, GTS_1_OUT_2);
        gts_surface_inter_boolean(si, s3, GTS_2_IN_1);
        gts_surface_foreach_face(si->s2, (GtsFunc)gts_triangle_revert, NULL);
        gts_surface_foreach_face(s2, (GtsFunc)gts_triangle_revert, NULL);
    }
    else if (Type == 3) {  // cut inner
        gts_surface_inter_boolean(si, s3, GTS_1_IN_2);
    }
    else if (Type == 4) {  // cut outer
        gts_surface_inter_boolean(si, s3, GTS_1_OUT_2);
    }

    // check that the resulting surface is not self-intersecting
    if (check_self_intersection) {
        GtsSurface* self_intersects;

        self_intersects = gts_surface_is_self_intersecting(s3);
        if (self_intersects != NULL) {
            //      if (verbose)
            //        gts_surface_print_stats (self_intersects, stderr);
            //     gts_surface_write (self_intersects, stdout);
            gts_object_destroy(GTS_OBJECT(self_intersects));
            gts_object_destroy(GTS_OBJECT(s1));
            gts_object_destroy(GTS_OBJECT(s2));
            gts_object_destroy(GTS_OBJECT(s3));
            gts_object_destroy(GTS_OBJECT(si));
            gts_bb_tree_destroy(tree1, true);
            gts_bb_tree_destroy(tree2, true);
            throw std::runtime_error("the resulting surface is self-intersecting\n");
        }
    }
    // display summary information about the resulting surface
    //  if (verbose)
    //    gts_surface_print_stats (s3, stderr);
    // write resulting surface to standard output

    // get the standard mesh
    fillMeshFromGTSSurface(pResult, s3);


    // destroy surfaces
    gts_object_destroy(GTS_OBJECT(s1));
    gts_object_destroy(GTS_OBJECT(s2));
    //  gts_object_destroy (GTS_OBJECT (s3));
    //  gts_object_destroy (GTS_OBJECT (si));

    // destroy bounding box trees (including bounding boxes)
    //  gts_bb_tree_destroy (tree1, true);
    //  gts_bb_tree_destroy (tree2, true);

#endif
    return pMesh1;
}


#ifdef FC_USE_GTS


/// helper function - construct a Edge out of two Vertexes if not already there
static GtsEdge* new_edge(GtsVertex* v1, GtsVertex* v2)
{
    GtsSegment* s = gts_vertices_are_connected(v1, v2);
    if (s == NULL) {
        return gts_edge_new(gts_edge_class(), v1, v2);
    }
    else {
        return GTS_EDGE(s);
    }
}


GtsSurface* MeshAlgos::createGTSSurface(MeshCore::MeshKernel* Mesh)
{
    GtsSurface* Surf = gts_surface_new(gts_surface_class(),
                                       gts_face_class(),
                                       gts_edge_class(),
                                       gts_vertex_class());

    unsigned long p1, p2, p3;
    Base::Vector3f Vertex;


    // Getting all the points
    GtsVertex** aVertex = (GtsVertex**)malloc(Mesh->CountPoints() * sizeof(GtsVertex*));
    for (unsigned int PIter = 0; PIter < Mesh->CountPoints(); PIter++) {
        Vertex = Mesh->GetPoint(PIter);
        aVertex[PIter] = gts_vertex_new(gts_vertex_class(), Vertex.x, Vertex.y, Vertex.z);
    }

    // cycling through the facets
    for (unsigned int pFIter = 0; pFIter < Mesh->CountFacets(); pFIter++) {
        // getting the three points of the facet
        Mesh->GetFacetPoints(pFIter, p1, p2, p3);

        // creating the edges and add the face to the surface
        gts_surface_add_face(Surf,
                             gts_face_new(Surf->face_class,
                                          new_edge(aVertex[p1], aVertex[p2]),
                                          new_edge(aVertex[p2], aVertex[p3]),
                                          new_edge(aVertex[p3], aVertex[p1])));
    }

    Base::Console().Log("GTS [%d faces, %d Points, %d Edges,%s ,%s]\n",
                        gts_surface_face_number(Surf),
                        gts_surface_vertex_number(Surf),
                        gts_surface_edge_number(Surf),
                        gts_surface_is_orientable(Surf) ? "orientable" : "not orientable",
                        gts_surface_is_self_intersecting(Surf) ? "self-intersections"
                                                               : "no self-intersection");

    return Surf;
}

/// helper function for the face (triangle iteration
static void onFaces(GtsTriangle* t, std::vector<MeshGeomFacet>* VAry)
{
    GtsVertex *mv0, *mv1, *mv2;

    gts_triangle_vertices(t, &mv0, &mv1, &mv2);

    VAry->push_back(MeshGeomFacet(Base::Vector3f(mv0->p.x, mv0->p.y, mv0->p.z),
                                  Base::Vector3f(mv1->p.x, mv1->p.y, mv1->p.z),
                                  Base::Vector3f(mv2->p.x, mv2->p.y, mv2->p.z)));
}

/*
static void onVertices(GtsVertex *v, MeshKernel *pKernel )
{
  Base::Vector3f Point(GTS_POINT(v)->x,GTS_POINT(v)->y,GTS_POINT(v)->z);
}*/

void MeshAlgos::fillMeshFromGTSSurface(MeshCore::MeshKernel* pMesh, GtsSurface* pSurface)
{
    std::vector<MeshGeomFacet> VAry;

    // remove old mesh
    pMesh->Clear();

    //  gts_surface_foreach_vertex(pSurface,(GtsFunc) onVertices,&MeshK);
    gts_surface_foreach_face(pSurface, (GtsFunc)onFaces, &VAry);

    // destroy surfaces
    gts_object_destroy(GTS_OBJECT(pSurface));

    // put the facets the simple way in the mesh, totp is recalculated!
    (*pMesh) = VAry;
}

#endif

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

    // calculate the projection for each Edge
    //  CurveProjectorShape Project(aShape,*pMesh);
    CurveProjectorWithToolMesh Project(aShape, *pMesh, *pToolMesh);

    // IntersectionLine Lines;
    //  MeshWithProperty *ResultMesh = new MeshWithProperty();


    // boolean(pMesh,ToolMesh,ResultMesh,1);
}

/*
void MeshAlgos::doIntersection(const MeshWithProperty &pMesh,const MeshWithProperty
ToolMesh,IntersectionLine &Lines)
  {


  }

*/

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

            //      Base::Console().Log("Pos: %f %f %f \n",Ptn.x,Ptn.y,Ptn.z);

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
