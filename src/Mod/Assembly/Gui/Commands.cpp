// SPDX - License - Identifier: LGPL - 2.1 - or -later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pierre-Louis Boyer                                  *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <vector>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>

#include <Mod/Assembly/App/AssemblyObject.h>
#include <Mod/Assembly/App/AssemblyUtils.h>

#include "Commands.h"
#include "ViewProviderAssembly.h"


using namespace Assembly;
using namespace AssemblyGui;

// Helper function to get the active AssemblyObject in edit mode
static AssemblyObject* getActiveAssembly()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (!doc) {
        return nullptr;
    }

    auto* vp = doc->getInEdit();
    if (auto* assemblyVP = freecad_cast<ViewProviderAssembly*>(vp)) {
        return assemblyVP->getObject<AssemblyObject>();
    }

    return nullptr;
}

void selectObjects(const std::vector<App::DocumentObject*>& objectsToSelect)
{
    if (objectsToSelect.empty()) {
        return;
    }

    Gui::Selection().clearSelection();
    for (App::DocumentObject* obj : objectsToSelect) {
        Gui::Selection().addSelection(obj->getDocument()->getName(), obj->getNameInDocument());
    }
}

void selectObjectsByName(AssemblyObject* assembly, const std::vector<std::string>& names)
{
    if (!assembly || names.empty()) {
        return;
    }

    std::vector<App::DocumentObject*> objectsToSelect;
    App::Document* doc = assembly->getDocument();

    for (const auto& name : names) {
        if (auto* obj = doc->getObject(name.c_str())) {
            objectsToSelect.push_back(obj);
        }
    }

    selectObjects(objectsToSelect);
}

// ================================================================================
// Select Conflicting Constraints
// ================================================================================

DEF_STD_CMD_A(CmdAssemblySelectConflictingConstraints)

CmdAssemblySelectConflictingConstraints::CmdAssemblySelectConflictingConstraints()
    : Command("Assembly_SelectConflictingConstraints")
{
    sGroup = QT_TR_NOOP("Assembly");
    sMenuText = QT_TR_NOOP("Select conflicting constraints");
    sToolTipText = QT_TR_NOOP("Selects conflicting joints in the active assembly");
    sWhatsThis = "Assembly_SelectConflictingConstraints";
    sStatusTip = sToolTipText;
    eType = ForEdit;
}

void CmdAssemblySelectConflictingConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    AssemblyObject* assembly = getActiveAssembly();
    if (!assembly) {
        return;
    }

    // NOTE: The solver currently reports conflicting constraints as redundant.
    // This uses the redundant list until the solver provides a separate conflicting list.
    selectObjectsByName(assembly, assembly->getLastRedundant());
}

bool CmdAssemblySelectConflictingConstraints::isActive()
{
    return getActiveAssembly() != nullptr;
}

// ================================================================================
// Select Redundant Constraints
// ================================================================================

DEF_STD_CMD_A(CmdAssemblySelectRedundantConstraints)

CmdAssemblySelectRedundantConstraints::CmdAssemblySelectRedundantConstraints()
    : Command("Assembly_SelectRedundantConstraints")
{
    sGroup = QT_TR_NOOP("Assembly");
    sMenuText = QT_TR_NOOP("Select redundant constraints");
    sToolTipText = QT_TR_NOOP("Selects redundant joints in the active assembly");
    sWhatsThis = "Assembly_SelectRedundantConstraints";
    sStatusTip = sToolTipText;
    eType = ForEdit;
}

void CmdAssemblySelectRedundantConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    AssemblyObject* assembly = getActiveAssembly();
    if (!assembly) {
        return;
    }

    selectObjectsByName(assembly, assembly->getLastRedundant());
}

bool CmdAssemblySelectRedundantConstraints::isActive()
{
    return getActiveAssembly() != nullptr;
}

// ================================================================================
// Select Malformed Constraints
// ================================================================================

DEF_STD_CMD_A(CmdAssemblySelectMalformedConstraints)

CmdAssemblySelectMalformedConstraints::CmdAssemblySelectMalformedConstraints()
    : Command("Assembly_SelectMalformedConstraints")
{
    sGroup = QT_TR_NOOP("Assembly");
    sMenuText = QT_TR_NOOP("Select malformed constraints");
    sToolTipText = QT_TR_NOOP("Selects malformed joints in the active assembly");
    sWhatsThis = "Assembly_SelectMalformedConstraints";
    sStatusTip = sToolTipText;
    eType = ForEdit;
}

void CmdAssemblySelectMalformedConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    AssemblyObject* assembly = getActiveAssembly();
    if (!assembly) {
        return;
    }

    selectObjectsByName(assembly, assembly->getLastMalformed());
}

bool CmdAssemblySelectMalformedConstraints::isActive()
{
    return getActiveAssembly() != nullptr;
}


// ================================================================================
// Select Components with Degrees of Freedom
// ================================================================================

DEF_STD_CMD_A(CmdAssemblySelectComponentsWithDoFs)

CmdAssemblySelectComponentsWithDoFs::CmdAssemblySelectComponentsWithDoFs()
    : Command("Assembly_SelectComponentsWithDoFs")
{
    sGroup = QT_TR_NOOP("Assembly");
    sMenuText = QT_TR_NOOP("Select components with DoFs");
    sToolTipText = QT_TR_NOOP("Selects unconstrained components in the active assembly");
    sWhatsThis = "Assembly_SelectComponentsWithDoFs";
    sStatusTip = sToolTipText;
    eType = ForEdit;
}

void CmdAssemblySelectComponentsWithDoFs::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    AssemblyObject* assembly = getActiveAssembly();
    if (!assembly) {
        return;
    }

    std::vector<App::DocumentObject*> objectsToSelect;
    std::vector<App::DocumentObject*> allParts = getAssemblyComponents(assembly);

    // Iterate through all collected parts and check their connectivity
    for (App::DocumentObject* part : allParts) {
        if (!assembly->isPartConnected(part)) {
            objectsToSelect.push_back(part);
        }
    }

    selectObjects(objectsToSelect);
}

bool CmdAssemblySelectComponentsWithDoFs::isActive()
{
    return getActiveAssembly() != nullptr;
}

// ================================================================================
// Select Joints of Component
// ================================================================================

DEF_STD_CMD_A(CmdAssemblySelectJointsOfComponent)

CmdAssemblySelectJointsOfComponent::CmdAssemblySelectJointsOfComponent()
    : Command("Assembly_SelectJointsOfComponent")
{
    sGroup = QT_TR_NOOP("Assembly");
    sMenuText = QT_TR_NOOP("Select component joints");
    sToolTipText = QT_TR_NOOP("Selects all joints referencing the selected component");
    sWhatsThis = "Assembly_SelectJointsOfComponent";
    sStatusTip = sToolTipText;
    sPixmap = "Assembly_SelectJointsOfComponent";
    eType = ForEdit;
}

void CmdAssemblySelectJointsOfComponent::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    AssemblyObject* assembly = getActiveAssembly();
    if (!assembly) {
        return;
    }

    auto selection = Gui::Selection().getSelectionEx(
        "",
        App::DocumentObject::getClassTypeId(),
        Gui::ResolveMode::NoResolve
    );
    if (selection.empty()) {
        return;
    }

    std::set<App::DocumentObject*> components;
    App::Document* doc = assembly->getDocument();

    for (auto& sel : selection) {
        const std::vector<std::string> subs = sel.getSubNames();
        std::string sub = subs.empty() ? "" : subs.front();

        App::DocumentObject* comp = getMovingPartFromSel(assembly, sel.getObject(), sub);
        if (comp) {
            components.insert(comp);
        }
    }

    if (components.empty()) {
        return;
    }

    std::vector<App::DocumentObject*> jointsToSelect;
    for (auto* comp : components) {
        std::vector<App::DocumentObject*> partJoints = assembly->getJointsOfPart(comp);
        jointsToSelect.insert(jointsToSelect.end(), partJoints.begin(), partJoints.end());
    }

    selectObjects(jointsToSelect);
}

bool CmdAssemblySelectJointsOfComponent::isActive()
{
    return getActiveAssembly() != nullptr && !Gui::Selection().getSelection().empty();
}


// ================================================================================
// Command Creation
// ================================================================================

void AssemblyGui::CreateAssemblyCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdAssemblySelectConflictingConstraints());
    rcCmdMgr.addCommand(new CmdAssemblySelectRedundantConstraints());
    rcCmdMgr.addCommand(new CmdAssemblySelectMalformedConstraints());
    rcCmdMgr.addCommand(new CmdAssemblySelectComponentsWithDoFs());
    rcCmdMgr.addCommand(new CmdAssemblySelectJointsOfComponent());
}
