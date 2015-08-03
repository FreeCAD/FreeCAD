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

#include "Utils.h"
#include "WorkflowManager.h"



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
    if ( PartDesignGui::assureModernWorkflow( getDocument() ) )
        return;

    openCommand("Add a part");
    std::string FeatName = getUniqueObjectName("Part");

    std::string PartName;
    PartName = getUniqueObjectName("Part");
    doCommand(Doc,"App.activeDocument().Tip = App.activeDocument().addObject('App::Part','%s')",PartName.c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.Label = '%s'", QObject::tr(PartName.c_str()).toStdString().c_str());
    PartDesignGui::setUpPart(dynamic_cast<App::Part *>(getDocument()->getObject(PartName.c_str())));
    doCommand(Gui::Command::Gui, "Gui.activeView().setActiveObject('%s', App.activeDocument().%s)", PARTKEY, PartName.c_str());

    updateActive();
}

bool CmdPartDesignPart::isActive(void)
{
    return hasActiveDocument() && !PartDesignGui::isLegacyWorkflow ( getDocument () );
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
    if ( PartDesignGui::assureModernWorkflow( getDocument() ) )
        return;
    std::vector<App::DocumentObject*> features =
        getSelection().getObjectsOfType(Part::Feature::getClassTypeId());
    App::DocumentObject* baseFeature = nullptr;

    if (!features.empty()) {
        if (features.size() == 1) {
            baseFeature = features[0];
            if ( baseFeature->isDerivedFrom ( PartDesign::Feature::getClassTypeId() ) &&
                    PartDesign::Body::findBodyOf ( baseFeature ) ) {
                // Prevent creatung bodies based on features already belonging to other bodies
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Bad base feature"),
                        QObject::tr("Body can't be based on a PartDesign feature."));
                return;

            }

        } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Bad base feature"),
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
    return hasActiveDocument() && !PartDesignGui::isLegacyWorkflow ( getDocument () );
}

//===========================================================================
// PartDesign_Migrate
//===========================================================================

DEF_STD_CMD_A(CmdPartDesignMigrate);

CmdPartDesignMigrate::CmdPartDesignMigrate()
  : Command("PartDesign_Migrate")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Migrate");
    sToolTipText  = QT_TR_NOOP("Migrate document to the new workflow");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
}

void CmdPartDesignMigrate::activated(int iMsg)
{
    App::Document *doc = getDocument();
    // TODO make a proper implementation
    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Not implemented yet"),
            QObject::tr("The migration not implemented yet, just force-switching to the new workflow.\n"
                "Previous workflow was: %1").arg(int(
                    PartDesignGui::WorkflowManager::instance()->determinWorkflow( doc ) )));
    PartDesignGui::WorkflowManager::instance()->forceWorkflow(doc, PartDesignGui::Workflow::Modern);
}

bool CmdPartDesignMigrate::isActive(void)
{
    return hasActiveDocument() && !PartDesignGui::isLegacyWorkflow ( getDocument () );
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

    if (!selFeature) {
        QMessageBox::warning (0, QObject::tr( "Selection error" ),
                QObject::tr( "Select exactly one PartDesign feature or a body." ) );
        return;
    } else if (!body) {
        QMessageBox::warning (0, QObject::tr( "Selection error" ),
                QObject::tr( "Couldn't determin a body for the selected feature '%s'.", selFeature->Label.getValue() ) );
        return;
    } else if ( !selFeature->isDerivedFrom(PartDesign::Feature::getClassTypeId () ) &&
            !selFeature->getTypeId().isDerivedFrom ( PartDesign::Body::getClassTypeId() ) &&
            body->BaseFeature.getValue() != selFeature ) {
        QMessageBox::warning (0, QObject::tr( "Selection error" ),
                QObject::tr( "Only a solid feature can be the tip of a body." ) );
        return;
    }

    App::DocumentObject* oldTip = body->Tip.getValue();
    if (oldTip == selFeature) { // it's not generally an error, so print only a console message
        Base::Console().Message ("%s is already the tip of the body", selFeature->getNameInDocument () );
        return;
    }

    openCommand("Move tip to selected feature");
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
    updateActive();
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
}

void CmdPartDesignDuplicateSelection::activated(int iMsg) {
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(/*messageIfNot = */false);

    std::vector<App::DocumentObject*> beforeFeatures = getDocument()->getObjects();

    openCommand("Duplicate a PartDesign object");
    doCommand(Doc,"FreeCADGui.runCommand('Std_DuplicateSelection')");

    if (pcActiveBody) {
        // Find the features that were added
        std::vector<App::DocumentObject*> afterFeatures = getDocument()->getObjects();
        std::vector<App::DocumentObject*> newFeatures;
        std::sort(beforeFeatures.begin(), beforeFeatures.end());
        std::sort(afterFeatures.begin(), afterFeatures.end());
        std::set_difference(afterFeatures.begin(), afterFeatures.end(), beforeFeatures.begin(), beforeFeatures.end(),
                            std::back_inserter(newFeatures));

        for (auto feature : newFeatures) {
            if (PartDesign::Body::isAllowed(feature)) {
                doCommand(Doc,"App.activeDocument().%s.addFeature(App.activeDocument().%s)",
                          pcActiveBody->getNameInDocument(), feature->getNameInDocument());
                doCommand(Gui,"Gui.activeDocument().hide(\"%s\")", feature->getNameInDocument());
            }
        }

        // Adjust visibility of features
        doCommand(Gui,"Gui.activeDocument().show(\"%s\")", newFeatures.back()->getNameInDocument());
    }

    updateActive();
}

bool CmdPartDesignDuplicateSelection::isActive(void)
{
    return hasActiveDocument();
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

    for (auto feat: features) {
        // Find body of this feature
        Part::BodyBase* source = PartDesign::Body::findBodyOf(feat);
        bool featureWasTip = false;

        if (source == target) continue;

        // Remove from the source body if the feature belonged to a body
        if (source) {
            featureWasTip = (source->Tip.getValue() == feat);
            doCommand(Doc,"App.activeDocument().%s.removeFeature(App.activeDocument().%s)",
                      source->getNameInDocument(), (feat)->getNameInDocument());
        }

        App::DocumentObject* targetOldTip = target->Tip.getValue();

        // Add to target body (always at the Tip)
        doCommand(Doc,"App.activeDocument().%s.addFeature(App.activeDocument().%s)",
                      target->getNameInDocument(), (feat)->getNameInDocument());
        // Recompute to update the shape
        doCommand(Gui,"App.activeDocument().recompute()");

        // Adjust visibility of features
        // TODO: May be something can be done in view provider (2015-08-05, Fat-Zer)
        // If we removed the tip of the source body, make the new tip visible
        if ( featureWasTip ) {
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
        if (feat->getTypeId().isDerivedFrom(Sketcher::SketchObject::getClassTypeId())) {
            Sketcher::SketchObject *sketch = static_cast<Sketcher::SketchObject*>(feat);
            try {
                PartDesignGui::fixSketchSupport(sketch);
            } catch (Base::Exception &) {
                QMessageBox::warning( Gui::getMainWindow(), QObject::tr("Sketch plane cannot be migrated"),
                        QObject::tr("Please edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.").
                        arg( QString::fromAscii( sketch->Label.getValue () ) ) );
            }
        }
    }

    updateActive();
}

bool CmdPartDesignMoveFeature::isActive(void)
{
    return hasActiveDocument () && !PartDesignGui::isLegacyWorkflow ( getDocument () );
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
    std::vector<App::DocumentObject*> features = getSelection().getObjectsOfType(Part::Feature::getClassTypeId());
    if (features.empty()) return;

    PartDesign::Body *body = PartDesignGui::getBodyFor ( features.front(), false );
    App::DocumentObject * bodyBase = nullptr;
    // sanity check
    bool allFeaturesFromSameBody = true;

    if ( body ) {
        bodyBase= body->BaseFeature.getValue();
        for ( auto feat: features ) {
            if ( !body->hasFeature ( feat ) ) {
                allFeaturesFromSameBody = false;
                break;
            }
            if ( bodyBase== feat) {
                QMessageBox::warning (0, QObject::tr( "Selection error" ),
                        QObject::tr( "Impossible to move the base feature of a body." ) );
                return;
            }
        }
    }
    if (!body || ! allFeaturesFromSameBody) {
        QMessageBox::warning (0, QObject::tr( "Selection error" ),
                QObject::tr( "Select one or more features from the same body." ) );
        return;
    }

    // Create a list of all features in this body
    const std::vector<App::DocumentObject*> & model = body->Model.getValues();

    // Ask user to select the target feature
    bool ok;
    QStringList items;
    if ( bodyBase ) {
        items.push_back( QString::fromUtf8 ( bodyBase->Label.getValue () ) );
    } else {
        items.push_back( QObject::tr( "Beginning of the body" ) );
    }
    for ( auto feat: model ) {
        items.push_back( QString::fromUtf8 ( feat->Label.getValue() ) );
    }

    QString text = QInputDialog::getItem(Gui::getMainWindow(),
        qApp->translate(className(), "Select feature"),
        qApp->translate(className(), "Select a feature from the list"),
        items, 0, false, &ok);
    if (!ok) return;
    int index = items.indexOf(text);
    // first object is the beginning of the body
    App::DocumentObject* target = index != 0 ? model[index-1] : nullptr;

    openCommand("Move an object inside tree");

    for ( auto feat: features ) {
        if ( feat == target ) continue;

        std::string targetStr;
        if (target) {
            targetStr.append("App.activeDocument().").append(target->getNameInDocument());
        } else {
            targetStr = "None";
        }

        // Remove and re-insert the feature to/from the Body
        // TODO if tip was moved the new position of tip is quite undetermined (2015-08-07, Fat-Zer)
        // TODO warn the user if we are moving an object to some place before the object's link (2015-08-07, Fat-Zer)
        doCommand ( Doc,"App.activeDocument().%s.removeFeature(App.activeDocument().%s)",
                body->getNameInDocument(), feat->getNameInDocument() );
        doCommand ( Doc, "App.activeDocument().%s.insertFeature(App.activeDocument().%s, %s, True)",
                body->getNameInDocument(), feat->getNameInDocument(), targetStr.c_str () );
    }

    updateActive();
}

bool CmdPartDesignMoveFeatureInTree::isActive(void)
{
    return hasActiveDocument () && !PartDesignGui::isLegacyWorkflow ( getDocument () );
}


//===========================================================================
// Initialization
//===========================================================================

void CreatePartDesignBodyCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdPartDesignPart());
    rcCmdMgr.addCommand(new CmdPartDesignBody());
    rcCmdMgr.addCommand(new CmdPartDesignMigrate());
    rcCmdMgr.addCommand(new CmdPartDesignMoveTip());

    rcCmdMgr.addCommand(new CmdPartDesignDuplicateSelection());
    rcCmdMgr.addCommand(new CmdPartDesignMoveFeature());
    rcCmdMgr.addCommand(new CmdPartDesignMoveFeatureInTree());
}
