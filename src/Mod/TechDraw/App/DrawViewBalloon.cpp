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
#include <Precision.hxx>
#include <cstdlib>
#include <cstring>
#include <sstream>
#endif

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Mod/Measure/App/Measurement.h>

#include "ArrowPropEnum.h"
#include "DrawViewBalloon.h"
#include "DrawViewPart.h"
#include "Preferences.h"


using namespace TechDraw;

App::PropertyFloatConstraint::Constraints DrawViewBalloon::SymbolScaleRange = {
    Precision::Confusion(), std::numeric_limits<double>::max(), (0.1)};

//===========================================================================
// DrawViewBalloon
//===========================================================================
// Balloon coordinates are relative to the position of the SourceView
// X, Y is the center of the balloon bubble
// OriginX, OriginY is the tip of the arrow
// these are in unscaled SourceView coordinates
// Note that if the SourceView coordinate system changes
// (ie location changes or additional items added to the View
// the location of the balloon may also change.

PROPERTY_SOURCE(TechDraw::DrawViewBalloon, TechDraw::DrawView)

const char* DrawViewBalloon::balloonTypeEnums[] = {"Circular",   "None",    "Triangle",
                                                   "Inspection", "Hexagon", "Square",
                                                   "Rectangle",  "Line",    nullptr};

DrawViewBalloon::DrawViewBalloon()
{
    ADD_PROPERTY_TYPE(Text, (""), "", App::Prop_None, "The text to be displayed");
    ADD_PROPERTY_TYPE(SourceView, (nullptr), "", (App::PropertyType)(App::Prop_None),
                      "Source view for balloon");
    ADD_PROPERTY_TYPE(OriginX, (0), "", (App::PropertyType)(App::Prop_None), "Balloon origin x");
    ADD_PROPERTY_TYPE(OriginY, (0), "", (App::PropertyType)(App::Prop_None), "Balloon origin y");

    EndType.setEnums(ArrowPropEnum::ArrowTypeEnums);
    ADD_PROPERTY_TYPE(EndType, (Preferences::balloonArrow()), "", (App::PropertyType)(App::Prop_None),
                      "End symbol for the balloon line");

    ADD_PROPERTY_TYPE(EndTypeScale, (1.0), "", (App::PropertyType)(App::Prop_None),
                      "End symbol scale factor");
    EndTypeScale.setConstraints(&SymbolScaleRange);

    BubbleShape.setEnums(balloonTypeEnums);
    ADD_PROPERTY_TYPE(BubbleShape, (Preferences::balloonShape()), "", (App::PropertyType)(App::Prop_None),
                      "Shape of the balloon bubble");

    ADD_PROPERTY_TYPE(ShapeScale, (1.0), "", (App::PropertyType)(App::Prop_None),
                      "Balloon shape scale");
    ShapeScale.setConstraints(&SymbolScaleRange);

    ADD_PROPERTY_TYPE(TextWrapLen, (-1), "", (App::PropertyType)(App::Prop_None),
                      "Text wrap length; -1 means no wrap");

    ADD_PROPERTY_TYPE(KinkLength, (Preferences::balloonKinkLength()), "", (App::PropertyType)(App::Prop_None),
                      "Distance from symbol to leader kink");

    SourceView.setScope(App::LinkScope::Global);
    // hide the DrawView properties that don't apply to Dimensions
    ScaleType.setStatus(App::Property::ReadOnly, true);
    ScaleType.setStatus(App::Property::Hidden, true);
    Scale.setStatus(App::Property::ReadOnly, true);
    Scale.setStatus(App::Property::Hidden, true);
    Rotation.setStatus(App::Property::ReadOnly, true);
    Rotation.setStatus(App::Property::Hidden, true);
    Caption.setStatus(App::Property::Hidden, true);
}

DrawViewBalloon::~DrawViewBalloon() {}

void DrawViewBalloon::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if ((prop == &EndType) || (prop == &BubbleShape) || (prop == &ShapeScale) || (prop == &Text)
            || (prop == &KinkLength) || (prop == &EndTypeScale) || (prop == &OriginX)
            || (prop == &OriginY)) {
            requestPaint();
        }
    }
    DrawView::onChanged(prop);
}

void DrawViewBalloon::handleChangedPropertyName(Base::XMLReader& reader, const char* TypeName,
                                                const char* PropName)
{
    Base::Type type = Base::Type::fromName(TypeName);
    // was sourceView in the past, now is SourceView
    if (SourceView.getClassTypeId() == type && strcmp(PropName, "sourceView") == 0) {
        SourceView.Restore(reader);
    }
    else if (BubbleShape.getClassTypeId() == type && strcmp(PropName, "Symbol") == 0) {
        // was Symbol, then Shape in the past, now is BubbleShape
        BubbleShape.Restore(reader);
    }
    else if (BubbleShape.getClassTypeId() == type && strcmp(PropName, "Shape") == 0) {
        // was Symbol, then Shape in the past, now is BubbleShape
        BubbleShape.Restore(reader);
    }
    else if (ShapeScale.getClassTypeId() == type && strcmp(PropName, "SymbolScale") == 0) {
        // was SymbolScale in the past, now is ShapeScale
        ShapeScale.Restore(reader);
    }
    else {
        DrawView::handleChangedPropertyName(reader, TypeName, PropName);
    }
}

void DrawViewBalloon::handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName,
                                                App::Property* prop)
// transforms properties that had been changed
{
    // also check for changed properties of the base class
    DrawView::handleChangedPropertyType(reader, TypeName, prop);

    // property OriginX had the App::PropertyFloat and was changed to App::PropertyDistance
    if ((prop == &OriginX) && (strcmp(TypeName, "App::PropertyFloat") == 0)) {
        App::PropertyFloat OriginXProperty;
        // restore the PropertyFloat to be able to set its value
        OriginXProperty.Restore(reader);
        OriginX.setValue(OriginXProperty.getValue());
    }
    else if ((prop == &OriginX) && (strcmp(TypeName, "App::PropertyLength") == 0)) {
        App::PropertyLength OriginXProperty;
        // restore the PropertyFloat to be able to set its value
        OriginXProperty.Restore(reader);
        OriginX.setValue(OriginXProperty.getValue());

        // property OriginY had the App::PropertyFloat and was changed to App::PropertyDistance
    }
    else if ((prop == &OriginY) && (strcmp(TypeName, "App::PropertyFloat") == 0)) {
        App::PropertyFloat OriginYProperty;
        // restore the PropertyFloat to be able to set its value
        OriginYProperty.Restore(reader);
        OriginY.setValue(OriginYProperty.getValue());
    }
    else if ((prop == &OriginY) && (strcmp(TypeName, "App::PropertyLength") == 0)) {
        App::PropertyLength OriginYProperty;
        // restore the PropertyLength to be able to set its value
        OriginYProperty.Restore(reader);
        OriginY.setValue(OriginYProperty.getValue());
    }
}

//NOTE: DocumentObject::mustExecute returns 1/0 and not true/false
short DrawViewBalloon::mustExecute() const
{
    if (!isRestoring() && Text.isTouched()) {
        return 1;
    }

    auto dv = getParentView();
    if (dv && dv->isTouched()) {
        return 1;
    }

    return DrawView::mustExecute();
}

void DrawViewBalloon::handleXYLock()
{
    bool on = isLocked();
    if (!OriginX.testStatus(App::Property::ReadOnly)) {
        OriginX.setStatus(App::Property::ReadOnly, on);
        OriginX.purgeTouched();
    }
    if (!OriginY.testStatus(App::Property::ReadOnly)) {
        OriginY.setStatus(App::Property::ReadOnly, on);
        OriginY.purgeTouched();
    }
    DrawView::handleXYLock();
}


DrawView* DrawViewBalloon::getParentView() const
{
    App::DocumentObject* obj = SourceView.getValue();
    DrawView* result = dynamic_cast<DrawView*>(obj);
    return result;
}

App::DocumentObjectExecReturn* DrawViewBalloon::execute()
{
    requestPaint();
    return App::DocumentObject::execute();
}

void DrawViewBalloon::setOrigin(Base::Vector3d newOrigin)
{
    //suspend onChanged/recompute?
    OriginX.setValue(newOrigin.x);
    OriginY.setValue(newOrigin.y);
    origin = QPointF(newOrigin.x, newOrigin.y);
}

QPointF DrawViewBalloon::getOrigin()
{
    double x = OriginX.getValue();
    double y = OriginY.getValue();
    return QPointF(x, y);
}

void DrawViewBalloon::setOrigin(QPointF p)
{
    OriginX.setValue(p.x());
    OriginY.setValue(p.y());
    origin = p;
}

Base::Vector3d DrawViewBalloon::getOriginOffset() const
{
    double x = X.getValue();
    double y = Y.getValue();
    Base::Vector3d pos(x, y, 0.0);
    double ox = OriginX.getValue();
    double oy = OriginY.getValue();
    Base::Vector3d org(ox, oy, 0.0);
    return Base::Vector3d(pos - org);
}

/*
PyObject *DrawViewBalloon::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewBalloonPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
*/
