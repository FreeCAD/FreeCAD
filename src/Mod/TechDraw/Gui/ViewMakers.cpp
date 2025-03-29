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

// TODO: wf: we should split makexxxxSelection and makexxxx into separate files so
//       makexxx can be used on the App side without the Gui.

//! ViewMakers is a collection of methods for creating views on a page and is
//! phase 1 of a refactoring effort to reduce duplicated code in commands and task
//! dialogs.


#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <QMessageBox>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Tools.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>

#include <Mod/Spreadsheet/App/Sheet.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSpreadsheet.h>
#include <Mod/TechDraw/App/DrawViewClip.h>
#include <Mod/TechDraw/App/DrawViewDraft.h>
#include <Mod/TechDraw/App/DrawViewArch.h>
#include <Mod/TechDraw/App/DrawViewImage.h>
#include <Mod/TechDraw/App/DrawBrokenView.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "PreferencesGui.h"
#include "DrawGuiUtil.h"
#include "CommandHelpers.h"
#include "ViewMakers.h"

using namespace TechDraw;
using namespace TechDrawGui;
using DU = DrawUtil;

// TODO: wf: need to establish where selection validation is done and
//       where the projection direction is set.

//! returns a DrawViewPart based on the current selection.
DrawViewPart* ViewMakers::makeShapeViewSelection(DrawPage* page)
{
    std::vector<App::DocumentObject*> shapeObjs;
    std::vector<App::DocumentObject*> xShapeObjs;
    CommandHelpers::getShapeObjectsFromSelection(shapeObjs, xShapeObjs);
    return makeShapeView(page, shapeObjs, xShapeObjs);
}


//! returns a DrawViewPart based on the passed objects and links.
DrawViewPart* ViewMakers::makeShapeView(DrawPage* page,
                                        const std::vector<App::DocumentObject *> &shapeObjs,
                                        const std::vector<App::DocumentObject *> &xShapeObjs)
{
    std::string FeatName = page->getDocument()->getUniqueObjectName("View");
    std::string PageName = page->getNameInDocument();

    //NOLINTBEGIN
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('TechDraw::DrawProjGroupItem', '%s')",
                   FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.translateLabel('DrawProjGroupItem', 'View', '%s')",
                   FeatName.c_str(), FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", PageName.c_str(),
                   FeatName.c_str());
    //NOLINTEND

    App::DocumentObject* docObj = page->getDocument()->getObject(FeatName.c_str());
    auto* dvp = dynamic_cast<TechDraw::DrawViewPart*>(docObj);
    if (!dvp) {
        throw Base::TypeError("CmdTechDrawView DVP not found\n");
    }
    dvp->Source.setValues(shapeObjs);
    dvp->XSource.setValues(xShapeObjs);

//    cmd->getDocument()->setStatus(App::Document::Status::SkipRecompute, true);

    //NOLINTBEGIN
    auto dirs = CommandHelpers::viewDirection();
    Gui::Command::doCommand(Gui::Command::Doc,
                   "App.activeDocument().%s.Direction = FreeCAD.Vector(%.12f, %.12f, %.12f)",
                   FeatName.c_str(), dirs.first.x, dirs.first.y, dirs.first.z);
    Gui::Command::doCommand(Gui::Command::Doc,
                   "App.activeDocument().%s.RotationVector = FreeCAD.Vector(%.12f, %.12f, %.12f)",
                   FeatName.c_str(), dirs.second.x, dirs.second.y, dirs.second.z);
    Gui::Command::doCommand(Gui::Command::Doc,
                   "App.activeDocument().%s.XDirection = FreeCAD.Vector(%.12f, %.12f, %.12f)",
                   FeatName.c_str(), dirs.second.x, dirs.second.y, dirs.second.z);
    //NOLINTEND

//    cmd->getDocument()->setStatus(App::Document::Status::SkipRecompute, false);

    return dvp;
}

//! returns a DrawProjGroup based on the current selection.
DrawProjGroup* ViewMakers::makeProjectionGroupSelection(DrawPage* page)
{
    std::vector<App::DocumentObject*> shapeObjs;
    std::vector<App::DocumentObject*> xShapeObjs;
    CommandHelpers::getShapeObjectsFromSelection(shapeObjs, xShapeObjs);
    auto directions = CommandHelpers::viewDirection();
    return makeProjectionGroup(page, shapeObjs, xShapeObjs, directions);
}

//! returns a DrawProjGroup based on parameters.
DrawProjGroup* ViewMakers::makeProjectionGroup(DrawPage* page,
                                          const std::vector<App::DocumentObject*>& shapes,
                                          const std::vector<App::DocumentObject*>& xShapes,
                                          const std::pair<Base::Vector3d, Base::Vector3d>& directions)
{
    std::string PageName = page->getNameInDocument();
    std::string multiViewName = page->getDocument()->getUniqueObjectName("ProjGroup");
    //NOLINTBEGIN
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('TechDraw::DrawProjGroup', '%s')",
              multiViewName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", PageName.c_str(),
              multiViewName.c_str());
    //NOLINTEND

    App::DocumentObject* docObj = page->getDocument()->getObject(multiViewName.c_str());
    auto multiView(static_cast<TechDraw::DrawProjGroup*>(docObj));
    multiView->Source.setValues(shapes);
    multiView->XSource.setValues(xShapes);

    //NOLINTBEGIN
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addProjection('Front')", multiViewName.c_str());

    Gui::Command::doCommand(Gui::Command::Doc,
              "App.activeDocument().%s.Anchor.Direction = FreeCAD.Vector(%.12f, %.12f, %.12f)",
              multiViewName.c_str(), directions.first.x, directions.first.y, directions.first.z);
    Gui::Command::doCommand(Gui::Command::Doc,
              "App.activeDocument().%s.Anchor.RotationVector = FreeCAD.Vector(%.12f, %.12f, %.12f)",
              multiViewName.c_str(), directions.second.x, directions.second.y, directions.second.z);
    Gui::Command::doCommand(Gui::Command::Doc,
              "App.activeDocument().%s.Anchor.XDirection = FreeCAD.Vector(%.12f, %.12f, %.12f)",
              multiViewName.c_str(), directions.second.x, directions.second.y, directions.second.z);

    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Anchor.recompute()", multiViewName.c_str());
    //NOLINTEND
    return multiView;
}



DrawViewSpreadsheet*  ViewMakers::makeSpreadsheetView(DrawPage* page)
{
    std::string PageName = page->getNameInDocument();

    auto spreads = Gui::Selection().getObjectsOfType(Spreadsheet::Sheet::getClassTypeId());
    if (spreads.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("Select exactly one Spreadsheet object."));
        return {};
    }
    std::string SpreadName = spreads.front()->getNameInDocument();
    // removefromselection(spreads.front()); ??

    //NOLINTBEGIN
    std::string FeatName = page->getDocument()->getUniqueObjectName("Sheet");
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('TechDraw::DrawViewSpreadsheet', '%s')",
              FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.translateLabel('DrawViewSpreadsheet', 'Sheet', '%s')",
              FeatName.c_str(), FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Source = App.activeDocument().%s", FeatName.c_str(),
              SpreadName.c_str());
    //NOLINTEND

    // look for an owner view in the selection
    auto thisView = page->getDocument()->getObject(FeatName.c_str() );
    auto baseView = CommandHelpers::firstViewInSelection(thisView);
    if (baseView) {
        auto baseName = baseView->getNameInDocument();
        //NOLINTNEXTLINE
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Owner = App.activeDocument().%s",
                  FeatName.c_str(), baseName);
    }

    //NOLINTNEXTLINE
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", PageName.c_str(),
              FeatName.c_str());

    auto* newobj = page->getDocument()->getObject(FeatName.c_str());
    return static_cast<DrawViewSpreadsheet*>(newobj);
}


DrawViewSymbol* ViewMakers::makeSymbolView(DrawPage* page, const QString& filename)
{
    std::string PageName = page->getNameInDocument();
    std::string FeatName = page->getDocument()->getUniqueObjectName("Symbol");
    auto localfname = Base::Tools::escapeEncodeFilename(filename);
    auto filespec = DU::cleanFilespecBackslash(localfname.toStdString());

    //NOLINTBegin
    Gui::Command::doCommand(Gui::Command::Doc, "import codecs");
    Gui::Command::doCommand(Gui::Command::Doc,
                   "f = codecs.open(\"%s\", 'r', encoding=\"utf-8\")",
                   filespec.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "svg = f.read()");
    Gui::Command::doCommand(Gui::Command::Doc, "f.close()");
    Gui::Command::doCommand(Gui::Command::Doc,
                   "App.activeDocument().addObject('TechDraw::DrawViewSymbol', '%s')",
                   FeatName.c_str());
    Gui::Command::doCommand(
        Gui::Command::Doc,
        "App.activeDocument().%s.translateLabel('DrawViewSymbol', 'Symbol', '%s')",
        FeatName.c_str(),
        FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Symbol = svg", FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,
                   "App.activeDocument().%s.addView(App.activeDocument().%s)",
                   PageName.c_str(),
                   FeatName.c_str());
    //NOLINTEND

    auto thisView = page->getDocument()->getObject(FeatName.c_str() );
    auto baseView = CommandHelpers::firstViewInSelection(thisView);
    if (baseView) {
        auto baseName = baseView->getNameInDocument();
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Owner = App.activeDocument().%s",
                  FeatName.c_str(), baseName);
    }

    //NOLINTNEXTLINE
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", PageName.c_str(),
              FeatName.c_str());

    auto* newobj = page->getDocument()->getObject(FeatName.c_str());
    return static_cast<DrawViewSymbol*>(newobj);
}


DrawViewImage*  ViewMakers:: makeImageView(TechDraw::DrawPage* page, const QString& filename)
{
    std::string PageName = page->getNameInDocument();
    std::string FeatName = page->getDocument()->getUniqueObjectName("Image");
    auto localfname = Base::Tools::escapeEncodeFilename(filename);
    auto filespec = DU::cleanFilespecBackslash(localfname.toStdString());

    //NOLINTBEGIN
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('TechDraw::DrawViewImage', '%s')", FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.translateLabel('DrawViewImage', 'Image', '%s')",
                   FeatName.c_str(), FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.ImageFile = '%s'", FeatName.c_str(), filespec.c_str());
    //NOLINTEND

    auto thisView = page->getDocument()->getObject(FeatName.c_str() );
    auto baseView = CommandHelpers::firstViewInSelection(thisView);
    if (baseView) {
        auto baseName = baseView->getNameInDocument();
        //NOLINTNEXTLINE
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Owner = App.activeDocument().%s",
                       FeatName.c_str(), baseName);
    }

    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", PageName.c_str(), FeatName.c_str());    //NNOLINT

    auto* newobj = page->getDocument()->getObject(FeatName.c_str());
    return static_cast<DrawViewImage*>(newobj);
}


DrawViewDraft*  ViewMakers::makeDraftView(TechDraw::DrawPage* page)
{
    std::string PageName = page->getNameInDocument();

    auto dirs = CommandHelpers::viewDirection();
    auto objects = Gui::Selection().getObjectsOfType(App::DocumentObject::getClassTypeId());
    if (objects.empty()) {
        return {};
    }

    std::vector<std::string> objectAddedNames;
    for (auto* obj : objects) {
        if (obj->isDerivedFrom(TechDraw::DrawPage::getClassTypeId()) ||
            obj->isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
            // skip over TechDraw objects as they are not valid subjects for a DraftView
            continue;
        }
        std::string FeatName = page->getDocument()->getUniqueObjectName("DraftView");
        objectAddedNames.push_back(FeatName);
        std::string SourceName = obj->getNameInDocument();

        //NOLINTBEGIN
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('TechDraw::DrawViewDraft', '%s')",
                       FeatName.c_str());
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.translateLabel('DrawViewDraft', 'DraftView', '%s')",
                       FeatName.c_str(), FeatName.c_str());
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Source = App.activeDocument().%s", FeatName.c_str(),
                       SourceName.c_str());
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", PageName.c_str(),
                       FeatName.c_str());
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Direction = FreeCAD.Vector(%.12f, %.12f, %.12f)",
                       FeatName.c_str(), dirs.first.x, dirs.first.y, dirs.first.z);
        //NOLINTEND
    }

    auto* newobj = page->getDocument()->getObject(objectAddedNames.front().c_str());
    return static_cast<DrawViewDraft*>(newobj);
}


DrawViewArch*  ViewMakers::makeBIMView(TechDraw::DrawPage* page)
{
    auto objects = Gui::Selection().getObjectsOfType(App::DocumentObject::getClassTypeId());
    App::DocumentObject* archObject = nullptr;
    for (auto& obj : objects) {
        if (DrawGuiUtil::isArchSection(obj)) {
            archObject = obj;
            break;
        }
    }

            // this check is probably superfluous since the caller already checked
    if (!archObject) {
        return {};
    }

    std::string PageName = page->getNameInDocument();
    std::string FeatName = page->getDocument()->getUniqueObjectName("ArchView");
    std::string SourceName = archObject->getNameInDocument();

    //NOLINTBEGIN
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('TechDraw::DrawViewArch', '%s')",
                   FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.translateLabel('DrawViewArch', 'ArchView', '%s')",
                   FeatName.c_str(), FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Source = App.activeDocument().%s", FeatName.c_str(),
                   SourceName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", PageName.c_str(),
                   FeatName.c_str());
    //NOLINTEND

    auto* newobj = page->getDocument()->getObject(FeatName.c_str());
    return static_cast<DrawViewArch*>(newobj);
}


DrawViewClip*  ViewMakers::makeClipGroup(TechDraw::DrawPage* page)
{
    std::string PageName = page->getNameInDocument();
    std::string FeatName = page->getDocument()->getUniqueObjectName("Clip");

    //NOLINTBEGIN
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('TechDraw::DrawViewClip', '%s')",
                   FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", PageName.c_str(),
                   FeatName.c_str());
    //NOLINTEND

    auto* newobj = page->getDocument()->getObject(FeatName.c_str());
    return static_cast<DrawViewClip*>(newobj);
}


void  ViewMakers::addViewToClipGroup(TechDraw::DrawViewClip* clip, TechDraw::DrawView* view)
{
    std::string ClipName = clip->getNameInDocument();
    std::string ViewName = view->getNameInDocument();

    //NOLINTBEGIN
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.ViewObject.Visibility = False", ViewName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", ClipName.c_str(),
                   ViewName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.ViewObject.Visibility = True", ViewName.c_str());
    //NOLINTEND
}


void  ViewMakers::removeViewFromClipGroup(TechDraw::DrawViewClip* clip, TechDraw::DrawView* view)
{
    std::string ClipName = clip->getNameInDocument();
    std::string ViewName = view->getNameInDocument();

    //NOLINTBEGIN
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.ViewObject.Visibility = False", ViewName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.removeView(App.activeDocument().%s)", ClipName.c_str(),
                   ViewName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.ViewObject.Visibility = True", ViewName.c_str());
    //NOLINTEND
}

DrawBrokenView* ViewMakers::makeBrokenViewSelection(DrawPage* page)
{
    std::vector<App::DocumentObject*> shapes;
    std::vector<App::DocumentObject*> xShapes;
    auto* baseView = CommandHelpers::findBaseViewInSelection();
    if (baseView) {
        CommandHelpers::getShapeObjectsFromBase(*baseView, shapes, xShapes);
    } else {
        CommandHelpers::getShapeObjectsFromSelection(shapes, xShapes);
    }

            // we need either a base view (baseView) or some shape objects in the selection
    if (!baseView && (shapes.empty() && xShapes.empty())) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Empty selection"),
                             QObject::tr("Please select objects to break or a base view and break definition objects."));
        return nullptr;
    }

    std::vector<App::DocumentObject*> breakObjects;
    CommandHelpers::findBreakObjectsInSelection(breakObjects);

    if (breakObjects.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("No Break objects found in this selection"));
        return nullptr;
    }


            // remove Break objects from shape pile
    shapes = DrawBrokenView::removeBreakObjects(breakObjects, shapes);
    xShapes = DrawBrokenView::removeBreakObjects(breakObjects, xShapes);
    if (shapes.empty() &&
        xShapes.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("No Shapes, Groups or Links in this selection"));
        return nullptr;
    }

    return makeBrokenView(page, shapes, xShapes, breakObjects);
}


DrawBrokenView* ViewMakers::makeBrokenView(DrawPage* page,
                                           const std::vector<App::DocumentObject*>& shapes,
                                           const std::vector<App::DocumentObject*>& xShapes,
                                           const std::vector<App::DocumentObject *> &breakObjects)
{
    std::string PageName = page->getNameInDocument();

    std::string FeatName = page->getDocument()->getUniqueObjectName("BrokenView");
    //NOLINTBEGIN
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('TechDraw::DrawBrokenView','%s')", FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", PageName.c_str(), FeatName.c_str());
    //NOLINTEND
    App::DocumentObject* docObj = page->getDocument()->getObject(FeatName.c_str());
    auto* dbv = dynamic_cast<TechDraw::DrawBrokenView*>(docObj);
    if (!dbv) {
        throw Base::TypeError("CmdTechDrawBrokenView DBV not found\n");
    }
    dbv->Source.setValues(shapes);
    dbv->XSource.setValues(xShapes);
    dbv->Breaks.setValues(breakObjects);

    auto dirs = CommandHelpers::viewDirection();

    //NOLINTBEGIN
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Direction = FreeCAD.Vector(%.9f,%.9f,%.9f)",
                   FeatName.c_str(), dirs.first.x, dirs.first.y, dirs.first.z);
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.XDirection = FreeCAD.Vector(%.9f,%.9f,%.9f)",
                   FeatName.c_str(), dirs.second.x, dirs.second.y, dirs.second.z);

    Gui::Command::doCommand(Gui::Command::Doc, " ");
    //NOLINTEND
    return dbv;
}
