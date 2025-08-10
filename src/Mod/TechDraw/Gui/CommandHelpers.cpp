// SPDX-License-Identifier: LGPL-2.0-or-later

/***************************************************************************
 *   Copyright (c) 2024 WandererFan <wandererfan@gmail.com>                *
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

//! CommandHelpers is a collection of methods for common actions in commands
#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <QMessageBox>

#include <App/DocumentObject.h>
#include <App/Link.h>

#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSpreadsheet.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Preferences.h>

#include <Mod/TechDraw/Gui/PreferencesGui.h>
#include <Mod/TechDraw/Gui/DrawGuiUtil.h>


#include "CommandHelpers.h"

using namespace TechDraw;
using namespace TechDrawGui;

//! find the first DrawView in the current selection for use as a base view (owner)
TechDraw::DrawView* CommandHelpers::firstViewInSelection(Gui::Command* cmd)
{
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    TechDraw::DrawView* baseView{nullptr};
    if (!selection.empty()) {
        for (auto& selobj : selection) {
            if (selobj.getObject()->isDerivedFrom<DrawView>()) {
                auto docobj = selobj.getObject();
                baseView =  static_cast<TechDraw::DrawView *>(docobj);
                break;
            }
        }
    }
    return baseView;
}

std::vector<std::string> CommandHelpers::getSelectedSubElements(Gui::Command* cmd,
                                                TechDraw::DrawViewPart* &dvp,
                                                std::string subType)
{
    std::vector<std::string> selectedSubs;
    std::vector<std::string> subNames;
    dvp = nullptr;
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom<TechDraw::DrawViewPart>()) {
            dvp = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            subNames = (*itSel).getSubNames();
            break;
        }
    }
    if (!dvp) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("No part view in selection"));
        return selectedSubs;
    }

    for (auto& s: subNames) {
        if (TechDraw::DrawUtil::getGeomTypeFromName(s) == subType) {
            selectedSubs.push_back(s);
        }
    }

    if (selectedSubs.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("No %1 in selection")
                                 .arg(QString::fromStdString(subType)));
        return selectedSubs;
    }

    return selectedSubs;
}


std::pair<App::DocumentObject*, std::string> CommandHelpers::faceFromSelection()
{
    auto selection = Gui::Selection().getSelectionEx(
        nullptr, App::DocumentObject::getClassTypeId(), Gui::ResolveMode::NoResolve);

    if (selection.empty()) {
        return { nullptr, "" };
    }

    for (auto& sel : selection) {
        for (auto& sub : sel.getSubNames()) {
            if (TechDraw::DrawUtil::getGeomTypeFromName(sub) == "Face") {
                return { sel.getObject(), sub };
            }
        }
    }

    return { nullptr, "" };
}
