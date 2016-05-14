/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2009     *
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
# include <cstdlib>
# include <memory>
# include <strstream>
# include <Bnd_Box.hxx>
# include <BRep_Tool.hxx>
# include <BRepBndLib.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <TopoDS_Vertex.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <gp_Pnt.hxx>
#endif

#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/TimeInfo.h>
#include <Base/Console.h>
#include <App/Application.h>

#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/Evaluation.h>
#include <Mod/Mesh/App/Core/Iterator.h>

#include "FemMesh.h"

#include <boost/assign/list_of.hpp>
#include <SMESH_Gen.hxx>
#include <SMESH_Mesh.hxx>
#include <SMDS_PolyhedralVolumeOfNodes.hxx>
#include <SMDS_VolumeTool.hxx>
#include <StdMeshers_MaxLength.hxx>
#include <StdMeshers_LocalLength.hxx>
#include <StdMeshers_MaxElementArea.hxx>
#include <StdMeshers_NumberOfSegments.hxx>
#include <StdMeshers_Deflection1D.hxx>
#include <StdMeshers_Regular_1D.hxx>
#include <StdMeshers_StartEndLength.hxx>
#include <StdMeshers_QuadranglePreference.hxx>
#include <StdMeshers_Quadrangle_2D.hxx>
#include <StdMeshers_QuadraticMesh.hxx>

# include <TopoDS_Face.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Shape.hxx>

//to simplify parsing input files we use the boost lib
#include <boost/tokenizer.hpp>


using namespace Fem;
using namespace Base;
using namespace boost;

static int StatCount = 0;

TYPESYSTEM_SOURCE(Fem::FemMesh , Base::Persistence);

FemMesh::FemMesh()
{
    //Base::Console().Log("FemMesh::FemMesh():%p (id=%i)\n",this,StatCount);
    myGen = new SMESH_Gen();
    // create a mesh allways with new StudyId to avoid overlapping destruction
    myMesh = myGen->CreateMesh(StatCount++,false);

}

FemMesh::FemMesh(const FemMesh& mesh)
{
    //Base::Console().Log("FemMesh::FemMesh(mesh):%p (id=%i)\n",this,StatCount);
    myGen = new SMESH_Gen();
    myMesh = myGen->CreateMesh(StatCount++,false);
    copyMeshData(mesh);
}

FemMesh::~FemMesh()
{
    //Base::Console().Log("FemMesh::~FemMesh():%p\n",this);

    TopoDS_Shape aNull;
    myMesh->ShapeToMesh(aNull);
    myMesh->Clear();
    //myMesh->ClearLog();
    delete myMesh;
#if defined(__GNUC__)
    delete myGen; // crashes with MSVC
#endif
}

FemMesh &FemMesh::operator=(const FemMesh& mesh)
{
    if (this != &mesh) {
	myGen = new SMESH_Gen();
	myMesh = myGen->CreateMesh(0,true);
        copyMeshData(mesh);
    }
    return *this;
}

void FemMesh::copyMeshData(const FemMesh& mesh)
{
    SMESH_Mesh *test;
    _Mtrx = mesh._Mtrx;

    SMESHDS_Mesh* meshds = this->myMesh->GetMeshDS();
    
    SMDS_NodeIteratorPtr aNodeIter = mesh.myMesh->GetMeshDS()->nodesIterator();
    for (;aNodeIter->more();) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        double temp[3];
        aNode->GetXYZ(temp);
        meshds->AddNodeWithID(temp[0],temp[1],temp[2], aNode->GetID());
    }
    SMDS_EdgeIteratorPtr aEdgeIter = mesh.myMesh->GetMeshDS()->edgesIterator();
    for (;aEdgeIter->more();) {
        const SMDS_MeshEdge* aEdge = aEdgeIter->next();
        meshds->AddEdgeWithID(aEdge->GetNode(0), aEdge->GetNode(1), aEdge->GetID());
    }

    SMDS_FaceIteratorPtr aFaceIter = mesh.myMesh->GetMeshDS()->facesIterator();
    for (;aFaceIter->more();) {
        const SMDS_MeshFace* aFace = aFaceIter->next();
        switch (aFace->NbNodes()) {
            case 3:
                meshds->AddFaceWithID(aFace->GetNode(0),
                                      aFace->GetNode(1),
                                      aFace->GetNode(2),
                                      aFace->GetID());
                break;
            case 4:
                meshds->AddFaceWithID(aFace->GetNode(0),
                                      aFace->GetNode(1),
                                      aFace->GetNode(2),
                                      aFace->GetNode(3),
                                      aFace->GetID());
                break;
            case 6:
                meshds->AddFaceWithID(aFace->GetNode(0),
                                      aFace->GetNode(1),
                                      aFace->GetNode(2),
                                      aFace->GetNode(3),
                                      aFace->GetNode(4),
                                      aFace->GetNode(5),
                                      aFace->GetID());
                break;
            case 8:
                meshds->AddFaceWithID(aFace->GetNode(0),
                                      aFace->GetNode(1),
                                      aFace->GetNode(2),
                                      aFace->GetNode(3),
                                      aFace->GetNode(4),
                                      aFace->GetNode(5),
                                      aFace->GetNode(6),
                                      aFace->GetNode(7),
                                      aFace->GetID());
                break;
            default:
                {
                    std::vector<const SMDS_MeshNode*> aNodes;
                    for (int i=0; aFace->NbNodes(); i++)
                        aNodes.push_back(aFace->GetNode(0));
                    meshds->AddPolygonalFaceWithID(aNodes, aFace->GetID());
                }
                break;
        }
    }

    SMDS_VolumeIteratorPtr aVolIter = mesh.myMesh->GetMeshDS()->volumesIterator();
    for (;aVolIter->more();) {
        const SMDS_MeshVolume* aVol = aVolIter->next();
        switch (aVol->NbNodes()) {
            case 4:
                meshds->AddVolumeWithID(aVol->GetNode(0),
                                        aVol->GetNode(1),
                                        aVol->GetNode(2),
                                        aVol->GetNode(3),
                                        aVol->GetID());
                break;
            case 5:
                meshds->AddVolumeWithID(aVol->GetNode(0),
                                        aVol->GetNode(1),
                                        aVol->GetNode(2),
                                        aVol->GetNode(3),
                                        aVol->GetNode(4),
                                        aVol->GetID());
                break;
            case 6:
                meshds->AddVolumeWithID(aVol->GetNode(0),
                                        aVol->GetNode(1),
                                        aVol->GetNode(2),
                                        aVol->GetNode(3),
                                        aVol->GetNode(4),
                                        aVol->GetNode(5),
                                        aVol->GetID());
                break;
            case 8:
                meshds->AddVolumeWithID(aVol->GetNode(0),
                                        aVol->GetNode(1),
                                        aVol->GetNode(2),
                                        aVol->GetNode(3),
                                        aVol->GetNode(4),
                                        aVol->GetNode(5),
                                        aVol->GetNode(6),
                                        aVol->GetNode(7),
                                        aVol->GetID());
                break;
            case 10:
                meshds->AddVolumeWithID(aVol->GetNode(0),
                                        aVol->GetNode(1),
                                        aVol->GetNode(2),
                                        aVol->GetNode(3),
                                        aVol->GetNode(4),
                                        aVol->GetNode(5),
                                        aVol->GetNode(6),
                                        aVol->GetNode(7),
                                        aVol->GetNode(8),
                                        aVol->GetNode(9),
                                        aVol->GetID());
                break;
            case 13:
                meshds->AddVolumeWithID(aVol->GetNode(0),
                                        aVol->GetNode(1),
                                        aVol->GetNode(2),
                                        aVol->GetNode(3),
                                        aVol->GetNode(4),
                                        aVol->GetNode(5),
                                        aVol->GetNode(6),
                                        aVol->GetNode(7),
                                        aVol->GetNode(8),
                                        aVol->GetNode(9),
                                        aVol->GetNode(10),
                                        aVol->GetNode(11),
                                        aVol->GetNode(12),
                                        aVol->GetID());
                break;
            case 15:
                meshds->AddVolumeWithID(aVol->GetNode(0),
                                        aVol->GetNode(1),
                                        aVol->GetNode(2),
                                        aVol->GetNode(3),
                                        aVol->GetNode(4),
                                        aVol->GetNode(5),
                                        aVol->GetNode(6),
                                        aVol->GetNode(7),
                                        aVol->GetNode(8),
                                        aVol->GetNode(9),
                                        aVol->GetNode(10),
                                        aVol->GetNode(11),
                                        aVol->GetNode(12),
                                        aVol->GetNode(13),
                                        aVol->GetNode(14),
                                        aVol->GetID());
                break;
            case 20:
                meshds->AddVolumeWithID(aVol->GetNode(0),
                                        aVol->GetNode(1),
                                        aVol->GetNode(2),
                                        aVol->GetNode(3),
                                        aVol->GetNode(4),
                                        aVol->GetNode(5),
                                        aVol->GetNode(6),
                                        aVol->GetNode(7),
                                        aVol->GetNode(8),
                                        aVol->GetNode(9),
                                        aVol->GetNode(10),
                                        aVol->GetNode(11),
                                        aVol->GetNode(12),
                                        aVol->GetNode(13),
                                        aVol->GetNode(14),
                                        aVol->GetNode(15),
                                        aVol->GetNode(16),
                                        aVol->GetNode(17),
                                        aVol->GetNode(18),
                                        aVol->GetNode(19),
                                        aVol->GetID());
                break;
            default:
                {
                    if (aVol->IsPoly()) {
                        const SMDS_PolyhedralVolumeOfNodes* aPolyVol = dynamic_cast<const SMDS_PolyhedralVolumeOfNodes*>(aVol);
                        if (!aPolyVol) break;
                        std::vector<const SMDS_MeshNode*> aNodes;
                        for (int i=0; i<aPolyVol->NbNodes(); i++)
                            aNodes.push_back(aPolyVol->GetNode(i));
                        meshds->AddPolyhedralVolumeWithID(aNodes,
                            aPolyVol->GetQuanities(), aPolyVol->GetID());
                    }
                }
                break;
        }
    }
}

const SMESH_Mesh* FemMesh::getSMesh() const
{
    return myMesh;
}

SMESH_Mesh* FemMesh::getSMesh()
{
    return myMesh;
}


SMESH_Gen * FemMesh::getGenerator()
{
    return myGen;
}

void FemMesh::addHypothesis(const TopoDS_Shape & aSubShape, SMESH_HypothesisPtr hyp)
{
    myMesh->AddHypothesis(aSubShape, hyp->GetID());
    SMESH_HypothesisPtr ptr(hyp);
    hypoth.push_back(ptr);
}

void FemMesh::setStanardHypotheses()
{
    if (!hypoth.empty())
        return;
    int hyp=0;
    SMESH_HypothesisPtr len(new StdMeshers_MaxLength(hyp++, 1, myGen));
    static_cast<StdMeshers_MaxLength*>(len.get())->SetLength(1.0);
    hypoth.push_back(len);

    SMESH_HypothesisPtr loc(new StdMeshers_LocalLength(hyp++, 1, myGen));
    static_cast<StdMeshers_LocalLength*>(loc.get())->SetLength(1.0);
    hypoth.push_back(loc);

    SMESH_HypothesisPtr area(new StdMeshers_MaxElementArea(hyp++, 1, myGen));
    static_cast<StdMeshers_MaxElementArea*>(area.get())->SetMaxArea(1.0);
    hypoth.push_back(area);

    SMESH_HypothesisPtr segm(new StdMeshers_NumberOfSegments(hyp++, 1, myGen));
    static_cast<StdMeshers_NumberOfSegments*>(segm.get())->SetNumberOfSegments(1);
    hypoth.push_back(segm);

    SMESH_HypothesisPtr defl(new StdMeshers_Deflection1D(hyp++, 1, myGen));
    static_cast<StdMeshers_Deflection1D*>(defl.get())->SetDeflection(0.01);
    hypoth.push_back(defl);

    SMESH_HypothesisPtr reg(new StdMeshers_Regular_1D(hyp++, 1, myGen));
    hypoth.push_back(reg);

    //SMESH_HypothesisPtr sel(new StdMeshers_StartEndLength(hyp++, 1, myGen));
    //static_cast<StdMeshers_StartEndLength*>(sel.get())->SetLength(1.0, true);
    //hypoth.push_back(sel);

    SMESH_HypothesisPtr qdp(new StdMeshers_QuadranglePreference(hyp++,1,myGen));
    hypoth.push_back(qdp);

    SMESH_HypothesisPtr q2d(new StdMeshers_Quadrangle_2D(hyp++,1,myGen));
    hypoth.push_back(q2d);

    // Apply hypothesis
    for (int i=0; i<hyp;i++)
        myMesh->AddHypothesis(myMesh->GetShapeToMesh(), i);
}

void FemMesh::compute()
{
    myGen->Compute(*myMesh, myMesh->GetShapeToMesh());
}

std::set<long> FemMesh::getSurfaceNodes(long ElemId, short FaceId, float Angle) const
{
    std::set<long> result;
    //const SMESHDS_Mesh* data = myMesh->GetMeshDS();

    //const SMDS_MeshElement * element = data->FindElement(ElemId);
    //int fNbr = element->NbFaces();
    //element->

    return result;
}

/*! That function returns map containing volume ID and face ID.
 */
std::list<std::pair<int, int> > FemMesh::getVolumesByFace(const TopoDS_Face &face) const
{
    std::list<std::pair<int, int> > result;
    std::set<int> nodes_on_face = getNodesByFace(face);

    SMDS_VolumeIteratorPtr vol_iter = myMesh->GetMeshDS()->volumesIterator();
    while (vol_iter->more()) {
        const SMDS_MeshVolume* vol = vol_iter->next();
        SMDS_ElemIteratorPtr face_iter = vol->facesIterator();

        while (face_iter && face_iter->more()) {
            const SMDS_MeshFace* face = static_cast<const SMDS_MeshFace*>(face_iter->next());
            int numNodes = face->NbNodes();

            std::set<int> face_nodes;
            for (int i=0; i<numNodes; i++) {
                face_nodes.insert(face->GetNode(i)->GetID());
            }

            std::vector<int> element_face_nodes;
            std::set_intersection(nodes_on_face.begin(), nodes_on_face.end(), face_nodes.begin(), face_nodes.end(),
                std::back_insert_iterator<std::vector<int> >(element_face_nodes));

            // For curved faces it is possible that a volume contributes more than one face
            if (element_face_nodes.size() == static_cast<std::size_t>(numNodes)) {
                result.push_back(std::make_pair(vol->GetID(), face->GetID()));
            }
        }
    }

    result.sort();
    return result;
}

/*! That function returns map containing volume ID and face number
 * as per CalculiX definition for tetrahedral elements. See CalculiX
 * documentation for the details.
 */
std::map<int, int> FemMesh::getccxVolumesByFace(const TopoDS_Face &face) const
{
    std::map<int, int> result;
    std::set<int> nodes_on_face = getNodesByFace(face);

    static std::map<int, std::vector<int> > elem_order;
    if (elem_order.empty()) {
        std::vector<int> c3d4  = boost::assign::list_of(1)(0)(2)(3);
        std::vector<int> c3d10 = boost::assign::list_of(1)(0)(2)(3)(4)(6)(5)(8)(7)(9);

        elem_order.insert(std::make_pair(c3d4.size(), c3d4));
        elem_order.insert(std::make_pair(c3d10.size(), c3d10));
    }

    SMDS_VolumeIteratorPtr vol_iter = myMesh->GetMeshDS()->volumesIterator();
    std::set<int> element_nodes;
    int num_of_nodes;
    while (vol_iter->more()) {
        const SMDS_MeshVolume* vol = vol_iter->next();
        num_of_nodes = vol->NbNodes();
        std::pair<int, std::vector<int> > apair;
        apair.first = vol->GetID();

        std::map<int, std::vector<int> >::iterator it = elem_order.find(num_of_nodes);
        if (it != elem_order.end()) {
            const std::vector<int>& order = it->second;
            for (std::vector<int>::const_iterator jt = order.begin(); jt != order.end(); ++jt) {
                int vid = vol->GetNode(*jt)->GetID();
                apair.second.push_back(vid);
            }
        }

        // Get volume nodes on face
        std::vector<int> element_face_nodes;
        std::set<int> element_nodes;
        element_nodes.insert(apair.second.begin(), apair.second.end());
        std::set_intersection(nodes_on_face.begin(), nodes_on_face.end(), element_nodes.begin(), element_nodes.end(),
        std::back_insert_iterator<std::vector<int> >(element_face_nodes));

        if ((element_face_nodes.size() == 3 && num_of_nodes == 4) ||
            (element_face_nodes.size() == 6 && num_of_nodes == 10)) {
            int missing_node = 0;
            for (int i=0; i<4; i++) {
                // search for the ID of the volume which is not part of 'element_face_nodes'
                if (std::find(element_face_nodes.begin(), element_face_nodes.end(), apair.second[i]) == element_face_nodes.end()) {
                    missing_node = i + 1;
                    break;
                }
            }
            /* for tetrahedral elements as per CalculiX definition:
             Face 1: 1-2-3, missing point 4 means it's face P1
             Face 2: 1-4-2, missing point 3 means it's face P2
             Face 3: 2-4-3, missing point 1 means it's face P3
             Face 4: 3-4-1, missing point 2 means it's face P4 */
            int face_ccx = 0;
            switch (missing_node) {
            case 1:
                face_ccx = 3;
                break;
            case 2:
                face_ccx = 4;
                break;
            case 3:
                face_ccx = 2;
                break;
            case 4:
                face_ccx = 1;
                break;
            default:
                assert(false); // should never happen
                break;
            }
            result[apair.first] = face_ccx;
        }
    }

    return result;
}

std::set<int> FemMesh::getNodesBySolid(const TopoDS_Solid &solid) const
{
    std::set<int> result;

    Bnd_Box box;
    BRepBndLib::Add(solid, box);
    // limit where the mesh node belongs to the solid:
    double limit = box.SquareExtent()/10000.0;
    //double limit = BRep_Tool::Tolerance(solid);   // does not compile --> no matching function for call to 'BRep_Tool::Tolerance(const TopoDS_Solid&)'
    box.Enlarge(limit);

    // get the current transform of the FemMesh
    const Base::Matrix4D Mtrx(getTransform());

    SMDS_NodeIteratorPtr aNodeIter = myMesh->GetMeshDS()->nodesIterator();
    while (aNodeIter->more()) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        Base::Vector3d vec(aNode->X(),aNode->Y(),aNode->Z());
        // Apply the matrix to hold the BoundBox in absolute space.
        vec = Mtrx * vec;

        if (!box.IsOut(gp_Pnt(vec.x,vec.y,vec.z))) {
            // create a vertex
            BRepBuilderAPI_MakeVertex aBuilder(gp_Pnt(vec.x,vec.y,vec.z));
            TopoDS_Shape s = aBuilder.Vertex();
            // measure distance
            BRepExtrema_DistShapeShape measure(solid,s);
            measure.Perform();
            if (!measure.IsDone() || measure.NbSolution() < 1)
                continue;

            if (measure.Value() < limit)
                result.insert(aNode->GetID());
        }
    }
    return result;
}

std::set<int> FemMesh::getNodesByFace(const TopoDS_Face &face) const
{
    std::set<int> result;

    Bnd_Box box;
    BRepBndLib::Add(face, box);
    // limit where the mesh node belongs to the face:
    double limit = BRep_Tool::Tolerance(face);
    box.Enlarge(limit);

    // get the current transform of the FemMesh
    const Base::Matrix4D Mtrx(getTransform());

    SMDS_NodeIteratorPtr aNodeIter = myMesh->GetMeshDS()->nodesIterator();
    while (aNodeIter->more()) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        Base::Vector3d vec(aNode->X(),aNode->Y(),aNode->Z());
        // Apply the matrix to hold the BoundBox in absolute space.
        vec = Mtrx * vec;

        if (!box.IsOut(gp_Pnt(vec.x,vec.y,vec.z))) {
            // create a vertex
            BRepBuilderAPI_MakeVertex aBuilder(gp_Pnt(vec.x,vec.y,vec.z));
            TopoDS_Shape s = aBuilder.Vertex();
            // measure distance
            BRepExtrema_DistShapeShape measure(face,s);
            measure.Perform();
            if (!measure.IsDone() || measure.NbSolution() < 1)
                continue;

            if (measure.Value() < limit)
                result.insert(aNode->GetID());
        }
    }

    return result;
}

std::set<int> FemMesh::getNodesByEdge(const TopoDS_Edge &edge) const
{
    std::set<int> result;

    Bnd_Box box;
    BRepBndLib::Add(edge, box);
    // limit where the mesh node belongs to the edge:
    double limit = BRep_Tool::Tolerance(edge);
    box.Enlarge(limit);

    // get the current transform of the FemMesh
    const Base::Matrix4D Mtrx(getTransform());

    SMDS_NodeIteratorPtr aNodeIter = myMesh->GetMeshDS()->nodesIterator();
    while (aNodeIter->more()) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        Base::Vector3d vec(aNode->X(),aNode->Y(),aNode->Z());
        // Apply the matrix to hold the BoundBox in absolute space.
        vec = Mtrx * vec;

        if (!box.IsOut(gp_Pnt(vec.x,vec.y,vec.z))) {
            // create a vertex
            BRepBuilderAPI_MakeVertex aBuilder(gp_Pnt(vec.x,vec.y,vec.z));
            TopoDS_Shape s = aBuilder.Vertex();
            // measure distance
            BRepExtrema_DistShapeShape measure(edge,s);
            measure.Perform();
            if (!measure.IsDone() || measure.NbSolution() < 1)
                continue;

            if (measure.Value() < limit)
                result.insert(aNode->GetID());
        }
    }

    return result;
}

std::set<int> FemMesh::getNodesByVertex(const TopoDS_Vertex &vertex) const
{
    std::set<int> result;

    double limit = BRep_Tool::Tolerance(vertex);
    limit *= limit; // use square to improve speed
    gp_Pnt pnt = BRep_Tool::Pnt(vertex);
    Base::Vector3d node(pnt.X(), pnt.Y(), pnt.Z());

    // get the current transform of the FemMesh
    const Base::Matrix4D Mtrx(getTransform());

    SMDS_NodeIteratorPtr aNodeIter = myMesh->GetMeshDS()->nodesIterator();
    while (aNodeIter->more()) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        Base::Vector3d vec(aNode->X(),aNode->Y(),aNode->Z());
        vec = Mtrx * vec;

        if (Base::DistanceP2(node, vec) <= limit) {
            result.insert(aNode->GetID());
        }
    }

    return result;
}

std::list<int> FemMesh::getElementNodes(int id) const
{
    std::list<int> result;
    const SMDS_MeshElement* elem = myMesh->GetMeshDS()->FindElement(id);
    if (elem) {
        for (int i = 0; i < elem->NbNodes(); i++)
            result.push_back(elem->GetNode(i)->GetID());
    }
    return result;
}

void FemMesh::readNastran(const std::string &Filename)
{
    Base::TimeInfo Start;
    Base::Console().Log("Start: FemMesh::readNastran() =================================\n");

    _Mtrx = Base::Matrix4D();

    std::ifstream inputfile;
    inputfile.open(Filename.c_str());
    inputfile.seekg(std::ifstream::beg);
    std::string line1,line2,temp;
    std::vector<string> token_results;
    token_results.clear();
    Base::Vector3d current_node;
    std::vector<Base::Vector3d> vertices;
    vertices.clear();
    std::vector<unsigned int> nodal_id;
    nodal_id.clear();
    std::vector<unsigned int> tetra_element;
    std::vector<std::vector<unsigned int> > all_elements;
    std::vector<unsigned int> element_id;
    element_id.clear();
    bool nastran_free_format = false;
    do
    {
        std::getline(inputfile,line1);
        if (line1.size() == 0) continue;
        if (!nastran_free_format && line1.find(",")!= std::string::npos)
            nastran_free_format = true;
        if (!nastran_free_format && line1.find("GRID*")!= std::string::npos ) //We found a Grid line
        {
            //Now lets extract the GRID Points = Nodes
            //As each GRID Line consists of two subsequent lines we have to
            //take care of that as well
            std::getline(inputfile,line2);
            //Get the Nodal ID
            nodal_id.push_back(atoi(line1.substr(8,24).c_str()));
            //Extract X Value
            current_node.x = atof(line1.substr(40,56).c_str());
            //Extract Y Value
            current_node.y = atof(line1.substr(56,72).c_str());
            //Extract Z Value
            current_node.z = atof(line2.substr(8,24).c_str());

            vertices.push_back(current_node);
        }
        else if (!nastran_free_format && line1.find("CTETRA")!= std::string::npos)
        {
            tetra_element.clear();
            //Lets extract the elements
            //As each Element Line consists of two subsequent lines as well
            //we have to take care of that
            //At a first step we only extract Quadratic Tetrahedral Elements
            std::getline(inputfile,line2);
            unsigned int id = atoi(line1.substr(8,16).c_str());
            int offset = 0;

            if(id < 1000000)
                offset = 0;
            else if (id < 10000000)
                offset = 1;
            else if (id < 100000000)
                offset = 2;


            element_id.push_back(id);
            tetra_element.push_back(atoi(line1.substr(24,32).c_str()));
            tetra_element.push_back(atoi(line1.substr(32,40).c_str()));
            tetra_element.push_back(atoi(line1.substr(40,48).c_str()));
            tetra_element.push_back(atoi(line1.substr(48,56).c_str()));
            tetra_element.push_back(atoi(line1.substr(56,64).c_str()));
            tetra_element.push_back(atoi(line1.substr(64,72).c_str()));
            tetra_element.push_back(atoi(line2.substr(8+offset,16+offset).c_str()));
            tetra_element.push_back(atoi(line2.substr(16+offset,24+offset).c_str()));
            tetra_element.push_back(atoi(line2.substr(24+offset,32+offset).c_str()));
            tetra_element.push_back(atoi(line2.substr(32+offset,40+offset).c_str()));

            all_elements.push_back(tetra_element);
        }
        else if (nastran_free_format && line1.find("GRID")!= std::string::npos ) //We found a Grid line
        {
            char_separator<char> sep(",");
            tokenizer<char_separator<char> > tokens(line1, sep);
            token_results.assign(tokens.begin(),tokens.end());
            if (token_results.size() < 3)
                continue;//Line does not include Nodal coordinates
            nodal_id.push_back(atoi(token_results[1].c_str()));
            current_node.x = atof(token_results[3].c_str());
            current_node.y = atof(token_results[4].c_str());
            current_node.z = atof(token_results[5].c_str());
            vertices.push_back(current_node);
        }
        else if (nastran_free_format && line1.find("CTETRA")!= std::string::npos)
        {
            tetra_element.clear();
            //Lets extract the elements
            //As each Element Line consists of two subsequent lines as well
            //we have to take care of that
            //At a first step we only extract Quadratic Tetrahedral Elements
            std::getline(inputfile,line2);
            char_separator<char> sep(",");
            tokenizer<char_separator<char> > tokens(line1.append(line2), sep);
            token_results.assign(tokens.begin(),tokens.end());
            if (token_results.size() < 11)
                continue;//Line does not include enough nodal IDs
            element_id.push_back(atoi(token_results[1].c_str()));
            tetra_element.push_back(atoi(token_results[3].c_str()));
            tetra_element.push_back(atoi(token_results[4].c_str()));
            tetra_element.push_back(atoi(token_results[5].c_str()));
            tetra_element.push_back(atoi(token_results[6].c_str()));
            tetra_element.push_back(atoi(token_results[7].c_str()));
            tetra_element.push_back(atoi(token_results[8].c_str()));
            tetra_element.push_back(atoi(token_results[10].c_str()));
            tetra_element.push_back(atoi(token_results[11].c_str()));
            tetra_element.push_back(atoi(token_results[12].c_str()));
            tetra_element.push_back(atoi(token_results[13].c_str()));

            all_elements.push_back(tetra_element);
        }

    }
    while (inputfile.good());
    inputfile.close();

    Base::Console().Log("    %f: File read, start building mesh\n",Base::TimeInfo::diffTimeF(Start,Base::TimeInfo()));

    //Now fill the SMESH datastructure
    std::vector<Base::Vector3d>::const_iterator anodeiterator;
    SMESHDS_Mesh* meshds = this->myMesh->GetMeshDS();
    meshds->ClearMesh();
    unsigned int j=0;
    for(anodeiterator=vertices.begin(); anodeiterator!=vertices.end(); anodeiterator++)
    {
        meshds->AddNodeWithID((*anodeiterator).x,(*anodeiterator).y,(*anodeiterator).z,nodal_id[j]);
        j++;
    }

    for(unsigned int i=0;i<all_elements.size();i++)
    {
        //Die Reihenfolge wie hier die Elemente hinzugefügt werden ist sehr wichtig.
        //Ansonsten ist eine konsistente Datenstruktur nicht möglich
        //meshds->AddVolumeWithID
        //(
        //    meshds->FindNode(all_elements[i][0]),
        //    meshds->FindNode(all_elements[i][2]),
        //    meshds->FindNode(all_elements[i][1]),
        //    meshds->FindNode(all_elements[i][3]),
        //    meshds->FindNode(all_elements[i][6]),
        //    meshds->FindNode(all_elements[i][5]),
        //    meshds->FindNode(all_elements[i][4]),
        //    meshds->FindNode(all_elements[i][9]),
        //    meshds->FindNode(all_elements[i][7]),
        //    meshds->FindNode(all_elements[i][8]),
        //    element_id[i]
        //);
        meshds->AddVolumeWithID
        (
            meshds->FindNode(all_elements[i][1]),
            meshds->FindNode(all_elements[i][0]),
            meshds->FindNode(all_elements[i][2]),
            meshds->FindNode(all_elements[i][3]),
            meshds->FindNode(all_elements[i][4]),
            meshds->FindNode(all_elements[i][6]),
            meshds->FindNode(all_elements[i][5]),
            meshds->FindNode(all_elements[i][8]),
            meshds->FindNode(all_elements[i][7]),
            meshds->FindNode(all_elements[i][9]),
            element_id[i]
        );
    }
    Base::Console().Log("    %f: Done \n",Base::TimeInfo::diffTimeF(Start,Base::TimeInfo()));

}


void FemMesh::read(const char *FileName)
{
    Base::FileInfo File(FileName);
    _Mtrx = Base::Matrix4D();

    // checking on the file
    if (!File.isReadable())
        throw Base::Exception("File to load not existing or not readable");

    if (File.hasExtension("unv") ) {
        // read UNV file
        myMesh->UNVToMesh(File.filePath().c_str());
    }
    else if (File.hasExtension("med") ) {
        myMesh->MEDToMesh(File.filePath().c_str(),File.fileNamePure().c_str());
    }
    else if (File.hasExtension("stl") ) {
        // read brep-file
        myMesh->STLToMesh(File.filePath().c_str());
    }
    else if (File.hasExtension("dat") ) {
        // read brep-file
	// vejmarie disable
        myMesh->DATToMesh(File.filePath().c_str());
    }
    else if (File.hasExtension("bdf") ) {
        // read Nastran-file
        readNastran(File.filePath());
    }
    else{
        throw Base::Exception("Unknown extension");
    }
}

void FemMesh::writeABAQUS(const std::string &Filename) const
{
    static std::map<std::string, std::vector<int> > elemOrderMap;
    static std::map<int, std::string> edgeTypeMap;
    static std::map<int, std::string> faceTypeMap;
    static std::map<int, std::string> volTypeMap;
    if (elemOrderMap.empty()) {
        // node order fits with node order in ccxFrdReader.py module to import CalculiX result meshes

        // dimension 1
        //
        // seg2 FreeCAD --> CalculiX B31
        // N1, N2
        std::vector<int> b31 = boost::assign::list_of(0)(1);
        //
        // seg3 FreeCAD --> CalculiX B32
        // N1, N3, N2
        std::vector<int> b32 = boost::assign::list_of(0)(2)(1);

        elemOrderMap.insert(std::make_pair("B31", b31));
        edgeTypeMap.insert(std::make_pair(elemOrderMap["B31"].size(), "B31"));
        elemOrderMap.insert(std::make_pair("B32", b32));
        edgeTypeMap.insert(std::make_pair(elemOrderMap["B32"].size(), "B32"));

        // dimension 2
        //
        // tria3 FreeCAD --> S3 CalculiX
        // N1, N2, N3
        std::vector<int> s3 = boost::assign::list_of(0)(1)(2);
        //
        // tria6 FreeCAD --> S6 CalculiX
        // N1, N2, N3, N4, N5, N6
        std::vector<int> s6 = boost::assign::list_of(0)(1)(2)(3)(4)(5);
        //
        // quad4 FreeCAD --> S4 CalculiX
        // N1, N2, N3, N4
        std::vector<int> s4 = boost::assign::list_of(0)(1)(2)(3);
        //
        // quad8 FreeCAD --> S8 CalculiX
        // N1, N2, N3, N4, N5, N6, N7, N8
        std::vector<int> s8 = boost::assign::list_of(0)(1)(2)(3)(4)(5)(6)(7);

        elemOrderMap.insert(std::make_pair("S3", s3));
        faceTypeMap.insert(std::make_pair(elemOrderMap["S3"].size(), "S3"));
        elemOrderMap.insert(std::make_pair("S6", s6));
        faceTypeMap.insert(std::make_pair(elemOrderMap["S6"].size(), "S6"));
        elemOrderMap.insert(std::make_pair("S4", s4));
        faceTypeMap.insert(std::make_pair(elemOrderMap["S4"].size(), "S4"));
        elemOrderMap.insert(std::make_pair("S8", s8));
        faceTypeMap.insert(std::make_pair(elemOrderMap["S8"].size(), "S8"));

        // dimension 3
        //
        // tetras
        // master 0.14 release
        // changed to this in August 2013, commited by juergen (jriedel)
        // https://github.com/FreeCAD/FreeCAD/commit/af56b324b9566b20f3b6e7880c29354c1dbe7a99
        //std::vector<int> c3d4  = boost::assign::list_of(0)(3)(1)(2);
        //std::vector<int> c3d10 = boost::assign::list_of(0)(2)(1)(3)(6)(5)(4)(7)(9)(8);

        // since master 0.15
        // added by werner (wmayer) March 2015, http://forum.freecadweb.org/viewtopic.php?f=18&t=10110&start=10#p81681
        // https://github.com/FreeCAD/FreeCAD/commit/5d159f5cf352a93b1aff4fb7b82e8b747ee4f35b
        // https://github.com/FreeCAD/FreeCAD/commit/b007bd19e4e4608caa4cdad350a9f480287fac6b
        // tetra4 FreeCAD --> C3D4 CalculiX
        // N2, N1, N3, N4
        std::vector<int> c3d4  = boost::assign::list_of(1)(0)(2)(3);
        // tetra10: FreeCAD --> C3D10 CalculiX
        // N2, N1, N3, N4, N5, N7, N6, N9, N8, N10
        std::vector<int> c3d10 = boost::assign::list_of(1)(0)(2)(3)(4)(6)(5)(8)(7)(9);

        // tetra node order for the system which is used for hexa8, hexa20, penta6 and penda15
        // be careful with activating because of method getccxVolumesByFace())
        // tetra4 FreeCAD --> C3D4 CalculiX
        // N2, N3, N4, N1
        //std::vector<int> c3d4  = boost::assign::list_of(1)(2)(3)(0);
        //
        // tetra10: FreeCAD --> C3D10 CalculiX
        // N2, N3, N4, N1, N6, N10, N9, N5, N7, N8
        //std::vector<int> c3d10 = boost::assign::list_of(1)(2)(3)(0)(5)(9)(8)(4)(6)(7);

        // hexa8 FreeCAD --> C3D8 CalculiX
        // N6, N7, N8, N5, N2, N3, N4, N1
        std::vector<int> c3d8 = boost::assign::list_of(5)(6)(7)(4)(1)(2)(3)(0) ;
        //
        // hexa20 FreeCAD --> C3D20 CalculiX
        // N6, N7, N8, N5, N2, N3, N4, N1, N14, N15, N16, N13, N10, N11, N12, N9, N18, N19, N20, N17
        std::vector<int> c3d20 = boost::assign::list_of(5)(6)(7)(4)(1)(2)(3)(0)(13)(14)(15)(12)(9)(10)(11)(8)(17)(18)(19)(16) ;
        //
        // penta6 FreeCAD --> C3D6 CalculiX
        // N5, N6, N4, N2, N3, N1
        std::vector<int> c3d6 = boost::assign::list_of(4)(5)(3)(1)(2)(0) ;
        //
        // penta15 FreeCAD --> C3D15 CalculiX
        // N5, N6, N4, N2, N3, N1, N11, N12, N10, N8, N9, N7, N14, N15, N13
        std::vector<int> c3d15 = boost::assign::list_of(4)(5)(3)(1)(2)(0)(10)(11)(9)(7)(8)(6)(13)(14)(12) ;

        elemOrderMap.insert(std::make_pair("C3D4", c3d4));
        volTypeMap.insert(std::make_pair(elemOrderMap["C3D4"].size(), "C3D4"));
        elemOrderMap.insert(std::make_pair("C3D10", c3d10));
        volTypeMap.insert(std::make_pair(elemOrderMap["C3D10"].size(), "C3D10"));
        elemOrderMap.insert(std::make_pair("C3D8", c3d8));
        volTypeMap.insert(std::make_pair(elemOrderMap["C3D8"].size(), "C3D8"));
        elemOrderMap.insert(std::make_pair("C3D20", c3d20));
        volTypeMap.insert(std::make_pair(elemOrderMap["C3D20"].size(), "C3D20"));
        elemOrderMap.insert(std::make_pair("C3D6", c3d6));
        volTypeMap.insert(std::make_pair(elemOrderMap["C3D6"].size(), "C3D6"));
        elemOrderMap.insert(std::make_pair("C3D15", c3d15));
        volTypeMap.insert(std::make_pair(elemOrderMap["C3D15"].size(), "C3D15"));
    }

    std::ofstream anABAQUS_Output;
    anABAQUS_Output.open(Filename.c_str());

    // add nodes
    //
    anABAQUS_Output << "*Node, NSET=Nall" << std::endl;
    typedef std::map<int, Base::Vector3d> VertexMap;
    VertexMap vertexMap;

    //Extract Nodes and Elements of the current SMESH datastructure
    SMDS_NodeIteratorPtr aNodeIter = myMesh->GetMeshDS()->nodesIterator();

    Base::Vector3d current_node;
    while (aNodeIter->more()) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        current_node.Set(aNode->X(),aNode->Y(),aNode->Z());
        current_node = _Mtrx * current_node;
        vertexMap[aNode->GetID()] = current_node;
    }

    // This way we get sorted output.
    // See http://forum.freecadweb.org/viewtopic.php?f=18&t=12646&start=40#p103004
    for (VertexMap::iterator it = vertexMap.begin(); it != vertexMap.end(); ++it) {
        anABAQUS_Output << it->first << ", "
            << it->second.x << ", "
            << it->second.y << ", "
            << it->second.z << std::endl;
    }

    typedef std::map<int, std::vector<int> > NodesMap;
    typedef std::map<std::string, NodesMap> ElementsMap;
    ElementsMap elementsMap;

    // add volumes
    //
    SMDS_VolumeIteratorPtr aVolIter = myMesh->GetMeshDS()->volumesIterator();
    while (aVolIter->more()) {
        const SMDS_MeshVolume* aVol = aVolIter->next();
        std::pair<int, std::vector<int> > apair;
        apair.first = aVol->GetID();

        int numNodes = aVol->NbNodes();
        std::map<int, std::string>::iterator it = volTypeMap.find(numNodes);
        if (it != volTypeMap.end()) {
            const std::vector<int>& order = elemOrderMap[it->second];
            for (std::vector<int>::const_iterator jt = order.begin(); jt != order.end(); ++jt)
                apair.second.push_back(aVol->GetNode(*jt)->GetID());
            elementsMap[it->second].insert(apair);
        }
    }

    for (ElementsMap::iterator it = elementsMap.begin(); it != elementsMap.end(); ++it) {
        anABAQUS_Output << "*Element, TYPE=" << it->first << ", ELSET=Eall" << std::endl;
        for (NodesMap::iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
            anABAQUS_Output << jt->first;
            // Calculix allows max 16 enntries in one line, an hexa20 has more !
            int ct = 0;  // counter
            bool first_line = true;
            for (std::vector<int>::iterator kt = jt->second.begin(); kt != jt->second.end(); ++kt, ++ct) {
                if (ct < 15) {
                    anABAQUS_Output  << ", " << *kt;
                }
                else {
                    if (first_line == true) {
                        anABAQUS_Output << "," << std::endl;
                        first_line = false;
                    }
                    anABAQUS_Output << *kt << ", ";
                }
            }
            anABAQUS_Output << std::endl;
        }
    }

    if (!elementsMap.empty()) {
        anABAQUS_Output.close();
        return; // done
    }

    // add faces
    //
    elementsMap.clear();
    SMDS_FaceIteratorPtr aFaceIter = myMesh->GetMeshDS()->facesIterator();
    while (aFaceIter->more()) {
        const SMDS_MeshFace* aFace = aFaceIter->next();
        std::pair<int, std::vector<int> > apair;
        apair.first = aFace->GetID();

        int numNodes = aFace->NbNodes();
        std::map<int, std::string>::iterator it = faceTypeMap.find(numNodes);
        if (it != faceTypeMap.end()) {
            const std::vector<int>& order = elemOrderMap[it->second];
            for (std::vector<int>::const_iterator jt = order.begin(); jt != order.end(); ++jt)
                apair.second.push_back(aFace->GetNode(*jt)->GetID());
            elementsMap[it->second].insert(apair);
        }
    }

    for (ElementsMap::iterator it = elementsMap.begin(); it != elementsMap.end(); ++it) {
        anABAQUS_Output << "*Element, TYPE=" << it->first << ", ELSET=Eall" << std::endl;
        for (NodesMap::iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
            anABAQUS_Output << jt->first;
            for (std::vector<int>::iterator kt = jt->second.begin(); kt != jt->second.end(); ++kt) {
                anABAQUS_Output << ", " << *kt;
            }
            anABAQUS_Output << std::endl;
        }
    }

    if (!elementsMap.empty()) {
        anABAQUS_Output.close();
        return; // done
    }

    // add edges
    //
    elementsMap.clear();
    SMDS_EdgeIteratorPtr aEdgeIter = myMesh->GetMeshDS()->edgesIterator();
    while (aEdgeIter->more()) {
        const SMDS_MeshEdge* aEdge = aEdgeIter->next();
        std::pair<int, std::vector<int> > apair;
        apair.first = aEdge->GetID();

        int numNodes = aEdge->NbNodes();
        std::map<int, std::string>::iterator it = edgeTypeMap.find(numNodes);
        if (it != edgeTypeMap.end()) {
            const std::vector<int>& order = elemOrderMap[it->second];
            for (std::vector<int>::const_iterator jt = order.begin(); jt != order.end(); ++jt)
                apair.second.push_back(aEdge->GetNode(*jt)->GetID());
            elementsMap[it->second].insert(apair);
        }
    }

    for (ElementsMap::iterator it = elementsMap.begin(); it != elementsMap.end(); ++it) {
        anABAQUS_Output << "*Element, TYPE=" << it->first << ", ELSET=Eall" << std::endl;
        for (NodesMap::iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
            anABAQUS_Output << jt->first;
            for (std::vector<int>::iterator kt = jt->second.begin(); kt != jt->second.end(); ++kt) {
                anABAQUS_Output << ", " << *kt;
            }
            anABAQUS_Output << std::endl;
        }
    }
    elementsMap.clear();

    anABAQUS_Output.close();
}

void FemMesh::write(const char *FileName) const
{
    Base::FileInfo File(FileName);

    if (File.hasExtension("unv") ) {
        // read UNV file
         myMesh->ExportUNV(File.filePath().c_str());
    }
    else if (File.hasExtension("med") ) {
         myMesh->ExportMED(File.filePath().c_str());
    }
    else if (File.hasExtension("stl") ) {
        // read brep-file
        myMesh->ExportSTL(File.filePath().c_str(),false);
    }
    else if (File.hasExtension("dat") ) {
        // read brep-file
        myMesh->ExportDAT(File.filePath().c_str());
    }
    else if (File.hasExtension("inp") ) {
        // write ABAQUS Output
        writeABAQUS(File.filePath());
    }
    else{
        throw Base::Exception("Unknown extension");
    }
}

// ==== Base class implementer ==============================================================

unsigned int FemMesh::getMemSize (void) const
{
    return 0;
}

void FemMesh::Save (Base::Writer &writer) const
{
    if (!writer.isForceXML()) {
        //See SaveDocFile(), RestoreDocFile()
        writer.Stream() << writer.ind() << "<FemMesh file=\"" ;
        writer.Stream() << writer.addFile("FemMesh.unv", this) << "\"";
        writer.Stream() << " a11=\"" <<  _Mtrx[0][0] << "\" a12=\"" <<  _Mtrx[0][1] << "\" a13=\"" <<  _Mtrx[0][2] << "\" a14=\"" <<  _Mtrx[0][3] << "\"";
        writer.Stream() << " a21=\"" <<  _Mtrx[1][0] << "\" a22=\"" <<  _Mtrx[1][1] << "\" a23=\"" <<  _Mtrx[1][2] << "\" a24=\"" <<  _Mtrx[1][3] << "\"";
        writer.Stream() << " a31=\"" <<  _Mtrx[2][0] << "\" a32=\"" <<  _Mtrx[2][1] << "\" a33=\"" <<  _Mtrx[2][2] << "\" a34=\"" <<  _Mtrx[2][3] << "\"";
        writer.Stream() << " a41=\"" <<  _Mtrx[3][0] << "\" a42=\"" <<  _Mtrx[3][1] << "\" a43=\"" <<  _Mtrx[3][2] << "\" a44=\"" <<  _Mtrx[3][3] << "\"";
        writer.Stream() << "/>" << std::endl;
    }
    else {
        writer.Stream() << writer.ind() << "<FemMesh file=\"\"" ;
        writer.Stream() << " a11=\"" <<  _Mtrx[0][0] << "\" a12=\"" <<  _Mtrx[0][1] << "\" a13=\"" <<  _Mtrx[0][2] << "\" a14=\"" <<  _Mtrx[0][3] << "\"";
        writer.Stream() << " a21=\"" <<  _Mtrx[1][0] << "\" a22=\"" <<  _Mtrx[1][1] << "\" a23=\"" <<  _Mtrx[1][2] << "\" a24=\"" <<  _Mtrx[1][3] << "\"";
        writer.Stream() << " a31=\"" <<  _Mtrx[2][0] << "\" a32=\"" <<  _Mtrx[2][1] << "\" a33=\"" <<  _Mtrx[2][2] << "\" a34=\"" <<  _Mtrx[2][3] << "\"";
        writer.Stream() << " a41=\"" <<  _Mtrx[3][0] << "\" a42=\"" <<  _Mtrx[3][1] << "\" a43=\"" <<  _Mtrx[3][2] << "\" a44=\"" <<  _Mtrx[3][3] << "\"";
        writer.Stream() << "/>" << std::endl;
    }
}

void FemMesh::Restore(Base::XMLReader &reader)
{
    reader.readElement("FemMesh");
    std::string file (reader.getAttribute("file") );

    if (!file.empty()) {
        // initate a file read
        reader.addFile(file.c_str(),this);
    }
    if( reader.hasAttribute("a11")){
        _Mtrx[0][0] = (float)reader.getAttributeAsFloat("a11");
        _Mtrx[0][1] = (float)reader.getAttributeAsFloat("a12");
        _Mtrx[0][2] = (float)reader.getAttributeAsFloat("a13");
        _Mtrx[0][3] = (float)reader.getAttributeAsFloat("a14");

        _Mtrx[1][0] = (float)reader.getAttributeAsFloat("a21");
        _Mtrx[1][1] = (float)reader.getAttributeAsFloat("a22");
        _Mtrx[1][2] = (float)reader.getAttributeAsFloat("a23");
        _Mtrx[1][3] = (float)reader.getAttributeAsFloat("a24");

        _Mtrx[2][0] = (float)reader.getAttributeAsFloat("a31");
        _Mtrx[2][1] = (float)reader.getAttributeAsFloat("a32");
        _Mtrx[2][2] = (float)reader.getAttributeAsFloat("a33");
        _Mtrx[2][3] = (float)reader.getAttributeAsFloat("a34");

        _Mtrx[3][0] = (float)reader.getAttributeAsFloat("a41");
        _Mtrx[3][1] = (float)reader.getAttributeAsFloat("a42");
        _Mtrx[3][2] = (float)reader.getAttributeAsFloat("a43");
        _Mtrx[3][3] = (float)reader.getAttributeAsFloat("a44");
    }
}

void FemMesh::SaveDocFile (Base::Writer &writer) const
{
    // create a temporary file and copy the content to the zip stream
    Base::FileInfo fi(App::Application::getTempFileName().c_str());

    myMesh->ExportUNV(fi.filePath().c_str());

    Base::ifstream file(fi, std::ios::in | std::ios::binary);
    if (file){
        unsigned long ulSize = 0;
        std::streambuf* buf = file.rdbuf();
        if (buf) {
            unsigned long ulCurr;
            ulCurr = buf->pubseekoff(0, std::ios::cur, std::ios::in);
            ulSize = buf->pubseekoff(0, std::ios::end, std::ios::in);
            buf->pubseekoff(ulCurr, std::ios::beg, std::ios::in);
        }

        // read in the ASCII file and write back to the stream
        std::strstreambuf sbuf(ulSize);
        file >> &sbuf;
        writer.Stream() << &sbuf;
    }

    file.close();
    // remove temp file
    fi.deleteFile();
}

void FemMesh::RestoreDocFile(Base::Reader &reader)
{
    // create a temporary file and copy the content from the zip stream
    Base::FileInfo fi(App::Application::getTempFileName().c_str());

    // read in the ASCII file and write back to the file stream
    Base::ofstream file(fi, std::ios::out | std::ios::binary);
    if (reader)
        reader >> file.rdbuf();
    file.close();

    // read the shape from the temp file
    myMesh->UNVToMesh(fi.filePath().c_str());

    // delete the temp file
    fi.deleteFile();
}

void FemMesh::transformGeometry(const Base::Matrix4D& rclTrf)
{
    //We perform a translation and rotation of the current active Mesh object
    Base::Matrix4D clMatrix(rclTrf);
    SMDS_NodeIteratorPtr aNodeIter = myMesh->GetMeshDS()->nodesIterator();
    Base::Vector3d current_node;
    for (;aNodeIter->more();) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        current_node.Set(aNode->X(),aNode->Y(),aNode->Z());
        current_node = clMatrix * current_node;
        myMesh->GetMeshDS()->MoveNode(aNode,current_node.x,current_node.y,current_node.z);
    }
}

void FemMesh::setTransform(const Base::Matrix4D& rclTrf)
{
    // Placement handling, no geometric transformation
    _Mtrx = rclTrf;
}

Base::Matrix4D FemMesh::getTransform(void) const
{
    return _Mtrx;
}

Base::BoundBox3d FemMesh::getBoundBox(void) const
{
    Base::BoundBox3d box;

    SMESHDS_Mesh* data = const_cast<SMESH_Mesh*>(getSMesh())->GetMeshDS();

    SMDS_NodeIteratorPtr aNodeIter = data->nodesIterator();
    for (;aNodeIter->more();) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        Base::Vector3d vec(aNode->X(),aNode->Y(),aNode->Z());
        // Apply the matrix to hold the BoundBox in absolute space.
        vec = _Mtrx * vec;
        box.Add(vec);
    }

    return box;
}

std::vector<const char*> FemMesh::getElementTypes(void) const
{
    std::vector<const char*> temp;
    temp.push_back("Vertex");
    temp.push_back("Edge");
    temp.push_back("Face");
    temp.push_back("Volume");

    return temp;
}

unsigned long FemMesh::countSubElements(const char* Type) const
{
    return 0;
}

Data::Segment* FemMesh::getSubElement(const char* Type, unsigned long n) const
{
    // FIXME implement subelement interface
    //std::stringstream str;
    //str << Type << n;
    //std::string temp = str.str();
    //return new ShapeSegment(getSubShape(temp.c_str()));
    return 0;
}

struct Fem::FemMesh::FemMeshInfo FemMesh::getInfo(void) const{

    struct FemMeshInfo rtrn;

    SMESHDS_Mesh* data =  const_cast<SMESH_Mesh*>(getSMesh())->GetMeshDS();
    const SMDS_MeshInfo& info = data->GetMeshInfo();
    rtrn.numFaces = data->NbFaces();
    rtrn.numNode = info.NbNodes();
    rtrn.numTria = info.NbTriangles();
    rtrn.numQuad = info.NbQuadrangles();
    rtrn.numPoly = info.NbPolygons();
    rtrn.numVolu = info.NbVolumes();
    rtrn.numTetr = info.NbTetras();
    rtrn.numHexa = info.NbHexas();
    rtrn.numPyrd = info.NbPyramids();
    rtrn.numPris = info.NbPrisms();
    rtrn.numHedr = info.NbPolyhedrons();

    return rtrn;

}
//    for(unsigned int i=0;i<all_elements.size();i++)
//        {
//            //Die Reihenfolge wie hier die Elemente hinzugefügt werden ist sehr wichtig.
//            //Ansonsten ist eine konsistente Datenstruktur nicht möglich
//                meshds->AddVolumeWithID(
//                meshds->FindNode(all_elements[i][0]),
//                meshds->FindNode(all_elements[i][2]),
//                meshds->FindNode(all_elements[i][1]),
//                meshds->FindNode(all_elements[i][3]),
//                meshds->FindNode(all_elements[i][6]),
//                meshds->FindNode(all_elements[i][5]),
//                meshds->FindNode(all_elements[i][4]),
//                meshds->FindNode(all_elements[i][9]),
//                meshds->FindNode(all_elements[i][7]),
//                meshds->FindNode(all_elements[i][8]),
//                element_id[i]
//            );
//        }

Base::Quantity FemMesh::getVolume(void)const
{
    SMDS_VolumeIteratorPtr aVolIter = myMesh->GetMeshDS()->volumesIterator();

    //Calculate Mesh Volume
    //For an accurate Volume Calculation of a quadratic Tetrahedron
    //we have to calculate the Volume of 8 Sub-Tetrahedrons
    Base::Vector3d a,b,c,a_b_product;
    double volume = 0.0;

    for (;aVolIter->more();)
    {
        const SMDS_MeshVolume* aVol = aVolIter->next();

        if ( aVol->NbNodes() != 10 ) continue;

        Base::Vector3d v1(aVol->GetNode(1)->X(),aVol->GetNode(1)->Y(),aVol->GetNode(1)->Z());
        Base::Vector3d v0(aVol->GetNode(0)->X(),aVol->GetNode(0)->Y(),aVol->GetNode(0)->Z());
        Base::Vector3d v2(aVol->GetNode(2)->X(),aVol->GetNode(2)->Y(),aVol->GetNode(2)->Z());
        Base::Vector3d v3(aVol->GetNode(3)->X(),aVol->GetNode(3)->Y(),aVol->GetNode(3)->Z());
        Base::Vector3d v4(aVol->GetNode(4)->X(),aVol->GetNode(4)->Y(),aVol->GetNode(4)->Z());
        Base::Vector3d v6(aVol->GetNode(6)->X(),aVol->GetNode(6)->Y(),aVol->GetNode(6)->Z());
        Base::Vector3d v5(aVol->GetNode(5)->X(),aVol->GetNode(5)->Y(),aVol->GetNode(5)->Z());
        Base::Vector3d v8(aVol->GetNode(8)->X(),aVol->GetNode(8)->Y(),aVol->GetNode(8)->Z());
        Base::Vector3d v7(aVol->GetNode(7)->X(),aVol->GetNode(7)->Y(),aVol->GetNode(7)->Z());
        Base::Vector3d v9(aVol->GetNode(9)->X(),aVol->GetNode(9)->Y(),aVol->GetNode(9)->Z());


        //1,5,8,7
        a = v4 -v0 ;
        b = v7 -v0 ;
        c = v6 -v0 ;
        a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
        volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
        //5,9,8,7
        a = v8 -v4 ;
        b = v7 -v4 ;
        c = v6 -v4 ;
        a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
        volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
        //5,2,9,7
        a = v1 -v4 ;
        b = v8 -v4 ;
        c = v6 -v4 ;
        a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
        volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
        //2,6,9,7
        a = v5 -v1 ;
        b = v8 -v1 ;
        c = v6 -v1 ;
        a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
        volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
        //9,6,10,7
        a = v5 -v8 ;
        b = v9 -v8 ;
        c = v6 -v8 ;
        a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
        volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
        //6,3,10,7
        a = v2 -v5 ;
        b = v9 -v5 ;
        c = v6 -v5 ;
        a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
        volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
        //8,9,10,7
        a = v8 -v7 ;
        b = v9 -v7 ;
        c = v6 -v7 ;
        a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
        volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));
        //8,9,10,4
        a = v8 -v7 ;
        b = v9 -v7 ;
        c = v3 -v7 ;
        a_b_product.x = a.y*b.z-b.y*a.z;a_b_product.y = a.z*b.x-b.z*a.x;a_b_product.z = a.x*b.y-b.x*a.y;
        volume += 1.0/6.0 * fabs((a_b_product.x * c.x)+ (a_b_product.y * c.y)+(a_b_product.z * c.z));

    }

    return Base::Quantity(volume,Unit::Volume);


}

