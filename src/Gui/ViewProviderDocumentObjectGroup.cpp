/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/DocumentObjectGroup.h>

#include "ViewProviderDocumentObjectGroup.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Document.h"


using namespace Gui;


PROPERTY_SOURCE_WITH_EXTENSIONS(Gui::ViewProviderDocumentObjectGroup, Gui::ViewProviderDocumentObject)


/**
 * Creates the view provider for an object group.
 */
ViewProviderDocumentObjectGroup::ViewProviderDocumentObjectGroup()
{
#if 0
    setDefaultMode(SO_SWITCH_ALL);
#endif
    ViewProviderGroupExtension::initExtension(this);

    sPixmap = "folder";
}

ViewProviderDocumentObjectGroup::~ViewProviderDocumentObjectGroup()
{
}

std::vector<std::string> ViewProviderDocumentObjectGroup::getDisplayModes(void) const
{
    // empty
    return std::vector<std::string>();
}

bool ViewProviderDocumentObjectGroup::isShow(void) const
{
    return Visibility.getValue();
}

QIcon ViewProviderDocumentObjectGroup::getIcon(void) const
{
    return mergeGreyableOverlayIcons (Gui::BitmapFactory().iconFromTheme(sPixmap));
}

/**
 * Extracts the associated view providers of the objects of the associated object group group.
 */
void ViewProviderDocumentObjectGroup::getViewProviders(std::vector<ViewProviderDocumentObject*>& vp) const
{
    App::DocumentObject* doc = getObject();
    if (doc->getTypeId().isDerivedFrom(App::DocumentObjectGroup::getClassTypeId())) {
        Gui::Document* gd = Application::Instance->getDocument(doc->getDocument());
        App::DocumentObjectGroup* grp = (App::DocumentObjectGroup*)doc;
        std::vector<App::DocumentObject*> obj = grp->getObjects();
        for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it) {
            ViewProvider* v = gd->getViewProvider(*it);
            if (v && v->getTypeId().isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()))
                vp.push_back((ViewProviderDocumentObject*)v);
        }
    }
}


// Python feature -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderDocumentObjectGroupPython, Gui::ViewProviderDocumentObjectGroup)
/// @endcond

// explicit template instantiation
template class GuiExport ViewProviderPythonFeatureT<ViewProviderDocumentObjectGroup>;
}
