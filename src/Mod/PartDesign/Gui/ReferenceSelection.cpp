/******************************************************************************
 *   Copyright (c)2012 Konstantinos Poulios <logari81@gmail.com>              *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Face.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
#endif

#include <App/Plane.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/DatumPoint.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include "ReferenceSelection.h"
#include "Workbench.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::ReferenceSelection.cpp */

bool ReferenceSelection::allow(App::Document* pDoc, App::DocumentObject* pObj, const char* sSubName)
{
    // Don't allow selection in other document
    if ((support != NULL) && (pDoc != support->getDocument()))
        return false;

    if (plane && (pObj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())))
        // Note: It is assumed that a Part has exactly 3 App::Plane objects at the root of the feature tree
        return true;

    if (pObj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId())) {
        // Allow selecting Part::Datum features from the active Body
        if (!ActivePartObject->hasFeature(pObj))
            return false;

        if (plane && (pObj->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId())))
            return true;
        if (edge && (pObj->getTypeId().isDerivedFrom(PartDesign::Line::getClassTypeId())))
            return true;
        if (point && (pObj->getTypeId().isDerivedFrom(PartDesign::Point::getClassTypeId())))
            return true;

        return false;
    }

    // Handle selection of geometry elements
    if (support == NULL)
        return false;    
    if (!sSubName || sSubName[0] == '\0')
        return false;
    if (pObj != support)
        return false;
    std::string subName(sSubName);
    if (edge && subName.size() > 4 && subName.substr(0,4) == "Edge") {
        const Part::TopoShape &shape = static_cast<const Part::Feature*>(support)->Shape.getValue();
        TopoDS_Shape sh = shape.getSubShape(subName.c_str());
        const TopoDS_Edge& edge = TopoDS::Edge(sh);
        if (!edge.IsNull()) {
            if (planar) {
                BRepAdaptor_Curve adapt(edge);
                if (adapt.GetType() == GeomAbs_Line)
                    return true;
            } else {
                return true;
            }
        }
    }
    if (plane && subName.size() > 4 && subName.substr(0,4) == "Face") {
        const Part::TopoShape &shape = static_cast<const Part::Feature*>(support)->Shape.getValue();
        TopoDS_Shape sh = shape.getSubShape(subName.c_str());
        const TopoDS_Face& face = TopoDS::Face(sh);
        if (!face.IsNull()) {
            if (planar) {
                BRepAdaptor_Surface adapt(face);
                if (adapt.GetType() == GeomAbs_Plane)
                    return true;
            } else {
                return true;
            }
        }
    }
    if (point && subName.size() > 6 && subName.substr(0,6) == "Vertex") {
        return true;
    }
    return false;
}

namespace PartDesignGui
{

const QString getRefStr(const App::DocumentObject* obj, const std::vector<std::string>& sub)
{
    if (obj == NULL)
        return QString::fromAscii("");

    if (PartDesign::Feature::isDatum(obj))
        return QString::fromAscii(obj->getNameInDocument());
    else
        return QString::fromAscii(obj->getNameInDocument()) + QString::fromAscii(":") +
               QString::fromAscii(sub.front().c_str());
}

const std::string getPythonStr(const App::DocumentObject* obj, const std::vector<std::string>& sub)
{
    if (obj == NULL)
        return "";

    if (PartDesign::Feature::isDatum(obj))
        return std::string("(App.ActiveDocument.") + obj->getNameInDocument() + ", [\"\"])";
    else
        return std::string("(App.ActiveDocument.") + obj->getNameInDocument() + ", [\"" + sub.front() + "\"])";
}

}
