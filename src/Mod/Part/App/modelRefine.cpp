/***************************************************************************
 *   Copyright (c) 2011 Thomas Anderson <blobfish[at]gmx.com>              *
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
#include <algorithm>
#include <Geom_Surface.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <gp_Pln.hxx>
#include <gp_Cylinder.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <ShapeAnalysis_Edge.hxx>
#include <BRep_Tool.hxx>
#include <BRepLib_MakeWire.hxx>
#include <BRepLib_FuseEdges.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <ShapeBuild_ReShape.hxx>
#include <ShapeFix_Face.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include "modelRefine.h"

using namespace ModelRefine;



//this following struct and function was stole from freecad AppPartPy.cpp
namespace ModelRefine {
struct EdgePoints {
    gp_Pnt v1, v2;
    TopoDS_Edge edge;
};
static std::list<TopoDS_Edge> sort_Edges(double tol3d, const std::vector<TopoDS_Edge>& edges)
{
    tol3d = tol3d * tol3d;
    std::list<EdgePoints>  edge_points;
    TopExp_Explorer xp;
    for (std::vector<TopoDS_Edge>::const_iterator it = edges.begin(); it != edges.end(); ++it) {
        EdgePoints ep;
        xp.Init(*it,TopAbs_VERTEX);
        ep.v1 = BRep_Tool::Pnt(TopoDS::Vertex(xp.Current()));
        xp.Next();
        ep.v2 = BRep_Tool::Pnt(TopoDS::Vertex(xp.Current()));
        ep.edge = *it;
        edge_points.push_back(ep);
    }

    if (edge_points.empty())
        return std::list<TopoDS_Edge>();

    std::list<TopoDS_Edge> sorted;
    gp_Pnt first, last;
    first = edge_points.front().v1;
    last  = edge_points.front().v2;

    sorted.push_back(edge_points.front().edge);
    edge_points.erase(edge_points.begin());

    while (!edge_points.empty()) {
        // search for adjacent edge
        std::list<EdgePoints>::iterator pEI;
        for (pEI = edge_points.begin(); pEI != edge_points.end(); ++pEI) {
            if (pEI->v1.SquareDistance(last) <= tol3d) {
                last = pEI->v2;
                sorted.push_back(pEI->edge);
                edge_points.erase(pEI);
                break;
            }
            else if (pEI->v2.SquareDistance(first) <= tol3d) {
                first = pEI->v1;
                sorted.push_front(pEI->edge);
                edge_points.erase(pEI);
                break;
            }
            else if (pEI->v2.SquareDistance(last) <= tol3d) {
                last = pEI->v1;
                sorted.push_back(pEI->edge);
                edge_points.erase(pEI);
                break;
            }
            else if (pEI->v1.SquareDistance(first) <= tol3d) {
                first = pEI->v2;
                sorted.push_front(pEI->edge);
                edge_points.erase(pEI);
                break;
            }
        }

        if ((pEI == edge_points.end()) || (last.SquareDistance(first) <= tol3d)) {
            // no adjacent edge found or polyline is closed
            return sorted;
        }
    }
    return sorted;
}
}
//end stolen freecad.


void ModelRefine::getFaceEdges(const TopoDS_Face &face, EdgeVectorType &edges)
{
    TopExp_Explorer it;
    for (it.Init(face, TopAbs_EDGE); it.More(); it.Next())
        edges.push_back(TopoDS::Edge(it.Current()));
}

void ModelRefine::boundaryEdges(const FaceVectorType &faces, EdgeVectorType &edgesOut)
{
    //this finds all the boundary edges. Maybe more than one boundary.
    std::list<TopoDS_Edge> edges;
    FaceVectorType::const_iterator faceIt;
    for (faceIt = faces.begin(); faceIt != faces.end(); ++faceIt)
    {
        EdgeVectorType faceEdges;
        EdgeVectorType::iterator faceEdgesIt;
        getFaceEdges(*faceIt, faceEdges);
        for (faceEdgesIt = faceEdges.begin(); faceEdgesIt != faceEdges.end(); ++faceEdgesIt)
        {
            bool foundSignal(false);
            std::list<TopoDS_Edge>::iterator edgesIt;
            for (edgesIt = edges.begin(); edgesIt != edges.end(); ++edgesIt)
            {
                if ((*edgesIt).IsSame(*faceEdgesIt))
                {
                    edgesIt = edges.erase(edgesIt);
                    foundSignal = true;
                    break;
                }
            }
            if (!foundSignal)
                edges.push_back(*faceEdgesIt);
        }
    }

    edgesOut.reserve(edges.size());
    std::copy(edges.begin(), edges.end(), back_inserter(edgesOut));
}

TopoDS_Shell ModelRefine::removeFaces(const TopoDS_Shell &shell, const FaceVectorType &faces)
{
    ShapeBuild_ReShape rebuilder;
    FaceVectorType::const_iterator it;
    for (it = faces.begin(); it != faces.end(); ++it)
        rebuilder.Remove(*it);
    return TopoDS::Shell(rebuilder.Apply(shell));
}

void BoundaryEdgeSplitter::split(const EdgeVectorType &edgesIn)
{
    buildMap(edgesIn);
    EdgeVectorType::const_iterator workIt;
    for (workIt = edgesIn.begin(); workIt != edgesIn.end(); ++workIt)
    {
        TopoDS_Edge current = *workIt;

        if (processed.Contains(*workIt))
            continue;

        EdgeVectorType temp;
        temp.reserve(edgesIn.size() + 1);
        temp.push_back(current);
        //recursive call
        splitRecursive(temp, edgesIn);
        groupedEdges.push_back(temp);
    }
}

void BoundaryEdgeSplitter::splitRecursive(EdgeVectorType &tempEdges, const EdgeVectorType &workEdges)
{
    EdgeVectorType::iterator tempIt;
    EdgeVectorType::const_iterator workIt;
    for (tempIt = tempEdges.begin(); tempIt != tempEdges.end(); ++tempIt)
    {
        for (workIt = workEdges.begin(); workIt != workEdges.end(); ++workIt)
        {
            if ((*tempIt).IsSame(*workIt))
                continue;
            if (processed.Contains(*workIt))
                continue;
            if (edgeTest(*tempIt, *workIt))
            {
                tempEdges.push_back(*workIt);
                processed.Add(*workIt);
                splitRecursive(tempEdges, workEdges);
            }
        }
    }
}

void BoundaryEdgeSplitter::buildMap(const EdgeVectorType &edgesIn)
{
    EdgeVectorType::const_iterator vit;
    for (vit = edgesIn.begin(); vit != edgesIn.end(); ++vit)
    {
        TopTools_ListOfShape shapeList;
        TopExp_Explorer it;
        for (it.Init(*vit, TopAbs_VERTEX); it.More(); it.Next())
            shapeList.Append(it.Current());
        edgeVertexMap.Bind((*vit), shapeList);
    }
}

bool BoundaryEdgeSplitter::edgeTest(const TopoDS_Edge &edgeOne, const TopoDS_Edge &edgeTwo)
{
    const TopTools_ListOfShape &verticesOne = edgeVertexMap.Find(edgeOne);
    const TopTools_ListOfShape &verticesTwo = edgeVertexMap.Find(edgeTwo);
    TopTools_ListIteratorOfListOfShape itOne;
    TopTools_ListIteratorOfListOfShape itTwo;

    for (itOne.Initialize(verticesOne); itOne.More(); itOne.Next())
    {
        for (itTwo.Initialize(verticesTwo); itTwo.More(); itTwo.Next())
        {
            if (itOne.Value().IsSame(itTwo.Value()))
                return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////

void FaceTypeSplitter::addShell(const TopoDS_Shell &shellIn)
{
    shell = shellIn;
}

void FaceTypeSplitter::registerType(const GeomAbs_SurfaceType &type)
{
    typeMap.insert(SplitMapType::value_type(type, FaceVectorType()));
}

bool FaceTypeSplitter::hasType(const GeomAbs_SurfaceType &type) const
{
    return typeMap.find(type) != typeMap.end();
}

void FaceTypeSplitter::split()
{
    TopExp_Explorer shellIt;
    for (shellIt.Init(shell, TopAbs_FACE); shellIt.More(); shellIt.Next())
    {
        TopoDS_Face tempFace(TopoDS::Face(shellIt.Current()));
        GeomAbs_SurfaceType currentType = FaceTypedBase::getFaceType(tempFace);
        SplitMapType::iterator mapIt = typeMap.find(currentType);
        if (mapIt == typeMap.end())
            continue;
        (*mapIt).second.push_back(tempFace);
    }
}

const FaceVectorType& FaceTypeSplitter::getTypedFaceVector(const GeomAbs_SurfaceType &type) const
{
    if (this->hasType(type))
        return (*(typeMap.find(type))).second;
    //error here.
    static FaceVectorType error;
    return error;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void FaceAdjacencySplitter::split(const FaceVectorType &facesIn)
{
    //the reserve call guarantees the vector will never get "pushed back" in the
    //recursiveFind calls, thus invalidating the iterators. We can be sure of this as any one
    //matched set can't be bigger than the set passed in. if we have seg faults, we will
    //want to turn this tempFaces vector back into a std::list ensuring valid iterators
    //at the expense of std::find speed.
    buildMap(facesIn);
    FaceVectorType tempFaces;
    tempFaces.reserve(facesIn.size() + 1);

    FaceVectorType::const_iterator it;
    for (it = facesIn.begin(); it != facesIn.end(); ++it)
    {
        //skip already processed shapes.
        if (processedMap.Contains(*it))
            continue;

        tempFaces.clear();
        tempFaces.push_back(*it);
        processedMap.Add(*it);
        recursiveFind(tempFaces, facesIn);
        if (tempFaces.size() > 1)
        {
            adjacencyArray.push_back(tempFaces);
        }
    }
}

void FaceAdjacencySplitter::recursiveFind(FaceVectorType &tempFaces, const FaceVectorType &facesIn)
{
    FaceVectorType::iterator tempIt;
    FaceVectorType::const_iterator faceIt;

    for (tempIt = tempFaces.begin(); tempIt != tempFaces.end(); ++tempIt)
    {
        for(faceIt = facesIn.begin(); faceIt != facesIn.end(); ++faceIt)
        {
            if ((*tempIt).IsSame(*faceIt))
                continue;
            if (processedMap.Contains(*faceIt))
                continue;
            if (adjacentTest(*tempIt, *faceIt))
            {
                tempFaces.push_back(*faceIt);
                processedMap.Add(*faceIt);
                recursiveFind(tempFaces, facesIn);
            }
        }
    }
}

bool FaceAdjacencySplitter::hasBeenMapped(const TopoDS_Face &shape)
{
    for (std::vector<FaceVectorType>::iterator it(adjacencyArray.begin()); it != adjacencyArray.end(); ++it)
    {
        if (std::find((*it).begin(), (*it).end(), shape) != (*it).end())
            return true;
    }
    return false;
}

void FaceAdjacencySplitter::buildMap(const FaceVectorType &facesIn)
{
    FaceVectorType::const_iterator vit;
    for (vit = facesIn.begin(); vit != facesIn.end(); ++vit)
    {
        TopTools_ListOfShape shapeList;
        TopExp_Explorer it;
        for (it.Init(*vit, TopAbs_EDGE); it.More(); it.Next())
            shapeList.Append(it.Current());
        faceEdgeMap.Bind((*vit), shapeList);
    }
}

bool FaceAdjacencySplitter::adjacentTest(const TopoDS_Face &faceOne, const TopoDS_Face &faceTwo)
{
    const TopTools_ListOfShape &faceOneEdges = faceEdgeMap.Find(faceOne);
    const TopTools_ListOfShape &faceTwoEdges = faceEdgeMap.Find(faceTwo);
    TopTools_ListIteratorOfListOfShape itOne, itTwo;

    for (itOne.Initialize(faceOneEdges); itOne.More(); itOne.Next())
    {
        for (itTwo.Initialize(faceTwoEdges); itTwo.More(); itTwo.Next())
        {
            if ((itOne.Value()).IsSame(itTwo.Value()))
                return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

void FaceEqualitySplitter::split(const FaceVectorType &faces, FaceTypedBase *object)
{
    std::vector<FaceVectorType> tempVector;
    tempVector.reserve(faces.size());
    FaceVectorType::const_iterator faceIt;
    for (faceIt = faces.begin(); faceIt != faces.end(); ++faceIt)
    {
        bool foundMatch(false);
        std::vector<FaceVectorType>::iterator tempIt;
        for (tempIt = tempVector.begin(); tempIt != tempVector.end(); ++tempIt)
        {
            if (object->isEqual((*tempIt).front(), *faceIt))
            {
                (*tempIt).push_back(*faceIt);
                foundMatch = true;
                break;
            }
        }
        if (!foundMatch)
        {
            FaceVectorType another;
            another.reserve(faces.size());
            another.push_back(*faceIt);
            tempVector.push_back(another);
        }
    }
    std::vector<FaceVectorType>::iterator it;
    for (it = tempVector.begin(); it != tempVector.end(); ++it)
    {
        if ((*it).size() < 2)
            continue;
        equalityVector.push_back(*it);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GeomAbs_SurfaceType FaceTypedBase::getFaceType(const TopoDS_Face &faceIn)
{
    Handle(Geom_Surface) surface = BRep_Tool::Surface(faceIn);
    GeomAdaptor_Surface surfaceTest(surface);
    return surfaceTest.GetType();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

FaceTypedPlane::FaceTypedPlane() : FaceTypedBase(GeomAbs_Plane)
{
}

bool FaceTypedPlane::isEqual(const TopoDS_Face &faceOne, const TopoDS_Face &faceTwo) const
{
    Handle(Geom_Plane) planeSurfaceOne = Handle(Geom_Plane)::DownCast(BRep_Tool::Surface(faceOne));
    Handle(Geom_Plane) planeSurfaceTwo = Handle(Geom_Plane)::DownCast(BRep_Tool::Surface(faceTwo));
    if (planeSurfaceOne.IsNull() || planeSurfaceTwo.IsNull())
        return false;//error?
    gp_Pln planeOne(planeSurfaceOne->Pln());
    gp_Pln planeTwo(planeSurfaceTwo->Pln());
    return (planeOne.Position().Direction().IsParallel(planeTwo.Position().Direction(), Precision::Confusion()) &&
            planeOne.Distance(planeTwo.Position().Location()) < Precision::Confusion());
}

GeomAbs_SurfaceType FaceTypedPlane::getType() const
{
    return GeomAbs_Plane;
}

TopoDS_Face FaceTypedPlane::buildFace(const FaceVectorType &faces) const
{
    EdgeVectorType bEdges;
    boundaryEdges(faces, bEdges);

    BoundaryEdgeSplitter bSplitter;
    bSplitter.split(bEdges);


    //parallel vectors. Topo* doesn't have less than. map wouldn't work.
    FaceVectorType  facesParallel;
    std::vector<TopoDS_Wire> wiresParallel;

    std::vector<EdgeVectorType> splitEdges = bSplitter.getGroupedEdges();
    std::vector<EdgeVectorType>::iterator splitIt;
    for (splitIt = splitEdges.begin(); splitIt != splitEdges.end(); ++splitIt)
    {
        std::list<TopoDS_Edge> sortedEdges;
        sortedEdges = sort_Edges(Precision::Confusion(), *splitIt);

        BRepLib_MakeWire wireMaker;
        std::list<TopoDS_Edge>::iterator sortedIt;
        for (sortedIt = sortedEdges.begin(); sortedIt != sortedEdges.end(); ++sortedIt)
            wireMaker.Add(*sortedIt);
        TopoDS_Wire currentWire = wireMaker.Wire();

        TopoDS_Face currentFace = BRepBuilderAPI_MakeFace(currentWire, Standard_True);

        facesParallel.push_back(currentFace);
        wiresParallel.push_back(currentWire);
    }
    if (facesParallel.size() < 1)//shouldn't be here.
        return BRepBuilderAPI_MakeFace();//will cause exception.
    if (facesParallel.size() == 1)
        return (facesParallel.front());

    TopoDS_Face current;
    current = facesParallel.at(0);
    //now we have more than one wire.
    //there has to be a better way to determin which face is inside other
    //without have to build all the faces.
    for(size_t index(1); index<facesParallel.size(); ++index)
    {
        //this algorithm assumes that the boundaries don't intersect.
        gp_Pnt point;
        Handle(Geom_Surface) surface = BRep_Tool::Surface(facesParallel.at(index));
        surface->D0(0.5, 0.5, point);
        BRepClass_FaceClassifier faceTest(current, point, Precision::Confusion());
        if (faceTest.State() == TopAbs_EXTERNAL)
            current = facesParallel.at(index);
    }

    ShapeFix_Face faceFix(current);
    faceFix.SetContext(new ShapeBuild_ReShape());
    for (size_t index(0); index<facesParallel.size(); ++index)
    {
        if (current.IsSame(facesParallel.at(index)))
            continue;
        faceFix.Add(wiresParallel.at(index));
    }
    faceFix.FixOrientation();
    if (faceFix.Perform())
        return faceFix.Face();
    else
        return TopoDS_Face();
}

FaceTypedPlane& ModelRefine::getPlaneObject()
{
    static FaceTypedPlane object;
    return object;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////

FaceTypedCylinder::FaceTypedCylinder() : FaceTypedBase(GeomAbs_Cylinder)
{
}

bool FaceTypedCylinder::isEqual(const TopoDS_Face &faceOne, const TopoDS_Face &faceTwo) const
{
    //check if these handles are valid?
    Handle(Geom_CylindricalSurface) surfaceOne = Handle(Geom_CylindricalSurface)::DownCast(BRep_Tool::Surface(faceOne));
    Handle(Geom_CylindricalSurface) surfaceTwo = Handle(Geom_CylindricalSurface)::DownCast(BRep_Tool::Surface(faceTwo));
    if (surfaceOne.IsNull() || surfaceTwo.IsNull())
        return false;//probably need an error
    gp_Cylinder cylinderOne = surfaceOne->Cylinder();
    gp_Cylinder cylinderTwo = surfaceTwo->Cylinder();

    if (cylinderOne.Radius() != cylinderTwo.Radius())
        return false;
    if (!cylinderOne.Axis().IsCoaxial(cylinderTwo.Axis(), Precision::Confusion(), Precision::Confusion()))
        return false;

    return true;
}

GeomAbs_SurfaceType FaceTypedCylinder::getType() const
{
    return GeomAbs_Cylinder;
}

TopoDS_Face FaceTypedCylinder::buildFace(const FaceVectorType &faces) const
{
    //to do.
    return TopoDS_Face();
}

FaceTypedCylinder& ModelRefine::getCylinderObject()
{
    static FaceTypedCylinder object;
    return object;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

FaceUniter::FaceUniter(const TopoDS_Shell &shellIn)
{
    workShell = shellIn;
}

FaceUniter::FaceUniter(const TopoDS_Solid &solidIn)
{
    //get first shell
    TopExp_Explorer it;
    it.Init(solidIn, TopAbs_SHELL);
    workShell = TopoDS::Shell(it.Current());
}

bool FaceUniter::process()
{
    if (workShell.IsNull())
        return false;
    typeObjects.push_back(&getPlaneObject());
//    typeObjects.push_back(&getCylinderObject());
    //add more face types.

    ModelRefine::FaceTypeSplitter splitter;
    splitter.addShell(workShell);
    std::vector<FaceTypedBase *>::iterator typeIt;
    for(typeIt = typeObjects.begin(); typeIt != typeObjects.end(); ++typeIt)
        splitter.registerType((*typeIt)->getType());
    splitter.split();

    ModelRefine::FaceVectorType facesToRemove;
    ModelRefine::FaceVectorType facesToSew;

    for(typeIt = typeObjects.begin(); typeIt != typeObjects.end(); ++typeIt)
    {
        ModelRefine::FaceVectorType typedFaces = splitter.getTypedFaceVector((*typeIt)->getType());
        ModelRefine::FaceEqualitySplitter equalitySplitter;
        equalitySplitter.split(typedFaces, *typeIt);
        for (std::size_t indexEquality(0); indexEquality < equalitySplitter.getGroupCount(); ++indexEquality)
        {
            ModelRefine::FaceAdjacencySplitter adjacencySplitter;
            adjacencySplitter.split(equalitySplitter.getGroup(indexEquality));
//            std::cout << "      adjacency group count: " << adjacencySplitter.getGroupCount() << std::endl;
            for (std::size_t adjacentIndex(0); adjacentIndex < adjacencySplitter.getGroupCount(); ++adjacentIndex)
            {
//                    std::cout << "         face count is: " << adjacencySplitter.getGroup(adjacentIndex).size() << std::endl;
                TopoDS_Face newFace = (*typeIt)->buildFace(adjacencySplitter.getGroup(adjacentIndex));
                if (!newFace.IsNull())
                {
                    facesToSew.push_back(newFace);
                    if (facesToRemove.capacity() <= facesToRemove.size() + adjacencySplitter.getGroup(adjacentIndex).size())
                        facesToRemove.reserve(facesToRemove.size() + adjacencySplitter.getGroup(adjacentIndex).size());
                    FaceVectorType temp = adjacencySplitter.getGroup(adjacentIndex);
                    facesToRemove.insert(facesToRemove.end(), temp.begin(), temp.end());
                }
            }
        }
    }
    if (facesToSew.size() > 0)
    {
        workShell = ModelRefine::removeFaces(workShell, facesToRemove);
        TopExp_Explorer xp;
        bool emptyShell = true;
        for (xp.Init(workShell, TopAbs_FACE); xp.More(); xp.Next())
        {
            emptyShell = false;
            break;
        }

        if (!emptyShell || facesToSew.size() > 1)
        {
            BRepBuilderAPI_Sewing sew;
            sew.Add(workShell);
            FaceVectorType::iterator sewIt;
            for(sewIt = facesToSew.begin(); sewIt != facesToSew.end(); ++sewIt)
                sew.Add(*sewIt);
            sew.Perform();
            workShell = TopoDS::Shell(sew.SewedShape());
        }
        else
        {
            // workShell has no more faces and we add exactly one face
            BRep_Builder builder;
            builder.MakeShell(workShell);
            FaceVectorType::iterator sewIt;
            for(sewIt = facesToSew.begin(); sewIt != facesToSew.end(); ++sewIt)
                builder.Add(workShell, *sewIt);
        }

        BRepLib_FuseEdges edgeFuse(workShell, true);
        workShell = TopoDS::Shell(edgeFuse.Shape());
    }
    return true;
}

bool FaceUniter::getSolid(TopoDS_Solid &outSolid) const
{
    BRepBuilderAPI_MakeSolid solidMaker;
    solidMaker.Add(workShell);
    outSolid = solidMaker.Solid();
    return solidMaker.IsDone() ? true : false;
}
