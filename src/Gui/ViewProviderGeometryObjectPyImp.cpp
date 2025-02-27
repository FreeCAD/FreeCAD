// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef _PreComp_
#include <sstream>
#endif

#include <App/GeoFeature.h>
#include <App/MaterialPy.h>
#include <App/PropertyStandard.h>

#include "ViewProviderGeometryObjectPy.h"

#include "ViewProviderGeometryObjectPy.cpp"


using namespace Gui;

// returns a string which represents the object e.g. when printed in python
std::string ViewProviderGeometryObjectPy::representation() const
{
    std::stringstream str;
    str << "<View provider geometry object at " << getViewProviderGeometryObjectPtr() << ">";

    return str.str();
}

PyObject* ViewProviderGeometryObjectPy::getCustomAttributes(const char* attr) const
{
    ViewProviderGeometryObject* vp = getViewProviderGeometryObjectPtr();
    if (strcmp(attr, "ShapeColor") == 0) {
        // Get material property of ViewProviderGeometryObject
        App::PropertyColor prop;
        prop.setValue(vp->ShapeAppearance.getDiffuseColor());
        return prop.getPyObject();
    }
    if (strcmp(attr, "ShapeMaterial") == 0) {
        // Get material property of ViewProviderGeometryObject
        auto geometry = vp->getObject<App::GeoFeature>();
        if (geometry) {
            auto material = geometry->getMaterialAppearance();
            App::PropertyMaterial prop;
            prop.setValue(material);
            return prop.getPyObject();
        }
    }
    return nullptr;
}

PyObject* ViewProviderGeometryObjectPy::getUserDefinedMaterial()
{
    App::Material mat = App::Material::getDefaultAppearance();
    return new App::MaterialPy(new App::Material(mat));
}

int ViewProviderGeometryObjectPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    ViewProviderGeometryObject* vp = getViewProviderGeometryObjectPtr();
    if (strcmp(attr, "ShapeColor") == 0) {
        // Get material property of ViewProviderGeometryObject
        App::PropertyColor prop;
        prop.setPyObject(obj);
        vp->ShapeAppearance.setDiffuseColor(prop.getValue());
        // Assign the value to the new property type
        // ...
        return 1;
    }
    if (strcmp(attr, "ShapeMaterial") == 0) {
        // Get material property of ViewProviderGeometryObject
        auto geometry = vp->getObject<App::GeoFeature>();
        if (geometry) {
            App::PropertyMaterial prop;
            prop.setPyObject(obj);
            geometry->setMaterialAppearance(prop.getValue());
        }
        return 1;
    }
    return 0;
}
