// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 The FreeCAD Project Association AISBL               *
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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QCoreApplication>
#include <QLayout>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Gui/Command.h>
#include <Gui/DockWindowManager.h>
#include <Gui/MDIView.h>
#include <Gui/MainWindow.h>
#include <Gui/ToolBarManager.h>

#include "Workbench.h"
#include "StartView.h"

#include <3rdParty/GSL/include/gsl/pointers>

using namespace StartGui;

TYPESYSTEM_SOURCE(StartGui::Workbench, Gui::StdWorkbench)  // NOLINT

void StartGui::Workbench::activated()
{
    loadStart();
}

void StartGui::Workbench::loadStart()
{
    auto mw = Gui::getMainWindow();
    auto doc = Gui::Application::Instance->activeDocument();
    auto existingView = mw->findChild<StartView*>(QLatin1String("StartView"));
    if (!existingView) {
        existingView = gsl::owner<StartView*>(new StartView(doc, mw));
        mw->addWindow(existingView);  // Transfers ownership
    }
    Gui::getMainWindow()->setActiveWindow(existingView);
    existingView->show();
}

Gui::MenuItem* StartGui::Workbench::setupMenuBar() const
{
    return Gui::StdWorkbench::setupMenuBar();
}

Gui::ToolBarItem* StartGui::Workbench::setupToolBars() const
{
    return Gui::StdWorkbench::setupToolBars();
}

Gui::ToolBarItem* StartGui::Workbench::setupCommandBars() const
{
    return Gui::StdWorkbench::setupCommandBars();
}

Gui::DockWindowItems* StartGui::Workbench::setupDockWindows() const
{
    Gui::DockWindowItems* root = Gui::StdWorkbench::setupDockWindows();
    root->setVisibility(false);                  // hide all dock windows by default
    root->setVisibility("Std_ComboView", true);  // except of the combo view
    root->setVisibility("Std_TaskView", true);   // and the task view
    return root;
}
