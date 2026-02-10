// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pierre-Louis Boyer                                  *
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

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
// #include <Mod/Assembly/App/AssemblyObject.h>

#include "TaskAssemblyMessages.h"
#include "ViewProviderAssembly.h"

using namespace AssemblyGui;
using namespace Gui::TaskView;
namespace sp = std::placeholders;

TaskAssemblyMessages::TaskAssemblyMessages(ViewProviderAssembly* vp)
    : TaskSolverMessages(Gui::BitmapFactory().pixmap("Geoassembly"), tr("Solver messages"))
    , vp(vp)
{
    // NOLINTBEGIN
    connectionSetUp = vp->signalSetUp.connect(
        std::bind(&TaskAssemblyMessages::slotSetUp, this, sp::_1, sp::_2, sp::_3, sp::_4)
    );
    // NOLINTEND
}

TaskAssemblyMessages::~TaskAssemblyMessages()
{
    connectionSetUp.disconnect();
}

void TaskAssemblyMessages::updateToolTip(const QString& link)
{
    if (link == QStringLiteral("#conflicting")) {
        setLinkTooltip(tr("Click to select these conflicting joints."));
    }
    else if (link == QStringLiteral("#redundant")) {
        setLinkTooltip(tr("Click to select these redundant joints."));
    }
    else if (link == QStringLiteral("#dofs")) {
        setLinkTooltip(tr(
            "The assembly has unconstrained components giving rise to those "
            "Degrees Of Freedom.\nClick to select these unconstrained components.\nNote: Currently "
            "this selects only unconnected parts, not constrained parts that still have free "
            "DoF."
        ));
    }
    else if (link == QStringLiteral("#malformed")) {
        setLinkTooltip(tr("Click to select these malformed joints."));
    }
}

void TaskAssemblyMessages::onLabelStatusLinkClicked(const QString& str)
{
    if (str == QStringLiteral("#conflicting")) {
        Gui::Application::Instance->commandManager().runCommandByName(
            "Assembly_SelectConflictingConstraints"
        );
    }
    else if (str == QStringLiteral("#redundant")) {
        Gui::Application::Instance->commandManager().runCommandByName(
            "Assembly_SelectRedundantConstraints"
        );
    }
    else if (str == QStringLiteral("#dofs")) {
        Gui::Application::Instance->commandManager().runCommandByName(
            "Assembly_SelectComponentsWithDoFs"
        );
    }
    else if (str == QStringLiteral("#malformed")) {
        Gui::Application::Instance->commandManager().runCommandByName(
            "Assembly_SelectMalformedConstraints"
        );
    }
}

#include "moc_TaskAssemblyMessages.cpp"
