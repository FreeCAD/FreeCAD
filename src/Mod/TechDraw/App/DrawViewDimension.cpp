/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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
# include <sstream>
# include <cstring>
# include <cstdlib>
# include <exception>
# include <QString>
# include <QStringList>
# include <QRegExp>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepBndLib.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#endif

#include <QLocale>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Quantity.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include <Mod/Measure/App/Measurement.h>

#include "Preferences.h"
#include "Geometry.h"
#include "DrawViewPart.h"
#include "DrawViewDimension.h"
#include "DrawUtil.h"
#include "LineGroup.h"


#include <Mod/TechDraw/App/DrawViewDimensionPy.h>  // generated from DrawViewDimensionPy.xml

using namespace TechDraw;

//===========================================================================
// DrawViewDimension
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewDimension, TechDraw::DrawView)

namespace {
    // keep this enum synchronized with TypeEnums
    enum DrawViewType {
        Distance,
        DistanceX,
        DistanceY,
        DistanceZ,
        Radius,
        Diameter,
        Angle,
        Angle3Pt
    };
}

const char* DrawViewDimension::TypeEnums[]= {"Distance",
                                             "DistanceX",
                                             "DistanceY",
                                             "DistanceZ",
                                             "Radius",
                                             "Diameter",
                                             "Angle",
                                             "Angle3Pt",
                                             NULL};

const char* DrawViewDimension::MeasureTypeEnums[]= {"True",
                                                    "Projected",
                                                    NULL};

// constraint to set the step size to 0.1
static const App::PropertyQuantityConstraint::Constraints ToleranceConstraint = { -DBL_MAX, DBL_MAX, 0.1 };
// constraint to force positive values
static const App::PropertyQuantityConstraint::Constraints PositiveConstraint = { 0.0, DBL_MAX, 0.1 };

DrawViewDimension::DrawViewDimension(void)
{
    ADD_PROPERTY_TYPE(References2D, (0,0), "", (App::Prop_None), "Projected Geometry References");
    References2D.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(References3D, (0,0), "", (App::Prop_None), "3D Geometry References");
    References3D.setScope(App::LinkScope::Global);

    ADD_PROPERTY_TYPE(FormatSpec, (getDefaultFormatSpec()), "Format", App::Prop_Output,"Dimension format");
    ADD_PROPERTY_TYPE(FormatSpecOverTolerance, (getDefaultFormatSpec(true)), "Format", App::Prop_Output, "Dimension overtolerance format");
    ADD_PROPERTY_TYPE(FormatSpecUnderTolerance, (getDefaultFormatSpec(true)), "Format", App::Prop_Output, "Dimension undertolerance format");
    ADD_PROPERTY_TYPE(Arbitrary,(false), "Format", App::Prop_Output, "Value overridden by user");
    ADD_PROPERTY_TYPE(ArbitraryTolerances, (false), "Format", App::Prop_Output, "Tolerance values overridden by user");

    Type.setEnums(TypeEnums);                                         //dimension type: length, radius etc
    ADD_PROPERTY(Type, ((long)0));
    MeasureType.setEnums(MeasureTypeEnums);
    ADD_PROPERTY(MeasureType, ((long)1));                             //Projected (or True) measurement
    ADD_PROPERTY_TYPE(TheoreticalExact,(false), "", App::Prop_Output, "If theoretical exact (basic) dimension");
    ADD_PROPERTY_TYPE(EqualTolerance, (true), "", App::Prop_Output, "If over- and undertolerance are equal");

    ADD_PROPERTY_TYPE(OverTolerance, (0.0), "", App::Prop_Output, "Overtolerance value\nIf 'Equal Tolerance' is true this is also\nthe negated value for 'Under Tolerance'");
    OverTolerance.setUnit(Base::Unit::Length);
    OverTolerance.setConstraints(&ToleranceConstraint);
    ADD_PROPERTY_TYPE(UnderTolerance, (0.0), "", App::Prop_Output, "Undertolerance value\nIf 'Equal Tolerance' is true it will be replaced\nby negative value of 'Over Tolerance'");
    UnderTolerance.setUnit(Base::Unit::Length);
    UnderTolerance.setConstraints(&ToleranceConstraint);
    ADD_PROPERTY_TYPE(Inverted, (false), "", App::Prop_Output, "The dimensional value is displayed inverted");

    // hide the DrawView properties that don't apply to Dimensions
    ScaleType.setStatus(App::Property::ReadOnly, true);
    ScaleType.setStatus(App::Property::Hidden, true);
    Scale.setStatus(App::Property::ReadOnly, true);
    Scale.setStatus(App::Property::Hidden, true);
    Rotation.setStatus(App::Property::ReadOnly, true);
    Rotation.setStatus(App::Property::Hidden, true);
    Caption.setStatus(App::Property::Hidden, true);
    LockPosition.setStatus(App::Property::Hidden, true);

    // by default EqualTolerance is true, thus make UnderTolerance read-only
    UnderTolerance.setStatus(App::Property::ReadOnly, true);
    FormatSpecUnderTolerance.setStatus(App::Property::ReadOnly, true);

    measurement = new Measure::Measurement();
    //TODO: should have better initial datumLabel position than (0,0) in the DVP?? something closer to the object being measured?

    //initialize the descriptive geometry.
    //TODO: should this be more like DVP with a "geometry object"?
    m_linearPoints.first  = Base::Vector3d(0,0,0);
    m_linearPoints.second = Base::Vector3d(0,0,0);

    m_anglePoints.ends.first = Base::Vector3d(0,0,0);
    m_anglePoints.ends.second = Base::Vector3d(0,0,0);
    m_anglePoints.vertex = Base::Vector3d(0,0,0);

    m_arcPoints.isArc = false;
    m_arcPoints.center = Base::Vector3d(0,0,0);
    m_arcPoints.onCurve.first = Base::Vector3d(0,0,0);
    m_arcPoints.onCurve.second = Base::Vector3d(0,0,0);
    m_arcPoints.arcEnds.first = Base::Vector3d(0,0,0);
    m_arcPoints.arcEnds.second = Base::Vector3d(0,0,0);
    m_arcPoints.midArc = Base::Vector3d(0,0,0);
    m_arcPoints.arcCW = false;
    m_hasGeometry = false;
}

DrawViewDimension::~DrawViewDimension()
{
    delete measurement;
    measurement = 0;
}

void DrawViewDimension::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &MeasureType) {
            if (MeasureType.isValue("True") && !measurement->has3DReferences()) {
                Base::Console().Warning("%s has no 3D References but is Type: True\n", getNameInDocument());
                MeasureType.setValue("Projected");
            }
        }
        else if (prop == &References3D) {   //have to rebuild the Measurement object
//            Base::Console().Message("DVD::onChanged - References3D\n");
            clear3DMeasurements();                             //Measurement object
            if (!(References3D.getValues()).empty()) {
                setAll3DMeasurement();
            } else {
                if (MeasureType.isValue("True")) {             //empty 3dRefs, but True
                    MeasureType.touch();                       //run MeasureType logic for this case
                }
            }
        }
        else if (prop == &Type) {                                    //why??
            FormatSpec.setValue(getDefaultFormatSpec().c_str());

            DrawViewType type = static_cast<DrawViewType>(Type.getValue());
            if (type == DrawViewType::Angle || type == DrawViewType::Angle3Pt) {
                OverTolerance.setUnit(Base::Unit::Angle);
                UnderTolerance.setUnit(Base::Unit::Angle);
            }
            else {
                OverTolerance.setUnit(Base::Unit::Length);
                UnderTolerance.setUnit(Base::Unit::Length);
            }
        }
        else if (prop == &TheoreticalExact) {
            // if TheoreticalExact disable tolerances and set them to zero
            if (TheoreticalExact.getValue()) {
                OverTolerance.setValue(0.0);
                UnderTolerance.setValue(0.0);
                OverTolerance.setReadOnly(true);
                UnderTolerance.setReadOnly(true);
                FormatSpecOverTolerance.setReadOnly(true);
                FormatSpecUnderTolerance.setReadOnly(true);
                ArbitraryTolerances.setValue(false);
                ArbitraryTolerances.setReadOnly(true);
            }
            else {
                OverTolerance.setReadOnly(false);
                FormatSpecOverTolerance.setReadOnly(false);
                ArbitraryTolerances.setReadOnly(false);
                if (!EqualTolerance.getValue()) {
                    UnderTolerance.setReadOnly(false);
                    FormatSpecUnderTolerance.setReadOnly(false);
                }
            }
            requestPaint();
        }
        else if (prop == &EqualTolerance) {
            // if EqualTolerance set negated overtolerance for untertolerance
            // then also the OverTolerance must be positive
            if (EqualTolerance.getValue()) {
                // if OverTolerance is negative or zero, first set it to zero
                if (OverTolerance.getValue() < 0) {
                    OverTolerance.setValue(0.0);
                }
                OverTolerance.setConstraints(&PositiveConstraint);
                UnderTolerance.setValue(-1.0 * OverTolerance.getValue());
                UnderTolerance.setUnit(OverTolerance.getUnit());
                UnderTolerance.setReadOnly(true);
                FormatSpecUnderTolerance.setValue(FormatSpecOverTolerance.getValue());
                FormatSpecUnderTolerance.setReadOnly(true);
            }
            else {
                OverTolerance.setConstraints(&ToleranceConstraint);
                if (!TheoreticalExact.getValue()) {
                    UnderTolerance.setReadOnly(false);
                    FormatSpecUnderTolerance.setReadOnly(false);
                }
            }
            requestPaint();
        } 
        else if (prop == &OverTolerance) {
            // if EqualTolerance set negated overtolerance for untertolerance
            if (EqualTolerance.getValue()) {
                UnderTolerance.setValue(-1.0 * OverTolerance.getValue());
                UnderTolerance.setUnit(OverTolerance.getUnit());
            }
            requestPaint();
        }
        else if (prop == &FormatSpecOverTolerance) {
            if (!ArbitraryTolerances.getValue()) {
                FormatSpecUnderTolerance.setValue(FormatSpecOverTolerance.getValue());
            }
            requestPaint();
        }
        else if (prop == &FormatSpecUnderTolerance) {
            if (!ArbitraryTolerances.getValue()) {
                FormatSpecOverTolerance.setValue(FormatSpecUnderTolerance.getValue());
            }
            requestPaint();
        }
        else if ( (prop == &FormatSpec) ||
             (prop == &Arbitrary) ||
             (prop == &ArbitraryTolerances) ||
             (prop == &MeasureType) ||
             (prop == &UnderTolerance) ||
             (prop == &Inverted) ) {
            requestPaint();
        }
    }

    DrawView::onChanged(prop);
}

void DrawViewDimension::Restore(Base::XMLReader& reader)
// Old drawings did not have the equal tolerance options.
// We cannot just introduce it as being set to true because that would e.g. destroy tolerances like +1-2
// Therefore set it to false for existing documents
{
    EqualTolerance.setValue(false);
    DrawView::Restore(reader);
}

void DrawViewDimension::onDocumentRestored()
{
    if (has3DReferences()) {
        setAll3DMeasurement();
    }

    DrawViewType type = static_cast<DrawViewType>(Type.getValue());
    if (type == DrawViewType::Angle || type == DrawViewType::Angle3Pt) {
        OverTolerance.setUnit(Base::Unit::Angle);
        UnderTolerance.setUnit(Base::Unit::Angle);
    }
}

void DrawViewDimension::handleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, App::Property * prop)
{
    if (prop == &OverTolerance && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat v;
        v.Restore(reader);
        OverTolerance.setValue(v.getValue());
    }
    else if (prop == &UnderTolerance && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat v;
        v.Restore(reader);
        UnderTolerance.setValue(v.getValue());
    }
    else {
        TechDraw::DrawView::handleChangedPropertyType(reader, TypeName, prop);
    }

    // Over/Undertolerance were further changed from App::PropertyQuantity to App::PropertyQuantityConstraint
    if ((prop == &OverTolerance) && (strcmp(TypeName, "App::PropertyQuantity") == 0)) {
        App::PropertyQuantity OverToleranceProperty;
        // restore the PropertyQuantity to be able to set its value
        OverToleranceProperty.Restore(reader);
        OverTolerance.setValue(OverToleranceProperty.getValue());
    }
    else if ((prop == &UnderTolerance) && (strcmp(TypeName, "App::PropertyQuantity") == 0)) {
        App::PropertyQuantity UnderToleranceProperty;
        // restore the PropertyQuantity to be able to set its value
        UnderToleranceProperty.Restore(reader);
        UnderTolerance.setValue(UnderToleranceProperty.getValue());
    }
}


short DrawViewDimension::mustExecute() const
{
    bool result = 0;
    if (!isRestoring()) {
        result = (References2D.isTouched() ||
                  Type.isTouched() ||
                  FormatSpec.isTouched() ||
                  Arbitrary.isTouched() ||
                  FormatSpecOverTolerance.isTouched() ||
                  FormatSpecUnderTolerance.isTouched() ||
                  ArbitraryTolerances.isTouched() ||
                  MeasureType.isTouched() ||
                  TheoreticalExact.isTouched() ||
                  EqualTolerance.isTouched() ||
                  OverTolerance.isTouched() ||
                  UnderTolerance.isTouched() ||
                  Inverted.isTouched() );
    }
    if (result) {
        return result;
    }

    return DrawView::mustExecute();
}

App::DocumentObjectExecReturn *DrawViewDimension::execute(void)
{
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }
    DrawViewPart* dvp = getViewPart();
    if (dvp == nullptr) {
        return App::DocumentObject::StdReturn;
    }

    if (!has2DReferences()) {                                            //too soon?
        if (isRestoring() ||
            getDocument()->testStatus(App::Document::Status::Restoring)) {
            return App::DocumentObject::StdReturn;
        } else {
            Base::Console().Warning("%s has no 2D References\n", getNameInDocument());
        }
        return App::DocumentObject::StdReturn;
    }

    //can't do anything until Source has geometry
    if (!getViewPart()->hasGeometry()) {                              //happens when loading saved document
        //if (isRestoring() ||
        //    getDocument()->testStatus(App::Document::Status::Restoring)) {
            return App::DocumentObject::StdReturn;
        //} else {
        //    return App::DocumentObject::StdReturn;
        //}
    }

    //now we can check if Reference2ds have valid targets.
    if (!checkReferences2D()) {
        Base::Console().Warning("%s has invalid 2D References\n", getNameInDocument());
        return App::DocumentObject::StdReturn;
    }

    const std::vector<std::string> &subElements = References2D.getSubValues();

    if ( Type.isValue("Distance")  ||
         Type.isValue("DistanceX") ||
         Type.isValue("DistanceY") )  {
        if (getRefType() == oneEdge) {
            m_linearPoints = getPointsOneEdge();
        } else if (getRefType() == twoEdge) {
            m_linearPoints = getPointsTwoEdges();
        } else if (getRefType() == twoVertex) {
            m_linearPoints = getPointsTwoVerts();
        } else if (getRefType() == vertexEdge) {
            m_linearPoints = getPointsEdgeVert();
        }
        m_hasGeometry = true;
    } else if (Type.isValue("Radius")){
        int idx = DrawUtil::getIndexFromName(subElements[0]);
        TechDraw::BaseGeom* base = getViewPart()->getGeomByIndex(idx);
        TechDraw::Circle* circle;
        arcPoints pts;
        pts.center = Base::Vector3d(0.0,0.0,0.0);
        pts.radius = 0.0;
        if ( (base && base->geomType == TechDraw::GeomType::CIRCLE) ||
           (base && base->geomType == TechDraw::GeomType::ARCOFCIRCLE))  {
            circle = static_cast<TechDraw::Circle*> (base);
            pts.center = Base::Vector3d(circle->center.x,circle->center.y,0.0);
            pts.radius = circle->radius;
            if (base->geomType == TechDraw::GeomType::ARCOFCIRCLE) {
                TechDraw::AOC* aoc = static_cast<TechDraw::AOC*> (circle);
                pts.isArc = true;
                pts.onCurve.first  = Base::Vector3d(aoc->midPnt.x,aoc->midPnt.y,0.0);
                pts.midArc         = Base::Vector3d(aoc->midPnt.x,aoc->midPnt.y,0.0);
                pts.arcEnds.first  = Base::Vector3d(aoc->startPnt.x,aoc->startPnt.y,0.0);
                pts.arcEnds.second = Base::Vector3d(aoc->endPnt.x,aoc->endPnt.y,0.0);
                pts.arcCW          = aoc->cw;
            } else {
                pts.isArc = false;
                pts.onCurve.first  = pts.center + Base::Vector3d(1,0,0) * circle->radius;   //arbitrary point on edge
                pts.onCurve.second = pts.center + Base::Vector3d(-1,0,0) * circle->radius;  //arbitrary point on edge
            }
        } else if ((base && base->geomType == TechDraw::GeomType::ELLIPSE)  ||
                   (base && base->geomType == TechDraw::GeomType::ARCOFELLIPSE))  {
            TechDraw::Ellipse* ellipse = static_cast<TechDraw::Ellipse*> (base);
            if (ellipse->closed()) {
                double r1 = ellipse->minor;
                double r2 = ellipse->major;
                double rAvg = (r1 + r2) / 2.0;
                pts.center = Base::Vector3d(ellipse->center.x,
                                      ellipse->center.y,
                                      0.0);
                pts.radius = rAvg;
                pts.isArc = false;
                pts.onCurve.first  = pts.center + Base::Vector3d(1,0,0) * rAvg;   //arbitrary point on edge
                pts.onCurve.second = pts.center + Base::Vector3d(-1,0,0) * rAvg;  //arbitrary point on edge
            } else {
                TechDraw::AOE* aoe = static_cast<TechDraw::AOE*> (base);
                double r1 = aoe->minor;
                double r2 = aoe->major;
                double rAvg = (r1 + r2) / 2.0;
                pts.isArc = true;
                pts.center = Base::Vector3d(aoe->center.x,
                                      aoe->center.y,
                                      0.0);
                pts.radius = rAvg;
                pts.arcEnds.first  = Base::Vector3d(aoe->startPnt.x,aoe->startPnt.y,0.0);
                pts.arcEnds.second = Base::Vector3d(aoe->endPnt.x,aoe->endPnt.y,0.0);
                pts.midArc         = Base::Vector3d(aoe->midPnt.x,aoe->midPnt.y,0.0);
                pts.arcCW          = aoe->cw;
                pts.onCurve.first  = Base::Vector3d(aoe->midPnt.x,aoe->midPnt.y,0.0);
                pts.onCurve.second = pts.center + Base::Vector3d(-1,0,0) * rAvg;  //arbitrary point on edge
            }
        } else if (base && base->geomType == TechDraw::GeomType::BSPLINE) {
            TechDraw::BSpline* spline = static_cast<TechDraw::BSpline*> (base);
            if (spline->isCircle()) {
                bool circ,arc;
                double rad;
                Base::Vector3d center;
                spline->getCircleParms(circ,rad,center,arc);
                pts.center = Base::Vector3d(center.x,center.y,0.0);
                pts.radius = rad;
                pts.arcEnds.first  = Base::Vector3d(spline->startPnt.x,spline->startPnt.y,0.0);
                pts.arcEnds.second = Base::Vector3d(spline->endPnt.x,spline->endPnt.y,0.0);
                pts.midArc         = Base::Vector3d(spline->midPnt.x,spline->midPnt.y,0.0);
                pts.isArc = arc;
                pts.arcCW          = spline->cw;
                if (arc) {
                    pts.onCurve.first  = Base::Vector3d(spline->midPnt.x,spline->midPnt.y,0.0);
                } else {
                    pts.onCurve.first  = pts.center + Base::Vector3d(1,0,0) * rad;   //arbitrary point on edge
                    pts.onCurve.second = pts.center + Base::Vector3d(-1,0,0) * rad;  //arbitrary point on edge
                }
            } else {
                //fubar - can't have non-circular spline as target of Radius dimension
                Base::Console().Error("Dimension %s refers to invalid BSpline\n",getNameInDocument());
                return App::DocumentObject::StdReturn;
            }
        } else {
            Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
            return App::DocumentObject::StdReturn;
        }
        m_arcPoints = pts;
        m_hasGeometry = true;
    } else if (Type.isValue("Diameter")){
        int idx = DrawUtil::getIndexFromName(subElements[0]);
        TechDraw::BaseGeom* base = getViewPart()->getGeomByIndex(idx);
        TechDraw::Circle* circle;
        arcPoints pts;
        pts.center = Base::Vector3d(0.0,0.0,0.0);
        pts.radius = 0.0;
        if ((base && base->geomType == TechDraw::GeomType::CIRCLE) ||
           (base && base->geomType == TechDraw::GeomType::ARCOFCIRCLE)) {
            circle = static_cast<TechDraw::Circle*> (base);
            pts.center = Base::Vector3d(circle->center.x,circle->center.y,0.0);
            pts.radius = circle->radius;
            if (base->geomType == TechDraw::GeomType::ARCOFCIRCLE) {
                TechDraw::AOC* aoc = static_cast<TechDraw::AOC*> (circle);
                pts.isArc = true;
                pts.onCurve.first  = Base::Vector3d(aoc->midPnt.x,aoc->midPnt.y,0.0);
                pts.midArc         = Base::Vector3d(aoc->midPnt.x,aoc->midPnt.y,0.0);
                pts.arcEnds.first  = Base::Vector3d(aoc->startPnt.x,aoc->startPnt.y,0.0);
                pts.arcEnds.second = Base::Vector3d(aoc->endPnt.x,aoc->endPnt.y,0.0);
                pts.arcCW          = aoc->cw;
            } else {
                pts.isArc = false;
                pts.onCurve.first  = pts.center + Base::Vector3d(1,0,0) * circle->radius;   //arbitrary point on edge
                pts.onCurve.second = pts.center + Base::Vector3d(-1,0,0) * circle->radius;  //arbitrary point on edge
            }
        } else if ( (base && base->geomType == TechDraw::GeomType::ELLIPSE) ||
                    (base && base->geomType == TechDraw::GeomType::ARCOFELLIPSE) )  {
            TechDraw::Ellipse* ellipse = static_cast<TechDraw::Ellipse*> (base);
            if (ellipse->closed()) {
                double r1 = ellipse->minor;
                double r2 = ellipse->major;
                double rAvg = (r1 + r2) / 2.0;
                pts.center = Base::Vector3d(ellipse->center.x,
                                      ellipse->center.y,
                                      0.0);
                pts.radius = rAvg;
                pts.isArc = false;
                pts.onCurve.first  = pts.center + Base::Vector3d(1,0,0) * rAvg;   //arbitrary point on edge
                pts.onCurve.second = pts.center + Base::Vector3d(-1,0,0) * rAvg;  //arbitrary point on edge
           } else {
                TechDraw::AOE* aoe = static_cast<TechDraw::AOE*> (base);
                double r1 = aoe->minor;
                double r2 = aoe->major;
                double rAvg = (r1 + r2) / 2.0;
                pts.isArc = true;
                pts.center = Base::Vector3d(aoe->center.x,
                                      aoe->center.y,
                                      0.0);
                pts.radius = rAvg;
                pts.arcEnds.first  = Base::Vector3d(aoe->startPnt.x,aoe->startPnt.y,0.0);
                pts.arcEnds.second = Base::Vector3d(aoe->endPnt.x,aoe->endPnt.y,0.0);
                pts.midArc         = Base::Vector3d(aoe->midPnt.x,aoe->midPnt.y,0.0);
                pts.arcCW          = aoe->cw;
                pts.onCurve.first  = Base::Vector3d(aoe->midPnt.x,aoe->midPnt.y,0.0);
                pts.onCurve.second = pts.center + Base::Vector3d(-1,0,0) * rAvg;  //arbitrary point on edge
           }
        } else if (base && base->geomType == TechDraw::GeomType::BSPLINE) {
            TechDraw::BSpline* spline = static_cast<TechDraw::BSpline*> (base);
            if (spline->isCircle()) {
                bool circ,arc;
                double rad;
                Base::Vector3d center;
                spline->getCircleParms(circ,rad,center,arc);
                pts.center = Base::Vector3d(center.x,center.y,0.0);
                pts.radius = rad;
                pts.arcEnds.first  = Base::Vector3d(spline->startPnt.x,spline->startPnt.y,0.0);
                pts.arcEnds.second = Base::Vector3d(spline->endPnt.x,spline->endPnt.y,0.0);
                pts.midArc         = Base::Vector3d(spline->midPnt.x,spline->midPnt.y,0.0);
                pts.isArc = arc;
                pts.arcCW          = spline->cw;
                if (arc) {
                    pts.onCurve.first  = Base::Vector3d(spline->midPnt.x,spline->midPnt.y,0.0);
                } else {
                    pts.onCurve.first  = pts.center + Base::Vector3d(1,0,0) * rad;   //arbitrary point on edge
                    pts.onCurve.second = pts.center + Base::Vector3d(-1,0,0) * rad;  //arbitrary point on edge
                }
            } else {
                //fubar - can't have non-circular spline as target of Diameter dimension
                Base::Console().Error("%s: can not make a Circle from this BSpline edge\n",getNameInDocument());
                return App::DocumentObject::StdReturn;
            }
        } else {
            Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
            return App::DocumentObject::StdReturn;
        }
        m_arcPoints = pts;
        m_hasGeometry = true;
    } else if (Type.isValue("Angle")){
        if (getRefType() != twoEdge) {
             Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
             return App::DocumentObject::StdReturn;
        }
        int idx0 = DrawUtil::getIndexFromName(subElements[0]);
        int idx1 = DrawUtil::getIndexFromName(subElements[1]);
        TechDraw::BaseGeom* edge0 = getViewPart()->getGeomByIndex(idx0);
        TechDraw::BaseGeom* edge1 = getViewPart()->getGeomByIndex(idx1);
        TechDraw::Generic *gen0;
        TechDraw::Generic *gen1;
        if (edge0 && edge0->geomType == TechDraw::GeomType::GENERIC) {
             gen0 = static_cast<TechDraw::Generic*>(edge0);
        } else {
             Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
             return App::DocumentObject::StdReturn;
        }
        if (edge1 && edge1->geomType == TechDraw::GeomType::GENERIC) {
             gen1 = static_cast<TechDraw::Generic*>(edge1);
        } else {
             Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
             return App::DocumentObject::StdReturn;
        }

        anglePoints pts;
        Base::Vector3d apex = gen0->apparentInter(gen1);
        Base::Vector3d extPoint0,extPoint1;
        if ((gen0->getStartPoint() - apex).Length() >
            (gen0->getEndPoint() - apex).Length()) {
            extPoint0 = gen0->getStartPoint();
        } else {
            extPoint0 = gen0->getEndPoint();
        }
        if ((gen1->getStartPoint() - apex).Length() >
            (gen1->getEndPoint() - apex).Length()) {
            extPoint1 = gen1->getStartPoint();
        } else {
            extPoint1 = gen1->getEndPoint();
        }
        pts.ends.first  = extPoint0;
        pts.ends.second = extPoint1;
        pts.vertex = apex;
        m_anglePoints = pts;
        m_hasGeometry = true;
    } else if (Type.isValue("Angle3Pt")){
        if (getRefType() != threeVertex) {
             Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
             return App::DocumentObject::StdReturn;
        }
        int idx0 = DrawUtil::getIndexFromName(subElements[0]);
        int idx1 = DrawUtil::getIndexFromName(subElements[1]);
        int idx2 = DrawUtil::getIndexFromName(subElements[2]);

        TechDraw::VertexPtr vert0 = getViewPart()->getProjVertexByIndex(idx0);
        TechDraw::VertexPtr vert1 = getViewPart()->getProjVertexByIndex(idx1);
        TechDraw::VertexPtr vert2 = getViewPart()->getProjVertexByIndex(idx2);
        if (!vert0 || !vert1 || !vert2) {
             Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n",getNameInDocument());
             return App::DocumentObject::StdReturn;
        }

        anglePoints pts;
        Base::Vector3d apex =  vert1->point();
        Base::Vector3d extPoint0 = vert0->point();
        Base::Vector3d extPoint2 = vert2->point();
        pts.ends.first  = extPoint0;
        pts.ends.second = extPoint2;
        pts.vertex = apex;
        m_anglePoints = pts;
        m_hasGeometry = true;
    }

    //TODO: if MeasureType = Projected and the Projected shape changes, the Dimension may become invalid (see tilted Cube example)
    return DrawView::execute();
}

bool DrawViewDimension::isMultiValueSchema(void) const
{
    bool result = false;
    bool angularMeasure = false;
    if ( (Type.isValue("Angle")) ||
         (Type.isValue("Angle3Pt")) ) {
        angularMeasure = true;
    }

    Base::UnitSystem uniSys = Base::UnitsApi::getSchema();
    if ((uniSys == Base::UnitSystem::ImperialBuilding) &&
            !angularMeasure) {
        result = true;
    } else if ((uniSys == Base::UnitSystem::ImperialCivil) &&
               !angularMeasure) {
        result = true;
    }
    return result;
}

std::string DrawViewDimension::getBaseLengthUnit(Base::UnitSystem system)
{
    switch (system) {
    case Base::UnitSystem::SI1:
        return "mm";
    case Base::UnitSystem::SI2:
        return "m";
    case Base::UnitSystem::Imperial1:
        return "in";
    case Base::UnitSystem::ImperialDecimal:
        return "in";
    case Base::UnitSystem::Centimeters:
        return "cm";
    case Base::UnitSystem::ImperialBuilding:
        return "ft";
    case Base::UnitSystem::MmMin:
        return "mm";
    case Base::UnitSystem::ImperialCivil:
        return "ft";
    case Base::UnitSystem::FemMilliMeterNewton:
        return "mm";
    default:
        return "Unknown schema";
    }
}

std::string DrawViewDimension::formatValue(qreal value, QString qFormatSpec, int partial)
{
    std::string result;

    QString qUserStringUnits;
    QString formattedValue;
    bool angularMeasure = false;
    QLocale loc;

    Base::Quantity asQuantity;
    asQuantity.setValue(value);
    if ( (Type.isValue("Angle")) ||
         (Type.isValue("Angle3Pt")) ) {
        angularMeasure = true;
        asQuantity.setUnit(Base::Unit::Angle);
    } else {
        asQuantity.setUnit(Base::Unit::Length);
    }

    QString qUserString = asQuantity.getUserString();  // this handles mm to inch/km/parsec etc
                                                       // and decimal positions but won't give more than
                                                       // Global_Decimals precision

    //units api: get schema to figure out if this is multi-value schema(Imperial1, ImperialBuilding, etc)
    //if it is multi-unit schema, don't even try to use Alt Decimals
    Base::UnitSystem unitSystem = Base::UnitsApi::getSchema();

    // we need to know what length unit is used by the scheme
    std::string BaseLengthUnit = getBaseLengthUnit(unitSystem);

    //get formatSpec prefix/suffix/specifier
    QStringList qsl = getPrefixSuffixSpec(qFormatSpec);
    QString formatPrefix    = qsl[0];   //FormatSpec prefix
    QString formatSuffix    = qsl[1];   //FormatSpec suffix
    QString formatSpecifier = qsl[2];   //FormatSpec specifier

    //handle multi value schemes (yd/ft/in, dms, etc)
    std::string genPrefix = getPrefix();     //general prefix - diameter, radius, etc
    QString qMultiValueStr;
    QString qGenPrefix = QString::fromUtf8(genPrefix.data(), genPrefix.size());
    if ((unitSystem == Base::UnitSystem::ImperialCivil) && angularMeasure) {
        QString dispMinute = QString::fromUtf8("\'");
        QString dispSecond = QString::fromUtf8("\"");
        QString schemeMinute = QString::fromUtf8("M");
        QString schemeSecond = QString::fromUtf8("S");
        QString displaySub = qUserString.replace(schemeMinute, dispMinute);
        displaySub = displaySub.replace(schemeSecond, dispSecond);
        qMultiValueStr = displaySub;
        if (!genPrefix.empty()) {
            // prefix + 48*30'30" + suffix
            qMultiValueStr = formatPrefix + qGenPrefix + displaySub + formatSuffix;
        }
        formattedValue = qMultiValueStr;
    } else if (isMultiValueSchema()) {
        qMultiValueStr = qUserString;
        if (!genPrefix.empty()) {
            //qUserString from Quantity includes units - prefix + R + nnn ft + suffix
            qMultiValueStr = formatPrefix + qUserString + formatSuffix;
        }
        return qMultiValueStr.toStdString();
    } else {
        if (formatSpecifier.isEmpty()) {
            Base::Console().Warning("Warning - no numeric format in Format Spec %s - %s\n",
                                    qPrintable(qFormatSpec), getNameInDocument());
            return Base::Tools::toStdString(qFormatSpec);
        }

        // for older TD drawings the formatSpecifier "%g" was used, but the number of decimals was
        // neverheless limited. To keep old drawings, we limit the number of decimals too
        // if the TD preferences option to use the global decimal number is set
        // the formatSpecifier can have a prefix and/or suffix
        if (useDecimals() && formatSpecifier.contains(QString::fromLatin1("%g"), Qt::CaseInsensitive)) {
                int globalPrecision = Base::UnitsApi::getDecimals();
                // change formatSpecifier to e.g. "%.2f"
                QString newSpecifier = QString::fromStdString("%." + std::to_string(globalPrecision) + "f");
                formatSpecifier.replace(QString::fromLatin1("%g"), newSpecifier, Qt::CaseInsensitive);
        }

        // qUserString is the value + unit with default decimals, so extract the unit
        // we cannot just use unit.getString() because this would convert '°' to 'deg'
        QRegExp rxUnits(QString::fromUtf8(" \\D*$")); // space + any non digits at end of string
        int pos = 0;
        if ((pos = rxUnits.indexIn(qUserString, 0)) != -1) {
            qUserStringUnits = rxUnits.cap(0); // entire capture - non numerics at end of qUserString
        }
        
        // get value in the base unit with default decimals
        // for the conversion we use the same method as in DlgUnitsCalculator::valueChanged
        // get the conversion factor for the unit
        double convertValue = Base::Quantity::parse(QString::fromLatin1("1") + QString::fromStdString(BaseLengthUnit)).getValue();
        // the result is now just val / convertValue because val is always in the base unit
        // don't do this for angular values since they are not in the BaseLengthUnit
        double userVal;
        if (!angularMeasure) {
            userVal = asQuantity.getValue() / convertValue;
            // since we converted to the BaseLengthUnit we must assure it is also used for qUserStringUnits
            qUserStringUnits = QChar::fromLatin1(' ') + QString::fromStdString(BaseLengthUnit);
        }
        else {
            userVal = asQuantity.getValue();
        }

        // we reformat the value
        // the user can overwrite the decimal settings, so we must in every case use the formatSpecifier
        // the default is: if useDecimals(), then formatSpecifier = global decimals, otherwise it is %.2f
        formattedValue = QString::asprintf(Base::Tools::toStdString(formatSpecifier).c_str(), userVal);

        // if abs(1 - userVal / formattedValue) > 0.1 we know that we make an error greater than 10%
        // then we need more digits
        if (abs(userVal - formattedValue.toDouble()) > 0.1 * abs(userVal)) {
            int i = 1;
            do { // increase decimals step by step until error is < 10 %
                formattedValue = QLocale().toString(userVal, 'f', i);
                ++i;
            } while (abs(userVal - loc.toDouble(formattedValue)) > 0.1 * abs(userVal));
            // We purposely don't reset the formatSpecifier.
            // Why "%.1f" is overwritten for a value of e.g. "0.001" is obvious,
            // moreover such cases only occurs when
            // changing unit schemes on existing drawings. Moreover a typical case is that
            // you accidentally used e.g. a building scheme, see your mistake and go back
            // then you would end up with e.g. "%.5f" and must manually correct this.
        }

        // replace decimal sign if necessary
        QChar dp = QChar::fromLatin1('.');
        if (loc.decimalPoint() != dp) {
            formattedValue.replace(dp, loc.decimalPoint());
        }
    }

    result = formattedValue.toStdString();

    if (partial == 0) {
        result = Base::Tools::toStdString(formatPrefix) +
            Base::Tools::toStdString(qMultiValueStr) +
            Base::Tools::toStdString(formatSuffix) +
            Base::Tools::toStdString(qUserStringUnits);
    }
    else if (partial == 1)  {            // prefix number suffix
        result = Base::Tools::toStdString(formatPrefix) +
            result +
            Base::Tools::toStdString(formatSuffix);
    }
    else if (partial == 2) {             // just the unit
        if (angularMeasure) {
            // remove space between dimension and unit if unit is not "deg"
            if ( !qUserStringUnits.contains(QString::fromLatin1("deg")) ) {
                QRegExp space(QString::fromUtf8("\\s"));
                qUserStringUnits.remove(space);
            }
            result = Base::Tools::toStdString(qUserStringUnits);
        } else if (showUnits()) {
            result = Base::Tools::toStdString(qUserStringUnits);
        } else {
            result = "";
        }
    }

    return result;
}

std::string DrawViewDimension::getFormattedToleranceValue(int partial)
{
    QString FormatSpec = QString::fromUtf8(FormatSpecOverTolerance.getStrValue().data());
    QString ToleranceString;

    if (ArbitraryTolerances.getValue())
        ToleranceString = FormatSpec;
    else
        ToleranceString = QString::fromUtf8(formatValue(OverTolerance.getValue(), FormatSpec, partial).c_str());

    return ToleranceString.toStdString();
}

std::pair<std::string, std::string> DrawViewDimension::getFormattedToleranceValues(int partial)
{
    QString underFormatSpec = QString::fromUtf8(FormatSpecUnderTolerance.getStrValue().data());
    QString overFormatSpec = QString::fromUtf8(FormatSpecOverTolerance.getStrValue().data());
    std::pair<std::string, std::string> tolerances;
    QString underTolerance, overTolerance;

    if (ArbitraryTolerances.getValue()) {
        underTolerance = underFormatSpec;
        overTolerance = overFormatSpec;
    } else {
        if (DrawUtil::fpCompare(UnderTolerance.getValue(), 0.0)) {
            underTolerance = QString::fromUtf8(formatValue(UnderTolerance.getValue(), QString::fromUtf8("%.0f"), partial).c_str());
        }
        else {
            underTolerance = QString::fromUtf8(formatValue(UnderTolerance.getValue(), underFormatSpec, partial).c_str());
        }
        if (DrawUtil::fpCompare(OverTolerance.getValue(), 0.0)) {
            overTolerance = QString::fromUtf8(formatValue(OverTolerance.getValue(), QString::fromUtf8("%.0f"), partial).c_str());
        }
        else {
            overTolerance = QString::fromUtf8(formatValue(OverTolerance.getValue(), overFormatSpec, partial).c_str());
        }
    }

    tolerances.first = underTolerance.toStdString();
    tolerances.second = overTolerance.toStdString();

    return tolerances;
}

std::string DrawViewDimension::getFormattedDimensionValue(int partial)
{
    QString qFormatSpec = QString::fromUtf8(FormatSpec.getStrValue().data());

    if ( (Arbitrary.getValue() && !EqualTolerance.getValue())
        || (Arbitrary.getValue() && TheoreticalExact.getValue()) ) {
        return FormatSpec.getStrValue();
    }

    // if there is an equal over-/undertolerance and not theoretically exact, add the tolerance to dimension
    if (EqualTolerance.getValue() && !TheoreticalExact.getValue() &&
            (!DrawUtil::fpCompare(OverTolerance.getValue(), 0.0) || ArbitraryTolerances.getValue())) {
        QString labelText = QString::fromUtf8(formatValue(getDimValue(), qFormatSpec, 1).c_str()); //just the number pref/spec/suf
        QString unitText = QString::fromUtf8(formatValue(getDimValue(), qFormatSpec, 2).c_str()); //just the unit
        QString tolerance = QString::fromStdString(getFormattedToleranceValue(1).c_str());
        QString result;
        if (Arbitrary.getValue()) {
            labelText = QString::fromStdString(FormatSpec.getStrValue());
            unitText = QString();
        }
        // tolerance might start with a plus sign that we don't want, so cut it off
        if (tolerance.at(0) == QChar::fromLatin1('+'))
            tolerance.remove(0, 1);
        if ((Type.isValue("Angle")) || (Type.isValue("Angle3Pt"))) {
            result = labelText + unitText + QString::fromUtf8(" \xC2\xB1 ") + tolerance;
        } else {
            // add the tolerance to the dimension using the ± sign
            result = labelText + QString::fromUtf8(" \xC2\xB1 ") + tolerance;
        }
        if (partial == 2) {
            result = unitText;
        }

        return result.toStdString();
    }
    if (Arbitrary.getValue()) {
        return FormatSpec.getStrValue();
    }

    return formatValue(getDimValue(), qFormatSpec, partial);
}

QStringList DrawViewDimension::getPrefixSuffixSpec(QString fSpec)
{
    QStringList result;
    QString formatPrefix;
    QString formatSuffix;
    //find the %x.y tag in FormatSpec
    QRegExp rxFormat(QString::fromUtf8("%[+-]?[0-9]*\\.*[0-9]*[aefgAEFG]")); //printf double format spec
    QString match;
    int pos = 0;
    if ((pos = rxFormat.indexIn(fSpec, 0)) != -1)  {
        match = rxFormat.cap(0);                                          //entire capture of rx
//        formatted = QString::asprintf(Base::Tools::toStdString(match).c_str(),value);
        formatPrefix = fSpec.left(pos);
        result.append(formatPrefix);
        formatSuffix = fSpec.right(fSpec.size() - pos - match.size());
        result.append(formatSuffix);
        result.append(match);
    } else {       //printf format not found!
        Base::Console().Warning("Warning - no numeric format in formatSpec %s - %s\n",
                                qPrintable(fSpec), getNameInDocument());
        result.append(QString());
        result.append(QString());
        result.append(fSpec);
    }
    return result;
}


//!NOTE: this returns the Dimension value in internal units (ie mm)!!!!
double DrawViewDimension::getDimValue()
{
//    Base::Console().Message("DVD::getDimValue()\n");
    double result = 0.0;
    if (!has2DReferences()) {                                            //too soon?
        if (isRestoring() ||
            getDocument()->testStatus(App::Document::Status::Restoring)) {
            return result;
        } else {
            Base::Console().Warning("%s has no 2D References\n", getNameInDocument());
        }
        return result;
    }
    if  (getViewPart() == nullptr) {
        return result;
    }

    if  (!getViewPart()->hasGeometry() ) {                              //happens when loading saved document
        return result;
    }

    if (MeasureType.isValue("True")) {
        // True Values
        if (!measurement->has3DReferences()) {
            Base::Console().Warning("%s - True dimension has no 3D References\n", getNameInDocument());
            return result;
        }
        if ( Type.isValue("Distance")  ||
             Type.isValue("DistanceX") ||
             Type.isValue("DistanceY") )  {
            result = measurement->length();
        } else if (Type.isValue("Radius")){
            result = measurement->radius();
        } else if (Type.isValue("Diameter")){
            result = 2.0 * measurement->radius();
        } else if (Type.isValue("Angle")){
            result = measurement->angle();
        } else {  //tarfu
            throw Base::ValueError("getDimValue() - Unknown Dimension Type (3)");
        }
    } else {
        // Projected Values
        if (!checkReferences2D()) {
            Base::Console().Warning("DVD::getDimValue - %s - 2D references are corrupt (5)\n",getNameInDocument());
            return result;
        }
        if ( Type.isValue("Distance")  ||
             Type.isValue("DistanceX") ||
             Type.isValue("DistanceY") )  {
            pointPair pts = m_linearPoints;
            Base::Vector3d dimVec = pts.first - pts.second;
            if (Type.isValue("Distance")) {
                result = dimVec.Length() / getViewPart()->getScale();
            } else if (Type.isValue("DistanceX")) {
                result = fabs(dimVec.x) / getViewPart()->getScale();
            } else {
                result = fabs(dimVec.y) / getViewPart()->getScale();
            }

        } else if (Type.isValue("Radius")){
            arcPoints pts = m_arcPoints;
            result = pts.radius / getViewPart()->getScale();            //Projected BaseGeom is scaled for drawing

        } else if (Type.isValue("Diameter")){
            arcPoints pts = m_arcPoints;
            result = (pts.radius  * 2.0) / getViewPart()->getScale();   //Projected BaseGeom is scaled for drawing

        } else if (Type.isValue("Angle")){
            anglePoints pts = m_anglePoints;
            Base::Vector3d vertex = pts.vertex;
            Base::Vector3d leg0 = pts.ends.first - vertex;
            Base::Vector3d leg1 = pts.ends.second - vertex;
            double legAngle =  leg0.GetAngle(leg1) * 180.0 / M_PI;
            result = legAngle;

        } else if (Type.isValue("Angle3Pt")){    //same as case "Angle"?
            anglePoints pts = m_anglePoints;
            Base::Vector3d vertex = pts.vertex;
            Base::Vector3d leg0 = pts.ends.first - vertex;
            Base::Vector3d leg1 = pts.ends.second - vertex;
            double legAngle =  leg0.GetAngle(leg1) * 180.0 / M_PI;
            result = legAngle;
        }
    }

    result = fabs(result);
    if (Inverted.getValue()) {
        if (Type.isValue("Angle") || Type.isValue("Angle3Pt")) {
            result = 360 - result;
        }
        else {
            result = -result;
        }
    }
    return result;
}

pointPair DrawViewDimension::getPointsOneEdge()
{
//    Base::Console().Message("DVD::getPointsOneEdge() - %s\n",getNameInDocument());
    pointPair result;
    const std::vector<std::string> &subElements      = References2D.getSubValues();

    //TODO: Check for straight line Edge?
    int idx = DrawUtil::getIndexFromName(subElements[0]);
    TechDraw::BaseGeom* geom = getViewPart()->getGeomByIndex(idx);
    TechDraw::Generic* gen;
    if (geom && geom->geomType == TechDraw::GeomType::GENERIC) {
        gen = static_cast<TechDraw::Generic*>(geom);
    } else {
        Base::Console().Error("Error: DVD - %s - 2D references are corrupt (1)\n",getNameInDocument());
        return result;
    }
    result.first = gen->points[0];
    result.second = gen->points[1];
    return result;
}

pointPair DrawViewDimension::getPointsTwoEdges()
{
//    Base::Console().Message("DVD::getPointsTwoEdges() - %s\n",getNameInDocument());
    pointPair result;
    const std::vector<std::string> &subElements      = References2D.getSubValues();

    int idx0 = DrawUtil::getIndexFromName(subElements[0]);
    int idx1 = DrawUtil::getIndexFromName(subElements[1]);
    TechDraw::BaseGeom* geom0 = getViewPart()->getGeomByIndex(idx0);
    TechDraw::BaseGeom* geom1 = getViewPart()->getGeomByIndex(idx1);
    if ((geom0 == nullptr) ||
        (geom1 == nullptr) ) {
        Base::Console().Error("Error: DVD - %s - 2D references are corrupt (2)\n",getNameInDocument());
        return result;
    }
    result = closestPoints(geom0->occEdge,geom1->occEdge);
    return result;
}

pointPair DrawViewDimension::getPointsTwoVerts()
{
//    Base::Console().Message("DVD::getPointsTwoVerts() - %s\n",getNameInDocument());
    pointPair result;
    const std::vector<std::string> &subElements      = References2D.getSubValues();

    int idx0 = DrawUtil::getIndexFromName(subElements[0]);
    int idx1 = DrawUtil::getIndexFromName(subElements[1]);
    TechDraw::VertexPtr v0 = getViewPart()->getProjVertexByIndex(idx0);
    TechDraw::VertexPtr v1 = getViewPart()->getProjVertexByIndex(idx1);
    if ((v0 == nullptr) ||
        (v1 == nullptr) ) {
        Base::Console().Error("Error: DVD - %s - 2D references are corrupt (3)\n",getNameInDocument());
        return result;
    }
    result.first  = v0->pnt;
    result.second = v1->pnt;
    return result;
}

pointPair DrawViewDimension::getPointsEdgeVert()
{
    pointPair result;
    const std::vector<std::string> &subElements      = References2D.getSubValues();
    int idx0 = DrawUtil::getIndexFromName(subElements[0]);
    int idx1 = DrawUtil::getIndexFromName(subElements[1]);
    TechDraw::BaseGeom* e;
    TechDraw::VertexPtr v;
    if (DrawUtil::getGeomTypeFromName(subElements[0]) == "Edge") {
        e = getViewPart()->getGeomByIndex(idx0);
        v = getViewPart()->getProjVertexByIndex(idx1);
    } else {
        e = getViewPart()->getGeomByIndex(idx1);
        v = getViewPart()->getProjVertexByIndex(idx0);
    }
    if ((v == nullptr) || (e == nullptr) ) {
        Base::Console().Error("Error: DVD - %s - 2D references are corrupt (4)\n", getNameInDocument());
        return result;
    }

    result = closestPoints(e->occEdge,v->occVertex);

    return result;
}

DrawViewPart* DrawViewDimension::getViewPart() const
{
    if (References2D.getValues().empty()) {
        return nullptr;
    }
    return dynamic_cast<TechDraw::DrawViewPart * >(References2D.getValues().at(0));
}

int DrawViewDimension::getRefType() const
{
    return getRefTypeSubElements(References2D.getSubValues());
}

int DrawViewDimension::getRefTypeSubElements(const std::vector<std::string> &subElements)
{
    int refType = invalidRef;
    int refEdges = 0, refVertices = 0;

    for (const auto& se: subElements) {
        if (DrawUtil::getGeomTypeFromName(se) == "Vertex") { refVertices++; }
        if (DrawUtil::getGeomTypeFromName(se) == "Edge") { refEdges++; }
    }

    if (refEdges == 0 && refVertices == 2) { refType = twoVertex; }
    if (refEdges == 0 && refVertices == 3) { refType = threeVertex; }
    if (refEdges == 1 && refVertices == 0) { refType = oneEdge; }
    if (refEdges == 1 && refVertices == 1) { refType = vertexEdge; }
    if (refEdges == 2 && refVertices == 0) { refType = twoEdge; }

    return refType;
}

//! validate 2D references - only checks if the target exists
bool DrawViewDimension::checkReferences2D() const
{
//    Base::Console().Message("DVD::checkReFerences2d() - %s\n",getNameInDocument());
    bool result = true;
    const std::vector<App::DocumentObject*> &objects = References2D.getValues();
    if (!objects.empty()) {
        const std::vector<std::string> &subElements      = References2D.getSubValues();
        if (!subElements.empty()) {
            for (auto& s: subElements) {
                if (!s.empty()) {
                    int idx = DrawUtil::getIndexFromName(s);
                    if (DrawUtil::getGeomTypeFromName(s) == "Edge") {
                        TechDraw::BaseGeom* geom = getViewPart()->getGeomByIndex(idx);
                        if (geom == nullptr) {
                            result = false;
                            break;
                        }
                    } else if (DrawUtil::getGeomTypeFromName(s) == "Vertex") {
                        TechDraw::VertexPtr v = getViewPart()->getProjVertexByIndex(idx);
                        if (v == nullptr) {
                            result = false;
                            break;
                        }
                    }
                } else {
                    result = false;
                }
            }
        } else {
            Base::Console().Log("DVD::checkRegerences2d() - %s - subelements empty!\n",getNameInDocument());
            result = false;
        }
    } else {
        Base::Console().Log("DVD::checkRegerences2d() - %s - objects empty!\n",getNameInDocument());
        result = false;
    }
    return result;
}

pointPair DrawViewDimension::closestPoints(TopoDS_Shape s1,
                                           TopoDS_Shape s2) const
{
    pointPair result;
    BRepExtrema_DistShapeShape extss(s1, s2);
    if (!extss.IsDone()) {
        throw Base::RuntimeError("DVD::closestPoints - BRepExtrema_DistShapeShape failed");
    }
    int count = extss.NbSolution();
    if (count != 0) {
        gp_Pnt p = extss.PointOnShape1(1);
        result.first = Base::Vector3d(p.X(),p.Y(),p.Z());
        p = extss.PointOnShape2(1);
        result.second = Base::Vector3d(p.X(),p.Y(),p.Z());
    } //TODO: else { explode }

    return result;
}

//!add Dimension 3D references to measurement
void DrawViewDimension::setAll3DMeasurement()
{
    measurement->clear();
    const std::vector<App::DocumentObject*> &Objs = References3D.getValues();
    const std::vector<std::string> &Subs      = References3D.getSubValues();
    int end = Objs.size();
    int i = 0;
    for ( ; i < end; i++) {
        static_cast<void> (measurement->addReference3D(Objs.at(i), Subs.at(i)));
    }
}

//delete all previous measurements
void DrawViewDimension::clear3DMeasurements()
{
    //set sublinklist to empty?
    measurement->clear();
}

void DrawViewDimension::dumpRefs2D(const char* text) const
{
    Base::Console().Message("DUMP - %s\n",text);
    const std::vector<App::DocumentObject*> &objects = References2D.getValues();
    const std::vector<std::string> &subElements = References2D.getSubValues();
    std::vector<App::DocumentObject*>::const_iterator objIt = objects.begin();
    std::vector<std::string>::const_iterator subIt = subElements.begin();
    int i = 0;
    for( ;objIt != objects.end();objIt++,subIt++,i++) {
        Base::Console().Message("DUMP - ref: %d object: %s subElement: %s\n",i,(*objIt)->getNameInDocument(),(*subIt).c_str());
    }
}

double DrawViewDimension::dist2Segs(Base::Vector3d s1,
                                       Base::Vector3d e1,
                                       Base::Vector3d s2,
                                       Base::Vector3d e2) const
{
    gp_Pnt start(s1.x,s1.y,0.0);
    gp_Pnt end(e1.x,e1.y,0.0);
    TopoDS_Vertex v1 = BRepBuilderAPI_MakeVertex(start);
    TopoDS_Vertex v2 = BRepBuilderAPI_MakeVertex(end);
    BRepBuilderAPI_MakeEdge makeEdge1(v1,v2);
    TopoDS_Edge edge1 = makeEdge1.Edge();

    start = gp_Pnt(s2.x,s2.y,0.0);
    end = gp_Pnt(e2.x,e2.y,0.0);
    v1 = BRepBuilderAPI_MakeVertex(start);
    v2 = BRepBuilderAPI_MakeVertex(end);
    BRepBuilderAPI_MakeEdge makeEdge2(v1,v2);
    TopoDS_Edge edge2 = makeEdge2.Edge();

    BRepExtrema_DistShapeShape extss(edge1, edge2);
    if (!extss.IsDone()) {
        throw Base::RuntimeError("DVD::dist2Segs - BRepExtrema_DistShapeShape failed");
    }
    int count = extss.NbSolution();
    double minDist = 0.0;
    if (count != 0) {
        minDist = extss.Value();
    } //TODO: else { explode }

    return minDist;
}

bool DrawViewDimension::leaderIntersectsArc(Base::Vector3d s, Base::Vector3d pointOnCircle) {
    bool result = false;
    const std::vector<std::string> &subElements      = References2D.getSubValues();
    int idx = DrawUtil::getIndexFromName(subElements[0]);
    TechDraw::BaseGeom* base = getViewPart()->getGeomByIndex(idx);
    if ( base && base->geomType == TechDraw::GeomType::ARCOFCIRCLE )  {
        TechDraw::AOC* aoc = static_cast<TechDraw::AOC*> (base);
        if (aoc->intersectsArc(s,pointOnCircle)) {
            result = true;
        }
    } else if ( base && base->geomType == TechDraw::GeomType::BSPLINE )  {
        TechDraw::BSpline* spline = static_cast<TechDraw::BSpline*> (base);
        if (spline->isCircle()) {
            if (spline->intersectsArc(s,pointOnCircle)) {
                result = true;
            }
        }
    }

    return result;
}

void DrawViewDimension::saveArrowPositions(const Base::Vector2d positions[])
{
    if (positions == nullptr) {
        m_arrowPositions.first = Base::Vector3d(0.0, 0.0, 0.0);
        m_arrowPositions.second = Base::Vector3d(0.0, 0.0, 0.0);
    } else {
        double scale = getViewPart()->getScale();
        m_arrowPositions.first = Base::Vector3d(positions[0].x, positions[0].y, 0.0) / scale;
        m_arrowPositions.second = Base::Vector3d(positions[1].x, positions[1].y, 0.0) / scale;
    }
}

//return position within parent view of dimension arrow heads/dimline endpoints
//note positions are in apparent coord (inverted y).
pointPair DrawViewDimension::getArrowPositions(void)
{
    return m_arrowPositions;
}

bool DrawViewDimension::has2DReferences(void) const
{
//    Base::Console().Message("DVD::has2DReferences() - %s\n",getNameInDocument());
    bool result = false;

    const std::vector<App::DocumentObject*> &objects = References2D.getValues();
    const std::vector<std::string> &SubNames         = References2D.getSubValues();
    if (!objects.empty()) {
        App::DocumentObject* testRef = objects.at(0);
        if (testRef != nullptr) {
            if (!SubNames.empty()) {
                result = true;              //not empty is good
                for (auto& s: SubNames) {   //but check individual entries
                    if (s.empty()) {
                        result = false;
                        break;
                    }
                }
            }
        }
    }
    return result;
}

bool DrawViewDimension::has3DReferences(void) const
{
    return (References3D.getSize() > 0);
}

bool DrawViewDimension::hasOverUnderTolerance(void) const
{
    bool result = false;
    if (ArbitraryTolerances.getValue() ||
            !DrawUtil::fpCompare(OverTolerance.getValue(), 0.0) ||
            !DrawUtil::fpCompare(UnderTolerance.getValue(), 0.0)) {
        result = true;
    }
    return result;
}

bool DrawViewDimension::showUnits() const
{
    bool result = false;
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    result = hGrp->GetBool("ShowUnits", false);
    return result;
}

bool DrawViewDimension::useDecimals() const
{
    return Preferences::useGlobalDecimals();
}

std::string DrawViewDimension::getPrefix() const
{
    std::string result = "";
    if (Type.isValue("Distance")) {
        result = "";
    } else if (Type.isValue("DistanceX")){
        result = "";
    } else if (Type.isValue("DistanceY")){
        result = "";
    } else if (Type.isValue("DistanceZ")){
        result = "";
    } else if (Type.isValue("Radius")){
        result =  "R";
    } else if (Type.isValue("Diameter")){
        Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
            .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
        std::string diamSym = hGrp->GetASCII("DiameterSymbol","\xe2\x8c\x80");
        result = diamSym;
    } else if (Type.isValue("Angle")){
        result = "";
    }
    return result;
}

std::string DrawViewDimension::getDefaultFormatSpec(bool isToleranceFormat) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                         .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    std::string prefFormat = hGrp->GetASCII("formatSpec","");
    QString formatSpec;
    QString qPrefix;
    if (prefFormat.empty()) {
        QString format1 = Base::Tools::fromStdString("%.");
        QString format2 = Base::Tools::fromStdString("f");
        int precision;
        if (useDecimals()) {
            precision = Base::UnitsApi::getDecimals();
        } else {
            precision = hGrp->GetInt("AltDecimals", 2);
        }
        QString formatPrecision = QString::number(precision);

        std::string prefix = getPrefix();

        if (!prefix.empty()) {
            qPrefix = QString::fromUtf8(prefix.data(),prefix.size());
        }

        formatSpec = qPrefix + format1 + formatPrecision + format2;
    } else {

        std::string prefix = getPrefix();
        qPrefix = QString::fromUtf8(prefix.data(),prefix.size());
        formatSpec = qPrefix + QString::fromStdString(prefFormat);

    }

    if (isToleranceFormat) {
        formatSpec.replace(QString::fromUtf8("%"), QString::fromUtf8("%+"));
    }

    return Base::Tools::toStdString(formatSpec);
}

////! is refName a target of this Dim (2D references)
//bool DrawViewDimension::references(std::string refName) const
//{
//    Base::Console().Message("DVD::references(%s) - %s\n",refName.c_str(),getNameInDocument());
//    bool result = false;
//    const std::vector<App::DocumentObject*> &objects = References2D.getValues();
//    if (!objects.empty()) {
//        const std::vector<std::string> &subElements = References2D.getSubValues();
//        if (!subElements.empty()) {
//            for (auto& s: subElements) {
//                if (!s.empty()) {
//                    if (s == refName) {
//                        result = true;
//                        break;
//                    }
//                }
//            }
//        }
//    }
//    return result;
//}

PyObject *DrawViewDimension::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewDimensionPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
