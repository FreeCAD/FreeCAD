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
#include <App/Document.h>
#include <Gui/Selection/Selection.h>
#include <Gui/BitmapFactory.h>
#include <QMenu>
#include <QAction>
#include <QObject>

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
    ViewProviderGroupExtension::initExtension(this);

    sPixmap = "folder";
}

ViewProviderDocumentObjectGroup::~ViewProviderDocumentObjectGroup() = default;

std::vector<std::string> ViewProviderDocumentObjectGroup::getDisplayModes() const
{
    // empty
    return {};
}

bool ViewProviderDocumentObjectGroup::isShow() const
{
    return Visibility.getValue();
}

QIcon ViewProviderDocumentObjectGroup::getIcon() const
{
    return mergeGreyableOverlayIcons (Gui::BitmapFactory().iconFromTheme(sPixmap));
}

/**
 * Extracts the associated view providers of the objects of the associated object group group.
 */
void ViewProviderDocumentObjectGroup::getViewProviders(std::vector<ViewProviderDocumentObject*>& vp) const
{
    App::DocumentObject* doc = getObject();
    if (doc->isDerivedFrom<App::DocumentObjectGroup>()) {
        Gui::Document* gd = Application::Instance->getDocument(doc->getDocument());
        auto grp = static_cast<App::DocumentObjectGroup*>(doc);
        std::vector<App::DocumentObject*> obj = grp->getObjects();
        for (const auto & it : obj) {
            ViewProvider* v = gd->getViewProvider(it);
            if (v && v->isDerivedFrom<ViewProviderDocumentObject>())
                vp.push_back(static_cast<ViewProviderDocumentObject*>(v));
        }
    }
}

void ViewProviderDocumentObjectGroup::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    // First, call the base class implementation to get all the standard menu items.
    ViewProviderDocumentObject::setupContextMenu(menu, receiver, member);

    App::DocumentObject* obj = this->getObject();

    // This action is only for plain "Std::Group" objects, not for derived types like "Draft::Layer".
    // We check the TypeId to ensure we don't add this menu to other group-like objects.
    if (obj->getTypeId() == App::DocumentObjectGroup::getClassTypeId()) {
        auto* group = static_cast<App::DocumentObjectGroup*>(obj);

        // Only add the action if the group actually contains objects.
        if (group && !group->getObjects().empty()) {
            // Add the custom action.
            QIcon icon = BitmapFactory().iconFromTheme("Std_SelectGroupContents");
            QAction* selectAction = new QAction(icon, QObject::tr("Select group contents"), menu);
            selectAction->setToolTip(QObject::tr("Selects all objects that are children of this group."));

            // Connect the action's triggered signal to a lambda function that performs the selection.
            QObject::connect(selectAction, &QAction::triggered, [group]() {
                if (!group) return;

                // Use getAllChildren() to recursively select contents of subgroups.
                const auto& children = group->getAllChildren();

                if (!children.empty()) {
                    const char* docName = group->getDocument()->getName();

                    Gui::Selection().clearSelection(docName);

                    // Add each child object to the selection individually.
                    for (App::DocumentObject* child : children) {
                        if (child && child->isAttachedToDocument()) {
                            Gui::Selection().addSelection(docName, child->getNameInDocument());
                        }
                    }
                }
            });

            // Insert the action at the top of the menu for better visibility.
            menu->insertAction(menu->actions().isEmpty() ? nullptr : menu->actions().first(), selectAction);
        }
    }
}

// Python feature -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderDocumentObjectGroupPython, Gui::ViewProviderDocumentObjectGroup)
/// @endcond

// explicit template instantiation
template class GuiExport ViewProviderFeaturePythonT<ViewProviderDocumentObjectGroup>;
}
