/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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


#include <BRep_Tool.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomLib_IsPlanarSurface.hxx>
#include <QMessageBox>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>


#include <App/Origin.h>
#include <App/Part.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureBoolean.h>
#include <Mod/PartDesign/App/FeatureGroove.h>
#include <Mod/PartDesign/App/FeatureMultiTransform.h>
#include <Mod/PartDesign/App/FeatureRevolution.h>
#include <Mod/PartDesign/App/FeatureTransformed.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/DatumPoint.h>
#include <Mod/PartDesign/App/FeatureDressUp.h>
#include <Mod/PartDesign/App/ShapeBinder.h>

#include "DlgActiveBody.h"
#include "ReferenceSelection.h"
#include "SketchWorkflow.h"
#include "TaskFeaturePick.h"
#include "Utils.h"
#include "WorkflowManager.h"
#include "ViewProvider.h"
#include "ViewProviderBody.h"


// TODO Remove this header after fixing code so it won;t be needed here (2015-10-20, Fat-Zer)
#include "ui_DlgReference.h"

FC_LOG_LEVEL_INIT("PartDesign", true, true)

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
void UnifiedDatumCommand(Gui::Command& cmd, Base::Type type, std::string name)
{
    try {
        std::string fullTypeName(type.getName());

        App::PropertyLinkSubList support;
        cmd.getSelection().getAsPropertyLinkSubList(support);

        bool bEditSelected = false;
        if (support.getSize() == 1 && support.getValue()) {
            if (support.getValue()->isDerivedFrom(type)) {
                bEditSelected = true;
            }
        }

        PartDesign::Body* pcActiveBody = PartDesignGui::getBody(/*messageIfNot = */ true);

        if (bEditSelected) {
            std::string tmp = std::string("Edit ") + name;
            cmd.openCommand(tmp.c_str());
            PartDesignGui::setEdit(support.getValue(), pcActiveBody);
        }
        else if (pcActiveBody) {

            // TODO Check how this will work outside of a body (2015-10-20, Fat-Zer)
            std::string FeatName = cmd.getUniqueObjectName(name.c_str(), pcActiveBody);

            std::string tmp = std::string("Create ") + name;

            cmd.openCommand(tmp.c_str());
            FCMD_OBJ_CMD(pcActiveBody, "newObject('" << fullTypeName << "','" << FeatName << "')");

            // remove the body from links in case it's selected as
            // otherwise a cyclic dependency will be created
            support.removeValue(pcActiveBody);

            auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());
            if (!Feat) {
                return;
            }

            // test if current selection fits a mode.
            if (support.getSize() > 0) {
                Part::AttachExtension* pcDatum = Feat->getExtensionByType<Part::AttachExtension>();
                pcDatum->attacher().setReferences(support);
                SuggestResult sugr;
                pcDatum->attacher().suggestMapModes(sugr);
                if (sugr.message == Attacher::SuggestResult::srOK) {
                    // fits some mode. Populate AttachmentSupport property.
                    FCMD_OBJ_CMD(Feat, "AttachmentSupport = " << support.getPyReprString());
                    FCMD_OBJ_CMD(
                        Feat,
                        "MapMode = '" << AttachEngine::getModeName(sugr.bestFitMode) << "'"
                    );
                }
                else {
                    QMessageBox::information(
                        Gui::getMainWindow(),
                        QObject::tr("Invalid selection"),
                        QObject::tr("There are no attachment modes that fit selected objects. Select something else.")
                    );
                }
            }
            cmd.doCommand(
                Gui::Command::Doc,
                "App.activeDocument().recompute()"
            );  // recompute the feature based on its references
            PartDesignGui::setEdit(Feat, pcActiveBody);
        }
        else {
            QMessageBox::warning(
                Gui::getMainWindow(),
                QObject::tr("Error"),
                QObject::tr("There is no active body. Please activate a body before inserting a datum entity.")
            );
        }
    }
    catch (Base::Exception& e) {
        QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("Error"),
            QApplication::translate("Exception", e.what())
        );
    }
    catch (Standard_Failure& e) {
        QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("Error"),
            QString::fromLatin1(e.GetMessageString())
        );
    }
}

/* Datum feature commands =======================================================*/

DEF_STD_CMD_A(CmdPartDesignPlane)

CmdPartDesignPlane::CmdPartDesignPlane()
    : Command("PartDesign_Plane")
{
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Datum Plane");
    sToolTipText = QT_TR_NOOP("Creates a new datum plane");
    sWhatsThis = "PartDesign_Plane";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_Plane";
}

void CmdPartDesignPlane::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    UnifiedDatumCommand(*this, Base::Type::fromName("PartDesign::Plane"), "DatumPlane");
}

bool CmdPartDesignPlane::isActive()
{
    if (getActiveGuiDocument()) {
        return true;
    }
    else {
        return false;
    }
}

DEF_STD_CMD_A(CmdPartDesignLine)

CmdPartDesignLine::CmdPartDesignLine()
    : Command("PartDesign_Line")
{
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Datum Line");
    sToolTipText = QT_TR_NOOP("Creates a new datum line");
    sWhatsThis = "PartDesign_Line";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_Line";
}

void CmdPartDesignLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    UnifiedDatumCommand(*this, Base::Type::fromName("PartDesign::Line"), "DatumLine");
}

bool CmdPartDesignLine::isActive()
{
    if (getActiveGuiDocument()) {
        return true;
    }
    else {
        return false;
    }
}

DEF_STD_CMD_A(CmdPartDesignPoint)

CmdPartDesignPoint::CmdPartDesignPoint()
    : Command("PartDesign_Point")
{
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Datum Point");
    sToolTipText = QT_TR_NOOP("Creates a new datum point");
    sWhatsThis = "PartDesign_Point";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_Point";
}

void CmdPartDesignPoint::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    UnifiedDatumCommand(*this, Base::Type::fromName("PartDesign::Point"), "DatumPoint");
}

bool CmdPartDesignPoint::isActive()
{
    if (getActiveGuiDocument()) {
        return true;
    }
    else {
        return false;
    }
}

DEF_STD_CMD_A(CmdPartDesignCS)

CmdPartDesignCS::CmdPartDesignCS()
    : Command("PartDesign_CoordinateSystem")
{
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Local Coordinate System");
    sToolTipText = QT_TR_NOOP("Creates a new local coordinate system");
    sWhatsThis = "PartDesign_CoordinateSystem";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_CoordinateSystem";
}

void CmdPartDesignCS::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    UnifiedDatumCommand(*this, Base::Type::fromName("PartDesign::CoordinateSystem"), "Local_CS");
}

bool CmdPartDesignCS::isActive()
{
    if (getActiveGuiDocument()) {
        return true;
    }
    else {
        return false;
    }
}

//===========================================================================
// PartDesign_ShapeBinder
//===========================================================================

DEF_STD_CMD_A(CmdPartDesignShapeBinder)

CmdPartDesignShapeBinder::CmdPartDesignShapeBinder()
    : Command("PartDesign_ShapeBinder")
{
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Shape Binder");
    sToolTipText = QT_TR_NOOP("Creates a new shape binder");
    sWhatsThis = "PartDesign_ShapeBinder";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_ShapeBinder";
}

void CmdPartDesignShapeBinder::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    App::PropertyLinkSubList support;
    getSelection().getAsPropertyLinkSubList(support);

    bool bEditSelected = false;
    if (support.getSize() == 1 && support.getValue()) {
        if (support.getValue()->isDerivedFrom<PartDesign::ShapeBinder>()) {
            bEditSelected = true;
        }
    }

    if (bEditSelected) {
        openCommand(QT_TRANSLATE_NOOP("Command", "Edit Shape Binder"));
        PartDesignGui::setEdit(support.getValue());
    }
    else {
        PartDesign::Body* pcActiveBody = PartDesignGui::getBody(/*messageIfNot = */ true);
        if (!pcActiveBody) {
            return;
        }

        std::string FeatName = getUniqueObjectName("ShapeBinder", pcActiveBody);

        openCommand(QT_TRANSLATE_NOOP("Command", "Create Shape Binder"));
        FCMD_OBJ_CMD(pcActiveBody, "newObject('PartDesign::ShapeBinder','" << FeatName << "')");

        // remove the body from links in case it's selected as
        // otherwise a cyclic dependency will be created
        support.removeValue(pcActiveBody);

        auto Feat = pcActiveBody->getObject(FeatName.c_str());
        if (!Feat) {
            return;
        }

        // test if current selection fits a mode.
        if (support.getSize() > 0) {
            FCMD_OBJ_CMD(Feat, "Support = " << support.getPyReprString());
        }
        updateActive();
        PartDesignGui::setEdit(Feat, pcActiveBody);
    }
    // TODO do a proper error processing (2015-09-11, Fat-Zer)
}

bool CmdPartDesignShapeBinder::isActive()
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_SubShapeBinder
//===========================================================================

DEF_STD_CMD_A(CmdPartDesignSubShapeBinder)

CmdPartDesignSubShapeBinder::CmdPartDesignSubShapeBinder()
    : Command("PartDesign_SubShapeBinder")
{
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Sub-Shape Binder");
    sToolTipText = QT_TR_NOOP(
        "Creates a reference to geometry from one or more objects, allowing it to be used inside "
        "or outside a body. It tracks relative placements, supports multiple geometry types "
        "(solids, faces, edges, vertices), and can work with objects in the same or external "
        "documents."
    );
    sWhatsThis = "PartDesign_SubShapeBinder";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_SubShapeBinder";
}

void CmdPartDesignSubShapeBinder::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    App::DocumentObject* parent = nullptr;
    std::string parentSub;
    std::map<App::DocumentObject*, std::vector<std::string>> values;
    for (auto& sel : Gui::Selection().getCompleteSelection(Gui::ResolveMode::NoResolve)) {
        if (!sel.pObject) {
            continue;
        }
        auto& subs = values[sel.pObject];
        if (sel.SubName && sel.SubName[0]) {
            subs.emplace_back(sel.SubName);
        }
    }

    std::string FeatName;
    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(false, true, true, &parent, &parentSub);
    FeatName = getUniqueObjectName("Binder", pcActiveBody);
    if (parent) {
        decltype(values) links;
        for (auto& v : values) {
            App::DocumentObject* obj = v.first;
            if (obj != parent) {
                auto& subs = links[obj];
                subs.insert(subs.end(), v.second.begin(), v.second.end());
                continue;
            }
            for (auto& sub : v.second) {
                auto link = obj;
                auto linkSub = parentSub;
                parent->resolveRelativeLink(linkSub, link, sub);
                if (link && link != pcActiveBody) {
                    links[link].push_back(sub);
                }
            }
        }
        values = std::move(links);
    }

    PartDesign::SubShapeBinder* binder = nullptr;
    try {
        openCommand(QT_TRANSLATE_NOOP("Command", "Create Sub-Shape Binder"));
        if (pcActiveBody) {
            FCMD_OBJ_CMD(pcActiveBody, "newObject('PartDesign::SubShapeBinder','" << FeatName << "')");
            binder = dynamic_cast<PartDesign::SubShapeBinder*>(
                pcActiveBody->getObject(FeatName.c_str())
            );
        }
        else {
            doCommand(
                Command::Doc,
                "App.ActiveDocument.addObject('PartDesign::SubShapeBinder','%s')",
                FeatName.c_str()
            );
            binder = dynamic_cast<PartDesign::SubShapeBinder*>(
                App::GetApplication().getActiveDocument()->getObject(FeatName.c_str())
            );
        }
        if (!binder) {
            return;
        }
        binder->setLinks(std::move(values));
        updateActive();
        commitCommand();
    }
    catch (Base::Exception& e) {
        e.reportException();
        QMessageBox::critical(
            Gui::getMainWindow(),
            QObject::tr("Sub-shape binder"),
            QApplication::translate("Exception", e.what())
        );
        abortCommand();
    }
}

bool CmdPartDesignSubShapeBinder::isActive()
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Clone
//===========================================================================

DEF_STD_CMD_A(CmdPartDesignClone)

CmdPartDesignClone::CmdPartDesignClone()
    : Command("PartDesign_Clone")
{
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Clone");
    sToolTipText = QT_TR_NOOP("Copies a solid object parametrically as the base feature of a new body");
    sWhatsThis = "PartDesign_Clone";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_Clone";
}

void CmdPartDesignClone::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<App::DocumentObject*> objs = getSelection().getObjectsOfType(
        Part::Feature::getClassTypeId()
    );
    if (objs.size() == 1) {
        // As suggested in https://forum.freecad.org/viewtopic.php?f=3&t=25265&p=198547#p207336
        // put the clone into its own new body.
        // This also fixes bug #3447 because the clone is a PD feature and thus
        // requires a body where it is part of.
        openCommand(QT_TRANSLATE_NOOP("Command", "Create Clone"));

        auto obj = objs[0];
        auto objCmd = getObjectCmd(obj);
        std::string cloneName = getUniqueObjectName("Clone", obj);
        std::string bodyName = getUniqueObjectName("Body", obj);

        // Create body and clone
        Gui::cmdAppDocument(
            obj,
            std::stringstream() << "addObject('PartDesign::Body','" << bodyName << "')"
        );
        Gui::cmdAppDocument(
            obj,
            std::stringstream() << "addObject('PartDesign::FeatureBase','" << cloneName << "')"
        );

        auto bodyObj = obj->getDocument()->getObject(bodyName.c_str());
        auto cloneObj = obj->getDocument()->getObject(cloneName.c_str());

        Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup(
            "BaseApp/Preferences/Mod/PartDesign"
        );

        bool allowCompound = hGrp->GetBool("AllowCompoundDefault", true);

        // In the first step set the group link and tip of the body
        Gui::cmdAppObject(bodyObj, std::stringstream() << "Group = [" << getObjectCmd(cloneObj) << "]");
        Gui::cmdAppObject(bodyObj, std::stringstream() << "Tip = " << getObjectCmd(cloneObj));
        Gui::cmdAppObject(
            bodyObj,
            std::stringstream() << "AllowCompound = " << (allowCompound ? "True" : "False")
        );

        // In the second step set the link of the base feature
        Gui::cmdAppObject(cloneObj, std::stringstream() << "BaseFeature = " << objCmd);
        Gui::cmdAppObject(cloneObj, std::stringstream() << "Placement = " << objCmd << ".Placement");
        Gui::cmdAppObject(cloneObj, std::stringstream() << "setEditorMode('Placement', 0)");

        updateActive();
        copyVisual(cloneObj, "ShapeAppearance", obj);
        copyVisual(cloneObj, "LineColor", obj);
        copyVisual(cloneObj, "PointColor", obj);
        copyVisual(cloneObj, "Transparency", obj);
        copyVisual(cloneObj, "DisplayMode", obj);
        commitCommand();
    }
}

bool CmdPartDesignClone::isActive()
{
    return getSelection().countObjectsOfType<Part::Feature>() == 1;
}

//===========================================================================
// PartDesign_Sketch
//===========================================================================

/* Sketch commands =======================================================*/
DEF_STD_CMD_A(CmdPartDesignNewSketch)

CmdPartDesignNewSketch::CmdPartDesignNewSketch()
    : Command("PartDesign_NewSketch")
{
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("New Sketch");
    sToolTipText = QT_TR_NOOP("Creates a new sketch");
    sWhatsThis = "PartDesign_NewSketch";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_NewSketch";
}


void CmdPartDesignNewSketch::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    PartDesignGui::SketchWorkflow creator(getActiveGuiDocument());
    creator.createSketch();
}

bool CmdPartDesignNewSketch::isActive()
{
    if (getActiveGuiDocument()) {
        return true;
    }
    else {
        return false;
    }
}

//===========================================================================
// Common utility functions for all features creating solids
//===========================================================================

static void finishFeature(
    const Gui::Command* cmd,
    App::DocumentObject* feature,
    App::DocumentObject* prevSolidFeature = nullptr,
    const bool hidePrevSolid = true,
    const bool updateDocument = true
)
{
    PartDesign::Body* activeBody;

    if (prevSolidFeature) {
        // insert into the same body as the given previous one
        activeBody = PartDesignGui::getBodyFor(prevSolidFeature, /*messageIfNot = */ false);
    }
    else {
        activeBody = PartDesignGui::getBody(/*messageIfNot = */ false);
    }

    if (hidePrevSolid && prevSolidFeature) {
        FCMD_OBJ_HIDE(prevSolidFeature);
    }

    if (updateDocument) {
        cmd->updateActive();
    }

    auto base = dynamic_cast<PartDesign::Feature*>(feature);
    if (base) {
        base = dynamic_cast<PartDesign::Feature*>(base->getBaseObject(true));
    }
    App::DocumentObject* obj = base;
    if (!obj) {
        obj = activeBody;
    }

    // Do this before calling setEdit to avoid to override the 'Shape preview' mode (#0003621)
    if (obj) {
        cmd->copyVisual(feature, "ShapeAppearance", obj);
        cmd->copyVisual(feature, "LineColor", obj);
        cmd->copyVisual(feature, "PointColor", obj);
        cmd->copyVisual(feature, "Transparency", obj);
        cmd->copyVisual(feature, "DisplayMode", obj);
    }

    PartDesignGui::setEdit(feature, activeBody);
    cmd->doCommand(cmd->Gui, "Gui.Selection.clearSelection()");
}

//===========================================================================
// Common utility functions for ProfileBased features
//===========================================================================

// Take a list of Part2DObjects and classify them for creating a
// ProfileBased feature. FirstFreeSketch is the first free sketch in the same body
// or sketches.end() if non available. The returned number is the amount of free sketches
unsigned validateSketches(
    std::vector<App::DocumentObject*>& sketches,
    std::vector<PartDesignGui::TaskFeaturePick::featureStatus>& status,
    std::vector<App::DocumentObject*>::iterator& firstFreeSketch
)
{
    // TODO Review the function for non-part bodies (2015-09-04, Fat-Zer)
    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(false);
    App::Part* pcActivePart = PartDesignGui::getPartFor(pcActiveBody, false);

    // TODO: If the user previously opted to allow multiple use of sketches or use of sketches from
    // other bodies, then count these as valid sketches!
    unsigned freeSketches = 0;
    firstFreeSketch = sketches.end();

    for (std::vector<App::DocumentObject*>::iterator s = sketches.begin(); s != sketches.end(); s++) {

        if (!pcActiveBody) {
            // We work in the old style outside any body
            if (PartDesign::Body::findBodyOf(*s)) {
                status.push_back(PartDesignGui::TaskFeaturePick::otherPart);
                continue;
            }
        }
        else if (!pcActiveBody->hasObject(*s)) {
            // Check whether this plane belongs to a body of the same part
            PartDesign::Body* b = PartDesign::Body::findBodyOf(*s);
            if (!b) {
                status.push_back(PartDesignGui::TaskFeaturePick::notInBody);
            }
            else if (pcActivePart && pcActivePart->hasObject(b, true)) {
                status.push_back(PartDesignGui::TaskFeaturePick::otherBody);
            }
            else {
                status.push_back(PartDesignGui::TaskFeaturePick::otherPart);
            }

            continue;
        }

        // Base::Console().error("Checking sketch %s\n", (*s)->getNameInDocument());
        //  Check whether this sketch is already being used by another feature
        //  Body features don't count...
        std::vector<App::DocumentObject*> inList = (*s)->getInList();
        std::vector<App::DocumentObject*>::iterator o = inList.begin();
        while (o != inList.end()) {
            // Base::Console().error("Inlist: %s\n", (*o)->getNameInDocument());
            if ((*o)->isDerivedFrom<PartDesign::Body>()) {
                o = inList.erase(o);  // ignore bodies
            }
            else if (!((*o)->isDerivedFrom<PartDesign::Feature>())) {
                o = inList.erase(o);  // ignore non-partDesign
            }
            else {
                ++o;
            }
        }
        if (!inList.empty()) {
            status.push_back(PartDesignGui::TaskFeaturePick::isUsed);
            continue;
        }

        if (pcActiveBody && pcActiveBody->isAfterInsertPoint(*s)) {
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
        int ctWires = 0;
        TopExp_Explorer ex;
        for (ex.Init(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
            ctWires++;
        }
        if (ctWires == 0) {
            status.push_back(PartDesignGui::TaskFeaturePick::noWire);
            continue;
        }

        // All checks passed - found a valid sketch
        if (firstFreeSketch == sketches.end()) {
            firstFreeSketch = s;
        }
        freeSketches++;
        status.push_back(PartDesignGui::TaskFeaturePick::validFeature);
    }

    return freeSketches;
}

/**
 *  Partially pulled from Linkstage3 importExternalObjects for toponaming element map
 *  compatibility with sketches that contain point objects.  By adding an empty
 *  subobject when appropriate, we allow those sketches to be used as profiles without error.
 *
 * @param prop  The property ( generally a Profile link )
 * @param _sobjs    Subobjects to use
 * @param report    True if we should raise a dialog, otherwise raise and exception
 * @return  True if elements were found
 */
bool importExternalElements(App::PropertyLinkSub& prop, std::vector<App::SubObjectT> _sobjs)
{
    if (!prop.getName() || !prop.getName()[0]) {
        FC_THROWM(Base::RuntimeError, "Invalid property");
    }
    auto editObj = freecad_cast<App::DocumentObject*>(prop.getContainer());
    if (!editObj) {
        FC_THROWM(Base::RuntimeError, "Editing object not found");
    }
    auto body = PartDesign::Body::findBodyOf(editObj);
    if (!body) {
        FC_THROWM(Base::RuntimeError, "No body for editing object: " << editObj->getNameInDocument());
    }
    std::map<App::DocumentObject*, std::vector<std::string>> links;
    std::vector<App::SubObjectT> sobjs;
    auto docName = editObj->getDocument()->getName();
    auto inList = editObj->getInListEx(true);
    for (auto sobjT : _sobjs) {
        auto sobj = sobjT.getSubObject();
        if (sobj == editObj) {
            continue;
        }
        if (!sobj) {
            FC_THROWM(Base::RuntimeError, "Object not found: " << sobjT.getSubObjectFullName(docName));
        }
        if (inList.count(sobj)) {
            FC_THROWM(
                Base::RuntimeError,
                "Cyclic dependency on object " << sobjT.getSubObjectFullName(docName)
            );
        }
        sobjT.normalized();
        // Make sure that if a subelement is chosen for some object,
        // we exclude whole object reference for that object.
        auto& subs = links[sobj];
        std::string element = sobjT.getOldElementName();
        if (element.size()) {
            if (subs.size() == 1 && subs.front().empty()) {
                for (auto it = sobjs.begin(); it != sobjs.end();) {
                    if (it->getSubObject() == sobj) {
                        sobjs.erase(it);
                        break;
                    }
                }
            }
        }
        else if (subs.size() > 0) {
            continue;
        }
        subs.push_back(std::move(element));
        sobjs.push_back(sobjT);
    }

    int import = 0;
    App::DocumentObject* obj = nullptr;
    std::vector<std::string> subs;
    for (const auto& sobjT : sobjs) {
        auto sobj = sobjT.getSubObject();
        if (PartDesign::Body::findBodyOf(sobj) != body) {
            import = 1;
            break;
        }
        if (!obj) {
            obj = sobj;
        }
        else if (obj != sobj) {
            if (!import) {
                import = -1;
            }
            break;
        }
        subs.push_back(sobjT.getOldElementName());
    }
    if (!import) {
        if (subs.empty()) {
            subs.emplace_back();
        }
        if (obj == prop.getValue() && prop.getSubValues() == subs) {
            return false;
        }
        prop.setValue(obj, std::move(subs));
        return true;
    }
    return false;
}


void prepareProfileBased(
    PartDesign::Body* pcActiveBody,
    Gui::Command* cmd,
    const std::string& which,
    std::function<void(Part::Feature*, App::DocumentObject*)> func
)
{
    auto base_worker = [=](App::DocumentObject* feature, const std::vector<std::string>& subs) {
        if (!feature || !feature->isDerivedFrom<Part::Feature>()) {
            return;
        }

        // Related to #0002760: when an operation can't be performed due to a broken
        // profile then make sure that it is recomputed when cancelling the operation
        // otherwise it might be impossible to see that it's broken.
        if (feature->isTouched()) {
            feature->recomputeFeature();
        }

        std::string FeatName = cmd->getUniqueObjectName(which.c_str(), pcActiveBody);

        Gui::Command::openCommand((std::string("Make ") + which).c_str());

        FCMD_OBJ_CMD(pcActiveBody, "newObject('PartDesign::" << which << "','" << FeatName << "')");
        auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());

        auto objCmd = Gui::Command::getObjectCmd(feature);

        // Populate the subs parameter by checking for external elements before
        // we construct our command.
        auto ProfileFeature = freecad_cast<PartDesign::ProfileBased*>(Feat);

        std::vector<std::string>& cmdSubs = const_cast<vector<std::string>&>(subs);
        if (subs.size() == 0) {
            importExternalElements(ProfileFeature->Profile, {feature});
            cmdSubs = ProfileFeature->Profile.getSubValues();
        }
        // run the command in console to set the profile (without selected subelements)
        auto runProfileCmd = [=]() {
            FCMD_OBJ_CMD(Feat, "Profile = " << objCmd);
        };

        // run the command in console to set the profile with selected subelements
        // useful to set, say, a face of a solid as the "profile"
        auto runProfileCmdWithSubs = [=]() {
            std::ostringstream ss;
            for (auto& s : cmdSubs) {
                ss << "'" << s << "',";
            }
            FCMD_OBJ_CMD(Feat, "Profile = (" << objCmd << ", [" << ss.str() << "])");
        };

        if (which.compare("AdditiveLoft") == 0 || which.compare("SubtractiveLoft") == 0) {
            // for additive and subtractive lofts set subvalues even for sketches
            // when a vertex is first selected
            auto subName = subs.empty() ? "" : subs.front();

            // `ProfileBased::getProfileShape()` and other methods will return
            // just the sub-shapes if they are set. So when whole sketches are
            // desired, do not set sub-values.
            if (feature->isDerivedFrom<Part::Part2DObject>() && subName.compare(0, 6, "Vertex") != 0) {
                runProfileCmd();
            }
            else {
                runProfileCmdWithSubs();
            }

            // for additive and subtractive lofts allow the user to preselect the sections
            std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
            if (selection.size() > 1) {  // treat additional selected objects as sections
                for (std::vector<Gui::SelectionObject>::size_type ii = 1; ii < selection.size();
                     ii++) {
                    // Add subvalues even for sketches in case we just want points
                    auto objCmdSection = Gui::Command::getObjectCmd(selection[ii].getObject());
                    const auto& subnames = selection[ii].getSubNames();
                    std::ostringstream ss;
                    if (!subnames.empty()) {
                        for (auto& s : subnames) {
                            ss << "'" << s << "',";
                        }
                    }
                    else {
                        // an empty string indicates the whole object
                        ss << "''";
                    }
                    FCMD_OBJ_CMD(Feat, "Sections += [(" << objCmdSection << ", [" << ss.str() << "])]");
                }
            }
        }
        else if (which.compare("AdditivePipe") == 0 || which.compare("SubtractivePipe") == 0) {
            // for additive and subtractive pipes set subvalues even for sketches
            // to support point sections
            auto subName = subs.empty() ? "" : subs.front();

            // `ProfileBased::getProfileShape()` and other methods will return
            // just the sub-shapes if they are set. So when whole sketches are
            // desired, don't set sub-values.
            if (feature->isDerivedFrom<Part::Part2DObject>() && subName.compare(0, 6, "Vertex") != 0) {
                runProfileCmd();
            }
            else {
                runProfileCmdWithSubs();
            }

            // for additive and subtractive pipes allow the user to preselect the spines
            std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
            if (selection.size() == 2) {  // treat additional selected object as spine
                std::vector<string> subnames = selection[1].getSubNames();
                auto objCmdSpine = Gui::Command::getObjectCmd(selection[1].getObject());
                if (selection[1].getObject()->isDerivedFrom<Part::Part2DObject>()
                    && subnames.empty()) {
                    FCMD_OBJ_CMD(Feat, "Spine = " << objCmdSpine);
                }
                else {
                    std::ostringstream ss;
                    for (auto& s : subnames) {
                        if (s.find("Edge") != std::string::npos) {
                            ss << "'" << s << "',";
                        }
                    }
                    FCMD_OBJ_CMD(Feat, "Spine = (" << objCmdSpine << ", [" << ss.str() << "])");
                }
            }
        }
        else {
            // Always use the subs
            runProfileCmdWithSubs();
        }

        func(static_cast<Part::Feature*>(feature), Feat);
    };


    // in case of subtractive types, check that there is something to subtract from
    if ((which.find("Subtractive") != std::string::npos) || (which.compare("Groove") == 0)
        || (which.compare("Pocket") == 0)) {

        if (!pcActiveBody->isSolid()) {
            QMessageBox msgBox(Gui::getMainWindow());
            msgBox.setText(
                QObject::tr("Cannot use this command as there is no solid to subtract from.")
            );
            msgBox.setInformativeText(
                QObject::tr("Ensure that the body contains a feature before attempting a subtractive command.")
            );
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
            return;
        }
    }


    // if a profile is selected we can make our life easy and fast
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    if (!selection.empty()) {
        bool onlyAllowed = true;
        for (const auto& it : selection) {
            if (PartDesign::Body::findBodyOf(it.getObject())
                != pcActiveBody) {  // the selected objects must belong to the body
                onlyAllowed = false;
                break;
            }
        }
        if (!onlyAllowed) {
            QMessageBox msgBox(Gui::getMainWindow());
            msgBox.setText(
                QObject::tr("Cannot use selected object. Selected object must belong to the active body")
            );
            msgBox.setInformativeText(
                QObject::tr("Consider using a shape binder or a base feature to reference external geometry in a body")
            );
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
        }
        else {
            base_worker(selection.front().getObject(), selection.front().getSubNames());
        }
        return;
    }

    // no face profile was selected, do the extended sketch logic

    bool bNoSketchWasSelected = false;
    // Get a valid sketch from the user
    // First check selections
    std::vector<App::DocumentObject*> sketches = cmd->getSelection().getObjectsOfType(
        Part::Part2DObject::getClassTypeId()
    );
    if (sketches.empty()) {  // no sketches were selected. Let user pick an object from valid ones
                             // available in document
        sketches = cmd->getDocument()->getObjectsOfType(Part::Part2DObject::getClassTypeId());
        bNoSketchWasSelected = true;
    }

    if (sketches.empty()) {
        QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("No sketch to work on"),
            QObject::tr("No sketch is available in the document")
        );
        return;
    }

    std::vector<PartDesignGui::TaskFeaturePick::featureStatus> status;
    std::vector<App::DocumentObject*>::iterator firstFreeSketch;
    int freeSketches = validateSketches(sketches, status, firstFreeSketch);

    auto accepter = [=](const std::vector<App::DocumentObject*>& features) -> bool {
        if (features.empty()) {
            return false;
        }

        return true;
    };

    auto sketch_worker = [&, base_worker](std::vector<App::DocumentObject*> features) {
        base_worker(features.front(), {});
    };

    // if there is a sketch selected which is from another body or part we need to bring up the
    // pick task dialog to decide how those are handled
    bool extReference = std::find_if(
                            status.begin(),
                            status.end(),
                            [](const PartDesignGui::TaskFeaturePick::featureStatus& s) {
                                return s == PartDesignGui::TaskFeaturePick::otherBody
                                    || s == PartDesignGui::TaskFeaturePick::otherPart
                                    || s == PartDesignGui::TaskFeaturePick::notInBody;
                            }
                        )
        != status.end();

    // TODO Clean this up (2015-10-20, Fat-Zer)
    if (pcActiveBody && !bNoSketchWasSelected && extReference) {

        // Hint: In an older version the function expected the body to be inside
        // a Part container and if not an error was raised and the function aborted.
        // First of all, for the user this wasn't obvious because the error message
        // was quite confusing (and thus the user may have done the wrong thing since
        // they may have assumed the that the sketch was meant) and
        // Second, there is no need that the body must be inside a Part container.
        // For more details see: https://forum.freecad.org/viewtopic.php?f=19&t=32164
        // The function has been modified not to expect the body to be in the Part
        // and it now directly invokes the 'makeCopy' dialog.
        auto* pcActivePart = PartDesignGui::getPartFor(pcActiveBody, false);

        QDialog dia(Gui::getMainWindow());
        PartDesignGui::Ui_DlgReference dlg;
        dlg.setupUi(&dia);
        dia.setModal(true);
        int result = dia.exec();
        if (result == QDialog::DialogCode::Rejected) {
            return;
        }

        if (!dlg.radioXRef->isChecked()) {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Make Copy"));
            auto copy = PartDesignGui::TaskFeaturePick::makeCopy(
                sketches[0],
                "",
                dlg.radioIndependent->isChecked()
            );
            auto oBody = PartDesignGui::getBodyFor(sketches[0], false);
            if (oBody) {
                pcActiveBody->addObject(copy);
            }
            else if (pcActivePart) {
                pcActivePart->addObject(copy);
            }

            sketches[0] = copy;
            firstFreeSketch = sketches.begin();
        }
    }

    // Show sketch choose dialog and let user pick sketch if no sketch was selected and no free one
    // available or multiple free ones are available
    if (bNoSketchWasSelected && (freeSketches != 1)) {

        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
        PartDesignGui::TaskDlgFeaturePick* pickDlg
            = qobject_cast<PartDesignGui::TaskDlgFeaturePick*>(dlg);
        if (dlg && !pickDlg) {
            QMessageBox msgBox(Gui::getMainWindow());
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes) {
                Gui::Control().closeDialog();
            }
            else {
                return;
            }
        }

        if (dlg) {
            Gui::Control().closeDialog();
        }

        Gui::Selection().clearSelection();
        pickDlg = new PartDesignGui::TaskDlgFeaturePick(sketches, status, accepter, sketch_worker, true);
        // Logically dead code because 'bNoSketchWasSelected' must be true
        // if (!bNoSketchWasSelected && extReference)
        //    pickDlg->showExternal(true);

        Gui::Control().showDialog(pickDlg);
    }
    else {
        std::vector<App::DocumentObject*> theSketch;
        if (!bNoSketchWasSelected) {
            theSketch.push_back(sketches[0]);
        }
        else {
            theSketch.push_back(*firstFreeSketch);
        }

        sketch_worker(theSketch);
    }
}

void finishProfileBased(const Gui::Command* cmd, const Part::Feature* sketch, App::DocumentObject* Feat)
{
    if (sketch && sketch->isDerivedFrom<Part::Part2DObject>()) {
        FCMD_OBJ_HIDE(sketch);
    }
    finishFeature(cmd, Feat);
}

void prepareProfileBased(Gui::Command* cmd, const std::string& which, double length)
{
    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody) {
        return;
    }

    auto worker = [cmd, length](Part::Feature* profile, App::DocumentObject* Feat) {
        if (!Feat) {
            return;
        }

        // specific parameters for Pad/Pocket
        FCMD_OBJ_CMD(Feat, "Length = " << length);
        Gui::Command::updateActive();

        Part::Part2DObject* sketch = dynamic_cast<Part::Part2DObject*>(profile);

        if (sketch) {
            std::ostringstream str;
            Gui::cmdAppObject(
                Feat,
                str << "ReferenceAxis = (" << Gui::Command::getObjectCmd(sketch) << ",['N_Axis'])"
            );
        }

        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, cmd, which, worker);
}

//===========================================================================
// PartDesign_Pad
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignPad)

CmdPartDesignPad::CmdPartDesignPad()
    : Command("PartDesign_Pad")
{
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Pad");
    sToolTipText = QT_TR_NOOP("Extrudes the selected sketch or profile and adds it to the body");
    sWhatsThis = "PartDesign_Pad";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_Pad";
}

void CmdPartDesignPad::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    prepareProfileBased(this, "Pad", 10.0);
}

bool CmdPartDesignPad::isActive()
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
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Pocket");
    sToolTipText = QT_TR_NOOP("Extrudes the selected sketch or profile and removes it from the body");
    sWhatsThis = "PartDesign_Pocket";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_Pocket";
}

void CmdPartDesignPocket::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    prepareProfileBased(this, "Pocket", 5.0);
}

bool CmdPartDesignPocket::isActive()
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
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Hole");
    sToolTipText
        = QT_TR_NOOP("Creates holes in the active body at the center points of circles or arcs of the selected sketch or profile");
    sWhatsThis = "PartDesign_Hole";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_Hole";
}

void CmdPartDesignHole::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody) {
        return;
    }

    Gui::Command* cmd = this;
    auto worker = [cmd](Part::Feature* sketch, App::DocumentObject* Feat) {
        if (!Feat) {
            return;
        }

        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, this, "Hole", worker);
}

bool CmdPartDesignHole::isActive()
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
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Revolve");
    sToolTipText = QT_TR_NOOP(
        "Revolves the selected sketch or profile around a line or axis and adds it to the body"
    );
    sWhatsThis = "PartDesign_Revolution";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_Revolution";
}

void CmdPartDesignRevolution::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody) {
        return;
    }

    Gui::Command* cmd = this;
    auto worker = [cmd, &pcActiveBody](Part::Feature* sketch, App::DocumentObject* Feat) {
        if (!Feat) {
            return;
        }

        if (sketch->isDerivedFrom<Part::Part2DObject>()) {
            FCMD_OBJ_CMD(Feat, "ReferenceAxis = (" << getObjectCmd(sketch) << ",['V_Axis'])");
        }
        else {
            FCMD_OBJ_CMD(
                Feat,
                "ReferenceAxis = (" << getObjectCmd(pcActiveBody->getOrigin()->getY()) << ",[''])"
            );
        }

        FCMD_OBJ_CMD(Feat, "Angle = 360.0");
        PartDesign::Revolution* pcRevolution = dynamic_cast<PartDesign::Revolution*>(Feat);
        if (pcRevolution && pcRevolution->suggestReversed()) {
            FCMD_OBJ_CMD(Feat, "Reversed = 1");
        }

        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, this, "Revolution", worker);
}

bool CmdPartDesignRevolution::isActive()
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
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Groove");
    sToolTipText = QT_TR_NOOP(
        "Revolves the sketch or profile around a line or axis and removes it from the body"
    );
    sWhatsThis = "PartDesign_Groove";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_Groove";
}

void CmdPartDesignGroove::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody) {
        return;
    }

    Gui::Command* cmd = this;
    auto worker = [cmd, &pcActiveBody](Part::Feature* sketch, App::DocumentObject* Feat) {
        if (!Feat) {
            return;
        }

        if (sketch->isDerivedFrom<Part::Part2DObject>()) {
            FCMD_OBJ_CMD(Feat, "ReferenceAxis = (" << getObjectCmd(sketch) << ",['V_Axis'])");
        }
        else {
            FCMD_OBJ_CMD(
                Feat,
                "ReferenceAxis = (" << getObjectCmd(pcActiveBody->getOrigin()->getY()) << ",[''])"
            );
        }

        FCMD_OBJ_CMD(Feat, "Angle = 360.0");

        try {
            // This raises as exception if line is perpendicular to sketch/support face.
            // Here we should continue to give the user a chance to change the default values.
            PartDesign::Groove* pcGroove = dynamic_cast<PartDesign::Groove*>(Feat);
            if (pcGroove && pcGroove->suggestReversed()) {
                FCMD_OBJ_CMD(Feat, "Reversed = 1");
            }
        }
        catch (const Base::Exception& e) {
            e.reportException();
        }

        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, this, "Groove", worker);
}

bool CmdPartDesignGroove::isActive()
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
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Additive Pipe");
    sToolTipText = QT_TR_NOOP(
        "Sweeps the selected sketch or profile along a path and adds it to the body"
    );
    sWhatsThis = "PartDesign_AdditivePipe";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_AdditivePipe";
}

void CmdPartDesignAdditivePipe::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody) {
        return;
    }

    Gui::Command* cmd = this;
    auto worker = [cmd](Part::Feature* sketch, App::DocumentObject* Feat) {
        if (!Feat) {
            return;
        }

        // specific parameters for pipe
        Gui::Command::updateActive();

        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, this, "AdditivePipe", worker);
}

bool CmdPartDesignAdditivePipe::isActive()
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
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Subtractive Pipe");
    sToolTipText = QT_TR_NOOP(
        "Sweeps the selected sketch or profile along a path and removes it from the body"
    );
    sWhatsThis = "PartDesign_SubtractivePipe";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_SubtractivePipe";
}

void CmdPartDesignSubtractivePipe::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody) {
        return;
    }

    Gui::Command* cmd = this;
    auto worker = [cmd](Part::Feature* sketch, App::DocumentObject* Feat) {
        if (!Feat) {
            return;
        }

        // specific parameters for pipe
        Gui::Command::updateActive();

        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, this, "SubtractivePipe", worker);
}

bool CmdPartDesignSubtractivePipe::isActive()
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
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Additive Loft");
    sToolTipText = QT_TR_NOOP(
        "Lofts the selected sketch or profile along a path and adds it to the body"
    );
    sWhatsThis = "PartDesign_AdditiveLoft";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_AdditiveLoft";
}

void CmdPartDesignAdditiveLoft::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody) {
        return;
    }

    Gui::Command* cmd = this;
    auto worker = [cmd](Part::Feature* sketch, App::DocumentObject* Feat) {
        if (!Feat) {
            return;
        }

        // specific parameters for pipe
        Gui::Command::updateActive();

        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, this, "AdditiveLoft", worker);
}

bool CmdPartDesignAdditiveLoft::isActive()
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
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Subtractive Loft");
    sToolTipText = QT_TR_NOOP(
        "Lofts the selected sketch or profile along a path and removes it from the body"
    );
    sWhatsThis = "PartDesign_SubtractiveLoft";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_SubtractiveLoft";
}

void CmdPartDesignSubtractiveLoft::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody) {
        return;
    }

    Gui::Command* cmd = this;
    auto worker = [cmd](Part::Feature* sketch, App::DocumentObject* Feat) {
        if (!Feat) {
            return;
        }

        // specific parameters for pipe
        Gui::Command::updateActive();

        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, this, "SubtractiveLoft", worker);
}

bool CmdPartDesignSubtractiveLoft::isActive()
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
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Additive Helix");
    sToolTipText = QT_TR_NOOP(
        "Sweeps the selected sketch or profile along a helix and adds it to the body"
    );
    sWhatsThis = "PartDesign_AdditiveHelix";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_AdditiveHelix";
}

void CmdPartDesignAdditiveHelix::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody) {
        return;
    }

    Gui::Command* cmd = this;
    auto worker = [cmd, &pcActiveBody](Part::Feature* sketch, App::DocumentObject* Feat) {
        if (!Feat) {
            return;
        }

        // Creating a helix with default values isn't always valid but fixes
        // itself when more values are set. So, this guard is used to suppress
        // errors before the user is able to change the parameters.
        Base::ObjectStatusLocker<App::Document::Status, App::Document> guard(
            App::Document::IgnoreErrorOnRecompute,
            Feat->getDocument(),
            true
        );

        // specific parameters for helix
        Gui::Command::updateActive();

        if (sketch->isDerivedFrom<Part::Part2DObject>()) {
            FCMD_OBJ_CMD(Feat, "ReferenceAxis = (" << getObjectCmd(sketch) << ",['V_Axis'])");
        }
        else {
            FCMD_OBJ_CMD(
                Feat,
                "ReferenceAxis = (" << getObjectCmd(pcActiveBody->getOrigin()->getY()) << ",[''])"
            );
        }

        finishProfileBased(cmd, sketch, Feat);

        // If the initial helix creation fails then it leaves the base object invisible which makes
        // things more difficult for the user. To avoid this the base object will be made tmp.
        // visible again.
        if (Feat->isError()) {
            App::DocumentObject* base = static_cast<PartDesign::Feature*>(Feat)->BaseFeature.getValue();
            if (base) {
                PartDesignGui::ViewProvider* view = dynamic_cast<PartDesignGui::ViewProvider*>(
                    Gui::Application::Instance->getViewProvider(base)
                );
                if (view) {
                    view->makeTemporaryVisible(true);
                }
            }
        }
    };

    prepareProfileBased(pcActiveBody, this, "AdditiveHelix", worker);
}

bool CmdPartDesignAdditiveHelix::isActive()
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
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Subtractive Helix");
    sToolTipText = QT_TR_NOOP(
        "Sweeps the selected sketch or profile along a helix and removes it from the body"
    );
    sWhatsThis = "PartDesign_SubtractiveHelix";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_SubtractiveHelix";
}

void CmdPartDesignSubtractiveHelix::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody) {
        return;
    }

    Gui::Command* cmd = this;
    auto worker = [cmd, &pcActiveBody](Part::Feature* sketch, App::DocumentObject* Feat) {
        if (!Feat) {
            return;
        }

        // specific parameters for helix
        Gui::Command::updateActive();

        if (sketch->isDerivedFrom<Part::Part2DObject>()) {
            FCMD_OBJ_CMD(Feat, "ReferenceAxis = (" << getObjectCmd(sketch) << ",['V_Axis'])");
        }
        else {
            FCMD_OBJ_CMD(
                Feat,
                "ReferenceAxis = (" << getObjectCmd(pcActiveBody->getOrigin()->getY()) << ",[''])"
            );
        }

        finishProfileBased(cmd, sketch, Feat);
    };

    prepareProfileBased(pcActiveBody, this, "SubtractiveHelix", worker);
}

bool CmdPartDesignSubtractiveHelix::isActive()
{
    return hasActiveDocument();
}

//===========================================================================
// Common utility functions for Dressup features
//===========================================================================

bool dressupGetSelected(
    Gui::Command* cmd,
    const std::string& which,
    Gui::SelectionObject& selected,
    bool& useAllEdges,
    bool& noSelection
)
{
    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody) {
        return false;
    }

    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();

    if (selection.empty()) {
        noSelection = true;
        return true;
    }
    else if (selection.size() != 1) {
        QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("Wrong selection"),
            QObject::tr("Select an edge, face, or body from a single body.")
        );
        return false;
    }
    else if (pcActiveBody != PartDesignGui::getBodyFor(selection[0].getObject(), false)) {
        QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("Selection is not in the active body"),
            QObject::tr("Select an edge, face, or body from an active body.")
        );
        return false;
    }

    Gui::Selection().clearSelection();

    // set the
    selected = selection[0];

    if (!selected.isObjectTypeOf(Part::Feature::getClassTypeId())) {
        QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("Wrong object type"),
            QObject::tr("%1 works only on parts.").arg(QString::fromStdString(which))
        );
        return false;
    }

    Part::Feature* base = static_cast<Part::Feature*>(selected.getObject());

    const Part::TopoShape& TopShape = base->Shape.getShape();

    if (TopShape.getShape().IsNull()) {
        QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("Wrong selection"),
            QObject::tr("Shape of the selected part is empty")
        );
        return false;
    }

    // if 1 Part::Feature object selected, but no subobjects, select all edges for the user
    // but only for fillet and chamfer (not for draft or thickness)
    if (selection[0].getSubNames().empty()
        && (which.compare("Fillet") == 0 || which.compare("Chamfer") == 0)) {
        useAllEdges = true;
        std::string edgeTypeName = Part::TopoShape::shapeName(TopAbs_EDGE);  //"Edge"
        int count = TopShape.countSubElements(edgeTypeName.c_str());
        std::string docName = App::GetApplication().getDocumentName(base->getDocument());
        std::string objName = base->getNameInDocument();
        for (int ii = 0; ii < count; ii++) {
            std::ostringstream edgeName;
            edgeName << edgeTypeName << ii + 1;
            Gui::Selection().addSelection(docName.c_str(), objName.c_str(), edgeName.str().c_str());
        }
        selection = cmd->getSelection().getSelectionEx();
        if (selection.size() == 1) {
            selected = selection[0];
        }
    }
    return true;
}

void finishDressupFeature(
    const Gui::Command* cmd,
    const std::string& which,
    Part::Feature* base,
    const std::vector<std::string>& SubNames,
    const bool useAllEdges
)
{
    std::ostringstream str;
    str << '(' << Gui::Command::getObjectCmd(base) << ",[";
    for (const auto& SubName : SubNames) {
        str << "'" << SubName << "',";
    }
    str << "])";

    std::string FeatName = cmd->getUniqueObjectName(which.c_str(), base);

    auto body = PartDesignGui::getBodyFor(base, false);
    if (!body) {
        return;
    }
    cmd->openCommand((std::string("Make ") + which).c_str());
    FCMD_OBJ_CMD(body, "newObject('PartDesign::" << which << "','" << FeatName << "')");
    auto Feat = body->getDocument()->getObject(FeatName.c_str());
    FCMD_OBJ_CMD(Feat, "Base = " << str.str());
    if (useAllEdges && (which.compare("Fillet") == 0 || which.compare("Chamfer") == 0)) {
        FCMD_OBJ_CMD(Feat, "UseAllEdges = True");
    }
    cmd->doCommand(cmd->Gui, "Gui.Selection.clearSelection()");
    finishFeature(cmd, Feat, base);

    App::DocumentObject* baseFeature = static_cast<PartDesign::DressUp*>(Feat)->Base.getValue();
    if (baseFeature) {
        PartDesignGui::ViewProvider* view = dynamic_cast<PartDesignGui::ViewProvider*>(
            Gui::Application::Instance->getViewProvider(baseFeature)
        );
        // in case there is an error, for example when a fillet is larger than the available space
        // display the base feature to avoid that the user sees nothing
        if (view && Feat->isError()) {
            view->Visibility.setValue(true);
        }
    }
}

void makeChamferOrFillet(Gui::Command* cmd, const std::string& which)
{
    bool useAllEdges = false;
    bool noSelection = false;
    Gui::SelectionObject selected;
    if (!dressupGetSelected(cmd, which, selected, useAllEdges, noSelection)) {
        return;
    }

    Part::Feature* base;
    std::vector<std::string> SubNames;
    if (noSelection) {
        base = static_cast<Part::Feature*>(PartDesignGui::getBody(true)->Tip.getValue());
    }
    else {
        base = static_cast<Part::Feature*>(selected.getObject());
        SubNames = std::vector<std::string>(selected.getSubNames());
    }

    finishDressupFeature(cmd, which, base, SubNames, useAllEdges);
}

//===========================================================================
// PartDesign_Fillet
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignFillet)

CmdPartDesignFillet::CmdPartDesignFillet()
    : Command("PartDesign_Fillet")
{
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Fillet");
    sToolTipText = QT_TR_NOOP("Applies a fillet to the selected edges or faces");
    sWhatsThis = "PartDesign_Fillet";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_Fillet";
}

void CmdPartDesignFillet::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    makeChamferOrFillet(this, "Fillet");
}

bool CmdPartDesignFillet::isActive()
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Chamfer
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignChamfer)

CmdPartDesignChamfer::CmdPartDesignChamfer()
    : Command("PartDesign_Chamfer")
{
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Chamfer");
    sToolTipText = QT_TR_NOOP("Applies a chamfer to the selected edges or faces");
    sWhatsThis = "PartDesign_Chamfer";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_Chamfer";
}

void CmdPartDesignChamfer::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    makeChamferOrFillet(this, "Chamfer");
    doCommand(Gui, "Gui.Selection.clearSelection()");
}

bool CmdPartDesignChamfer::isActive()
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Draft
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignDraft)

CmdPartDesignDraft::CmdPartDesignDraft()
    : Command("PartDesign_Draft")
{
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Draft");
    sToolTipText = QT_TR_NOOP("Applies a draft to the selected faces");
    sWhatsThis = "PartDesign_Draft";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_Draft";
}

void CmdPartDesignDraft::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::SelectionObject selected;
    bool useAllEdges = false;
    bool noSelection = false;
    if (!dressupGetSelected(this, "Draft", selected, useAllEdges, noSelection)) {
        return;
    }

    Part::Feature* base;
    std::vector<std::string> SubNames;
    if (noSelection) {
        base = static_cast<Part::Feature*>(PartDesignGui::getBody(true)->Tip.getValue());
    }
    else {
        base = static_cast<Part::Feature*>(selected.getObject());
        SubNames = std::vector<std::string>(selected.getSubNames());

        const Part::TopoShape& TopShape = base->Shape.getShape();

        // filter out the edges
        size_t i = 0;
        while (i < SubNames.size()) {
            std::string aSubName = SubNames.at(i);

            if (aSubName.compare(0, 4, "Face") == 0) {
                // Check for valid face types
                TopoDS_Face face = TopoDS::Face(TopShape.getSubShape(aSubName.c_str()));
                BRepAdaptor_Surface sf(face);
                if ((sf.GetType() != GeomAbs_Plane) && (sf.GetType() != GeomAbs_Cylinder)
                    && (sf.GetType() != GeomAbs_Cone)) {
                    SubNames.erase(SubNames.begin() + i);
                }
            }
            else {
                // empty name or any other sub-element
                SubNames.erase(SubNames.begin() + i);
            }

            i++;
        }
    }

    finishDressupFeature(this, "Draft", base, SubNames, useAllEdges);
}

bool CmdPartDesignDraft::isActive()
{
    return hasActiveDocument();
}


//===========================================================================
// PartDesign_Thickness
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignThickness)

CmdPartDesignThickness::CmdPartDesignThickness()
    : Command("PartDesign_Thickness")
{
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Thickness");
    sToolTipText = QT_TR_NOOP("Applies thickness and removes the selected faces");
    sWhatsThis = "PartDesign_Thickness";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_Thickness";
}

void CmdPartDesignThickness::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::SelectionObject selected;
    bool useAllEdges = false;
    bool noSelection = false;
    if (!dressupGetSelected(this, "Thickness", selected, useAllEdges, noSelection)) {
        return;
    }


    Part::Feature* base;
    std::vector<std::string> SubNames;
    if (noSelection) {
        base = static_cast<Part::Feature*>(PartDesignGui::getBody(true)->Tip.getValue());
    }
    else {
        base = static_cast<Part::Feature*>(selected.getObject());
        SubNames = std::vector<std::string>(selected.getSubNames());

        // filter out the edges
        size_t i = 0;
        while (i < SubNames.size()) {
            std::string aSubName = SubNames.at(i);

            if (aSubName.compare(0, 4, "Face") != 0) {
                // empty name or any other sub-element
                SubNames.erase(SubNames.begin() + i);
            }
            i++;
        }
    }

    finishDressupFeature(this, "Thickness", base, SubNames, useAllEdges);
}

bool CmdPartDesignThickness::isActive()
{
    return hasActiveDocument();
}

//===========================================================================
// Common functions for all Transformed features
//===========================================================================

void prepareTransformed(
    PartDesign::Body* pcActiveBody,
    Gui::Command* cmd,
    const std::string& which,
    std::function<void(App::DocumentObject*, std::vector<App::DocumentObject*>)> func
)
{
    std::string FeatName = cmd->getUniqueObjectName(which.c_str(), pcActiveBody);

    auto worker = [=](std::vector<App::DocumentObject*> features) {
        std::string msg("Make ");
        msg += which;
        Gui::Command::openCommand(msg.c_str());
        FCMD_OBJ_CMD(pcActiveBody, "newObject('PartDesign::" << which << "','" << FeatName << "')");
        // FIXME: There seems to be kind of a race condition here, leading to sporadic errors like
        // Exception (Thu Sep  6 11:52:01 2012): 'App.Document' object has no attribute 'Mirrored'
        Gui::Command::updateActive();  // Helps to ensure that the object already exists when the
                                       // next command comes up

        auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());

        if (features.empty()) {
            FCMD_OBJ_CMD(Feat, "TransformMode = \"Whole shape\"");
        }
        else {
            std::stringstream str;
            str << "Originals = [";
            for (auto feature : features) {
                str << cmd->getObjectCmd(feature) << ",";
            }
            str << "]";
            FCMD_OBJ_CMD(Feat, str.str().c_str());
        }

        // TODO What is this function supposed to do? (2015-08-05, Fat-Zer)
        func(Feat, features);

        // Set the tip of the body
        FCMD_OBJ_CMD(pcActiveBody, "Tip = " << Gui::Command::getObjectCmd(Feat));
        Gui::Command::updateActive();
    };

    // Get a valid original from the user
    std::vector<App::DocumentObject*> features = cmd->getSelection().getObjectsOfType(
        PartDesign::Feature::getClassTypeId()
    );

    PartDesign::Body* activeBody = PartDesignGui::getBody(true);
    for (auto feature : features) {
        if (activeBody != PartDesignGui::getBodyFor(feature, false)) {
            QMessageBox::warning(
                Gui::getMainWindow(),
                QObject::tr("Selection is not in the active body"),
                QObject::tr("Please select only one feature in an active body.")
            );
            return;
        }
    }
    worker(features);
}

void finishTransformed(Gui::Command* cmd, App::DocumentObject* Feat)
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
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Mirror");
    sToolTipText = QT_TR_NOOP("Mirrors the selected features or active body");
    sWhatsThis = "PartDesign_Mirrored";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_Mirrored";
}

void CmdPartDesignMirrored::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody) {
        return;
    }

    Gui::Command* cmd = this;
    auto worker = [cmd,
                   pcActiveBody](App::DocumentObject* Feat, std::vector<App::DocumentObject*> features) {
        bool direction = false;
        if (!features.empty() && features.front()->isDerivedFrom<PartDesign::ProfileBased>()) {
            Part::Part2DObject* sketch = (static_cast<PartDesign::ProfileBased*>(features.front()))
                                             ->getVerifiedSketch(/* silent =*/true);
            if (sketch) {
                FCMD_OBJ_CMD(Feat, "MirrorPlane = (" << getObjectCmd(sketch) << ", ['V_Axis'])");
                direction = true;
            }
        }
        if (!direction) {
            FCMD_OBJ_CMD(
                Feat,
                "MirrorPlane = (" << getObjectCmd(pcActiveBody->getOrigin()->getXY()) << ", [''])"
            );
        }

        finishTransformed(cmd, Feat);
    };

    prepareTransformed(pcActiveBody, this, "Mirrored", worker);
}

bool CmdPartDesignMirrored::isActive()
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
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Linear Pattern");
    sToolTipText = QT_TR_NOOP(
        "Duplicates the selected features or the active body in a linear pattern"
    );
    sWhatsThis = "PartDesign_LinearPattern";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_LinearPattern";
}

void CmdPartDesignLinearPattern::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody) {
        return;
    }

    Gui::Command* cmd = this;
    auto worker =
        [cmd, pcActiveBody](App::DocumentObject* Feat, std::vector<App::DocumentObject*> features) {
            bool direction = false;
            if (!features.empty() && features.front()->isDerivedFrom<PartDesign::ProfileBased>()) {
                Part::Part2DObject* sketch = (static_cast<PartDesign::ProfileBased*>(features.front()))
                                                 ->getVerifiedSketch(/* silent =*/true);
                if (sketch) {
                    FCMD_OBJ_CMD(
                        Feat,
                        "Direction = (" << Gui::Command::getObjectCmd(sketch) << ", ['H_Axis'])"
                    );
                    FCMD_OBJ_CMD(
                        Feat,
                        "Direction2 = (" << Gui::Command::getObjectCmd(sketch) << ", ['V_Axis'])"
                    );
                    direction = true;
                }
            }
            if (!direction) {
                FCMD_OBJ_CMD(
                    Feat,
                    "Direction = (" << Gui::Command::getObjectCmd(pcActiveBody->getOrigin()->getX())
                                    << ",[''])"
                );
            }
            FCMD_OBJ_CMD(Feat, "Length = 100");
            FCMD_OBJ_CMD(Feat, "Occurrences = 2");

            finishTransformed(cmd, Feat);
        };

    prepareTransformed(pcActiveBody, this, "LinearPattern", worker);
}

bool CmdPartDesignLinearPattern::isActive()
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
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Polar Pattern");
    sToolTipText = QT_TR_NOOP(
        "Duplicates the selected features or the active body in a circular pattern"
    );
    sWhatsThis = "PartDesign_PolarPattern";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_PolarPattern";
}

void CmdPartDesignPolarPattern::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody) {
        return;
    }

    Gui::Command* cmd = this;
    auto worker = [cmd,
                   pcActiveBody](App::DocumentObject* Feat, std::vector<App::DocumentObject*> features) {
        bool direction = false;
        if (!features.empty() && features.front()->isDerivedFrom<PartDesign::ProfileBased>()) {
            Part::Part2DObject* sketch = (static_cast<PartDesign::ProfileBased*>(features.front()))
                                             ->getVerifiedSketch(/* silent =*/true);
            if (sketch) {
                FCMD_OBJ_CMD(Feat, "Axis = (" << Gui::Command::getObjectCmd(sketch) << ",['N_Axis'])");
                direction = true;
            }
        }
        if (!direction) {
            FCMD_OBJ_CMD(
                Feat,
                "Axis = (" << Gui::Command::getObjectCmd(pcActiveBody->getOrigin()->getZ()) << ",[''])"
            );
        }

        FCMD_OBJ_CMD(Feat, "Angle = 360");
        FCMD_OBJ_CMD(Feat, "Occurrences = 2");

        finishTransformed(cmd, Feat);
    };

    prepareTransformed(pcActiveBody, this, "PolarPattern", worker);
}

bool CmdPartDesignPolarPattern::isActive()
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
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Scale");
    sToolTipText = QT_TR_NOOP("Scales the selected features or the active body");
    sWhatsThis = "PartDesign_Scaled";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_Scaled";
}

void CmdPartDesignScaled::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody) {
        return;
    }

    Gui::Command* cmd = this;
    auto worker = [cmd](App::DocumentObject* Feat, std::vector<App::DocumentObject*> /*features*/) {
        FCMD_OBJ_CMD(Feat, "Factor = 2");
        FCMD_OBJ_CMD(Feat, "Occurrences = 2");

        finishTransformed(cmd, Feat);
    };

    prepareTransformed(pcActiveBody, this, "Scale", worker);
}

bool CmdPartDesignScaled::isActive()
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
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Multi-Transform");
    sToolTipText = QT_TR_NOOP(
        "Applies multiple transformations to the selected features or active body"
    );
    sWhatsThis = "PartDesign_MultiTransform";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_MultiTransform";
}

void CmdPartDesignMultiTransform::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody) {
        return;
    }

    std::vector<App::DocumentObject*> features;

    // Check if a Transformed feature has been selected, convert it to MultiTransform
    features = getSelection().getObjectsOfType(PartDesign::Transformed::getClassTypeId());
    if (!features.empty()) {
        // Throw out MultiTransform features, we don't want to nest them
        for (std::vector<App::DocumentObject*>::iterator f = features.begin(); f != features.end();) {
            if ((*f)->isDerivedFrom<PartDesign::MultiTransform>()) {
                f = features.erase(f);
            }
            else {
                f++;
            }
        }

        if (features.empty()) {
            return;
        }
        // Note: If multiple Transformed features were selected, only the first one is used
        PartDesign::Transformed* trFeat = static_cast<PartDesign::Transformed*>(features.front());

        // Move the insert point back one feature
        App::DocumentObject* oldTip = nullptr;
        App::DocumentObject* prevFeature = nullptr;
        if (pcActiveBody) {
            oldTip = pcActiveBody->Tip.getValue();
            prevFeature = pcActiveBody->getPrevSolidFeature(trFeat);
        }
        Gui::Selection().clearSelection();
        if (prevFeature) {
            Gui::Selection().addSelection(
                prevFeature->getDocument()->getName(),
                prevFeature->getNameInDocument()
            );
        }

        openCommand(QT_TRANSLATE_NOOP("Command", "Convert to Multi-Transform feature"));

        Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
        rcCmdMgr.runCommandByName("PartDesign_MoveTip");

        // We cannot remove the Transform feature from the body as otherwise
        // we will have a PartDesign feature without a body which is not allowed
        // and causes to pop up the migration dialog later when adding new features
        // to the body.
        // Additionally it creates the error message: "Links go out of the allowed scope"
        // #0003509
#if 0
        // Remove the Transformed feature from the Body
        if (pcActiveBody)
            FCMD_OBJ_CMD(pcActiveBody,"removeObject("<<getObjectCmd(trFeat)<<")");
#endif

        // Create a MultiTransform feature and move the Transformed feature inside it
        std::string FeatName = getUniqueObjectName("MultiTransform", pcActiveBody);
        FCMD_OBJ_CMD(pcActiveBody, "newObject('PartDesign::MultiTransform','" << FeatName << "')");
        auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());
        auto objCmd = getObjectCmd(trFeat);
        FCMD_OBJ_CMD(Feat, "Originals = " << objCmd << ".Originals");
        FCMD_OBJ_CMD(Feat, "TransformMode = " << objCmd << ".TransformMode");
        FCMD_OBJ_CMD(Feat, "BaseFeature = " << objCmd << ".BaseFeature");
        FCMD_OBJ_CMD(Feat, "Transformations = [" << objCmd << "]");

        FCMD_OBJ_CMD(trFeat, "Originals = []");

        // Add the MultiTransform into the Body at the current insert point
        finishFeature(this, Feat);

        // Restore the insert point
        if (pcActiveBody && oldTip != trFeat) {
            Gui::Selection().clearSelection();
            Gui::Selection().addSelection(oldTip->getDocument()->getName(), oldTip->getNameInDocument());
            rcCmdMgr.runCommandByName("PartDesign_MoveTip");
            Gui::Selection().clearSelection();
        }  // otherwise the insert point remains at the new MultiTransform, which is fine
    }
    else {

        Gui::Command* cmd = this;
        auto worker =
            [cmd,
             pcActiveBody](App::DocumentObject* Feat, std::vector<App::DocumentObject*> /*features*/) {
                // Make sure the user isn't presented with an empty screen because no
                // transformations are defined yet...
                App::DocumentObject* prevSolid = pcActiveBody->Tip.getValue();
                if (prevSolid) {
                    Part::Feature* feat = static_cast<Part::Feature*>(prevSolid);
                    FCMD_OBJ_CMD(Feat, "Shape = " << getObjectCmd(feat) << ".Shape");
                }
                finishFeature(cmd, Feat);
            };

        prepareTransformed(pcActiveBody, this, "MultiTransform", worker);
    }
}

bool CmdPartDesignMultiTransform::isActive()
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Boolean
//===========================================================================

/* Boolean commands =======================================================*/
DEF_STD_CMD_A(CmdPartDesignBoolean)

CmdPartDesignBoolean::CmdPartDesignBoolean()
    : Command("PartDesign_Boolean")
{
    sAppModule = "PartDesign";
    sGroup = QT_TR_NOOP("PartDesign");
    sMenuText = QT_TR_NOOP("Boolean Operation");
    sToolTipText = QT_TR_NOOP(
        "Applies boolean operations with the selected objects and the active body"
    );
    sWhatsThis = "PartDesign_Boolean";
    sStatusTip = sToolTipText;
    sPixmap = "PartDesign_Boolean";
}


void CmdPartDesignBoolean::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    PartDesign::Body* pcActiveBody = PartDesignGui::getBody(/*messageIfNot = */ true);
    if (!pcActiveBody) {
        return;
    }

    Gui::SelectionFilter BodyFilter("SELECT Part::Feature COUNT 1..");

    openCommand(QT_TRANSLATE_NOOP("Command", "Create Boolean"));
    std::string FeatName = getUniqueObjectName("Boolean", pcActiveBody);
    FCMD_OBJ_CMD(pcActiveBody, "newObject('PartDesign::Boolean','" << FeatName << "')");
    auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());

    // If we don't add an object to the boolean group then don't update the body
    // as otherwise this will fail and it will be marked as invalid
    bool updateDocument = false;
    if (BodyFilter.match() && !BodyFilter.Result.empty()) {
        std::vector<App::DocumentObject*> bodies;
        for (auto& results : BodyFilter.Result) {
            for (auto& result : results) {
                if (result.getObject() != pcActiveBody) {
                    bodies.push_back(result.getObject());
                }
            }
        }
        if (!bodies.empty()) {
            updateDocument = true;
            std::string bodyString = PartDesignGui::buildLinkListPythonStr(bodies);
            FCMD_OBJ_CMD(Feat, "addObjects(" << bodyString << ")");
        }
    }

    finishFeature(this, Feat, nullptr, false, updateDocument);
}

bool CmdPartDesignBoolean::isActive()
{
    if (getActiveGuiDocument()) {
        return true;
    }
    else {
        return false;
    }
}

// Command group for datums =============================================

class CmdPartDesignCompDatums: public Gui::GroupCommand
{
public:
    CmdPartDesignCompDatums()
        : GroupCommand("PartDesign_CompDatums")
    {
        sAppModule = "PartDesign";
        sGroup = "PartDesign";
        sMenuText = QT_TR_NOOP("Create Datum");
        sToolTipText = QT_TR_NOOP("Creates a datum object or local coordinate system");
        sWhatsThis = "PartDesign_CompDatums";
        sStatusTip = sToolTipText;
        eType = ForEdit;

        setCheckable(false);

        addCommand("PartDesign_Plane");
        addCommand("PartDesign_Line");
        addCommand("PartDesign_Point");
        addCommand("PartDesign_CoordinateSystem");
    }

    const char* className() const override
    {
        return "CmdPartDesignCompDatums";
    }

    bool isActive() override
    {
        return (hasActiveDocument() && !Gui::Control().activeDialog());
    }
};

// Command group for datums =============================================

class CmdPartDesignCompSketches: public Gui::GroupCommand
{
public:
    CmdPartDesignCompSketches()
        : GroupCommand("PartDesign_CompSketches")
    {
        sAppModule = "PartDesign";
        sGroup = "PartDesign";
        sMenuText = QT_TR_NOOP("Create Datum");
        sToolTipText = QT_TR_NOOP("Creates a datum object or local coordinate system");
        sWhatsThis = "PartDesign_CompDatums";
        sStatusTip = sToolTipText;
        eType = ForEdit;

        setCheckable(false);
        setRememberLast(false);

        addCommand("PartDesign_NewSketch");
        addCommand("Sketcher_MapSketch");
        addCommand("Sketcher_EditSketch");
    }

    const char* className() const override
    {
        return "CmdPartDesignCompSketches";
    }

    bool isActive() override
    {
        return (hasActiveDocument() && !Gui::Control().activeDialog());
    }
};

//===========================================================================
// Initialization
//===========================================================================

void CreatePartDesignCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

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
    rcCmdMgr.addCommand(new CmdPartDesignAdditiveHelix);
    rcCmdMgr.addCommand(new CmdPartDesignSubtractiveHelix);

    rcCmdMgr.addCommand(new CmdPartDesignFillet());
    rcCmdMgr.addCommand(new CmdPartDesignDraft());
    rcCmdMgr.addCommand(new CmdPartDesignChamfer());
    rcCmdMgr.addCommand(new CmdPartDesignThickness());

    rcCmdMgr.addCommand(new CmdPartDesignMirrored());
    rcCmdMgr.addCommand(new CmdPartDesignLinearPattern());
    rcCmdMgr.addCommand(new CmdPartDesignPolarPattern());
    // rcCmdMgr.addCommand(new CmdPartDesignScaled());
    rcCmdMgr.addCommand(new CmdPartDesignMultiTransform());

    rcCmdMgr.addCommand(new CmdPartDesignBoolean());
    rcCmdMgr.addCommand(new CmdPartDesignCompDatums());
    rcCmdMgr.addCommand(new CmdPartDesignCompSketches());
}
