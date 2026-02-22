// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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


#include <QMessageBox>


#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>
#include <App/GroupExtension.h>
#include <Base/Console.h>
#include <Base/Tools.h>

#include "ViewProviderGroupExtension.h"
#include "ViewProviderDocumentObject.h"
#include "Application.h"
#include "Command.h"
#include "Document.h"
#include "MainWindow.h"


using namespace Gui;

namespace
{
// helper function to recursively delete group contents while respecting view provider onDelete methods
void deleteGroupContentsRecursively(App::GroupExtension* group)
{
    if (!group) {
        return;
    }

    std::vector<App::DocumentObject*> children = group->Group.getValues();

    for (App::DocumentObject* child : children) {
        if (!child || !child->isAttachedToDocument() || child->isRemoving()) {
            continue;
        }

        // if the child is a group, recursively delete its contents first
        if (child->hasExtension(App::GroupExtension::getExtensionClassTypeId())) {
            auto* childGroup = child->getExtensionByType<App::GroupExtension>();
            deleteGroupContentsRecursively(childGroup);
        }

        Gui::Document* guiDoc = Application::Instance->getDocument(child->getDocument());
        if (guiDoc) {
            ViewProvider* vp = guiDoc->getViewProvider(child);
            if (vp) {
                // give group_recursive_deletion marker to the VP to mark that the deletion
                // is supposed to delete all of its children
                std::vector<std::string> groupDeletionMarker = {"group_recursive_deletion"};
                bool shouldDelete = vp->onDelete(groupDeletionMarker);

                if (!shouldDelete) {
                    return;
                }
            }
        }

        // if the object still exists and wasn't deleted by its view provider, delete it directly
        if (child->isAttachedToDocument() && !child->isRemoving()) {
            child->getDocument()->removeObject(child->getNameInDocument());
        }
    }
}
}  // namespace

EXTENSION_PROPERTY_SOURCE(Gui::ViewProviderGroupExtension, Gui::ViewProviderExtension)

ViewProviderGroupExtension::ViewProviderGroupExtension()
{
    initExtensionType(ViewProviderGroupExtension::getExtensionClassTypeId());
}

ViewProviderGroupExtension::~ViewProviderGroupExtension() = default;

bool ViewProviderGroupExtension::extensionCanDragObjects() const
{
    return true;
}

bool ViewProviderGroupExtension::extensionCanDragObject(App::DocumentObject*) const
{

    // we can drag anything out
    return true;
}

void ViewProviderGroupExtension::extensionDragObject(App::DocumentObject* obj)
{

    Gui::Command::doCommand(
        Gui::Command::Doc,
        "App.getDocument(\"%s\").getObject(\"%s\").removeObject("
        "App.getDocument(\"%s\").getObject(\"%s\"))",
        getExtendedViewProvider()->getObject()->getDocument()->getName(),
        getExtendedViewProvider()->getObject()->getNameInDocument(),
        obj->getDocument()->getName(),
        obj->getNameInDocument()
    );
}

bool ViewProviderGroupExtension::extensionCanDropObjects() const
{
    return true;
}

bool ViewProviderGroupExtension::extensionCanDropObject(App::DocumentObject* obj) const
{
#ifdef FC_DEBUG
    Base::Console().log("Check ViewProviderGroupExtension\n");
#endif

    auto extobj = getExtendedViewProvider()->getObject();
    auto group = extobj->getExtensionByType<App::GroupExtension>();

    // we cannot drop thing of this group into it again if it does not allow reorder
    if (group->hasObject(obj) && !getExtendedViewProvider()->acceptReorderingObjects()) {
        return false;
    }

    // Check for possible cyclic dependencies if we allowed to drop the object
    const auto& list = obj->getOutList();
    if (std::ranges::find(list, extobj) != list.end()) {
        Base::Console().warning("Do not add cyclic dependency to %s\n", extobj->Label.getValue());
        return false;
    }

    return group->allowObject(obj);
}

void ViewProviderGroupExtension::extensionDropObject(App::DocumentObject* obj)
{

    auto grp = getExtendedViewProvider()->getObject();
    App::Document* doc = grp->getDocument();

    // build Python command for execution
    QString cmd;
    cmd = QStringLiteral(
              "App.getDocument(\"%1\").getObject(\"%2\").addObject("
              "App.getDocument(\"%1\").getObject(\"%3\"))"
    )
              .arg(
                  QString::fromUtf8(doc->getName()),
                  QString::fromUtf8(grp->getNameInDocument()),
                  QString::fromUtf8(obj->getNameInDocument())
              );

    Gui::Command::doCommand(Gui::Command::App, cmd.toUtf8());
}

std::vector<App::DocumentObject*> ViewProviderGroupExtension::extensionClaimChildren() const
{

    auto* obj = getExtendedViewProvider()->getObject();
    if (!obj) {
        return {};
    }

    auto* group = obj->getExtensionByType<App::GroupExtension>();
    return group->Group.getValues();
}

void ViewProviderGroupExtension::extensionShow()
{

    // avoid possible infinite recursion
    if (guard) {
        return;
    }
    Base::StateLocker lock(guard);

    // when reading the Visibility property from file then do not hide the
    // objects of this group because they have stored their visibility status, too
    //
    // Property::User1 is used by ViewProviderDocumentObject to mark for
    // temporary visibility changes. Do not propagate the change to children.
    if (!getExtendedViewProvider()->isRestoring()
        && !getExtendedViewProvider()->Visibility.testStatus(App::Property::User1)) {
        auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GroupExtension>();
        for (auto obj : group->Group.getValues()) {
            if (obj && !obj->Visibility.getValue()) {
                obj->Visibility.setValue(true);
            }
        }
    }

    ViewProviderExtension::extensionShow();
}

void ViewProviderGroupExtension::extensionHide()
{

    // avoid possible infinite recursion
    if (guard) {
        return;
    }
    Base::StateLocker lock(guard);

    // when reading the Visibility property from file then do not hide the
    // objects of this group because they have stored their visibility status, too
    //
    // Property::User1 is used by ViewProviderDocumentObject to mark for
    // temporary visibility changes. Do not propagate the change to children.
    if (!getExtendedViewProvider()->isRestoring()
        && !getExtendedViewProvider()->Visibility.testStatus(App::Property::User1)) {
        auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GroupExtension>();
        for (auto obj : group->Group.getValues()) {
            if (obj && obj->Visibility.getValue()) {
                obj->Visibility.setValue(false);
            }
        }
    }
    ViewProviderExtension::extensionHide();
}

bool ViewProviderGroupExtension::extensionOnDelete(const std::vector<std::string>&)
{

    auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GroupExtension>();

    std::vector<App::DocumentObject*> directChildren = group->Group.getValues();

    // just delete without messagebox if group is empty
    if (directChildren.empty()) {
        return true;
    }

    const auto* docGroup = freecad_cast<App::DocumentObjectGroup*>(
        getExtendedViewProvider()->getObject()
    );
    auto allDescendants = docGroup ? docGroup->getAllChildren() : directChildren;

    QString message;
    if (allDescendants.size() == directChildren.size()) {
        message
            = QObject::tr("The group '%1' contains %2 object(s). Do you want to delete them as well?")
                  .arg(QString::fromUtf8(getExtendedViewProvider()->getObject()->Label.getValue()))
                  .arg(allDescendants.size());
    }
    else {
        // if we have nested groups
        message = QObject::tr(
                      "The group '%1' contains %2 direct children and %3 total descendants "
                      "(including nested groups). Do you want to delete all of them recursively?"
        )
                      .arg(QString::fromUtf8(getExtendedViewProvider()->getObject()->Label.getValue()))
                      .arg(directChildren.size())
                      .arg(allDescendants.size());
    }

    QMessageBox::StandardButton choice = QMessageBox::question(
        getMainWindow(),
        QObject::tr("Delete group contents recursively?"),
        message,
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
        QMessageBox::No
    );

    if (choice == QMessageBox::Cancel) {
        // don't delete anything if user has cancelled
        return false;
    }

    if (choice == QMessageBox::Yes) {
        // delete all of the children recursively and call their viewprovider method
        deleteGroupContentsRecursively(group);
    }
    // if user has specified "No" then delete the group but move children to the parent or root

    return true;
}


namespace Gui
{
EXTENSION_PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderGroupExtensionPython, Gui::ViewProviderGroupExtension)

// explicit template instantiation
template class GuiExport ViewProviderExtensionPythonT<ViewProviderGroupExtension>;
}  // namespace Gui
