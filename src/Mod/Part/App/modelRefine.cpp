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
#include <iterator>
#include <Geom_Surface.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <gp_Pln.hxx>
#include <gp_Cylinder.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <BRep_Tool.hxx>
#include <BRepLib_MakeWire.hxx>
#include <BRepLib_FuseEdges.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <ShapeBuild_ReShape.hxx>
#include <ShapeFix_Face.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_DataMapIteratorOfDataMapOfShapeShape.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <ShapeAnalysis_Edge.hxx>
#include "modelRefine.h"

using namespace ModelRefine;

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

namespace ModelRefine
{
    class WireSort
    {
    public:
        bool operator() (const TopoDS_Wire& wire1, const TopoDS_Wire& wire2)
        {
            Bnd_Box box1, box2;
            BRepBndLib::Add(wire1, box1);
            BRepBndLib::Add(wire2, box2);
            return box2.SquareExtent() < box1.SquareExtent();
        }
    };
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

FaceAdjacencySplitter::FaceAdjacencySplitter(const TopoDS_Shell &shell)
{
    TopExp_Explorer shellIt;
    for (shellIt.Init(shell, TopAbs_FACE); shellIt.More(); shellIt.Next())
    {
        TopTools_ListOfShape shapeList;
        TopExp_Explorer it;
        for (it.Init(shellIt.Current(), TopAbs_EDGE); it.More(); it.Next())
            shapeList.Append(it.Current());
        faceToEdgeMap.Add(shellIt.Current(), shapeList);
    }
    TopExp::MapShapesAndAncestors(shell, TopAbs_EDGE, TopAbs_FACE, edgeToFaceMap);
}


void FaceAdjacencySplitter::split(const FaceVectorType &facesIn)
{
    facesInMap.Clear();
    processedMap.Clear();
    adjacencyArray.clear();

    FaceVectorType::const_iterator it;
    for (it = facesIn.begin(); it != facesIn.end(); ++it)
        facesInMap.Add(*it);
    //the reserve call guarantees the vector will never get "pushed back" in the
    //recursiveFind calls, thus invalidating the iterators. We can be sure of this as any one
    //matched set can't be bigger than the set passed in. if we have seg faults, we will
    //want to turn this tempFaces vector back into a std::list ensuring valid iterators
    //at the expense of std::find speed.
    FaceVectorType tempFaces;
    tempFaces.reserve(facesIn.size() + 1);

    for (it = facesIn.begin(); it != facesIn.end(); ++it)
    {
        //skip already processed shapes.
        if (processedMap.Contains(*it))
            continue;

        tempFaces.clear();
        processedMap.Add(*it);
        recursiveFind(*it, tempFaces);
        if (tempFaces.size() > 1)
        {
            adjacencyArray.push_back(tempFaces);
        }
    }
}

void FaceAdjacencySplitter::recursiveFind(const TopoDS_Face &face, FaceVectorType &outVector)
{
    outVector.push_back(face);

    const TopTools_ListOfShape &edges = faceToEdgeMap.FindFromKey(face);
    TopTools_ListIteratorOfListOfShape edgeIt;
    for (edgeIt.Initialize(edges); edgeIt.More(); edgeIt.Next())
    {
        //don't try to join across seams.
        ShapeAnalysis_Edge edgeCheck;
        if(edgeCheck.IsSeam(TopoDS::Edge(edgeIt.Value()), face))
            continue;

        const TopTools_ListOfShape &faces = edgeToFaceMap.FindFromKey(edgeIt.Value());
        TopTools_ListIteratorOfListOfShape faceIt;
        for (faceIt.Initialize(faces); faceIt.More(); faceIt.Next())
        {
            if (!facesInMap.Contains(faceIt.Value()))
                continue;
            if (processedMap.Contains(faceIt.Value()))
                continue;
            processedMap.Add(faceIt.Value());
            recursiveFind(TopoDS::Face(faceIt.Value()), outVector);
        }
    }
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

void FaceTypedBase::boundarySplit(const FaceVectorType &facesIn, std::vector<EdgeVectorType> &boundariesOut) const
{
    EdgeVectorType bEdges;
    boundaryEdges(facesIn, bEdges);

    std::list<TopoDS_Edge> edges;
    std::copy(bEdges.begin(), bEdges.end(), back_inserter(edges));
    while(!edges.empty())
    {
        TopoDS_Vertex destination = TopExp::FirstVertex(edges.front(), Standard_True);
        TopoDS_Vertex lastVertex = TopExp::LastVertex(edges.front(), Standard_True);
        EdgeVectorType boundary;
        boundary.push_back(edges.front());
        edges.pop_front();
        //single edge closed check.
        if (destination.IsSame(lastVertex))
        {
            boundariesOut.push_back(boundary);
            continue;
        }

        bool closedSignal(false);
        std::list<TopoDS_Edge>::iterator it;
        for (it = edges.begin(); it != edges.end();)
        {
            TopoDS_Vertex currentVertex = TopExp::FirstVertex(*it, Standard_True);
            if (lastVertex.IsSame(currentVertex))
            {
                boundary.push_back(*it);
                lastVertex = TopExp::LastVertex(*it, Standard_True);
                edges.erase(it);
                it = edges.begin();
                if (lastVertex.IsSame(destination))
                {
                    closedSignal = true;
                    break;
                }
                continue;
            }
            ++it;
        }
        if (closedSignal)
            boundariesOut.push_back(boundary);
    }
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
    std::vector<TopoDS_Wire> wires;

    std::vector<EdgeVectorType> splitEdges;
    this->boundarySplit(faces, splitEdges);
    if (splitEdges.empty())
        return TopoDS_Face();
    std::vector<EdgeVectorType>::iterator splitIt;
    for (splitIt = splitEdges.begin(); splitIt != splitEdges.end(); ++splitIt)
    {
        BRepLib_MakeWire wireMaker;
        EdgeVectorType::iterator it;
        for (it = (*splitIt).begin(); it != (*splitIt).end(); ++it)
            wireMaker.Add(*it);
        TopoDS_Wire currentWire = wireMaker.Wire();
        wires.push_back(currentWire);
    }

    std::sort(wires.begin(), wires.end(), ModelRefine::WireSort());

    BRepLib_MakeFace faceMaker(wires.at(0), Standard_True);
    if (faceMaker.Error() != BRepLib_FaceDone)
        return TopoDS_Face();
    TopoDS_Face current = faceMaker.Face();
    if (wires.size() > 1)
    {
        ShapeFix_Face faceFix(current);
        faceFix.SetContext(new ShapeBuild_ReShape());
        for (size_t index(1); index<wires.size(); ++index)
            faceFix.Add(wires.at(index));
        faceFix.Perform();
        if (faceFix.Status(ShapeExtend_FAIL))
            return TopoDS_Face();
        faceFix.FixOrientation();
        faceFix.Perform();
        if(faceFix.Status(ShapeExtend_FAIL))
            return TopoDS_Face();
        current = faceFix.Face();
    }

    return current;
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
    static TopoDS_Face dummy;
    std::vector<EdgeVectorType> boundaries;
    boundarySplit(faces, boundaries);
    if (boundaries.size() < 1)
        return dummy;

    //make wires
    std::vector<TopoDS_Wire> wires;
    std::vector<EdgeVectorType>::iterator boundaryIt;
    for (boundaryIt = boundaries.begin(); boundaryIt != boundaries.end(); ++boundaryIt)
    {
        BRepLib_MakeWire wireMaker;
        EdgeVectorType::iterator it;
        for (it = (*boundaryIt).begin(); it != (*boundaryIt).end(); ++it)
            wireMaker.Add(*it);
        if (wireMaker.Error() != BRepLib_WireDone)
            return dummy;
        wires.push_back(wireMaker.Wire());
    }
    if (wires.size() < 1)
        return dummy;
    std::sort(wires.begin(), wires.end(), ModelRefine::WireSort());

    //make face from surface and outer wire.
    Handle(Geom_CylindricalSurface) surface = Handle(Geom_CylindricalSurface)::DownCast(BRep_Tool::Surface(faces.at(0)));
    std::vector<TopoDS_Wire>::iterator wireIt;
    wireIt = wires.begin();
    BRepBuilderAPI_MakeFace faceMaker(surface, *wireIt);
    if (!faceMaker.IsDone())
        return dummy;

    //add additional boundaries.
    for (wireIt++; wireIt != wires.end(); ++wireIt)
    {
        faceMaker.Add(*wireIt);
        if (!faceMaker.IsDone())
            return dummy;
    }

    //fix newly constructed face. Orientation doesn't seem to get fixed the first call.
    ShapeFix_Face faceFixer(faceMaker.Face());
    faceFixer.SetContext(new ShapeBuild_ReShape());
    faceFixer.Perform();
    if (faceFixer.Status(ShapeExtend_FAIL))
        return dummy;
    faceFixer.FixOrientation();
    faceFixer.Perform();
    if (faceFixer.Status(ShapeExtend_FAIL))
        return dummy;

    return faceFixer.Face();
}

void FaceTypedCylinder::boundarySplit(const FaceVectorType &facesIn, std::vector<EdgeVectorType> &boundariesOut) const
{
    //get all the seam edges
    EdgeVectorType seamEdges;
    FaceVectorType::const_iterator faceIt;
    for (faceIt = facesIn.begin(); faceIt != facesIn.end(); ++faceIt)
    {
        TopExp_Explorer explorer;
        for (explorer.Init(*faceIt, TopAbs_EDGE); explorer.More(); explorer.Next())
        {
            ShapeAnalysis_Edge edgeCheck;
            if(edgeCheck.IsSeam(TopoDS::Edge(explorer.Current()), *faceIt))
                seamEdges.push_back(TopoDS::Edge(explorer.Current()));
        }
    }

    //normal edges.
    EdgeVectorType normalEdges;
    ModelRefine::boundaryEdges(facesIn, normalEdges);

    //put seam edges in front.the idea is that we always want to traverse along a seam edge if possible.
    std::list<TopoDS_Edge> sortedEdges;
    std::copy(normalEdges.begin(), normalEdges.end(), back_inserter(sortedEdges));
    std::copy(seamEdges.begin(), seamEdges.end(), front_inserter(sortedEdges));

    while (!sortedEdges.empty())
    {
        //detecting closed boundary works best if we start off of the seam edges.
        TopoDS_Vertex destination = TopExp::FirstVertex(sortedEdges.back(), Standard_True);
        TopoDS_Vertex lastVertex = TopExp::LastVertex(sortedEdges.back(), Standard_True);
        bool closedSignal(false);
        std::list<TopoDS_Edge> boundary;
        boundary.push_back(sortedEdges.back());
        sortedEdges.pop_back();

        std::list<TopoDS_Edge>::iterator sortedIt;
        for (sortedIt = sortedEdges.begin(); sortedIt != sortedEdges.end();)
        {
            TopoDS_Vertex currentVertex = TopExp::FirstVertex(*sortedIt, Standard_True);

            //Seam edges lie on top of each other. i.e. same. and we remove every match from the list
            //so we don't actually ever compare the same edge.
            if ((*sortedIt).IsSame(boundary.back()))
            {
                ++sortedIt;
                continue;
            }
            if (lastVertex.IsSame(currentVertex))
            {
                boundary.push_back(*sortedIt);
                lastVertex = TopExp::LastVertex(*sortedIt, Standard_True);
                if (lastVertex.IsSame(destination))
                {
                    closedSignal = true;
                    sortedEdges.erase(sortedIt);
                    break;
                }
                sortedEdges.erase(sortedIt);
                sortedIt = sortedEdges.begin();
                continue;
            }
            ++sortedIt;
        }
        if (closedSignal)
        {
            EdgeVectorType temp;
            std::copy(boundary.begin(), boundary.end(), std::back_inserter(temp));
            boundariesOut.push_back(temp);
        }
    }
}

FaceTypedCylinder& ModelRefine::getCylinderObject()
{
    static FaceTypedCylinder object;
    return object;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

FaceUniter::FaceUniter(const TopoDS_Shell &shellIn) : modifiedSignal(false)
{
    workShell = shellIn;
}

bool FaceUniter::process()
{
    if (workShell.IsNull())
        return false;
    modifiedShapes.clear();
    deletedShapes.clear();
    typeObjects.push_back(&getPlaneObject());
    typeObjects.push_back(&getCylinderObject());
    //add more face types.

    ModelRefine::FaceTypeSplitter splitter;
    splitter.addShell(workShell);
    std::vector<FaceTypedBase *>::iterator typeIt;
    for(typeIt = typeObjects.begin(); typeIt != typeObjects.end(); ++typeIt)
        splitter.registerType((*typeIt)->getType());
    splitter.split();

    ModelRefine::FaceVectorType facesToRemove;
    ModelRefine::FaceVectorType facesToSew;

    ModelRefine::FaceAdjacencySplitter adjacencySplitter(workShell);

    for(typeIt = typeObjects.begin(); typeIt != typeObjects.end(); ++typeIt)
    {
        ModelRefine::FaceVectorType typedFaces = splitter.getTypedFaceVector((*typeIt)->getType());
        ModelRefine::FaceEqualitySplitter equalitySplitter;
        equalitySplitter.split(typedFaces, *typeIt);
        for (std::size_t indexEquality(0); indexEquality < equalitySplitter.getGroupCount(); ++indexEquality)
        {
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
                    // the first shape will be marked as modified, i.e. replaced by newFace, all others are marked as deleted
                    if (!temp.empty())
                    {
                        modifiedShapes.push_back(std::make_pair(temp.front(), newFace));
                        deletedShapes.insert(deletedShapes.end(), temp.begin()+1, temp.end());
                    }
                }
            }
        }
    }
    if (facesToSew.size() > 0)
    {
        modifiedSignal = true;
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
            // update the list of modifications
            for (std::vector<ShapePairType>::iterator it = modifiedShapes.begin(); it != modifiedShapes.end(); ++it)
            {
                if (sew.IsModified(it->second))
                {
                    it->second = sew.Modified(it->second);
                    break;
                }
            }
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

        BRepLib_FuseEdges edgeFuse(workShell, Standard_True);
        TopTools_DataMapOfShapeShape affectedFaces;
        edgeFuse.Faces(affectedFaces);
        TopTools_DataMapIteratorOfDataMapOfShapeShape mapIt;
        for (mapIt.Initialize(affectedFaces); mapIt.More(); mapIt.Next())
        {
            ShapeFix_Face faceFixer(TopoDS::Face(mapIt.Value()));
            faceFixer.Perform();
        }
        workShell = TopoDS::Shell(edgeFuse.Shape());
        // update the list of modifications
        TopTools_DataMapOfShapeShape faceMap;
        edgeFuse.Faces(faceMap);
        for (std::vector<ShapePairType>::iterator it = modifiedShapes.begin(); it != modifiedShapes.end(); ++it)
        {
            if (faceMap.IsBound(it->second))
            {
                const TopoDS_Shape& value = faceMap.Find(it->second);
                if (!value.IsSame(it->second))
                    it->second = value;
            }
        }
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

//BRepBuilderAPI_RefineModel implement a way to log all modifications on the faces

Part::BRepBuilderAPI_RefineModel::BRepBuilderAPI_RefineModel(const TopoDS_Shape& shape)
{
    myShape = shape;
    Build();
}

void Part::BRepBuilderAPI_RefineModel::Build()
{
    if (myShape.IsNull())
        Standard_Failure::Raise("Cannot remove splitter from empty shape");

    if (myShape.ShapeType() == TopAbs_SOLID) {
        const TopoDS_Solid &solid = TopoDS::Solid(myShape);
        BRepBuilderAPI_MakeSolid mkSolid;
        TopExp_Explorer it;
        for (it.Init(solid, TopAbs_SHELL); it.More(); it.Next()) {
            const TopoDS_Shell &currentShell = TopoDS::Shell(it.Current());
            ModelRefine::FaceUniter uniter(currentShell);
            if (uniter.process()) {
                if (uniter.isModified()) {
                    const TopoDS_Shell &newShell = uniter.getShell();
                    mkSolid.Add(newShell);
                    LogModifications(uniter);
                }
                else {
                    mkSolid.Add(currentShell);
                }
            }
            else {
                Standard_Failure::Raise("Removing splitter failed");
            }
        }
        myShape = mkSolid.Solid();
    }
    else if (myShape.ShapeType() == TopAbs_SHELL) {
        const TopoDS_Shell& shell = TopoDS::Shell(myShape);
        ModelRefine::FaceUniter uniter(shell);
        if (uniter.process()) {
            myShape = uniter.getShell();
            LogModifications(uniter);
        }
        else {
            Standard_Failure::Raise("Removing splitter failed");
        }
    }
    else if (myShape.ShapeType() == TopAbs_COMPOUND) {
        BRep_Builder builder;
        TopoDS_Compound comp;
        builder.MakeCompound(comp);

        TopExp_Explorer xp;
        // solids
        for (xp.Init(myShape, TopAbs_SOLID); xp.More(); xp.Next()) {
            const TopoDS_Solid &solid = TopoDS::Solid(xp.Current());
            BRepTools_ReShape reshape;
            TopExp_Explorer it;
            for (it.Init(solid, TopAbs_SHELL); it.More(); it.Next()) {
                const TopoDS_Shell &currentShell = TopoDS::Shell(it.Current());
                ModelRefine::FaceUniter uniter(currentShell);
                if (uniter.process()) {
                    if (uniter.isModified()) {
                        const TopoDS_Shell &newShell = uniter.getShell();
                        reshape.Replace(currentShell, newShell);
                        LogModifications(uniter);
                    }
                }
            }
            builder.Add(comp, reshape.Apply(solid));
        }
        // free shells
        for (xp.Init(myShape, TopAbs_SHELL, TopAbs_SOLID); xp.More(); xp.Next()) {
            const TopoDS_Shell& shell = TopoDS::Shell(xp.Current());
            ModelRefine::FaceUniter uniter(shell);
            if (uniter.process()) {
                builder.Add(comp, uniter.getShell());
                LogModifications(uniter);
            }
        }
        // the rest
        for (xp.Init(myShape, TopAbs_FACE, TopAbs_SHELL); xp.More(); xp.Next()) {
            if (!xp.Current().IsNull())
                builder.Add(comp, xp.Current());
        }
        for (xp.Init(myShape, TopAbs_WIRE, TopAbs_FACE); xp.More(); xp.Next()) {
            if (!xp.Current().IsNull())
                builder.Add(comp, xp.Current());
        }
        for (xp.Init(myShape, TopAbs_EDGE, TopAbs_WIRE); xp.More(); xp.Next()) {
            if (!xp.Current().IsNull())
                builder.Add(comp, xp.Current());
        }
        for (xp.Init(myShape, TopAbs_VERTEX, TopAbs_EDGE); xp.More(); xp.Next()) {
            if (!xp.Current().IsNull())
                builder.Add(comp, xp.Current());
        }

        myShape = comp;
    }

    Done();
}

void Part::BRepBuilderAPI_RefineModel::LogModifications(const ModelRefine::FaceUniter& uniter)
{
    const std::vector<ShapePairType>& modShapes = uniter.getModifiedShapes();
    for (std::vector<ShapePairType>::const_iterator it = modShapes.begin(); it != modShapes.end(); ++it) {
        TopTools_ListOfShape list;
        list.Append(it->second);
        myModified.Bind(it->first, list);
    }
    const ShapeVectorType& delShapes = uniter.getDeletedShapes();
    for (ShapeVectorType::const_iterator it = delShapes.begin(); it != delShapes.end(); ++it) {
        myDeleted.Append(*it);
    }
}

const TopTools_ListOfShape& Part::BRepBuilderAPI_RefineModel::Modified(const TopoDS_Shape& S)
{
    if (myModified.IsBound(S))
        return myModified.Find(S);
    else
        return myEmptyList;
}

Standard_Boolean Part::BRepBuilderAPI_RefineModel::IsDeleted(const TopoDS_Shape& S)
{
    TopTools_ListIteratorOfListOfShape it;
    for (it.Initialize(myDeleted); it.More(); it.Next())
    {
        if (it.Value().IsSame(S))
            return Standard_True;
    }

    return Standard_False;
}

