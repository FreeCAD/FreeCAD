/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2019 Ludovic Mercier, lidiriel <ludovic@scilink.net>    *
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
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Quantity.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include "Geometry.h"
#include "DrawViewPart.h"
#include "DrawViewGDTReference.h"
#include "DrawUtil.h"
#include "LineGroup.h"

using namespace TechDraw;

//===========================================================================
// DrawViewGDTReference
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewGDTReference, TechDraw::DrawView)

const char *DrawViewGDTReference::TypeEnums[] = { "Edge", "Cosmetic",
        "FeatureFrame",
        NULL };

DrawViewGDTReference::DrawViewGDTReference(void) {
    ADD_PROPERTY_TYPE(References2D, (0,0), "", (App::Prop_None),
            "Projected Geometry References");
    References2D.setScope(App::LinkScope::Global);

    Type.setEnums(TypeEnums); // object referenced type : Edge, Cosmetic line, Feature frame.
    ADD_PROPERTY(Type, ((long)0));
    Type.setStatus(App::Property::ReadOnly, true);

    ADD_PROPERTY_TYPE(Text, ("A"), "", App::Prop_None,
            "The text to be displayed");
    ADD_PROPERTY_TYPE(SymbolScale, (1), "",
            (App::PropertyType )(App::Prop_None), "Reference symbol scale");

    m_linearPoints.first = Base::Vector3d(0, 0, 0);
    m_linearPoints.second = Base::Vector3d(0, 0, 0);

    //hide the DrawView properties that don't apply to reference
    References2D.setStatus(App::Property::Hidden, true);
    X.setStatus(App::Property::Hidden, true);
    Y.setStatus(App::Property::Hidden, true);
    ScaleType.setStatus(App::Property::ReadOnly, true);
    ScaleType.setStatus(App::Property::Hidden, true);
    Scale.setStatus(App::Property::Hidden, true);
    Rotation.setStatus(App::Property::Hidden, true);
    Caption.setStatus(App::Property::Hidden, true);
}

DrawViewGDTReference::~DrawViewGDTReference() {
}

//are there non-blank references?
bool DrawViewGDTReference::has2DReferences(void) const {
    //Base::Console().Message("DrawViewGDTReference::has2DReferences() - %s\n",getNameInDocument());
    bool result = false;

    const std::vector<App::DocumentObject*> &objects = References2D.getValues();
    const std::vector<std::string> &SubNames = References2D.getSubValues();
    if (!objects.empty()) {
        App::DocumentObject *testRef = objects.at(0);
        if (testRef != nullptr) {
            if (!SubNames.empty()) {
                result = true;              //not empty is good
                for (auto &s : SubNames) {   //but check individual entries
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

void DrawViewGDTReference::onChanged(const App::Property *prop) {
    DrawView::onChanged(prop);
}

void DrawViewGDTReference::onDocumentRestored() {
    execute();
}

short DrawViewGDTReference::mustExecute() const {
    bool result = 0;
    if (!isRestoring()) {
        result = (References2D.isTouched() || Text.isTouched());
    }
    if (result) {
        return result;
    }

    return DrawView::mustExecute();
}

DrawViewPart* DrawViewGDTReference::getViewPart() const {
    if (References2D.getValues().empty()) {
        return nullptr;
    }
    return dynamic_cast<TechDraw::DrawViewPart*>(References2D.getValues().at(0));
}

PointPair DrawViewGDTReference::getPointsOneEdge() {
    //Base::Console().Message("DrawViewGDTReference::getPointsOneEdge() - %s\n",getNameInDocument());
    PointPair result;
    const std::vector<std::string> &subElements = References2D.getSubValues();

    //TODO: Check for straight line Edge?
    int idx = DrawUtil::getIndexFromName(subElements[0]);
    TechDraw::BaseGeom *geom = getViewPart()->getGeomByIndex(idx);
    TechDraw::Generic *gen;
    if (geom && geom->geomType == TechDraw::GeomType::GENERIC) {
        gen = static_cast<TechDraw::Generic*>(geom);
    } else {
        Base::Console().Error(
                "Error: DrawViewGDTReference - %s - 2D references are corrupt (1)\n",
                getNameInDocument());
        return result;
    }
    result.first = gen->points[0];
    result.second = gen->points[1];
    return result;
}

App::DocumentObjectExecReturn* DrawViewGDTReference::execute(void) {
    //Base::Console().Message("DrawViewGDTReference::execute() - %s\n", getNameInDocument());
    //any empty Reference2D??
    if (!has2DReferences()) {                                        //too soon?
        if (isRestoring()
                || getDocument()->testStatus(
                        App::Document::Status::Restoring)) {
            return App::DocumentObject::StdReturn;
        } else {
            Base::Console().Warning("%s has no 2D References\n",
                    getNameInDocument());
        }
        return App::DocumentObject::StdReturn;
    }
    //can't do anything until Source has geometry
    if (!getViewPart()->hasGeometry()) {   //happens when loading saved document
        if (isRestoring()
                || getDocument()->testStatus(
                        App::Document::Status::Restoring)) {
            return App::DocumentObject::StdReturn;
        } else {
            Base::Console().Warning("%s - target has no geometry\n",
                    getNameInDocument());
            return App::DocumentObject::StdReturn;
        }
    }
    //now we can check if Reference2ds have valid targets.
    if (!checkReferences2D()) {
        Base::Console().Warning("%s has invalid 2D References\n",
                getNameInDocument());
        return App::DocumentObject::StdReturn;
    }

    m_linearPoints = getPointsOneEdge();

    return DrawView::execute();
}

//! validate 2D references - only checks if the target exists
bool DrawViewGDTReference::checkReferences2D() const {
    Base::Console().Message("DrawViewGDTReference::checkReferences2d() - %s\n",
            getNameInDocument());
    bool result = true;
    const std::vector<App::DocumentObject*> &objects = References2D.getValues();
    if (!objects.empty()) {
        const std::vector<std::string> &subElements =
                References2D.getSubValues();
        if (!subElements.empty()) {
            for (auto &s : subElements) {
                if (!s.empty()) {
                    int idx = DrawUtil::getIndexFromName(s);
                    if (DrawUtil::getGeomTypeFromName(s) == "Edge") {
                        TechDraw::BaseGeom *geom =
                                getViewPart()->getGeomByIndex(idx);
                        if (geom == nullptr) {
                            result = false;
                            break;
                        }
                    }
                } else {
                    result = false;
                }
            }
        } else {
            Base::Console().Log(
                    "DrawViewGDTReference::checkReferences2d() - %s - subelements empty!\n",
                    getNameInDocument());
            result = false;
        }
    } else {
        Base::Console().Log(
                "DrawViewGDTReference::checkReferences2d() - %s - objects empty!\n",
                getNameInDocument());
        result = false;
    }
    return result;
}

// TODO add python wrapper
//PyObject *DrawViewGDTReference::getPyObject(void)
//{
//    if (PythonObject.is(Py::_None())) {
//        // ref counter is set to 1
//        PythonObject = Py::Object(new DrawViewGDTReferencePy(this),true);
//    }
//    return Py::new_reference_to(PythonObject);
//}

