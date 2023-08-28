/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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

#include <App/DocumentObject.h>

#include "ViewProviderTemplateExtension.h"
#include "ViewProviderTemplate.h"


using namespace TechDrawGui;

EXTENSION_PROPERTY_SOURCE(TechDrawGui::ViewProviderTemplateExtension, Gui::ViewProviderExtension)

ViewProviderTemplateExtension::ViewProviderTemplateExtension()
{
    initExtensionType(ViewProviderTemplateExtension::getExtensionClassTypeId());
}

ViewProviderTemplateExtension::~ViewProviderTemplateExtension() {}

//there are no child objects to drag currently, so we will say we handle any dragging rather than letting some
//other extension trying to drag and causing problems.
bool ViewProviderTemplateExtension::extensionCanDragObjects() const { return true; }

//there are no child objects to drag currently, so we will say we handle any dragging
bool ViewProviderTemplateExtension::extensionCanDragObject(App::DocumentObject* docObj) const
{
    (void)docObj;
    return true;
}

//templates do not accept drops, so rather that let some other extension try to drop into a template, we will
//claim that we can handle drops
bool ViewProviderTemplateExtension::extensionCanDropObjects() const { return true; }

//templates do not accept drops, so rather that let some other extension try to drop into a template, we will
//claim that we can handle drops
bool ViewProviderTemplateExtension::extensionCanDropObject(App::DocumentObject* docObj) const
{
    (void)docObj;
    return true;
}

const ViewProviderTemplate* ViewProviderTemplateExtension::getViewProviderTemplate() const
{
    return dynamic_cast<const ViewProviderTemplate*>(getExtendedViewProvider());
}

const char* ViewProviderTemplateExtension::whoAmI() const
{
    auto parent = getViewProviderTemplate();
    if (parent) {
        return parent->whoAmI();
    }
    return nullptr;
}

namespace Gui
{
EXTENSION_PROPERTY_SOURCE_TEMPLATE(TechDrawGui::ViewProviderTemplateExtensionPython,
                                   TechDrawGui::ViewProviderTemplateExtension)

// explicit template instantiation
template class TechDrawGuiExport
    ViewProviderExtensionPythonT<TechDrawGui::ViewProviderTemplateExtension>;
}// namespace Gui
