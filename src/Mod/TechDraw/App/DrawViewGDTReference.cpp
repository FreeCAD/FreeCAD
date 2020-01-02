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

//#include <Mod/Measure/App/Measurement.h>

#include "Geometry.h"
#include "DrawViewPart.h"
#include "DrawViewGDTReference.h"
#include "DrawUtil.h"
#include "LineGroup.h"

using namespace TechDraw;

//===========================================================================
// DrawViewDimension
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewGDTReference, TechDraw::DrawView)

const char* DrawViewGDTReference::TypeEnums[]= {"Edge",
                                         	 "Cosmetic",
                                             "FeatureFrame",
											 NULL};

DrawViewGDTReference::DrawViewGDTReference(void)
{
	ADD_PROPERTY_TYPE(References2D,(0,0),"",(App::Prop_None),"Projected Geometry References");
	References2D.setScope(App::LinkScope::Global);

	Type.setEnums(TypeEnums);     // object referenced type : Edge, Cosmetic line, Feature frame.
	ADD_PROPERTY(Type,((long)0));

	ADD_PROPERTY_TYPE(Text ,     ("A"),"",App::Prop_None,"The text to be displayed");
    //ADD_PROPERTY_TYPE(sourceView,(0),"",(App::PropertyType)(App::Prop_None),"Source view for balloon");
    ADD_PROPERTY_TYPE(OriginX,(0),"",(App::PropertyType)(App::Prop_None),"Balloon origin x");
    ADD_PROPERTY_TYPE(OriginY,(0),"",(App::PropertyType)(App::Prop_None),"Balloon origin y");
    ADD_PROPERTY_TYPE(OriginIsSet, (false), "",(App::PropertyType)(App::Prop_None),"Balloon origin is set");

    ADD_PROPERTY_TYPE(SymbolScale,(1),"",(App::PropertyType)(App::Prop_None),"Balloon symbol scale");

    ADD_PROPERTY_TYPE(TextWrapLen,(-1),"",(App::PropertyType)(App::Prop_None),"Balloon symbol scale");


    OriginIsSet.setStatus(App::Property::Hidden,false);
    OriginIsSet.setStatus(App::Property::ReadOnly,true);

    //hide the DrawView properties that don't apply to reference
    ScaleType.setStatus(App::Property::ReadOnly,true);
    ScaleType.setStatus(App::Property::Hidden,true);
    Scale.setStatus(App::Property::ReadOnly,true);
    Scale.setStatus(App::Property::Hidden,true);
    Rotation.setStatus(App::Property::ReadOnly,true);
    Rotation.setStatus(App::Property::Hidden,true);
    Caption.setStatus(App::Property::Hidden,true);
}

DrawViewGDTReference::~DrawViewGDTReference()
{

}

//are there non-blank references?
bool DrawViewGDTReference::has2DReferences(void) const
{
    Base::Console().Message("DVRef::has2DReferences() - %s\n",getNameInDocument());
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

void DrawViewGDTReference::onChanged(const App::Property* prop)
{
    DrawView::onChanged(prop);
}

void DrawViewGDTReference::onDocumentRestored()
{

}

//void DrawViewReference::handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property *prop)
//// transforms properties that had been changed
//{
//    // also check for changed properties of the base class
//    DrawView::handleChangedPropertyType(reader, TypeName, prop);
//
//    // property OriginX had the App::PropertyFloat and was changed to App::PropertyDistance
//    if ( (prop == &OriginX) &&
//         (strcmp(TypeName, "App::PropertyFloat") == 0) )  {
//        App::PropertyFloat OriginXProperty;
//        // restore the PropertyFloat to be able to set its value
//        OriginXProperty.Restore(reader);
//        OriginX.setValue(OriginXProperty.getValue());
//    } else if ( (prop == &OriginX) &&
//                (strcmp(TypeName, "App::PropertyLength") == 0) )  {
//        App::PropertyLength OriginXProperty;
//        // restore the PropertyFloat to be able to set its value
//        OriginXProperty.Restore(reader);
//        OriginX.setValue(OriginXProperty.getValue());
//
//    // property OriginY had the App::PropertyFloat and was changed to App::PropertyDistance
//    } else if ( (prop == &OriginY) &&
//                (strcmp(TypeName, "App::PropertyFloat") == 0) )  {
//        App::PropertyFloat OriginYProperty;
//        // restore the PropertyFloat to be able to set its value
//        OriginYProperty.Restore(reader);
//        OriginY.setValue(OriginYProperty.getValue());
//    } else if ( (prop == &OriginY) &&
//                (strcmp(TypeName, "App::PropertyLength") == 0) )  {
//        App::PropertyLength OriginYProperty;
//        // restore the PropertyLength to be able to set its value
//        OriginYProperty.Restore(reader);
//        OriginY.setValue(OriginYProperty.getValue());
//    }
//}


short DrawViewGDTReference::mustExecute() const
{
    bool result = 0;
    if (!isRestoring()) {
        result = (	References2D.isTouched() ||
        			Text.isTouched()	);
    }
    if (result) {
        return result;
    }

//    auto dvp = getViewPart();
//    if (dvp != nullptr) {
//        result = dvp->isTouched();
//    }
//    if (result) {
//        return result;
//    }

    return DrawView::mustExecute();
}

DrawViewPart* DrawViewGDTReference::getViewPart() const
{
    if (References2D.getValues().empty()) {
        return nullptr;
    }
    return dynamic_cast<TechDraw::DrawViewPart * >(References2D.getValues().at(0));
}

pointPair DrawViewGDTReference::getPointsOneEdge()
{
    Base::Console().Message("DVRef::getPointsOneEdge() - %s\n",getNameInDocument());
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

App::DocumentObjectExecReturn *DrawViewGDTReference::execute(void)
{
	Base::Console().Message("DVRef::execute() - %s\n", getNameInDocument());
	//any empty Reference2D??
	if (!has2DReferences()) {                                            //too soon?
//		if (isRestoring() ||
//				App::DocumentObject->testStatus(App::Document::Status::Restoring)) {
//	            return App::DocumentObject::StdReturn;
//		} else {
//			Base::Console().Warning("%s has no 2D References\n", getNameInDocument());
//	    }
		return App::DocumentObject::StdReturn;
	}
    //requestPaint();
	m_linearPoints = getPointsOneEdge();
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
