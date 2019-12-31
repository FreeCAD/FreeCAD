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
#include <gp_Pnt.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#endif

#include <QLocale>

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Quantity.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include <Mod/Measure/App/Measurement.h>

#include "Geometry.h"
#include "DrawViewPart.h"
#include "DrawViewBalloon.h"
#include "DrawUtil.h"
#include "LineGroup.h"


//#include <Mod/TechDraw/App/DrawViewBalloonPy.h>  // generated from DrawViewBalloonPy.xml

using namespace TechDraw;

//===========================================================================
// DrawViewBalloon
//===========================================================================
//
// X,Y is the center of the balloon bubble
// OriginX, OriginY is the tip of the arrow
// these are in ???? coordinates

PROPERTY_SOURCE(TechDraw::DrawViewBalloon, TechDraw::DrawView)

//from Gui/QGIArrow.h
//enum ArrowType {
//        FILLED_TRIANGLE = 0,
//        OPEN_ARROW,
//        HASH_MARK,
//        DOT,
//        OPEN_CIRCLE,
//        FORK,
//        PYRAMID
//    };

const char* DrawViewBalloon::endTypeEnums[]= { "FILLED_TRIANGLE",
                                               "OPEN_ARROW",
                                               "HASH_MARK",
                                               "DOT",
                                               "OPEN_CIRCLE",
                                               "FORK",
                                               "PYRAMID",
                                               NULL};

//const char* DrawViewBalloon::endTypeEnums[]= {"Arrow",
//                                              "Dot",
//                                               NULL};

const char* DrawViewBalloon::balloonTypeEnums[]= {"Circular",
                                                  "None",
                                                  "Triangle",
                                                  "Inspection",
                                                  "Hexagon",
                                                  "Square",
                                                  "Rectangle",
                                                  NULL};

DrawViewBalloon::DrawViewBalloon(void)
{
    ADD_PROPERTY_TYPE(Text ,     (""),"",App::Prop_None,"The text to be displayed");
//    ADD_PROPERTY_TYPE(SourceView,(0),"",(App::PropertyType)(App::Prop_None),"Source view for balloon");
    ADD_PROPERTY_TYPE(SourceView,(0),"",(App::PropertyType)(App::Prop_None),"Source view for balloon");
    ADD_PROPERTY_TYPE(OriginX,(0),"",(App::PropertyType)(App::Prop_None),"Balloon origin x");
    ADD_PROPERTY_TYPE(OriginY,(0),"",(App::PropertyType)(App::Prop_None),"Balloon origin y");
    ADD_PROPERTY_TYPE(OriginIsSet, (false), "",(App::PropertyType)(App::Prop_None),"Balloon origin is set");

    EndType.setEnums(endTypeEnums);
    ADD_PROPERTY(EndType,((long)0));

    Symbol.setEnums(balloonTypeEnums);
    ADD_PROPERTY(Symbol,((long)0));

    ADD_PROPERTY_TYPE(SymbolScale,(1),"",(App::PropertyType)(App::Prop_None),"Balloon symbol scale");

    ADD_PROPERTY_TYPE(TextWrapLen,(-1),"",(App::PropertyType)(App::Prop_None),"Balloon symbol scale");

//    OriginX.setStatus(App::Property::Hidden,false);
//    OriginY.setStatus(App::Property::Hidden,false);
    OriginIsSet.setStatus(App::Property::Hidden,false);
    OriginIsSet.setStatus(App::Property::ReadOnly,true);

    SourceView.setScope(App::LinkScope::Global);
//    SourceView.setStatus(App::Property::Hidden,true);
    Rotation.setStatus(App::Property::Hidden,true);
//    ScaleType.setStatus(App::Property::Hidden,true);
//    Scale.setStatus(App::Property::Hidden,true);
    Caption.setStatus(App::Property::Hidden,true);
//    X.setStatus(App::Property::Hidden,true);
//    Y.setStatus(App::Property::Hidden,true);
}

DrawViewBalloon::~DrawViewBalloon()
{

}

void DrawViewBalloon::onChanged(const App::Property* prop)
{
    DrawView::onChanged(prop);
}

void DrawViewBalloon::handleChangedPropertyName(Base::XMLReader &reader, const char * TypeName, const char *PropName)
{
    // was sourceView in the past, now is SourceView
    Base::Type type = Base::Type::fromName(TypeName);
    if (SourceView.getClassTypeId() == type && strcmp(PropName, "sourceView") == 0) {
        SourceView.Restore(reader);
    }
    else {
        DrawView::handleChangedPropertyName(reader, TypeName, PropName);
    }
}

void DrawViewBalloon::handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property *prop)
// transforms properties that had been changed
{
    // also check for changed properties of the base class
    DrawView::handleChangedPropertyType(reader, TypeName, prop);

    // property OriginX had the App::PropertyFloat and was changed to App::PropertyDistance
    if ( (prop == &OriginX) && 
         (strcmp(TypeName, "App::PropertyFloat") == 0) )  {
        App::PropertyFloat OriginXProperty;
        // restore the PropertyFloat to be able to set its value
        OriginXProperty.Restore(reader);
        OriginX.setValue(OriginXProperty.getValue());
    } else if ( (prop == &OriginX) && 
                (strcmp(TypeName, "App::PropertyLength") == 0) )  {
        App::PropertyLength OriginXProperty;
        // restore the PropertyFloat to be able to set its value
        OriginXProperty.Restore(reader);
        OriginX.setValue(OriginXProperty.getValue());

    // property OriginY had the App::PropertyFloat and was changed to App::PropertyDistance
    } else if ( (prop == &OriginY) && 
                (strcmp(TypeName, "App::PropertyFloat") == 0) )  {
        App::PropertyFloat OriginYProperty;
        // restore the PropertyFloat to be able to set its value
        OriginYProperty.Restore(reader);
        OriginY.setValue(OriginYProperty.getValue());
    } else if ( (prop == &OriginY) && 
                (strcmp(TypeName, "App::PropertyLength") == 0) )  {
        App::PropertyLength OriginYProperty;
        // restore the PropertyLength to be able to set its value
        OriginYProperty.Restore(reader);
        OriginY.setValue(OriginYProperty.getValue());
    }
}


short DrawViewBalloon::mustExecute() const
{
    bool result = 0;
    if (!isRestoring()) {
        result =  Text.isTouched();
    }

    if (result) {
        return result;
    }

    auto dvp = getViewPart();
    if (dvp != nullptr) {
        result = dvp->isTouched();
    }
    if (result) {
        return result;
    }

    return DrawView::mustExecute();
}

void DrawViewBalloon::handleXYLock(void) {
    if (isLocked()) {
        if (!OriginX.testStatus(App::Property::ReadOnly)) {
            OriginX.setStatus(App::Property::ReadOnly, true);
            OriginX.purgeTouched();
        }
        if (!OriginY.testStatus(App::Property::ReadOnly)) {
            OriginY.setStatus(App::Property::ReadOnly, true);
            OriginY.purgeTouched();
        }
    } else {
        if (OriginX.testStatus(App::Property::ReadOnly)) {
            OriginX.setStatus(App::Property::ReadOnly, false);
            OriginX.purgeTouched();
        }
        if (OriginY.testStatus(App::Property::ReadOnly)) {
            OriginY.setStatus(App::Property::ReadOnly, false);
            OriginY.purgeTouched();
        }
    }
    DrawView::handleXYLock();
}


DrawViewPart* DrawViewBalloon::getViewPart() const
{
    App::DocumentObject* obj = SourceView.getValue();
    DrawViewPart* result = dynamic_cast<DrawViewPart*>(obj);
    return result;
}

App::DocumentObjectExecReturn *DrawViewBalloon::execute(void)
{
    requestPaint();
    return App::DocumentObject::execute();
}
/*
PyObject *DrawViewBalloon::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewBalloonPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
*/
