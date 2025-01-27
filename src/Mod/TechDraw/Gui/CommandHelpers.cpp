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
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>

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
                baseView =  dynamic_cast<TechDraw::DrawView *>(docobj);
                break;
            }
        }
    }
    return baseView;
}

//! find the first DrawView in the current selection for use as a base view (owner)
TechDraw::DrawView* CommandHelpers::firstNonSpreadsheetInSelection(Gui::Command* cmd)
{
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    TechDraw::DrawView* baseView{nullptr};
    if (!selection.empty()) {
        for (auto& selobj : selection) {
            if (selobj.getObject()->isDerivedFrom<DrawViewSpreadsheet>()) {
                continue;
            } else {
                auto docobj = selobj.getObject();
                baseView =  dynamic_cast<TechDraw::DrawView *>(docobj);
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
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            dvp = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            subNames = (*itSel).getSubNames();
            break;
        }
    }
    if (!dvp) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("No Part View in Selection"));
        return selectedSubs;
    }

    for (auto& s: subNames) {
        if (TechDraw::DrawUtil::getGeomTypeFromName(s) == subType) {
            selectedSubs.push_back(s);
        }
    }

    if (selectedSubs.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong Selection"),
                             QObject::tr("No %1 in Selection")
                                 .arg(QString::fromStdString(subType)));
        return selectedSubs;
    }

    return selectedSubs;
}

//! extract the selected shapes and xShapes and determine if a face has been
//! selected to define the projection direction
void CommandHelpers::getSelectedShapes(Gui::Command* cmd,
                       std::vector<App::DocumentObject*>& shapes,
                       std::vector<App::DocumentObject*>& xShapes,
                       App::DocumentObject* faceObj,
                       std::string& faceName)
{
    Gui::ResolveMode resolve = Gui::ResolveMode::OldStyleElement;//mystery
    bool single = false;                                         //mystery
    auto selection = cmd->getSelection().getSelectionEx(nullptr, App::DocumentObject::getClassTypeId(),
                                                        resolve, single);
    for (auto& sel : selection) {
        bool is_linked = false;
        auto obj = sel.getObject();
        if (obj->isDerivedFrom(TechDraw::DrawPage::getClassTypeId())) {
            continue;
        }
        if (obj->isDerivedFrom(App::LinkElement::getClassTypeId())
            || obj->isDerivedFrom(App::LinkGroup::getClassTypeId())
            || obj->isDerivedFrom(App::Link::getClassTypeId())) {
            is_linked = true;
        }
        // If parent of the obj is a link to another document, we possibly need to treat non-link obj as linked, too
        // 1st, is obj in another document?
        if (obj->getDocument() != cmd->getDocument()) {
            std::set<App::DocumentObject*> parents = obj->getInListEx(true);
            for (auto& parent : parents) {
                // Only consider parents in the current document, i.e. possible links in this View's document
                if (parent->getDocument() != cmd->getDocument()) {
                    continue;
                }
                // 2nd, do we really have a link to obj?
                if (parent->isDerivedFrom(App::LinkElement::getClassTypeId())
                    || parent->isDerivedFrom(App::LinkGroup::getClassTypeId())
                    || parent->isDerivedFrom(App::Link::getClassTypeId())) {
                    // We have a link chain from this document to obj, and obj is in another document -> it is an XLink target
                    is_linked = true;
                }
            }
        }
        if (is_linked) {
            xShapes.push_back(obj);
            continue;
        }
        //not a Link and not null.  assume to be drawable.  Undrawables will be
        // skipped later.
        shapes.push_back(obj);
        if (faceObj) {
            continue;
        }
        //don't know if this works for an XLink
        for (auto& sub : sel.getSubNames()) {
            if (TechDraw::DrawUtil::getGeomTypeFromName(sub) == "Face") {
                faceName = sub;
                //
                faceObj = obj;
                break;
            }
        }
    }
}


std::pair<Base::Vector3d, Base::Vector3d> CommandHelpers::viewDirection()
{
    if (!Preferences::useCameraDirection()) {
        return { Base::Vector3d(0, -1, 0), Base::Vector3d(1, 0, 0) };
    }

    auto faceInfo = faceFromSelection();
    if (faceInfo.first) {
        return DrawGuiUtil::getProjDirFromFace(faceInfo.first, faceInfo.second);
    }

    return DrawGuiUtil::get3DDirAndRot();
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
