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

#include <Standard_math.hxx>
#ifndef _PreComp_
# include <Inventor/nodes/SoSeparator.h>
#endif
#include <Gui/ViewProviderBuilder.h>

#include "ViewProviderPython.h"


using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderCustom, PartGui::ViewProviderPart)

ViewProviderCustom::ViewProviderCustom() = default;

ViewProviderCustom::~ViewProviderCustom() = default;

void ViewProviderCustom::onChanged(const App::Property* prop)
{
    std::map<const App::Property*, Gui::ViewProvider*>::iterator it;
    for (it = propView.begin(); it != propView.end(); ++it) {
        App::Property* view = it->second->getPropertyByName(prop->getName());
        if (view) {
            App::Property* copy = prop->Copy();
            if (copy) {
                view->Paste(*copy);
                delete copy;
            }
        }
    }
    PartGui::ViewProviderPart::onChanged(prop);
}

void ViewProviderCustom::updateData(const App::Property* prop)
{
    if (prop->getTypeId().isDerivedFrom(App::PropertyComplexGeoData::getClassTypeId())) {
        std::map<const App::Property*, Gui::ViewProvider*>::iterator it = propView.find(prop);
        if (it == propView.end()) {
            Gui::ViewProvider* view = Gui::ViewProviderBuilder::create(prop->getTypeId());
            if (view) {
                if (view->getTypeId().isDerivedFrom(Gui::ViewProviderDocumentObject::getClassTypeId())) {
                    static_cast<Gui::ViewProviderDocumentObject*>(view)->attach(this->getObject());
                    static_cast<Gui::ViewProviderDocumentObject*>(view)->setDisplayMode(this->getActiveDisplayMode().c_str());
                }
                propView[prop] = view;
                view->updateData(prop);
                this->getRoot()->addChild(view->getRoot());
            }
        }
        else {
            it->second->updateData(prop);
        }
    }
}

// -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(PartGui::ViewProviderPython, PartGui::ViewProviderPart)
/// @endcond

// explicit template instantiation
template class PartGuiExport ViewProviderPythonFeatureT<PartGui::ViewProviderPart>;

/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(PartGui::ViewProviderCustomPython, PartGui::ViewProviderCustom)
/// @endcond

// explicit template instantiation
template class PartGuiExport ViewProviderPythonFeatureT<PartGui::ViewProviderCustom>;
}

