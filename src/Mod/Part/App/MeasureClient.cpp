/***************************************************************************
 *   Copyright (c) 2023 Wandererfan <wandererfan@gmail.com>                *
 *   Copyright (c) 2023 Joel Meijering (EDG5000) <joel@meijering.email>    *
 *   Copyright (c) 2023 David Friedli <david@friedli-be.ch>                *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


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
#include <BRepBuilderAPI_Copy.hxx>

#include <DatumFeature.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/MeasureManager.h>
#include <App/DocumentObserver.h>
#include <App/GeoFeature.h>
#include <Base/Console.h>
#include <Base/Matrix.h>
#include <Base/Rotation.h>
#include <Base/Vector3D.h>

#include "VectorAdapter.h"
#include "PartFeature.h"

#include "MeasureClient.h"

using namespace Part;


// From: https://github.com/Celemation/FreeCAD/blob/joel_selection_summary_demo/src/Gui/Selection/SelectionSummary.cpp

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

TopoDS_Shape getLocatedShape(const App::SubObjectT& subject, Base::Matrix4D* mat = nullptr)
{
    App::DocumentObject* obj = subject.getSubObjectList().back();
    if (!obj) {
        return {};
    }

    Part::TopoShape shape = Part::Feature::getTopoShape(
        obj,
          Part::ShapeOption::ResolveLink 
        | Part::ShapeOption::Transform,
        subject.getElementName(),
        mat);

    if (shape.isNull()) {
        Base::Console().log("Part::MeasureClient::getLocatedShape: Did not retrieve shape for %s, %s\n", obj->getNameInDocument(), subject.getElementName());
        return {};
    }

    auto placement = App::GeoFeature::getGlobalPlacement(obj, subject.getObject(), subject.getSubName());
    shape.setPlacement(placement);

    // Don't get the subShape from datum elements
    if (obj->isDerivedFrom<Part::Datum>()) {
        return shape.getShape();
    }

    if (!subject.getElementName()) {
        return shape.getShape();
    }
    return shape.getSubShape(subject.getElementName(), true);
}


App::MeasureElementType PartMeasureTypeCb(App::DocumentObject* ob, const char* subName)
{
    TopoDS_Shape shape = Part::Feature::getShape(ob,
                                                    Part::ShapeOption::NeedSubElement
                                                  | Part::ShapeOption::ResolveLink
                                                  | Part::ShapeOption::Transform,
                                                 subName);

    if (shape.IsNull()) {
        // failure here on loading document with existing measurement.
        Base::Console().message("Part::PartMeasureTypeCb did not retrieve shape for %s, %s\n", ob->getNameInDocument(), subName);
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
                    return ob->isDerivedFrom<Part::Datum>() ? App::MeasureElementType::LINE
                                                            : App::MeasureElementType::LINESEGMENT;
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
                default: {
                    return App::MeasureElementType::SURFACE; }
            }
        }
        case TopAbs_SOLID: {
            return App::MeasureElementType::VOLUME;
        }
        default: {
            return App::MeasureElementType::INVALID;
        }
    }
}


bool getShapeFromStrings(TopoDS_Shape &shapeOut, const App::SubObjectT& subject, Base::Matrix4D *mat)
{
    App::DocumentObject *obj = subject.getObject();
    if (!obj) {
        return {};
     }
     shapeOut = Part::Feature::getShape(obj,
                                            Part::ShapeOption::NeedSubElement
                                          | Part::ShapeOption::ResolveLink
                                          | Part::ShapeOption::Transform,
                                        subject.getElementName(),
                                        mat);
     return !shapeOut.IsNull();
}


Part::VectorAdapter buildAdapter(const App::SubObjectT& subject)
{
    Base::Matrix4D mat;
    TopoDS_Shape shape = getLocatedShape(subject, &mat);

    if (shape.IsNull()) {
        // failure here on loading document with existing measurement.
        Base::Console().message("Part::buildAdapter did not retrieve shape for %s, %s\n",
                                subject.getObjectName(), subject.getElementName());
        return Part::VectorAdapter();
    }
    TopAbs_ShapeEnum shapeType = shape.ShapeType();

    if (shapeType == TopAbs_EDGE)
    {
      TopoDS_Edge edge = TopoDS::Edge(shape);
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
      TopoDS_Face face = TopoDS::Face(shape);
      Base::Vector3d vTemp(0.0, 0.0, 0.0); //v(current.x, current.y, current.z);
      vTemp = mat*vTemp;
      gp_Vec pickPoint(vTemp.x, vTemp.y, vTemp.z);
      return {face, pickPoint};
    }

    return {};
}


MeasureLengthInfoPtr MeasureLengthHandler(const App::SubObjectT& subject)
{
    TopoDS_Shape shape = getLocatedShape(subject);

    if (shape.IsNull()) {
        // failure here on loading document with existing measurement.
        Base::Console().message("MeasureLengthHandler did not retrieve shape for %s, %s\n",
                                subject.getObjectName(), subject.getElementName());
        return std::make_shared<MeasureLengthInfo>(false, 0.0, Base::Matrix4D());
    }
    TopAbs_ShapeEnum sType = shape.ShapeType();

    if (sType != TopAbs_EDGE) {
        return std::make_shared<MeasureLengthInfo>(false, 0.0, Base::Matrix4D());
    }

    // Get Center of mass as the attachment point of the label
    GProp_GProps gprops;
    BRepGProp::LinearProperties(shape, gprops);
    auto origin = gprops.CentreOfMass();

    Base::Placement placement(Base::Vector3d(origin.X(), origin.Y(), origin.Z()), Base::Rotation());
    return std::make_shared<MeasureLengthInfo>(true, getLength(shape), placement);
}

MeasureRadiusInfoPtr MeasureRadiusHandler(const App::SubObjectT& subject)
{
    Base::Placement placement;      // curve center + orientation
    Base::Vector3d pointOnCurve;

    TopoDS_Shape shape = getLocatedShape(subject);

    if (shape.IsNull()) {
        return std::make_shared<MeasureRadiusInfo>( false, 0.0, pointOnCurve, placement);
    }
        TopAbs_ShapeEnum sType = shape.ShapeType();

    if (sType != TopAbs_EDGE) {
        return std::make_shared<MeasureRadiusInfo>( false, 0.0, pointOnCurve, placement);
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

    return std::make_shared<MeasureRadiusInfo>( true, getRadius(shape), pointOnCurve, placement);
}


MeasureAreaInfoPtr MeasureAreaHandler(const App::SubObjectT& subject)
{
    TopoDS_Shape shape = getLocatedShape(subject);

    if (shape.IsNull()) {
        // failure here on loading document with existing measurement.
        Base::Console().message("MeasureAreaHandler did not retrieve shape for %s, %s\n",
                                subject.getObjectName(), subject.getElementName());
        return std::make_shared<MeasureAreaInfo>(false, 0.0, Base::Matrix4D());
    }
    TopAbs_ShapeEnum sType = shape.ShapeType();

    if (sType != TopAbs_FACE) {
        return std::make_shared<MeasureAreaInfo>(false, 0.0, Base::Matrix4D());
    }

    // Get Center of mass as the attachment point of the label
    GProp_GProps gprops;
    BRepGProp::SurfaceProperties(shape, gprops);
    auto origin = gprops.CentreOfMass();

    // TODO: Center of Mass might not lie on the surface, somehow snap to the closest point on the surface?

    Base::Placement placement(Base::Vector3d(origin.X(), origin.Y(), origin.Z()), Base::Rotation());
    return std::make_shared<MeasureAreaInfo>(true, getFaceArea(shape), placement);
}


MeasurePositionInfoPtr MeasurePositionHandler(const App::SubObjectT& subject)
{
    TopoDS_Shape shape = getLocatedShape(subject);

    if (shape.IsNull()) {
        Base::Console().message("MeasurePositionHandler did not retrieve shape for %s, %s\n",
                                subject.getObjectName(), subject.getElementName());
        return std::make_shared<MeasurePositionInfo>(false, Base::Vector3d());
    }
    TopAbs_ShapeEnum sType = shape.ShapeType();

    if (sType != TopAbs_VERTEX) {
        return std::make_shared<MeasurePositionInfo>(false, Base::Vector3d());
    }

    TopoDS_Vertex vertex = TopoDS::Vertex(shape);
    auto point = BRep_Tool::Pnt(vertex);
    return std::make_shared<MeasurePositionInfo>( true, Base::Vector3d(point.X(), point.Y(), point.Z()));
}


MeasureAngleInfoPtr MeasureAngleHandler(const App::SubObjectT& subject)
{
    TopoDS_Shape shape = getLocatedShape(subject);

    if (shape.IsNull()) {
        // failure here on loading document with existing measurement.
        Base::Console().message("MeasureAngleHandler did not retrieve shape for %s, %s\n",
                                subject.getObjectName(), subject.getElementName());
        return std::make_shared<MeasureAngleInfo>();
    }

    TopAbs_ShapeEnum sType = shape.ShapeType();

    Part::VectorAdapter vAdapt = buildAdapter(subject);

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

    auto info = std::make_shared<MeasureAngleInfo>(vAdapt.isValid(), (Base::Vector3d)vAdapt, position);
    return info;
}


MeasureDistanceInfoPtr MeasureDistanceHandler(const App::SubObjectT& subject)
{
    TopoDS_Shape shape = getLocatedShape(subject);

    if (shape.IsNull()) {
        // failure here on loading document with existing measurement.
        Base::Console().message("MeasureDistanceHandler did not retrieve shape for %s, %s\n",
                                subject.getObjectName(), subject.getElementName());
        return std::make_shared<MeasureDistanceInfo>();
    }

    // return a persistent copy of the TopoDS_Shape here as shape will go out of scope at end
    BRepBuilderAPI_Copy copy(shape);
    return std::make_shared<MeasureDistanceInfo>(true, copy.Shape());
}


void Part::MeasureClient::initialize() {
    App::MeasureManager::addMeasureHandler("Part", PartMeasureTypeCb);

}

Part::CallbackRegistrationList Part::MeasureClient::reportLengthCB()
{
    CallbackRegistrationList callbacks;
    callbacks.emplace_back("Part", "Length", MeasureLengthHandler);
    callbacks.emplace_back("PartDesign", "Length", MeasureLengthHandler);
    callbacks.emplace_back("Sketcher", "Length", MeasureLengthHandler);
    return callbacks;
}

Part::CallbackRegistrationList Part::MeasureClient::reportPositionCB()
{
    CallbackRegistrationList callbacks;
    callbacks.emplace_back("Part", "Position", MeasurePositionHandler);
    callbacks.emplace_back("PartDesign", "Position", MeasurePositionHandler);
    callbacks.emplace_back("Sketcher", "Position", MeasurePositionHandler);
    return callbacks;
}

Part::CallbackRegistrationList Part::MeasureClient::reportAreaCB()
{
    CallbackRegistrationList callbacks;
    callbacks.emplace_back("Part", "Area", MeasureAreaHandler);
    callbacks.emplace_back("PartDesign", "Area", MeasureAreaHandler);
    callbacks.emplace_back("Sketcher", "Area", MeasureAreaHandler);
    return callbacks;
}


Part::CallbackRegistrationList Part::MeasureClient::reportAngleCB()
{
    CallbackRegistrationList callbacks;
    callbacks.emplace_back("Part", "Angle", MeasureAngleHandler);
    callbacks.emplace_back("PartDesign", "Angle", MeasureAngleHandler);
    callbacks.emplace_back("Sketcher", "Angle", MeasureAngleHandler);
    return callbacks;
}


Part::CallbackRegistrationList Part::MeasureClient::reportDistanceCB()
{
    CallbackRegistrationList callbacks;
    callbacks.emplace_back("Part", "Distance", MeasureDistanceHandler);
    callbacks.emplace_back("PartDesign", "Distance", MeasureDistanceHandler);
    callbacks.emplace_back("Sketcher", "Distance", MeasureDistanceHandler);
    return callbacks;
}


Part::CallbackRegistrationList Part::MeasureClient::reportRadiusCB()
{
    CallbackRegistrationList callbacks;
    callbacks.emplace_back("Part", "Radius", MeasureRadiusHandler);
    callbacks.emplace_back("PartDesign", "Radius", MeasureRadiusHandler);
    callbacks.emplace_back("Sketcher", "Radius", MeasureRadiusHandler);
    return callbacks;
}
