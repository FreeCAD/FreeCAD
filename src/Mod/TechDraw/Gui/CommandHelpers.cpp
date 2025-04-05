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

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Link.h>

#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSpreadsheet.h>
#include <Mod/TechDraw/App/DrawComplexSection.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Preferences.h>

#include <Mod/TechDraw/Gui/PreferencesGui.h>
#include <Mod/TechDraw/Gui/DrawGuiUtil.h>


#include "CommandHelpers.h"

using namespace TechDraw;
using namespace TechDrawGui;

//! find the first DrawView in the current selection for use as a base view (owner).  notThis parameter
//! is used to prevent selecting oneself.
TechDraw::DrawView* CommandHelpers::firstViewInSelection(App::DocumentObject* notThis)
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    TechDraw::DrawView* baseView{nullptr};
    if (!selection.empty()) {
        for (auto& selobj : selection) {
            if (selobj.getObject()->isDerivedFrom<DrawView>()) {
                auto docobj = selobj.getObject();
                if (docobj == notThis) {
                    continue;
                }
                baseView =  dynamic_cast<TechDraw::DrawView *>(docobj);
                break;
            }
        }
    }
    return baseView;
}


std::vector<std::string> CommandHelpers::getSelectedSubElements(TechDraw::DrawViewPart* &dvp,
                                                 const std::string& subType)
{
    std::vector<std::string> selectedSubs;
    std::vector<std::string> subNames;
    dvp = nullptr;
    auto selection = Gui::Selection().getSelectionEx();
    auto itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom<TechDraw::DrawViewPart>()) {
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

//! extract the selected shapes and xShapes
void CommandHelpers::getShapeObjectsFromSelection(std::vector<App::DocumentObject*>& shapes,
                                                  std::vector<App::DocumentObject*>& xShapes, App::DocumentObject *notThis)
{
    auto resolve = Gui::ResolveMode::OldStyleElement;
    bool single = false;
    auto selection = Gui::Selection().getSelectionEx(nullptr, App::DocumentObject::getClassTypeId(),
                                                        resolve, single);
    if (selection.empty()) {
        return;
    }

    auto* activeDoc{App::GetApplication().getActiveDocument()};
    for (auto& sel : selection) {
        bool is_linked = false;
        auto obj = sel.getObject();

        if (notThis && obj == notThis) {
            continue;
        }

        if (obj->isDerivedFrom<TechDraw::DrawPage>()) {
            continue;
        }
        if (obj->isDerivedFrom<App::LinkElement>()
            || obj->isDerivedFrom<App::LinkGroup>()
            || obj->isDerivedFrom<App::Link>()) {
            is_linked = true;
        }
        // If parent of the obj is a link to another document, we possibly need to treat non-link obj as linked, too
        // 1st, is obj in another document?
        if (obj->getDocument() != activeDoc) {
            std::set<App::DocumentObject*> parents = obj->getInListEx(true);
            for (auto& parent : parents) {
                // Only consider parents in the current document, i.e. possible links in this View's document
                if (parent->getDocument() != activeDoc) {
                    continue;
                }
                // 2nd, do we really have a link to obj?
                if (parent->isDerivedFrom<App::LinkElement>()
                    || parent->isDerivedFrom<App::LinkGroup>()
                    || parent->isDerivedFrom<App::Link>()) {
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


DrawViewPart* CommandHelpers::findBaseViewInSelection(App::DocumentObject *notThis)
{
    std::vector<App::DocumentObject*> baseCandidates =
        Gui::Command::getSelection().getObjectsOfType(TechDraw::DrawViewPart::getClassTypeId());
    App::DocumentObject* baseView{nullptr};
    for (auto& candidate : baseCandidates) {
        if (candidate == notThis) {
            continue;
        }
        baseView = candidate;
        break;
    }
    return static_cast<DrawViewPart*>(baseView);
}


void CommandHelpers::getShapeObjectsFromBase(const DrawViewPart& baseView,
                                             std::vector<App::DocumentObject*>& shapesFromBase,
                                             std::vector<App::DocumentObject*>& xShapesFromBase)
{
    shapesFromBase = baseView.Source.getValues();
    xShapesFromBase = baseView.XSource.getValues();
}


void CommandHelpers::findBreakObjectsInSelection(std::vector<App::DocumentObject*>& breakObjects)
{
    std::vector<Gui::SelectionObject> selection = Gui::Command::getSelection().getSelectionEx(
        nullptr, App::DocumentObject::getClassTypeId(), Gui::ResolveMode::NoResolve);

    if (selection.empty()) {
        return;
    }

    std::vector<App::DocumentObject*> candidates;
    for (auto& selObj : selection) {
        auto temp = selObj.getObject();
        // a sketch outside a body is returned as an independent object in the selection
        if (selObj.getSubNames().empty()) {
            candidates.push_back(temp);
            continue;
        }

        // a sketch inside a body is returned as body + subelement, so we have to search through
        // subnames to find it.  This may(?) apply to App::Part and Group also?
        auto subname = selObj.getSubNames().front();
        if (subname.back() == '.') {
            subname = subname.substr(0, subname.length() - 1);
            auto objects = selObj.getObject()->getDocument()->getObjects();
            for (auto& obj : objects) {
                std::string objname{obj->getNameInDocument()};
                if (subname == objname) {
                    candidates.push_back(obj);
                }
            }
        }
    }

    DrawBrokenView::findBreakObjectsInShapePile(candidates, breakObjects);
}


void CommandHelpers::findProfileObjectsInSelection(App::DocumentObject* &profileObject,
                                                   std::vector<std::string>& profileSubs)
{
    std::vector<Gui::SelectionObject> selection = Gui::Command::getSelection().getSelectionEx(
        nullptr, App::DocumentObject::getClassTypeId(), Gui::ResolveMode::NoResolve);

    if (selection.empty()) {
        return;
    }

    auto* baseView = findBaseViewInSelection();
    if (baseView) {
        auto profileSubs = getSelectedSubElements(baseView);
    }

    std::vector<App::DocumentObject*> candidates;
    for (auto& selObj : selection) {
        auto obj = selObj.getObject();

        if (obj->isDerivedFrom<TechDraw::DrawViewPart>()) {
            //use the dvp's Sources as sources for this ComplexSection &
            //check the subelement(s) to see if they can be used as a profile
            baseView = static_cast<TechDraw::DrawViewPart*>(obj);
            if (!selObj.getSubNames().empty()) {
                //need to add profile subs as parameter
                profileObject = baseView;
                auto temp = selObj.getSubNames();
                profileSubs = temp;
                return;
            }
        }

        if (TechDraw::DrawComplexSection::isProfileObject(obj)) {
            profileObject = obj;
        }
    }
}

//! true if common requirements for commands are satisfied.  Many new views (section, detail, etc)
//! require that there be a page, a base view and that no other task dialog is active.
bool CommandHelpers::isActiveCommon(Gui::Command* cmd)
{
    return DrawGuiUtil::needPage(cmd) &&
           DrawGuiUtil::needView(cmd) &&
           (Gui::Control().activeDialog() == nullptr);
}


//! true if the caller is free to start a new task dialog
bool CommandHelpers::guardActiveDialog()
{
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return false;
    }
    return true;
}
