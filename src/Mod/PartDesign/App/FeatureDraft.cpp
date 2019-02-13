/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender[at]users.sourceforge.net>     *
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
# include <BRepOffsetAPI_DraftAngle.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <gp_Dir.hxx>
# include <gp_Pln.hxx>
# include <gp_Ax1.hxx>
//# include <BRepAdaptor_CompCurve.hxx>
# include <gp_Pln.hxx>
# include <gp_Lin.hxx>
# include <gp_Dir.hxx>
# include <gp_Circ.hxx>
# include <GeomAbs_SurfaceType.hxx>
# include <GeomAPI_IntSS.hxx>
# include <Geom_Plane.hxx>
# include <Geom_Curve.hxx>
# include <Geom_Line.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
#endif

#include <App/OriginFeature.h>
#include <Base/Tools.h>
#include <Base/Exception.h>
#include <App/Document.h>
#include <Mod/Part/App/TopoShape.h>

#include "FeatureDraft.h"
#include "DatumLine.h"
#include "DatumPlane.h"


#include <Base/Console.h>
#include <Base/Exception.h>

using namespace PartDesign;

FC_LOG_LEVEL_INIT("PartDesign", true,true)


PROPERTY_SOURCE(PartDesign::Draft, PartDesign::DressUp)

const App::PropertyFloatConstraint::Constraints floatAngle = {0.0,89.99,0.1};

Draft::Draft()
{
    ADD_PROPERTY(Angle,(1.5));
    Angle.setConstraints(&floatAngle);
    ADD_PROPERTY_TYPE(NeutralPlane,(0),"Draft",(App::PropertyType)(App::Prop_None),"NeutralPlane");
    ADD_PROPERTY_TYPE(PullDirection,(0),"Draft",(App::PropertyType)(App::Prop_None),"PullDirection");
    ADD_PROPERTY(Reversed,(0));
}

short Draft::mustExecute() const
{
    if (Placement.isTouched() ||
        Angle.isTouched() ||
        NeutralPlane.isTouched() ||
        PullDirection.isTouched() ||
        Reversed.isTouched())
        return 1;
    return DressUp::mustExecute();
}

App::DocumentObjectExecReturn *Draft::execute(void)
{
    // Get parameters
    // Base shape
    Part::TopoShape baseShape;
    try {
        baseShape = getBaseShape();
    } catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    baseShape.setTransform(Base::Matrix4D());

    // Faces where draft should be applied
    auto faces = getFaces(baseShape);
    if (faces.size() == 0)
        return new App::DocumentObjectExecReturn("No faces specified");

    // Draft angle
    double angle = Angle.getValue() / 180.0 * M_PI;

    // Pull direction
    gp_Dir pullDirection;
    App::DocumentObject* refDirection = PullDirection.getValue();    
    if (refDirection != NULL) {
        if (refDirection->getTypeId().isDerivedFrom(PartDesign::Line::getClassTypeId())) {
                    PartDesign::Line* line = static_cast<PartDesign::Line*>(refDirection);
                    Base::Vector3d d = line->getDirection();
                    pullDirection = gp_Dir(d.x, d.y, d.z);
        } else if (refDirection->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            std::vector<std::string> subStrings = PullDirection.getSubValues(true);
            if (subStrings.empty() || subStrings[0].empty())
                throw Base::ValueError("No pull direction reference specified");

            Part::Feature* refFeature = static_cast<Part::Feature*>(refDirection);
            Part::TopoShape refShape = refFeature->Shape.getShape();
            TopoDS_Shape ref = refShape.getSubShape(subStrings[0].c_str());

            if (ref.ShapeType() == TopAbs_EDGE) {
                TopoDS_Edge refEdge = TopoDS::Edge(ref);
                if (refEdge.IsNull())
                    throw Base::ValueError("Failed to extract pull direction reference edge");
                BRepAdaptor_Curve adapt(refEdge);
                if (adapt.GetType() != GeomAbs_Line)
                    throw Base::TypeError("Pull direction reference edge must be linear");

                pullDirection = adapt.Line().Direction();
            } else {
                throw Base::TypeError("Pull direction reference must be an edge or a datum line");
            }
        } else {
            throw Base::TypeError("Pull direction reference must be an edge of a feature or a datum line");
        }

        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        pullDirection.Transform(invObjLoc.Transformation());
    }

    // Neutral plane
    gp_Pln neutralPlane;
    App::DocumentObject* refPlane = NeutralPlane.getValue();
    if (refPlane == NULL) {
        // Try to guess a neutral plane from the first selected face
        // Get edges of first selected face
        TopoDS_Shape face = TopoDS::Face(faces[0].getShape());
        TopTools_IndexedMapOfShape mapOfEdges;
        TopExp::MapShapes(face, TopAbs_EDGE, mapOfEdges);
        bool found = false;

        for (int i = 1; i <= mapOfEdges.Extent(); i++) {
            // Note: What happens if mapOfEdges(i) is the degenerated edge of a cone?
            // But in that case the draft is not possible anyway!
            BRepAdaptor_Curve c(TopoDS::Edge(mapOfEdges(i)));
            gp_Pnt p1 = c.Value(c.FirstParameter());
            gp_Pnt p2 = c.Value(c.LastParameter());

            if (c.IsClosed()) {
                // Edge is a circle or a circular arc (other types are not allowed for drafting)
                if (c.GetType() == GeomAbs_Circle) {
                    neutralPlane = gp_Pln(p1, c.Circle().Axis().Direction());
                    found = true;
                    FC_LOG("guess draft neutral plane using Edge" << i);
                    break;
                }
            } else {
                // Edge is linear
                // Find midpoint of edge and create auxiliary plane through midpoint normal to edge
                gp_Pnt pm = c.Value((c.FirstParameter() + c.LastParameter()) / 2.0);
                Handle(Geom_Plane) aux = new Geom_Plane(pm, gp_Dir(p2.X() - p1.X(), p2.Y() - p1.Y(), p2.Z() - p1.Z()));
                // Intersect plane with face. Is there no easier way?
                BRepAdaptor_Surface adapt(TopoDS::Face(face), Standard_False);
                Handle(Geom_Surface) sf = adapt.Surface().Surface();
                GeomAPI_IntSS intersector(aux, sf, Precision::Confusion());
                if (!intersector.IsDone())
                    continue;
                Handle(Geom_Curve) icurve = intersector.Line(1);
                if (!icurve->IsKind(STANDARD_TYPE(Geom_Line)))
                    continue;
                // TODO: How to extract the line from icurve without creating an edge first?
                TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(icurve);
                BRepAdaptor_Curve c(edge);
                neutralPlane = gp_Pln(pm, c.Line().Direction());
                found = true;
                FC_LOG("guess draft neutral plane using Edge" << i);
                break;
            }
        }

        if (!found)
            throw Base::RuntimeError("No neutral plane specified and none can be guessed");
    } else {
        if (refPlane->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId())) {
            PartDesign::Plane* plane = static_cast<PartDesign::Plane*>(refPlane);
            Base::Vector3d b = plane->getBasePoint();
            Base::Vector3d n = plane->getNormal();
            neutralPlane = gp_Pln(gp_Pnt(b.x, b.y, b.z), gp_Dir(n.x, n.y, n.z));
        } else if (refPlane->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
            neutralPlane = Feature::makePlnFromPlane(refPlane);
        } else if (refPlane->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            std::vector<std::string> subStrings = NeutralPlane.getSubValues(true);
            if (subStrings.empty() || subStrings[0].empty())
                throw Base::ValueError("No neutral plane reference specified");

            Part::Feature* refFeature = static_cast<Part::Feature*>(refPlane);
            Part::TopoShape refShape = refFeature->Shape.getShape();
            TopoDS_Shape ref = refShape.getSubShape(subStrings[0].c_str());

            if (ref.ShapeType() == TopAbs_FACE) {
                TopoDS_Face refFace = TopoDS::Face(ref);
                if (refFace.IsNull())
                    throw Base::ValueError("Failed to extract neutral plane reference face");
                BRepAdaptor_Surface adapt(refFace);
                if (adapt.GetType() != GeomAbs_Plane)
                    throw Base::TypeError("Neutral plane reference face must be planar");

                neutralPlane = adapt.Plane();
            } else if (ref.ShapeType() == TopAbs_EDGE) {
                if (refDirection != NULL) {
                    // Create neutral plane through edge normal to pull direction
                    TopoDS_Edge refEdge = TopoDS::Edge(ref);
                    if (refEdge.IsNull())
                        throw Base::ValueError("Failed to extract neutral plane reference edge");
                    BRepAdaptor_Curve c(refEdge);
                    if (c.GetType() != GeomAbs_Line)
                        throw Base::TypeError("Neutral plane reference edge must be linear");
                    double a = c.Line().Angle(gp_Lin(c.Value(c.FirstParameter()), pullDirection));
                    if (std::fabs(a - M_PI_2) > Precision::Confusion())
                        throw Base::ValueError("Neutral plane reference edge must be normal to pull direction");
                    neutralPlane = gp_Pln(c.Value(c.FirstParameter()), pullDirection);
                } else {
                    throw Base::TypeError("Neutral plane reference can only be an edge if pull direction is defined");
                }
            } else {
                throw Base::TypeError("Neutral plane reference must be a face");
            }
        } else {
            throw Base::TypeError("Neutral plane reference must be face of a feature or a datum plane");
        }

        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        neutralPlane.Transform(invObjLoc.Transformation());
    }

    if (refDirection == NULL) {
        // Choose pull direction normal to neutral plane
        pullDirection = neutralPlane.Axis().Direction();
    }

    // Reversed pull direction
    bool reversed = Reversed.getValue();
    if (reversed)
        angle *= -1.0;

    this->positionByBaseFeature();
    try {
        // Note:
        // LocOpe_SplitDrafts can split a face with a wire and apply draft to both parts
        //       Not clear though whether the face must have free boundaries
        // LocOpe_DPrism can create a stand-alone draft prism. The sketch can only have a single
        //       wire, though.
        // BRepFeat_MakeDPrism requires a support for the operation but will probably support multiple
        //       wires in the sketch
        TopoShape shape(0,getDocument()->getStringHasher());
        try {
            shape.makEDraft(baseShape,faces,pullDirection,angle,neutralPlane);
        }catch(Standard_Failure &e) {
            std::ostringstream ss;
            ss << "Failed to create draft: " << e.GetMessageString();
            return new App::DocumentObjectExecReturn(ss.str().c_str());
        }
        if (shape.isNull())
            return new App::DocumentObjectExecReturn("Resulting shape is null");

        this->Shape.setValue(getSolid(shape));
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}
