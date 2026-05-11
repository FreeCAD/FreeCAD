// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <BRep_Builder.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS_Compound.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include <App/GeoFeatureGroupExtension.h>
#include <App/GroupExtension.h>

#include "FeatureCompound.h"


using namespace Part;

namespace
{

App::DocumentObject* collectCompoundShape(App::DocumentObject* obj,
                                          std::set<App::DocumentObject*>& visited,
                                          std::vector<TopoShape>& shapes)
{
    if (!obj || !visited.insert(obj).second) {
        return nullptr;
    }

    if (App::GeoFeatureGroupExtension::isNonGeoGroup(obj)) {
        auto group = obj->getExtensionByType<App::GroupExtension>(true);
        if (group) {
            App::DocumentObject* firstShapeObject = nullptr;
            for (auto child : group->getObjects()) {
                auto shapeObject = collectCompoundShape(child, visited, shapes);
                if (!firstShapeObject) {
                    firstShapeObject = shapeObject;
                }
            }
            if (firstShapeObject) {
                return firstShapeObject;
            }
        }
    }

    auto shape = Feature::getTopoShape(obj, ShapeOption::ResolveLink | ShapeOption::Transform);
    if (shape.isNull()) {
        return nullptr;
    }

    shapes.push_back(shape);
    return obj;
}

}  // namespace

PROPERTY_SOURCE(Part::Compound, Part::Feature)

Compound::Compound()
{
    ADD_PROPERTY(Links, (nullptr));
    Links.setSize(0);
}

Compound::~Compound() = default;

short Compound::mustExecute() const
{
    if (Links.isTouched()) {
        return 1;
    }
    return 0;
}

App::DocumentObjectExecReturn* Compound::execute()
{
    try {
        // avoid duplicates without changing the order
        // See also ViewProviderCompound::updateData
        std::set<App::DocumentObject*> tempLinks;

        std::vector<TopoShape> shapes;
        App::DocumentObject* materialLink = nullptr;
        for (auto obj : Links.getValues()) {
            auto shapeObject = collectCompoundShape(obj, tempLinks, shapes);
            if (!materialLink) {
                materialLink = shapeObject;
            }
        }
        this->Shape.setValue(TopoShape().makeElementCompound(shapes));
        if (materialLink) {
            copyMaterial(materialLink);
        }
        return Part::Feature::execute();
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

////////////////////////////////////////////////////////////////////////

PROPERTY_SOURCE(Part::Compound2, Part::Compound)

Compound2::Compound2()
{
    Shape.setStatus(App::Property::Transient, true);
}

void Compound2::onDocumentRestored()
{
    Base::Placement pla = Placement.getValue();
    auto res = execute();
    delete res;
    Placement.setValue(pla);
}
