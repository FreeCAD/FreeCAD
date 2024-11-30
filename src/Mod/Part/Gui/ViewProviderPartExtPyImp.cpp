// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
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
#include <App/PropertyStandard.h>

#include "ViewProviderPartExtPy.h"
#include "ViewProviderPartExtPy.cpp"


using namespace PartGui;

// returns a string which represents the object e.g. when printed in python
std::string ViewProviderPartExtPy::representation() const
{
    std::stringstream str;
    str << "<View provider geometry object at " << getViewProviderPartExtPtr() << ">";

    return str.str();
}

PyObject* ViewProviderPartExtPy::getCustomAttributes(const char* attr) const
{
    ViewProviderPartExt* vp = getViewProviderPartExtPtr();
    if (strcmp(attr, "DiffuseColor") == 0) {
        // Get the color properties
        App::PropertyColorList prop;

        std::vector<App::Color> colors = vp->ShapeAppearance.getDiffuseColors();
        std::vector<float> transparencies = vp->ShapeAppearance.getTransparencies();
        for (int i = 0; i < static_cast<int>(colors.size()); i++) {
            colors[i].setTransparency(transparencies[i]);
        }

        prop.setValues(colors);
        return prop.getPyObject();
    }
    return nullptr;
}

int ViewProviderPartExtPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    ViewProviderPartExt* vp = getViewProviderPartExtPtr();
    if (strcmp(attr, "DiffuseColor") == 0) {
        // Set the color properties
        App::PropertyColorList prop;
        prop.setPyObject(obj);

        std::vector<App::Color> colors = prop.getValues();
        std::vector<float> transparencies;
        transparencies.resize(static_cast<int>(colors.size()));
        for (int i = 0; i < static_cast<int>(colors.size()); i++) {
            transparencies[i] = colors[i].transparency();
            colors[i].a = 1.0F;
        }
        vp->ShapeAppearance.setDiffuseColors(colors);
        vp->ShapeAppearance.setTransparencies(transparencies);
        return 1;
    }
    return 0;
}
