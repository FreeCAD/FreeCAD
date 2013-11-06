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

#include <Base/Console.h>
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
#include <StdMeshers_Arithmetic1D.hxx>
#include <StdMeshers_MaxElementArea.hxx>
#include <StdMeshers_Regular_1D.hxx>
#include <StdMeshers_QuadranglePreference.hxx>
#include <StdMeshers_Quadrangle_2D.hxx>

#include <StdMeshers_LengthFromEdges.hxx>
#include <StdMeshers_NotConformAllowed.hxx>
#if defined(_MSC_VER)
#include <NETGENPlugin_NETGEN_2D.hxx>
#include <NETGENPlugin_Hypothesis_2D.hxx>
#include <NETGENPlugin_SimpleHypothesis_2D.hxx>
#endif
#endif // HAVE_SMESH

using namespace MeshPart;

MeshingOutput::MeshingOutput() 
{
    buffer.reserve(80);
}

int MeshingOutput::overflow(int c)
{
    if (c != EOF)
        buffer.push_back((char)c);
    return c;
}

int MeshingOutput::sync()
{
    // Print as log as this might be verbose
    if (!buffer.empty()) {
        if (buffer.find("failed") != std::string::npos) {
            std::string::size_type pos = buffer.find(" : ");
            std::string sub;
            if (pos != std::string::npos) {
                // chop the last newline
                sub = buffer.substr(pos+3, buffer.size()-pos-4);
            }
            else {
                sub = buffer;
            }
            Base::Console().Error("%s", sub.c_str());
        }
        buffer.clear();
    }
    return 0;
}

Mesher::Mesher(const TopoDS_Shape& s)
  : shape(s), maxLength(0), maxArea(0), localLength(0),
    deflection(0), minLen(0), maxLen(0), regular(false)
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

#if defined(_MSC_VER)
    NETGENPlugin_Hypothesis_2D* hyp2d = new NETGENPlugin_Hypothesis_2D(hyp++,0,meshgen);

    //TODO: Try to find values so that we get similar results as with Mefisto meshing
    double growth = 0;
    if (maxLength > 0 && localLength > 0) {
        growth = std::min<double>(maxLength, localLength);
    }
    else if (maxLength > 0) {
        growth = maxLength;
    }
    else if (localLength > 0) {
        growth = localLength;
    }

    if (growth == 0.0)
        hyp2d->SetFineness(NETGENPlugin_Hypothesis_2D::Moderate);
    else if (growth <= 0.1)
        hyp2d->SetFineness(NETGENPlugin_Hypothesis_2D::VeryFine);
    else if (growth <= 0.2f)
        hyp2d->SetFineness(NETGENPlugin_Hypothesis_2D::Fine);
    else if (growth <= 0.5f)
        hyp2d->SetFineness(NETGENPlugin_Hypothesis_2D::Moderate);
    else if (growth <= 0.7f)
        hyp2d->SetFineness(NETGENPlugin_Hypothesis_2D::Coarse);
    else
        hyp2d->SetFineness(NETGENPlugin_Hypothesis_2D::VeryCoarse);

    //hyp2d->SetGrowthRate(growth);
    //hyp2d->SetNbSegPerEdge(5);
    //hyp2d->SetNbSegPerRadius(10);
    hyp2d->SetQuadAllowed(false);
    hyp2d->SetOptimize(true);
    hyp2d->SetSecondOrder(true); // apply bisecting to create four triangles out of one
    hypoth.push_back(hyp2d);

    NETGENPlugin_NETGEN_2D* alg2d = new NETGENPlugin_NETGEN_2D(hyp++,0,meshgen);
    hypoth.push_back(alg2d);
#else
    if (maxLength > 0) {
        StdMeshers_MaxLength* hyp1d = new StdMeshers_MaxLength(hyp++, 0, meshgen);
        hyp1d->SetLength(maxLength);
        hypoth.push_back(hyp1d);
    }
    else if (localLength > 0) {
        StdMeshers_LocalLength* hyp1d = new StdMeshers_LocalLength(hyp++,0,meshgen);
        hyp1d->SetLength(localLength);
        hypoth.push_back(hyp1d);
    }
    else if (maxArea > 0) {
        StdMeshers_MaxElementArea* hyp2d = new StdMeshers_MaxElementArea(hyp++,0,meshgen);
        hyp2d->SetMaxArea(maxArea);
        hypoth.push_back(hyp2d);
    }
    else if (deflection > 0) {
        StdMeshers_Deflection1D* hyp1d = new StdMeshers_Deflection1D(hyp++,0,meshgen);
        hyp1d->SetDeflection(deflection);
        hypoth.push_back(hyp1d);
    }
    else if (minLen > 0 && maxLen > 0) {
        StdMeshers_Arithmetic1D* hyp1d = new StdMeshers_Arithmetic1D(hyp++,0,meshgen);
        hyp1d->SetLength(minLen, false);
        hyp1d->SetLength(maxLen, true);
        hypoth.push_back(hyp1d);
    }
    else {
        StdMeshers_AutomaticLength* hyp1d = new StdMeshers_AutomaticLength(hyp++,0,meshgen);
        hypoth.push_back(hyp1d);
    }

    {
        StdMeshers_NumberOfSegments* hyp1d = new StdMeshers_NumberOfSegments(hyp++,0,meshgen);
        hyp1d->SetNumberOfSegments(1);
        hypoth.push_back(hyp1d);
    }

    if (regular) {
        StdMeshers_Regular_1D* hyp1d = new StdMeshers_Regular_1D(hyp++,0,meshgen);
        hypoth.push_back(hyp1d);
    }

    StdMeshers_TrianglePreference* hyp2d_1 = new StdMeshers_TrianglePreference(hyp++,0,meshgen);
    hypoth.push_back(hyp2d_1);
    StdMeshers_MEFISTO_2D* alg2d = new StdMeshers_MEFISTO_2D(hyp++,0,meshgen);
    hypoth.push_back(alg2d);
#endif

    // Set new cout
    MeshingOutput stdcout;
    std::streambuf* oldcout = std::cout.rdbuf(&stdcout);

    // Apply the hypothesis and create the mesh
    mesh->ShapeToMesh(shape);
    for (int i=0; i<hyp;i++)
        mesh->AddHypothesis(shape, i);
    meshgen->Compute(*mesh, mesh->GetShapeToMesh());

    // Restore old cout
    std::cout.rdbuf(oldcout);

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
        if (aFace->NbNodes() == 3) {
            MeshCore::MeshFacet f;
            for (int i=0; i<3;i++) {
                const SMDS_MeshNode* node = aFace->GetNode(i);
                f._aulPoints[i] = mapNodeIndex[node];
            }
            faces.push_back(f);
        }
        else if (aFace->NbNodes() == 4) {
            MeshCore::MeshFacet f1, f2;
            const SMDS_MeshNode* node0 = aFace->GetNode(0);
            const SMDS_MeshNode* node1 = aFace->GetNode(1);
            const SMDS_MeshNode* node2 = aFace->GetNode(2);
            const SMDS_MeshNode* node3 = aFace->GetNode(3);

            f1._aulPoints[0] = mapNodeIndex[node0];
            f1._aulPoints[1] = mapNodeIndex[node1];
            f1._aulPoints[2] = mapNodeIndex[node2];

            f2._aulPoints[0] = mapNodeIndex[node0];
            f2._aulPoints[1] = mapNodeIndex[node2];
            f2._aulPoints[2] = mapNodeIndex[node3];

            faces.push_back(f1);
            faces.push_back(f2);
        }
        else if (aFace->NbNodes() == 6) {
            MeshCore::MeshFacet f1, f2, f3, f4;
            const SMDS_MeshNode* node0 = aFace->GetNode(0);
            const SMDS_MeshNode* node1 = aFace->GetNode(1);
            const SMDS_MeshNode* node2 = aFace->GetNode(2);
            const SMDS_MeshNode* node3 = aFace->GetNode(3);
            const SMDS_MeshNode* node4 = aFace->GetNode(4);
            const SMDS_MeshNode* node5 = aFace->GetNode(5);

            f1._aulPoints[0] = mapNodeIndex[node0];
            f1._aulPoints[1] = mapNodeIndex[node3];
            f1._aulPoints[2] = mapNodeIndex[node5];

            f2._aulPoints[0] = mapNodeIndex[node1];
            f2._aulPoints[1] = mapNodeIndex[node4];
            f2._aulPoints[2] = mapNodeIndex[node3];

            f3._aulPoints[0] = mapNodeIndex[node2];
            f3._aulPoints[1] = mapNodeIndex[node5];
            f3._aulPoints[2] = mapNodeIndex[node4];

            f4._aulPoints[0] = mapNodeIndex[node3];
            f4._aulPoints[1] = mapNodeIndex[node4];
            f4._aulPoints[2] = mapNodeIndex[node5];

            faces.push_back(f1);
            faces.push_back(f2);
            faces.push_back(f3);
            faces.push_back(f4);
        }
    }

    // clean up
    delete meshgen;

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

