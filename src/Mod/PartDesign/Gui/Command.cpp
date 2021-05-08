/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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
# include <TopoDS_Face.hxx>
# include <TopoDS.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRep_Tool.hxx>
# include <TopExp_Explorer.hxx>
# include <TopLoc_Location.hxx>
# include <GeomLib_IsPlanarSurface.hxx>
# include <QMessageBox>
# include <Inventor/nodes/SoCamera.h>
# include <sstream>
# include <algorithm>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <boost_bind_bind.hpp>

#include <App/DocumentObjectGroup.h>
#include <App/Origin.h>
#include <App/OriginFeature.h>
#include <App/Part.h>
#include <App/AutoTransaction.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/Control.h>
#include <Gui/Selection.h>
#include <Gui/MainWindow.h>
#include <Gui/Document.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureGroove.h>
#include <Mod/PartDesign/App/FeatureSplit.h>
#include <Mod/PartDesign/App/FeatureRevolution.h>

#include <Mod/PartDesign/App/FeatureTransformed.h>
#include <Mod/PartDesign/App/FeatureMultiTransform.h>
#include <Mod/PartDesign/App/DatumPoint.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/FeatureDressUp.h>
#include <Mod/PartDesign/App/ShapeBinder.h>

#include <Mod/Part/Gui/TaskAttacher.h>

#include "TaskFeaturePick.h"
#include "ReferenceSelection.h"
#include "Utils.h"
#include "WorkflowManager.h"
#include "ViewProvider.h"
#include "ViewProviderBody.h"

#include <Mod/Part/Gui/PartParams.h>

// TODO Remove this header after fixing code so it won;t be needed here (2015-10-20, Fat-Zer)
#include "ui_DlgReference.h"

FC_LOG_LEVEL_INIT("PartDesign",true,true)

using namespace std;
using namespace Attacher;
namespace bp = boost::placeholders;

static bool commandOverride(Gui::Command *cmd, int idx, const char *, int)
{
    if (PartDesignGui::queryCommandOverride()) {
        cmd->invoke(idx);
        return false;
    }
    return true;
}

//===========================================================================
// PartDesign_Datum
//===========================================================================

/**
 * @brief UnifiedDatumCommand is a common routine called by datum plane, line and point commands
 * @param cmd (i/o) command, to have shortcuts to doCommand, etc.
 * @param type (input)
 * @param name (input). Is used to generate new name for an object, and to fill undo messages.
 *
 */
void UnifiedDatumCommand(Gui::Command &cmd, Base::Type type, std::string name)
{
    try{
        App::DocumentObject *topParent = 0;
        std::string parentSub;

        std::set<App::DocumentObject *> valueSet;
        auto sels = Gui::Selection().getSelectionT("*", 0);
        for (auto &sel : sels)
            valueSet.insert(sel.getSubObject());

        Part::Datum *pcDatum = nullptr;
        if (valueSet.size() == 1 && (*valueSet.begin())->isDerivedFrom(type))
            pcDatum = static_cast<Part::Datum*>(*valueSet.begin());

        auto checkContainer = [&](App::DocumentObject *container) {
            // Check the potential container to add in the newly created datum.
            // We check if the container or any of its parent objects is selected
            // (i.e. for attaching), and exclude the container if it causes cyclic
            // reference.
            
            if (!container)
                return 0;

            if (valueSet.size() == 1 && (*valueSet.begin()) == container) {
                sels.clear();
                valueSet.clear();
                return 1;
            }

            if (pcDatum && pcDatum->getInListEx(false).count(container)) {
                std::string tmp = std::string("Edit ")+name;
                cmd.openCommand(tmp.c_str());
                Gui::cmdSetEdit(pcDatum);
                return -1;
            }

            auto inlist = container->getInListEx(true);
            for (auto obj : valueSet) {
                if (inlist.count(obj)) {
                    topParent = nullptr;
                    parentSub.clear();
                    return 0;
                }
            }
            return 1;
        };

        Gui::MDIView *activeView = Gui::Application::Instance->activeView();
        Part::BodyBase *pcActiveBody = nullptr;
        if (activeView)
            pcActiveBody = activeView->getActiveObject<Part::BodyBase*>(
                    PDBODYKEY, &topParent, &parentSub);
        int res = checkContainer(pcActiveBody);
        if (res < 0)
            return;
        if (res == 0)
            pcActiveBody = nullptr;
        App::Part *pcActivePart = nullptr;
        if (!pcActiveBody && activeView) {
            pcActivePart = activeView->getActiveObject<App::Part*>(
                    PARTKEY, &topParent, &parentSub);
            res = checkContainer(pcActivePart);
            if (res < 0)
                return;
            if (res == 0)
                pcActivePart = nullptr;
        }

        App::DocumentObject *pcActiveContainer = pcActiveBody;
        if (!pcActiveContainer)
            pcActiveContainer = pcActivePart;

        std::string FeatName;
        App::Document *doc = 0;
        FeatName = cmd.getUniqueObjectName(name.c_str(), pcActiveContainer);

        std::string tmp = std::string("Create ")+name;

        cmd.openCommand(tmp.c_str());

        if(pcActiveContainer) {
            Gui::cmdAppObject(pcActiveContainer, std::ostringstream()
                    << "newObject('" << type.getName() << "','" << FeatName << "')");
            doc = pcActiveContainer->getDocument();
        } else {
            doc = App::GetApplication().getActiveDocument();
            Gui::cmdAppDocument(doc, std::ostringstream()
                    << "addObject('" << type.getName() << "','" << FeatName << "')");
        }

        auto Feat = doc->getObject(FeatName.c_str());
        if(!Feat) return;

        App::SubObjectT containerT(topParent ? topParent : pcActiveContainer, parentSub.c_str());
        App::SubObjectT editObjT;
        if (pcActiveContainer)
            editObjT = containerT.getChild(Feat);
        else
            editObjT = Feat;

        for (auto it = sels.begin(); it != sels.end();) {
            auto &sel = *it;
            if (topParent) {
                std::string sub = sel.getSubName();
                if (sub.empty()) {
                    ++it;
                    continue;
                }
                auto link = sel.getObject();
                auto psub = parentSub;
                topParent->resolveRelativeLink(psub,link,sub);
                if (!link) {
                    FC_WARN("Failed to resolve relative link "
                            << sel.getSubObjectFullName() << " v.s. "
                            << containerT.getSubObjectFullName());
                    it = sels.erase(it);
                    continue;
                }
                sel = App::SubObjectT(link, sub.c_str());
            }

            sel = Part::SubShapeBinder::import(sel, editObjT, true);
            ++it;
        }


        //test if current selection fits a mode.
        if (sels.size() > 0) {
            Part::AttachExtension* pcDatum = Feat->getExtensionByType<Part::AttachExtension>();
            pcDatum->attacher().setReferences(sels);
            SuggestResult sugr;
            pcDatum->attacher().suggestMapModes(sugr);
            if (sugr.message == Attacher::SuggestResult::srOK) {
                std::ostringstream ss;
                for (auto &sel : sels)
                    ss << sel.getSubObjectPython() << ", ";
                //fits some mode. Populate support property.
                Gui::cmdAppObject(Feat, std::ostringstream() << "Support = [" << ss.str() << "]");
                Gui::cmdAppObject(Feat, std::ostringstream() << "MapMode = '" << AttachEngine::getModeName(sugr.bestFitMode) << "'");
            } else {
                QMessageBox::information(Gui::getMainWindow(),QObject::tr("Invalid selection"), QObject::tr("There are no attachment modes that fit selected objects. Select something else."));
            }
        }
        cmd.doCommand(Gui::Command::Doc,"App.activeDocument().recompute()");  // recompute the feature based on its references

        Gui::Selection().selStackPush();
        Gui::Selection().clearSelection();
        Gui::Selection().addSelection(editObjT);
        Gui::cmdSetEdit(Feat);

    } catch (Base::Exception &e) {
        QMessageBox::warning(Gui::getMainWindow(),QObject::tr("Error"),QString::fromLatin1(e.what()));
    } catch (Standard_Failure &e) {
        QMessageBox::warning(Gui::getMainWindow(),QObject::tr("Error"),QString::fromLatin1(e.GetMessageString()));
    }
}

/* Datum feature commands =======================================================*/

DEF_STD_CMD_A(CmdPartDesignPlane)

CmdPartDesignPlane::CmdPartDesignPlane()
  :Command("PartDesign_Plane")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Create a datum plane");
    sToolTipText    = QT_TR_NOOP("Create a new datum plane");
    sWhatsThis      = "PartDesign_Plane";
    sStatusTip      = sToolTipText;
    sPixmap         = "PartDesign_Plane";
}

void CmdPartDesignPlane::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    UnifiedDatumCommand(*this, Base::Type::fromName("PartDesign::Plane"),"DatumPlane");
}

bool CmdPartDesignPlane::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

DEF_STD_CMD_A(CmdPartDesignLine)

CmdPartDesignLine::CmdPartDesignLine()
  :Command("PartDesign_Line")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Create a datum line");
    sToolTipText    = QT_TR_NOOP("Create a new datum line");
    sWhatsThis      = "PartDesign_Line";
    sStatusTip      = sToolTipText;
    sPixmap         = "PartDesign_Line";
}

void CmdPartDesignLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    UnifiedDatumCommand(*this, Base::Type::fromName("PartDesign::Line"),"DatumLine");
}

bool CmdPartDesignLine::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

DEF_STD_CMD_A(CmdPartDesignPoint)

CmdPartDesignPoint::CmdPartDesignPoint()
  :Command("PartDesign_Point")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Create a datum point");
    sToolTipText    = QT_TR_NOOP("Create a new datum point");
    sWhatsThis      = "PartDesign_Point";
    sStatusTip      = sToolTipText;
    sPixmap         = "PartDesign_Point";
}

void CmdPartDesignPoint::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    UnifiedDatumCommand(*this, Base::Type::fromName("PartDesign::Point"),"DatumPoint");
}

bool CmdPartDesignPoint::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

DEF_STD_CMD_A(CmdPartDesignCS)

CmdPartDesignCS::CmdPartDesignCS()
  :Command("PartDesign_CoordinateSystem")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Create a local coordinate system");
    sToolTipText    = QT_TR_NOOP("Create a new local coordinate system");
    sWhatsThis      = "PartDesign_CoordinateSystem";
    sStatusTip      = sToolTipText;
    sPixmap         = "PartDesign_CoordinateSystem";
}

void CmdPartDesignCS::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    UnifiedDatumCommand(*this, Base::Type::fromName("PartDesign::CoordinateSystem"),"Local_CS");
}

bool CmdPartDesignCS::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// PartDesign_ShapeBinder
//===========================================================================

DEF_STD_CMD_A(CmdPartDesignShapeBinder)

CmdPartDesignShapeBinder::CmdPartDesignShapeBinder()
  :Command("PartDesign_ShapeBinder")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Create a shape binder");
    sToolTipText    = QT_TR_NOOP("Create a new shape binder");
    sWhatsThis      = "PartDesign_ShapeBinder";
    sStatusTip      = sToolTipText;
    sPixmap         = "PartDesign_ShapeBinder";
}

void CmdPartDesignShapeBinder::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::PropertyLinkSubList support;
    getSelection().getAsPropertyLinkSubList(support);

    bool bEditSelected = false;
    if (support.getSize() == 1 && support.getValue() ){
        if (support.getValue()->isDerivedFrom(PartDesign::ShapeBinder::getClassTypeId()))
            bEditSelected = true;
    }

    if (bEditSelected) {
        openCommand(QT_TRANSLATE_NOOP("Command", "Edit ShapeBinder"));
        PartDesignGui::setEdit(support.getValue());
    } else {
        PartDesign::Body *pcActiveBody = PartDesignGui::getBody(/*messageIfNot = */true);
        if (pcActiveBody == 0)
            return;

        std::string FeatName = getUniqueObjectName("ShapeBinder",pcActiveBody);

        openCommand(QT_TRANSLATE_NOOP("Command", "Create ShapeBinder"));
        Gui::cmdAppObject(pcActiveBody, std::ostringstream()
                << "newObject('PartDesign::ShapeBinder', '" << FeatName << "')");

        // remove the body from links in case it's selected as
        // otherwise a cyclic dependency will be created
        support.removeValue(pcActiveBody);

        auto Feat = pcActiveBody->getObject(FeatName.c_str());
        if (!Feat) return;

        //test if current selection fits a mode.
        if (support.getSize() > 0) {
            Gui::cmdAppObject(Feat, std::ostringstream() <<"Support = " << support.getPyReprString());
        }
        updateActive();
        PartDesignGui::setEdit(Feat,pcActiveBody);
    }
    // TODO do a proper error processing (2015-09-11, Fat-Zer)
}

bool CmdPartDesignShapeBinder::isActive(void)
{
    return hasActiveDocument ();
}

//===========================================================================
// PartDesign_SubShapeBinder
//===========================================================================

DEF_STD_CMD_A(CmdPartDesignSubShapeBinder)

CmdPartDesignSubShapeBinder::CmdPartDesignSubShapeBinder()
  :Command("PartDesign_SubShapeBinder")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Create a sub-object(s) shape binder");
    sToolTipText    = QT_TR_NOOP("Create a sub-object(s) shape binder");
    sWhatsThis      = "PartDesign_SubShapeBinder";
    sStatusTip      = sToolTipText;
    sPixmap         = "PartDesign_SubShapeBinder";

    Gui::Application::Instance->commandManager().registerCallback(
            boost::bind(&commandOverride, this, 0, bp::_1, bp::_2), "Part_MakeFace");
}

namespace PartGui {
PartGuiExport void makeSubShapeBinder(Base::Type type);
}

void CmdPartDesignSubShapeBinder::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    PartGui::makeSubShapeBinder(PartDesign::SubShapeBinder::getClassTypeId());
}

bool CmdPartDesignSubShapeBinder::isActive(void) {
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Clone
//===========================================================================

DEF_STD_CMD_A(CmdPartDesignClone)

CmdPartDesignClone::CmdPartDesignClone()
  :Command("PartDesign_Clone")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Create a clone");
    sToolTipText    = QT_TR_NOOP("Create a new clone");
    sWhatsThis      = "PartDesign_Clone";
    sStatusTip      = sToolTipText;
    sPixmap         = "PartDesign_Clone";
}

void CmdPartDesignClone::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<App::DocumentObject*> objs = getSelection().getObjectsOfType
            (Part::Feature::getClassTypeId());
    if (objs.size() == 1) {
        // As suggested in https://forum.freecadweb.org/viewtopic.php?f=3&t=25265&p=198547#p207336
        // put the clone into its own new body.
        // This also fixes bug #3447 because the clone is a PD feature and thus
        // requires a body where it is part of.
        openCommand(QT_TRANSLATE_NOOP("Command", "Create Clone"));
        auto obj = objs[0];
        std::string FeatName = getUniqueObjectName("Clone",obj);
        std::string BodyName = getUniqueObjectName("Body",obj);
        FCMD_OBJ_DOC_CMD(obj,"addObject('PartDesign::Body','" << BodyName << "')");
        FCMD_OBJ_DOC_CMD(obj,"addObject('PartDesign::FeatureBase','" << FeatName << "')");
        auto Feat = obj->getDocument()->getObject(FeatName.c_str());
        auto objCmd = getObjectCmd(obj);
        Gui::cmdAppObject(Feat, std::ostringstream() <<"BaseFeature = " << objCmd);
        Gui::cmdAppObject(Feat, std::ostringstream() <<"Placement = " << objCmd << ".Placement");
        Gui::cmdAppObject(Feat, std::ostringstream() <<"setEditorMode('Placement',0)");

        auto Body = obj->getDocument()->getObject(BodyName.c_str());
        Gui::cmdAppObject(Body, std::ostringstream() <<"Group = [" << getObjectCmd(Feat) << "]");

        // Set the tip of the body
        Gui::cmdAppObject(Body, std::ostringstream() <<"Tip = " << getObjectCmd(Feat));
        updateActive();
        copyVisual(Feat, "ShapeColor", obj);
        copyVisual(Feat, "LineColor", obj);
        copyVisual(Feat, "PointColor", obj);
        copyVisual(Feat, "Transparency", obj);
        copyVisual(Feat, "DisplayMode", obj);
        commitCommand();
    }
}

bool CmdPartDesignClone::isActive(void)
{
    return getSelection().countObjectsOfType(Part::Feature::getClassTypeId()) == 1;
}

//===========================================================================
// PartDesign_Sketch
//===========================================================================

/* Sketch commands =======================================================*/
DEF_STD_CMD_A(CmdPartDesignNewSketch)

CmdPartDesignNewSketch::CmdPartDesignNewSketch()
  :Command("PartDesign_NewSketch")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Create sketch");
    sToolTipText    = QT_TR_NOOP("Create a new sketch");
    sWhatsThis      = "PartDesign_NewSketch";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_NewSketch";
}


void CmdPartDesignNewSketch::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::Document *doc = getDocument ();
    PartDesign::Body *pcActiveBody( nullptr );
    auto shouldMakeBody( false );

    App::SubObjectT bodyT;
    App::SubObjectT reference;

    if ( PartDesignGui::assureModernWorkflow( doc ) ) {
        // We need either an active Body, or for there to be no Body
        // objects (in which case, just make one) to make a new sketch.

        pcActiveBody = PartDesignGui::getBody(bodyT, /* messageIfNot = */ false );
        if (pcActiveBody == nullptr) {
            if ( doc->getObjectsOfType(PartDesign::Body::getClassTypeId()).empty() ) {
                shouldMakeBody = true;
            } else {
                PartDesignGui::needActiveBodyError();
                return;
            }
        }

    } else {
        // No PartDesign feature without Body past FreeCAD 0.13
        if ( PartDesignGui::isLegacyWorkflow( doc ) ) {
            Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
            rcCmdMgr.runCommandByName("Sketcher_NewSketch");
        }
        return;
    }

    // Obtain a single selection from any object in any document. We'll use
    // SubShapeBinder::import() to deal with external references.
    auto sels = Gui::Selection().getSelectionT("*", 0, true);
    App::DocumentObject *obj = nullptr;
    if (!sels.empty() && (obj = sels[0].getSubObject())!=nullptr) {
        reference = sels[0];
        obj = obj->getLinkedObject(true);
        if (!obj->isDerivedFrom(App::Plane::getClassTypeId())
                && !obj->isDerivedFrom(PartDesign::Plane::getClassTypeId()))
        {
            auto shape = Part::Feature::getTopoShape(reference.getObject(),
                                                     reference.getSubName().c_str(),
                                                     true);
            if (shape.isNull() && obj == pcActiveBody) {
                obj = nullptr;
                reference = App::SubObjectT();
            } else {
                gp_Pln pln;
                if (!shape.findPlane(pln)) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No planar support"),
                            QObject::tr("You need a planar face as support for a sketch!"));
                    return;
                }
            }
        }

        // In case the selected face belongs to the body then it means its
        // Display Mode Body is set to Tip. But the body face is not allowed
        // to be used as support because otherwise it would cause a cyclic
        // dependency. So, instead we use the tip object as reference.
        // https://forum.freecadweb.org/viewtopic.php?f=3&t=37448
        if (obj && obj == pcActiveBody) {
            App::DocumentObject* tip = pcActiveBody->Tip.getValue();
            if (tip && tip->isDerivedFrom(Part::Feature::getClassTypeId())) {
                reference.setSubName(reference.getSubNameNoElement()
                        + tip->getNameInDocument() + "." + reference.getOldElementName());
                // automatically switch to 'Through' mode
                PartDesignGui::ViewProviderBody* vpBody = dynamic_cast<PartDesignGui::ViewProviderBody*>
                        (Gui::Application::Instance->getViewProvider(pcActiveBody));
                if (vpBody) {
                    vpBody->DisplayModeBody.setValue("Through");
                }
            }
        }
    } else {
        Gui::Selection().selStackPush();
        Gui::Selection().clearSelection();
    }

    // Start command early, so undo will undo any Body creation
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create a new Sketch"));
    if (shouldMakeBody) {
        pcActiveBody = PartDesignGui::makeBody(doc);
        if ( !pcActiveBody ) {
            Base::Console().Error("Failed to create a Body object");
            return;
        }
    }

    PartDesignGui::getBody(bodyT, false);
    if (reference.getObjectName().size())
        reference = Part::SubShapeBinder::import(reference, bodyT, true);

    // create Sketch on Face or Plane
    std::string FeatName = getUniqueObjectName("Sketch",pcActiveBody);

    openCommand(QT_TRANSLATE_NOOP("Command", "Create a Sketch on Face"));
    Gui::cmdAppObject(pcActiveBody, std::ostringstream()
            << "newObjectAt('Sketcher::SketchObject', '" << FeatName << "', "
                        <<  "FreeCADGui.Selection.getSelection())");
    auto sketch = pcActiveBody->getDocument()->getObject(FeatName.c_str());
    if (!reference.getObjectName().empty()) {
        Gui::cmdAppObject(sketch, std::ostringstream() <<"Support = " << reference.getSubObjectPython());
        Gui::cmdAppObject(sketch, std::ostringstream() <<"MapMode = '" << Attacher::AttachEngine::getModeName(Attacher::mmFlatFace)<<"'");
        updateActive();
        PartDesignGui::setEdit(sketch,pcActiveBody);
        return;
    }

    // No attachment reference. Open attachment task panel
    auto sketchvp = Base::freecad_dynamic_cast<Gui::ViewProviderDocumentObject>(
            Gui::Application::Instance->getViewProvider(sketch));
    if (sketchvp) {
        Gui::Selection().selStackPush();
        Gui::Selection().clearSelection();
        // Add selection so that TaskAttacher can deduce editing context
        Gui::Selection().addSelection(bodyT.getChild(sketch));
        Gui::Control().closeDialog();
        auto task = new PartGui::TaskDlgAttacher(sketchvp,
                                                 true,
                                                 QString::fromLatin1("Sketcher_Sketch"),
                                                 QObject::tr("Sketch attachment"));
        task->editAfterClose();
        Gui::Control().showDialog(task);
    }
}

bool CmdPartDesignNewSketch::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Common utility functions for all features creating solids
//===========================================================================

void finishFeature(const Gui::Command* cmd, App::DocumentObject *Feat,
                   App::DocumentObject* prevSolidFeature = nullptr,
                   const bool hidePrevSolid = true,
                   const bool updateDocument = true)
{
    PartDesign::Body *pcActiveBody;

    if (prevSolidFeature) {
        pcActiveBody = PartDesignGui::getBodyFor(prevSolidFeature, /*messageIfNot = */false);
    } else { // insert into the same body as the given previous one
        pcActiveBody = PartDesignGui::getBody(/*messageIfNot = */false);
    }

    if (hidePrevSolid && prevSolidFeature)
        FCMD_OBJ_HIDE(prevSolidFeature);

    if (updateDocument)
        cmd->updateActive();

    auto base = dynamic_cast<PartDesign::Feature*>(Feat);
    if (base)
        base = dynamic_cast<PartDesign::Feature*>(base->getBaseObject(true));
    App::DocumentObject *obj = base;
    if (!obj)
        obj = pcActiveBody;

    // Do this before calling setEdit to avoid to override the 'Shape preview' mode (#0003621)
    if (obj) {
        cmd->copyVisual(Feat, "ShapeColor", obj);
        cmd->copyVisual(Feat, "LineColor", obj);
        cmd->copyVisual(Feat, "PointColor", obj);
        cmd->copyVisual(Feat, "Transparency", obj);
        cmd->copyVisual(Feat, "DisplayMode", obj);
    }

    // #0001721: use '0' as edit value to avoid switching off selection in
    // ViewProviderGeometryObject::setEditViewer
    PartDesignGui::setEdit(Feat,pcActiveBody);
    cmd->doCommand(cmd->Gui,"Gui.Selection.clearSelection()");
    //cmd->doCommand(cmd->Gui,"Gui.Selection.addSelection(App.ActiveDocument.ActiveObject)");
}

//===========================================================================
// Common utility functions for ProfileBased features
//===========================================================================

// Take a list of Part2DObjects and classify them for creating a
// ProfileBased feature. FirstFreeSketch is the first free sketch in the same body
// or sketches.end() if non available. The returned number is the amount of free sketches
unsigned validateSketches(std::vector<App::DocumentObject*>& sketches,
                          std::vector<PartDesignGui::TaskFeaturePick::featureStatus>& status,
                          std::vector<App::DocumentObject*>::iterator& firstFreeSketch)
{
    // TODO Review the function for non-part bodies (2015-09-04, Fat-Zer)
    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(false);
    App::Part* pcActivePart = PartDesignGui::getPartFor(pcActiveBody, false);

    // TODO: If the user previously opted to allow multiple use of sketches or use of sketches from other bodies,
    // then count these as valid sketches!
    unsigned freeSketches = 0;
    firstFreeSketch = sketches.end();

    for (std::vector<App::DocumentObject*>::iterator s = sketches.begin(); s != sketches.end(); s++) {

        if (!pcActiveBody) {
            // We work in the old style outside any body
            if (PartDesign::Body::findBodyOf (*s)) {
                status.push_back(PartDesignGui::TaskFeaturePick::otherPart);
                continue;
            }
        } else if (!pcActiveBody->hasObject(*s)) {
            // Check whether this plane belongs to a body of the same part
            PartDesign::Body* b = PartDesign::Body::findBodyOf(*s);
            if (!b)
                status.push_back(PartDesignGui::TaskFeaturePick::notInBody);
            else if (pcActivePart && pcActivePart->hasObject(b, true))
                status.push_back(PartDesignGui::TaskFeaturePick::otherBody);
            else
                status.push_back(PartDesignGui::TaskFeaturePick::otherPart);

            continue;
        }

        //Base::Console().Error("Checking sketch %s\n", (*s)->getNameInDocument());
        // Check whether this sketch is already being used by another feature
        // Body features don't count...
        std::vector<App::DocumentObject*> inList = (*s)->getInList();
        std::vector<App::DocumentObject*>::iterator o = inList.begin();
        while (o != inList.end()) {
            //Base::Console().Error("Inlist: %s\n", (*o)->getNameInDocument());
            if ((*o)->getTypeId().isDerivedFrom(PartDesign::Body::getClassTypeId()))
                o = inList.erase(o); //ignore bodies
            else if (!(  (*o)->getTypeId().isDerivedFrom(PartDesign::Feature::getClassTypeId())  ))
                o = inList.erase(o); //ignore non-partDesign
            else
                ++o;
        }
        if (inList.size() > 0) {
            status.push_back(PartDesignGui::TaskFeaturePick::isUsed);
            continue;
        }

        if (pcActiveBody && pcActiveBody->isAfterInsertPoint(*s)){
            status.push_back(PartDesignGui::TaskFeaturePick::afterTip);
            continue;
        }

        // Check whether the sketch shape is valid
        Part::Part2DObject* sketch = static_cast<Part::Part2DObject*>(*s);
        const TopoDS_Shape& shape = sketch->Shape.getValue();
        if (shape.IsNull()) {
            status.push_back(PartDesignGui::TaskFeaturePick::invalidShape);
            continue;
        }

        // count free wires
        int ctWires=0;
        TopExp_Explorer ex;
        for (ex.Init(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
            ctWires++;
        }
        if (ctWires == 0) {
            status.push_back(PartDesignGui::TaskFeaturePick::noWire);
            continue;
        }

        // All checks passed - found a valid sketch
        if (firstFreeSketch == sketches.end())
            firstFreeSketch = s;
        freeSketches++;
        status.push_back(PartDesignGui::TaskFeaturePick::validFeature);
    }

    return freeSketches;
}

void prepareProfileBased(PartDesign::Body *pcActiveBody, Gui::Command* cmd, const std::string& which,
                        boost::function<void (Part::Feature*, App::DocumentObject*)> func,
                        bool needSubElement = false)
{
    //if a profile is selected we can make our life easy and fast
    auto sels = Gui::Selection().getSelectionT("*", 0);

    auto base_worker = [=](App::DocumentObject* feature, const std::vector<string> &subs) {

        if (!feature || !feature->isDerivedFrom(Part::Feature::getClassTypeId()))
            return;

        // Related to #0002760: when an operation can't be performed due to a broken
        // profile then make sure that it is recomputed when cancelling the operation
        // otherwise it might be impossible to see that it's broken.
        if (feature->isTouched())
            feature->recomputeFeature();

        std::string FeatName = cmd->getUniqueObjectName(which.c_str(),pcActiveBody);

        Gui::Command::openCommand((std::string("Make ") + which).c_str());

        Gui::cmdAppObject(pcActiveBody, std::ostringstream()
                << "newObjectAt('PartDesign::" << which << "','" << FeatName << "', "
                            <<  "FreeCADGui.Selection.getSelection())");
        auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());

        auto objCmd = Gui::Command::getObjectCmd(feature);
        bool profileSet = false;
        if (subs.size() && !needSubElement
                        && feature->isDerivedFrom(Part::Part2DObject::getClassTypeId()))
        {
            auto shape = Part::Feature::getTopoShape(feature);
            if (subs.size() < shape.countSubShapes(TopAbs_EDGE)) {
                std::vector<Part::TopoShape> shapes;
                for (auto &sub : subs) {
                    auto subshape = shape.getSubShape(sub.c_str(), true);
                    if (subshape.IsNull() || subshape.ShapeType() != TopAbs_EDGE)
                        break;
                    shapes.push_back(subshape);
                }
                if (subs.size() == shapes.size()) {
                    std::string BinderName = cmd->getUniqueObjectName(
                            "Binder", pcActiveBody);
                    Gui::cmdAppObject(pcActiveBody, std::ostringstream()
                            << "newObject('PartDesign::SubShapeBinder', '" << BinderName << "')");
                    auto binder = pcActiveBody->getDocument()->getObject(BinderName.c_str());
                    std::ostringstream ss;
                    for (auto &s : subs)
                        ss << "'" << s << "',";
                    Gui::cmdAppObject(binder, std::ostringstream() << "Support = (" << objCmd
                            << ", [" << ss.str() << "])");
                    Gui::cmdAppObject(Feat, std::ostringstream() << "Profile = "
                            << Gui::Command::getObjectCmd(binder));
                    profileSet = true;
                }
            }
        }

        if (!profileSet) {
            if (subs.empty()
                    || (!needSubElement
                        && feature->isDerivedFrom(Part::Part2DObject::getClassTypeId())))
            {
                Gui::cmdAppObject(Feat, std::ostringstream() <<"Profile = " << objCmd);
            } else {
                std::ostringstream ss;
                for (auto &s : subs)
                    ss << "'" << s << "',";
                Gui::cmdAppObject(Feat, std::ostringstream() <<"Profile = (" << objCmd
                        << ", [" << ss.str() << "])");
            }
        }

        //for additive and subtractive lofts allow the user to preselect the sections
        if (which.compare("AdditiveLoft") == 0 || which.compare("SubtractiveLoft") == 0) {
            std::ostringstream ss;
            int count = 0;
            ss << "Sections = [";
            std::set<App::DocumentObject*> objSet;
            for (unsigned i=1; i<sels.size(); ++i) {
                if (!objSet.insert(sels[i].getSubObject()).second)
                    continue;
                auto objT = PartDesignGui::importExternalObject(sels[i], false);
                if (objT.getObjectName().empty())
                    continue;
                ss << objT.getObjectPython() << ", ";
                ++count;
            }
            if (count)
                Gui::cmdAppObject(Feat, ss << "]");
        }

        // for additive and subtractive pipes allow the user to preselect the spines
        if (which.compare("AdditivePipe") == 0 || which.compare("SubtractivePipe") == 0) {
            if (sels.size() > 1) {
                //treat additional selected object as spine
                App::DocumentObject *spine = nullptr;
                App::SubObjectT ref;
                std::vector<std::string> subs;
                for (unsigned i=1; i<sels.size(); ++i) {
                    if (!spine) {
                        ref = PartDesignGui::importExternalObject(sels[i], false);
                        if (ref.getObjectName().empty())
                            continue;
                        spine = sels[i].getSubObject();
                        if (!spine)
                            continue;
                    } else if (spine != sels[i].getSubObject())
                        continue;
                    auto sub = sels[i].getOldElementName();
                    if (sub.size())
                        subs.push_back(std::move(sub));
                }
                if (spine) {
                    std::ostringstream ss;
                    for(auto &s : subs)
                        ss << "'" << s << "',";
                    Gui::cmdAppObject(Feat, std::ostringstream()
                            <<"Spine = (" << ref.getObjectPython() << ", [" << ss.str() << "])");
                }
            }
        }

        func(static_cast<Part::Feature*>(feature), Feat);
    };


    // in case of subtractive types, check that there is something to subtract from
    if ((which.find("Subtractive") != std::string::npos)  ||
        (which.compare("Groove") == 0) ||
        (which.compare("Pocket") == 0)) {

        if (!pcActiveBody->isSolid()) {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("Cannot use this command as there is no solid to subtract from."));
            msgBox.setInformativeText(QObject::tr("Ensure that the body contains a feature  before attempting a subtractive command."));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
            return;
        }
    }


    if (!sels.empty()) {
        App::SubObjectT ref;
        for (unsigned i=0; i<sels.size(); ++i) {
            auto sobj = sels[i].getSubObject();
            if (!sobj)
                continue;
            ref = PartDesignGui::importExternalObject(sels[i], false);
            if (auto obj = ref.getSubObject()) {
                std::vector<std::string> subs;
                if (sels[i].hasSubElement())
                    subs.push_back(sels[i].getOldElementName());
                for (unsigned j=i+1; j<sels.size(); ++j) {
                    if (sels[j].getSubObject() == sobj && sels[j].hasSubElement())
                        subs.push_back(sels[j].getOldElementName());
                }
                base_worker(obj, subs);
                if (PartGui::PartParams::AdjustCameraForNewFeature())
                    cmd->adjustCameraPosition();
                return;
            }
        }
    }

    //no face profile was selected, do the extended sketch logic

    // Get a valid sketch from the user
    // First check selections
    std::vector<App::DocumentObject*> sketches = pcActiveBody->getObjectsOfType(Part::Part2DObject::getClassTypeId());
    if (sketches.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No sketch to work on"),
            QObject::tr("No sketch is available in the body"));
        return;
    }

    std::vector<PartDesignGui::TaskFeaturePick::featureStatus> status;
    std::vector<App::DocumentObject*>::iterator firstFreeSketch;
    int freeSketches = validateSketches(sketches, status, firstFreeSketch);

    auto accepter = [=](const std::vector<App::DocumentObject*>& features) -> bool {

        if (features.empty())
            return false;

        return true;
    };

    auto sketch_worker = [&, base_worker](std::vector<App::DocumentObject*> features) {
        base_worker(features.front(), {});
    };

    // Show sketch choose dialog and let user pick sketch if no sketch was selected and no free one available or
    // multiple free ones are available
    if (freeSketches != 1) {
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        PartDesignGui::TaskDlgFeaturePick *pickDlg = qobject_cast<PartDesignGui::TaskDlgFeaturePick *>(dlg);
        if (dlg && !pickDlg) {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes)
                Gui::Control().closeDialog();
            else
                return;
        }

        if (dlg)
            Gui::Control().closeDialog();

        Gui::Selection().clearSelection();
        pickDlg = new PartDesignGui::TaskDlgFeaturePick(sketches, status, accepter, sketch_worker, true);
        Gui::Control().showDialog(pickDlg);
    }
    else {
        sketch_worker({*firstFreeSketch});
    }
}

void finishProfileBased(const Gui::Command* cmd, const Part::Feature* sketch, App::DocumentObject *Feat)
{
    (void)sketch;
    for (auto obj : Feat->getOutList()) {
        if (obj->isDerivedFrom(Part::Part2DObject::getClassTypeId())
                || obj->isDerivedFrom(Part::SubShapeBinder::getClassTypeId()))
            Gui::cmdAppObjectHide(obj);
    }
    finishFeature(cmd, Feat);
}

//===========================================================================
// PartDesign_Pad
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignPad)

CmdPartDesignPad::CmdPartDesignPad()
  : Command("PartDesign_Pad")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Pad");
    sToolTipText  = QT_TR_NOOP("Pad a selected sketch");
    sWhatsThis    = "PartDesign_Pad";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Pad";

    Gui::Application::Instance->commandManager().registerCallback(
            boost::bind(&commandOverride, this, 0, bp::_1, bp::_2), "Part_Extrude");
}

void CmdPartDesignPad::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::Command* cmd = this;
    auto worker = [cmd](Part::Feature* profile, App::DocumentObject *Feat) {

        if (!Feat) return;

        // specific parameters for Pad
        Gui::cmdAppObject(Feat, std::ostringstream() <<"Length = 10.0");
        Gui::Command::updateActive();

        Part::Part2DObject* sketch = dynamic_cast<Part::Part2DObject*>(profile);
        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, this, "Pad", worker);
}

bool CmdPartDesignPad::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Extrusion
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignExtrusion)

CmdPartDesignExtrusion::CmdPartDesignExtrusion()
  : Command("PartDesign_Extrusion")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Extrusion");
    sToolTipText  = QT_TR_NOOP("Extrude a vertex/edge/face/sketch.\n"
                               "The resulting shape standalone and not merged into tip.");
    sWhatsThis    = "PartDesign_Extrusion";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Extrusion";
}

void CmdPartDesignExtrusion::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::Command* cmd = this;
    auto worker = [cmd](Part::Feature* profile, App::DocumentObject *Feat) {

        if (!Feat) return;

        Gui::cmdAppObject(Feat, std::ostringstream() <<"Length = 10.0");
        Gui::Command::updateActive();

        (void)profile;
        finishFeature(cmd, Feat, nullptr, false, false);
    };

    prepareProfileBased(pcActiveBody, this, "Extrusion", worker, true);
}

bool CmdPartDesignExtrusion::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Pocket
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignPocket)

CmdPartDesignPocket::CmdPartDesignPocket()
  : Command("PartDesign_Pocket")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Pocket");
    sToolTipText  = QT_TR_NOOP("Create a pocket with the selected sketch");
    sWhatsThis    = "PartDesign_Pocket";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Pocket";
}

void CmdPartDesignPocket::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::Command* cmd = this;
    auto worker = [cmd](Part::Feature* sketch, App::DocumentObject *Feat) {

        if (!Feat) return;

        Gui::cmdAppObject(Feat, std::ostringstream() <<"Length = 5.0");
        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, this, "Pocket", worker);
}

bool CmdPartDesignPocket::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Hole
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignHole)

CmdPartDesignHole::CmdPartDesignHole()
  : Command("PartDesign_Hole")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Hole");
    sToolTipText  = QT_TR_NOOP("Create a hole with the selected sketch");
    sWhatsThis    = "PartDesign_Hole";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Hole";
}

void CmdPartDesignHole::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
                return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::Command* cmd = this;
    auto worker = [cmd](Part::Feature* sketch, App::DocumentObject *Feat) {

        if (!Feat) return;

        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, this, "Hole", worker);
}

bool CmdPartDesignHole::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Revolution
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignRevolution)

CmdPartDesignRevolution::CmdPartDesignRevolution()
  : Command("PartDesign_Revolution")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Revolution");
    sToolTipText  = QT_TR_NOOP("Revolve a selected sketch");
    sWhatsThis    = "PartDesign_Revolution";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Revolution";

    Gui::Application::Instance->commandManager().registerCallback(
            boost::bind(&commandOverride, this, 0, bp::_1, bp::_2), "Part_MakeRevolve");
}

void CmdPartDesignRevolution::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::Command* cmd = this;
    auto worker = [cmd, &pcActiveBody](Part::Feature* sketch, App::DocumentObject *Feat) {

        if (!Feat) return;

        if (sketch->isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
            Gui::cmdAppObject(Feat, std::ostringstream() <<"ReferenceAxis = (" << getObjectCmd(sketch) << ",['V_Axis'])");
        }
        else {
            Gui::cmdAppObject(Feat, std::ostringstream() <<"ReferenceAxis = (" << getObjectCmd(pcActiveBody->getOrigin()->getY()) << ",[''])");
        }

        Gui::cmdAppObject(Feat, std::ostringstream() <<"Angle = 360.0");
        PartDesign::Revolution* pcRevolution = dynamic_cast<PartDesign::Revolution*>(Feat);
        if (pcRevolution && pcRevolution->suggestReversed())
            Gui::cmdAppObject(Feat, std::ostringstream() <<"Reversed = 1");

        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, this, "Revolution", worker);
}

bool CmdPartDesignRevolution::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Groove
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignGroove)

CmdPartDesignGroove::CmdPartDesignGroove()
  : Command("PartDesign_Groove")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Groove");
    sToolTipText  = QT_TR_NOOP("Groove a selected sketch");
    sWhatsThis    = "PartDesign_Groove";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Groove";
}

void CmdPartDesignGroove::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::Command* cmd = this;
    auto worker = [cmd, &pcActiveBody](Part::Feature* sketch, App::DocumentObject *Feat) {

        if (!Feat) return;

        if (sketch->isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
            Gui::cmdAppObject(Feat, std::ostringstream() <<"ReferenceAxis = ("<<getObjectCmd(sketch)<<",['V_Axis'])");
        }
        else {
            Gui::cmdAppObject(Feat, std::ostringstream() <<"ReferenceAxis = ("<<getObjectCmd(pcActiveBody->getOrigin()->getY())<<",[''])");
        }

        Gui::cmdAppObject(Feat, std::ostringstream() <<"Angle = 360.0");

        try {
            // This raises as exception if line is perpendicular to sketch/support face.
            // Here we should continue to give the user a chance to change the default values.
            PartDesign::Groove* pcGroove = dynamic_cast<PartDesign::Groove*>(Feat);
            if (pcGroove && pcGroove->suggestReversed())
                Gui::cmdAppObject(Feat, std::ostringstream() <<"Reversed = 1");
        }
        catch (const Base::Exception& e) {
            e.ReportException();
        }

        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, this, "Groove", worker);
}

bool CmdPartDesignGroove::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_AdditivePipe
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignAdditivePipe)

CmdPartDesignAdditivePipe::CmdPartDesignAdditivePipe()
  : Command("PartDesign_AdditivePipe")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Additive pipe");
    sToolTipText  = QT_TR_NOOP("Sweep a selected sketch along a path or to other profiles");
    sWhatsThis    = "PartDesign_AdditivePipe";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_AdditivePipe";

    Gui::Application::Instance->commandManager().registerCallback(
            boost::bind(&commandOverride, this, 0, bp::_1, bp::_2), "Part_Sweep");
}

void CmdPartDesignAdditivePipe::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::Command* cmd = this;
    auto worker = [cmd](Part::Feature* sketch, App::DocumentObject *Feat) {

        if (!Feat) return;

        // specific parameters for pipe
        Gui::Command::updateActive();

        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, this, "AdditivePipe", worker);
}

bool CmdPartDesignAdditivePipe::isActive(void)
{
    return hasActiveDocument();
}


//===========================================================================
// PartDesign_SubtractivePipe
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignSubtractivePipe)

CmdPartDesignSubtractivePipe::CmdPartDesignSubtractivePipe()
  : Command("PartDesign_SubtractivePipe")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Subtractive pipe");
    sToolTipText  = QT_TR_NOOP("Sweep a selected sketch along a path or to other profiles and remove it from the body");
    sWhatsThis    = "PartDesign_SubtractivePipe";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_SubtractivePipe";
}

void CmdPartDesignSubtractivePipe::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::Command* cmd = this;
    auto worker = [cmd](Part::Feature* sketch, App::DocumentObject *Feat) {

        if (!Feat) return;

        // specific parameters for pipe
        Gui::Command::updateActive();

        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, this, "SubtractivePipe", worker);
}

bool CmdPartDesignSubtractivePipe::isActive(void)
{
    return hasActiveDocument();
}


//===========================================================================
// PartDesign_AdditiveLoft
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignAdditiveLoft)

CmdPartDesignAdditiveLoft::CmdPartDesignAdditiveLoft()
  : Command("PartDesign_AdditiveLoft")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Additive loft");
    sToolTipText  = QT_TR_NOOP("Loft a selected profile through other profile sections");
    sWhatsThis    = "PartDesign_AdditiveLoft";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_AdditiveLoft";

    Gui::Application::Instance->commandManager().registerCallback(
            boost::bind(&commandOverride, this, 0, bp::_1, bp::_2), "Part_Loft");
}

void CmdPartDesignAdditiveLoft::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::Command* cmd = this;
    auto worker = [cmd](Part::Feature* sketch, App::DocumentObject *Feat) {

        if (!Feat) return;

        // specific parameters for pipe
        Gui::Command::updateActive();

        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, this, "AdditiveLoft", worker);
}

bool CmdPartDesignAdditiveLoft::isActive(void)
{
    return hasActiveDocument();
}


//===========================================================================
// PartDesign_SubtractiveLoft
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignSubtractiveLoft)

CmdPartDesignSubtractiveLoft::CmdPartDesignSubtractiveLoft()
  : Command("PartDesign_SubtractiveLoft")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Subtractive loft");
    sToolTipText  = QT_TR_NOOP("Loft a selected profile through other profile sections and remove it from the body");
    sWhatsThis    = "PartDesign_SubtractiveLoft";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_SubtractiveLoft";
}

void CmdPartDesignSubtractiveLoft::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::Command* cmd = this;
    auto worker = [cmd](Part::Feature* sketch, App::DocumentObject *Feat) {

        if (!Feat) return;

        // specific parameters for pipe
        Gui::Command::updateActive();

        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, this, "SubtractiveLoft", worker);
}

bool CmdPartDesignSubtractiveLoft::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_AdditiveHelix
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignAdditiveHelix)

CmdPartDesignAdditiveHelix::CmdPartDesignAdditiveHelix()
  : Command("PartDesign_AdditiveHelix")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Additive helix");
    sToolTipText  = QT_TR_NOOP("Sweep a selected sketch along a helix");
    sWhatsThis    = "PartDesign_AdditiveHelix";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_AdditiveHelix";
}

void CmdPartDesignAdditiveHelix::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::Command* cmd = this;
    auto worker = [cmd, &pcActiveBody](Part::Feature* sketch, App::DocumentObject *Feat) {

        if (!Feat) return;

        // specific parameters for helix
        Gui::Command::updateActive();

        if (sketch->isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
            FCMD_OBJ_CMD(Feat,"ReferenceAxis = (" << getObjectCmd(sketch) << ",['V_Axis'])");
        }
        else {
            FCMD_OBJ_CMD(Feat,"ReferenceAxis = (" << getObjectCmd(pcActiveBody->getOrigin()->getY()) << ",[''])");
        }

        finishProfileBased(cmd, sketch, Feat);

        // If the initial helix creation fails then it leaves the base object invisible which makes things
        // more difficult for the user.
        // To avoid this the base object will be made tmp. visible again.
        if (Feat->isError()) {
            App::DocumentObject* base = static_cast<PartDesign::Feature*>(Feat)->BaseFeature.getValue();
            if (base) {
                PartDesignGui::ViewProvider* view = dynamic_cast<PartDesignGui::ViewProvider*>(Gui::Application::Instance->getViewProvider(base));
                if (view)
                    view->makeTemporaryVisible(true);
            }
        }

        cmd->adjustCameraPosition();
    };

    prepareProfileBased(pcActiveBody, this, "AdditiveHelix", worker);
}

bool CmdPartDesignAdditiveHelix::isActive(void)
{
    return hasActiveDocument();
}


//===========================================================================
// PartDesign_SubtractiveHelix
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignSubtractiveHelix)

CmdPartDesignSubtractiveHelix::CmdPartDesignSubtractiveHelix()
  : Command("PartDesign_SubtractiveHelix")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Subtractive helix");
    sToolTipText  = QT_TR_NOOP("Sweep a selected sketch along a helix and remove it from the body");
    sWhatsThis    = "PartDesign_SubtractiveHelix";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_SubtractiveHelix";
}

void CmdPartDesignSubtractiveHelix::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::Command* cmd = this;
    auto worker = [cmd, &pcActiveBody](Part::Feature* sketch, App::DocumentObject *Feat) {

        if (!Feat) return;

        // specific parameters for helix
        Gui::Command::updateActive();

        if (sketch->isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
            FCMD_OBJ_CMD(Feat,"ReferenceAxis = (" << getObjectCmd(sketch) << ",['V_Axis'])");
        }
        else {
            FCMD_OBJ_CMD(Feat,"ReferenceAxis = (" << getObjectCmd(pcActiveBody->getOrigin()->getY()) << ",[''])");
        }

        finishProfileBased(cmd, sketch, Feat);
        cmd->adjustCameraPosition();
    };

    prepareProfileBased(pcActiveBody, this, "SubtractiveHelix", worker);
}

bool CmdPartDesignSubtractiveHelix::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// Common utility functions for Dressup features
//===========================================================================

bool dressupGetSelected(Gui::Command* cmd, const std::string& which,
        Gui::SelectionObject &selected)
{
    // No PartDesign feature without Body past FreeCAD 0.16
    App::Document *doc = cmd->getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return false;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return false;

    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();

    if (selection.size() == 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select an edge, face, or body."));
        return false;
    } else if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select an edge, face, or body from a single body."));
        return false;
    }
    else if (pcActiveBody != PartDesignGui::getBodyFor(selection[0].getObject(), false)) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection is not in Active Body"),
            QObject::tr("Select an edge, face, or body from an active body."));
        return false;
    }

    // set the
    selected = selection[0];

    if (!selected.isObjectTypeOf(Part::Feature::getClassTypeId())) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong object type"),
            QObject::tr("%1 works only on parts.").arg(QString::fromStdString(which)));
        return false;
    }

    Part::Feature *base = static_cast<Part::Feature*>(selected.getObject());

    const Part::TopoShape& TopShape = base->Shape.getShape();

    if (TopShape.getShape().IsNull()){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Shape of the selected Part is empty"));
        return false;
    }

    return true;
}

void finishDressupFeature(const Gui::Command* cmd, const std::string& which,
        Part::Feature *base, const std::vector<std::string> & SubNames)
{
    if (SubNames.size() == 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QString::fromStdString(which) + QObject::tr(" not possible on selected faces/edges."));
        return;
    }

    std::ostringstream str;
    str << '(' << Gui::Command::getObjectCmd(base) << ",[";
    for (std::vector<std::string>::const_iterator it = SubNames.begin();it!=SubNames.end();++it){
        str << "'" << *it << "',";
    }
    str << "])";

    std::string FeatName = cmd->getUniqueObjectName(which.c_str(),base);

    auto body = PartDesignGui::getBodyFor(base,false);
    if (!body) return;
    cmd->openCommand((std::string("Make ") + which).c_str());
    Gui::cmdAppObject(body, std::ostringstream()
            << "newObjectAt('PartDesign::" << which << "','" << FeatName << "', "
                        <<  "FreeCADGui.Selection.getSelection())");
    auto Feat = body->getDocument()->getObject(FeatName.c_str());
    Gui::cmdAppObject(Feat, std::ostringstream() << "Base = " << str.str());
    cmd->doCommand(cmd->Gui,"Gui.Selection.clearSelection()");
    finishFeature(cmd, Feat, base);

    App::DocumentObject* baseFeature = static_cast<PartDesign::DressUp*>(Feat)->Base.getValue();
    if (baseFeature) {
        PartDesignGui::ViewProvider* view = dynamic_cast<PartDesignGui::ViewProvider*>(Gui::Application::Instance->getViewProvider(baseFeature));
        // in case there is an error, for example when a fillet is larger than the available space
        // display the base feature to avoid that the user sees nothing
        if (view && Feat->isError())
            view->Visibility.setValue(true);
    }
}

void makeChamferOrFillet(Gui::Command* cmd, const std::string& which)
{
    Gui::SelectionObject selected;
    if (!dressupGetSelected ( cmd, which, selected))
        return;

    Part::Feature *base = static_cast<Part::Feature*>(selected.getObject());

    std::vector<std::string> SubNames = std::vector<std::string>(selected.getSubNames());

    finishDressupFeature (cmd, which, base, SubNames);
}

//===========================================================================
// PartDesign_Fillet
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignFillet)

CmdPartDesignFillet::CmdPartDesignFillet()
  :Command("PartDesign_Fillet")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Fillet");
    sToolTipText  = QT_TR_NOOP("Make a fillet on an edge, face or body");
    sWhatsThis    = "PartDesign_Fillet";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Fillet";

    Gui::Application::Instance->commandManager().registerCallback(
            boost::bind(&commandOverride, this, 0, bp::_1, bp::_2), "Part_Fillet");
}

void CmdPartDesignFillet::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    makeChamferOrFillet(this, "Fillet");
}

bool CmdPartDesignFillet::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Chamfer
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignChamfer)

CmdPartDesignChamfer::CmdPartDesignChamfer()
  :Command("PartDesign_Chamfer")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Chamfer");
    sToolTipText  = QT_TR_NOOP("Chamfer the selected edges of a shape");
    sWhatsThis    = "PartDesign_Chamfer";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Chamfer";

    Gui::Application::Instance->commandManager().registerCallback(
            boost::bind(&commandOverride, this, 0, bp::_1, bp::_2), "Part_Chamfer");
}

void CmdPartDesignChamfer::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    makeChamferOrFillet(this, "Chamfer");
    doCommand(Gui,"Gui.Selection.clearSelection()");
}

bool CmdPartDesignChamfer::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Draft
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignDraft)

CmdPartDesignDraft::CmdPartDesignDraft()
  :Command("PartDesign_Draft")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Draft");
    sToolTipText  = QT_TR_NOOP("Make a draft on a face");
    sWhatsThis    = "PartDesign_Draft";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Draft";
}

void CmdPartDesignDraft::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::SelectionObject selected;
    if (!dressupGetSelected ( this, "Draft", selected))
        return;

    Part::Feature *base = static_cast<Part::Feature*>(selected.getObject());
    std::vector<std::string> SubNames = std::vector<std::string>(selected.getSubNames());
    const Part::TopoShape& TopShape = base->Shape.getShape();
    size_t i = 0;

    // filter out the edges
    while(i < SubNames.size())
    {
        std::string aSubName = static_cast<std::string>(SubNames.at(i));

        if (aSubName.size() > 4 && aSubName.substr(0,4) == "Face") {
            // Check for valid face types
            TopoDS_Face face = TopoDS::Face(TopShape.getSubShape(aSubName.c_str()));
            BRepAdaptor_Surface sf(face);
            if ((sf.GetType() != GeomAbs_Plane) && (sf.GetType() != GeomAbs_Cylinder) && (sf.GetType() != GeomAbs_Cone))
                SubNames.erase(SubNames.begin()+i);
        } else {
            // empty name or any other sub-element
            SubNames.erase(SubNames.begin()+i);
        }

        i++;
    }

    finishDressupFeature (this, "Draft", base, SubNames);
}

bool CmdPartDesignDraft::isActive(void)
{
    return hasActiveDocument();
}


//===========================================================================
// PartDesign_Thickness
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignThickness)

CmdPartDesignThickness::CmdPartDesignThickness()
  :Command("PartDesign_Thickness")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Thickness");
    sToolTipText  = QT_TR_NOOP("Make a thick solid");
    sWhatsThis    = "PartDesign_Thickness";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Thickness";

    Gui::Application::Instance->commandManager().registerCallback(
            boost::bind(&commandOverride, this, 0, bp::_1, bp::_2), "Part_Thickness");
}

void CmdPartDesignThickness::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::SelectionObject selected;
    if (!dressupGetSelected ( this, "Thickness", selected))
        return;

    Part::Feature *base = static_cast<Part::Feature*>(selected.getObject());
    std::vector<std::string> SubNames = std::vector<std::string>(selected.getSubNames());
    size_t i = 0;

    // filter out the edges
    while(i < SubNames.size())
    {
        std::string aSubName = static_cast<std::string>(SubNames.at(i));

        if (aSubName.size() > 4 && aSubName.substr(0,4) != "Face") {
            // empty name or any other sub-element
            SubNames.erase(SubNames.begin()+i);
        }
        i++;
    }

    finishDressupFeature (this, "Thickness", base, SubNames);
}

bool CmdPartDesignThickness::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// Common functions for all Transformed features
//===========================================================================

template<class F>
void prepareTransformed(PartDesign::Body *pcActiveBody,
                        Gui::Command* cmd,
                        const std::string& which, F func)
{
    std::string FeatName = cmd->getUniqueObjectName(which.c_str(), pcActiveBody);

    auto worker = [=](const std::vector<App::DocumentObject*> & features) {
        std::stringstream str;
        str << cmd->getObjectCmd(FeatName.c_str(), pcActiveBody->getDocument()) << ".Originals = [";
        for (auto obj : features) {
            str << cmd->getObjectCmd(obj) << ",";
        }
        str << "]";

        std::string msg("Make ");
        msg += which;
        Gui::Command::openCommand(msg.c_str());
        Gui::cmdAppObject(pcActiveBody, std::ostringstream()
                << "newObjectAt('PartDesign::" << which << "','" << FeatName << "', "
                            <<  "FreeCADGui.Selection.getSelection())");
#if 0
        // FIXME: There seems to be kind of a race condition here, leading to sporadic errors like
        // Exception (Thu Sep  6 11:52:01 2012): 'App.Document' object has no attribute 'Mirrored'
        Gui::Command::updateActive(); // Helps to ensure that the object already exists when the next command comes up
#endif
        Gui::Command::doCommand(Gui::Command::Doc, str.str().c_str());

        auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());
        if (Feat) {
            func(Feat, features);
            Gui::Command::updateActive();
        }
    };

    PartDesign::Body* activeBody = PartDesignGui::getBody(true);

    std::vector<App::DocumentObject*> features;

    // We now allow no selection for transformed feature, in which case the tip
    // will be used for transformation.
    for (auto & sel : Gui::Selection().getSelectionT()) {
        auto obj = sel.getObject();
        if (!obj)
            continue;
        if (!obj->isDerivedFrom(PartDesign::Feature::getClassTypeId())
                || activeBody != PartDesign::Body::findBodyOf(obj))
        {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection is not in Active Body"),
                QObject::tr("Please select only feature(s) in the active body."));
            return;
        }
        features.push_back(obj);
    }
    worker(features);
}

void finishTransformed(Gui::Command* cmd, App::DocumentObject *Feat)
{
    finishFeature(cmd, Feat);
}

//===========================================================================
// PartDesign_Mirrored
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignMirrored)

CmdPartDesignMirrored::CmdPartDesignMirrored()
  : Command("PartDesign_Mirrored")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Mirrored");
    sToolTipText  = QT_TR_NOOP("Create a mirrored feature");
    sWhatsThis    = "PartDesign_Mirrored";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Mirrored";

    Gui::Application::Instance->commandManager().registerCallback(
            boost::bind(&commandOverride, this, 0, bp::_1, bp::_2), "Part_Mirror");
}

void CmdPartDesignMirrored::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // No PartDesign feature without Body past FreeCAD 0.16
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::Command* cmd = this;
    auto worker = [pcActiveBody, cmd](App::DocumentObject *Feat,
                                      const std::vector<App::DocumentObject*> &features)
    {
        bool direction = false;
        if (features.size()
                && features.front()->isDerivedFrom(PartDesign::ProfileBased::getClassTypeId()))
        {
            Part::Part2DObject *sketch = (static_cast<PartDesign::ProfileBased*>(features.front()))->getVerifiedSketch(/* silent =*/ true);
            if (sketch) {
                Gui::cmdAppObject(Feat, std::ostringstream()
                        <<"MirrorPlane = ("<<getObjectCmd(sketch)<<", ['V_Axis'])");
                direction = true;
            }
        }
        if (!direction)
            Gui::cmdAppObject(Feat, std::ostringstream() <<"MirrorPlane = ("
                    << getObjectCmd(pcActiveBody->getOrigin()->getXY()) <<", [''])");

        finishTransformed(cmd, Feat);
    };

    prepareTransformed(pcActiveBody, this, "Mirrored", worker);
}

bool CmdPartDesignMirrored::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_LinearPattern
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignLinearPattern)

CmdPartDesignLinearPattern::CmdPartDesignLinearPattern()
  : Command("PartDesign_LinearPattern")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("LinearPattern");
    sToolTipText  = QT_TR_NOOP("Create a linear pattern feature");
    sWhatsThis    = "PartDesign_LinearPattern";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_LinearPattern";
}

void CmdPartDesignLinearPattern::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // No PartDesign feature without Body past FreeCAD 0.16
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::Command* cmd = this;
    auto worker = [pcActiveBody, cmd](App::DocumentObject *Feat,
                                      const std::vector<App::DocumentObject*> &features)
    {
        bool direction = false;
        if (features.size()
                && features.front()->isDerivedFrom(PartDesign::ProfileBased::getClassTypeId()))
        {
            Part::Part2DObject *sketch = (static_cast<PartDesign::ProfileBased*>(features.front()))->getVerifiedSketch(/* silent =*/ true);
            if (sketch) {
                Gui::cmdAppObject(Feat, std::ostringstream()
                        <<"Direction = ("<<Gui::Command::getObjectCmd(sketch)<<", ['H_Axis'])");
                direction = true;
            }
        }
        if (!direction)
            Gui::cmdAppObject(Feat, std::ostringstream() <<"Direction = ("
                    << Gui::Command::getObjectCmd(pcActiveBody->getOrigin()->getX())<<",[''])");

        Gui::cmdAppObject(Feat, std::ostringstream() <<"Length = 100");
        Gui::cmdAppObject(Feat, std::ostringstream() <<"Occurrences = 2");

        finishTransformed(cmd, Feat);
    };

    prepareTransformed(pcActiveBody, this, "LinearPattern", worker);
}

bool CmdPartDesignLinearPattern::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_GenericPattern
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignGenericPattern)

CmdPartDesignGenericPattern::CmdPartDesignGenericPattern()
  : Command("PartDesign_GenericPattern")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("GenericPattern");
    sToolTipText  = QT_TR_NOOP("Create a generic pattern feature");
    sWhatsThis    = "PartDesign_GenericPattern";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_GenericPattern";
}

void CmdPartDesignGenericPattern::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // No PartDesign feature without Body past FreeCAD 0.16
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::Command* cmd = this;
    auto worker = [pcActiveBody, cmd](App::DocumentObject *Feat,
                                      const std::vector<App::DocumentObject*> &features)
    {
        (void)features;
        finishTransformed(cmd, Feat);
    };

    prepareTransformed(pcActiveBody, this, "GenericPattern", worker);
}

bool CmdPartDesignGenericPattern::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_PolarPattern
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignPolarPattern)

CmdPartDesignPolarPattern::CmdPartDesignPolarPattern()
  : Command("PartDesign_PolarPattern")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("PolarPattern");
    sToolTipText  = QT_TR_NOOP("Create a polar pattern feature");
    sWhatsThis    = "PartDesign_PolarPattern";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_PolarPattern";
}

void CmdPartDesignPolarPattern::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // No PartDesign feature without Body past FreeCAD 0.16
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::Command* cmd = this;
    auto worker = [pcActiveBody, cmd](App::DocumentObject *Feat,
                                      const std::vector<App::DocumentObject*> &features)
    {
        bool direction = false;
        if (features.size()
                && features.front()->isDerivedFrom(PartDesign::ProfileBased::getClassTypeId())) {
            Part::Part2DObject *sketch = (static_cast<PartDesign::ProfileBased*>(features.front()))->getVerifiedSketch(/* silent =*/ true);
            if (sketch) {
                Gui::cmdAppObject(Feat, std::ostringstream() <<"Axis = ("<<Gui::Command::getObjectCmd(sketch)<<",['N_Axis'])");
                direction = true;
            }
        }
        if (!direction)
            Gui::cmdAppObject(Feat, std::ostringstream() <<"Axis = ("
                    << Gui::Command::getObjectCmd(pcActiveBody->getOrigin()->getZ())<<",[''])");

        Gui::cmdAppObject(Feat, std::ostringstream() <<"Angle = 360");
        Gui::cmdAppObject(Feat, std::ostringstream() <<"Occurrences = 2");

        finishTransformed(cmd, Feat);
    };

    prepareTransformed(pcActiveBody, this, "PolarPattern", worker);
}

bool CmdPartDesignPolarPattern::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Scaled
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignScaled)

CmdPartDesignScaled::CmdPartDesignScaled()
  : Command("PartDesign_Scaled")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Scaled");
    sToolTipText  = QT_TR_NOOP("Create a scaled feature");
    sWhatsThis    = "PartDesign_Scaled";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Scaled";
}

void CmdPartDesignScaled::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::Command* cmd = this;
    auto worker = [pcActiveBody, cmd](App::DocumentObject *Feat,
                                      const std::vector<App::DocumentObject*> &features)
    {
        if (!Feat || features.empty())
            return;

        Gui::cmdAppObject(Feat, std::ostringstream() <<"Factor = 2");
        Gui::cmdAppObject(Feat, std::ostringstream() <<"Occurrences = 2");

        finishTransformed(cmd, Feat);
    };

    prepareTransformed(pcActiveBody, this, "Scaled", worker);
}

bool CmdPartDesignScaled::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_MultiTransform
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignMultiTransform)

CmdPartDesignMultiTransform::CmdPartDesignMultiTransform()
  : Command("PartDesign_MultiTransform")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Create MultiTransform");
    sToolTipText  = QT_TR_NOOP("Create a multitransform feature");
    sWhatsThis    = "PartDesign_MultiTransform";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_MultiTransform";
}

void CmdPartDesignMultiTransform::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // No PartDesign feature without Body past FreeCAD 0.16
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    PartDesign::Transformed * trFeat = nullptr;

    // Check if a Transformed feature has been selected, convert it to MultiTransform
    for (auto &selT : Gui::Selection().getSelectionT()) {
        auto obj = selT.getObject();
        if (PartDesign::Body::findBodyOf(obj) != pcActiveBody) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection is not in Active Body"),
                QObject::tr("Please select only one feature in the active body."));
            return;
        }

        if (obj->isDerivedFrom(PartDesign::MultiTransform::getClassTypeId())) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Invalid MultiTransform selection"),
                QObject::tr("MultiTransform feature cannot be nested."));
            return;
        }

        if (obj->isDerivedFrom(PartDesign::Transformed::getClassTypeId())) {
            if (trFeat) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Invalid MultiTransform selection"),
                    QObject::tr("Please select one and only one pattern feature."));
                return;
            }
            trFeat = static_cast<PartDesign::Transformed*>(obj);
        }
    }

    if (trFeat) {

        // Create a MultiTransform feature and move the Transformed feature inside it
        std::string FeatName = getUniqueObjectName("MultiTransform",pcActiveBody);
        std::string baseFeature = getObjectCmd(trFeat->BaseFeature.getValue());
        Gui::cmdAppObject(pcActiveBody, std::ostringstream()
                << "newObjectAt('PartDesign::MultiTransform','"
                << FeatName << "', " << baseFeature  << ")");

        auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());

        if (pcActiveBody->Tip.getValue() == trFeat)
            pcActiveBody->setTip(Feat);

        auto objCmd = getObjectCmd(trFeat);

        Gui::cmdAppObject(Feat, std::ostringstream() <<"OriginalSubs = "<<objCmd<<".OriginalSubs");
        Gui::cmdAppObject(trFeat, std::ostringstream() <<"BaseFeature = None");
        Gui::cmdAppObject(trFeat, std::ostringstream() <<"OriginalSubs = []");
        Gui::cmdAppObject(Feat, std::ostringstream() <<"Transformations = ["<<objCmd<<"]");

        finishFeature(this, Feat);

    } else {
        Gui::Command* cmd = this;
        auto worker = [cmd, pcActiveBody](App::DocumentObject *Feat,
                                          const std::vector<App::DocumentObject*> & features)
        {
            (void)features;
            if (!Feat)
                return;

            // Make sure the user isn't presented with an empty screen because no transformations are defined yet...
            App::DocumentObject* prevSolid = pcActiveBody->Tip.getValue();
            if (prevSolid != NULL) {
                Part::Feature* feat = static_cast<Part::Feature*>(prevSolid);
                FCMD_OBJ_CMD(Feat,"Shape = "<<getObjectCmd(feat)<<".Shape");
            }
            finishFeature(cmd, Feat);
        };

        prepareTransformed(pcActiveBody, this, "MultiTransform", worker);
    }
}

bool CmdPartDesignMultiTransform::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Boolean
//===========================================================================

/* Boolean commands =======================================================*/
DEF_STD_CMD_A(CmdPartDesignBoolean)

CmdPartDesignBoolean::CmdPartDesignBoolean()
  :Command("PartDesign_Boolean")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Boolean operation");
    sToolTipText    = QT_TR_NOOP("Boolean operation with two or more bodies");
    sWhatsThis      = "PartDesign_Boolean";
    sStatusTip      = sToolTipText;
    sPixmap         = "PartDesign_Boolean";

    Gui::Application::Instance->commandManager().registerCallback(
            boost::bind(&commandOverride, this, 0, bp::_1, bp::_2), "Part_Boolean");

    Gui::Application::Instance->commandManager().registerCallback(
            boost::bind(&commandOverride, this, 1, bp::_1, bp::_2), "Part_Cut");

    Gui::Application::Instance->commandManager().registerCallback(
            boost::bind(&commandOverride, this, 2, bp::_1, bp::_2), "Part_Common");
}


void CmdPartDesignBoolean::activated(int iMsg)
{
    std::string bodySub;
    App::DocumentObject *bodyParent = nullptr;
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true,true,true,&bodyParent,&bodySub);
    if (!pcActiveBody) return;

    auto inList = pcActiveBody->getInListEx(true);
    inList.insert(pcActiveBody);

    std::string support;
    std::set<App::DocumentObject*> objSet;
    std::vector<App::DocumentObject*> objs;
    std::map<std::pair<App::DocumentObject*,std::string>, std::vector<std::string> > binderLinks;

    for(auto &sel : Gui::Selection().getSelectionT("*",0)) {
        auto obj = sel.getSubObject();
        if(!obj || inList.count(obj))
            continue;

        App::DocumentObject *link;
        std::string linkSub;

        if (PartDesign::Body::findBodyOf(obj) == pcActiveBody) {
            link = obj;
            linkSub = sel.getOldElementName();
        }
        else {
            link = sel.getObject();
            sel.getSubName();
            if(bodyParent && bodyParent != pcActiveBody) {
                std::string sub = bodySub;
                bodyParent->resolveRelativeLink(sub,link,linkSub);
            }

            if(!link || link == pcActiveBody)
                continue;
        }

        const char *element = Data::ComplexGeoData::findElementName(linkSub.c_str());
        if(!element || !element[0]) {
            binderLinks[std::make_pair(link,linkSub)];
            continue;
        }

        linkSub.resize(element-linkSub.c_str());
        auto &elements = binderLinks[std::make_pair(link,linkSub)];

        // Try to convert the current selection to solid selection
        auto shape = Part::Feature::getTopoShape(link,linkSub.c_str());
        // If more than one solids, convert the reference to actual solid indexed name
        if(shape.countSubShapes(TopAbs_SOLID) > 1) {
            auto subshape = shape.getSubTopoShape(element);
            for(auto face : subshape.getSubShapes(TopAbs_FACE)) {
                for(int idx : shape.findAncestors(face, TopAbs_SOLID))
                    elements.push_back(std::string("Solid") + std::to_string(idx));
            }
        }
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Create Boolean"));
    std::string FeatName = getUniqueObjectName("Boolean",pcActiveBody);
    Gui::cmdAppObject(pcActiveBody, std::ostringstream()
            << "newObjectAt('PartDesign::Boolean','" << FeatName << "', "
                        <<  "FreeCADGui.Selection.getSelection())");
    auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());

    switch(iMsg) {
    case 1:
        Gui::cmdAppObject(Feat, "Type = 'Cut'");
        break;
    case 2:
        Gui::cmdAppObject(Feat, "Type = 'Common'");
        break;
    }

    for(auto &v : binderLinks) {
        std::string FeatName = getUniqueObjectName("Reference",pcActiveBody);
        Gui::cmdAppObject(pcActiveBody, std::ostringstream()
                << "newObject('PartDesign::SubShapeBinder','" << FeatName << "')");
        auto binder = Base::freecad_dynamic_cast<PartDesign::SubShapeBinder>(
                        pcActiveBody->getObject(FeatName.c_str()));
        if(!binder)
            continue;

        std::map<App::DocumentObject*, std::vector<std::string> > links;
        auto &subs = links[v.first.first];
        if(v.second.empty())
            v.second.push_back("");
        for(auto &s : v.second)
            subs.push_back(v.first.second + s);
        binder->setLinks(std::move(links));
        objs.push_back(binder);
    }

    // If we don't add an object to the boolean group then don't update the body
    // as otherwise this will fail and it will be marked as invalid
    bool updateDocument = false;
    if (objs.size()) {
        updateDocument = true;
        std::string bodyString = PartDesignGui::buildLinkListPythonStr(objs);
        Gui::cmdAppObject(Feat, std::ostringstream() <<"addObjects("<<bodyString<<")");
    }

    finishFeature(this, Feat, nullptr, false, updateDocument);
}

bool CmdPartDesignBoolean::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// PartDesign_Split
//===========================================================================

/* Split commands =======================================================*/
DEF_STD_CMD_A(CmdPartDesignSplit)

CmdPartDesignSplit::CmdPartDesignSplit()
  :Command("PartDesign_Split")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Split operation");
    sToolTipText    = QT_TR_NOOP("Split the previous feature into multiple solids");
    sWhatsThis      = "PartDesign_Split";
    sStatusTip      = sToolTipText;
    sPixmap         = "PartDesign_Split";

    Gui::Application::Instance->commandManager().registerCallback(
            boost::bind(&commandOverride, this, 1, bp::_1, bp::_2), "Part_BooleanFragments");

    Gui::Application::Instance->commandManager().registerCallback(
            boost::bind(&commandOverride, this, 0, bp::_1, bp::_2), "Part_Slice");

    Gui::Application::Instance->commandManager().registerCallback(
            boost::bind(&commandOverride, this, 0, bp::_1, bp::_2), "Part_SliceApart");

    Gui::Application::Instance->commandManager().registerCallback(
            boost::bind(&commandOverride, this, 0, bp::_1, bp::_2), "Part_ExplodeCompound");
}

void CmdPartDesignSplit::activated(int iMsg)
{
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(/*messageIfNot = */true);
    if (!pcActiveBody) return;

    openCommand("Create Split");
    std::string FeatName = getUniqueObjectName("Split",pcActiveBody);
    Gui::cmdAppObject(pcActiveBody, std::ostringstream()
            << "newObjectAt('PartDesign::Split','" << FeatName << "', "
                        <<  "FreeCADGui.Selection.getSelection())");
    auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());

    switch(iMsg) {
    case 1:
        Gui::cmdAppObject(Feat, "Fregment = True");
        break;
    }

    Gui::cmdAppObject(Feat, std::ostringstream() <<"Tools = FreeCADGui.Selection.getSelection()");
    for(auto tool : static_cast<PartDesign::Split*>(Feat)->Tools.getValues())
        Gui::cmdAppObject(tool, std::ostringstream() <<"Visibility = False");
    finishFeature(this, Feat, nullptr, false, true);
}

bool CmdPartDesignSplit::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}


//===========================================================================
// Initialization
//===========================================================================

void CreatePartDesignCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdPartDesignShapeBinder());
    rcCmdMgr.addCommand(new CmdPartDesignSubShapeBinder());
    rcCmdMgr.addCommand(new CmdPartDesignClone());
    rcCmdMgr.addCommand(new CmdPartDesignPlane());
    rcCmdMgr.addCommand(new CmdPartDesignLine());
    rcCmdMgr.addCommand(new CmdPartDesignPoint());
    rcCmdMgr.addCommand(new CmdPartDesignCS());

    rcCmdMgr.addCommand(new CmdPartDesignNewSketch());

    rcCmdMgr.addCommand(new CmdPartDesignPad());
    rcCmdMgr.addCommand(new CmdPartDesignExtrusion());
    rcCmdMgr.addCommand(new CmdPartDesignPocket());
    rcCmdMgr.addCommand(new CmdPartDesignHole());
    rcCmdMgr.addCommand(new CmdPartDesignRevolution());
    rcCmdMgr.addCommand(new CmdPartDesignGroove());
    rcCmdMgr.addCommand(new CmdPartDesignAdditivePipe);
    rcCmdMgr.addCommand(new CmdPartDesignSubtractivePipe);
    rcCmdMgr.addCommand(new CmdPartDesignAdditiveLoft);
    rcCmdMgr.addCommand(new CmdPartDesignSubtractiveLoft);
    rcCmdMgr.addCommand(new CmdPartDesignAdditiveHelix);
    rcCmdMgr.addCommand(new CmdPartDesignSubtractiveHelix);

    rcCmdMgr.addCommand(new CmdPartDesignFillet());
    rcCmdMgr.addCommand(new CmdPartDesignDraft());
    rcCmdMgr.addCommand(new CmdPartDesignChamfer());
    rcCmdMgr.addCommand(new CmdPartDesignThickness());

    rcCmdMgr.addCommand(new CmdPartDesignMirrored());
    rcCmdMgr.addCommand(new CmdPartDesignLinearPattern());
    rcCmdMgr.addCommand(new CmdPartDesignPolarPattern());
    //rcCmdMgr.addCommand(new CmdPartDesignScaled());
    rcCmdMgr.addCommand(new CmdPartDesignGenericPattern());
    rcCmdMgr.addCommand(new CmdPartDesignMultiTransform());

    rcCmdMgr.addCommand(new CmdPartDesignBoolean());
    rcCmdMgr.addCommand(new CmdPartDesignSplit());
}
