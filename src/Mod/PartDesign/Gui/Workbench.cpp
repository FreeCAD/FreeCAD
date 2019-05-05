/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <boost/bind.hpp>
# include <QMessageBox>
#endif

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/MDIView.h>
#include <Gui/MenuManager.h>

#include <Mod/Sketcher/Gui/Workbench.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/PartDesign/App/FeatureMultiTransform.h>

#include "Utils.h"

#include "Workbench.h"

#include "WorkflowManager.h"

using namespace PartDesignGui;

#if 0 // needed for Qt's lupdate utility
    qApp->translate("Workbench", "Part Design");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Face tools");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Sketch tools");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Create Geometry");
#endif

/// @namespace PartDesignGui @class Workbench
TYPESYSTEM_SOURCE(PartDesignGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench() {
}

Workbench::~Workbench() {
    WorkflowManager::destruct();
}

void Workbench::_switchToDocument(const App::Document* /*doc*/)
{
// TODO Commented out for thurther remove or rewrite  (2015-09-04, Fat-Zer)
//    if (doc == NULL) return;
//
//    PartDesign::Body* activeBody = NULL;
//    std::vector<App::DocumentObject*> bodies = doc->getObjectsOfType(PartDesign::Body::getClassTypeId());
//
//    // No tip, so build up structure or migrate
//    if (!doc->Tip.getValue())
//    {
//        ;/*if (doc->countObjects() == 0){
//            buildDefaultPartAndBody(doc);
//            activeBody = Gui::Application::Instance->activeView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);
//            assert(activeBody);
//        } else {
//            // empty document with no tip, so do migration
//            _doMigration(doc);
//                        activeBody = Gui::Application::Instance->activeView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);
//                        assert(activeBody);
//                }
//                */
//    }
//    else
//    {
//      App::Part *docPart = dynamic_cast<App::Part *>(doc->Tip.getValue());
//      if (docPart) {
//          App::Part *viewPart = Gui::Application::Instance->activeView()->getActiveObject<App::Part *>("Part");
//          if (viewPart != docPart)
//            Gui::Application::Instance->activeView()->setActiveObject(docPart, "Part");
//          //if (docPart->countObjectsOfType(PartDesign::Body::getClassTypeId()) < 1)
//          //  setUpPart(docPart);
//          PartDesign::Body *tempBody = dynamic_cast<PartDesign::Body *> (docPart->getObjectsOfType(PartDesign::Body::getClassTypeId()).front());
//          if (tempBody) {
//              PartDesign::Body *viewBody = Gui::Application::Instance->activeView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);
//              activeBody = viewBody;
//              if (!viewBody)
//                activeBody = tempBody;
//              else if (!docPart->hasObject(viewBody))
//                activeBody = tempBody;
//
//              if (activeBody != viewBody)
//                Gui::Application::Instance->activeView()->setActiveObject(activeBody, PDBODYKEY);
//          }
//      }
//    }
//
//    /*if (activeBody == NULL) {
//        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Could not create body"),
//            QObject::tr("No body was found in this document, and none could be created. Please report this bug."
//                        "We recommend you do not use this document with the PartDesign workbench until the bug has been fixed."
//                        ));
//    }*/
}

void Workbench::slotActiveDocument(const Gui::Document& /*Doc*/)
{
//     _switchToDocument(Doc.getDocument());
}

void Workbench::slotNewDocument(const App::Document& /*Doc*/)
{
//     _switchToDocument(&Doc);
}

void Workbench::slotFinishRestoreDocument(const App::Document& /*Doc*/)
{
//     _switchToDocument(&Doc);
}

void Workbench::slotDeleteDocument(const App::Document&)
{
    //ActivePartObject = 0;
    //ActiveGuiDoc = 0;
    //ActiveAppDoc = 0;
    //ActiveVp = 0;
}
/*
  This does not work for Std_DuplicateSelection:
  Tree.cpp gives: "Cannot reparent unknown object", probably because the signalNewObject is emitted
  before the duplication of the object has been completely finished

void Workbench::slotNewObject(const App::DocumentObject& obj)
{
    if ((obj.getDocument() == ActiveAppDoc) && (ActivePartObject != NULL)) {
        // Add the new object to the active Body
        // Note: Will this break Undo? But how else can we catch Edit->Duplicate selection?
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)",
                                ActivePartObject->getNameInDocument(), obj.getNameInDocument());
    }
}
*/

void Workbench::setupContextMenu(const char* recipient, Gui::MenuItem* item) const
{
    auto selection = Gui::Selection().getSelection();
    // Add move Tip Command
    if ( selection.size () >= 1 ) {
        App::DocumentObject *feature = selection.front().pObject;
        PartDesign::Body *body = nullptr;

        // if PD workflow is not new-style then add a command to the context-menu
        bool assertModern = true;
        if (feature && !isModernWorkflow(feature->getDocument())) {
            assertModern = false;
            *item << "PartDesign_Migrate";
        }

        body = PartDesignGui::getBodyFor (feature, false, false, assertModern);
        // lote of assertion so feature should be marked as a tip
        if ( selection.size () == 1 && feature && (
            feature->isDerivedFrom ( PartDesign::Body::getClassTypeId () ) ||
            ( feature->isDerivedFrom ( PartDesign::Feature::getClassTypeId () ) && body ) ||
            ( feature->isDerivedFrom ( Part::Feature::getClassTypeId () ) && body &&
              body->BaseFeature.getValue() == feature )
        ) ) {
            *item << "PartDesign_MoveTip";
        }

        if (strcmp(recipient, "Tree") == 0) {

            Gui::MDIView *activeView = Gui::Application::Instance->activeView();

            if ( selection.size () > 0 && activeView ) {
                bool docHaveBodies = activeView->getAppDocument()->countObjectsOfType (
                                        PartDesign::Body::getClassTypeId () ) > 0;

                if ( docHaveBodies ) {
                    bool addMoveFeature = true;
                    bool addMoveFeatureInTree = (body != nullptr);
                    for (auto sel: selection) {
                        // if at least one selected feature cannot be moved to a body
                        // disable the entry
                        if ( addMoveFeature && !PartDesign::Body::isAllowed ( sel.pObject ) ) {
                            addMoveFeature = false;
                        }
                        // if all at least one selected feature doesn't belong to the same body
                        // disable the menu entry
                        if ( addMoveFeatureInTree && !body->hasObject ( sel.pObject ) ) {
                            addMoveFeatureInTree = false;
                        }

                        if ( !addMoveFeatureInTree && !addMoveFeature ) {
                            break;
                        }
                    }

                    if (addMoveFeature) {
                        *item   << "PartDesign_MoveFeature";
                    }

                    if (addMoveFeatureInTree) {
                        *item   << "PartDesign_MoveFeatureInTree";
                    }
                }
            }

            if (Gui::Selection().countObjectsOfType(PartDesign::Transformed::getClassTypeId()) -
                Gui::Selection().countObjectsOfType(PartDesign::MultiTransform::getClassTypeId()) == 1 )
                *item << "PartDesign_MultiTransform";

            if (Gui::Selection().countObjectsOfType(App::DocumentObject::getClassTypeId()) > 0) {
                *item << "Std_SetAppearance"
                      << "Std_RandomColor"
                      << "Std_Cut"
                      << "Std_Copy"
                      << "Std_Paste"
                      << "Separator"
                      << "Std_Delete";
            }
        }
    }
}

void Workbench::activated()
{
    Gui::Workbench::activated();

    WorkflowManager::init();

    std::vector<Gui::TaskView::TaskWatcher*> Watcher;

    const char* Vertex[] = {
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
        "PartDesign_CoordinateSystem",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Part::Feature SUBELEMENT Vertex COUNT 1..",
        Vertex,
        "Vertex tools",
        "Part_Box"
    ));

    const char* Edge[] = {
        "PartDesign_Fillet",
        "PartDesign_Chamfer",
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
        "PartDesign_CoordinateSystem",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Part::Feature SUBELEMENT Edge COUNT 1..",
        Edge,
        "Edge tools",
        "Part_Box"
    ));

    const char* Face[] = {
        "PartDesign_NewSketch",
        "PartDesign_Fillet",
        "PartDesign_Chamfer",
        "PartDesign_Draft",
        "PartDesign_Thickness",
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
        "PartDesign_CoordinateSystem",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Part::Feature SUBELEMENT Face COUNT 1",
        Face,
        "Face tools",
        "Part_Box"
    ));

    const char* Body[] = {
        "PartDesign_NewSketch",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Body COUNT 1",
        Body,
        "Start Body",
        "Part_Box"
    ));

    const char* Body2[] = {
        "PartDesign_Boolean",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Body COUNT 1..",
        Body2,
        "Start Boolean",
        "Part_Box"
    ));

    const char* Plane1[] = {
        "PartDesign_NewSketch",
        "PartDesign_Plane",
        "PartDesign_Line",
        "PartDesign_Point",
        "PartDesign_CoordinateSystem",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT App::Plane COUNT 1",
        Plane1,
        "Start Part",
        "Part_Box"
    ));
    const char* Plane2[] = {
        "PartDesign_NewSketch",
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
        "PartDesign_CoordinateSystem",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Plane COUNT 1",
        Plane2,
        "Start Part",
        "Part_Box"
    ));

    const char* Line[] = {
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Line COUNT 1",
        Line,
        "Start Part",
        "Part_Box"
    ));

    const char* Point[] = {
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
        "PartDesign_CoordinateSystem",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Point COUNT 1",
        Point,
        "Start Part",
        "Part_Box"
    ));

    const char* NoSel[] = {
        "PartDesign_Body",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommandsEmptySelection(
        NoSel,
        "Start Part",
        "Part_Box"
    ));

    const char* Faces[] = {
        "PartDesign_Fillet",
        "PartDesign_Chamfer",
        "PartDesign_Draft",
        "PartDesign_Thickness",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Part::Feature SUBELEMENT Face COUNT 2..",
        Faces,
        "Face tools",
        "Part_Box"
    ));

    const char* Sketch[] = {
        "PartDesign_NewSketch",
        "PartDesign_Pad",
        "PartDesign_Pocket",
        "PartDesign_Hole",
        "PartDesign_Revolution",
        "PartDesign_Groove",
        "PartDesign_AdditivePipe",
        "PartDesign_SubtractivePipe",
        "PartDesign_AdditiveLoft",
        "PartDesign_SubtractiveLoft",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Sketcher::SketchObject COUNT 1",
        Sketch,
        "Sketch tools",
        "Part_Box"
    ));

    const char* Transformed[] = {
        "PartDesign_Mirrored",
        "PartDesign_LinearPattern",
        "PartDesign_PolarPattern",
//        "PartDesign_Scaled",
        "PartDesign_MultiTransform",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::SketchBased",
        Transformed,
        "Transformation tools",
        "PartDesign_MultiTransform"
    ));

    // make the previously used active Body active again
    //PartDesignGui::ActivePartObject = NULL;
    _switchToDocument(App::GetApplication().getActiveDocument());

    addTaskWatcher(Watcher);
    Gui::Control().showTaskView();

    // Let us be notified when a document is activated, so that we can update the ActivePartObject
    Gui::Application::Instance->signalActiveDocument.connect(boost::bind(&Workbench::slotActiveDocument, this, _1));
    App::GetApplication().signalNewDocument.connect(boost::bind(&Workbench::slotNewDocument, this, _1));
    App::GetApplication().signalFinishRestoreDocument.connect(boost::bind(&Workbench::slotFinishRestoreDocument, this, _1));
    App::GetApplication().signalDeleteDocument.connect(boost::bind(&Workbench::slotDeleteDocument, this, _1));
    // Watch out for objects being added to the active document, so that we can add them to the body
    //App::GetApplication().signalNewObject.connect(boost::bind(&Workbench::slotNewObject, this, _1));
}

void Workbench::deactivated()
{
    // Let us be notified when a document is activated, so that we can update the ActivePartObject
    Gui::Application::Instance->signalActiveDocument.disconnect(boost::bind(&Workbench::slotActiveDocument, this, _1));
    App::GetApplication().signalNewDocument.disconnect(boost::bind(&Workbench::slotNewDocument, this, _1));
    App::GetApplication().signalFinishRestoreDocument.disconnect(boost::bind(&Workbench::slotFinishRestoreDocument, this, _1));
    App::GetApplication().signalDeleteDocument.disconnect(boost::bind(&Workbench::slotDeleteDocument, this, _1));
    //App::GetApplication().signalNewObject.disconnect(boost::bind(&Workbench::slotNewObject, this, _1));

    removeTaskWatcher();
    // reset the active Body
    Gui::Command::doCommand(Gui::Command::Doc,"import PartDesignGui");

    Gui::Workbench::deactivated();

}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");

    Gui::MenuItem* part = new Gui::MenuItem;
    root->insertItem(item, part);
    part->setCommand("&Part Design");
    *part << "PartDesign_Body"
          << "PartDesign_NewSketch"
          << "Sketcher_LeaveSketch"
          << "Sketcher_ViewSketch"
          << "Sketcher_MapSketch"
          << "Sketcher_ReorientSketch"
          << "Sketcher_ValidateSketch"
          << "Separator"
          << "PartDesign_Point"
          << "PartDesign_Line"
          << "PartDesign_Plane"
          << "PartDesign_CoordinateSystem"
          << "PartDesign_ShapeBinder"
          << "PartDesign_SubShapeBinder"
          << "PartDesign_Clone"
          << "Separator"
          << "PartDesign_Pad"
          << "PartDesign_Revolution"
          << "PartDesign_AdditiveLoft"
          << "PartDesign_AdditivePipe"
          << "PartDesign_CompPrimitiveAdditive"
          << "Separator"
          << "PartDesign_Pocket"
          << "PartDesign_Hole"
          << "PartDesign_Groove"
          << "PartDesign_SubtractiveLoft"
          << "PartDesign_SubtractivePipe"
          << "PartDesign_CompPrimitiveSubtractive"
          << "Separator"
          << "PartDesign_Mirrored"
          << "PartDesign_LinearPattern"
          << "PartDesign_PolarPattern"
//          << "PartDesign_Scaled"
          << "PartDesign_MultiTransform"
          << "Separator"
          << "PartDesign_Fillet"
          << "PartDesign_Chamfer"
          << "PartDesign_Draft"
          << "PartDesign_Thickness"
          << "Separator"
          << "PartDesign_Boolean"
          << "Separator"
          //<< "PartDesign_Hole"
          << "PartDesign_InvoluteGear"
          << "Separator"
          << "PartDesign_Migrate";

    // For 0.13 a couple of python packages like numpy, matplotlib and others
    // are not deployed with the installer on Windows. Thus, the WizardShaft is
    // not deployed either hence the check for the existence of the command.
    if (Gui::Application::Instance->commandManager().getCommandByName("PartDesign_InvoluteGear")) {
        *part << "PartDesign_InvoluteGear";
    }
    if (Gui::Application::Instance->commandManager().getCommandByName("PartDesign_WizardShaft")) {
        *part << "Separator" << "PartDesign_WizardShaft";
    }

    // Replace the "Duplicate selection" menu item with a replacement that is compatible with Body
    item = root->findItem("&Edit");
    Gui::MenuItem* dup = item->findItem("Std_DuplicateSelection");
    dup->setCommand("PartDesign_DuplicateSelection");

    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* part = new Gui::ToolBarItem(root);
    part->setCommand("Part Design Helper");
    *part << "PartDesign_Body"
          << "PartDesign_NewSketch"
          << "Sketcher_EditSketch"
          << "Sketcher_MapSketch"
          << "Separator"
          << "PartDesign_Point"
          << "PartDesign_Line"
          << "PartDesign_Plane"
          << "PartDesign_CoordinateSystem"
          << "PartDesign_ShapeBinder"
          << "PartDesign_SubShapeBinder"
          << "PartDesign_Clone";

    part = new Gui::ToolBarItem(root);
    part->setCommand("Part Design Modeling");
    *part << "PartDesign_Pad"
          << "PartDesign_Revolution"
          << "PartDesign_AdditiveLoft"
          << "PartDesign_AdditivePipe"
          << "PartDesign_CompPrimitiveAdditive"
          << "Separator"
          << "PartDesign_Pocket"
          << "PartDesign_Hole"
          << "PartDesign_Groove"
          << "PartDesign_SubtractiveLoft"
          << "PartDesign_SubtractivePipe"
          << "PartDesign_CompPrimitiveSubtractive"
          << "Separator"
          << "PartDesign_Mirrored"
          << "PartDesign_LinearPattern"
          << "PartDesign_PolarPattern"
//          << "PartDesign_Scaled"
          << "PartDesign_MultiTransform"
          << "Separator"
          << "PartDesign_Fillet"
          << "PartDesign_Chamfer"
          << "PartDesign_Draft"
          << "PartDesign_Thickness"
          << "Separator"
          << "PartDesign_Boolean";

    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    // Part tools
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    return root;
}

