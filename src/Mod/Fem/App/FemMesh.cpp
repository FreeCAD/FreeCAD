/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <Python.h>
#include <cstdlib>
#include <memory>

#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <SMDS_MeshGroup.hxx>
#include <SMESHDS_Group.hxx>
#include <SMESHDS_GroupBase.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMESH_Gen.hxx>
#include <SMESH_Group.hxx>
#include <SMESH_Mesh.hxx>
#include <SMESH_MeshEditor.hxx>
#include <ShapeAnalysis_ShapeTolerance.hxx>
#include <StdMeshers_Deflection1D.hxx>
#include <StdMeshers_LocalLength.hxx>
#include <StdMeshers_MaxElementArea.hxx>
#include <StdMeshers_MaxLength.hxx>
#include <StdMeshers_NumberOfSegments.hxx>
#include <StdMeshers_QuadranglePreference.hxx>
#include <StdMeshers_Quadrangle_2D.hxx>
#include <StdMeshers_Regular_1D.hxx>
#include <StdMeshers_StartEndLength.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>

#include <boost/assign/list_of.hpp>
#include <boost/tokenizer.hpp>  //to simplify parsing input files we use the boost lib
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/TimeInfo.h>
#include <Base/Writer.h>
#include <Mod/Mesh/App/Core/Iterator.h>

#include "FemMesh.h"
#include <FemMeshPy.h>

#ifdef FC_USE_VTK
#include "FemVTKTools.h"
#endif


using namespace Fem;
using namespace Base;
using namespace boost;

#if SMESH_VERSION_MAJOR < 9
static int StatCount = 0;
#endif

SMESH_Gen* FemMesh::_mesh_gen = nullptr;

TYPESYSTEM_SOURCE(Fem::FemMesh, Base::Persistence)

FemMesh::FemMesh()
{
    // Base::Console().Log("FemMesh::FemMesh():%p (id=%i)\n",this,StatCount);
    //  create a mesh always with new StudyId to avoid overlapping destruction
#if SMESH_VERSION_MAJOR >= 9
    myMesh = getGenerator()->CreateMesh(false);
#else
    myMesh = getGenerator()->CreateMesh(StatCount++, false);
#endif
}

FemMesh::FemMesh(const FemMesh& mesh)
{
#if SMESH_VERSION_MAJOR >= 9
    myMesh = getGenerator()->CreateMesh(false);
#else
    myMesh = getGenerator()->CreateMesh(StatCount++, false);
#endif
    copyMeshData(mesh);
}

FemMesh::~FemMesh()
{
    // Base::Console().Log("FemMesh::~FemMesh():%p\n",this);

    try {
        TopoDS_Shape aNull;
        myMesh->ShapeToMesh(aNull);
        myMesh->Clear();
        // myMesh->ClearLog();
        delete myMesh;
    }
    catch (...) {
    }
}

FemMesh& FemMesh::operator=(const FemMesh& mesh)
{
    if (this != &mesh) {
#if SMESH_VERSION_MAJOR >= 9
        myMesh = getGenerator()->CreateMesh(true);
#else
        myMesh = getGenerator()->CreateMesh(0, true);
#endif
        copyMeshData(mesh);
    }
    return *this;
}

void FemMesh::copyMeshData(const FemMesh& mesh)
{
    _Mtrx = mesh._Mtrx;

    // See file SMESH_I/SMESH_Gen_i.cxx in the git repo of smesh at
    // https://git.salome-platform.org
#if 1
    // 1. Get source mesh
    SMESHDS_Mesh* srcMeshDS = mesh.myMesh->GetMeshDS();

    // 2. Get target mesh
    SMESHDS_Mesh* newMeshDS = this->myMesh->GetMeshDS();
    SMESH_MeshEditor editor(this->myMesh);

    // 3. Get elements to copy
    SMDS_ElemIteratorPtr srcElemIt;
    SMDS_NodeIteratorPtr srcNodeIt;
    srcElemIt = srcMeshDS->elementsIterator();
    srcNodeIt = srcMeshDS->nodesIterator();

    // 4. Copy elements
    int iN;
    const SMDS_MeshNode *nSrc, *nTgt;
    std::vector<const SMDS_MeshNode*> nodes;
    while (srcElemIt->more()) {
        const SMDS_MeshElement* elem = srcElemIt->next();
        // find / add nodes
        nodes.resize(elem->NbNodes());
        SMDS_ElemIteratorPtr nIt = elem->nodesIterator();
        for (iN = 0; nIt->more(); ++iN) {
            nSrc = static_cast<const SMDS_MeshNode*>(nIt->next());
            nTgt = newMeshDS->FindNode(nSrc->GetID());
            if (!nTgt) {
                nTgt = newMeshDS->AddNodeWithID(nSrc->X(), nSrc->Y(), nSrc->Z(), nSrc->GetID());
            }
            nodes[iN] = nTgt;
        }

        // add elements
        if (elem->GetType() != SMDSAbs_Node) {
            int ID = elem->GetID();
            switch (elem->GetEntityType()) {
                case SMDSEntity_Polyhedra:
#if SMESH_VERSION_MAJOR >= 9
                    editor.GetMeshDS()->AddPolyhedralVolumeWithID(
                        nodes,
                        static_cast<const SMDS_MeshVolume*>(elem)->GetQuantities(),
                        ID);
#else
                    editor.GetMeshDS()->AddPolyhedralVolumeWithID(
                        nodes,
                        static_cast<const SMDS_VtkVolume*>(elem)->GetQuantities(),
                        ID);
#endif
                    break;
                case SMDSEntity_Ball: {
                    SMESH_MeshEditor::ElemFeatures elemFeat;
                    elemFeat.Init(static_cast<const SMDS_BallElement*>(elem)->GetDiameter());
                    elemFeat.SetID(ID);
                    editor.AddElement(nodes, elemFeat);
                    break;
                }
                default: {
                    SMESH_MeshEditor::ElemFeatures elemFeat(elem->GetType(), elem->IsPoly());
                    elemFeat.SetID(ID);
                    editor.AddElement(nodes, elemFeat);
                    break;
                }
            }
        }
    }

    // 4(b). Copy free nodes
    if (srcNodeIt && srcMeshDS->NbNodes() != newMeshDS->NbNodes()) {
        while (srcNodeIt->more()) {
            nSrc = srcNodeIt->next();
            if (nSrc->NbInverseElements() == 0) {
                nTgt = newMeshDS->AddNodeWithID(nSrc->X(), nSrc->Y(), nSrc->Z(), nSrc->GetID());
            }
        }
    }

    // 5. Copy groups
    SMESH_Mesh::GroupIteratorPtr gIt = mesh.myMesh->GetGroups();
    while (gIt->more()) {
        SMESH_Group* group = gIt->next();
        const SMESHDS_GroupBase* groupDS = group->GetGroupDS();

        // Check group type. We copy nodal groups containing nodes of copied element
        SMDSAbs_ElementType groupType = groupDS->GetType();
        if (groupType != SMDSAbs_Node && newMeshDS->GetMeshInfo().NbElements(groupType) == 0) {
            continue;  // group type differs from types of meshPart
        }

        // Find copied elements in the group
        std::vector<const SMDS_MeshElement*> groupElems;
        SMDS_ElemIteratorPtr eIt = groupDS->GetElements();
        const SMDS_MeshElement* foundElem;
        if (groupType == SMDSAbs_Node) {
            while (eIt->more()) {
                if ((foundElem = newMeshDS->FindNode(eIt->next()->GetID()))) {
                    groupElems.push_back(foundElem);
                }
            }
        }
        else {
            while (eIt->more()) {
                if ((foundElem = newMeshDS->FindElement(eIt->next()->GetID()))) {
                    groupElems.push_back(foundElem);
                }
            }
        }

        // Make a new group
        if (!groupElems.empty()) {
            int aId = -1;
            SMESH_Group* newGroupObj = this->myMesh->AddGroup(groupType, group->GetName(), aId);
            SMESHDS_Group* newGroupDS = dynamic_cast<SMESHDS_Group*>(newGroupObj->GetGroupDS());
            if (newGroupDS) {
                SMDS_MeshGroup& smdsGroup = ((SMESHDS_Group*)newGroupDS)->SMDSGroup();
                for (auto it : groupElems) {
                    smdsGroup.Add(it);
                }
            }
        }
    }

    newMeshDS->Modified();

#else
    SMESHDS_Mesh* meshds = this->myMesh->GetMeshDS();

    // Some further information is still not copied:
    // https://forum.freecad.org/viewtopic.php?f=18&t=18982#p148114
    SMDS_NodeIteratorPtr aNodeIter = mesh.myMesh->GetMeshDS()->nodesIterator();
    for (; aNodeIter->more();) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        double temp[3];
        aNode->GetXYZ(temp);
        meshds->AddNodeWithID(temp[0], temp[1], temp[2], aNode->GetID());
    }
    SMDS_EdgeIteratorPtr aEdgeIter = mesh.myMesh->GetMeshDS()->edgesIterator();
    for (; aEdgeIter->more();) {
        const SMDS_MeshEdge* aEdge = aEdgeIter->next();
        meshds->AddEdgeWithID(aEdge->GetNode(0), aEdge->GetNode(1), aEdge->GetID());
    }

    SMDS_FaceIteratorPtr aFaceIter = mesh.myMesh->GetMeshDS()->facesIterator();
    for (; aFaceIter->more();) {
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
            default: {
                std::vector<const SMDS_MeshNode*> aNodes;
                for (int i = 0; aFace->NbNodes(); i++) {
                    aNodes.push_back(aFace->GetNode(0));
                }
                meshds->AddPolygonalFaceWithID(aNodes, aFace->GetID());
            } break;
        }
    }

    SMDS_VolumeIteratorPtr aVolIter = mesh.myMesh->GetMeshDS()->volumesIterator();
    for (; aVolIter->more();) {
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
            default: {
                if (aVol->IsPoly()) {
                    const SMDS_PolyhedralVolumeOfNodes* aPolyVol =
                        dynamic_cast<const SMDS_PolyhedralVolumeOfNodes*>(aVol);
                    if (!aPolyVol) {
                        break;
                    }
                    std::vector<const SMDS_MeshNode*> aNodes;
                    for (int i = 0; i < aPolyVol->NbNodes(); i++) {
                        aNodes.push_back(aPolyVol->GetNode(i));
                    }
                    meshds->AddPolyhedralVolumeWithID(aNodes,
                                                      aPolyVol->GetQuanities(),
                                                      aPolyVol->GetID());
                }
            } break;
        }
    }

    // Copy groups
    std::list<int> grpIds = mesh.myMesh->GetGroupIds();
    for (auto it : grpIds) {
        // group of source mesh
        SMESH_Group* sourceGroup = mesh.myMesh->GetGroup(it);
        SMESHDS_GroupBase* sourceGroupDS = sourceGroup->GetGroupDS();

        int aId;
        if (sourceGroupDS->GetType() == SMDSAbs_Node) {
            SMESH_Group* targetGroup =
                this->myMesh->AddGroup(SMDSAbs_Node, sourceGroupDS->GetStoreName(), aId);
            if (targetGroup) {
                SMESHDS_Group* targetGroupDS =
                    dynamic_cast<SMESHDS_Group*>(targetGroup->GetGroupDS());
                if (targetGroupDS) {
                    SMDS_ElemIteratorPtr aIter = sourceGroupDS->GetElements();
                    while (aIter->more()) {
                        const SMDS_MeshElement* aElem = aIter->next();
                        const SMDS_MeshNode* aNode = meshds->FindNode(aElem->GetID());
                        if (aNode) {
                            targetGroupDS->SMDSGroup().Add(aNode);
                        }
                    }
                }
            }
        }
        else {
            SMESH_Group* targetGroup = this->myMesh->AddGroup(sourceGroupDS->GetType(),
                                                              sourceGroupDS->GetStoreName(),
                                                              aId);
            if (targetGroup) {
                SMESHDS_Group* targetGroupDS =
                    dynamic_cast<SMESHDS_Group*>(targetGroup->GetGroupDS());
                if (targetGroupDS) {
                    SMDS_ElemIteratorPtr aIter = sourceGroupDS->GetElements();
                    while (aIter->more()) {
                        const SMDS_MeshElement* aElem = aIter->next();
                        const SMDS_MeshElement* aElement = meshds->FindElement(aElem->GetID());
                        if (aElement) {
                            targetGroupDS->SMDSGroup().Add(aElement);
                        }
                    }
                }
            }
        }
    }
#endif
}

const SMESH_Mesh* FemMesh::getSMesh() const
{
    return myMesh;
}

SMESH_Mesh* FemMesh::getSMesh()
{
    return myMesh;
}

SMESH_Gen* FemMesh::getGenerator()
{
    if (!FemMesh::_mesh_gen) {
        FemMesh::_mesh_gen = new SMESH_Gen();
    }
    return FemMesh::_mesh_gen;
}

void FemMesh::addHypothesis(const TopoDS_Shape& aSubShape, SMESH_HypothesisPtr hyp)
{
    myMesh->AddHypothesis(aSubShape, hyp->GetID());
    SMESH_HypothesisPtr ptr(hyp);
    hypoth.push_back(ptr);
}

void FemMesh::setStandardHypotheses()
{
    if (!hypoth.empty()) {
        return;
    }
#if SMESH_VERSION_MAJOR >= 9
    int hyp = 0;
    SMESH_HypothesisPtr len(new StdMeshers_MaxLength(hyp++, getGenerator()));
    static_cast<StdMeshers_MaxLength*>(len.get())->SetLength(1.0);
    hypoth.push_back(len);

    SMESH_HypothesisPtr loc(new StdMeshers_LocalLength(hyp++, getGenerator()));
    static_cast<StdMeshers_LocalLength*>(loc.get())->SetLength(1.0);
    hypoth.push_back(loc);

    SMESH_HypothesisPtr area(new StdMeshers_MaxElementArea(hyp++, getGenerator()));
    static_cast<StdMeshers_MaxElementArea*>(area.get())->SetMaxArea(1.0);
    hypoth.push_back(area);

    SMESH_HypothesisPtr segm(new StdMeshers_NumberOfSegments(hyp++, getGenerator()));
    static_cast<StdMeshers_NumberOfSegments*>(segm.get())->SetNumberOfSegments(1);
    hypoth.push_back(segm);

    SMESH_HypothesisPtr defl(new StdMeshers_Deflection1D(hyp++, getGenerator()));
    static_cast<StdMeshers_Deflection1D*>(defl.get())->SetDeflection(0.01);
    hypoth.push_back(defl);

    SMESH_HypothesisPtr reg(new StdMeshers_Regular_1D(hyp++, getGenerator()));
    hypoth.push_back(reg);

    SMESH_HypothesisPtr qdp(new StdMeshers_QuadranglePreference(hyp++, getGenerator()));
    hypoth.push_back(qdp);

    SMESH_HypothesisPtr q2d(new StdMeshers_Quadrangle_2D(hyp++, getGenerator()));
    hypoth.push_back(q2d);
#else
    int hyp = 0;
    SMESH_HypothesisPtr len(new StdMeshers_MaxLength(hyp++, 1, getGenerator()));
    static_cast<StdMeshers_MaxLength*>(len.get())->SetLength(1.0);
    hypoth.push_back(len);

    SMESH_HypothesisPtr loc(new StdMeshers_LocalLength(hyp++, 1, getGenerator()));
    static_cast<StdMeshers_LocalLength*>(loc.get())->SetLength(1.0);
    hypoth.push_back(loc);

    SMESH_HypothesisPtr area(new StdMeshers_MaxElementArea(hyp++, 1, getGenerator()));
    static_cast<StdMeshers_MaxElementArea*>(area.get())->SetMaxArea(1.0);
    hypoth.push_back(area);

    SMESH_HypothesisPtr segm(new StdMeshers_NumberOfSegments(hyp++, 1, getGenerator()));
    static_cast<StdMeshers_NumberOfSegments*>(segm.get())->SetNumberOfSegments(1);
    hypoth.push_back(segm);

    SMESH_HypothesisPtr defl(new StdMeshers_Deflection1D(hyp++, 1, getGenerator()));
    static_cast<StdMeshers_Deflection1D*>(defl.get())->SetDeflection(0.01);
    hypoth.push_back(defl);

    SMESH_HypothesisPtr reg(new StdMeshers_Regular_1D(hyp++, 1, getGenerator()));
    hypoth.push_back(reg);

    SMESH_HypothesisPtr qdp(new StdMeshers_QuadranglePreference(hyp++, 1, getGenerator()));
    hypoth.push_back(qdp);

    SMESH_HypothesisPtr q2d(new StdMeshers_Quadrangle_2D(hyp++, 1, getGenerator()));
    hypoth.push_back(q2d);
#endif

    // Apply hypothesis
    for (int i = 0; i < hyp; i++) {
        myMesh->AddHypothesis(myMesh->GetShapeToMesh(), i);
    }
}

void FemMesh::compute()
{
    getGenerator()->Compute(*myMesh, myMesh->GetShapeToMesh());
}

std::set<long> FemMesh::getSurfaceNodes(long /*ElemId*/, short /*FaceId*/, float /*Angle*/) const
{
    std::set<long> result;
    return result;
}

/*! That function returns map containing volume ID and face ID.
 */
std::list<std::pair<int, int>> FemMesh::getVolumesByFace(const TopoDS_Face& face) const
{
    std::list<std::pair<int, int>> result;
    std::set<int> nodes_on_face = getNodesByFace(face);

    // SMDS_MeshVolume::facesIterator() is broken with SMESH7 as it is impossible
    // to iterate volume faces
    // In SMESH9 this function has been removed
    //
    std::map<int, std::set<int>> face_nodes;

    // get faces that contribute to 'nodes_on_face' with all of its nodes
    SMDS_FaceIteratorPtr face_iter = myMesh->GetMeshDS()->facesIterator();
    while (face_iter && face_iter->more()) {
        const SMDS_MeshFace* face = face_iter->next();
        SMDS_NodeIteratorPtr node_iter = face->nodeIterator();

        // all nodes of the current face must be part of 'nodes_on_face'
        std::set<int> node_ids;
        while (node_iter && node_iter->more()) {
            const SMDS_MeshNode* node = node_iter->next();
            node_ids.insert(node->GetID());
        }

        std::vector<int> element_face_nodes;
        std::set_intersection(nodes_on_face.begin(),
                              nodes_on_face.end(),
                              node_ids.begin(),
                              node_ids.end(),
                              std::back_insert_iterator<std::vector<int>>(element_face_nodes));

        if (element_face_nodes.size() == node_ids.size()) {
            face_nodes[face->GetID()] = node_ids;
        }
    }

    // get all nodes of a volume and check which faces contribute to it with all of its nodes
    SMDS_VolumeIteratorPtr vol_iter = myMesh->GetMeshDS()->volumesIterator();
    while (vol_iter->more()) {
        const SMDS_MeshVolume* vol = vol_iter->next();
        SMDS_NodeIteratorPtr node_iter = vol->nodeIterator();
        std::set<int> node_ids;
        while (node_iter && node_iter->more()) {
            const SMDS_MeshNode* node = node_iter->next();
            node_ids.insert(node->GetID());
        }

        for (const auto& it : face_nodes) {
            std::vector<int> element_face_nodes;
            std::set_intersection(node_ids.begin(),
                                  node_ids.end(),
                                  it.second.begin(),
                                  it.second.end(),
                                  std::back_insert_iterator<std::vector<int>>(element_face_nodes));

            // For curved faces it is possible that a volume contributes more than one face
            if (element_face_nodes.size() == it.second.size()) {
                result.emplace_back(vol->GetID(), it.first);
            }
        }
    }
    result.sort();
    return result;
}

/*! That function returns a list of face IDs.
 */
std::list<int> FemMesh::getFacesByFace(const TopoDS_Face& face) const
{
    // TODO: This function is broken with SMESH7 as it is impossible to iterate volume faces
    std::list<int> result;
    std::set<int> nodes_on_face = getNodesByFace(face);

    SMDS_FaceIteratorPtr face_iter = myMesh->GetMeshDS()->facesIterator();
    while (face_iter->more()) {
        const SMDS_MeshFace* face = static_cast<const SMDS_MeshFace*>(face_iter->next());
        int numNodes = face->NbNodes();

        std::set<int> face_nodes;
        for (int i = 0; i < numNodes; i++) {
            face_nodes.insert(face->GetNode(i)->GetID());
        }

        std::vector<int> element_face_nodes;
        std::set_intersection(nodes_on_face.begin(),
                              nodes_on_face.end(),
                              face_nodes.begin(),
                              face_nodes.end(),
                              std::back_insert_iterator<std::vector<int>>(element_face_nodes));

        // For curved faces it is possible that a volume contributes more than one face
        if (element_face_nodes.size() == static_cast<std::size_t>(numNodes)) {
            result.push_back(face->GetID());
        }
    }

    result.sort();
    return result;
}

std::list<int> FemMesh::getEdgesByEdge(const TopoDS_Edge& edge) const
{
    std::list<int> result;
    std::set<int> nodes_on_edge = getNodesByEdge(edge);

    SMDS_EdgeIteratorPtr edge_iter = myMesh->GetMeshDS()->edgesIterator();
    while (edge_iter->more()) {
        const SMDS_MeshEdge* edge = static_cast<const SMDS_MeshEdge*>(edge_iter->next());
        int numNodes = edge->NbNodes();

        std::set<int> edge_nodes;
        for (int i = 0; i < numNodes; i++) {
            edge_nodes.insert(edge->GetNode(i)->GetID());
        }

        std::vector<int> element_edge_nodes;
        std::set_intersection(nodes_on_edge.begin(),
                              nodes_on_edge.end(),
                              edge_nodes.begin(),
                              edge_nodes.end(),
                              std::back_insert_iterator<std::vector<int>>(element_edge_nodes));

        if (element_edge_nodes.size() == static_cast<std::size_t>(numNodes)) {
            result.push_back(edge->GetID());
        }
    }

    result.sort();
    return result;
}

/*! That function returns map containing volume ID and face number
 * as per CalculiX definition for tetrahedral elements. See CalculiX
 * documentation for the details.
 */
std::map<int, int> FemMesh::getccxVolumesByFace(const TopoDS_Face& face) const
{
    std::map<int, int> result;
    std::set<int> nodes_on_face = getNodesByFace(face);

    static std::map<int, std::vector<int>> elem_order;
    if (elem_order.empty()) {
        std::vector<int> c3d4 = boost::assign::list_of(1)(0)(2)(3);
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
        std::pair<int, std::vector<int>> apair;
        apair.first = vol->GetID();

        std::map<int, std::vector<int>>::iterator it = elem_order.find(num_of_nodes);
        if (it != elem_order.end()) {
            const std::vector<int>& order = it->second;
            for (int jt : order) {
                int vid = vol->GetNode(jt)->GetID();
                apair.second.push_back(vid);
            }
        }

        // Get volume nodes on face
        std::vector<int> element_face_nodes;
        std::set<int> element_nodes;
        element_nodes.insert(apair.second.begin(), apair.second.end());
        std::set_intersection(nodes_on_face.begin(),
                              nodes_on_face.end(),
                              element_nodes.begin(),
                              element_nodes.end(),
                              std::back_insert_iterator<std::vector<int>>(element_face_nodes));

        if ((element_face_nodes.size() == 3 && num_of_nodes == 4)
            || (element_face_nodes.size() == 6 && num_of_nodes == 10)) {
            int missing_node = 0;
            for (int i = 0; i < 4; i++) {
                // search for the ID of the volume which is not part of 'element_face_nodes'
                if (std::find(element_face_nodes.begin(), element_face_nodes.end(), apair.second[i])
                    == element_face_nodes.end()) {
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
                    assert(false);  // should never happen
                    break;
            }
            result[apair.first] = face_ccx;
        }
    }

    return result;
}

std::set<int> FemMesh::getNodesBySolid(const TopoDS_Solid& solid) const
{
    std::set<int> result;

    Bnd_Box box;
    BRepBndLib::Add(solid, box);

    // limit where the mesh node belongs to the solid
    TopAbs_ShapeEnum shapetype = TopAbs_SHAPE;
    ShapeAnalysis_ShapeTolerance analysis;
    double limit = analysis.Tolerance(solid, 1, shapetype);
    Base::Console().Log("The limit if a node is in or out: %.12lf in scientific: %.4e \n",
                        limit,
                        limit);

    // get the current transform of the FemMesh
    const Base::Matrix4D Mtrx(getTransform());

    std::vector<const SMDS_MeshNode*> nodes;
    SMDS_NodeIteratorPtr aNodeIter = myMesh->GetMeshDS()->nodesIterator();
    while (aNodeIter->more()) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        nodes.push_back(aNode);
    }

#pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < nodes.size(); ++i) {
        const SMDS_MeshNode* aNode = nodes[i];
        double xyz[3];
        aNode->GetXYZ(xyz);
        Base::Vector3d vec(xyz[0], xyz[1], xyz[2]);
        // Apply the matrix to hold the BoundBox in absolute space.
        vec = Mtrx * vec;

        if (!box.IsOut(gp_Pnt(vec.x, vec.y, vec.z))) {
            // create a vertex
            BRepBuilderAPI_MakeVertex aBuilder(gp_Pnt(vec.x, vec.y, vec.z));
            TopoDS_Shape s = aBuilder.Vertex();
            // measure distance
            BRepExtrema_DistShapeShape measure(solid, s);
            measure.Perform();
            if (!measure.IsDone() || measure.NbSolution() < 1) {
                continue;
            }

            if (measure.Value() < limit)
#pragma omp critical
            {
                result.insert(aNode->GetID());
            }
        }
    }
    return result;
}

std::set<int> FemMesh::getNodesByFace(const TopoDS_Face& face) const
{
    std::set<int> result;

    Bnd_Box box;
    BRepBndLib::Add(
        face,
        box,
        Standard_False);  // https://forum.freecad.org/viewtopic.php?f=18&t=21571&start=70#p221591
    // limit where the mesh node belongs to the face:
    double limit = BRep_Tool::Tolerance(face);
    box.Enlarge(limit);

    // get the current transform of the FemMesh
    const Base::Matrix4D Mtrx(getTransform());

    std::vector<const SMDS_MeshNode*> nodes;
    SMDS_NodeIteratorPtr aNodeIter = myMesh->GetMeshDS()->nodesIterator();
    while (aNodeIter->more()) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        nodes.push_back(aNode);
    }

#pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < nodes.size(); ++i) {
        const SMDS_MeshNode* aNode = nodes[i];
        double xyz[3];
        aNode->GetXYZ(xyz);
        Base::Vector3d vec(xyz[0], xyz[1], xyz[2]);
        // Apply the matrix to hold the BoundBox in absolute space.
        vec = Mtrx * vec;

        if (!box.IsOut(gp_Pnt(vec.x, vec.y, vec.z))) {
            // create a vertex
            BRepBuilderAPI_MakeVertex aBuilder(gp_Pnt(vec.x, vec.y, vec.z));
            TopoDS_Shape s = aBuilder.Vertex();
            // measure distance
            BRepExtrema_DistShapeShape measure(face, s);
            measure.Perform();
            if (!measure.IsDone() || measure.NbSolution() < 1) {
                continue;
            }

            if (measure.Value() < limit)
#pragma omp critical
            {
                result.insert(aNode->GetID());
            }
        }
    }

    return result;
}

std::set<int> FemMesh::getNodesByEdge(const TopoDS_Edge& edge) const
{
    std::set<int> result;

    Bnd_Box box;
    BRepBndLib::Add(edge, box);
    // limit where the mesh node belongs to the edge:
    double limit = BRep_Tool::Tolerance(edge);
    box.Enlarge(limit);

    // get the current transform of the FemMesh
    const Base::Matrix4D Mtrx(getTransform());

    std::vector<const SMDS_MeshNode*> nodes;
    SMDS_NodeIteratorPtr aNodeIter = myMesh->GetMeshDS()->nodesIterator();
    while (aNodeIter->more()) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        nodes.push_back(aNode);
    }

#pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < nodes.size(); ++i) {
        const SMDS_MeshNode* aNode = nodes[i];
        double xyz[3];
        aNode->GetXYZ(xyz);
        Base::Vector3d vec(xyz[0], xyz[1], xyz[2]);
        // Apply the matrix to hold the BoundBox in absolute space.
        vec = Mtrx * vec;

        if (!box.IsOut(gp_Pnt(vec.x, vec.y, vec.z))) {
            // create a vertex
            BRepBuilderAPI_MakeVertex aBuilder(gp_Pnt(vec.x, vec.y, vec.z));
            TopoDS_Shape s = aBuilder.Vertex();
            // measure distance
            BRepExtrema_DistShapeShape measure(edge, s);
            measure.Perform();
            if (!measure.IsDone() || measure.NbSolution() < 1) {
                continue;
            }

            if (measure.Value() < limit)
#pragma omp critical
            {
                result.insert(aNode->GetID());
            }
        }
    }

    return result;
}

std::set<int> FemMesh::getNodesByVertex(const TopoDS_Vertex& vertex) const
{
    std::set<int> result;

    double limit = BRep_Tool::Tolerance(vertex);
    limit *= limit;  // use square to improve speed
    gp_Pnt pnt = BRep_Tool::Pnt(vertex);
    Base::Vector3d node(pnt.X(), pnt.Y(), pnt.Z());

    // get the current transform of the FemMesh
    const Base::Matrix4D Mtrx(getTransform());

    std::vector<const SMDS_MeshNode*> nodes;
    SMDS_NodeIteratorPtr aNodeIter = myMesh->GetMeshDS()->nodesIterator();
    while (aNodeIter->more()) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        nodes.push_back(aNode);
    }

#pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < nodes.size(); ++i) {
        const SMDS_MeshNode* aNode = nodes[i];
        double xyz[3];
        aNode->GetXYZ(xyz);
        Base::Vector3d vec(xyz[0], xyz[1], xyz[2]);
        vec = Mtrx * vec;

        if (Base::DistanceP2(node, vec) <= limit)
#pragma omp critical
        {
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
        for (int i = 0; i < elem->NbNodes(); i++) {
            result.push_back(elem->GetNode(i)->GetID());
        }
    }

    return result;
}

std::list<int> FemMesh::getNodeElements(int id, SMDSAbs_ElementType type) const
{
    std::list<int> result;
    const SMDS_MeshNode* node = myMesh->GetMeshDS()->FindNode(id);
    if (node) {
        SMDS_ElemIteratorPtr it = node->GetInverseElementIterator(type);
        while (it->more()) {
            const SMDS_MeshElement* elem = it->next();
            result.push_back(elem->GetID());
        }
    }

    return result;
}

std::set<int> FemMesh::getEdgesOnly() const
{
    std::set<int> resultIDs;

    // edges
    SMDS_EdgeIteratorPtr aEdgeIter = myMesh->GetMeshDS()->edgesIterator();
    while (aEdgeIter->more()) {
        const SMDS_MeshEdge* aEdge = aEdgeIter->next();
        std::list<int> enodes = getElementNodes(aEdge->GetID());
        std::set<int> aEdgeNodes(enodes.begin(), enodes.end());  // convert list to set
        bool edgeBelongsToAFace = false;

        // faces
        SMDS_FaceIteratorPtr aFaceIter = myMesh->GetMeshDS()->facesIterator();
        while (aFaceIter->more()) {
            const SMDS_MeshFace* aFace = aFaceIter->next();
            std::list<int> fnodes = getElementNodes(aFace->GetID());
            std::set<int> aFaceNodes(fnodes.begin(), fnodes.end());  // convert list to set

            // if aEdgeNodes is not a subset of any aFaceNodes --> aEdge does not belong to any Face
            std::vector<int> inodes;
            std::set_intersection(aFaceNodes.begin(),
                                  aFaceNodes.end(),
                                  aEdgeNodes.begin(),
                                  aEdgeNodes.end(),
                                  std::back_inserter(inodes));
            std::set<int> intersection_nodes(inodes.begin(),
                                             inodes.end());  // convert vector to set
            if (aEdgeNodes == intersection_nodes) {
                edgeBelongsToAFace = true;
                break;
            }
        }
        if (!edgeBelongsToAFace) {
            resultIDs.insert(aEdge->GetID());
        }
    }

    return resultIDs;
}

std::set<int> FemMesh::getFacesOnly() const
{
    // How it works ATM:
    // for each face
    //     get the face nodes
    //     for each volume
    //         get the volume nodes
    //         if the face nodes are a subset of the volume nodes
    //             add the face to the volume faces and break
    //     if face doesn't belong to a volume
    //         add it to faces only
    //
    // This means it is iterated over a lot of volumes many times, this is quite expensive!
    //
    // TODO make this faster
    // Idea:
    // for each volume
    //     get the faces and add them to the volume faces
    // for each face
    //     if not in volume faces
    //     add it to the faces only
    //
    // but the volume faces do not seem to know their global mesh ID, I could not find any method in
    // SMESH

    std::set<int> resultIDs;

    // faces
    SMDS_FaceIteratorPtr aFaceIter = myMesh->GetMeshDS()->facesIterator();
    while (aFaceIter->more()) {
        const SMDS_MeshFace* aFace = aFaceIter->next();
        std::list<int> fnodes = getElementNodes(aFace->GetID());
        std::set<int> aFaceNodes(fnodes.begin(), fnodes.end());  // convert list to set
        bool faceBelongsToAVolume = false;

        // volumes
        SMDS_VolumeIteratorPtr aVolIter = myMesh->GetMeshDS()->volumesIterator();
        while (aVolIter->more()) {
            const SMDS_MeshVolume* aVol = aVolIter->next();
            std::list<int> vnodes = getElementNodes(aVol->GetID());
            std::set<int> aVolNodes(vnodes.begin(), vnodes.end());  // convert list to set

            // if aFaceNodes is not a subset of any aVolNodes --> aFace does not belong to any
            // Volume
            std::vector<int> inodes;
            std::set_intersection(aVolNodes.begin(),
                                  aVolNodes.end(),
                                  aFaceNodes.begin(),
                                  aFaceNodes.end(),
                                  std::back_inserter(inodes));
            std::set<int> intersection_nodes(inodes.begin(),
                                             inodes.end());  // convert vector to set
            if (aFaceNodes == intersection_nodes) {
                faceBelongsToAVolume = true;
                break;
            }
        }
        if (!faceBelongsToAVolume) {
            resultIDs.insert(aFace->GetID());
        }
    }

    return resultIDs;
}

namespace
{
class NastranElement
{
public:
    virtual ~NastranElement() = default;
    bool isValid() const
    {
        return element_id >= 0;
    }
    virtual void read(const std::string& str1, const std::string& str2) = 0;
    virtual void addToMesh(SMESHDS_Mesh* meshds) = 0;

protected:
    int element_id = -1;
    std::vector<int> elements;
};

using NastranElementPtr = std::shared_ptr<NastranElement>;

class GRIDElement: public NastranElement
{
    void addToMesh(SMESHDS_Mesh* meshds) override
    {
        meshds->AddNodeWithID(node.x, node.y, node.z, element_id);
    }

protected:
    Base::Vector3d node;
};

class GRIDFreeFieldElement: public GRIDElement
{
    void read(const std::string& str, const std::string&) override
    {
        char_separator<char> sep(",");
        tokenizer<char_separator<char>> tokens(str, sep);
        std::vector<std::string> token_results;
        token_results.assign(tokens.begin(), tokens.end());
        if (token_results.size() < 6) {
            return;  // Line does not include Nodal coordinates
        }

        element_id = atoi(token_results[1].c_str());
        node.x = atof(token_results[3].c_str());
        node.y = atof(token_results[4].c_str());
        node.z = atof(token_results[5].c_str());
    }
};

class GRIDLongFieldElement: public GRIDElement
{
    void read(const std::string& str1, const std::string& str2) override
    {
        element_id = atoi(str1.substr(8, 24).c_str());
        node.x = atof(str1.substr(40, 56).c_str());
        node.y = atof(str1.substr(56, 72).c_str());
        node.z = atof(str2.substr(8, 24).c_str());
    }
};

class GRIDSmallFieldElement: public GRIDElement
{
    void read(const std::string&, const std::string&) override
    {}
};

class CTRIA3Element: public NastranElement
{
public:
    void addToMesh(SMESHDS_Mesh* meshds) override
    {
        const SMDS_MeshNode* n0 = meshds->FindNode(elements[0]);
        const SMDS_MeshNode* n1 = meshds->FindNode(elements[1]);
        const SMDS_MeshNode* n2 = meshds->FindNode(elements[2]);
        if (n0 && n1 && n2) {
            meshds->AddFaceWithID(n0, n1, n2, element_id);
        }
        else {
            Base::Console().Warning("NASTRAN: Failed to add face %d from nodes: (%d, %d, %d,)\n",
                                    element_id,
                                    elements[0],
                                    elements[1],
                                    elements[2]);
        }
    }
};

class CTRIA3FreeFieldElement: public CTRIA3Element
{
public:
    void read(const std::string& str, const std::string&) override
    {
        char_separator<char> sep(",");
        tokenizer<char_separator<char>> tokens(str, sep);
        std::vector<std::string> token_results;
        token_results.assign(tokens.begin(), tokens.end());
        if (token_results.size() < 6) {
            return;  // Line does not include enough nodal IDs
        }

        element_id = atoi(token_results[1].c_str());
        elements.push_back(atoi(token_results[3].c_str()));
        elements.push_back(atoi(token_results[4].c_str()));
        elements.push_back(atoi(token_results[5].c_str()));
    }
};

class CTRIA3LongFieldElement: public CTRIA3Element
{
public:
    void read(const std::string& str, const std::string&) override
    {
        element_id = atoi(str.substr(8, 16).c_str());
        elements.push_back(atoi(str.substr(24, 32).c_str()));
        elements.push_back(atoi(str.substr(32, 40).c_str()));
        elements.push_back(atoi(str.substr(40, 48).c_str()));
    }
};

class CTRIA3SmallFieldElement: public CTRIA3Element
{
public:
    void read(const std::string&, const std::string&) override
    {}
};

class CTETRAElement: public NastranElement
{
public:
    void addToMesh(SMESHDS_Mesh* meshds) override
    {
        const SMDS_MeshNode* n0 = meshds->FindNode(elements[1]);
        const SMDS_MeshNode* n1 = meshds->FindNode(elements[0]);
        const SMDS_MeshNode* n2 = meshds->FindNode(elements[2]);
        const SMDS_MeshNode* n3 = meshds->FindNode(elements[3]);
        const SMDS_MeshNode* n4 = meshds->FindNode(elements[4]);
        const SMDS_MeshNode* n5 = meshds->FindNode(elements[6]);
        const SMDS_MeshNode* n6 = meshds->FindNode(elements[5]);
        const SMDS_MeshNode* n7 = meshds->FindNode(elements[8]);
        const SMDS_MeshNode* n8 = meshds->FindNode(elements[7]);
        const SMDS_MeshNode* n9 = meshds->FindNode(elements[9]);
        if (n0 && n1 && n2 && n3 && n4 && n5 && n6 && n7 && n8 && n9) {
            meshds->AddVolumeWithID(n0, n1, n2, n3, n4, n5, n6, n7, n8, n9, element_id);
        }
        else {
            Base::Console().Warning("NASTRAN: Failed to add volume %d from nodes: (%d, %d, %d, %d, "
                                    "%d, %d, %d, %d, %d, %d)\n",
                                    element_id,
                                    elements[1],
                                    elements[0],
                                    elements[2],
                                    elements[3],
                                    elements[4],
                                    elements[6],
                                    elements[5],
                                    elements[8],
                                    elements[7],
                                    elements[9]);
        }
    }
};

class CTETRAFreeFieldElement: public CTETRAElement
{
public:
    void read(const std::string& str, const std::string&) override
    {
        char_separator<char> sep(",");
        tokenizer<char_separator<char>> tokens(str, sep);
        std::vector<std::string> token_results;
        token_results.assign(tokens.begin(), tokens.end());
        if (token_results.size() < 14) {
            return;  // Line does not include enough nodal IDs
        }

        element_id = atoi(token_results[1].c_str());
        elements.push_back(atoi(token_results[3].c_str()));
        elements.push_back(atoi(token_results[4].c_str()));
        elements.push_back(atoi(token_results[5].c_str()));
        elements.push_back(atoi(token_results[6].c_str()));
        elements.push_back(atoi(token_results[7].c_str()));
        elements.push_back(atoi(token_results[8].c_str()));
        elements.push_back(atoi(token_results[10].c_str()));
        elements.push_back(atoi(token_results[11].c_str()));
        elements.push_back(atoi(token_results[12].c_str()));
        elements.push_back(atoi(token_results[13].c_str()));
    }
};

class CTETRALongFieldElement: public CTETRAElement
{
public:
    void read(const std::string& str1, const std::string& str2) override
    {
        int id = atoi(str1.substr(8, 16).c_str());
        int offset = 0;

        if (id < 1000000) {
            offset = 0;
        }
        else if (id < 10000000) {
            offset = 1;
        }
        else if (id < 100000000) {
            offset = 2;
        }


        element_id = id;
        elements.push_back(atoi(str1.substr(24, 32).c_str()));
        elements.push_back(atoi(str1.substr(32, 40).c_str()));
        elements.push_back(atoi(str1.substr(40, 48).c_str()));
        elements.push_back(atoi(str1.substr(48, 56).c_str()));
        elements.push_back(atoi(str1.substr(56, 64).c_str()));
        elements.push_back(atoi(str1.substr(64, 72).c_str()));
        elements.push_back(atoi(str2.substr(8 + offset, 16 + offset).c_str()));
        elements.push_back(atoi(str2.substr(16 + offset, 24 + offset).c_str()));
        elements.push_back(atoi(str2.substr(24 + offset, 32 + offset).c_str()));
        elements.push_back(atoi(str2.substr(32 + offset, 40 + offset).c_str()));
    }
};


class CTETRASmallFieldElement: public CTETRAElement
{
public:
    void read(const std::string&, const std::string&) override
    {}
};

// NASTRAN-95

class GRIDNastran95Element: public GRIDElement
{
    void read(const std::string& str, const std::string&) override
    {
        element_id = atoi(str.substr(8, 16).c_str());
        node.x = atof(str.substr(24, 32).c_str());
        node.y = atof(str.substr(32, 40).c_str());
        node.z = atof(str.substr(40, 48).c_str());
    }
};

class CBARElement: public NastranElement
{
    void read(const std::string& str, const std::string&) override
    {
        element_id = atoi(str.substr(8, 16).c_str());
        elements.push_back(atoi(str.substr(24, 32).c_str()));
        elements.push_back(atoi(str.substr(32, 40).c_str()));
    }
    void addToMesh(SMESHDS_Mesh* meshds) override
    {
        meshds->AddEdgeWithID(elements[0], elements[1], element_id);
    }
};

class CTRMEMElement: public NastranElement
{
    void read(const std::string& str, const std::string&) override
    {
        element_id = atoi(str.substr(8, 16).c_str());
        elements.push_back(atoi(str.substr(24, 32).c_str()));
        elements.push_back(atoi(str.substr(32, 40).c_str()));
        elements.push_back(atoi(str.substr(40, 48).c_str()));
    }
    void addToMesh(SMESHDS_Mesh* meshds) override
    {
        meshds->AddFaceWithID(elements[0], elements[1], elements[2], element_id);
    }
};

class CTRIA1Element: public NastranElement
{
    void read(const std::string& str, const std::string&) override
    {
        element_id = atoi(str.substr(8, 16).c_str());
        elements.push_back(atoi(str.substr(24, 32).c_str()));
        elements.push_back(atoi(str.substr(32, 40).c_str()));
        elements.push_back(atoi(str.substr(40, 48).c_str()));
    }
    void addToMesh(SMESHDS_Mesh* meshds) override
    {
        meshds->AddFaceWithID(elements[0], elements[1], elements[2], element_id);
    }
};

class CQUAD1Element: public NastranElement
{
    void read(const std::string& str, const std::string&) override
    {
        element_id = atoi(str.substr(8, 16).c_str());
        elements.push_back(atoi(str.substr(24, 32).c_str()));
        elements.push_back(atoi(str.substr(32, 40).c_str()));
        elements.push_back(atoi(str.substr(40, 48).c_str()));
        elements.push_back(atoi(str.substr(48, 56).c_str()));
    }
    void addToMesh(SMESHDS_Mesh* meshds) override
    {
        meshds->AddFaceWithID(elements[0], elements[1], elements[2], elements[3], element_id);
    }
};

class CTETRANastran95Element: public NastranElement
{
    void read(const std::string& str, const std::string&) override
    {
        element_id = atoi(str.substr(8, 16).c_str());
        elements.push_back(atoi(str.substr(24, 32).c_str()));
        elements.push_back(atoi(str.substr(32, 40).c_str()));
        elements.push_back(atoi(str.substr(40, 48).c_str()));
        elements.push_back(atoi(str.substr(48, 56).c_str()));
    }
    void addToMesh(SMESHDS_Mesh* meshds) override
    {
        meshds->AddFaceWithID(elements[0], elements[1], elements[2], elements[3], element_id);
    }
};

class CWEDGEElement: public NastranElement
{
    void read(const std::string& str, const std::string&) override
    {
        element_id = atoi(str.substr(8, 16).c_str());
        elements.push_back(atoi(str.substr(24, 32).c_str()));
        elements.push_back(atoi(str.substr(32, 40).c_str()));
        elements.push_back(atoi(str.substr(40, 48).c_str()));
        elements.push_back(atoi(str.substr(48, 56).c_str()));
        elements.push_back(atoi(str.substr(56, 64).c_str()));
        elements.push_back(atoi(str.substr(64, 72).c_str()));
    }
    void addToMesh(SMESHDS_Mesh* meshds) override
    {
        meshds->AddVolumeWithID(elements[0],
                                elements[1],
                                elements[2],
                                elements[3],
                                elements[4],
                                elements[5],
                                element_id);
    }
};

class CHEXA1Element: public NastranElement
{
    void read(const std::string& str1, const std::string& str2) override
    {
        element_id = atoi(str1.substr(8, 16).c_str());
        elements.push_back(atoi(str1.substr(24, 32).c_str()));
        elements.push_back(atoi(str1.substr(32, 40).c_str()));
        elements.push_back(atoi(str1.substr(40, 48).c_str()));
        elements.push_back(atoi(str1.substr(48, 56).c_str()));
        elements.push_back(atoi(str1.substr(56, 64).c_str()));
        elements.push_back(atoi(str1.substr(64, 72).c_str()));

        elements.push_back(atoi(str2.substr(8, 16).c_str()));
        elements.push_back(atoi(str2.substr(16, 24).c_str()));
    }
    void addToMesh(SMESHDS_Mesh* meshds) override
    {
        meshds->AddVolumeWithID(elements[0],
                                elements[1],
                                elements[2],
                                elements[3],
                                elements[4],
                                elements[5],
                                elements[6],
                                elements[7],
                                element_id);
    }
};

class CHEXA2Element: public NastranElement
{
    void read(const std::string& str1, const std::string& str2) override
    {
        element_id = atoi(str1.substr(8, 16).c_str());
        elements.push_back(atoi(str1.substr(24, 32).c_str()));
        elements.push_back(atoi(str1.substr(32, 40).c_str()));
        elements.push_back(atoi(str1.substr(40, 48).c_str()));
        elements.push_back(atoi(str1.substr(48, 56).c_str()));
        elements.push_back(atoi(str1.substr(56, 64).c_str()));
        elements.push_back(atoi(str1.substr(64, 72).c_str()));

        elements.push_back(atoi(str2.substr(8, 16).c_str()));
        elements.push_back(atoi(str2.substr(16, 24).c_str()));
    }
    void addToMesh(SMESHDS_Mesh* meshds) override
    {
        meshds->AddVolumeWithID(elements[0],
                                elements[1],
                                elements[2],
                                elements[3],
                                elements[4],
                                elements[5],
                                elements[6],
                                elements[7],
                                element_id);
    }
};

}  // namespace

void FemMesh::readNastran(const std::string& Filename)
{
    Base::TimeElapsed Start;
    Base::Console().Log("Start: FemMesh::readNastran() =================================\n");

    _Mtrx = Base::Matrix4D();

    Base::FileInfo fi(Filename);
    Base::ifstream inputfile;
    inputfile.open(fi);
    inputfile.seekg(std::ifstream::beg);
    std::string line1, line2;
    std::vector<NastranElementPtr> mesh_elements;
    enum Format
    {
        FreeField,
        SmallField,
        LongField
    };
    Format nastranFormat = Format::LongField;

    do {
        std::getline(inputfile, line1);
        if (line1.empty()) {
            continue;
        }
        if (line1.find(',') != std::string::npos) {
            nastranFormat = Format::FreeField;
        }

        NastranElementPtr ptr;
        if (line1.find("GRID*") != std::string::npos) {  // We found a Grid line
            // Now lets extract the GRID Points = Nodes
            // As each GRID Line consists of two subsequent lines we have to
            // take care of that as well
            if (nastranFormat == Format::LongField) {
                std::getline(inputfile, line2);
                ptr = std::make_shared<GRIDLongFieldElement>();
                ptr->read(line1, line2);
            }
        }
        else if (line1.find("GRID") != std::string::npos) {  // We found a Grid line
            if (nastranFormat == Format::FreeField) {
                ptr = std::make_shared<GRIDFreeFieldElement>();
                ptr->read(line1, "");
            }
        }
        else if (line1.find("CTRIA3") != std::string::npos) {
            if (nastranFormat == Format::FreeField) {
                ptr = std::make_shared<CTRIA3FreeFieldElement>();
                ptr->read(line1, "");
            }
            else {
                ptr = std::make_shared<CTRIA3LongFieldElement>();
                ptr->read(line1, "");
            }
        }
        else if (line1.find("CTETRA") != std::string::npos) {
            // Lets extract the elements
            // As each Element Line consists of two subsequent lines as well
            // we have to take care of that
            // At a first step we only extract Quadratic Tetrahedral Elements
            std::getline(inputfile, line2);
            if (nastranFormat == Format::FreeField) {
                ptr = std::make_shared<CTETRAFreeFieldElement>();
                ptr->read(line1.append(line2), "");
            }
            else {
                ptr = std::make_shared<CTETRALongFieldElement>();
                ptr->read(line1, line2);
            }
        }

        if (ptr && ptr->isValid()) {
            mesh_elements.push_back(ptr);
        }
    } while (inputfile.good());
    inputfile.close();

    Base::Console().Log("    %f: File read, start building mesh\n",
                        Base::TimeElapsed::diffTimeF(Start, Base::TimeElapsed()));

    // Now fill the SMESH datastructure
    SMESHDS_Mesh* meshds = this->myMesh->GetMeshDS();
    meshds->ClearMesh();

    for (auto it : mesh_elements) {
        it->addToMesh(meshds);
    }

    Base::Console().Log("    %f: Done \n",
                        Base::TimeElapsed::diffTimeF(Start, Base::TimeElapsed()));
}

void FemMesh::readNastran95(const std::string& Filename)
{
    Base::TimeElapsed Start;
    Base::Console().Log("Start: FemMesh::readNastran95() =================================\n");

    _Mtrx = Base::Matrix4D();

    Base::FileInfo fi(Filename);
    Base::ifstream inputfile;
    inputfile.open(fi);
    inputfile.seekg(std::ifstream::beg);
    std::string line1, line2, tcard;

    std::vector<NastranElementPtr> mesh_nodes;
    std::vector<NastranElementPtr> mesh_elements;

    do {
        NastranElementPtr node;
        NastranElementPtr elem;
        std::getline(inputfile, line1);
        // cout << line1 << endl;
        if (line1.empty()) {
            continue;
        }

        tcard = line1.substr(0, 8).c_str();
        // boost::algorithm::trim(tcard);
        if (line1.find("GRID*") != std::string::npos)  // We found a Grid line
        {
            // Now lets extract the GRID Points = Nodes
            // As each GRID Line consists of two subsequent lines we have to
            // take care of that as well
            std::getline(inputfile, line2);
            node = std::make_shared<GRIDLongFieldElement>();
            node->read(line1, line2);
        }
        else if (line1.find("GRID") != std::string::npos)  // We found a Grid line
        {
            // Base::Console().Log("Found a GRID\n");
            // D06.inp
            // GRID    109             .9      .7
            // Now lets extract the GRID Points = Nodes
            // Get the Nodal ID
            node = std::make_shared<GRIDNastran95Element>();
            node->read(line1, "");
        }

        // 1D
        else if (line1.substr(0, 6) == "CBAR") {
            elem = std::make_shared<CBARElement>();
            elem->read(line1, "");
        }
        // 2d
        else if (line1.substr(0, 6) == "CTRMEM") {
            // D06
            // CTRMEM  322     1       179     180     185
            elem = std::make_shared<CTRMEMElement>();
            elem->read(line1, "");
        }
        else if (line1.substr(0, 6) == "CTRIA1") {
            // D06
            // CTRMEM  322     1       179     180     185
            elem = std::make_shared<CTRIA1Element>();
            elem->read(line1, "");
        }
        else if (line1.substr(0, 6) == "CQUAD1") {
            // D06
            // CTRMEM  322     1       179     180     185
            elem = std::make_shared<CQUAD1Element>();
            elem->read(line1, "");
        }

        // 3d element
        else if (line1.find("CTETRA") != std::string::npos) {
            // d011121a.inp
            // CTETRA  3       200     104     114     3       103
            elem = std::make_shared<CTETRANastran95Element>();
            elem->read(line1, "");
        }
        else if (line1.find("CWEDGE") != std::string::npos) {
            // d011121a.inp
            // CWEDGE  11      200     6       17      16      106     117     116
            elem = std::make_shared<CTETRANastran95Element>();
            elem->read(line1, "");
        }
        else if (line1.find("CHEXA1") != std::string::npos) {
            // d011121a.inp
            // CHEXA1  1       200     1       2       13      12      101     102     +SOL1
            //+SOL1   113     112
            std::getline(inputfile, line2);
            elem = std::make_shared<CHEXA1Element>();
            elem->read(line1, line2);
        }
        else if (line1.find("CHEXA2") != std::string::npos) {
            // d011121a.inp
            // CHEXA1  1       200     1       2       13      12      101     102     +SOL1
            //+SOL1   113     112
            std::getline(inputfile, line2);
            elem = std::make_shared<CHEXA2Element>();
            elem->read(line1, line2);
        }

        if (node && node->isValid()) {
            mesh_nodes.push_back(node);
        }

        if (elem && elem->isValid()) {
            mesh_elements.push_back(elem);
        }
    } while (inputfile.good());
    inputfile.close();

    Base::Console().Log("    %f: File read, start building mesh\n",
                        Base::TimeElapsed::diffTimeF(Start, Base::TimeElapsed()));

    // Now fill the SMESH datastructure
    SMESHDS_Mesh* meshds = this->myMesh->GetMeshDS();
    meshds->ClearMesh();

    for (auto it : mesh_nodes) {
        it->addToMesh(meshds);
    }

    for (auto it : mesh_elements) {
        it->addToMesh(meshds);
    }

    Base::Console().Log("    %f: Done \n",
                        Base::TimeElapsed::diffTimeF(Start, Base::TimeElapsed()));
}

void FemMesh::readAbaqus(const std::string& FileName)
{
    Base::TimeElapsed Start;
    Base::Console().Log("Start: FemMesh::readAbaqus() =================================\n");

    /*
    Python command to read Abaqus inp mesh file from test suite:
    from feminout.importInpMesh import read as read_inp
    femmesh = read_inp(FreeCAD.ConfigGet("AppHomePath") +
    'Mod/Fem/femtest/data/mesh/tetra10_mesh.inp')
    */

    PyObject* module = PyImport_ImportModule("feminout.importInpMesh");
    if (!module) {
        return;
    }
    try {
        Py::Module abaqusmod(module, true);
        Py::Callable method(abaqusmod.getAttr("read"));
        Py::Tuple args(1);
        args.setItem(0, Py::String(FileName));
        Py::Object mesh(method.apply(args));
        if (PyObject_TypeCheck(mesh.ptr(), &FemMeshPy::Type)) {
            FemMeshPy* fempy = static_cast<FemMeshPy*>(mesh.ptr());
            FemMesh* fem = fempy->getFemMeshPtr();
            *this = *fem;  // the deep copy should be avoided, a pointer swap method could be
                           // implemented see
                           // https://forum.freecad.org/viewtopic.php?f=10&t=31999&start=10#p274241
        }
        else {
            throw Base::FileException("Problems reading file");
        }
    }
    catch (Py::Exception& e) {
        e.clear();
    }
    Base::Console().Log("    %f: Done \n",
                        Base::TimeElapsed::diffTimeF(Start, Base::TimeElapsed()));
}

void FemMesh::readZ88(const std::string& FileName)
{
    Base::TimeElapsed Start;
    Base::Console().Log("Start: FemMesh::readZ88() =================================\n");

    /*
    Python command to read Z88 mesh file from test suite:
    from feminout.importZ88Mesh import read as read_z88
    femmesh = read_z88(FreeCAD.ConfigGet("AppHomePath") +
    'Mod/Fem/femtest/data/mesh/tetra10_mesh.z88')
    */

    PyObject* module = PyImport_ImportModule("feminout.importZ88Mesh");
    if (!module) {
        return;
    }
    try {
        Py::Module z88mod(module, true);
        Py::Callable method(z88mod.getAttr("read"));
        Py::Tuple args(1);
        args.setItem(0, Py::String(FileName));
        Py::Object mesh(method.apply(args));
        if (PyObject_TypeCheck(mesh.ptr(), &FemMeshPy::Type)) {
            FemMeshPy* fempy = static_cast<FemMeshPy*>(mesh.ptr());
            FemMesh* fem = fempy->getFemMeshPtr();
            *this = *fem;  // the deep copy should be avoided, a pointer swap method could be
                           // implemented see
                           // https://forum.freecad.org/viewtopic.php?f=10&t=31999&start=10#p274241
        }
        else {
            throw Base::FileException("Problems reading file");
        }
    }
    catch (Py::Exception& e) {
        e.clear();
    }
    Base::Console().Log("    %f: Done \n",
                        Base::TimeElapsed::diffTimeF(Start, Base::TimeElapsed()));
}

void FemMesh::read(const char* FileName)
{
    Base::FileInfo File(FileName);
    _Mtrx = Base::Matrix4D();

    // checking on the file
    if (!File.isReadable()) {
        throw Base::FileException("File to load not existing or not readable", File);
    }

    if (File.hasExtension("unv")) {
        // read UNV file
        myMesh->UNVToMesh(File.filePath().c_str());
    }
    else if (File.hasExtension("med")) {
        myMesh->MEDToMesh(File.filePath().c_str(), File.fileNamePure().c_str());
    }
    else if (File.hasExtension("inp")) {
        // read Abaqus inp mesh file
        readAbaqus(File.filePath());

        // if the file doesn't contain supported geometries try Nastran95
        SMESHDS_Mesh* meshds = this->myMesh->GetMeshDS();
        if (meshds->NbNodes() == 0) {
            readNastran95(File.filePath());
        }
    }
    else if (File.hasExtension("stl")) {
        // read brep-file
        myMesh->STLToMesh(File.filePath().c_str());
    }
    else if (File.hasExtension("bdf")) {
        // read Nastran-file
        readNastran(File.filePath());
    }
#ifdef FC_USE_VTK
    else if (File.hasExtension({"vtk", "vtu", "pvtu"})) {
        // read *.vtk legacy format or *.vtu XML unstructure Mesh
        FemVTKTools::readVTKMesh(File.filePath().c_str(), this);
    }
#endif
    else if (File.hasExtension("z88")) {
        // read Z88 mesh file
        readZ88(File.filePath());
    }
    else {
        throw Base::FileException("Unknown extension");
    }
}

void FemMesh::writeABAQUS(const std::string& Filename, int elemParam, bool groupParam) const
{
    /*
     * elemParam:
     * 0 = all elements
     * 1 = highest elements only
     * 2 = FEM elements only (only edges not belonging to faces and faces not belonging to volumes)
     *
     * groupParam:
     * true = write group data
     * false = do not write group data
     */

    static std::map<std::string, std::vector<int>> elemOrderMap;
    static std::map<int, std::string> edgeTypeMap;
    static std::map<int, std::string> faceTypeMap;
    static std::map<int, std::string> volTypeMap;
    if (elemOrderMap.empty()) {
        // node order fits with node order in importCcxFrdResults.py module to import
        // CalculiX result meshes

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
        // changed to this in August 2013, committed by juergen (jriedel)
        // https://github.com/FreeCAD/FreeCAD/commit/af56b324b9566b20f3b6e7880c29354c1dbe7a99
        // std::vector<int> c3d4  = boost::assign::list_of(0)(3)(1)(2);
        // std::vector<int> c3d10 = boost::assign::list_of(0)(2)(1)(3)(6)(5)(4)(7)(9)(8);

        // since master 0.15
        // added by werner (wmayer) March 2015,
        // https://forum.freecad.org/viewtopic.php?f=18&t=10110&start=10#p81681
        // https://github.com/FreeCAD/FreeCAD/commit/5d159f5cf352a93b1aff4fb7b82e8b747ee4f35b
        // https://github.com/FreeCAD/FreeCAD/commit/b007bd19e4e4608caa4cdad350a9f480287fac6b
        // tetra4 FreeCAD --> C3D4 CalculiX
        // N2, N1, N3, N4
        std::vector<int> c3d4 = boost::assign::list_of(1)(0)(2)(3);
        // tetra10: FreeCAD --> C3D10 CalculiX
        // N2, N1, N3, N4, N5, N7, N6, N9, N8, N10
        std::vector<int> c3d10 = boost::assign::list_of(1)(0)(2)(3)(4)(6)(5)(8)(7)(9);

        // tetra node order for the system which is used for hexa8, hexa20, penta6 and penda15
        // be careful with activating because of method getccxVolumesByFace())
        // tetra4 FreeCAD --> C3D4 CalculiX
        // N2, N3, N4, N1
        // std::vector<int> c3d4  = boost::assign::list_of(1)(2)(3)(0);
        //
        // tetra10: FreeCAD --> C3D10 CalculiX
        // N2, N3, N4, N1, N6, N10, N9, N5, N7, N8
        // std::vector<int> c3d10 = boost::assign::list_of(1)(2)(3)(0)(5)(9)(8)(4)(6)(7);

        // hexa8 FreeCAD --> C3D8 CalculiX
        // N6, N7, N8, N5, N2, N3, N4, N1
        std::vector<int> c3d8 = boost::assign::list_of(5)(6)(7)(4)(1)(2)(3)(0);
        //
        // hexa20 FreeCAD --> C3D20 CalculiX
        // N6, N7, N8, N5, N2, N3, N4, N1, N14, N15, N16, N13, N10, N11, N12, N9, N18, N19, N20, N17
        std::vector<int> c3d20 = boost::assign::list_of(5)(6)(7)(4)(1)(2)(3)(0)(13)(14)(15)(12)(9)(
            10)(11)(8)(17)(18)(19)(16);
        //
        // penta6 FreeCAD --> C3D6 CalculiX
        // N5, N6, N4, N2, N3, N1
        std::vector<int> c3d6 = boost::assign::list_of(4)(5)(3)(1)(2)(0);
        //
        // penta15 FreeCAD --> C3D15 CalculiX
        // N5, N6, N4, N2, N3, N1, N11, N12, N10, N8, N9, N7, N14, N15, N13
        std::vector<int> c3d15 =
            boost::assign::list_of(4)(5)(3)(1)(2)(0)(10)(11)(9)(7)(8)(6)(13)(14)(12);

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

    // get all data --> Extract Nodes and Elements of the current SMESH datastructure
    using VertexMap = std::map<int, Base::Vector3d>;
    using NodesMap = std::map<int, std::vector<int>>;
    using ElementsMap = std::map<std::string, NodesMap>;

    // get nodes
    VertexMap vertexMap;  // empty nodes map
    SMDS_NodeIteratorPtr aNodeIter = myMesh->GetMeshDS()->nodesIterator();
    Base::Vector3d current_node;
    while (aNodeIter->more()) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        current_node.Set(aNode->X(), aNode->Y(), aNode->Z());
        current_node = _Mtrx * current_node;
        vertexMap[aNode->GetID()] = current_node;
    }

    // get volumes
    ElementsMap elementsMapVol;  // empty volumes map
    SMDS_VolumeIteratorPtr aVolIter = myMesh->GetMeshDS()->volumesIterator();
    while (aVolIter->more()) {
        const SMDS_MeshVolume* aVol = aVolIter->next();
        std::pair<int, std::vector<int>> apair;
        apair.first = aVol->GetID();
        int numNodes = aVol->NbNodes();
        std::map<int, std::string>::iterator it = volTypeMap.find(numNodes);
        if (it != volTypeMap.end()) {
            const std::vector<int>& order = elemOrderMap[it->second];
            for (int jt : order) {
                apair.second.push_back(aVol->GetNode(jt)->GetID());
            }
            elementsMapVol[it->second].insert(apair);
        }
    }

    // get faces
    ElementsMap elementsMapFac;  // empty faces map used for elemParam = 1
                                 // and elementsMapVol is not empty
    if ((elemParam == 0) || (elemParam == 1 && elementsMapVol.empty())) {
        // for elemParam = 1 we only fill the elementsMapFac if the elmentsMapVol is empty
        // we're going to fill the elementsMapFac with all faces
        SMDS_FaceIteratorPtr aFaceIter = myMesh->GetMeshDS()->facesIterator();
        while (aFaceIter->more()) {
            const SMDS_MeshFace* aFace = aFaceIter->next();
            std::pair<int, std::vector<int>> apair;
            apair.first = aFace->GetID();
            int numNodes = aFace->NbNodes();
            std::map<int, std::string>::iterator it = faceTypeMap.find(numNodes);
            if (it != faceTypeMap.end()) {
                const std::vector<int>& order = elemOrderMap[it->second];
                for (int jt : order) {
                    apair.second.push_back(aFace->GetNode(jt)->GetID());
                }
                elementsMapFac[it->second].insert(apair);
            }
        }
    }
    if (elemParam == 2) {
        // we're going to fill the elementsMapFac with the facesOnly
        std::set<int> facesOnly = getFacesOnly();
        for (int itfa : facesOnly) {
            std::pair<int, std::vector<int>> apair;
            apair.first = itfa;
            const SMDS_MeshElement* aFace = myMesh->GetMeshDS()->FindElement(itfa);
            int numNodes = aFace->NbNodes();
            std::map<int, std::string>::iterator it = faceTypeMap.find(numNodes);
            if (it != faceTypeMap.end()) {
                const std::vector<int>& order = elemOrderMap[it->second];
                for (int jt : order) {
                    apair.second.push_back(aFace->GetNode(jt)->GetID());
                }
                elementsMapFac[it->second].insert(apair);
            }
        }
    }

    // get edges
    ElementsMap elementsMapEdg;  // empty edges map used for elemParam == 1
                                 // and either elementMapVol or elementsMapFac are not empty
    if ((elemParam == 0) || (elemParam == 1 && elementsMapVol.empty() && elementsMapFac.empty())) {
        // for elemParam = 1 we only fill the elementsMapEdg if the elmentsMapVol
        // and elmentsMapFac are empty we're going to fill the elementsMapEdg with all edges
        SMDS_EdgeIteratorPtr aEdgeIter = myMesh->GetMeshDS()->edgesIterator();
        while (aEdgeIter->more()) {
            const SMDS_MeshEdge* aEdge = aEdgeIter->next();
            std::pair<int, std::vector<int>> apair;
            apair.first = aEdge->GetID();
            int numNodes = aEdge->NbNodes();
            std::map<int, std::string>::iterator it = edgeTypeMap.find(numNodes);
            if (it != edgeTypeMap.end()) {
                const std::vector<int>& order = elemOrderMap[it->second];
                for (int jt : order) {
                    apair.second.push_back(aEdge->GetNode(jt)->GetID());
                }
                elementsMapEdg[it->second].insert(apair);
            }
        }
    }
    if (elemParam == 2) {
        // we're going to fill the elementsMapEdg with the edgesOnly
        std::set<int> edgesOnly = getEdgesOnly();
        for (int ited : edgesOnly) {
            std::pair<int, std::vector<int>> apair;
            apair.first = ited;
            const SMDS_MeshElement* aEdge = myMesh->GetMeshDS()->FindElement(ited);
            int numNodes = aEdge->NbNodes();
            std::map<int, std::string>::iterator it = edgeTypeMap.find(numNodes);
            if (it != edgeTypeMap.end()) {
                const std::vector<int>& order = elemOrderMap[it->second];
                for (int jt : order) {
                    apair.second.push_back(aEdge->GetNode(jt)->GetID());
                }
                elementsMapEdg[it->second].insert(apair);
            }
        }
    }

    // write all data to file
    // take also care of special characters in path
    // https://forum.freecad.org/viewtopic.php?f=10&t=37436
    Base::FileInfo fi(Filename);
    Base::ofstream anABAQUS_Output(fi);
    // https://forum.freecad.org/viewtopic.php?f=18&t=22759#p176669
    anABAQUS_Output.precision(13);

    // add some text and make sure one of the known elemParam values is used
    anABAQUS_Output << "** written by FreeCAD inp file writer for CalculiX,Abaqus meshes"
                    << std::endl;
    switch (elemParam) {
        case 0:
            anABAQUS_Output << "** all mesh elements." << std::endl << std::endl;
            break;
        case 1:
            anABAQUS_Output << "** highest dimension mesh elements only." << std::endl << std::endl;
            break;
        case 2:
            anABAQUS_Output << "** FEM mesh elements only (edges if they do not belong to faces "
                               "and faces if they do not belong to volumes)."
                            << std::endl
                            << std::endl;
            break;
        default:
            anABAQUS_Output << "** Problem on writing" << std::endl;
            anABAQUS_Output.close();
            throw std::runtime_error(
                "Unknown ABAQUS element choice parameter, [0|1|2] are allowed.");
    }

    // write nodes
    anABAQUS_Output << "** Nodes" << std::endl;
    anABAQUS_Output << "*Node, NSET=Nall" << std::endl;
    // This way we get sorted output.
    // See https://forum.freecad.org/viewtopic.php?f=18&t=12646&start=40#p103004
    for (const auto& it : vertexMap) {
        anABAQUS_Output << it.first << ", " << it.second.x << ", " << it.second.y << ", "
                        << it.second.z << std::endl;
    }
    anABAQUS_Output << std::endl << std::endl;
    ;


    // write volumes to file
    std::string elsetname;
    if (!elementsMapVol.empty()) {
        for (const auto& it : elementsMapVol) {
            anABAQUS_Output << "** Volume elements" << std::endl;
            anABAQUS_Output << "*Element, TYPE=" << it.first << ", ELSET=Evolumes" << std::endl;
            for (const auto& jt : it.second) {
                anABAQUS_Output << jt.first;
                // Calculix allows max 16 entries in one line, a hexa20 has more !
                int ct = 0;  // counter
                bool first_line = true;
                for (auto kt = jt.second.begin(); kt != jt.second.end(); ++kt, ++ct) {
                    if (ct < 15) {
                        anABAQUS_Output << ", " << *kt;
                    }
                    else {
                        if (first_line) {
                            anABAQUS_Output << "," << std::endl;
                            first_line = false;
                        }
                        anABAQUS_Output << *kt << ", ";
                    }
                }
                anABAQUS_Output << std::endl;
            }
        }
        elsetname += "Evolumes";
        anABAQUS_Output << std::endl;
    }

    // write faces to file
    if (!elementsMapFac.empty()) {
        for (const auto& it : elementsMapFac) {
            anABAQUS_Output << "** Face elements" << std::endl;
            anABAQUS_Output << "*Element, TYPE=" << it.first << ", ELSET=Efaces" << std::endl;
            for (const auto& jt : it.second) {
                anABAQUS_Output << jt.first;
                for (int kt : jt.second) {
                    anABAQUS_Output << ", " << kt;
                }
                anABAQUS_Output << std::endl;
            }
        }
        if (elsetname.empty()) {
            elsetname += "Efaces";
        }
        else {
            elsetname += ", Efaces";
        }
        anABAQUS_Output << std::endl;
    }

    // write edges to file
    if (!elementsMapEdg.empty()) {
        for (const auto& it : elementsMapEdg) {
            anABAQUS_Output << "** Edge elements" << std::endl;
            anABAQUS_Output << "*Element, TYPE=" << it.first << ", ELSET=Eedges" << std::endl;
            for (const auto& jt : it.second) {
                anABAQUS_Output << jt.first;
                for (int kt : jt.second) {
                    anABAQUS_Output << ", " << kt;
                }
                anABAQUS_Output << std::endl;
            }
        }
        if (elsetname.empty()) {
            elsetname += "Eedges";
        }
        else {
            elsetname += ", Eedges";
        }
        anABAQUS_Output << std::endl;
    }

    // write elset Eall
    anABAQUS_Output << "** Define element set Eall" << std::endl;
    anABAQUS_Output << "*ELSET, ELSET=Eall" << std::endl;
    anABAQUS_Output << elsetname << std::endl;

    // groups
    if (!groupParam) {
        anABAQUS_Output.close();
    }
    else {
        // get and write group data
        anABAQUS_Output << std::endl << "** Group data" << std::endl;

        std::list<int> groupIDs = myMesh->GetGroupIds();
        for (int it : groupIDs) {

            // get and write group info and group definition
            // TODO group element type code has duplicate code of
            // PyObject* FemMeshPy::getGroupElementType()
            SMDSAbs_ElementType aElementType = myMesh->GetGroup(it)->GetGroupDS()->GetType();
            const char* groupElementType = "";
            switch (aElementType) {
                case SMDSAbs_All:
                    groupElementType = "All";
                    break;
                case SMDSAbs_Node:
                    groupElementType = "Node";
                    break;
                case SMDSAbs_Edge:
                    groupElementType = "Edge";
                    break;
                case SMDSAbs_Face:
                    groupElementType = "Face";
                    break;
                case SMDSAbs_Volume:
                    groupElementType = "Volume";
                    break;
                case SMDSAbs_0DElement:
                    groupElementType = "0DElement";
                    break;
                case SMDSAbs_Ball:
                    groupElementType = "Ball";
                    break;
                default:
                    groupElementType = "Unknown";
                    break;
            }
            const char* groupName = myMesh->GetGroup(it)->GetName();
            anABAQUS_Output << "** GroupID: " << (it) << " --> GroupName: " << groupName
                            << " --> GroupElementType: " << groupElementType << std::endl;

            if (aElementType == SMDSAbs_Node) {
                anABAQUS_Output << "*NSET, NSET=" << groupName << std::endl;
            }
            else {
                anABAQUS_Output << "*ELSET, ELSET=" << groupName << std::endl;
            }

            // get and write group elements
            std::set<int> ids;
            SMDS_ElemIteratorPtr aElemIter = myMesh->GetGroup(it)->GetGroupDS()->GetElements();
            while (aElemIter->more()) {
                const SMDS_MeshElement* aElement = aElemIter->next();
                ids.insert(aElement->GetID());
            }
            for (int it : ids) {
                anABAQUS_Output << it << std::endl;
            }

            // write newline after each group
            anABAQUS_Output << std::endl;
        }
        anABAQUS_Output.close();
    }
}


void FemMesh::writeZ88(const std::string& FileName) const
{
    Base::TimeElapsed Start;
    Base::Console().Log("Start: FemMesh::writeZ88() =================================\n");

    /*
    Python command to export FemMesh from StartWB FEM 3D example:
    import feminout.importZ88Mesh
    feminout.importZ88Mesh.write(App.ActiveDocument.Box_Mesh.FemMesh, '/tmp/mesh.z88')
    */

    PyObject* module = PyImport_ImportModule("feminout.importZ88Mesh");
    if (!module) {
        return;
    }
    try {
        Py::Module z88mod(module, true);
        Py::Object mesh = Py::asObject(new FemMeshPy(const_cast<FemMesh*>(this)));
        Py::Callable method(z88mod.getAttr("write"));
        Py::Tuple args(2);
        args.setItem(0, mesh);
        args.setItem(1, Py::String(FileName));
        method.apply(args);
    }
    catch (Py::Exception& e) {
        e.clear();
    }
}


void FemMesh::write(const char* FileName) const
{
    Base::FileInfo File(FileName);

    if (File.hasExtension("unv")) {
        Base::Console().Log("FEM mesh object will be exported to unv format.\n");
        // write UNV file
        myMesh->ExportUNV(File.filePath().c_str());
    }
    else if (File.hasExtension("med")) {
        Base::Console().Log("FEM mesh object will be exported to med format.\n");
        myMesh->ExportMED(File.filePath().c_str(),
                          File.fileNamePure().c_str(),
                          false,
                          2);  // 2 means MED_V2_2 version!
    }
    else if (File.hasExtension("stl")) {
        Base::Console().Log("FEM mesh object will be exported to stl format.\n");
        // export to stl file
        myMesh->ExportSTL(File.filePath().c_str(), false);
    }
    else if (File.hasExtension("dat")) {
        Base::Console().Log("FEM mesh object will be exported to dat format.\n");
        // export to dat file
        myMesh->ExportDAT(File.filePath().c_str());
    }
    else if (File.hasExtension("inp")) {
        Base::Console().Log("FEM mesh object will be exported to inp format.\n");
        // get Abaqus inp prefs
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Fem/Abaqus");
        int elemParam = hGrp->GetInt("AbaqusElementChoice", 1);
        bool groupParam = hGrp->GetBool("AbaqusWriteGroups", false);
        // write ABAQUS Output
        writeABAQUS(File.filePath(), elemParam, groupParam);
    }
#ifdef FC_USE_VTK
    else if (File.hasExtension({"vtk", "vtu"})) {
        Base::Console().Log("FEM mesh object will be exported to either vtk or vtu format.\n");
        // write unstructure mesh to VTK format *.vtk and *.vtu
        FemVTKTools::writeVTKMesh(File.filePath().c_str(), this);
    }
#endif
    else if (File.hasExtension("z88")) {
        Base::Console().Log("FEM mesh object will be exported to z88 format.\n");
        // write z88 file
        writeZ88(File.filePath());
    }
    else {
        throw Base::FileException("An unknown file extension was added!");
    }
}

// ==== Base class implementer ==============================================================

unsigned int FemMesh::getMemSize() const
{
    return 0;
}

void FemMesh::Save(Base::Writer& writer) const
{
    if (!writer.isForceXML()) {
        // See SaveDocFile(), RestoreDocFile()
        writer.Stream() << writer.ind() << "<FemMesh file=\"";
        writer.Stream() << writer.addFile("FemMesh.unv", this) << "\"";
        writer.Stream() << " a11=\"" << _Mtrx[0][0] << "\" a12=\"" << _Mtrx[0][1] << "\" a13=\""
                        << _Mtrx[0][2] << "\" a14=\"" << _Mtrx[0][3] << "\"";
        writer.Stream() << " a21=\"" << _Mtrx[1][0] << "\" a22=\"" << _Mtrx[1][1] << "\" a23=\""
                        << _Mtrx[1][2] << "\" a24=\"" << _Mtrx[1][3] << "\"";
        writer.Stream() << " a31=\"" << _Mtrx[2][0] << "\" a32=\"" << _Mtrx[2][1] << "\" a33=\""
                        << _Mtrx[2][2] << "\" a34=\"" << _Mtrx[2][3] << "\"";
        writer.Stream() << " a41=\"" << _Mtrx[3][0] << "\" a42=\"" << _Mtrx[3][1] << "\" a43=\""
                        << _Mtrx[3][2] << "\" a44=\"" << _Mtrx[3][3] << "\"";
        writer.Stream() << "/>" << std::endl;
    }
    else {
        writer.Stream() << writer.ind() << "<FemMesh file=\"\"";
        writer.Stream() << " a11=\"" << _Mtrx[0][0] << "\" a12=\"" << _Mtrx[0][1] << "\" a13=\""
                        << _Mtrx[0][2] << "\" a14=\"" << _Mtrx[0][3] << "\"";
        writer.Stream() << " a21=\"" << _Mtrx[1][0] << "\" a22=\"" << _Mtrx[1][1] << "\" a23=\""
                        << _Mtrx[1][2] << "\" a24=\"" << _Mtrx[1][3] << "\"";
        writer.Stream() << " a31=\"" << _Mtrx[2][0] << "\" a32=\"" << _Mtrx[2][1] << "\" a33=\""
                        << _Mtrx[2][2] << "\" a34=\"" << _Mtrx[2][3] << "\"";
        writer.Stream() << " a41=\"" << _Mtrx[3][0] << "\" a42=\"" << _Mtrx[3][1] << "\" a43=\""
                        << _Mtrx[3][2] << "\" a44=\"" << _Mtrx[3][3] << "\"";
        writer.Stream() << "/>" << std::endl;
    }
}

void FemMesh::Restore(Base::XMLReader& reader)
{
    reader.readElement("FemMesh");
    std::string file(reader.getAttribute("file"));

    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(), this);
    }
    if (reader.hasAttribute("a11")) {
        _Mtrx[0][0] = reader.getAttributeAsFloat("a11");
        _Mtrx[0][1] = reader.getAttributeAsFloat("a12");
        _Mtrx[0][2] = reader.getAttributeAsFloat("a13");
        _Mtrx[0][3] = reader.getAttributeAsFloat("a14");

        _Mtrx[1][0] = reader.getAttributeAsFloat("a21");
        _Mtrx[1][1] = reader.getAttributeAsFloat("a22");
        _Mtrx[1][2] = reader.getAttributeAsFloat("a23");
        _Mtrx[1][3] = reader.getAttributeAsFloat("a24");

        _Mtrx[2][0] = reader.getAttributeAsFloat("a31");
        _Mtrx[2][1] = reader.getAttributeAsFloat("a32");
        _Mtrx[2][2] = reader.getAttributeAsFloat("a33");
        _Mtrx[2][3] = reader.getAttributeAsFloat("a34");

        _Mtrx[3][0] = reader.getAttributeAsFloat("a41");
        _Mtrx[3][1] = reader.getAttributeAsFloat("a42");
        _Mtrx[3][2] = reader.getAttributeAsFloat("a43");
        _Mtrx[3][3] = reader.getAttributeAsFloat("a44");
    }
}

void FemMesh::SaveDocFile(Base::Writer& writer) const
{
    // create a temporary file and copy the content to the zip stream
    Base::FileInfo fi(App::Application::getTempFileName().c_str());

    myMesh->ExportUNV(fi.filePath().c_str());

    Base::ifstream file(fi, std::ios::in | std::ios::binary);
    if (file) {
        std::streambuf* buf = file.rdbuf();
        writer.Stream() << buf;
    }

    file.close();
    // remove temp file
    fi.deleteFile();
}

void FemMesh::RestoreDocFile(Base::Reader& reader)
{
    // create a temporary file and copy the content from the zip stream
    Base::FileInfo fi(App::Application::getTempFileName().c_str());

    // read in the ASCII file and write back to the file stream
    Base::ofstream file(fi, std::ios::out | std::ios::binary);
    if (reader) {
        reader >> file.rdbuf();
    }
    file.close();

    // read the shape from the temp file
    myMesh->UNVToMesh(fi.filePath().c_str());

    // delete the temp file
    fi.deleteFile();
}

void FemMesh::transformGeometry(const Base::Matrix4D& rclTrf)
{
    // We perform a translation and rotation of the current active Mesh object
    Base::Matrix4D clMatrix(rclTrf);
    SMDS_NodeIteratorPtr aNodeIter = myMesh->GetMeshDS()->nodesIterator();
    Base::Vector3d current_node;
    for (; aNodeIter->more();) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        current_node.Set(aNode->X(), aNode->Y(), aNode->Z());
        current_node = clMatrix * current_node;
        myMesh->GetMeshDS()->MoveNode(aNode, current_node.x, current_node.y, current_node.z);
    }
}

void FemMesh::setTransform(const Base::Matrix4D& rclTrf)
{
    // Placement handling, no geometric transformation
    _Mtrx = rclTrf;
}

Base::Matrix4D FemMesh::getTransform() const
{
    return _Mtrx;
}

Base::BoundBox3d FemMesh::getBoundBox() const
{
    Base::BoundBox3d box;

    const SMESHDS_Mesh* data = getSMesh()->GetMeshDS();

    SMDS_NodeIteratorPtr aNodeIter = data->nodesIterator();
    for (; aNodeIter->more();) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        Base::Vector3d vec(aNode->X(), aNode->Y(), aNode->Z());
        // Apply the matrix to hold the BoundBox in absolute space.
        vec = _Mtrx * vec;
        box.Add(vec);
    }

    return box;
}

std::vector<const char*> FemMesh::getElementTypes() const
{
    std::vector<const char*> temp;
    temp.push_back("Vertex");
    temp.push_back("Edge");
    temp.push_back("Face");
    temp.push_back("Volume");

    return temp;
}

unsigned long FemMesh::countSubElements(const char* /*Type*/) const
{
    return 0;
}

Data::Segment* FemMesh::getSubElement(const char* /*Type*/, unsigned long /*n*/) const
{
    // FIXME implement subelement interface
    // std::stringstream str;
    // str << Type << n;
    // std::string temp = str.str();
    // return new ShapeSegment(getSubShape(temp.c_str()));
    return nullptr;
}

void FemMesh::getPoints(std::vector<Base::Vector3d>& Points,
                        std::vector<Base::Vector3d>& /*Normals*/,
                        double /*Accuracy*/,
                        uint16_t /*flags*/) const
{
    const SMESHDS_Mesh* data = getSMesh()->GetMeshDS();
    std::vector<Base::Vector3d> nodes;
    nodes.reserve(data->NbNodes());

    SMDS_NodeIteratorPtr aNodeIter = data->nodesIterator();
    for (; aNodeIter->more();) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        nodes.emplace_back(aNode->X(), aNode->Y(), aNode->Z());
    }

    Points = transformPointsToOutside(nodes);
}

struct Fem::FemMesh::FemMeshInfo FemMesh::getInfo() const
{

    struct FemMeshInfo rtrn;

    const SMESHDS_Mesh* data = getSMesh()->GetMeshDS();
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


Base::Quantity FemMesh::getVolume() const
{
    SMDS_VolumeIteratorPtr aVolIter = myMesh->GetMeshDS()->volumesIterator();

    // Calculate Mesh Volume
    // For an accurate Volume Calculation of a quadratic Tetrahedron
    // we have to calculate the Volume of 8 Sub-Tetrahedrons
    Base::Vector3d a, b, c, a_b_product;
    double volume = 0.0;

    for (; aVolIter->more();) {
        const SMDS_MeshVolume* aVol = aVolIter->next();

        if (aVol->NbNodes() != 10) {
            continue;
        }

        Base::Vector3d v1(aVol->GetNode(1)->X(), aVol->GetNode(1)->Y(), aVol->GetNode(1)->Z());
        Base::Vector3d v0(aVol->GetNode(0)->X(), aVol->GetNode(0)->Y(), aVol->GetNode(0)->Z());
        Base::Vector3d v2(aVol->GetNode(2)->X(), aVol->GetNode(2)->Y(), aVol->GetNode(2)->Z());
        Base::Vector3d v3(aVol->GetNode(3)->X(), aVol->GetNode(3)->Y(), aVol->GetNode(3)->Z());
        Base::Vector3d v4(aVol->GetNode(4)->X(), aVol->GetNode(4)->Y(), aVol->GetNode(4)->Z());
        Base::Vector3d v6(aVol->GetNode(6)->X(), aVol->GetNode(6)->Y(), aVol->GetNode(6)->Z());
        Base::Vector3d v5(aVol->GetNode(5)->X(), aVol->GetNode(5)->Y(), aVol->GetNode(5)->Z());
        Base::Vector3d v8(aVol->GetNode(8)->X(), aVol->GetNode(8)->Y(), aVol->GetNode(8)->Z());
        Base::Vector3d v7(aVol->GetNode(7)->X(), aVol->GetNode(7)->Y(), aVol->GetNode(7)->Z());
        Base::Vector3d v9(aVol->GetNode(9)->X(), aVol->GetNode(9)->Y(), aVol->GetNode(9)->Z());


        // 1,5,8,7
        a = v4 - v0;
        b = v7 - v0;
        c = v6 - v0;
        a_b_product.x = a.y * b.z - b.y * a.z;
        a_b_product.y = a.z * b.x - b.z * a.x;
        a_b_product.z = a.x * b.y - b.x * a.y;
        volume +=
            1.0 / 6.0 * fabs((a_b_product.x * c.x) + (a_b_product.y * c.y) + (a_b_product.z * c.z));
        // 5,9,8,7
        a = v8 - v4;
        b = v7 - v4;
        c = v6 - v4;
        a_b_product.x = a.y * b.z - b.y * a.z;
        a_b_product.y = a.z * b.x - b.z * a.x;
        a_b_product.z = a.x * b.y - b.x * a.y;
        volume +=
            1.0 / 6.0 * fabs((a_b_product.x * c.x) + (a_b_product.y * c.y) + (a_b_product.z * c.z));
        // 5,2,9,7
        a = v1 - v4;
        b = v8 - v4;
        c = v6 - v4;
        a_b_product.x = a.y * b.z - b.y * a.z;
        a_b_product.y = a.z * b.x - b.z * a.x;
        a_b_product.z = a.x * b.y - b.x * a.y;
        volume +=
            1.0 / 6.0 * fabs((a_b_product.x * c.x) + (a_b_product.y * c.y) + (a_b_product.z * c.z));
        // 2,6,9,7
        a = v5 - v1;
        b = v8 - v1;
        c = v6 - v1;
        a_b_product.x = a.y * b.z - b.y * a.z;
        a_b_product.y = a.z * b.x - b.z * a.x;
        a_b_product.z = a.x * b.y - b.x * a.y;
        volume +=
            1.0 / 6.0 * fabs((a_b_product.x * c.x) + (a_b_product.y * c.y) + (a_b_product.z * c.z));
        // 9,6,10,7
        a = v5 - v8;
        b = v9 - v8;
        c = v6 - v8;
        a_b_product.x = a.y * b.z - b.y * a.z;
        a_b_product.y = a.z * b.x - b.z * a.x;
        a_b_product.z = a.x * b.y - b.x * a.y;
        volume +=
            1.0 / 6.0 * fabs((a_b_product.x * c.x) + (a_b_product.y * c.y) + (a_b_product.z * c.z));
        // 6,3,10,7
        a = v2 - v5;
        b = v9 - v5;
        c = v6 - v5;
        a_b_product.x = a.y * b.z - b.y * a.z;
        a_b_product.y = a.z * b.x - b.z * a.x;
        a_b_product.z = a.x * b.y - b.x * a.y;
        volume +=
            1.0 / 6.0 * fabs((a_b_product.x * c.x) + (a_b_product.y * c.y) + (a_b_product.z * c.z));
        // 8,9,10,7
        a = v8 - v7;
        b = v9 - v7;
        c = v6 - v7;
        a_b_product.x = a.y * b.z - b.y * a.z;
        a_b_product.y = a.z * b.x - b.z * a.x;
        a_b_product.z = a.x * b.y - b.x * a.y;
        volume +=
            1.0 / 6.0 * fabs((a_b_product.x * c.x) + (a_b_product.y * c.y) + (a_b_product.z * c.z));
        // 8,9,10,4
        a = v8 - v7;
        b = v9 - v7;
        c = v3 - v7;
        a_b_product.x = a.y * b.z - b.y * a.z;
        a_b_product.y = a.z * b.x - b.z * a.x;
        a_b_product.z = a.x * b.y - b.x * a.y;
        volume +=
            1.0 / 6.0 * fabs((a_b_product.x * c.x) + (a_b_product.y * c.y) + (a_b_product.z * c.z));
    }

    return Base::Quantity(volume, Unit::Volume);
}

int FemMesh::addGroup(const std::string TypeString, const std::string Name, const int theId)
{
    // define mapping between typestring and ElementType
    // TODO: remove code doubling by providing mappings for all FemMesh functions
    using string_eltype_map = std::map<std::string, SMDSAbs_ElementType>;
    string_eltype_map mapping;
    mapping["All"] = SMDSAbs_All;
    mapping["Node"] = SMDSAbs_Node;
    mapping["Edge"] = SMDSAbs_Edge;
    mapping["Face"] = SMDSAbs_Face;
    mapping["Volume"] = SMDSAbs_Volume;
    mapping["0DElement"] = SMDSAbs_0DElement;
    mapping["Ball"] = SMDSAbs_Ball;

    int aId = theId;

    // check whether typestring is valid
    bool typeStringValid = false;
    for (string_eltype_map::const_iterator it = mapping.begin(); it != mapping.end(); ++it) {
        std::string key = it->first;
        if (key == TypeString) {
            typeStringValid = true;
        }
    }
    if (!typeStringValid) {
        throw std::runtime_error("AddGroup: Invalid type string! Allowed: All, Node, Edge, Face, "
                                 "Volume, 0DElement, Ball");
    }
    // add group to mesh
    SMESH_Group* group = this->getSMesh()->AddGroup(mapping[TypeString], Name.c_str(), aId);
    if (!group) {
        throw std::runtime_error("AddGroup: Failed to create new group.");
    }
#if SMESH_VERSION_MAJOR >= 9
    return group->GetID();
#else
    return aId;
#endif
}

void FemMesh::addGroupElements(int GroupId, const std::set<int>& ElementIds)
{
    SMESH_Group* group = this->getSMesh()->GetGroup(GroupId);
    if (!group) {
        throw std::runtime_error("AddGroupElements: No group for given id.");
    }
    SMESHDS_Group* groupDS = dynamic_cast<SMESHDS_Group*>(group->GetGroupDS());
    if (!groupDS) {
        throw std::runtime_error("addGroupElements: Failed to add group elements.");
    }

    // Traverse the full mesh and add elements to group if id is in set 'ids'
    // and if group type is compatible with element
    SMDSAbs_ElementType aElementType = groupDS->GetType();

    SMDS_ElemIteratorPtr aElemIter = this->getSMesh()->GetMeshDS()->elementsIterator(aElementType);
    while (aElemIter->more()) {
        const SMDS_MeshElement* aElem = aElemIter->next();
        std::set<int>::iterator it;
        it = ElementIds.find(aElem->GetID());
        if (it != ElementIds.end()) {
            // the element was in the list
            if (!groupDS->Contains(aElem)) {  // check whether element is already in group
                groupDS->Add(aElem);          // if not, add it
            }
        }
    }
}

bool FemMesh::removeGroup(int GroupId)
{
    return this->getSMesh()->RemoveGroup(GroupId);
}
