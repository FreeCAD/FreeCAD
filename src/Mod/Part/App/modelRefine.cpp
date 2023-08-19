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

#ifndef _PreComp_
# include <algorithm>
# include <iterator>
# include <Bnd_Box.hxx>
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepBndLib.hxx>
# include <BRepGProp.hxx>
# include <BRepLib_FuseEdges.hxx>
# include <BRepLib_MakeWire.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <Geom_BSplineSurface.hxx>
# include <Geom_Conic.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <Geom_Plane.hxx>
# include <Geom_RectangularTrimmedSurface.hxx>
# include <Geom_Surface.hxx>
# include <GeomAdaptor_Surface.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <gp_Ax3.hxx>
# include <gp_Cylinder.hxx>
# include <gp_Pln.hxx>
# include <GProp_GProps.hxx>
# include <ShapeAnalysis_Curve.hxx>
# include <ShapeAnalysis_Shell.hxx>
# include <ShapeBuild_ReShape.hxx>
# include <ShapeFix_Face.hxx>
# include <Standard_Version.hxx>
# include <TColgp_Array2OfPnt.hxx>
# include <TColgp_SequenceOfPnt.hxx>
# include <TColStd_Array1OfReal.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>
# include <TopoDS_Shape.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_DataMapIteratorOfDataMapOfIntegerListOfShape.hxx>
# include <TopTools_DataMapIteratorOfDataMapOfShapeShape.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
# include <TopTools_ListOfShape.hxx>
#endif // _PreComp_

#include <Base/Console.h>

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
            // Note: This works because we can be sure that the wires do not intersect
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

static Handle(Geom_Plane) getGeomPlane(const TopoDS_Face &faceIn)
{
  Handle(Geom_Plane) planeSurfaceOut;
  Handle(Geom_Surface) surface = BRep_Tool::Surface(faceIn);
  if (!surface.IsNull())
  {
    planeSurfaceOut = Handle(Geom_Plane)::DownCast(surface);
    if (planeSurfaceOut.IsNull())
    {
      Handle(Geom_RectangularTrimmedSurface) trimmedSurface = Handle(Geom_RectangularTrimmedSurface)::DownCast(surface);
      if (!trimmedSurface.IsNull())
        planeSurfaceOut = Handle(Geom_Plane)::DownCast(trimmedSurface->BasisSurface());
    }
  }

  return planeSurfaceOut;
}

bool FaceTypedPlane::isEqual(const TopoDS_Face &faceOne, const TopoDS_Face &faceTwo) const
{
  Handle(Geom_Plane) planeSurfaceOne = getGeomPlane(faceOne);
  Handle(Geom_Plane) planeSurfaceTwo = getGeomPlane(faceTwo);
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
        return {};
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
        return {};
    TopoDS_Face current = faceMaker.Face();
    if (wires.size() > 1)
    {
        ShapeFix_Face faceFix(current);
        faceFix.SetContext(new ShapeBuild_ReShape());
        for (size_t index(1); index<wires.size(); ++index)
            faceFix.Add(wires.at(index));
        faceFix.Perform();
        if (faceFix.Status(ShapeExtend_FAIL))
            return {};
        faceFix.FixOrientation();
        faceFix.Perform();
        if(faceFix.Status(ShapeExtend_FAIL))
            return {};
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

static Handle(Geom_CylindricalSurface) getGeomCylinder(const TopoDS_Face &faceIn)
{
  Handle(Geom_CylindricalSurface) cylinderSurfaceOut;
  Handle(Geom_Surface) surface = BRep_Tool::Surface(faceIn);
  if (!surface.IsNull())
  {
    cylinderSurfaceOut = Handle(Geom_CylindricalSurface)::DownCast(surface);
    if (cylinderSurfaceOut.IsNull())
    {
      Handle(Geom_RectangularTrimmedSurface) trimmedSurface = Handle(Geom_RectangularTrimmedSurface)::DownCast(surface);
      if (!trimmedSurface.IsNull())
        cylinderSurfaceOut = Handle(Geom_CylindricalSurface)::DownCast(trimmedSurface->BasisSurface());
    }
  }

  return cylinderSurfaceOut;
}

bool FaceTypedCylinder::isEqual(const TopoDS_Face &faceOne, const TopoDS_Face &faceTwo) const
{
    Handle(Geom_CylindricalSurface) surfaceOne = getGeomCylinder(faceOne);
    Handle(Geom_CylindricalSurface) surfaceTwo = getGeomCylinder(faceTwo);
    if (surfaceOne.IsNull() || surfaceTwo.IsNull())
        return false;//probably need an error
    gp_Cylinder cylinderOne = surfaceOne->Cylinder();
    gp_Cylinder cylinderTwo = surfaceTwo->Cylinder();

    if (fabs(cylinderOne.Radius() - cylinderTwo.Radius()) > Precision::Confusion())
        return false;
    if (!cylinderOne.Axis().IsCoaxial(cylinderTwo.Axis(), Precision::Angular(), Precision::Confusion()) &&
        !cylinderOne.Axis().IsCoaxial(cylinderTwo.Axis().Reversed(), Precision::Angular(), Precision::Confusion()))
        return false;

    return true;
}

GeomAbs_SurfaceType FaceTypedCylinder::getType() const
{
    return GeomAbs_Cylinder;
}

// Auxiliary method
const TopoDS_Face fixFace(const TopoDS_Face& f) {
    static TopoDS_Face dummy;
    // Fix the face. Orientation doesn't seem to get fixed the first call.
    ShapeFix_Face faceFixer(f);
    faceFixer.SetContext(new ShapeBuild_ReShape());
    faceFixer.Perform();
    if (faceFixer.Status(ShapeExtend_FAIL))
        return dummy;
    faceFixer.FixMissingSeam();
    faceFixer.Perform();
    if (faceFixer.Status(ShapeExtend_FAIL))
      return dummy;
    faceFixer.FixOrientation();
    faceFixer.Perform();
    if (faceFixer.Status(ShapeExtend_FAIL))
        return dummy;
    return faceFixer.Face();
}

// Detect whether a wire encircles the cylinder axis or not. This is done by calculating the
// wire length, oriented with respect to the cylinder axis
bool wireEncirclesAxis(const TopoDS_Wire& wire, const Handle(Geom_CylindricalSurface)& cylinder)
{
    double radius = cylinder->Radius();
    gp_Ax1 cylAxis = cylinder->Axis();
    gp_Vec cv(cylAxis.Location().X(), cylAxis.Location().Y(), cylAxis.Location().Z()); // center of cylinder
    gp_Vec av(cylAxis.Direction().X(), cylAxis.Direction().Y(), cylAxis.Direction().Z()); // axis of cylinder
    Handle(Geom_Plane) plane = new Geom_Plane(gp_Ax3(cylAxis.Location(), cylAxis.Direction()));
    double totalArc = 0.0;
    bool firstSegment = false;
    bool secondSegment = false;
    gp_Pnt first, last;

    for (TopExp_Explorer ex(wire, TopAbs_EDGE); ex.More(); ex.Next()) {
        TopoDS_Edge segment(TopoDS::Edge(ex.Current()));
        BRepAdaptor_Curve adapt(segment);

        // Get curve data
        double fp = adapt.FirstParameter();
        double lp = adapt.LastParameter();
        gp_Pnt segFirst, segLast;
        gp_Vec segTangent;
        adapt.D1(fp, segFirst, segTangent);
        segLast = adapt.Value(lp);

        double length = 0.0;

        if (adapt.GetType() == GeomAbs_Line) {
            // Any line on the cylinder must be parallel to the cylinder axis
           length = 0.0;
        } else if (adapt.GetType() == GeomAbs_Circle) {
            // Arc segment
            length = (lp - fp) * radius;
            // Check orientation in relation to cylinder axis
            GeomAPI_ProjectPointOnSurf proj(segFirst, plane);
            gp_Vec bv = gp_Vec(proj.Point(1).X(), proj.Point(1).Y(), proj.Point(1).Z());
            if ((bv - cv).Crossed(segTangent).IsOpposite(av, Precision::Confusion()))
                length = -length;
        } else {
            // Linearize the edge. Idea taken from ShapeAnalysis.cxx ShapeAnalysis::TotCross2D()
            TColgp_SequenceOfPnt SeqPnt;
            ShapeAnalysis_Curve::GetSamplePoints(adapt.Curve().Curve(), fp, lp, SeqPnt);

            // Calculate the oriented length of the edge
            gp_Pnt begin;
            for (Standard_Integer j=1; j <= SeqPnt.Length(); j++) {
                gp_Pnt end = SeqPnt.Value(j);

                // Project end point onto the plane
                GeomAPI_ProjectPointOnSurf proj(end, plane);
                if (!proj.IsDone())
                    return false; // FIXME: What else can we do?
                gp_Pnt pend = proj.Point(1);

                if (j > 1) {
                    // Get distance between the points, equal to (linearised) arc length
                    gp_Vec bv = gp_Vec(begin.X(), begin.Y(), begin.Z());
                    gp_Vec dv = gp_Vec(pend.X(), pend.Y(), pend.Z()) - bv;
                    double dist = dv.Magnitude();

                    if (dist > 0) {
                        // Check orientation of this piece in relation to cylinder axis
                        if ((bv - cv).Crossed(dv).IsOpposite(av, Precision::Confusion()))
                            dist = -dist;

                        length += dist;
                    }
                }

                begin = pend;
            }
        }

        if (!firstSegment) {
            // First wire segment. Save the start and end point of the segment
            firstSegment = true;
            first = segFirst;
            last = segLast;
        } else if (!secondSegment) {
            // Second wire segment. Determine whether the second segment continues from
            // the first or from the last point of the first segment
            secondSegment = true;
            if (last.IsEqual(segFirst, Precision::Confusion())) {
                last = segLast; // Third segment must begin here
            } else if (last.IsEqual(segLast, Precision::Confusion())) {
                last = segFirst;
                length = -length;
            } else if (first.IsEqual(segLast, Precision::Confusion())) {
                last = segFirst;
                totalArc = -totalArc;
                length = -length;
            } else { // first.IsEqual(segFirst)
                last = segLast;
                totalArc = -totalArc;
            }
        } else {
            if (!last.IsEqual(segFirst, Precision::Confusion())) {
                // The length was calculated in the opposite direction of the wire traversal
                length = -length;
                last = segFirst;
            } else {
                last = segLast;
            }
        }

        totalArc += length;
    }

    // For an exact calculation, only two results would be possible:
    // totalArc = 0.0: The wire does not encircle the axis
    // totalArc = 2 * M_PI * radius: The wire encircles the axis
    return (fabs(totalArc) > M_PI * radius);
}

TopoDS_Face FaceTypedCylinder::buildFace(const FaceVectorType &faces) const
{
    static TopoDS_Face dummy;
    std::vector<EdgeVectorType> boundaries;
    boundarySplit(faces, boundaries);
    if (boundaries.empty())
        return dummy;

    //make wires
    std::vector<TopoDS_Wire> allWires;
    std::vector<EdgeVectorType>::iterator boundaryIt;
    for (boundaryIt = boundaries.begin(); boundaryIt != boundaries.end(); ++boundaryIt)
    {
        BRepLib_MakeWire wireMaker;
        EdgeVectorType::iterator it;
        for (it = (*boundaryIt).begin(); it != (*boundaryIt).end(); ++it)
            wireMaker.Add(*it);
        if (wireMaker.Error() != BRepLib_WireDone)
            return dummy;
        allWires.push_back(wireMaker.Wire());
    }
    if (allWires.empty())
        return dummy;

    // Sort wires by size, that is, the innermost wire comes last
    std::sort(allWires.begin(), allWires.end(), ModelRefine::WireSort());

    // Find outer boundary wires that cut the cylinder into segments. This will be the case f we
    // have removed the seam edges of a complete (360 degrees) cylindrical face
    Handle(Geom_CylindricalSurface) surface = getGeomCylinder(faces.at(0));
    if (surface.IsNull())
      return dummy;
    std::vector<TopoDS_Wire> innerWires, encirclingWires;
    std::vector<TopoDS_Wire>::iterator wireIt;
    for (wireIt = allWires.begin(); wireIt != allWires.end(); ++wireIt) {
        if (wireEncirclesAxis(*wireIt, surface))
            encirclingWires.push_back(*wireIt);
        else
            innerWires.push_back(*wireIt);
    }

    if (encirclingWires.empty()) {
        // We can use the result of the bounding box sort. First wire is the outer wire
        wireIt = allWires.begin();
        BRepBuilderAPI_MakeFace faceMaker(surface, *wireIt);
        if (!faceMaker.IsDone())
            return dummy;

        // Add additional boundaries (inner wires).
        for (wireIt++; wireIt != allWires.end(); ++wireIt)
        {
            faceMaker.Add(*wireIt);
            if (!faceMaker.IsDone())
                return dummy;
        }

        return fixFace(faceMaker.Face());
    } else {
        if (encirclingWires.size() != 2)
            return dummy;

        if (innerWires.empty()) {
            // We have just two outer boundaries
            BRepBuilderAPI_MakeFace faceMaker(surface, encirclingWires.front());
            if (!faceMaker.IsDone())
                return dummy;
            faceMaker.Add(encirclingWires.back());
            if (!faceMaker.IsDone())
                return dummy;

            return fixFace(faceMaker.Face());
        } else {
            // Add the inner wires first, because otherwise those that cut the seam edge will fail
            wireIt = innerWires.begin();
            BRepBuilderAPI_MakeFace faceMaker(surface, *wireIt, false);
            if (!faceMaker.IsDone())
                return dummy;

            // Add additional boundaries (inner wires).
            for (wireIt++; wireIt != innerWires.end(); ++wireIt)
            {
                faceMaker.Add(*wireIt);
                if (!faceMaker.IsDone())
                    return dummy;
            }

            // Add outer boundaries
            faceMaker.Add(encirclingWires.front());
            if (!faceMaker.IsDone())
                return dummy;
            faceMaker.Add(encirclingWires.back());
            if (!faceMaker.IsDone())
                return dummy;

            return fixFace(faceMaker.Face());
        }
    }
}

void FaceTypedCylinder::boundarySplit(const FaceVectorType &facesIn, std::vector<EdgeVectorType> &boundariesOut) const
{
    //normal edges.
    EdgeVectorType normalEdges;
    ModelRefine::boundaryEdges(facesIn, normalEdges);

    std::list<TopoDS_Edge> sortedEdges;
    std::copy(normalEdges.begin(), normalEdges.end(), back_inserter(sortedEdges));

    while (!sortedEdges.empty())
    {
        TopoDS_Vertex destination = TopExp::FirstVertex(sortedEdges.back(), Standard_True);
        TopoDS_Vertex lastVertex = TopExp::LastVertex(sortedEdges.back(), Standard_True);
        bool closedSignal(false);
        std::list<TopoDS_Edge> boundary;
        boundary.push_back(sortedEdges.back());
        sortedEdges.pop_back();

        if (destination.IsSame(lastVertex)) {
            // Single circular edge
            closedSignal = true;
        } else {
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

// TODO: change this version after occ fix. Freecad Mantis 1450
#if OCC_VERSION_HEX <= 0x7fffff
void collectConicEdges(const TopoDS_Shell &shell, TopTools_IndexedMapOfShape &map)
{
  TopTools_IndexedMapOfShape edges;
  TopExp::MapShapes(shell, TopAbs_EDGE, edges);

  for (int index = 1; index <= edges.Extent(); ++index)
  {
    const TopoDS_Edge &currentEdge = TopoDS::Edge(edges.FindKey(index));
    if (currentEdge.IsNull())
      continue;
    TopLoc_Location location;
    Standard_Real first, last;
    const Handle(Geom_Curve) &curve = BRep_Tool::Curve(currentEdge, location, first, last);
    if (curve.IsNull())
      continue;
    if (curve->IsKind(STANDARD_TYPE(Geom_Conic)))
      map.Add(currentEdge);
  }
}
#endif

FaceTypedBSpline::FaceTypedBSpline() : FaceTypedBase(GeomAbs_BSplineSurface)
{
}

bool FaceTypedBSpline::isEqual(const TopoDS_Face &faceOne, const TopoDS_Face &faceTwo) const
{
  try
  {
    Handle(Geom_BSplineSurface) surfaceOne = Handle(Geom_BSplineSurface)::DownCast(BRep_Tool::Surface(faceOne));
    Handle(Geom_BSplineSurface) surfaceTwo = Handle(Geom_BSplineSurface)::DownCast(BRep_Tool::Surface(faceTwo));

    if (surfaceOne.IsNull() || surfaceTwo.IsNull())
        return false;

    if (surfaceOne->IsURational() != surfaceTwo->IsURational())
        return false;
    if (surfaceOne->IsVRational() != surfaceTwo->IsVRational())
        return false;
    if (surfaceOne->IsUPeriodic() != surfaceTwo->IsUPeriodic())
        return false;
    if (surfaceOne->IsVPeriodic() != surfaceTwo->IsVPeriodic())
        return false;
    if (surfaceOne->IsUClosed() != surfaceTwo->IsUClosed())
        return false;
    if (surfaceOne->IsVClosed() != surfaceTwo->IsVClosed())
        return false;
    if (surfaceOne->UDegree() != surfaceTwo->UDegree())
        return false;
    if (surfaceOne->VDegree() != surfaceTwo->VDegree())
        return false;

    //pole test
    int uPoleCountOne(surfaceOne->NbUPoles());
    int vPoleCountOne(surfaceOne->NbVPoles());
    int uPoleCountTwo(surfaceTwo->NbUPoles());
    int vPoleCountTwo(surfaceTwo->NbVPoles());

    if (uPoleCountOne != uPoleCountTwo || vPoleCountOne != vPoleCountTwo)
        return false;

    TColgp_Array2OfPnt polesOne(1, uPoleCountOne, 1, vPoleCountOne);
    TColgp_Array2OfPnt polesTwo(1, uPoleCountTwo, 1, vPoleCountTwo);
    surfaceOne->Poles(polesOne);
    surfaceTwo->Poles(polesTwo);

    for (int indexU = 1; indexU <= uPoleCountOne; ++indexU)
    {
        for (int indexV = 1; indexV <= vPoleCountOne; ++indexV)
        {
            if (!(polesOne.Value(indexU, indexV).IsEqual(polesTwo.Value(indexU, indexV), Precision::Confusion())))
                return false;
        }
    }

    //knot test
    int uKnotCountOne(surfaceOne->NbUKnots());
    int vKnotCountOne(surfaceOne->NbVKnots());
    int uKnotCountTwo(surfaceTwo->NbUKnots());
    int vKnotCountTwo(surfaceTwo->NbVKnots());
    if (uKnotCountOne != uKnotCountTwo || vKnotCountOne != vKnotCountTwo)
        return false;
    TColStd_Array1OfReal uKnotsOne(1, uKnotCountOne);
    TColStd_Array1OfReal vKnotsOne(1, vKnotCountOne);
    TColStd_Array1OfReal uKnotsTwo(1, uKnotCountTwo);
    TColStd_Array1OfReal vKnotsTwo(1, vKnotCountTwo);
    surfaceOne->UKnots(uKnotsOne);
    surfaceOne->VKnots(vKnotsOne);
    surfaceTwo->UKnots(uKnotsTwo);
    surfaceTwo->VKnots(vKnotsTwo);
    for (int indexU = 1; indexU <= uKnotCountOne; ++indexU)
        if (uKnotsOne.Value(indexU) != uKnotsTwo.Value(indexU))
            return false;
    for (int indexV = 1; indexV <= vKnotCountOne; ++indexV)
        if (vKnotsOne.Value(indexV) != vKnotsTwo.Value(indexV))
            return false;

    //Formulas:
    //Periodic:     knots=poles+2*degree-mult(1)+2
    //Non-periodic: knots=poles+degree+1
    auto getUKnotSequenceSize = [](Handle(Geom_BSplineSurface) surface) {
        int uPoleCount(surface->NbUPoles());
        int uDegree(surface->UDegree());
        if (surface->IsUPeriodic()) {
            int uMult1(surface->UMultiplicity(1));
            int size = uPoleCount + 2*uDegree - uMult1 + 2;
            return size;
        }
        else {
            int size = uPoleCount + uDegree + 1;
            return size;
        }
    };
    auto getVKnotSequenceSize = [](Handle(Geom_BSplineSurface) surface) {
        int vPoleCount(surface->NbVPoles());
        int vDegree(surface->VDegree());
        if (surface->IsVPeriodic()) {
            int vMult1(surface->VMultiplicity(1));
            int size = vPoleCount + 2*vDegree - vMult1 + 2;
            return size;
        }
        else {
            int size = vPoleCount + vDegree + 1;
            return size;
        }
    };

    //knot sequence.
    int uKnotSequenceOneCount(getUKnotSequenceSize(surfaceOne));
    int vKnotSequenceOneCount(getVKnotSequenceSize(surfaceOne));
    int uKnotSequenceTwoCount(getUKnotSequenceSize(surfaceTwo));
    int vKnotSequenceTwoCount(getVKnotSequenceSize(surfaceTwo));
    if (uKnotSequenceOneCount != uKnotSequenceTwoCount || vKnotSequenceOneCount != vKnotSequenceTwoCount)
        return false;
    TColStd_Array1OfReal uKnotSequenceOne(1, uKnotSequenceOneCount);
    TColStd_Array1OfReal vKnotSequenceOne(1, vKnotSequenceOneCount);
    TColStd_Array1OfReal uKnotSequenceTwo(1, uKnotSequenceTwoCount);
    TColStd_Array1OfReal vKnotSequenceTwo(1, vKnotSequenceTwoCount);
    surfaceOne->UKnotSequence(uKnotSequenceOne);
    surfaceOne->VKnotSequence(vKnotSequenceOne);
    surfaceTwo->UKnotSequence(uKnotSequenceTwo);
    surfaceTwo->VKnotSequence(vKnotSequenceTwo);
    for (int indexU = 1; indexU <= uKnotSequenceOneCount; ++indexU)
        if (uKnotSequenceOne.Value(indexU) != uKnotSequenceTwo.Value(indexU))
            return false;
    for (int indexV = 1; indexV <= vKnotSequenceOneCount; ++indexV)
        if (vKnotSequenceOne.Value(indexV) != vKnotSequenceTwo.Value(indexV))
            return false;
    return true;
  }
  catch (Standard_Failure& e)
  {
    std::ostringstream stream;
    if (e.GetMessageString())
      stream << "FaceTypedBSpline::isEqual: OCC Error: " << e.GetMessageString() << std::endl;
    else
      stream << "FaceTypedBSpline::isEqual: Unknown OCC Error" << std::endl;
    Base::Console().Message(stream.str().c_str());
  }
  catch (...)
  {
    std::ostringstream stream;
    stream << "FaceTypedBSpline::isEqual: Unknown Error" << std::endl;
    Base::Console().Message(stream.str().c_str());
  }

  return false;
}

GeomAbs_SurfaceType FaceTypedBSpline::getType() const
{
    return GeomAbs_BSplineSurface;
}

TopoDS_Face FaceTypedBSpline::buildFace(const FaceVectorType &faces) const
{
    std::vector<TopoDS_Wire> wires;

    std::vector<EdgeVectorType> splitEdges;
    this->boundarySplit(faces, splitEdges);
    if (splitEdges.empty())
        return {};
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

    //make face from surface and outer wire.
    Handle(Geom_BSplineSurface) surface = Handle(Geom_BSplineSurface)::DownCast(BRep_Tool::Surface(faces.at(0)));
    if (!surface)
        return {};
    std::vector<TopoDS_Wire>::iterator wireIt;
    wireIt = wires.begin();
    BRepBuilderAPI_MakeFace faceMaker(surface, *wireIt);
    if (!faceMaker.IsDone())
        return {};

    //add additional boundaries.
    for (wireIt++; wireIt != wires.end(); ++wireIt)
    {
        faceMaker.Add(*wireIt);
        if (!faceMaker.IsDone())
            return {};
    }

    //fix newly constructed face. Orientation doesn't seem to get fixed the first call.
    ShapeFix_Face faceFixer(faceMaker.Face());
    faceFixer.SetContext(new ShapeBuild_ReShape());
    faceFixer.Perform();
    if (faceFixer.Status(ShapeExtend_FAIL))
        return {};
    faceFixer.FixOrientation();
    faceFixer.Perform();
    if (faceFixer.Status(ShapeExtend_FAIL))
        return {};

    return faceFixer.Face();
}

FaceTypedBSpline& ModelRefine::getBSplineObject()
{
    static FaceTypedBSpline object;
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
    typeObjects.push_back(&getBSplineObject());
    //add more face types.

    bool checkFinalShell = false;
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
                    // the created face should have the same orientation as the input faces
                    const FaceVectorType& faces = adjacencySplitter.getGroup(adjacentIndex);
                    if (!faces.empty() && newFace.Orientation() != faces[0].Orientation()) {
                        checkFinalShell = true;
                    }
                    facesToSew.push_back(newFace);
                    if (facesToRemove.capacity() <= facesToRemove.size() + adjacencySplitter.getGroup(adjacentIndex).size())
                        facesToRemove.reserve(facesToRemove.size() + adjacencySplitter.getGroup(adjacentIndex).size());
                    FaceVectorType temp = adjacencySplitter.getGroup(adjacentIndex);
                    facesToRemove.insert(facesToRemove.end(), temp.begin(), temp.end());
                    // the first shape will be marked as modified, i.e. replaced by newFace, all others are marked as deleted
                    // jrheinlaender: IMHO this is not correct because references to the deleted faces will be broken, whereas they should
                    // be replaced by references to the new face. To achieve this all shapes should be marked as
                    // modified, producing one single new face. This is the inverse behaviour to faces that are split e.g.
                    // by a boolean cut, where one old shape is marked as modified, producing multiple new shapes
                    if (!temp.empty())
                    {
                        for (const auto & f : temp)
                              modifiedShapes.emplace_back(f, newFace);
                    }
                }
            }
        }
    }
    if (!facesToSew.empty())
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
            try {
                workShell = TopoDS::Shell(sew.SewedShape());
            } catch (Standard_Failure&) {
                return false;
            }
            // update the list of modifications
            for (auto & it : modifiedShapes)
            {
                if (sew.IsModified(it.second))
                {
                    it.second = sew.Modified(it.second);
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

        BRepLib_FuseEdges edgeFuse(workShell);
// TODO: change this version after occ fix. Freecad Mantis 1450
#if OCC_VERSION_HEX <= 0x7fffff
        TopTools_IndexedMapOfShape map;
        collectConicEdges(workShell, map);
        edgeFuse.AvoidEdges(map);
#endif
        TopTools_DataMapOfShapeShape affectedFaces;
        edgeFuse.Faces(affectedFaces);
        TopTools_DataMapIteratorOfDataMapOfShapeShape mapIt;
        for (mapIt.Initialize(affectedFaces); mapIt.More(); mapIt.Next())
        {
            ShapeFix_Face faceFixer(TopoDS::Face(mapIt.Value()));
            faceFixer.Perform();
        }
        workShell = TopoDS::Shell(edgeFuse.Shape());

        // A difference of orientation between original and fused faces was previously detected
        if (checkFinalShell) {
            ShapeAnalysis_Shell check;
            check.LoadShells(workShell);
            if (!check.HasFreeEdges()) {
                // the shell is a solid, make sure that volume is positive
                GProp_GProps props;
                BRepGProp::VolumeProperties(workShell, props);
                if (props.Mass() < 0)
                    workShell = TopoDS::Shell(workShell.Reversed());
            }
        }
        // update the list of modifications
        TopTools_DataMapOfShapeShape faceMap;
        edgeFuse.Faces(faceMap);
        for (mapIt.Initialize(faceMap); mapIt.More(); mapIt.Next())
        {
            bool isModifiedFace = false;
            for (auto & it : modifiedShapes)
            {
                if (mapIt.Key().IsSame(it.second)) {
                    // Note: IsEqual() for some reason does not work
                    it.second = mapIt.Value();
                    isModifiedFace = true;
                }
            }
            if (!isModifiedFace)
            {
                // Catch faces that were not united but whose boundary was changed (probably because
                // several adjacent faces were united)
                // See https://sourceforge.net/apps/mantisbt/free-cad/view.php?id=873
                modifiedShapes.emplace_back(mapIt.Key(), mapIt.Value());
            }
        }
        // Handle edges that were fused. See https://sourceforge.net/apps/mantisbt/free-cad/view.php?id=873
        TopTools_DataMapOfIntegerListOfShape oldEdges;
        TopTools_DataMapOfIntegerShape newEdges;
        edgeFuse.Edges(oldEdges);
        edgeFuse.ResultEdges(newEdges);
        TopTools_DataMapIteratorOfDataMapOfIntegerListOfShape edgeMapIt;
        for (edgeMapIt.Initialize(oldEdges); edgeMapIt.More(); edgeMapIt.Next())
        {
            const TopTools_ListOfShape& edges = edgeMapIt.Value();
            int idx = edgeMapIt.Key();
            TopTools_ListIteratorOfListOfShape edgeIt;
            for (edgeIt.Initialize(edges); edgeIt.More(); edgeIt.Next())
            {
                if (newEdges.IsBound(idx))
                    modifiedShapes.emplace_back(edgeIt.Value(), newEdges(idx));
            }
            // TODO: Handle vertices that have disappeared in the fusion of the edges
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

#if OCC_VERSION_HEX >= 0x070600
void Part::BRepBuilderAPI_RefineModel::Build(const Message_ProgressRange&)
#else
void Part::BRepBuilderAPI_RefineModel::Build()
#endif
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
            // TODO: Why not check for uniter.isModified()?
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
    for (const auto & it : modShapes) {
        TopTools_ListOfShape list;
        list.Append(it.second);
        myModified.Bind(it.first, list);
    }
    const ShapeVectorType& delShapes = uniter.getDeletedShapes();
    for (const auto & it : delShapes) {
        myDeleted.Append(it);
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

