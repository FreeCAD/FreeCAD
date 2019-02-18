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
#endif

#include <sstream>
#include <algorithm>

#include <App/DocumentObjectGroup.h>
#include <App/Origin.h>
#include <App/OriginFeature.h>
#include <App/Part.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Selection.h>
#include <Gui/MainWindow.h>
#include <Gui/Document.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureGroove.h>
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

// TODO Remove this header after fixing code so it won;t be needed here (2015-10-20, Fat-Zer)
#include "ui_DlgReference.h"

FC_LOG_LEVEL_INIT("PartDesign",true,true);

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
        } else if (pcActiveBody) {

            // TODO Check how this will work outside of a body (2015-10-20, Fat-Zer)
            std::string FeatName = cmd.getUniqueObjectName(name.c_str(), pcActiveBody);

            std::string tmp = std::string("Create ")+name;

            cmd.openCommand(tmp.c_str());
            FCMD_OBJ_CMD(pcActiveBody,"newObject('" << fullTypeName << "','" << FeatName << "')");

            // remove the body from links in case it's selected as
            // otherwise a cyclic dependency will be created
            support.removeValue(pcActiveBody);

            auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());
            if(!Feat) return;

            //test if current selection fits a mode.
            if (support.getSize() > 0) {
                Part::AttachExtension* pcDatum = Feat->getExtensionByType<Part::AttachExtension>();
                pcDatum->attacher().setReferences(support);
                SuggestResult sugr;
                pcDatum->attacher().suggestMapModes(sugr);
                if (sugr.message == Attacher::SuggestResult::srOK) {
                    //fits some mode. Populate support property.
                    FCMD_OBJ_CMD(Feat,"Support = " << support.getPyReprString());
                    FCMD_OBJ_CMD(Feat,"MapMode = '" << AttachEngine::getModeName(sugr.bestFitMode) << "'");
                } else {
                    QMessageBox::information(Gui::getMainWindow(),QObject::tr("Invalid selection"), QObject::tr("There are no attachment modes that fit selected objects. Select something else."));
                }
            }
            cmd.doCommand(Gui::Command::Doc,"App.activeDocument().recompute()");  // recompute the feature based on its references
            PartDesignGui::setEdit(Feat,pcActiveBody);
        } else {
            QMessageBox::warning(Gui::getMainWindow(),QObject::tr("Error"), QObject::tr("There is no active body. Please make a body active before inserting a datum entity."));
        }
    } catch (Base::Exception &e) {
        QMessageBox::warning(Gui::getMainWindow(),QObject::tr("Error"),QString::fromLatin1(e.what()));
    } catch (Standard_Failure &e) {
        QMessageBox::warning(Gui::getMainWindow(),QObject::tr("Error"),QString::fromLatin1(e.GetMessageString()));
    }
}

/* Datum feature commands =======================================================*/

DEF_STD_CMD_A(CmdPartDesignPlane);

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

DEF_STD_CMD_A(CmdPartDesignLine);

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

DEF_STD_CMD_A(CmdPartDesignPoint);

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

DEF_STD_CMD_A(CmdPartDesignShapeBinder);

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
        FCMD_OBJ_CMD(pcActiveBody,"newObject('PartDesign::ShapeBinder','" << FeatName << "')");

        // remove the body from links in case it's selected as
        // otherwise a cyclic dependency will be created
        support.removeValue(pcActiveBody);

        auto Feat = pcActiveBody->getObject(FeatName.c_str());
        if(!Feat) return;

        //test if current selection fits a mode.
        if (support.getSize() > 0) {
            FCMD_OBJ_CMD(Feat,"Support = " << support.getPyReprString());
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

DEF_STD_CMD_A(CmdPartDesignSubShapeBinder);

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

    PartDesign::SubShapeBinder *binder = 0;
    App::DocumentObject *binderParent = 0;
    std::string binderSub;
    std::map<App::DocumentObject *, std::vector<std::string> > values;
    for(auto &sel : Gui::Selection().getCompleteSelection(0)) {
        if(!sel.pObject) continue;
        if(!binder) {
            const char *dot = sel.SubName?strrchr(sel.SubName,'.'):0;
            if(!dot || dot[1]==0) {
                auto sobj = sel.pObject->getSubObject(sel.SubName);
                if(!sobj) continue;
                binder = dynamic_cast<PartDesign::SubShapeBinder*>(sobj->getLinkedObject(true));
                if(binder) {
                    binderParent = sel.pObject;
                    if(sel.SubName)
                        binderSub = sel.SubName;
                    continue;
                }
            }
        }
        auto &subs = values[sel.pObject];
        if(sel.SubName && sel.SubName[0])
            subs.emplace_back(sel.SubName);
    }

    PartDesign::Body *pcActiveBody = 0;
    std::string FeatName;
    if(!binder) {
        pcActiveBody = PartDesignGui::getBody(false,true,true,&binderParent,&binderSub);
        FeatName = getUniqueObjectName("Binder",pcActiveBody);
    }
    Base::Matrix4D mat;
    if(values.size()==1 && binderParent && binderParent!=binder) {
        App::DocumentObject *obj = values.begin()->first;
        auto subs = values.begin()->second;
        App::DocumentObject *sobj = 0;
        App::DocumentObject *parent = 0;
        std::string parentSub = binderSub;
        for(auto &sub : subs) {
            auto link = obj;
            auto linkSub = binderSub;
            auto res = binderParent->resolveRelativeLink(linkSub,link,sub);
            if(!sobj) {
                sobj = link;
                parent = res;
                parentSub = linkSub;
            }else if(sobj!=link || parent!=res) {
                QMessageBox::critical(Gui::getMainWindow(), QObject::tr("SubShapeBinder"),
                        QObject::tr("Cannot link to more than one object"));
                return;
            }
        }
        if(sobj) {
            values.clear();
            values[sobj] = std::move(subs);
        }
        if(parent) {
            binderParent = parent;
            binderSub = parentSub;
            binderParent->getSubObject(binderSub.c_str(),0,&mat);
        }
    }
        
    try {
        if (binder)
            openCommand("Change SubShapeBinder");
        else {
            openCommand("Create SubShapeBinder");
            if(pcActiveBody) {
                FCMD_OBJ_CMD(pcActiveBody,"newObject('PartDesign::SubShapeBinder','" << FeatName << "')");
                binder = dynamic_cast<PartDesign::SubShapeBinder*>(pcActiveBody->getObject(FeatName.c_str()));
            } else {
                doCommand(Command::Doc,
                        "App.ActiveDocument.addObject('PartDesign::SubShapeBinder','%s')",FeatName.c_str());
                binder = dynamic_cast<PartDesign::SubShapeBinder*>(
                        App::GetApplication().getActiveDocument()->getObject(FeatName.c_str()));
            }
            if(!binder) return;
        }
        binder->setLinks(std::move(values));
        updateActive();
        commitCommand();
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
        FCMD_OBJ_CMD(Feat,"BaseFeature = " << objCmd);
        FCMD_OBJ_CMD(Feat,"Placement = " << objCmd << ".Placement");
        FCMD_OBJ_CMD(Feat,"setEditorMode('Placement',0)");

        auto Body = obj->getDocument()->getObject(BodyName.c_str());
        FCMD_OBJ_CMD(Body,"Group = [" << getObjectCmd(Feat) << "]");

        // Set the tip of the body
        FCMD_OBJ_CMD(Body,"Tip = " << getObjectCmd(Feat));
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
DEF_STD_CMD_A(CmdPartDesignNewSketch);

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

    Gui::SelectionFilter SketchFilter("SELECT Sketcher::SketchObject COUNT 1");
    Gui::SelectionFilter FaceFilter  ("SELECT Part::Feature SUBELEMENT Face COUNT 1");
    Gui::SelectionFilter PlaneFilter ("SELECT App::Plane COUNT 1");
    Gui::SelectionFilter PlaneFilter2("SELECT PartDesign::Plane COUNT 1");

    if (PlaneFilter2.match())
        PlaneFilter = PlaneFilter2;

    if (SketchFilter.match()) {
        Sketcher::SketchObject *Sketch = static_cast<Sketcher::SketchObject*>(SketchFilter.Result[0][0].getObject());
        // openCommand("Edit Sketch");
        PartDesignGui::setEdit(Sketch,pcActiveBody);
    }
    else if ( FaceFilter.match() || PlaneFilter.match() ) {
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
            obj = FaceFilter.Result[0][0].getObject();

            if(!obj->isDerivedFrom(Part::Feature::getClassTypeId()))
                return;

            Part::Feature* feat = static_cast<Part::Feature*>(obj);

            const std::vector<std::string> &sub = FaceFilter.Result[0][0].getSubNames();
            if (sub.size() > 1) {
                // No assert for wrong user input!
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Several sub-elements selected"),
                    QObject::tr("You have to select a single face as support for a sketch!"));
                return;
            }

            // get the selected sub shape (a Face)
            const Part::TopoShape &shape = feat->Shape.getValue();
            TopoDS_Shape sh = shape.getSubShape(sub[0].c_str());
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

            supportString = FaceFilter.Result[0][0].getAsPropertyLinkSubString();
        } else {
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
                QDialog* dia = new QDialog;
                Ui_Dialog dlg;
                dlg.setupUi(dia);
                dia->setModal(true);
                int result = dia->exec();
                if(result == QDialog::DialogCode::Rejected)
                    return;
                else if(!dlg.radioXRef->isChecked()) {

                    std::string sub;
                    if(FaceFilter.match())
                        sub = FaceFilter.Result[0][0].getSubNames()[0];
                    auto copy = PartDesignGui::TaskFeaturePick::makeCopy(obj, sub, dlg.radioIndependent->isChecked());

                    if(pcActiveBody)
                        pcActiveBody->addObject(copy);
                    else if (pcActivePart)
                        pcActivePart->addObject(copy);

                    if(PlaneFilter.match())
                        supportString = getObjectCmd(copy,"(",",'')");
                    else
                        //it is ensured that only a single face is selected, hence it must always be Face1 of the shapebinder
                        supportString = getObjectCmd(copy,"(",",'Face1')");
                }
            }
        }

        // create Sketch on Face or Plane
        std::string FeatName = getUniqueObjectName("Sketch",pcActiveBody);

        openCommand("Create a Sketch on Face");
        FCMD_OBJ_CMD(pcActiveBody,"newObject('Sketcher::SketchObject','" << FeatName << "')");
        auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());
        FCMD_OBJ_CMD(Feat,"Support = " << supportString);
        FCMD_OBJ_CMD(Feat,"MapMode = '" << Attacher::AttachEngine::getModeName(Attacher::mmFlatFace)<<"'");
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

            // The method 'SoCamera::viewBoundingBox' is still declared as protected in Coin3d versions
            // older than 4.0.
#if COIN_MAJOR_VERSION >= 4
            // if no part feature was there then auto-adjust the camera
            Gui::Document* guidoc = Gui::Application::Instance->getDocument(doc);
            Gui::View3DInventor* view = guidoc ? qobject_cast<Gui::View3DInventor*>(guidoc->getActiveView()) : nullptr;
            if (view) {
                SoCamera* camera = view->getViewer()->getCamera();
                SbViewportRegion vpregion = view->getViewer()->getViewportRegion();
                float aspectratio = vpregion.getViewportAspectRatio();

                float size = Gui::ViewProviderOrigin::defaultSize();
                SbBox3f bbox;
                bbox.setBounds(-size,-size,-size,size,size,size);
                camera->viewBoundingBox(bbox, aspectratio, 1.0f);
            }
#endif
        }

        // At this point, we have pcActiveBody

        unsigned validPlaneCount = 0;

        // Baseplanes are preaprooved
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
            if ( pcActiveBody && pcActiveBody->hasObject(plane) ) {
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
                    } else if (pcActiveBody) {
                        status.push_back ( PartDesignGui::TaskFeaturePick::notInBody );
                    } else { // if we are outside a body count it as valid
                        validPlaneCount++;
                        status.push_back(PartDesignGui::TaskFeaturePick::validFeature);
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

            FCMD_OBJ_CMD(pcActiveBody,"newObject('Sketcher::SketchObject','" << FeatName << "')");
            auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());
            FCMD_OBJ_CMD(Feat,"Support = " << supportString);
            FCMD_OBJ_CMD(Feat,"MapMode = '" << Attacher::AttachEngine::getModeName(Attacher::mmFlatFace)<<"'");
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

            if(dlg)
                Gui::Control().closeDialog();

            Gui::Selection().clearSelection();
            Gui::Control().showDialog(new PartDesignGui::TaskDlgFeaturePick(planes, status, accepter, worker, quitter));
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

    if (hidePrevSolid && prevSolidFeature && (prevSolidFeature != NULL))
        FCMD_OBJ_HIDE(prevSolidFeature);

    if (updateDocument)
        cmd->updateActive();

    auto base = dynamic_cast<PartDesign::Feature*>(Feat);
    if(base)
        base = dynamic_cast<PartDesign::Feature*>(base->getBaseObject(true));
    App::DocumentObject *obj = base;
    if(!obj)
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
            if(!b)
                status.push_back(PartDesignGui::TaskFeaturePick::notInBody);
            else if(pcActivePart && pcActivePart->hasObject(b, true))
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
                        boost::function<void (Part::Feature*, App::DocumentObject*)> func)
{
    auto base_worker = [=](App::DocumentObject* feature, std::string sub) {

        if (!feature || !feature->isDerivedFrom(Part::Feature::getClassTypeId()))
            return;

        // Related to #0002760: when an operation can't be performed due to a broken
        // profile then make sure that it is recomputed when cancelling the operation
        // otherwise it might be impossible to see that it's broken.
        if (feature->isTouched())
            feature->recomputeFeature();

        std::string FeatName = cmd->getUniqueObjectName(which.c_str(),pcActiveBody);

        Gui::Command::openCommand((std::string("Make ") + which).c_str());

        FCMD_OBJ_CMD(pcActiveBody,"newObject('PartDesign::" << which << "','" << FeatName << "')");
        auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());
        
        auto objCmd = Gui::Command::getObjectCmd(feature);
        if (feature->isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
            FCMD_OBJ_CMD(Feat,"Profile = " << objCmd);
        }
        else {
            FCMD_OBJ_CMD(Feat,"Profile = (" << objCmd << ", ['" << sub << "'])");   
        }         

        func(static_cast<Part::Feature*>(feature), Feat);
    };
    
    //if a profile is selected we can make our life easy and fast
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    if (!selection.empty() && selection.front().hasSubNames()) {
        base_worker(selection.front().getObject(), selection.front().getSubNames().front());
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

        if(features.empty())
            return false;

        return true;
    };
    
    auto sketch_worker = [&, base_worker](std::vector<App::DocumentObject*> features) {
        
        base_worker(features.front(), "");
    };
    
    //if there is a sketch selected which is from another body or part we need to bring up the
    //pick task dialog to decide how those are handled
    bool ext = std::find_if( status.begin(), status.end(),
            [] (const PartDesignGui::TaskFeaturePick::featureStatus& s) {
                return s == PartDesignGui::TaskFeaturePick::otherBody ||
                    s == PartDesignGui::TaskFeaturePick::otherPart ||
                    s == PartDesignGui::TaskFeaturePick::notInBody;
            }
        ) != status.end();
        
    // TODO Clean this up (2015-10-20, Fat-Zer)
    if (pcActiveBody && !bNoSketchWasSelected && ext) {

        auto* pcActivePart = PartDesignGui::getPartFor(pcActiveBody, true);
        // getPartFor() already has reported an error
        if (!pcActivePart)
            return;

        QDialog* dia = new QDialog;
        Ui_Dialog dlg;
        dlg.setupUi(dia);
        dia->setModal(true);
        int result = dia->exec();
        if(result == QDialog::DialogCode::Rejected)
            return;
        else if(!dlg.radioXRef->isChecked()) {

                auto copy = PartDesignGui::TaskFeaturePick::makeCopy(sketches[0], "", dlg.radioIndependent->isChecked());
                auto oBody = PartDesignGui::getBodyFor(sketches[0], false);
                if(oBody)
                    pcActiveBody->addObject(copy);
                else
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

        if(dlg)
            Gui::Control().closeDialog();

        Gui::Selection().clearSelection();
        pickDlg = new PartDesignGui::TaskDlgFeaturePick(sketches, status, accepter, sketch_worker);
        if (!bNoSketchWasSelected && ext)
            pickDlg->showExternal(true);

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
    if(sketch && sketch->isDerivedFrom(Part::Part2DObject::getClassTypeId()))
        FCMD_OBJ_HIDE(sketch);
    finishFeature(cmd, Feat);
}

//===========================================================================
// PartDesign_Pad
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignPad);

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
        FCMD_OBJ_CMD(Feat,"Length = 10.0");
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
// PartDesign_Pocket
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignPocket);

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

        FCMD_OBJ_CMD(Feat,"Length = 5.0");
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
DEF_STD_CMD_A(CmdPartDesignHole);

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

        if(!Feat) return;

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
DEF_STD_CMD_A(CmdPartDesignRevolution);

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
            FCMD_OBJ_CMD(Feat,"ReferenceAxis = (" << getObjectCmd(sketch) << ",['V_Axis'])");
        }
        else {
            FCMD_OBJ_CMD(Feat,"ReferenceAxis = (" << getObjectCmd(pcActiveBody->getOrigin()->getY()) << ",[''])");
        }

        FCMD_OBJ_CMD(Feat,"Angle = 360.0");
        PartDesign::Revolution* pcRevolution = dynamic_cast<PartDesign::Revolution*>(Feat);
        if (pcRevolution && pcRevolution->suggestReversed())
            FCMD_OBJ_CMD(Feat,"Reversed = 1");

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
DEF_STD_CMD_A(CmdPartDesignGroove);

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
            FCMD_OBJ_CMD(Feat,"ReferenceAxis = ("<<getObjectCmd(sketch)<<",['V_Axis'])");
        }
        else {
            FCMD_OBJ_CMD(Feat,"ReferenceAxis = ("<<getObjectCmd(pcActiveBody->getOrigin()->getY())<<",[''])");
        }
        
        FCMD_OBJ_CMD(Feat,"Angle = 360.0");

        try {
            // This raises as exception if line is perpendicular to sketch/support face.
            // Here we should continue to give the user a chance to change the default values.
            PartDesign::Groove* pcGroove = dynamic_cast<PartDesign::Groove*>(Feat);
            if (pcGroove && pcGroove->suggestReversed())
                FCMD_OBJ_CMD(Feat,"Reversed = 1");
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
DEF_STD_CMD_A(CmdPartDesignAdditivePipe);

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
DEF_STD_CMD_A(CmdPartDesignSubtractivePipe);

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
DEF_STD_CMD_A(CmdPartDesignAdditiveLoft);

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
DEF_STD_CMD_A(CmdPartDesignSubtractiveLoft);

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
            QObject::tr("Select an edge, face or body."));
        return false;
    } else if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select an edge, face or body from a single body."));
        return false;
    }
    else if (pcActiveBody != PartDesignGui::getBodyFor(selection[0].getObject(), false)) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection is not in Active Body"),
            QObject::tr("Select an edge, face or body from an active body."));
        return false;
    }

    Gui::Selection().clearSelection();

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
    for(std::vector<std::string>::const_iterator it = SubNames.begin();it!=SubNames.end();++it){
        str << "'" << *it << "',";
    }
    str << "])";

    std::string FeatName = cmd->getUniqueObjectName(which.c_str(),base);

    auto body = PartDesignGui::getBodyFor(base,false);
    if(!body) return;
    cmd->openCommand((std::string("Make ") + which).c_str());
    FCMD_OBJ_CMD(body,"newObject('PartDesign::"<<which<<"','"<<FeatName<<"')");
    auto Feat = body->getDocument()->getObject(FeatName.c_str());
    FCMD_OBJ_CMD(Feat,"Base = " << str.str());
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
DEF_STD_CMD_A(CmdPartDesignFillet);

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
DEF_STD_CMD_A(CmdPartDesignChamfer);

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
DEF_STD_CMD_A(CmdPartDesignDraft);

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

        if(aSubName.size() > 4 && aSubName.substr(0,4) == "Face") {
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
DEF_STD_CMD_A(CmdPartDesignThickness);

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

        if(aSubName.size() > 4 && aSubName.substr(0,4) != "Face") {
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

void prepareTransformed(PartDesign::Body *pcActiveBody, Gui::Command* cmd, const std::string& which,
                        boost::function<void(App::DocumentObject *, std::vector<App::DocumentObject*>)> func)
{
    std::string FeatName = cmd->getUniqueObjectName(which.c_str(),pcActiveBody);

    auto accepter = [=](std::vector<App::DocumentObject*> features) -> bool{

        if(features.empty())
            return false;

        return true;
    };

    auto worker = [=](std::vector<App::DocumentObject*> features) {
        std::stringstream str;
        str << cmd->getObjectCmd(FeatName.c_str(),pcActiveBody->getDocument()) << ".Originals = [";
        for (std::vector<App::DocumentObject*>::iterator it = features.begin(); it != features.end(); ++it){
            str << cmd->getObjectCmd(*it) << ",";
        }
        str << "]";

        std::string msg("Make ");
        msg += which;
        Gui::Command::openCommand(msg.c_str());
        FCMD_OBJ_CMD(pcActiveBody,"newObject('PartDesign::"<<which<<"','"<<FeatName<<"')");
        // FIXME: There seems to be kind of a race condition here, leading to sporadic errors like
        // Exception (Thu Sep  6 11:52:01 2012): 'App.Document' object has no attribute 'Mirrored'
        Gui::Command::updateActive(); // Helps to ensure that the object already exists when the next command comes up
        Gui::Command::doCommand(Gui::Command::Doc, str.str().c_str());

        auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());

        // TODO Wjat that function supposed to do? (2015-08-05, Fat-Zer)
        func(Feat, features);

        // Set the tip of the body
        FCMD_OBJ_CMD(pcActiveBody,"Tip = " << Gui::Command::getObjectCmd(Feat));
        Gui::Command::updateActive();
    };

    // Get a valid original from the user
    // First check selections
    std::vector<App::DocumentObject*> features = cmd->getSelection().getObjectsOfType(PartDesign::Feature::getClassTypeId());
    // Next create a list of all eligible objects
    if (features.size() == 0) {
        features = cmd->getDocument()->getObjectsOfType(PartDesign::Feature::getClassTypeId());
        // If there is more than one selected or eligible object, show dialog and let user pick one
        if (features.size() > 1) {
            std::vector<PartDesignGui::TaskFeaturePick::featureStatus> status;
            for (unsigned i = 0; i < features.size(); i++)
                status.push_back(PartDesignGui::TaskFeaturePick::validFeature);

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

            if(dlg)
                Gui::Control().closeDialog();

            Gui::Selection().clearSelection();
            Gui::Control().showDialog(new PartDesignGui::TaskDlgFeaturePick(features, status, accepter, worker));
            return;
        } else if(features.empty()) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No valid features in this document"),
                QObject::tr("Please create a feature first."));
            return;
        }
    }
    if (features.size() > 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Multiple Features Selected"),
            QObject::tr("Please select only one feature first."));
        return;
    }
    else {
        PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);
        if (pcActiveBody != PartDesignGui::getBodyFor(features[0], false)) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection is not in Active Body"),
                QObject::tr("Please select only one feature in an active body."));
            return;
        }
        worker(features);
    }
}

void finishTransformed(Gui::Command* cmd, App::DocumentObject *Feat)
{
    finishFeature(cmd, Feat);
}

//===========================================================================
// PartDesign_Mirrored
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignMirrored);

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
    auto worker = [cmd](App::DocumentObject *Feat, std::vector<App::DocumentObject*> features) {

        if (features.empty())
            return;

        bool direction = false;
        if(features.front()->isDerivedFrom(PartDesign::ProfileBased::getClassTypeId())) {
            Part::Part2DObject *sketch = (static_cast<PartDesign::ProfileBased*>(features.front()))->getVerifiedSketch(/* silent =*/ true);
            if (sketch) {
                FCMD_OBJ_CMD(Feat,"MirrorPlane = ("<<getObjectCmd(sketch)<<", ['V_Axis'])");
                direction = true;
            }
        }
        if(!direction) {
            auto body = static_cast<PartDesign::Body*>(Part::BodyBase::findBodyOf(features.front()));
            if(body) {                
                FCMD_OBJ_CMD(Feat,"MirrorPlane = ("<<getObjectCmd(body->getOrigin()->getXY())<<", [''])");
            }
        }

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
DEF_STD_CMD_A(CmdPartDesignLinearPattern);

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
    auto worker = [cmd](App::DocumentObject *Feat, std::vector<App::DocumentObject*> features) {

        if (!Feat || features.empty())
            return;

        bool direction = false;
        if(features.front()->isDerivedFrom(PartDesign::ProfileBased::getClassTypeId())) {
            Part::Part2DObject *sketch = (static_cast<PartDesign::ProfileBased*>(features.front()))->getVerifiedSketch(/* silent =*/ true);
            if (sketch) {
                FCMD_OBJ_CMD(Feat,"Direction = ("<<Gui::Command::getObjectCmd(sketch)<<", ['H_Axis'])");
                direction = true;
            }
        }
        if(!direction) {
            auto body = static_cast<PartDesign::Body*>(Part::BodyBase::findBodyOf(features.front()));
            if(body) {                
                FCMD_OBJ_CMD(Feat,"Direction = ("<<Gui::Command::getObjectCmd(body->getOrigin()->getX())<<",[''])");
            }
        }
        FCMD_OBJ_CMD(Feat,"Length = 100");
        FCMD_OBJ_CMD(Feat,"Occurrences = 2");

        finishTransformed(cmd, Feat);
    };

    prepareTransformed(pcActiveBody, this, "LinearPattern", worker);
}

bool CmdPartDesignLinearPattern::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_PolarPattern
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignPolarPattern);

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
    auto worker = [cmd](App::DocumentObject *Feat, std::vector<App::DocumentObject*> features) {

        if (!Feat || features.empty())
            return;

        bool direction = false;
        if(features.front()->isDerivedFrom(PartDesign::ProfileBased::getClassTypeId())) {
            Part::Part2DObject *sketch = (static_cast<PartDesign::ProfileBased*>(features.front()))->getVerifiedSketch(/* silent =*/ true);
            if (sketch) {
                FCMD_OBJ_CMD(Feat,"Axis = ("<<Gui::Command::getObjectCmd(sketch)<<",['N_Axis'])");
                direction = true;
            }
        }
        if(!direction) {
            auto body = static_cast<PartDesign::Body*>(Part::BodyBase::findBodyOf(features.front()));
            if(body) {                
                FCMD_OBJ_CMD(Feat,"Axis = ("<<Gui::Command::getObjectCmd(body->getOrigin()->getZ())<<",[''])");
            }
        }

        FCMD_OBJ_CMD(Feat,"Angle = 360");
        FCMD_OBJ_CMD(Feat,"Occurrences = 2");

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
DEF_STD_CMD_A(CmdPartDesignScaled);

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
    auto worker = [cmd](App::DocumentObject *Feat, std::vector<App::DocumentObject*> features) {

        if (!Feat || features.empty())
            return;

        FCMD_OBJ_CMD(Feat,"Factor = 2");
        FCMD_OBJ_CMD(Feat,"Occurrences = 2");

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
DEF_STD_CMD_A(CmdPartDesignMultiTransform);

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

    std::vector<App::DocumentObject*> features;

    // Check if a Transformed feature has been selected, convert it to MultiTransform
    features = getSelection().getObjectsOfType(PartDesign::Transformed::getClassTypeId());
    if (!features.empty()) {
        // Throw out MultiTransform features, we don't want to nest them
        for (std::vector<App::DocumentObject*>::iterator f = features.begin(); f != features.end(); ) {
            if ((*f)->getTypeId().isDerivedFrom(PartDesign::MultiTransform::getClassTypeId()))
                f = features.erase(f);
            else
                f++;
        }

        if (features.empty()) return;
        // Note: If multiple Transformed features were selected, only the first one is used
        PartDesign::Transformed* trFeat = static_cast<PartDesign::Transformed*>(features.front());

        // Move the insert point back one feature
        App::DocumentObject* oldTip = 0;
        App::DocumentObject* prevFeature = 0;
        if (pcActiveBody){
            oldTip = pcActiveBody->Tip.getValue();
            prevFeature = pcActiveBody->getPrevSolidFeature(trFeat);
        }
        Gui::Selection().clearSelection();
        if (prevFeature != NULL)
            Gui::Selection().addSelection(prevFeature->getDocument()->getName(), prevFeature->getNameInDocument());

        openCommand("Convert to MultiTransform feature", true);

        Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
        rcCmdMgr.runCommandByName("PartDesign_MoveTip");

        // We cannot remove the Transform feature from the body as otherwise
        // we will have a PartDesign feature without a body which is not allowed
        // and causes to pop up the migration dialog later when adding new features
        // to the body.
        // Additionally it creates the error message: "Links go out of the allowed scope"
        // #0003509
#if 0
        // Remove the Transformed feature from the Body
        if(pcActiveBody)
            FCMD_OBJ_CMD(pcActiveBody,"removeObject("<<getObjectCmd(trFeat)<<")");
#endif

        // Create a MultiTransform feature and move the Transformed feature inside it
        std::string FeatName = getUniqueObjectName("MultiTransform",pcActiveBody);
        FCMD_OBJ_CMD(pcActiveBody,"newObject('PartDesign::MultiTransform','"<<FeatName<<"')");
        auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());
        auto objCmd = getObjectCmd(trFeat);
        FCMD_OBJ_CMD(Feat,"OriginalSubs = "<<objCmd<<".OriginalSubs");
        FCMD_OBJ_CMD(Feat,"BaseFeature = "<<objCmd<<".BaseFeature");
        FCMD_OBJ_CMD(trFeat,"OriginalSubs = []");
        FCMD_OBJ_CMD(Feat,"Transformations = ["<<objCmd<<"]");

        // Add the MultiTransform into the Body at the current insert point
        finishFeature(this, Feat);

        // Restore the insert point
        if (pcActiveBody && oldTip != trFeat) {
            Gui::Selection().clearSelection();
            Gui::Selection().addSelection(oldTip->getDocument()->getName(), oldTip->getNameInDocument());
            rcCmdMgr.runCommandByName("PartDesign_MoveTip");
            Gui::Selection().clearSelection();
        } // otherwise the insert point remains at the new MultiTransform, which is fine
    } else {

        Gui::Command* cmd = this;
        auto worker = [cmd, pcActiveBody](App::DocumentObject *Feat, std::vector<App::DocumentObject*> features) {

            if (!Feat || features.empty())
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
DEF_STD_CMD_A(CmdPartDesignBoolean);

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
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(/*messageIfNot = */true);
    if (!pcActiveBody) return;

    Gui::SelectionFilter BodyFilter("SELECT Part::Feature COUNT 1..");

    openCommand("Create Boolean");
    std::string FeatName = getUniqueObjectName("Boolean",pcActiveBody);
    FCMD_OBJ_CMD(pcActiveBody,"newObject('PartDesign::Boolean','"<<FeatName<<"')");
    auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());
    
    // If we don't add an object to the boolean group then don't update the body
    // as otherwise this will fail and it will be marked as invalid
    bool updateDocument = false;
    if (BodyFilter.match() && !BodyFilter.Result.empty()) {
        std::vector<App::DocumentObject*> bodies;
        std::vector<std::vector<Gui::SelectionObject> >::iterator i = BodyFilter.Result.begin();
        for (; i != BodyFilter.Result.end(); i++) {
            for (std::vector<Gui::SelectionObject>::iterator j = i->begin(); j != i->end(); j++) {
                if(j->getObject() != pcActiveBody)
                    bodies.push_back(j->getObject());
            }
        }
        if (!bodies.empty()) {
            updateDocument = true;
            std::string bodyString = PartDesignGui::buildLinkListPythonStr(bodies);
            FCMD_OBJ_CMD(Feat,"addObjects("<<bodyString<<")");
        }
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
    rcCmdMgr.addCommand(new CmdPartDesignMultiTransform());

    rcCmdMgr.addCommand(new CmdPartDesignBoolean());
}
