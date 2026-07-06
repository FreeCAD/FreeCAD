/***************************************************************************
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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


#include <QCheckBox>
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
#include "Selection.h"


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

// Returns the currently selected objects that are non-empty groups, i.e. exactly
// the objects whose deletion will pop up a recursive-delete confirmation. The same
// group selected through several sub-elements is returned only once.
std::vector<App::DocumentObject*> selectedNonEmptyGroups()
{
    std::vector<App::DocumentObject*> groups;
    for (const auto& sel : Gui::Selection().getSelection()) {
        App::DocumentObject* obj = sel.pObject;
        if (!obj || !obj->hasExtension(App::GroupExtension::getExtensionClassTypeId())) {
            continue;
        }
        auto* group = obj->getExtensionByType<App::GroupExtension>();
        if (group->Group.getValues().empty()) {
            continue;
        }
        if (std::ranges::find(groups, obj) == groups.end()) {
            groups.push_back(obj);
        }
    }
    return groups;
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

bool ViewProviderGroupExtension::extensionOnDelete(const std::vector<std::string>& subNames)
{
    // If this group is being deleted as part of a parent group's recursive deletion,
    // the user already confirmed at the top level, so don't ask again. This matches
    // the marker checked by ViewProviderBoolean and ViewProviderCompound.
    bool inGroupDeletion = !subNames.empty() && subNames[0] == "group_recursive_deletion";
    if (inGroupDeletion) {
        return true;
    }

    auto* currentObj = getExtendedViewProvider()->getObject();
    auto* group = currentObj->getExtensionByType<App::GroupExtension>();

    std::vector<App::DocumentObject*> directChildren = group->Group.getValues();

    // just delete without messagebox if group is empty
    if (directChildren.empty()) {
        return true;
    }

    // The delete command calls this method once per selected object. When several
    // non-empty groups are selected we add an "Apply to all" checkbox so the user
    // can answer once for the whole selection instead of for every group.
    const std::vector<App::DocumentObject*> selectedGroups = selectedNonEmptyGroups();
    const bool multipleGroups = selectedGroups.size() > 1;

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

    QMessageBox msgBox(
        QMessageBox::Question,
        QObject::tr("Delete group contents recursively?"),
        message,
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
        getMainWindow()
    );
    msgBox.setDefaultButton(QMessageBox::No);

    QCheckBox* applyToAllCheckBox = nullptr;
    if (multipleGroups) {
        applyToAllCheckBox = new QCheckBox(
            QObject::tr("Apply to all selected objects (%1) and their children")
                .arg(selectedGroups.size()),
            &msgBox
        );
        msgBox.setCheckBox(applyToAllCheckBox);
    }

    msgBox.exec();
    QMessageBox::StandardButton choice = msgBox.standardButton(msgBox.clickedButton());
    const bool applyToAll = applyToAllCheckBox && applyToAllCheckBox->isChecked();

    if (choice == QMessageBox::Cancel) {
        // don't delete anything if user has cancelled
        return false;
    }

    if (choice == QMessageBox::No && applyToAll) {
        // Apply "No" to the whole selection: remove every other selected group now but
        // keep its children (removeObject reparents them to the document root). Those
        // groups are then already gone when their own delete call comes around; the
        // current group is left for the delete command to remove on return. We capture
        // names first because removing one group may already remove another (nested).
        std::vector<std::pair<App::Document*, std::string>> groupRefs;
        groupRefs.reserve(selectedGroups.size());
        for (App::DocumentObject* selObj : selectedGroups) {
            if (selObj != currentObj) {
                groupRefs.emplace_back(selObj->getDocument(), selObj->getNameInDocument());
            }
        }
        for (const auto& [doc, name] : groupRefs) {
            App::DocumentObject* obj = doc ? doc->getObject(name.c_str()) : nullptr;
            if (obj && obj->isAttachedToDocument() && !obj->isRemoving()) {
                doc->removeObject(name.c_str());
            }
        }
        return true;
    }

    if (choice == QMessageBox::Yes && applyToAll) {
        // Apply the decision to the whole selection right away by emptying every
        // selected non-empty group now. Each group's own delete call will then find
        // it empty and return without asking again. We capture the objects by name
        // first because emptying one group may already remove another (e.g. a nested
        // group), so we re-resolve and skip the ones that are already gone.
        std::vector<std::pair<App::Document*, std::string>> groupRefs;
        groupRefs.reserve(selectedGroups.size());
        for (App::DocumentObject* obj : selectedGroups) {
            groupRefs.emplace_back(obj->getDocument(), obj->getNameInDocument());
        }
        for (const auto& [doc, name] : groupRefs) {
            App::DocumentObject* obj = doc ? doc->getObject(name.c_str()) : nullptr;
            if (obj && obj->isAttachedToDocument() && !obj->isRemoving()
                && obj->hasExtension(App::GroupExtension::getExtensionClassTypeId())) {
                deleteGroupContentsRecursively(obj->getExtensionByType<App::GroupExtension>());
            }
        }
        return true;
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
