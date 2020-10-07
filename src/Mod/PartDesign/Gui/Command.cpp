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
#include <Mod/PartDesign/App/ShapeBinder.h>

#include "TaskFeaturePick.h"
#include "ReferenceSelection.h"
#include "Utils.h"
#include "WorkflowManager.h"
#include "ViewProviderBody.h"

// TODO Remove this header after fixing code so it won;t be needed here (2015-10-20, Fat-Zer)
#include "ui_DlgReference.h"

FC_LOG_LEVEL_INIT("PartDesign",true,true)

using namespace std;
using namespace Attacher;


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
        std::string fullTypeName (type.getName());

        App::PropertyLinkSubList support;
        cmd.getSelection().getAsPropertyLinkSubList(support);

        bool bEditSelected = false;
        if (support.getSize() == 1 && support.getValue() ) {
            if (support.getValue()->isDerivedFrom(type))
                bEditSelected = true;
        }

        PartDesign::Body *pcActiveBody = PartDesignGui::getBody(/*messageIfNot = */false);

        if (bEditSelected) {
            std::string tmp = std::string("Edit ")+name;
            cmd.openCommand(tmp.c_str());
            PartDesignGui::setEdit(support.getValue(),pcActiveBody);
            return;
        } 

        App::DocumentObject *pcActiveContainer;
        if(pcActiveBody)
            pcActiveContainer = pcActiveBody;
        else
            pcActiveContainer = PartDesignGui::getActivePart();
        
        std::string FeatName;
        App::Document *doc = 0;

        FeatName = cmd.getUniqueObjectName(name.c_str(), pcActiveContainer);

        std::string tmp = std::string("Create ")+name;

        cmd.openCommand(tmp.c_str());

        if(pcActiveContainer) {
            Gui::cmdAppObject(pcActiveContainer, std::ostringstream()
                    << "newObject('" << fullTypeName << "','" << FeatName << "')");
            doc = pcActiveContainer->getDocument();
        } else {
            doc = App::GetApplication().getActiveDocument();
            Gui::cmdAppDocument(doc, std::ostringstream()
                    << "addObject('" << fullTypeName << "','" << FeatName << "')");
        }

        support.removeValue(pcActiveContainer);

        auto Feat = doc->getObject(FeatName.c_str());
        if(!Feat) return;

        //test if current selection fits a mode.
        if (support.getSize() > 0) {
            Part::AttachExtension* pcDatum = Feat->getExtensionByType<Part::AttachExtension>();
            pcDatum->attacher().setReferences(support);
            SuggestResult sugr;
            pcDatum->attacher().suggestMapModes(sugr);
            if (sugr.message == Attacher::SuggestResult::srOK) {
                //fits some mode. Populate support property.
                Gui::cmdAppObject(Feat, std::ostringstream() << "Support = " << support.getPyReprString());
                Gui::cmdAppObject(Feat, std::ostringstream() << "MapMode = '" << AttachEngine::getModeName(sugr.bestFitMode) << "'");
            } else {
                QMessageBox::information(Gui::getMainWindow(),QObject::tr("Invalid selection"), QObject::tr("There are no attachment modes that fit selected objects. Select something else."));
            }
        }
        cmd.doCommand(Gui::Command::Doc,"App.activeDocument().recompute()");  // recompute the feature based on its references

        if(pcActiveContainer)
            PartDesignGui::setEdit(Feat,pcActiveContainer,pcActiveBody?PDBODYKEY:PARTKEY);

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
        openCommand("Edit ShapeBinder");
        PartDesignGui::setEdit(support.getValue());
    } else {
        PartDesign::Body *pcActiveBody = PartDesignGui::getBody(/*messageIfNot = */true);
        if (pcActiveBody == 0)
            return;

        std::string FeatName = getUniqueObjectName("ShapeBinder",pcActiveBody);

        openCommand("Create ShapeBinder");
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
}

void CmdPartDesignSubShapeBinder::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    App::DocumentObject *parent = 0;
    std::string parentSub;
    std::map<App::DocumentObject *, std::vector<std::string> > values;
    for (auto &sel : Gui::Selection().getCompleteSelection(0)) {
        if (!sel.pObject) continue;
        auto &subs = values[sel.pObject];
        if (sel.SubName && sel.SubName[0])
            subs.emplace_back(sel.SubName);
    }

    std::string FeatName;
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(false,true,true,&parent,&parentSub);
    App::Part *pcActivePart = nullptr;
    if (!pcActiveBody)
        pcActivePart = PartDesignGui::getActivePart (&parent, &parentSub);
    FeatName = getUniqueObjectName("Binder", pcActiveBody ?
            static_cast<App::DocumentObject*>(pcActiveBody) : pcActivePart);
    App::SubObjectT objT(parent,parentSub.c_str());
    if (parent) {
        decltype(values) links;
        for (auto &v : values) {
            App::DocumentObject *obj = v.first;
            if (obj != parent) {
                auto &subs = links[obj];
                subs.insert(subs.end(),v.second.begin(),v.second.end());
                continue;
            }
            for (auto &sub : v.second) {
                auto link = obj;
                auto linkSub = parentSub;
                parent->resolveRelativeLink(linkSub,link,sub);
                if (link && link != pcActiveBody)
                    links[link].push_back(sub);
            }
        }
        values = std::move(links);
    }
        
    PartDesign::SubShapeBinder *binder = 0;
    try {
        openCommand("Create SubShapeBinder");
        if (pcActiveBody) {
            Gui::cmdAppObject(pcActiveBody, std::ostringstream()
                    << "newObject('PartDesign::SubShapeBinder', '" << FeatName << "')");
            binder = dynamic_cast<PartDesign::SubShapeBinder*>(pcActiveBody->getObject(FeatName.c_str()));
        } else {
            doCommand(Command::Doc,
                    "App.ActiveDocument.addObject('PartDesign::SubShapeBinder','%s')",FeatName.c_str());
            binder = dynamic_cast<PartDesign::SubShapeBinder*>(
                    App::GetApplication().getActiveDocument()->getObject(FeatName.c_str()));
            if (pcActivePart)
                Gui::cmdAppObject(pcActivePart, std::ostringstream()
                        << "addObject(" << getObjectCmd(binder) << ")");
        }
        if (!binder) return;
        binder->setLinks(std::move(values));
        updateActive();
        commitCommand();

        Gui::Selection().selStackPush();
        Gui::Selection().clearCompleteSelection();
        if (objT.getObject()) {
            objT.setSubName(objT.getSubName() + FeatName + ".");
            if (objT.getSubObject()) {
                Gui::Selection().addSelection(objT.getDocumentName().c_str(),
                                              objT.getObjectName().c_str(),
                                              objT.getSubName().c_str());
                return;
            }
        }
        Gui::Selection().addSelection(binder->getDocument()->getName(),
                                      binder->getNameInDocument());
        Gui::Selection().selStackPush();
    }catch(Base::Exception &e) {
        e.ReportException();
        QMessageBox::critical(Gui::getMainWindow(), 
                QObject::tr("Sub-Shape Binder"), QString::fromUtf8(e.what()));
        abortCommand();
    }
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
        openCommand("Create Clone");
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

    if ( PartDesignGui::assureModernWorkflow( doc ) ) {
        // We need either an active Body, or for there to be no Body
        // objects (in which case, just make one) to make a new sketch.

        pcActiveBody = PartDesignGui::getBody( /* messageIfNot = */ false );
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

    // Hint:
    // The behaviour of this command has changed with respect to a selected sketch:
    // It doesn't try any more to edit a selected sketch but always tries to create
    // a new sketch.
    // See https://forum.freecadweb.org/viewtopic.php?f=3&t=44070

    Gui::SelectionFilter FaceFilter  ("SELECT Part::Feature SUBELEMENT Face COUNT 1");
    Gui::SelectionFilter PlaneFilter ("SELECT App::Plane COUNT 1");
    Gui::SelectionFilter PlaneFilter2("SELECT PartDesign::Plane COUNT 1");

    if (PlaneFilter2.match())
        PlaneFilter = PlaneFilter2;

    if ( FaceFilter.match() || PlaneFilter.match() ) {
        if (!pcActiveBody) {
            // We shouldn't make a new Body in this case, because that means
            // the source shape of the face/plane would be outside the Body.
            PartDesignGui::getBody( /* messageIfNot = */ true );
            return;
        }

        // get the selected object
        std::string supportString;
        App::DocumentObject* obj;

        if (FaceFilter.match()) {
            Gui::SelectionObject faceSelObject = FaceFilter.Result[0][0];
            const std::vector<std::string>& subNames = faceSelObject.getSubNames();
            obj = faceSelObject.getObject();

            if (!obj->isDerivedFrom(Part::Feature::getClassTypeId()))
                return;

            // In case the selected face belongs to the body then it means its
            // Display Mode Body is set to Tip. But the body face is not allowed
            // to be used as support because otherwise it would cause a cyclic
            // dependency. So, instead we use the tip object as reference.
            // https://forum.freecadweb.org/viewtopic.php?f=3&t=37448
            if (obj == pcActiveBody) {
                App::DocumentObject* tip = pcActiveBody->Tip.getValue();
                if (tip && tip->isDerivedFrom(Part::Feature::getClassTypeId()) && subNames.size() == 1) {
                    Gui::SelectionChanges msg;
                    msg.pDocName = faceSelObject.getDocName();
                    msg.pObjectName = tip->getNameInDocument();
                    msg.pSubName = subNames[0].c_str();
                    msg.pTypeName = tip->getTypeId().getName();

                    faceSelObject = Gui::SelectionObject(msg);
                    obj = tip;

                    // automatically switch to 'Through' mode
                    PartDesignGui::ViewProviderBody* vpBody = dynamic_cast<PartDesignGui::ViewProviderBody*>
                            (Gui::Application::Instance->getViewProvider(pcActiveBody));
                    if (vpBody) {
                        vpBody->DisplayModeBody.setValue("Through");
                    }
                }
            }

            Part::Feature* feat = static_cast<Part::Feature*>(obj);

            if (subNames.size() > 1) {
                // No assert for wrong user input!
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Several sub-elements selected"),
                    QObject::tr("You have to select a single face as support for a sketch!"));
                return;
            }

            // get the selected sub shape (a Face)
            const Part::TopoShape &shape = feat->Shape.getValue();
            TopoDS_Shape sh = shape.getSubShape(subNames[0].c_str());
            const TopoDS_Face& face = TopoDS::Face(sh);
            if (face.IsNull()) {
                // No assert for wrong user input!
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No support face selected"),
                    QObject::tr("You have to select a face as support for a sketch!"));
                return;
            }

            BRepAdaptor_Surface adapt(face);
            if (adapt.GetType() != GeomAbs_Plane) {
                TopLoc_Location loc;
                Handle(Geom_Surface) surf = BRep_Tool::Surface(face, loc);
                if (surf.IsNull() || !GeomLib_IsPlanarSurface(surf).IsPlanar()) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No planar support"),
                        QObject::tr("You need a planar face as support for a sketch!"));
                    return;
                }
            }

            supportString = faceSelObject.getAsPropertyLinkSubString();
        }
        else {
            obj = static_cast<Part::Feature*>(PlaneFilter.Result[0][0].getObject());
            supportString = getObjectCmd(obj,"(",",'')");
        }


        if (!pcActiveBody->hasObject(obj)) {
            if ( !obj->isDerivedFrom ( App::Plane::getClassTypeId() ) )  {
                // TODO check here if the plane associated with right part/body (2015-09-01, Fat-Zer)

                auto pcActivePart = PartDesignGui::getPartFor(pcActiveBody, false);

                //check the prerequisites for the selected objects
                //the user has to decide which option we should take if external references are used
                // TODO share this with UnifiedDatumCommand() (2015-10-20, Fat-Zer)
                QDialog dia(Gui::getMainWindow());
                PartDesignGui::Ui_DlgReference dlg;
                dlg.setupUi(&dia);
                dia.setModal(true);
                int result = dia.exec();
                if (result == QDialog::DialogCode::Rejected)
                    return;
                else if (!dlg.radioXRef->isChecked()) {
                    openCommand("Make copy");
                    std::string sub;
                    if (FaceFilter.match())
                        sub = FaceFilter.Result[0][0].getSubNames()[0];
                    auto copy = PartDesignGui::TaskFeaturePick::makeCopy(obj, sub, dlg.radioIndependent->isChecked());

                    if (pcActiveBody)
                        pcActiveBody->addObject(copy);
                    else if (pcActivePart)
                        pcActivePart->addObject(copy);

                    if (PlaneFilter.match())
                        supportString = getObjectCmd(copy,"(",",'')");
                    else
                        //it is ensured that only a single face is selected, hence it must always be Face1 of the shapebinder
                        supportString = getObjectCmd(copy,"(",",'Face1')");
                    commitCommand();
                }
            }
        }

        // create Sketch on Face or Plane
        std::string FeatName = getUniqueObjectName("Sketch",pcActiveBody);

        openCommand("Create a Sketch on Face");
        Gui::cmdAppObject(pcActiveBody, std::ostringstream()
                << "newObjectAt('Sketcher::SketchObject', '" << FeatName << "', "
                            <<  "FreeCADGui.Selection.getSelection())");
        auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());
        Gui::cmdAppObject(Feat, std::ostringstream() <<"Support = " << supportString);
        Gui::cmdAppObject(Feat, std::ostringstream() <<"MapMode = '" << Attacher::AttachEngine::getModeName(Attacher::mmFlatFace)<<"'");
        updateActive();
        PartDesignGui::setEdit(Feat,pcActiveBody);
    }
    else {
        App::GeoFeatureGroupExtension *geoGroup( nullptr );
        if (pcActiveBody) {
            auto group( App::GeoFeatureGroupExtension::getGroupOfObject(pcActiveBody) );
            if (group) {
                geoGroup = group->getExtensionByType<App::GeoFeatureGroupExtension>();
            }
        }

        std::vector<App::DocumentObject*> planes;
        std::vector<PartDesignGui::TaskFeaturePick::featureStatus> status;

        // Start command early, so undo will undo any Body creation
        Gui::Command::openCommand("Create a new Sketch");
        if (shouldMakeBody) {
            pcActiveBody = PartDesignGui::makeBody(doc);
            if ( !pcActiveBody ) {
                Base::Console().Error("Failed to create a Body object");
                return;
            }
        }

        // At this point, we have pcActiveBody

        unsigned validPlaneCount = 0;

        // Baseplanes are preapproved
        try {
            for ( auto plane: pcActiveBody->getOrigin ()->planes() ) {
                planes.push_back (plane);
                status.push_back(PartDesignGui::TaskFeaturePick::basePlane);
                validPlaneCount++;
            }
        } catch (const Base::Exception &ex) {
            Base::Console().Error ("%s\n", ex.what() );
        }

        auto datumPlanes( getDocument()->getObjectsOfType(PartDesign::Plane::getClassTypeId()) );
        for (auto plane: datumPlanes) {
            planes.push_back ( plane );
            // Check whether this plane belongs to the active body
            if ( pcActiveBody->hasObject(plane) ) {
                if ( !pcActiveBody->isAfterInsertPoint ( plane ) ) {
                    validPlaneCount++;
                    status.push_back(PartDesignGui::TaskFeaturePick::validFeature);
                } else {
                    status.push_back(PartDesignGui::TaskFeaturePick::afterTip);
                }
            } else {
                PartDesign::Body *planeBody = PartDesign::Body::findBodyOf (plane);
                if ( planeBody ) {
                    if ( ( geoGroup && geoGroup->hasObject ( planeBody, true ) ) ||
                           !App::GeoFeatureGroupExtension::getGroupOfObject (planeBody) ) {
                        status.push_back ( PartDesignGui::TaskFeaturePick::otherBody );
                    } else {
                        status.push_back ( PartDesignGui::TaskFeaturePick::otherPart );
                    }
                } else {
                    if ( ( geoGroup && geoGroup->hasObject ( plane, true ) ) ||
                           !App::GeoFeatureGroupExtension::getGroupOfObject ( plane ) ) {
                        status.push_back ( PartDesignGui::TaskFeaturePick::otherPart );
                    } else {
                        status.push_back ( PartDesignGui::TaskFeaturePick::notInBody );
                    }
                }
            }
        }

        // Collect also shape binders consisting of a single planar face
        auto shapeBinders( getDocument()->getObjectsOfType(PartDesign::ShapeBinder::getClassTypeId()) );
        auto binders( getDocument()->getObjectsOfType(PartDesign::SubShapeBinder::getClassTypeId()) );
        shapeBinders.insert(shapeBinders.end(),binders.begin(),binders.end());
        for (auto binder : shapeBinders) {
            // Check whether this plane belongs to the active body
            if (pcActiveBody->hasObject(binder)) {
                TopoDS_Shape shape = static_cast<Part::Feature*>(binder)->Shape.getValue();
                if (!shape.IsNull() && shape.ShapeType() == TopAbs_FACE) {
                    const TopoDS_Face& face = TopoDS::Face(shape);
                    TopLoc_Location loc;
                    Handle(Geom_Surface) surf = BRep_Tool::Surface(face, loc);
                    if (!surf.IsNull() && GeomLib_IsPlanarSurface(surf).IsPlanar()) {
                        if (!pcActiveBody->isAfterInsertPoint (binder)) {
                            validPlaneCount++;
                            planes.push_back(binder);
                            status.push_back(PartDesignGui::TaskFeaturePick::validFeature);
                        }
                    }
                }
            }
        }

        // Determines if user made a valid selection in dialog
        auto accepter = [](const std::vector<App::DocumentObject*>& features) -> bool {
            return !features.empty();
        };

        // Called by dialog when user hits "OK" and accepter returns true
        auto worker = [=](const std::vector<App::DocumentObject*>& features) {
            // may happen when the user switched to an empty document while the
            // dialog is open
            if (features.empty())
                return;
            App::Plane* plane = static_cast<App::Plane*>(features.front());
            std::string FeatName = getUniqueObjectName("Sketch",pcActiveBody);
            std::string supportString = getObjectCmd(plane,"(",",[''])");

            Gui::cmdAppObject(pcActiveBody, std::ostringstream()
                    << "newObjectAt('Sketcher::SketchObject', '" << FeatName << "', "
                                <<  "FreeCADGui.Selection.getSelection())");
            auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());
            Gui::cmdAppObject(Feat, std::ostringstream() <<"Support = " << supportString);
            Gui::cmdAppObject(Feat, std::ostringstream() <<"MapMode = '" << Attacher::AttachEngine::getModeName(Attacher::mmFlatFace)<<"'");
            Gui::Command::updateActive(); // Make sure the Support's Placement property is updated
            PartDesignGui::setEdit(Feat,pcActiveBody);
        };

        // Called by dialog for "Cancel", or "OK" if accepter returns false
        std::string docname = doc->getName();
        auto quitter = [docname]() {
            Gui::Document* document = Gui::Application::Instance->getDocument(docname.c_str());
            if (document)
                document->abortCommand();
        };

        if (validPlaneCount == 0) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No valid planes in this document"),
                QObject::tr("Please create a plane first or select a face to sketch on"));
            quitter();
            return;

        } else if (validPlaneCount == 1) {
            worker(planes);

        } else if (validPlaneCount > 1) {
            // Show dialog and let user pick plane
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
                else {
                    quitter();
                    return;
                }
            }

            if (dlg)
                Gui::Control().closeDialog();

            Gui::Selection().clearSelection();
            Gui::Control().showDialog(new PartDesignGui::TaskDlgFeaturePick(planes, status, accepter, worker, quitter));
            App::AutoTransaction::setEnable(false);
        }
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
        if (subs.empty()
                || (!needSubElement
                    && feature->isDerivedFrom(Part::Part2DObject::getClassTypeId())))
        {
            Gui::cmdAppObject(Feat, std::ostringstream() <<"Profile = " << objCmd);
        }
        else {
            std::ostringstream ss;
            for (auto &s : subs)
                ss << "'" << s << "',";
            Gui::cmdAppObject(Feat, std::ostringstream() <<"Profile = (" << objCmd << ", [" << ss.str() << "])");   
        }

        //for additive and subtractive lofts allow the user to preselect the sections
        if (which.compare("AdditiveLoft") == 0 || which.compare("SubtractiveLoft") == 0) {
            std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
            if (selection.size() > 1) { //treat additional selected objects as sections
                for (std::vector<Gui::SelectionObject>::size_type ii = 1; ii < selection.size(); ii++) {
                    if (selection[ii].getObject()->isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
                        auto objCmdSection = Gui::Command::getObjectCmd(selection[ii].getObject());
                        Gui::cmdAppObject(Feat, std::ostringstream() << "Sections += [" << objCmdSection << "]");
                    }
                }
            }
        }

        // for additive and subtractive pipes allow the user to preselect the spines
        if (which.compare("AdditivePipe") == 0 || which.compare("SubtractivePipe") == 0) {
            std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
            if (selection.size() == 2) { //treat additional selected object as spine
                std::vector <string> subnames = selection[1].getSubNames();
                auto objCmdSpine = Gui::Command::getObjectCmd(selection[1].getObject());
                if (selection[1].getObject()->isDerivedFrom(Part::Part2DObject::getClassTypeId()) && subnames.empty()) {
                    Gui::cmdAppObject(Feat, std::ostringstream() <<"Spine = " << objCmdSpine);
                }
                else {
                    std::ostringstream ss;
                    for(auto &s : subnames) {
                        if (s.find("Edge") != std::string::npos)
                            ss << "'" << s << "',";
                    }
                    Gui::cmdAppObject(Feat, std::ostringstream() <<"Spine = (" << objCmdSpine << ", [" << ss.str() << "])");
                }
            }
        }

        func(static_cast<Part::Feature*>(feature), Feat);
    };

    //if a profile is selected we can make our life easy and fast
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    if (!selection.empty()) {
        base_worker(selection.front().getObject(), selection.front().getSubNames());
        return;
    }

    //no face profile was selected, do the extended sketch logic

    bool bNoSketchWasSelected = false;
    // Get a valid sketch from the user
    // First check selections
    std::vector<App::DocumentObject*> sketches = cmd->getSelection().getObjectsOfType(Part::Part2DObject::getClassTypeId());
    if (sketches.empty()) {//no sketches were selected. Let user pick an object from valid ones available in document
        sketches = cmd->getDocument()->getObjectsOfType(Part::Part2DObject::getClassTypeId());
        bNoSketchWasSelected = true;
    }

    if (sketches.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No sketch to work on"),
            QObject::tr("No sketch is available in the document"));
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

    //if there is a sketch selected which is from another body or part we need to bring up the
    //pick task dialog to decide how those are handled
    bool extReference = std::find_if( status.begin(), status.end(),
            [] (const PartDesignGui::TaskFeaturePick::featureStatus& s) {
                return s == PartDesignGui::TaskFeaturePick::otherBody ||
                    s == PartDesignGui::TaskFeaturePick::otherPart ||
                    s == PartDesignGui::TaskFeaturePick::notInBody;
            }
        ) != status.end();

    // TODO Clean this up (2015-10-20, Fat-Zer)
    if (pcActiveBody && !bNoSketchWasSelected && extReference) {

        // Hint: In an older version the function expected the body to be inside
        // a Part container and if not an error was raised and the function aborted.
        // First of all, for the user this wasn't obvious because the error message
        // was quite confusing (and thus the user may have done the wrong thing since
        // he may have assumed the that the sketch was meant) and second there is no need
        // that the body must be inside a Part container.
        // For more details see: https://forum.freecadweb.org/viewtopic.php?f=19&t=32164
        // The function has been modified not to expect the body to be in the Part
        // and it now directly invokes the 'makeCopy' dialog.
        auto* pcActivePart = PartDesignGui::getPartFor(pcActiveBody, false);

        QDialog dia(Gui::getMainWindow());
        PartDesignGui::Ui_DlgReference dlg;
        dlg.setupUi(&dia);
        dia.setModal(true);
        int result = dia.exec();
        if (result == QDialog::DialogCode::Rejected)
            return;

        if (!dlg.radioXRef->isChecked()) {
            Gui::Command::openCommand("Make copy");
            auto copy = PartDesignGui::TaskFeaturePick::makeCopy(sketches[0], "", dlg.radioIndependent->isChecked());
            auto oBody = PartDesignGui::getBodyFor(sketches[0], false);
            if (oBody)
                pcActiveBody->addObject(copy);
            else if (pcActivePart)
                pcActivePart->addObject(copy);

            sketches[0] = copy;
            firstFreeSketch = sketches.begin();
        }
    }

    // Show sketch choose dialog and let user pick sketch if no sketch was selected and no free one available or
    // multiple free ones are available
    if (bNoSketchWasSelected && (freeSketches != 1) ) {

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
        pickDlg = new PartDesignGui::TaskDlgFeaturePick(sketches, status, accepter, sketch_worker);
        // Logically dead code because 'bNoSketchWasSelected' must be true
        //if (!bNoSketchWasSelected && extReference)
        //    pickDlg->showExternal(true);

        Gui::Control().showDialog(pickDlg);
    }
    else {
        std::vector<App::DocumentObject*> theSketch;
        if (!bNoSketchWasSelected)
            theSketch.push_back(sketches[0]);
        else
            theSketch.push_back(*firstFreeSketch);

        sketch_worker(theSketch);
    }
}

void finishProfileBased(const Gui::Command* cmd, const Part::Feature* sketch, App::DocumentObject *Feat)
{
    if (sketch && sketch->isDerivedFrom(Part::Part2DObject::getClassTypeId()))
        FCMD_OBJ_HIDE(sketch);
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
        cmd->adjustCameraPosition();
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
        cmd->adjustCameraPosition();
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
        cmd->adjustCameraPosition();
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
        cmd->adjustCameraPosition();
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
        cmd->adjustCameraPosition();
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
        cmd->adjustCameraPosition();
    };

    prepareProfileBased(pcActiveBody, this, "Groove", worker);
}

bool CmdPartDesignGroove::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Additive_Pipe
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
    sPixmap       = "PartDesign_Additive_Pipe";
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
        cmd->adjustCameraPosition();
    };

    prepareProfileBased(pcActiveBody, this, "AdditivePipe", worker);
}

bool CmdPartDesignAdditivePipe::isActive(void)
{
    return hasActiveDocument();
}


//===========================================================================
// PartDesign_Subtractive_Pipe
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
    sPixmap       = "PartDesign_Subtractive_Pipe";
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
        cmd->adjustCameraPosition();
    };

    prepareProfileBased(pcActiveBody, this, "SubtractivePipe", worker);
}

bool CmdPartDesignSubtractivePipe::isActive(void)
{
    return hasActiveDocument();
}


//===========================================================================
// PartDesign_Additive_Loft
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
    sPixmap       = "PartDesign_Additive_Loft";
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
        cmd->adjustCameraPosition();
    };

    prepareProfileBased(pcActiveBody, this, "AdditiveLoft", worker);
}

bool CmdPartDesignAdditiveLoft::isActive(void)
{
    return hasActiveDocument();
}


//===========================================================================
// PartDesign_Subtractive_Loft
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
    sPixmap       = "PartDesign_Subtractive_Loft";
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
        cmd->adjustCameraPosition();
    };

    prepareProfileBased(pcActiveBody, this, "SubtractiveLoft", worker);
}

bool CmdPartDesignSubtractiveLoft::isActive(void)
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

    auto worker = [=](std::vector<App::DocumentObject*> features) {
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
        // FIXME: There seems to be kind of a race condition here, leading to sporadic errors like
        // Exception (Thu Sep  6 11:52:01 2012): 'App.Document' object has no attribute 'Mirrored'
        Gui::Command::updateActive(); // Helps to ensure that the object already exists when the next command comes up
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
        }

        if (obj->isDerivedFrom(PartDesign::MultiTransform::getClassTypeId())) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Invalid MultiTransform selection"),
                QObject::tr("MultiTransform feature cannot be nested."));
            return;
        }

        if (trFeat || !obj->isDerivedFrom(PartDesign::Transformed::getClassTypeId())) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Invalid MultiTransform selection"),
                QObject::tr("Please select one and only one pattern feature."));
        }
        trFeat = static_cast<PartDesign::Transformed*>(obj);
    }

    // Create a MultiTransform feature and move the Transformed feature inside it
    std::string FeatName = getUniqueObjectName("MultiTransform",pcActiveBody);
    std::string baseFeature = getObjectCmd(trFeat->BaseFeature.getValue()); 
    Gui::cmdAppObject(pcActiveBody, std::ostringstream()
            << "newObjectAt('PartDesign::MultiTransform','"
            << FeatName << "', " << baseFeature  << ")");

    auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());

    if (trFeat) {
        if (pcActiveBody->Tip.getValue() == trFeat)
            pcActiveBody->setTip(Feat);

        auto objCmd = getObjectCmd(trFeat);

        Gui::cmdAppObject(Feat, std::ostringstream() <<"OriginalSubs = "<<objCmd<<".OriginalSubs");
        Gui::cmdAppObject(trFeat, std::ostringstream() <<"BaseFeature = None");
        Gui::cmdAppObject(trFeat, std::ostringstream() <<"OriginalSubs = []");
        Gui::cmdAppObject(Feat, std::ostringstream() <<"Transformations = ["<<objCmd<<"]");
    }

    finishFeature(this, Feat);
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
}


void CmdPartDesignBoolean::activated(int iMsg)
{
    Q_UNUSED(iMsg);

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

    openCommand("Create Boolean");
    std::string FeatName = getUniqueObjectName("Boolean",pcActiveBody);
    Gui::cmdAppObject(pcActiveBody, std::ostringstream()
            << "newObjectAt('PartDesign::Boolean','" << FeatName << "', "
                        <<  "FreeCADGui.Selection.getSelection())");
    auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());

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
}

void CmdPartDesignSplit::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(/*messageIfNot = */true);
    if (!pcActiveBody) return;

    openCommand("Create Split");
    std::string FeatName = getUniqueObjectName("Split",pcActiveBody);
    Gui::cmdAppObject(pcActiveBody, std::ostringstream()
            << "newObjectAt('PartDesign::Split','" << FeatName << "', "
                        <<  "FreeCADGui.Selection.getSelection())");
    auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());
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
