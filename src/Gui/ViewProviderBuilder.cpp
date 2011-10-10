/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Inventor/nodes/SoMaterial.h>
#endif

#include "ViewProviderBuilder.h"
#include <App/PropertyStandard.h>

using namespace Gui;

std::map<Base::Type, Base::Type> ViewProviderBuilder::_prop_to_view;

ViewProviderBuilder::ViewProviderBuilder() 
{
}

ViewProviderBuilder::~ViewProviderBuilder()
{
}

void ViewProviderBuilder::add(const Base::Type& prop, const Base::Type& view)
{
    _prop_to_view[prop] = view;
}

ViewProvider* ViewProviderBuilder::create(const Base::Type& type)
{
    std::map<Base::Type, Base::Type>::iterator it = _prop_to_view.find(type);
    if (it != _prop_to_view.end())
        return reinterpret_cast<ViewProvider*>(it->second.createInstance());
    return 0;
}

// --------------------------------------

ViewProviderColorBuilder::ViewProviderColorBuilder() 
{
}

ViewProviderColorBuilder::~ViewProviderColorBuilder()
{
}

void ViewProviderColorBuilder::buildNodes(const App::Property* prop, std::vector<SoNode*>&) const
{
    const App::PropertyColorList* color = static_cast<const App::PropertyColorList*>(prop);
    const std::vector<App::Color>& val = color->getValues();
    unsigned long i=0;

    SoMaterial* material = new SoMaterial();
    material->enableNotify(false);
    material->diffuseColor.deleteValues(0);
    material->diffuseColor.setNum(val.size());

    for (std::vector<App::Color>::const_iterator it = val.begin(); it != val.end(); ++it) {
        material->diffuseColor.set1Value(i++, SbColor(it->r, it->g, it->b));
    }

    material->enableNotify(true);
    material->touch();
}
