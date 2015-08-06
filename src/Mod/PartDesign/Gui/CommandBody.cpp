/***************************************************************************
 *  Copyright (C) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com      *
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
# include <QApplication>
# include <QMessageBox>
# include <QInputDialog>
#endif

#include <App/Part.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Gui/ActiveObjectList.h>
#include <Gui/MainWindow.h>

#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>

#include "Workbench.h"


//===========================================================================
// PartDesign_Part
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignPart);

CmdPartDesignPart::CmdPartDesignPart()
  : Command("PartDesign_Part")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Create part");
    sToolTipText  = QT_TR_NOOP("Create a new part and make it active");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "Tree_Annotation";
}

void CmdPartDesignPart::activated(int iMsg)
{
    openCommand("Add a part");
    std::string FeatName = getUniqueObjectName("Part");

    std::string PartName;
    PartName = getUniqueObjectName("Part");
    doCommand(Doc,"App.activeDocument().Tip = App.activeDocument().addObject('App::Part','%s')",PartName.c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.Label = '%s'", QObject::tr(PartName.c_str()).toStdString().c_str());
    PartDesignGui::Workbench::setUpPart(dynamic_cast<App::Part *>(getDocument()->getObject(PartName.c_str())));
    doCommand(Gui::Command::Gui, "Gui.activeView().setActiveObject('%s', App.activeDocument().%s)", PARTKEY, PartName.c_str());

    updateActive();
}

bool CmdPartDesignPart::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Body
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignBody);

CmdPartDesignBody::CmdPartDesignBody()
  : Command("PartDesign_Body")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Create body");
    sToolTipText  = QT_TR_NOOP("Create a new body and make it active");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Body_Create_New";
}

void CmdPartDesignBody::activated(int iMsg)
{
    std::vector<App::DocumentObject*> features =
        getSelection().getObjectsOfType(Part::Feature::getClassTypeId());
    App::DocumentObject* baseFeature = nullptr;

    if (!features.empty()) {
        if (features.size() == 1) {
            baseFeature = features[0];
            // TODO Check if the feature belongs to another body and may be some other sanity checks (2015-08-04, Fat-Zer)
        } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Body may be based no more than on one feature."));
            return;
        }
    }

    // first check if Part is already created:
    App::Part *actPart = getDocument()->Tip.getValue<App::Part *>();
    std::string PartName;

    if(!actPart){
        Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
        rcCmdMgr.runCommandByName("PartDesign_Part");
        actPart = getDocument()->Tip.getValue<App::Part *>();
    }

    openCommand("Add a body");
    std::string FeatName = getUniqueObjectName("Body");

    PartName = actPart->getNameInDocument();
    // add the Body feature itself, and make it active
    doCommand(Doc,"App.activeDocument().addObject('PartDesign::Body','%s')", FeatName.c_str());
    if (baseFeature) {
        doCommand(Doc,"App.activeDocument().%s.BaseFeature = App.activeDocument().%s",
                FeatName.c_str(), baseFeature->getNameInDocument());
    }
    addModule(Gui,"PartDesignGui"); // import the Gui module only once a session
    doCommand(Gui::Command::Gui, "Gui.activeView().setActiveObject('%s', App.activeDocument().%s)", PDBODYKEY, FeatName.c_str());

    // Make the "Create sketch" prompt appear in the task panel
    doCommand(Gui,"Gui.Selection.clearSelection()");
    doCommand(Gui,"Gui.Selection.addSelection(App.ActiveDocument.%s)", FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addObject(App.ActiveDocument.%s)",PartName.c_str(),FeatName.c_str());

    updateActive();
}

bool CmdPartDesignBody::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_MoveTip
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignMoveTip);

CmdPartDesignMoveTip::CmdPartDesignMoveTip()
  : Command("PartDesign_MoveTip")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Set tip");
    sToolTipText  = QT_TR_NOOP("Move the tip of the body");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_MoveTip";
}

void CmdPartDesignMoveTip::activated(int iMsg)
{
    std::vector<App::DocumentObject*> features = getSelection().getObjectsOfType(
            Part::Feature::getClassTypeId() );
    App::DocumentObject* selFeature;
    PartDesign::Body* body= nullptr;

    if ( features.size() == 1 ) {
        selFeature = features.front();
        if ( selFeature->getTypeId().isDerivedFrom ( PartDesign::Body::getClassTypeId() ) ) {
            body = static_cast<PartDesign::Body *> ( selFeature );
        } else {
            body = PartDesignGui::getBodyFor ( selFeature, /* messageIfNot =*/ false );
        }
    } else {
        selFeature = nullptr;
    }
    // TODO Refuse to set tip to nonsolid features (2015-08-05, Fat-Zer)

    if (!selFeature || !body ) {
        QMessageBox::warning (0, QObject::tr( "Selection error" ),
                QObject::tr( "Select exactly one PartDesign feature or a body." ) );
        return;
    } else if (!body) {
        QMessageBox::warning (0, QObject::tr( "Selection error" ),
                QObject::tr( "Couldn't determin a body for selected feature." ) );
        return;
    }

    openCommand("Move insert point to selected feature");
    App::DocumentObject* oldTip = body->Tip.getValue();
    if (oldTip) {
        doCommand(Gui, "Gui.activeDocument().hide(\"%s\")", oldTip->getNameInDocument() );
    }

    if (selFeature == body) {
        doCommand(Doc,"App.activeDocument().%s.Tip = None", body->getNameInDocument());
    } else {
        doCommand(Doc,"App.activeDocument().%s.Tip = App.activeDocument().%s",body->getNameInDocument(),
                selFeature->getNameInDocument());

        // Adjust visibility to show only the Tip feature
        doCommand(Gui,"Gui.activeDocument().show(\"%s\")", selFeature->getNameInDocument());
    }

    // TOOD: Hide all datum features after the Tip feature? But the user might have already hidden some and wants to see
    // others, so we would have to remember their state somehow
}

bool CmdPartDesignMoveTip::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_DuplicateSelection
//===========================================================================

DEF_STD_CMD_A(CmdPartDesignDuplicateSelection);

CmdPartDesignDuplicateSelection::CmdPartDesignDuplicateSelection()
  :Command("PartDesign_DuplicateSelection")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Duplicate selected object");
    sToolTipText    = QT_TR_NOOP("Duplicates the selected object and adds it to the active body");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "";
}

void CmdPartDesignDuplicateSelection::activated(int iMsg) {
    // TODO Review the function (2015-08-05, Fat-Zer)
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(/*messageIfNot = */true);
    if(!pcActiveBody) return;

    std::vector<App::DocumentObject*> features = getSelection().getObjectsOfType(Part::Feature::getClassTypeId());
    if (features.empty()) return;
    App::DocumentObject* selFeature = features.front();

    if (!pcActiveBody->hasFeature(selFeature)) {
        // NOTE: We assume all selected features will be in the same document
        // Switch to other body
        pcActiveBody = static_cast<PartDesign::Body*>(Part::BodyBase::findBodyOf(selFeature));
        if (pcActiveBody != NULL)
            Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeView().setActiveObject('%s', App.activeDocument().%s)",
                                    PDBODYKEY, pcActiveBody->getNameInDocument());
        else
            return;
    }

    std::vector<App::DocumentObject*> beforeFeatures = getDocument()->getObjects();

    openCommand("Duplicate a PartDesign object");
    doCommand(Doc,"FreeCADGui.runCommand('Std_DuplicateSelection')");

    // Find the features that were added
    std::vector<App::DocumentObject*> afterFeatures = getDocument()->getObjects();
    std::vector<App::DocumentObject*> newFeatures;
    std::sort(beforeFeatures.begin(), beforeFeatures.end());
    std::sort(afterFeatures.begin(), afterFeatures.end());
    std::set_difference(afterFeatures.begin(), afterFeatures.end(), beforeFeatures.begin(), beforeFeatures.end(),
                        std::back_inserter(newFeatures));

    for (std::vector<App::DocumentObject*>::const_iterator f = newFeatures.begin(); f != newFeatures.end(); f++) {
        if (PartDesign::Body::isAllowed(*f)) {
            doCommand(Doc,"App.activeDocument().%s.addFeature(App.activeDocument().%s)",
                      pcActiveBody->getNameInDocument(), (*f)->getNameInDocument());
            doCommand(Gui,"Gui.activeDocument().hide(\"%s\")", (*f)->getNameInDocument());
        }
    }

    // Adjust visibility of features
    doCommand(Gui,"Gui.activeDocument().show(\"%s\")", newFeatures.back()->getNameInDocument());
}

bool CmdPartDesignDuplicateSelection::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// PartDesign_MoveFeature
//===========================================================================

DEF_STD_CMD_A(CmdPartDesignMoveFeature);

CmdPartDesignMoveFeature::CmdPartDesignMoveFeature()
  :Command("PartDesign_MoveFeature")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Move object to other body");
    sToolTipText    = QT_TR_NOOP("Moves the selected object to another body");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "";
}

void CmdPartDesignMoveFeature::activated(int iMsg)
{
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(/*messageIfNot = */true);
    if(!pcActiveBody) return;

    std::vector<App::DocumentObject*> features = getSelection().getObjectsOfType(Part::Feature::getClassTypeId());
    if (features.empty()) return;

    // Create a list of all bodies in this part
    std::vector<App::DocumentObject*> bodies = getDocument()->getObjectsOfType(Part::BodyBase::getClassTypeId());

    // Ask user to select the target body
    bool ok;
    QStringList items;
    for (std::vector<App::DocumentObject*>::iterator it = bodies.begin(); it != bodies.end(); ++it)
        items.push_back(QString::fromUtf8((*it)->Label.getValue()));
    QString text = QInputDialog::getItem(Gui::getMainWindow(),
        qApp->translate(className(), "Select body"),
        qApp->translate(className(), "Select a body from the list"),
        items, 0, false, &ok);
    if (!ok) return;
    int index = items.indexOf(text);

    PartDesign::Body* target = static_cast<PartDesign::Body*>(bodies[index]);

    openCommand("Move an object");

    for (std::vector<App::DocumentObject*>::const_iterator f = features.begin(); f != features.end(); f++) {
        // Find body of this feature
        Part::BodyBase* source = PartDesign::Body::findBodyOf(*f);
        bool featureWasTip = false;

        if (source == target) continue;

        // Remove from the source body if the feature belonged to a body
        if (source) {
            featureWasTip = (source->Tip.getValue() == *f);
            doCommand(Doc,"App.activeDocument().%s.removeFeature(App.activeDocument().%s)",
                      source->getNameInDocument(), (*f)->getNameInDocument());
        }

        App::DocumentObject* targetOldTip = target->Tip.getValue();

        // Add to target body (always at the Tip)
        doCommand(Doc,"App.activeDocument().%s.addFeature(App.activeDocument().%s)",
                      target->getNameInDocument(), (*f)->getNameInDocument());
        // Recompute to update the shape
        doCommand(Gui,"App.activeDocument().recompute()");

        // Adjust visibility of features
        // TODO: May be something can be done in view provider (2015-08-05, Fat-Zer)
        // If we removed the tip of the source body, make the new tip visible
        if (featureWasTip ) {
            App::DocumentObject * sourceNewTip = source->Tip.getValue();
            doCommand(Gui,"Gui.activeDocument().show(\"%s\")", sourceNewTip->getNameInDocument());
        }

        // Hide old tip and show new tip (the moved feature) of the target body
        App::DocumentObject* targetNewTip = target->Tip.getValue();
        if ( targetOldTip != targetNewTip ) {
            if ( targetOldTip ) {
                doCommand(Gui,"Gui.activeDocument().hide(\"%s\")", targetOldTip->getNameInDocument());
            }
            if (targetNewTip) {
                doCommand(Gui,"Gui.activeDocument().show(\"%s\")", targetNewTip->getNameInDocument());
            }
        }

        // Fix sketch support
        if ((*f)->getTypeId().isDerivedFrom(Sketcher::SketchObject::getClassTypeId())) {
            Sketcher::SketchObject *sketch = static_cast<Sketcher::SketchObject*>(*f);
            try {
                PartDesignGui::Workbench::fixSketchSupport(sketch);
            } catch (Base::Exception &) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Sketch plane cannot be migrated"),
                        QObject::tr("Please edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.").
                        arg(QString::fromAscii(sketch->getNameInDocument()) ) );
            }
        }
    }
}

bool CmdPartDesignMoveFeature::isActive(void)
{
    return hasActiveDocument ();
}

DEF_STD_CMD_A(CmdPartDesignMoveFeatureInTree);

CmdPartDesignMoveFeatureInTree::CmdPartDesignMoveFeatureInTree()
  :Command("PartDesign_MoveFeatureInTree")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Move object after other object");
    sToolTipText    = QT_TR_NOOP("Moves the selected object and insert it after another object");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "";
}

void CmdPartDesignMoveFeatureInTree::activated(int iMsg)
{
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(/*messageIfNot = */true);
    if(!pcActiveBody) return;

    std::vector<App::DocumentObject*> features = getSelection().getObjectsOfType(Part::Feature::getClassTypeId());
    if (features.empty()) return;

    // Create a list of all features in this body
    std::vector<App::DocumentObject*> model = pcActiveBody->Model.getValues();

    // Ask user to select the target feature
    bool ok;
    QStringList items;
    for (std::vector<App::DocumentObject*>::iterator it = model.begin(); it != model.end(); ++it)
        items.push_back(QString::fromUtf8((*it)->Label.getValue()));
    QString text = QInputDialog::getItem(Gui::getMainWindow(),
        qApp->translate(className(), "Select feature"),
        qApp->translate(className(), "Select a feature from the list"),
        items, 0, false, &ok);
    if (!ok) return;
    int index = items.indexOf(text);
    PartDesign::Feature* target = static_cast<PartDesign::Feature*>(model[index]);

    openCommand("Move an object inside tree");

    for (std::vector<App::DocumentObject*>::const_iterator f = features.begin(); f != features.end(); f++) {
        if (*f == target) continue;

        // Remove and re-insert the feature from the Body
        // Note: If the tip was moved then the new tip will be at the moved position, that is, at the same
        // feature as before!
        doCommand(Doc,"App.activeDocument().%s.removeFeature(App.activeDocument().%s)",
                      pcActiveBody->getNameInDocument(), (*f)->getNameInDocument());
        doCommand(Doc,
                "App.activeDocument().%s.insertFeature(App.activeDocument().%s, App.activeDocument().%s, True)",
                pcActiveBody->getNameInDocument(), (*f)->getNameInDocument(), target->getNameInDocument());
    }

    // Recompute to update the shape
    doCommand(Gui,"App.activeDocument().recompute()");
}

bool CmdPartDesignMoveFeatureInTree::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}


//===========================================================================
// Initialization
//===========================================================================

void CreatePartDesignBodyCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdPartDesignPart());
    rcCmdMgr.addCommand(new CmdPartDesignBody());
    rcCmdMgr.addCommand(new CmdPartDesignMoveTip());

    rcCmdMgr.addCommand(new CmdPartDesignDuplicateSelection());
    rcCmdMgr.addCommand(new CmdPartDesignMoveFeature());
    rcCmdMgr.addCommand(new CmdPartDesignMoveFeatureInTree());
}
