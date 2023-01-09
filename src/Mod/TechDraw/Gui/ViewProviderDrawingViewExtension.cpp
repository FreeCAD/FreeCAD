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

#include <App/Document.h>

#include "ViewProviderDrawingViewExtension.h"
#include "ViewProviderDrawingView.h"
#include "ViewProviderPage.h"


using namespace TechDrawGui;

EXTENSION_PROPERTY_SOURCE(TechDrawGui::ViewProviderDrawingViewExtension, Gui::ViewProviderExtension)

ViewProviderDrawingViewExtension::ViewProviderDrawingViewExtension()
{
    initExtensionType(ViewProviderDrawingViewExtension::getExtensionClassTypeId());
}

ViewProviderDrawingViewExtension::~ViewProviderDrawingViewExtension() {}

bool ViewProviderDrawingViewExtension::extensionCanDragObjects() const { return true; }

//we don't support dragging children of Views (Dimensions, Balloons, Hatches, etc) now, but we don't want another
//extension to drag our children and cause problems
bool ViewProviderDrawingViewExtension::extensionCanDragObject(App::DocumentObject* docObj) const
{
    (void)docObj;
    return true;
}

//the default drag will remove the object from the document until it is dropped and re-added, so we claim
//to do the dragging.
void ViewProviderDrawingViewExtension::extensionDragObject(App::DocumentObject* obj) { (void)obj; }

//we don't support dropping of new children of Views (Dimensions, Balloons, Hatches, etc) now, but we don't want another
//extension to try to drop on us and cause problems
bool ViewProviderDrawingViewExtension::extensionCanDropObjects() const { return true; }

//let the page have any drops we receive
bool ViewProviderDrawingViewExtension::extensionCanDropObject(App::DocumentObject* obj) const
{
    return getViewProviderDrawingView()
        ->getViewProviderPage()
        ->getVPPExtension()
        ->extensionCanDropObject(obj);
}

//let the page have any drops we receive
void ViewProviderDrawingViewExtension::extensionDropObject(App::DocumentObject* obj)
{
    getViewProviderDrawingView()->getViewProviderPage()->getVPPExtension()->extensionDropObject(
        obj);
}

const ViewProviderDrawingView* ViewProviderDrawingViewExtension::getViewProviderDrawingView() const
{
    return dynamic_cast<const ViewProviderDrawingView*>(getExtendedViewProvider());
}

const char* ViewProviderDrawingViewExtension::whoAmI() const
{
    auto parent = getViewProviderDrawingView();
    if (parent) {
        return parent->whoAmI();
    }
    return nullptr;
}

namespace Gui
{
EXTENSION_PROPERTY_SOURCE_TEMPLATE(TechDrawGui::ViewProviderDrawingViewExtensionPython,
                                   TechDrawGui::ViewProviderDrawingViewExtension)

// explicit template instantiation
template class TechDrawGuiExport
    ViewProviderExtensionPythonT<TechDrawGui::ViewProviderDrawingViewExtension>;
}// namespace Gui
