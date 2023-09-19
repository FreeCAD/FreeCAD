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
#ifndef _PreComp_
#include <algorithm>

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <Standard_Version.hxx>
#include <TopoDS_Shape.hxx>
#endif

#include <Base/Console.h>
#include <Base/Tools.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Part/App/TopoShape.h>

#include "Mesher.h"


#ifdef HAVE_SMESH
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#pragma clang diagnostic ignored "-Wextra-semi"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#include <SMESHDS_Mesh.hxx>
#include <SMESH_Gen.hxx>
#include <SMESH_Mesh.hxx>
#include <StdMeshers_MaxLength.hxx>

#if SMESH_VERSION_MAJOR < 7
#include <StdMeshers_TrianglePreference.hxx>
#endif

#include <StdMeshers_Arithmetic1D.hxx>
#include <StdMeshers_AutomaticLength.hxx>
#include <StdMeshers_Deflection1D.hxx>
#include <StdMeshers_LocalLength.hxx>
#include <StdMeshers_MEFISTO_2D.hxx>
#include <StdMeshers_MaxElementArea.hxx>
#include <StdMeshers_NumberOfSegments.hxx>
#include <StdMeshers_QuadranglePreference.hxx>
#include <StdMeshers_Quadrangle_2D.hxx>
#include <StdMeshers_Regular_1D.hxx>

#include <StdMeshers_LengthFromEdges.hxx>
#include <StdMeshers_NotConformAllowed.hxx>
#if defined(HAVE_NETGEN)
#include <NETGENPlugin_Hypothesis_2D.hxx>
#include <NETGENPlugin_NETGEN_2D.hxx>
#include <NETGENPlugin_SimpleHypothesis_2D.hxx>
#endif  // HAVE_NETGEN
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif  // HAVE_SMESH

using namespace MeshPart;

SMESH_Gen* Mesher::_mesh_gen = nullptr;


MeshingOutput::MeshingOutput()
{
    buffer.reserve(80);
}

int MeshingOutput::overflow(int c)
{
    if (c != EOF) {
        buffer.push_back((char)c);
    }
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
                sub = buffer.substr(pos + 3, buffer.size() - pos - 4);
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

// ----------------------------------------------------------------------------

namespace MeshPart
{

struct Vertex
{
    static const double deflection;
    Standard_Real x, y, z;
    Standard_Integer i = 0;
    mutable MeshCore::MeshPoint p;

    Vertex(Standard_Real X, Standard_Real Y, Standard_Real Z)
        : x(X)
        , y(Y)
        , z(Z)
    {
        p.x = static_cast<float>(x);
        p.y = static_cast<float>(y);
        p.z = static_cast<float>(z);
    }

    const MeshCore::MeshPoint& toPoint() const
    {
        return p;
    }

    bool operator<(const Vertex& v) const
    {
        if (fabs(this->x - v.x) >= deflection) {
            return this->x < v.x;
        }
        if (fabs(this->y - v.y) >= deflection) {
            return this->y < v.y;
        }
        if (fabs(this->z - v.z) >= deflection) {
            return this->z < v.z;
        }
        return false;  // points are considered to be equal
    }
};

const double Vertex::deflection = gp::Resolution();

// ----------------------------------------------------------------------------

class BrepMesh
{
    bool segments;
    std::vector<uint32_t> colors;

public:
    BrepMesh(bool s, const std::vector<uint32_t>& c)
        : segments(s)
        , colors(c)
    {}

    Mesh::MeshObject* create(const std::vector<Part::TopoShape::Domain>& domains) const
    {
        std::map<uint32_t, std::vector<std::size_t>> colorMap;
        for (std::size_t i = 0; i < colors.size(); i++) {
            colorMap[colors[i]].push_back(i);
        }

        bool createSegm = (colors.size() == domains.size());

        MeshCore::MeshFacetArray faces;
        std::size_t numTriangles = 0;
        for (const auto& it : domains) {
            numTriangles += it.facets.size();
        }
        faces.reserve(numTriangles);

        std::set<Vertex> vertices;
        Standard_Real x1, y1, z1;
        Standard_Real x2, y2, z2;
        Standard_Real x3, y3, z3;

        std::vector<std::vector<MeshCore::FacetIndex>> meshSegments;
        std::size_t numMeshFaces = 0;

        for (const auto& domain : domains) {
            std::size_t numDomainFaces = 0;
            for (std::size_t j = 0; j < domain.facets.size(); ++j) {
                const Part::TopoShape::Facet& tria = domain.facets[j];
                x1 = domain.points[tria.I1].x;
                y1 = domain.points[tria.I1].y;
                z1 = domain.points[tria.I1].z;

                x2 = domain.points[tria.I2].x;
                y2 = domain.points[tria.I2].y;
                z2 = domain.points[tria.I2].z;

                x3 = domain.points[tria.I3].x;
                y3 = domain.points[tria.I3].y;
                z3 = domain.points[tria.I3].z;

                std::set<Vertex>::iterator it;
                MeshCore::MeshFacet face;

                // 1st vertex
                Vertex v1(x1, y1, z1);
                it = vertices.find(v1);
                if (it == vertices.end()) {
                    v1.i = vertices.size();
                    face._aulPoints[0] = v1.i;
                    vertices.insert(v1);
                }
                else {
                    face._aulPoints[0] = it->i;
                }

                // 2nd vertex
                Vertex v2(x2, y2, z2);
                it = vertices.find(v2);
                if (it == vertices.end()) {
                    v2.i = vertices.size();
                    face._aulPoints[1] = v2.i;
                    vertices.insert(v2);
                }
                else {
                    face._aulPoints[1] = it->i;
                }

                // 3rd vertex
                Vertex v3(x3, y3, z3);
                it = vertices.find(v3);
                if (it == vertices.end()) {
                    v3.i = vertices.size();
                    face._aulPoints[2] = v3.i;
                    vertices.insert(v3);
                }
                else {
                    face._aulPoints[2] = it->i;
                }

                // make sure that we don't insert invalid facets
                if (face._aulPoints[0] != face._aulPoints[1]
                    && face._aulPoints[1] != face._aulPoints[2]
                    && face._aulPoints[2] != face._aulPoints[0]) {
                    faces.push_back(face);
                    numDomainFaces++;
                }
            }

            // add a segment for the face
            if (createSegm || this->segments) {
                std::vector<MeshCore::FacetIndex> segment(numDomainFaces);
                std::generate(segment.begin(),
                              segment.end(),
                              Base::iotaGen<MeshCore::FacetIndex>(numMeshFaces));
                numMeshFaces += numDomainFaces;
                meshSegments.push_back(segment);
            }
        }

        MeshCore::MeshPointArray verts;
        verts.resize(vertices.size());
        for (const auto& it : vertices) {
            verts[it.i] = it.toPoint();
        }

        MeshCore::MeshKernel kernel;
        kernel.Adopt(verts, faces, true);

        Mesh::MeshObject* meshdata = new Mesh::MeshObject();
        meshdata->swap(kernel);
        if (createSegm) {
            int index = 0;
            for (const auto& it : colorMap) {
                Mesh::Segment segm(meshdata, false);
                for (auto jt : it.second) {
                    segm.addIndices(meshSegments[jt]);
                }
                segm.save(true);
                std::stringstream str;
                str << "patch" << index++;
                segm.setName(str.str());
                App::Color col;
                col.setPackedValue(it.first);
                segm.setColor(col.asHexString());
                meshdata->addSegment(segm);
            }
        }
        else {
            for (const auto& it : meshSegments) {
                meshdata->addSegment(it);
            }
        }
        return meshdata;
    }
};
}  // namespace MeshPart

// ----------------------------------------------------------------------------

Mesher::Mesher(const TopoDS_Shape& s)
    : shape(s)
{}

Mesher::~Mesher() = default;

Mesh::MeshObject* Mesher::createStandard() const
{
    if (!shape.IsNull()) {
        BRepTools::Clean(shape);
        BRepMesh_IncrementalMesh aMesh(shape, deflection, relative, angularDeflection);
    }

    std::vector<Part::TopoShape::Domain> domains;
    Part::TopoShape(shape).getDomains(domains);

    BrepMesh brepmesh(this->segments, this->colors);
    return brepmesh.create(domains);
}

Mesh::MeshObject* Mesher::createMesh() const
{
    // OCC standard mesher
    if (method == Standard) {
        return createStandard();
    }

#ifndef HAVE_SMESH
    throw Base::RuntimeError("SMESH is not available on this platform");
#else
    std::list<SMESH_Hypothesis*> hypoth;

    if (!Mesher::_mesh_gen) {
        Mesher::_mesh_gen = new SMESH_Gen();
    }
    SMESH_Gen* meshgen = Mesher::_mesh_gen;

#if SMESH_VERSION_MAJOR >= 9
    SMESH_Mesh* mesh = meshgen->CreateMesh(true);
#else
    SMESH_Mesh* mesh = meshgen->CreateMesh(0, true);
#endif


    int hyp = 0;

    switch (method) {
#if defined(HAVE_NETGEN)
        case Netgen: {
#if SMESH_VERSION_MAJOR >= 9
            NETGENPlugin_Hypothesis_2D* hyp2d = new NETGENPlugin_Hypothesis_2D(hyp++, meshgen);
#else
            NETGENPlugin_Hypothesis_2D* hyp2d = new NETGENPlugin_Hypothesis_2D(hyp++, 0, meshgen);
#endif

            if (fineness >= 0 && fineness < 5) {
                hyp2d->SetFineness(NETGENPlugin_Hypothesis_2D::Fineness(fineness));
            }
            // user defined values
            else {
                if (growthRate > 0) {
                    hyp2d->SetGrowthRate(growthRate);
                }
                if (nbSegPerEdge > 0) {
                    hyp2d->SetNbSegPerEdge(nbSegPerEdge);
                }
                if (nbSegPerRadius > 0) {
                    hyp2d->SetNbSegPerRadius(nbSegPerRadius);
                }
            }

            if (maxLen > 0) {
                hyp2d->SetMaxSize(maxLen);
            }
            if (minLen > 0) {
                hyp2d->SetMinSize(minLen);
            }

            hyp2d->SetQuadAllowed(allowquad);
            hyp2d->SetOptimize(optimize);
            hyp2d->SetSecondOrder(
                secondOrder);  // apply bisecting to create four triangles out of one
            hypoth.push_back(hyp2d);

#if SMESH_VERSION_MAJOR >= 9
            NETGENPlugin_NETGEN_2D* alg2d = new NETGENPlugin_NETGEN_2D(hyp++, meshgen);
#else
            NETGENPlugin_NETGEN_2D* alg2d = new NETGENPlugin_NETGEN_2D(hyp++, 0, meshgen);
#endif
            hypoth.push_back(alg2d);
        } break;
#endif
#if defined(HAVE_MEFISTO)
        case Mefisto: {
            if (maxLength > 0) {
#if SMESH_VERSION_MAJOR >= 9
                StdMeshers_MaxLength* hyp1d = new StdMeshers_MaxLength(hyp++, meshgen);
#else
                StdMeshers_MaxLength* hyp1d = new StdMeshers_MaxLength(hyp++, 0, meshgen);
#endif
                hyp1d->SetLength(maxLength);
                hypoth.push_back(hyp1d);
            }
            else if (localLength > 0) {
#if SMESH_VERSION_MAJOR >= 9
                StdMeshers_LocalLength* hyp1d = new StdMeshers_LocalLength(hyp++, meshgen);
#else
                StdMeshers_LocalLength* hyp1d = new StdMeshers_LocalLength(hyp++, 0, meshgen);
#endif
                hyp1d->SetLength(localLength);
                hypoth.push_back(hyp1d);
            }
            else if (maxArea > 0) {
#if SMESH_VERSION_MAJOR >= 9
                StdMeshers_MaxElementArea* hyp2d = new StdMeshers_MaxElementArea(hyp++, meshgen);
#else
                StdMeshers_MaxElementArea* hyp2d = new StdMeshers_MaxElementArea(hyp++, 0, meshgen);
#endif
                hyp2d->SetMaxArea(maxArea);
                hypoth.push_back(hyp2d);
            }
            else if (deflection > 0) {
#if SMESH_VERSION_MAJOR >= 9
                StdMeshers_Deflection1D* hyp1d = new StdMeshers_Deflection1D(hyp++, meshgen);
#else
                StdMeshers_Deflection1D* hyp1d = new StdMeshers_Deflection1D(hyp++, 0, meshgen);
#endif
                hyp1d->SetDeflection(deflection);
                hypoth.push_back(hyp1d);
            }
            else if (minLen > 0 && maxLen > 0) {
#if SMESH_VERSION_MAJOR >= 9
                StdMeshers_Arithmetic1D* hyp1d = new StdMeshers_Arithmetic1D(hyp++, meshgen);
#else
                StdMeshers_Arithmetic1D* hyp1d = new StdMeshers_Arithmetic1D(hyp++, 0, meshgen);
#endif
                hyp1d->SetLength(minLen, false);
                hyp1d->SetLength(maxLen, true);
                hypoth.push_back(hyp1d);
            }
            else {
#if SMESH_VERSION_MAJOR >= 9
                StdMeshers_AutomaticLength* hyp1d = new StdMeshers_AutomaticLength(hyp++, meshgen);
#else
                StdMeshers_AutomaticLength* hyp1d =
                    new StdMeshers_AutomaticLength(hyp++, 0, meshgen);
#endif
                hypoth.push_back(hyp1d);
            }

            {
#if SMESH_VERSION_MAJOR >= 9
                StdMeshers_NumberOfSegments* hyp1d =
                    new StdMeshers_NumberOfSegments(hyp++, meshgen);
#else
                StdMeshers_NumberOfSegments* hyp1d =
                    new StdMeshers_NumberOfSegments(hyp++, 0, meshgen);
#endif
                hyp1d->SetNumberOfSegments(1);
                hypoth.push_back(hyp1d);
            }

            if (regular) {
#if SMESH_VERSION_MAJOR >= 9
                StdMeshers_Regular_1D* hyp1d = new StdMeshers_Regular_1D(hyp++, meshgen);
#else
                StdMeshers_Regular_1D* hyp1d = new StdMeshers_Regular_1D(hyp++, 0, meshgen);
#endif
                hypoth.push_back(hyp1d);
            }
#if SMESH_VERSION_MAJOR < 7
            StdMeshers_TrianglePreference* hyp2d_1 =
                new StdMeshers_TrianglePreference(hyp++, 0, meshgen);
            hypoth.push_back(hyp2d_1);
#endif
#if SMESH_VERSION_MAJOR >= 9
            StdMeshers_MEFISTO_2D* alg2d = new StdMeshers_MEFISTO_2D(hyp++, meshgen);
#else
            StdMeshers_MEFISTO_2D* alg2d = new StdMeshers_MEFISTO_2D(hyp++, 0, meshgen);
#endif
            hypoth.push_back(alg2d);
        } break;
#endif
        default:
            break;
    }

    // Set new cout
    MeshingOutput stdcout;
    std::streambuf* oldcout = std::cout.rdbuf(&stdcout);

    // Apply the hypothesis and create the mesh
    mesh->ShapeToMesh(shape);
    for (int i = 0; i < hyp; i++) {
        mesh->AddHypothesis(shape, i);
    }
    meshgen->Compute(*mesh, mesh->GetShapeToMesh());

    // Restore old cout
    std::cout.rdbuf(oldcout);

    // build up the mesh structure
    Mesh::MeshObject* meshdata = createFrom(mesh);

    // clean up
    TopoDS_Shape aNull;
    mesh->ShapeToMesh(aNull);
    mesh->Clear();
    delete mesh;
    for (auto it : hypoth) {
        delete it;
    }

    return meshdata;
#endif  // HAVE_SMESH
}

Mesh::MeshObject* Mesher::createFrom(SMESH_Mesh* mesh) const
{
    // build up the mesh structure
    SMDS_FaceIteratorPtr aFaceIter = mesh->GetMeshDS()->facesIterator();
    SMDS_NodeIteratorPtr aNodeIter = mesh->GetMeshDS()->nodesIterator();

    MeshCore::MeshPointArray verts;
    MeshCore::MeshFacetArray faces;
    verts.reserve(mesh->NbNodes());
    faces.reserve(mesh->NbFaces());

    int index = 0;
    std::map<const SMDS_MeshNode*, int> mapNodeIndex;
    for (; aNodeIter->more();) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        MeshCore::MeshPoint p;
        p.Set((float)aNode->X(), (float)aNode->Y(), (float)aNode->Z());
        verts.push_back(p);
        mapNodeIndex[aNode] = index++;
    }

    for (; aFaceIter->more();) {
        const SMDS_MeshFace* aFace = aFaceIter->next();
        if (aFace->NbNodes() == 3) {
            MeshCore::MeshFacet f;
            for (int i = 0; i < 3; i++) {
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
        else if (aFace->NbNodes() == 8) {
            MeshCore::MeshFacet f1, f2, f3, f4, f5, f6;
            const SMDS_MeshNode* node0 = aFace->GetNode(0);
            const SMDS_MeshNode* node1 = aFace->GetNode(1);
            const SMDS_MeshNode* node2 = aFace->GetNode(2);
            const SMDS_MeshNode* node3 = aFace->GetNode(3);
            const SMDS_MeshNode* node4 = aFace->GetNode(4);
            const SMDS_MeshNode* node5 = aFace->GetNode(5);
            const SMDS_MeshNode* node6 = aFace->GetNode(6);
            const SMDS_MeshNode* node7 = aFace->GetNode(7);

            f1._aulPoints[0] = mapNodeIndex[node0];
            f1._aulPoints[1] = mapNodeIndex[node4];
            f1._aulPoints[2] = mapNodeIndex[node7];

            f2._aulPoints[0] = mapNodeIndex[node1];
            f2._aulPoints[1] = mapNodeIndex[node5];
            f2._aulPoints[2] = mapNodeIndex[node4];

            f3._aulPoints[0] = mapNodeIndex[node2];
            f3._aulPoints[1] = mapNodeIndex[node6];
            f3._aulPoints[2] = mapNodeIndex[node5];

            f4._aulPoints[0] = mapNodeIndex[node3];
            f4._aulPoints[1] = mapNodeIndex[node7];
            f4._aulPoints[2] = mapNodeIndex[node6];

            // Two solutions are possible:
            // <4,6,7>, <4,5,6> or <4,5,7>, <5,6,7>
            Base::Vector3d v4(node4->X(), node4->Y(), node4->Z());
            Base::Vector3d v5(node5->X(), node5->Y(), node5->Z());
            Base::Vector3d v6(node6->X(), node6->Y(), node6->Z());
            Base::Vector3d v7(node7->X(), node7->Y(), node7->Z());
            double dist46 = Base::DistanceP2(v4, v6);
            double dist57 = Base::DistanceP2(v5, v7);
            if (dist46 > dist57) {
                f5._aulPoints[0] = mapNodeIndex[node4];
                f5._aulPoints[1] = mapNodeIndex[node6];
                f5._aulPoints[2] = mapNodeIndex[node7];

                f6._aulPoints[0] = mapNodeIndex[node4];
                f6._aulPoints[1] = mapNodeIndex[node5];
                f6._aulPoints[2] = mapNodeIndex[node6];
            }
            else {
                f5._aulPoints[0] = mapNodeIndex[node4];
                f5._aulPoints[1] = mapNodeIndex[node5];
                f5._aulPoints[2] = mapNodeIndex[node7];

                f6._aulPoints[0] = mapNodeIndex[node5];
                f6._aulPoints[1] = mapNodeIndex[node6];
                f6._aulPoints[2] = mapNodeIndex[node7];
            }

            faces.push_back(f1);
            faces.push_back(f2);
            faces.push_back(f3);
            faces.push_back(f4);
            faces.push_back(f5);
            faces.push_back(f6);
        }
        else {
            Base::Console().Warning("Face with %d nodes ignored\n", aFace->NbNodes());
        }
    }

    MeshCore::MeshKernel kernel;
    kernel.Adopt(verts, faces, true);

    Mesh::MeshObject* meshdata = new Mesh::MeshObject();
    meshdata->swap(kernel);
    return meshdata;
}
