/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
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

#include <Mod/Part/PartGlobal.h>

#include <string>

#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopAbs.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <BRepGProp.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopExp.hxx>
#include <GProp_GProps.hxx>
#include <ShapeAnalysis_Edge.hxx>
#include <gp_Circ.hxx>
#include <DatumFeature.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Matrix.h>
#include <Base/Rotation.h>
#include <Base/Vector3D.h>

#include <Mod/Measure/App/MeasureAngle.h>
#include <Mod/Measure/App/MeasureDistance.h>
#include <Mod/Measure/App/MeasureLength.h>
#include <Mod/Measure/App/MeasurePosition.h>
#include <Mod/Measure/App/MeasureArea.h>
#include <Mod/Measure/App/MeasureRadius.h>


#include "VectorAdapter.h"
#include "PartFeature.h"

#include "MeasureClient.h"

// From: https://github.com/Celemation/FreeCAD/blob/joel_selection_summary_demo/src/Gui/SelectionSummary.cpp

// Should work with edges and wires
static float getLength(TopoDS_Shape& wire){
    GProp_GProps gprops;
    BRepGProp::LinearProperties(wire, gprops);
    return gprops.Mass();
}

static float getFaceArea(TopoDS_Shape& face){
    GProp_GProps gprops;
    BRepGProp::SurfaceProperties(face, gprops);
    return gprops.Mass();
}

static float getRadius(TopoDS_Shape& edge){
    // gprops.Mass() would be the circumference (length) of the circle (arc)
    if (edge.ShapeType() == TopAbs_EDGE) {
        BRepAdaptor_Curve adapt(TopoDS::Edge(edge));
        if (adapt.GetType() != GeomAbs_Circle) {
            // TODO: not sure what the error handling here should be. nan? 0.0?
            return 0.0;
        }
        gp_Circ circle = adapt.Circle();
        return circle.Radius();
    }
    return 0.0;
}

TopoDS_Shape getLocatedShape(const std::string& objectName, const std::string& subName)
{
    App::DocumentObject* obj = App::GetApplication().getActiveDocument()->getObject(objectName.c_str());
    // should this be getTopoShape(obj, subName, true)?
    Part::TopoShape shape = Part::Feature::getTopoShape(obj);
    auto geoFeat = dynamic_cast<App::GeoFeature*>(obj);
    if (geoFeat) {
        shape.setPlacement(geoFeat->globalPlacement());
    }

    // Don't get the subShape from datum elements
    if (obj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId())) {
        return shape.getShape();
    }

    if (subName.empty())
    {
        return shape.getShape();
    }
    return shape.getSubShape(subName.c_str(), true);
}


App::MeasureElementType PartMeasureTypeCb(const char* objectName, const char* subName) {
    App::DocumentObject* ob = App::GetApplication().getActiveDocument()->getObject(objectName);
//    auto sub = ob->getSubObject(subName);

    TopoDS_Shape shape = Part::Feature::getShape(ob, subName, true);
    if (shape.IsNull()) {
        // failure here on loading document with existing Measurement.
        Base::Console().Message("Part::VectorAdapter did not retrieve shape for %s, %s\n", objectName, subName);
        return App::MeasureElementType();
    }
    TopAbs_ShapeEnum shapeType = shape.ShapeType();

    switch (shapeType) {
        case TopAbs_VERTEX: {
            return App::MeasureElementType::POINT;
        }
        case TopAbs_EDGE: {
            const TopoDS_Edge& edge = TopoDS::Edge(shape);
            BRepAdaptor_Curve curve(edge);

            switch (curve.GetType()) {
                case GeomAbs_Line: {
                    if (ob->getTypeId().isDerivedFrom(Base::Type::fromName("Part::Datum"))) {
                        return App::MeasureElementType::LINE;
                    }
                    return App::MeasureElementType::LINESEGMENT;
                }
                case GeomAbs_Circle: { return App::MeasureElementType::CIRCLE; }
                case GeomAbs_BezierCurve:
                case GeomAbs_BSplineCurve: {
                    return App::MeasureElementType::CURVE;
                }
                default: { return App::MeasureElementType::INVALID; }
            }
        }
        case TopAbs_FACE: {
            const TopoDS_Face& face = TopoDS::Face(shape);
            BRepAdaptor_Surface surface(face);

            switch (surface.GetType()) {
                case GeomAbs_Cylinder: { return App::MeasureElementType::CYLINDER; }
                case GeomAbs_Plane: { return App::MeasureElementType::PLANE; }
                default: { return App::MeasureElementType::INVALID; }
            }
        }
        case TopAbs_SOLID: {
            return App::MeasureElementType::Volume;
        }
        default: {
            return App::MeasureElementType::INVALID;
        }
    }
}



bool getShapeFromStrings(TopoDS_Shape &shapeOut, const std::string &doc, const std::string &object, const std::string &sub, Base::Matrix4D *mat)
{
  App::Document *docPointer = App::GetApplication().getDocument(doc.c_str());
  if (!docPointer) {
    return false;
  }
  App::DocumentObject *objectPointer = docPointer->getObject(object.c_str());
  if (!objectPointer) {
    return false;
  }
  shapeOut = Part::Feature::getShape(objectPointer,sub.c_str(),true,mat);
  return !shapeOut.IsNull();
}



Part::VectorAdapter buildAdapter(const App::DocumentObject* ob, std::string* objectName, const std::string* subName)
{
    (void) objectName;
    Base::Matrix4D mat;
    TopoDS_Shape shape = Part::Feature::getShape(ob, subName->c_str(), true);
    if (shape.IsNull()) {
        // failure here on loading document with existing MeasureClientment.
        Base::Console().Message("Part::VectorAdapter did not retrieve shape for %s, %s\n", objectName->c_str(), subName->c_str());
        return Part::VectorAdapter();
    }

    TopAbs_ShapeEnum shapeType = shape.ShapeType();


    if (shapeType == TopAbs_EDGE)
    {
      TopoDS_Shape edgeShape;
      if (!getShapeFromStrings(edgeShape, ob->getDocument()->getName(), ob->getNameInDocument(), *subName, &mat)) {
        return {};
      }
      TopoDS_Edge edge = TopoDS::Edge(edgeShape);
      // make edge orientation so that end of edge closest to pick is head of vector.
      TopoDS_Vertex firstVertex = TopExp::FirstVertex(edge, Standard_True);
      TopoDS_Vertex lastVertex = TopExp::LastVertex(edge, Standard_True);
      if (firstVertex.IsNull() || lastVertex.IsNull()) {
        return {};
      }
      gp_Vec firstPoint = Part::VectorAdapter::convert(firstVertex);
      gp_Vec lastPoint = Part::VectorAdapter::convert(lastVertex);
      Base::Vector3d v(0.0, 0.0, 0.0); //v(current.x,current.y,current.z);
      v = mat*v;
      gp_Vec pickPoint(v.x, v.y, v.z);
      double firstDistance = (firstPoint - pickPoint).Magnitude();
      double lastDistance = (lastPoint - pickPoint).Magnitude();
      if (lastDistance > firstDistance)
      {
        if (edge.Orientation() == TopAbs_FORWARD) {
          edge.Orientation(TopAbs_REVERSED);
        }
        else {
          edge.Orientation(TopAbs_FORWARD);
        }
      }
      return {edge, pickPoint};
    }
    if (shapeType == TopAbs_FACE)
    {
      TopoDS_Shape faceShape;
      if (!getShapeFromStrings(faceShape, ob->getDocument()->getName(), ob->getNameInDocument(), *subName, &mat)) {
        return {};
      }

      TopoDS_Face face = TopoDS::Face(faceShape);
      Base::Vector3d vTemp(0.0, 0.0, 0.0); //v(current.x, current.y, current.z);
      vTemp = mat*vTemp;
      gp_Vec pickPoint(vTemp.x, vTemp.y, vTemp.z);
      return {face, pickPoint};
    }

    return {};
}


Measure::MeasureLengthInfo* MeasureLengthHandler(std::string* objectName, std::string* subName){
    TopoDS_Shape shape = getLocatedShape(*objectName, *subName);

    if (shape.IsNull()) {
        // failure here on loading document with existing MeasureClientment.
        Base::Console().Message("MeasureClientLengthHandler did not retrieve shape for %s, %s\n", objectName->c_str(), subName->c_str());
        return new Measure::MeasureLengthInfo{false, 0.0, Base::Matrix4D()};
    }
    TopAbs_ShapeEnum sType = shape.ShapeType();

    if (sType != TopAbs_EDGE) {
        return new Measure::MeasureLengthInfo{false, 0.0, Base::Matrix4D()};
    }

    // Get Center of mass as the attachment point of the label
    GProp_GProps gprops;
    BRepGProp::LinearProperties(shape, gprops);
    auto origin = gprops.CentreOfMass();

    // Get rotation of line
    auto edge = TopoDS::Edge(shape);
    ShapeAnalysis_Edge edgeAnalyzer;
    gp_Pnt firstPoint = BRep_Tool::Pnt(edgeAnalyzer.FirstVertex(edge));
    gp_Pnt lastPoint = BRep_Tool::Pnt(edgeAnalyzer.LastVertex(edge));
    auto dir = (lastPoint.XYZ() - firstPoint.XYZ()).Normalized();
    Base::Vector3d elementDirection(dir.X(), dir.Y(), dir.Z());
    Base::Vector3d axisUp(0.0, 0.0, 1.0);
    Base::Rotation rot(axisUp, elementDirection);

    Base::Placement placement(Base::Vector3d(origin.X(), origin.Y(), origin.Z()), rot);
    return new Measure::MeasureLengthInfo{true, getLength(shape), placement};
}

Measure::MeasureRadiusInfo* MeasureRadiusHandler(std::string* objectName, std::string* subName){
    Base::Placement placement;      // curve center + orientation
    Base::Vector3d pointOnCurve;

    TopoDS_Shape shape = getLocatedShape(*objectName, *subName);

    if (shape.IsNull()) {
        return new Measure::MeasureRadiusInfo{ false, 0.0, pointOnCurve, placement};
    }
        TopAbs_ShapeEnum sType = shape.ShapeType();

    if (sType != TopAbs_EDGE) {
        return new Measure::MeasureRadiusInfo{ false, 0.0, pointOnCurve, placement};
    }

    // Get Center of mass as the attachment point of the label
    GProp_GProps gprops;
    BRepGProp::LinearProperties(shape, gprops);
    auto origin = gprops.CentreOfMass();

    TopoDS_Edge edge = TopoDS::Edge(shape);
    gp_Pnt firstPoint = BRep_Tool::Pnt(TopExp::FirstVertex(edge));
    pointOnCurve = Base::Vector3d(firstPoint.X(), firstPoint.Y(), firstPoint.Z());
    // a somewhat arbitrary radius from center -> point on curve
    auto dir = (firstPoint.XYZ() - origin.XYZ()).Normalized();
    Base::Vector3d elementDirection(dir.X(), dir.Y(), dir.Z());
    Base::Vector3d axisUp(0.0, 0.0, 1.0);
    Base::Rotation rot(axisUp, elementDirection);

    placement = Base::Placement(Base::Vector3d(origin.X(), origin.Y(), origin.Z()), rot);

    return new Measure::MeasureRadiusInfo{ true, getRadius(shape), pointOnCurve, placement};
}


Measure::MeasureAreaInfo* MeasureAreaHandler(std::string* objectName, std::string* subName){
    TopoDS_Shape shape = getLocatedShape(*objectName, *subName);

    if (shape.IsNull()) {
        // failure here on loading document with existing MeasureClientment.
        Base::Console().Message("MeasureClientAreaHandler did not retrieve shape for %s, %s\n", objectName->c_str(), subName->c_str());
        return new Measure::MeasureAreaInfo{false, 0.0, Base::Matrix4D()};
    }
    TopAbs_ShapeEnum sType = shape.ShapeType();

    if (sType != TopAbs_FACE) {
        return new Measure::MeasureAreaInfo{false, 0.0, Base::Matrix4D()};
    }

    // Get Center of mass as the attachment point of the label
    GProp_GProps gprops;
    BRepGProp::SurfaceProperties(shape, gprops);
    auto origin = gprops.CentreOfMass();

    // TODO: Center of Mass might not lie on the surface, somehow snap to the closest point on the surface? 

    Base::Placement placement(Base::Vector3d(origin.X(), origin.Y(), origin.Z()), Base::Rotation());
    return new Measure::MeasureAreaInfo{true, getFaceArea(shape), placement};
}


Measure::MeasurePositionInfo* MeasurePositionHandler(std::string* objectName, std::string* subName) {
    TopoDS_Shape shape = getLocatedShape(*objectName, *subName);

    if (shape.IsNull()) {
        Base::Console().Message("MeasureClientPositionHandler did not retrieve shape for %s, %s\n", objectName->c_str(), subName->c_str());
        return new Measure::MeasurePositionInfo{false, Base::Vector3d()};
    }
    TopAbs_ShapeEnum sType = shape.ShapeType();

    if (sType != TopAbs_VERTEX) {
        return new Measure::MeasurePositionInfo{false, Base::Vector3d()};
    }

    TopoDS_Vertex vertex = TopoDS::Vertex(shape);    
    auto point = BRep_Tool::Pnt(vertex);
    return new Measure::MeasurePositionInfo{true, Base::Vector3d(point.X(), point.Y(), point.Z())};
}


Measure::MeasureAngleInfo* MeasureAngleHandler(std::string* objectName, std::string* subName) {
    App::DocumentObject* ob = App::GetApplication().getActiveDocument()->getObject(objectName->c_str());
    TopoDS_Shape shape = getLocatedShape(*objectName, *subName);
    if (shape.IsNull()) {
        // failure here on loading document with existing MeasureClientment.
        Base::Console().Message("MeasureClientAngleHandler did not retrieve shape for %s, %s\n", objectName->c_str(), subName->c_str());
        return new Measure::MeasureAngleInfo{};
    }

    TopAbs_ShapeEnum sType = shape.ShapeType();

    Part::VectorAdapter vAdapt = buildAdapter(ob, objectName, subName);

    gp_Pnt vec;
    Base::Vector3d position;
    if (sType == TopAbs_FACE) {
        TopoDS_Face face = TopoDS::Face(shape);
        
        GProp_GProps gprops;
        BRepGProp::SurfaceProperties(face, gprops);
        vec = gprops.CentreOfMass();
        
    } else if (sType == TopAbs_EDGE) {
        TopoDS_Edge edge = TopoDS::Edge(shape);

        GProp_GProps gprops;
        BRepGProp::LinearProperties(edge, gprops);
        vec = gprops.CentreOfMass();
    }

    position.Set(vec.X(), vec.Y(), vec.Z());

    Measure::MeasureAngleInfo* info = new Measure::MeasureAngleInfo{vAdapt.isValid(), (Base::Vector3d)vAdapt, position};
    return info;
}


Measure::MeasureDistanceInfo* MeasureDistanceHandler(std::string* objectName, std::string* subName) {
    TopoDS_Shape shape = getLocatedShape(*objectName, *subName);

    if (shape.IsNull()) {
        // failure here on loading document with existing MeasureClientment.
        Base::Console().Message("MeasureClientDistanceHandler did not retrieve shape for %s, %s\n", objectName->c_str(), subName->c_str());
        return new Measure::MeasureDistanceInfo();
    }

    // Just return the TopoDS_Shape here
    return new Measure::MeasureDistanceInfo{true, shape};
}


using namespace Measure;

void Part::MeasureClient::initialize() {

    App::Application& app = App::GetApplication();
    app.addMeasureHandler("Part", PartMeasureTypeCb);
}

void Part::MeasureClient::registerMeasureHandlers( )
{

    std::vector<std::string> proxyList(  { "Part", "PartDesign", "Sketcher" } );

    // Extend MeasureLength
    MeasureLength::addGeometryHandlerCBs(proxyList, MeasureLengthHandler);

    // Extend MeasurePosition
    MeasurePosition::addGeometryHandlerCBs(proxyList, MeasurePositionHandler);

    // Extend MeasureArea
    MeasureArea::addGeometryHandlerCBs(proxyList, MeasureAreaHandler);

    // Extend MeasureAngle
    MeasureAngle::addGeometryHandlerCBs(proxyList, MeasureAngleHandler);

    // Extend MeasureDistance
    MeasureDistance::addGeometryHandlerCBs(proxyList, MeasureDistanceHandler);

    // Extend MeasureRadius
    MeasureRadius::addGeometryHandlerCBs(proxyList, MeasureRadiusHandler);
}
