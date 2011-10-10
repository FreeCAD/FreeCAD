/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include "Mesher.h"

#include <Base/Exception.h>
#include <Mod/Mesh/App/Mesh.h>

#include <TopoDS_Shape.hxx>

#ifdef HAVE_SMESH
#include <SMESH_Gen.hxx>
#include <StdMeshers_MaxLength.hxx>
#include <StdMeshers_LocalLength.hxx>
#include <StdMeshers_NumberOfSegments.hxx>
#include <StdMeshers_AutomaticLength.hxx>
#include <StdMeshers_TrianglePreference.hxx>
#include <StdMeshers_MEFISTO_2D.hxx>
#include <StdMeshers_Deflection1D.hxx>
#include <StdMeshers_MaxElementArea.hxx>
#include <StdMeshers_Regular_1D.hxx>
#include <StdMeshers_QuadranglePreference.hxx>
#include <StdMeshers_Quadrangle_2D.hxx>

#include <StdMeshers_LengthFromEdges.hxx>
#include <StdMeshers_NotConformAllowed.hxx>
#include <StdMeshers_Arithmetic1D.hxx>
#endif // HAVE_SMESH

using namespace MeshPart;

Mesher::Mesher(const TopoDS_Shape& s)
  : shape(s), maxLength(0), maxArea(0), localLength(0),
    regular(false)
{
}

Mesher::~Mesher()
{
}

Mesh::MeshObject* Mesher::createMesh() const
{
#ifndef HAVE_SMESH
    throw Base::Exception("SMESH is not available on this platform");
#else
    std::list<SMESH_Hypothesis*> hypoth;

    SMESH_Gen* meshgen = new SMESH_Gen();
    SMESH_Mesh* mesh = meshgen->CreateMesh(0, true);
    int hyp=0;

    if (maxLength > 0) {
        StdMeshers_MaxLength* hyp1d = new StdMeshers_MaxLength(hyp++, 0, meshgen);
        hyp1d->SetLength(maxLength);
        hypoth.push_back(hyp1d);
    }

    if (localLength > 0) {
        StdMeshers_LocalLength* hyp1d = new StdMeshers_LocalLength(hyp++,0,meshgen);
        hyp1d->SetLength(localLength);
        hypoth.push_back(hyp1d);
    }

    if (maxArea > 0) {
        StdMeshers_MaxElementArea* hyp2d = new StdMeshers_MaxElementArea(hyp++,0,meshgen);
        hyp2d->SetMaxArea(1.0f);
        hypoth.push_back(hyp2d);
    }

    {
        StdMeshers_NumberOfSegments* hyp1d = new StdMeshers_NumberOfSegments(hyp++,0,meshgen);
        hyp1d->SetNumberOfSegments(1);
        hypoth.push_back(hyp1d);
    }

    // if none of the above hypothesis were applied
    if (hypoth.empty()) {
        StdMeshers_AutomaticLength* hyp1d = new StdMeshers_AutomaticLength(hyp++,0,meshgen);
        hypoth.push_back(hyp1d);
    }

    if (deflection > 0) {
        StdMeshers_Deflection1D* hyp1d = new StdMeshers_Deflection1D(hyp++,0,meshgen);
        hyp1d->SetDeflection(deflection);
        hypoth.push_back(hyp1d);
    }

    if (regular) {
        StdMeshers_Regular_1D* hyp1d = new StdMeshers_Regular_1D(hyp++,0,meshgen);
        hypoth.push_back(hyp1d);
    }

#if 1
    StdMeshers_TrianglePreference* hyp2d_1 = new StdMeshers_TrianglePreference(hyp++,0,meshgen);
    hypoth.push_back(hyp2d_1);
    StdMeshers_MEFISTO_2D* alg2d = new StdMeshers_MEFISTO_2D(hyp++,0,meshgen);
    hypoth.push_back(alg2d);
#else
    StdMeshers_QuadranglePreference hyp2d_1(hyp++,0,meshgen);
    StdMeshers_Quadrangle_2D alg2d(hyp++,0,meshgen);
#endif

    // Apply the hypothesis and create the mesh
    mesh->ShapeToMesh(shape);
    for (int i=0; i<hyp;i++)
        mesh->AddHypothesis(shape, i);
    meshgen->Compute(*mesh, mesh->GetShapeToMesh());

    // build up the mesh structure
    SMDS_FaceIteratorPtr aFaceIter = mesh->GetMeshDS()->facesIterator();
    SMDS_NodeIteratorPtr aNodeIter = mesh->GetMeshDS()->nodesIterator();

    MeshCore::MeshPointArray verts;
    MeshCore::MeshFacetArray faces;
    verts.reserve(mesh->NbNodes());
    faces.reserve(mesh->NbFaces());

    int index=0;
    std::map<const SMDS_MeshNode*, int> mapNodeIndex;
    for (;aNodeIter->more();) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        MeshCore::MeshPoint p;
        p.Set((float)aNode->X(), (float)aNode->Y(), (float)aNode->Z());
        verts.push_back(p);
        mapNodeIndex[aNode] = index++;
    }
    for (;aFaceIter->more();) {
        const SMDS_MeshFace* aFace = aFaceIter->next();
        MeshCore::MeshFacet f;
        for (int i=0; i<3;i++) {
            const SMDS_MeshNode* node = aFace->GetNode(i);
            //int index = node->GetID() - 1;
            f._aulPoints[i] = /*index*/mapNodeIndex[node];
        }

        faces.push_back(f);
    }

    // clean up
    //FIXME: Why can't we delete this object?
#if defined(__GNUC__)
    delete meshgen; // crashes with MSVC
#endif
    TopoDS_Shape aNull;
    mesh->ShapeToMesh(aNull);
    mesh->Clear();
    delete mesh;
    for (std::list<SMESH_Hypothesis*>::iterator it = hypoth.begin(); it != hypoth.end(); ++it)
        delete *it;

    MeshCore::MeshKernel kernel;
    kernel.Adopt(verts, faces, true);

    Mesh::MeshObject* meshdata = new Mesh::MeshObject();
    meshdata->swap(kernel);
    return meshdata;
#endif // HAVE_SMESH
}

