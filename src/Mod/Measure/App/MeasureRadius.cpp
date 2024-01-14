/***************************************************************************
 *   Copyright (c) 2023 WandererFan <wandererfan@gmail.com>                *
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

#include <tuple>

#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepLProp_CLProps.hxx>

#include <App/Application.h>
#include <App/Document.h>

#include <Mod/Part/App/PartFeature.h>

#include "MeasureRadius.h"


using namespace Measure;

PROPERTY_SOURCE(Measure::MeasureRadius, Measure::MeasureBase)



MeasureRadius::MeasureRadius()
{
    ADD_PROPERTY_TYPE(Element,(nullptr), "Measurement", App::Prop_None, "Element to get the radius from");
    Element.setScope(App::LinkScope::Global);
    Element.setAllowExternal(true);

    ADD_PROPERTY_TYPE(Radius,(0.0)       ,"Measurement",App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                                            "Radius of selection");

}

MeasureRadius::~MeasureRadius() = default;

//! validate all the object+subelement pairs in the selection. Must be circle or arc
//! and have a geometry handler available.  We only calculate radius if there is a
//! single valid item in the selection
bool MeasureRadius::isValidSelection(const App::MeasureSelection& selection){

    if (selection.empty() || selection.size() > 1) {
        // too few or too many selections
        return false;
    }

    App::Document* doc = App::GetApplication().getActiveDocument();
    const std::string& obName = get<0>(selection.front());
    App::DocumentObject* ob = doc->getObject(obName.c_str());

    const std::string& subName = get<1>(selection.front());
    const char* className = ob->getSubObject(subName.c_str())->getTypeId().getName();
    std::string mod = ob->getClassTypeId().getModuleName(className);

    if (!hasGeometryHandler(mod)) {
        return false;
    }

    App::MeasureHandler handler = App::GetApplication().getMeasureHandler(mod.c_str());
    App::MeasureElementType type = handler.typeCb(obName.c_str(), subName.c_str());

    if (type == App::MeasureElementType::INVALID) {
        return false;
    }

    if (type != App::MeasureElementType::CIRCLE
        && type != App::MeasureElementType::ARC) {
        return false;
    }

    return true;
}

//! return true if the selection is particularly interesting to MeasureRadius.
//! In this case we claim circles and arcs.
bool MeasureRadius::isPrioritizedSelection(const App::MeasureSelection& selection) {
    if (selection.empty()) {
        return false;
    }
    App::Document* doc = App::GetApplication().getActiveDocument();
    std::string firstObjectName = get<0>(selection.front());
    App::DocumentObject* firstObject = doc->getObject(firstObjectName.c_str());
    std::string firstSubName = get<1>(selection.front());
    if (!firstObject) {
        return false;
    }
    TopoDS_Shape firstShape = Part::Feature::getTopoShape(firstObject).getSubShape(firstSubName.c_str());
    TopoDS_Edge firstEdge;
    if (firstShape.ShapeType() == TopAbs_EDGE) {
        firstEdge = TopoDS::Edge(firstShape);
    } else if (firstShape.ShapeType() == TopAbs_WIRE) {
        TopoDS_Wire firstWire = TopoDS::Wire(firstShape);
        TopExp_Explorer edges(firstWire, TopAbs_EDGE);
        if (edges.More()) {
            firstEdge = TopoDS::Edge(edges.Current());
        }
    } else {
        // only edge or wire can have radius
        return false;
    }

    BRepAdaptor_Curve adapt(firstEdge);

    return selection.size() == 1  && adapt.GetType() == GeomAbs_Circle;
}


//! Set properties from first item in selection. assumes a valid selection.
void MeasureRadius::parseSelection(const App::MeasureSelection& selection) {
    App::Document* doc = App::GetApplication().getActiveDocument();

    std::vector<App::DocumentObject*> objects;
    std::vector<const char*> subElements;

    const std::tuple<std::string, std::string>& element = selection.front();
    App::DocumentObject* ob = doc->getObject(get<0>(element).c_str());
    std::string subElement = get<1>(element);
    std::vector<std::string> subElementList { subElement };

    Element.setValue(ob, subElementList);

    initialize();
}


App::DocumentObjectExecReturn *MeasureRadius::execute()
{
    recalculateRadius();
    return DocumentObject::StdReturn;
}


void MeasureRadius::recalculateRadius()
{
    Radius.setValue(getMeasureInfoFirst().radius);
}

void MeasureRadius::onChanged(const App::Property* prop)
{
    if (isRestoring() || isRemoving()) {
        return;
    }

    if (prop == &Element) {
        recalculateRadius();
    }
    
    MeasureBase::onChanged(prop);
}


//! return a placement (location + orientation) for the first element
Base::Placement MeasureRadius::getPlacement() {
    auto loc = getMeasureInfoFirst().pointOnCurve;
    auto p = Base::Placement();
    p.setPosition(loc);
    return p;
}


//! return the pointOnCurve element in MeasureRadiusInfo for the first element
Base::Vector3d MeasureRadius::getPointOnCurve() const
{
    return getMeasureInfoFirst().pointOnCurve;
}

//! get the handler's result for the first element
MeasureRadiusInfo MeasureRadius::getMeasureInfoFirst() const
{
   const App::DocumentObject* object = Element.getValue();
    const std::vector<std::string>& subElements = Element.getSubValues();

    if (!object || subElements.empty()) {
// NOLINTNEXTLINE(modernize-return-braced-init-list)
        return MeasureRadiusInfo();
    }

    std::string subElement = subElements.front();
    const char* className = object->getSubObject(subElement.c_str())->getTypeId().getName();
    const std::string& mod = object->getClassTypeId().getModuleName(className);

    auto handler = getGeometryHandler(mod);
    if (!handler) {
        throw Base::RuntimeError("No geometry handler available for submitted element type");
    }

    std::string obName = object->getNameInDocument();
    return handler(&obName, &subElement);
}

//! Return the object we are measuring
//! used by the viewprovider in determining visibility
std::vector<App::DocumentObject*> MeasureRadius::getSubject() const
{
    return {Element.getValue()};
}
