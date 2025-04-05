// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 PaddleStroke <>                                    *
 *                 2025 wandererfan <wondererfan[at]gmail[dot]com>         *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

// this is the multi-purpose view inserter that was originally in Command.cpp
// CmdTechDrawView::activated

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QMessageBox>
#include <QCheckBox>
#endif// #ifndef _PreComp_

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Link.h>

#include <Base/Console.h>
#include <Base/Tools.h>

#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewArch.h>
#include <Mod/TechDraw/App/DrawViewDraft.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSymbol.h>
#include <Mod/TechDraw/App/Preferences.h>

#include <Mod/Spreadsheet/App/Sheet.h>

#include "SingleInsertTool.h"
#include "ViewProviderPage.h"
#include "ViewMakers.h"
#include "DrawGuiUtil.h"
#include "TaskProjGroup.h"

using namespace TechDrawGui;
using namespace TechDraw;


void  TechDrawGui::singleInsertTool(Gui::Command& cmd, TechDraw::DrawPage* page)
{
    bool viewCreated{false};
    // switch to the page if it's not current active window
    // wf: but why? there's no real reason to change the active tab and it prevents picking 3d geometry
    auto* vpp = dynamic_cast<ViewProviderPage*>
        (Gui::Application::Instance->getViewProvider(page));
    if (vpp) {
        vpp->show();
    }

    std::vector<App::DocumentObject*> shapes;
    std::vector<App::DocumentObject*> xShapes;

    //set projection direction from selected Face
    //use first object with a face selected
    // wf: if there is no face selection then the 3d view direction determines the direction
    App::DocumentObject* partObj = nullptr;
    std::string faceName;
    auto selection = Gui::Command::getSelection().getSelectionEx(nullptr, App::DocumentObject::getClassTypeId());
    for (auto& sel : selection) {
        bool is_linked = false;
        auto obj = sel.getObject();
        if (obj->isDerivedFrom<TechDraw::DrawPage>() || obj->isDerivedFrom<TechDraw::DrawView>()) {
            continue;
        }

        if (obj->isDerivedFrom<Spreadsheet::Sheet>()) {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create BIM View"));
            ViewMakers::makeSpreadsheetView(page);
            viewCreated = true;
            Gui::Command::commitCommand();
            continue;
        }

        if (DrawGuiUtil::isArchSection(obj)) {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create BIM View"));
            ViewMakers::makeBIMView(page);
            Gui::Command::commitCommand();
            viewCreated = true;
            continue;
        }

        if (obj->isDerivedFrom<App::LinkElement>()
            || obj->isDerivedFrom<App::LinkGroup>()
            || obj->isDerivedFrom<App::Link>()) {
            is_linked = true;
        }
        // If parent of the obj is a link to another document, we possibly need to treat non-link obj as linked, too
        // 1st, is obj in another document?
        if (obj->getDocument() != cmd.getDocument()) {
            std::set<App::DocumentObject*> parents = obj->getInListEx(true);
            for (auto& parent : parents) {
                // Only consider parents in the current document, i.e. possible links in this View's document
                if (parent->getDocument() != cmd.getDocument()) {
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
        if (partObj) {
            continue;
        }
        //don't know if this works for an XLink
        for (auto& sub : sel.getSubNames()) {
            if (TechDraw::DrawUtil::getGeomTypeFromName(sub) == "Face") {
                faceName = sub;
                //
                partObj = obj;
                break;
            }
        }
    }

    if (shapes.empty() &&
        xShapes.empty() &&
        !viewCreated) {
        if (Preferences::ShowInsertFileMessage()) {
            auto msgText = QObject::tr("If you want to insert a view from existing objects, please select them before invoking this tool. Without a selection, a file browser will open, to insert a SVG or image file.");
            QMessageBox msgBox;
            msgBox.setText(msgText);
            auto dontShowMsg = QObject::tr("Do not show this message again");
            QCheckBox dontShowCheckBox(dontShowMsg, &msgBox);
            msgBox.setCheckBox(&dontShowCheckBox);
            // QPushButton* okButton = msgBox.addButton(QMessageBox::Ok);

            msgBox.exec();
            if (msgBox.standardButton(msgBox.clickedButton()) == QMessageBox::Ok &&
                dontShowCheckBox.isChecked()) {
                Preferences::setShowInsertFileMessage(false);
            }
        }

        QString filename = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(),
            QObject::tr("Select a SVG or Image file to open"),
            Preferences::defaultSymbolDir(),
            QString::fromLatin1("%1 (*.svg *.svgz *.jpg *.jpeg *.png *.bmp);;%2 (*.*)")
            .arg(QObject::tr("SVG or Image files"), QObject::tr("All Files")));

        if (filename.isEmpty()) {
            return;
        }

        if (filename.endsWith(QString::fromLatin1(".svg"), Qt::CaseInsensitive)
            || filename.endsWith(QString::fromLatin1(".svgz"), Qt::CaseInsensitive)) {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Symbol"));
            ViewMakers::makeSymbolView(page, filename);
        }
        else {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Image"));
            ViewMakers::makeImageView(page, filename);
        }

        Gui::Command::updateActive();
        Gui::Command::commitCommand();
        return;
    }

    Gui::WaitCursor wc;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create view"));
    auto dvp = ViewMakers::makeShapeView(page, shapes, xShapes);
    Gui::Command::updateActive();
    Gui::Command::commitCommand();

    // create the rest of the desired views
    Gui::Control().showDialog(new TaskDlgProjGroup(dvp, true));
}

