/***************************************************************************
 *   Copyright (c) 2014 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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
#include <cfloat>
#include <memory>

#include <QApplication>
#include <QClipboard>
#include <QMessageBox>

#include <Inventor/SbString.h>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Notifications.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Mod/Sketcher/App/PythonConverter.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/App/SolverGeometryExtension.h>

#include "DrawSketchHandler.h"
#include "SketchRectangularArrayDialog.h"
#include "Utils.h"
#include "ViewProviderSketch.h"

#include "DrawSketchHandlerTranslate.h"
#include "DrawSketchHandlerOffset.h"
#include "DrawSketchHandlerRotate.h"
#include "DrawSketchHandlerScale.h"
#include "DrawSketchHandlerSymmetry.h"

// Hint: this is to prevent to re-format big parts of the file. Remove it later again.
// clang-format off
using namespace std;
using namespace SketcherGui;
using namespace Sketcher;

std::vector<int> getListOfSelectedGeoIds(bool forceInternalSelection)
{
    std::vector<int> listOfGeoIds = {};

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = Gui::Selection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("Wrong selection"),
            QObject::tr("Select elements from a single sketch."));
        return {};
    }

    // get the needed lists and objects
    auto* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
    const std::vector<std::string>& subNames = selection[0].getSubNames();
    if (!subNames.empty()) {

        for (auto& name : subNames) {
            if (name.size() > 4 && name.substr(0, 4) == "Edge") {
                int geoId = std::atoi(name.substr(4, 4000).c_str()) - 1;
                listOfGeoIds.push_back(geoId);
            }
            else if (name.size() > 12 && name.substr(0, 12) == "ExternalEdge") {
                int geoId = -std::atoi(name.substr(12, 4000).c_str()) - 2;
                listOfGeoIds.push_back(geoId);
            }
            else if (name.size() > 6 && name.substr(0, 6) == "Vertex") {
                // only if it is a GeomPoint
                int VtId = std::atoi(name.substr(6, 4000).c_str()) - 1;
                int geoId;
                Sketcher::PointPos PosId;
                Obj->getGeoVertexIndex(VtId, geoId, PosId);
                if (isPoint(*Obj->getGeometry(geoId))) {
                    if (geoId >= 0) {
                        listOfGeoIds.push_back(geoId);
                    }
                }
            }
        }
    }

    if (forceInternalSelection) {
        size_t loopSize = listOfGeoIds.size();
        for (size_t i = 0; i < loopSize; i++) {
            const Part::Geometry* geo = Obj->getGeometry(listOfGeoIds[i]);
            if (isEllipse(*geo) || isArcOfEllipse(*geo) || isArcOfHyperbola(*geo) || isArcOfParabola(*geo) || isBSplineCurve(*geo)) {
                const std::vector<Sketcher::Constraint*>& constraints = Obj->Constraints.getValues();
                for (auto constr : constraints) {
                    if (constr->Type == InternalAlignment && constr->Second == listOfGeoIds[i]) {
                        if (std::find(listOfGeoIds.begin(), listOfGeoIds.end(), constr->First) == listOfGeoIds.end()) {
                            // If the value is not found, add it to the vector
                            listOfGeoIds.push_back(constr->First);
                        }
                    }
                }
            }
        }
    }

    if (listOfGeoIds.empty()) {
        Gui::NotifyUserError(Obj,
            QT_TRANSLATE_NOOP("Notifications", "Invalid selection"),
            QT_TRANSLATE_NOOP("Notifications", "Selection has no valid geometries."));
    }

    return listOfGeoIds;
}

Sketcher::SketchObject* getSketchObject()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    ReleaseHandler(doc);
    auto* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    return vp->getSketchObject();
}

// ================================================================================

// Copy

bool copySelectionToClipboard(Sketcher::SketchObject* obj) {
    std::vector<int> listOfGeoId = getListOfSelectedGeoIds(true);
    if (listOfGeoId.empty()) { return false; }
    sort(listOfGeoId.begin(), listOfGeoId.end());

    //Export selected geometries as a formatted string.
    std::vector<Part::Geometry*> shapeGeometry;
    for (auto geoId : listOfGeoId) {
        Part::Geometry* geoNew = obj->getGeometry(geoId)->copy();
        shapeGeometry.push_back(geoNew);
    }
    std::string geosAsStr = Sketcher::PythonConverter::convert(
        "objectStr",
        shapeGeometry,
        Sketcher::PythonConverter::Mode::OmitInternalGeometry);

    // Export constraints of selected geos.
    std::vector<Sketcher::Constraint*> shapeConstraints;
    for (auto constr : obj->Constraints.getValues()) {

        auto isSelectedGeoOrAxis = [](const std::vector<int>& vec, int value) {
            return (std::find(vec.begin(), vec.end(), value) != vec.end())
                || value == GeoEnum::GeoUndef || value == GeoEnum::RtPnt
                || value == GeoEnum::VAxis || value == GeoEnum::HAxis;
        };

        if (!isSelectedGeoOrAxis(listOfGeoId, constr->First)
            || !isSelectedGeoOrAxis(listOfGeoId, constr->Second)
            || !isSelectedGeoOrAxis(listOfGeoId, constr->Third)) {
            continue;
        }

        Constraint* temp = constr->copy();
        for (size_t j = 0; j < listOfGeoId.size(); j++) {
            if (temp->First == listOfGeoId[j]) {
                temp->First = j;
            }
            if (temp->Second == listOfGeoId[j]) {
                temp->Second = j;
            }
            if (temp->Third == listOfGeoId[j]) {
                temp->Third = j;
            }
        }
        shapeConstraints.push_back(temp);
    }
    std::string cstrAsStr = Sketcher::PythonConverter::convert("objectStr", shapeConstraints, Sketcher::PythonConverter::GeoIdMode::AddLastGeoIdToGeoIds);

    std::string exportedData = "# Copied from sketcher. From:\n#objectStr = " + Gui::Command::getObjectCmd(obj) + "\n"
        + geosAsStr + "\n" + cstrAsStr;

    if (!exportedData.empty()) {
        QClipboard* clipboard = QGuiApplication::clipboard();
        clipboard->setText(QString::fromStdString(exportedData));
        return true;
    }
    return false;
}

DEF_STD_CMD_A(CmdSketcherCopyClipboard)

CmdSketcherCopyClipboard::CmdSketcherCopyClipboard()
    : Command("Sketcher_CopyClipboard")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("C&opy in sketcher");
    sToolTipText = QT_TR_NOOP("Copy selected geometries and constraints to the clipboard");
    sWhatsThis = "Sketcher_CopyClipboard";
    sStatusTip = sToolTipText;
    sPixmap = "edit-copy";
    sAccel = keySequenceToAccel(QKeySequence::Copy);
    eType = ForEdit;
}

void CmdSketcherCopyClipboard::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    copySelectionToClipboard(getSketchObject());
}

bool CmdSketcherCopyClipboard::isActive()
{
    return isCommandActive(getActiveGuiDocument(), true);
}

// ================================================================================

// Cut

DEF_STD_CMD_A(CmdSketcherCut)

CmdSketcherCut::CmdSketcherCut()
    : Command("Sketcher_Cut")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("C&ut in sketcher");
    sToolTipText = QT_TR_NOOP("Cut selected geometries and constraints to the clipboard");
    sWhatsThis = "Sketcher_Cut";
    sStatusTip = sToolTipText;
    sPixmap = "edit-cut";
    sAccel = keySequenceToAccel(QKeySequence::Cut);
    eType = ForEdit;
}

void CmdSketcherCut::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (copySelectionToClipboard(getSketchObject())) {

        Gui::Document* doc = getActiveGuiDocument();
        ReleaseHandler(doc);
        auto* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());

        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cut in Sketcher"));
        vp->deleteSelected();
        Gui::Command::commitCommand();
    }
}

bool CmdSketcherCut::isActive()
{
    return isCommandActive(getActiveGuiDocument(), true);
}

// ================================================================================

// Paste

DEF_STD_CMD_A(CmdSketcherPaste)

CmdSketcherPaste::CmdSketcherPaste()
    : Command("Sketcher_Paste")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("P&aste in sketcher");
    sToolTipText = QT_TR_NOOP("Paste selected geometries and constraints from the clipboard");
    sWhatsThis = "Sketcher_Paste";
    sStatusTip = sToolTipText;
    sPixmap = "edit-paste";
    sAccel = keySequenceToAccel(QKeySequence::Paste);
    eType = ForEdit;
}

void CmdSketcherPaste::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document* doc = getActiveGuiDocument();
    ReleaseHandler(doc);
    auto* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    Sketcher::SketchObject* obj = vp->getSketchObject();

    std::string data = QGuiApplication::clipboard()->text().toStdString();
    if (data.find("# Copied from sketcher.", 0) == std::string::npos) {
        return;
    }
    data = "objectStr = " + Gui::Command::getObjectCmd(obj) +"\n" + data;

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Paste in Sketcher"));

    Gui::Command::doCommand(Gui::Command::Doc, data.c_str());

    obj->solve(true);
    vp->draw(false, false);

    Gui::Command::commitCommand();
}

bool CmdSketcherPaste::isActive()
{
    return isCommandActive(getActiveGuiDocument(), false);
}

// ================================================================================

// Select Constraints of selected elements
DEF_STD_CMD_A(CmdSketcherSelectConstraints)

CmdSketcherSelectConstraints::CmdSketcherSelectConstraints()
    : Command("Sketcher_SelectConstraints")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Select associated constraints");
    sToolTipText =
        QT_TR_NOOP("Select the constraints associated with the selected geometrical elements");
    sWhatsThis = "Sketcher_SelectConstraints";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_SelectConstraints";
    sAccel = "Z, K";
    eType = ForEdit;
}

void CmdSketcherSelectConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // Cancel any in-progress operation
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    SketcherGui::ReleaseHandler(doc);

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        Gui::TranslatedUserWarning(doc->getDocument(),
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Select elements from a single sketch."));

        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
    const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();

    getSelection().clearSelection();

    std::vector<std::string> constraintSubNames;
    // go through the selected subelements
    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
         ++it) {
        // only handle edges
        if (it->size() > 4 && it->substr(0, 4) == "Edge") {
            int GeoId = std::atoi(it->substr(4, 4000).c_str()) - 1;

            // push all the constraints
            int i = 0;
            for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin();
                 it != vals.end();
                 ++it, ++i) {
                if ((*it)->First == GeoId || (*it)->Second == GeoId || (*it)->Third == GeoId) {
                    constraintSubNames.push_back(
                        Sketcher::PropertyConstraintList::getConstraintName(i));
                }
            }
        }
    }

    if (!constraintSubNames.empty())
        Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), constraintSubNames);
}

bool CmdSketcherSelectConstraints::isActive()
{
    return isCommandActive(getActiveGuiDocument(), true);
}

// ================================================================================

// Select Origin
DEF_STD_CMD_A(CmdSketcherSelectOrigin)

CmdSketcherSelectOrigin::CmdSketcherSelectOrigin()
    : Command("Sketcher_SelectOrigin")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Select origin");
    sToolTipText = QT_TR_NOOP("Select the local origin point of the sketch");
    sWhatsThis = "Sketcher_SelectOrigin";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_SelectOrigin";
    sAccel = "Z, O";
    eType = ForEdit;
}

void CmdSketcherSelectOrigin::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Sketcher::SketchObject * Obj = getSketchObject();
    //    ViewProviderSketch * vp = static_cast<ViewProviderSketch
    //    *>(Gui::Application::Instance->getViewProvider(docobj)); Sketcher::SketchObject* Obj =
    //    vp->getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();
    std::stringstream ss;

    ss << "RootPoint";

    if (Gui::Selection().isSelected(doc_name.c_str(), obj_name.c_str(), ss.str().c_str()))
        Gui::Selection().rmvSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
    else
        Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
}

bool CmdSketcherSelectOrigin::isActive()
{
    return isCommandActive(getActiveGuiDocument(), false);
}

// ================================================================================

// Select Vertical Axis
DEF_STD_CMD_A(CmdSketcherSelectVerticalAxis)

CmdSketcherSelectVerticalAxis::CmdSketcherSelectVerticalAxis()
    : Command("Sketcher_SelectVerticalAxis")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Select vertical axis");
    sToolTipText = QT_TR_NOOP("Select the local vertical axis of the sketch");
    sWhatsThis = "Sketcher_SelectVerticalAxis";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_SelectVerticalAxis";
    sAccel = "Z, V";
    eType = ForEdit;
}

void CmdSketcherSelectVerticalAxis::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Sketcher::SketchObject* Obj = getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();
    std::stringstream ss;

    ss << "V_Axis";

    if (Gui::Selection().isSelected(doc_name.c_str(), obj_name.c_str(), ss.str().c_str()))
        Gui::Selection().rmvSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
    else
        Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
}

bool CmdSketcherSelectVerticalAxis::isActive()
{
    return isCommandActive(getActiveGuiDocument(), false);
}

// ================================================================================

// Select Horizontal Axis
DEF_STD_CMD_A(CmdSketcherSelectHorizontalAxis)

CmdSketcherSelectHorizontalAxis::CmdSketcherSelectHorizontalAxis()
    : Command("Sketcher_SelectHorizontalAxis")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Select horizontal axis");
    sToolTipText = QT_TR_NOOP("Select the local horizontal axis of the sketch");
    sWhatsThis = "Sketcher_SelectHorizontalAxis";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_SelectHorizontalAxis";
    sAccel = "Z, H";
    eType = ForEdit;
}

void CmdSketcherSelectHorizontalAxis::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Sketcher::SketchObject* Obj = getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();
    std::stringstream ss;

    ss << "H_Axis";

    if (Gui::Selection().isSelected(doc_name.c_str(), obj_name.c_str(), ss.str().c_str()))
        Gui::Selection().rmvSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
    else
        Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
}

bool CmdSketcherSelectHorizontalAxis::isActive()
{
    return isCommandActive(getActiveGuiDocument(), false);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherSelectRedundantConstraints)

CmdSketcherSelectRedundantConstraints::CmdSketcherSelectRedundantConstraints()
    : Command("Sketcher_SelectRedundantConstraints")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Select redundant constraints");
    sToolTipText = QT_TR_NOOP("Select redundant constraints");
    sWhatsThis = "Sketcher_SelectRedundantConstraints";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_SelectRedundantConstraints";
    sAccel = "Z, P, R";
    eType = ForEdit;
}

void CmdSketcherSelectRedundantConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Sketcher::SketchObject* Obj = getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();

    // get the needed lists and objects
    const std::vector<int>& solverredundant = Obj->getLastRedundant();
    const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();

    getSelection().clearSelection();

    // push the constraints
    std::vector<std::string> constraintSubNames;

    int i = 0;
    for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin(); it != vals.end();
         ++it, ++i) {
        for (std::vector<int>::const_iterator itc = solverredundant.begin();
             itc != solverredundant.end();
             ++itc) {
            if ((*itc) - 1 == i) {
                constraintSubNames.push_back(
                    Sketcher::PropertyConstraintList::getConstraintName(i));
                break;
            }
        }
    }

    if (!constraintSubNames.empty())
        Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), constraintSubNames);
}

bool CmdSketcherSelectRedundantConstraints::isActive()
{
    return isCommandActive(getActiveGuiDocument(), false);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherSelectMalformedConstraints)

CmdSketcherSelectMalformedConstraints::CmdSketcherSelectMalformedConstraints()
    : Command("Sketcher_SelectMalformedConstraints")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Select malformed constraints");
    sToolTipText = QT_TR_NOOP("Select malformed constraints");
    sWhatsThis = "Sketcher_SelectMalformedConstraints";
    sStatusTip = sToolTipText;
    eType = ForEdit;
}

void CmdSketcherSelectMalformedConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Sketcher::SketchObject* Obj = getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();

    // get the needed lists and objects
    const std::vector<int>& solvermalformed = Obj->getLastMalformedConstraints();
    const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();

    getSelection().clearSelection();

    // push the constraints
    std::vector<std::string> constraintSubNames;
    int i = 0;
    for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin(); it != vals.end();
         ++it, ++i) {
        for (std::vector<int>::const_iterator itc = solvermalformed.begin();
             itc != solvermalformed.end();
             ++itc) {
            if ((*itc) - 1 == i) {
                constraintSubNames.push_back(
                    Sketcher::PropertyConstraintList::getConstraintName(i));
                break;
            }
        }
    }

    if (!constraintSubNames.empty())
        Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), constraintSubNames);
}

bool CmdSketcherSelectMalformedConstraints::isActive()
{
    return isCommandActive(getActiveGuiDocument(), false);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherSelectPartiallyRedundantConstraints)

CmdSketcherSelectPartiallyRedundantConstraints::CmdSketcherSelectPartiallyRedundantConstraints()
    : Command("Sketcher_SelectPartiallyRedundantConstraints")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Select partially redundant constraints");
    sToolTipText = QT_TR_NOOP("Select partially redundant constraints");
    sWhatsThis = "Sketcher_SelectPartiallyRedundantConstraints";
    sStatusTip = sToolTipText;
    eType = ForEdit;
}

void CmdSketcherSelectPartiallyRedundantConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Sketcher::SketchObject* Obj = getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();

    // get the needed lists and objects
    const std::vector<int>& solverpartiallyredundant =
        Obj->getLastPartiallyRedundant();
    const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();

    getSelection().clearSelection();

    // push the constraints
    std::vector<std::string> constraintSubNames;
    int i = 0;
    for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin(); it != vals.end();
         ++it, ++i) {
        for (std::vector<int>::const_iterator itc = solverpartiallyredundant.begin();
             itc != solverpartiallyredundant.end();
             ++itc) {
            if ((*itc) - 1 == i) {
                constraintSubNames.push_back(
                    Sketcher::PropertyConstraintList::getConstraintName(i));
                break;
            }
        }
    }

    if (!constraintSubNames.empty())
        Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), constraintSubNames);
}

bool CmdSketcherSelectPartiallyRedundantConstraints::isActive()
{
    return isCommandActive(getActiveGuiDocument(), false);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherSelectConflictingConstraints)

CmdSketcherSelectConflictingConstraints::CmdSketcherSelectConflictingConstraints()
    : Command("Sketcher_SelectConflictingConstraints")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Select conflicting constraints");
    sToolTipText = QT_TR_NOOP("Select conflicting constraints");
    sWhatsThis = "Sketcher_SelectConflictingConstraints";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_SelectConflictingConstraints";
    sAccel = "Z, P, C";
    eType = ForEdit;
}

void CmdSketcherSelectConflictingConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Sketcher::SketchObject* Obj = getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();

    // get the needed lists and objects
    const std::vector<int>& solverconflicting = Obj->getLastConflicting();
    const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();

    getSelection().clearSelection();

    // push the constraints
    std::vector<std::string> constraintSubNames;
    int i = 0;
    for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin(); it != vals.end();
         ++it, ++i) {
        for (std::vector<int>::const_iterator itc = solverconflicting.begin();
             itc != solverconflicting.end();
             ++itc) {
            if ((*itc) - 1 == i) {
                constraintSubNames.push_back(
                    Sketcher::PropertyConstraintList::getConstraintName(i));
                break;
            }
        }
    }

    if (!constraintSubNames.empty())
        Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), constraintSubNames);
}

bool CmdSketcherSelectConflictingConstraints::isActive()
{
    return isCommandActive(getActiveGuiDocument(), false);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherSelectElementsAssociatedWithConstraints)

CmdSketcherSelectElementsAssociatedWithConstraints::
    CmdSketcherSelectElementsAssociatedWithConstraints()
    : Command("Sketcher_SelectElementsAssociatedWithConstraints")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Select associated geometry");
    sToolTipText =
        QT_TR_NOOP("Select the geometrical elements associated with the selected constraints");
    sWhatsThis = "Sketcher_SelectElementsAssociatedWithConstraints";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_SelectElementsAssociatedWithConstraints";
    sAccel = "Z, E";
    eType = ForEdit;
}

void CmdSketcherSelectElementsAssociatedWithConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    Sketcher::SketchObject* Obj = getSketchObject();

    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();

    getSelection().clearSelection();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();
    std::stringstream ss;

    std::vector<std::string> elementSubNames;
    // go through the selected subelements
    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
         ++it) {
        // only handle constraints
        if (it->size() > 10 && it->substr(0, 10) == "Constraint") {
            int ConstrId = Sketcher::PropertyConstraintList::getIndexFromConstraintName(*it);

            if (ConstrId < static_cast<int>(vals.size())) {
                if (vals[ConstrId]->First != GeoEnum::GeoUndef) {
                    ss.str(std::string());

                    switch (vals[ConstrId]->FirstPos) {
                        case Sketcher::PointPos::none:
                            ss << "Edge" << vals[ConstrId]->First + 1;
                            break;
                        case Sketcher::PointPos::start:
                        case Sketcher::PointPos::end:
                        case Sketcher::PointPos::mid:
                            int vertex = Obj->getVertexIndexGeoPos(vals[ConstrId]->First,
                                                                   vals[ConstrId]->FirstPos);
                            if (vertex > -1)
                                ss << "Vertex" << vertex + 1;
                            break;
                    }
                    elementSubNames.push_back(ss.str());
                }

                if (vals[ConstrId]->Second != GeoEnum::GeoUndef) {
                    ss.str(std::string());

                    switch (vals[ConstrId]->SecondPos) {
                        case Sketcher::PointPos::none:
                            ss << "Edge" << vals[ConstrId]->Second + 1;
                            break;
                        case Sketcher::PointPos::start:
                        case Sketcher::PointPos::end:
                        case Sketcher::PointPos::mid:
                            int vertex = Obj->getVertexIndexGeoPos(vals[ConstrId]->Second,
                                                                   vals[ConstrId]->SecondPos);
                            if (vertex > -1)
                                ss << "Vertex" << vertex + 1;
                            break;
                    }

                    elementSubNames.push_back(ss.str());
                }

                if (vals[ConstrId]->Third != GeoEnum::GeoUndef) {
                    ss.str(std::string());

                    switch (vals[ConstrId]->ThirdPos) {
                        case Sketcher::PointPos::none:
                            ss << "Edge" << vals[ConstrId]->Third + 1;
                            break;
                        case Sketcher::PointPos::start:
                        case Sketcher::PointPos::end:
                        case Sketcher::PointPos::mid:
                            int vertex = Obj->getVertexIndexGeoPos(vals[ConstrId]->Third,
                                                                   vals[ConstrId]->ThirdPos);
                            if (vertex > -1)
                                ss << "Vertex" << vertex + 1;
                            break;
                    }

                    elementSubNames.push_back(ss.str());
                }
            }
        }
    }

    if (elementSubNames.empty()) {
        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("No constraint selected"),
                                   QObject::tr("At least one constraint must be selected"));
    }
    else {
        Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), elementSubNames);
    }
}

bool CmdSketcherSelectElementsAssociatedWithConstraints::isActive()
{
    return isCommandActive(getActiveGuiDocument(), true);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherSelectElementsWithDoFs)

CmdSketcherSelectElementsWithDoFs::CmdSketcherSelectElementsWithDoFs()
    : Command("Sketcher_SelectElementsWithDoFs")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Select under-constrained elements");
    sToolTipText = QT_TR_NOOP("Select geometrical elements where the solver still detects "
                              "unconstrained degrees of freedom.");
    sWhatsThis = "Sketcher_SelectElementsWithDoFs";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_SelectElementsWithDoFs";
    sAccel = "Z, F";
    eType = ForEdit;
}

void CmdSketcherSelectElementsWithDoFs::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    getSelection().clearSelection();
    Sketcher::SketchObject* Obj = getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();
    std::stringstream ss;

    auto geos = Obj->getInternalGeometry();

    std::vector<std::string> elementSubNames;

    auto testselectvertex = [&Obj, &ss, &elementSubNames](int geoId, PointPos pos) {
        ss.str(std::string());

        int vertex = Obj->getVertexIndexGeoPos(geoId, pos);
        if (vertex > -1) {
            ss << "Vertex" << vertex + 1;

            elementSubNames.push_back(ss.str());
        }
    };

    auto testselectedge = [&ss, &elementSubNames](int geoId) {
        ss.str(std::string());

        ss << "Edge" << geoId + 1;
        elementSubNames.push_back(ss.str());
    };

    int geoid = 0;

    for (auto geo : geos) {
        if (geo) {
            if (geo->hasExtension(Sketcher::SolverGeometryExtension::getClassTypeId())) {

                auto solvext = std::static_pointer_cast<const Sketcher::SolverGeometryExtension>(
                    geo->getExtension(Sketcher::SolverGeometryExtension::getClassTypeId()).lock());

                if (solvext->getGeometry()
                    == Sketcher::SolverGeometryExtension::NotFullyConstraint) {
                    // Coded for consistency with getGeometryWithDependentParameters, read the
                    // comments on that function
                    if (solvext->getEdge() == SolverGeometryExtension::Dependent)
                        testselectedge(geoid);
                    if (solvext->getStart() == SolverGeometryExtension::Dependent)
                        testselectvertex(geoid, Sketcher::PointPos::start);
                    if (solvext->getEnd() == SolverGeometryExtension::Dependent)
                        testselectvertex(geoid, Sketcher::PointPos::end);
                    if (solvext->getMid() == SolverGeometryExtension::Dependent)
                        testselectvertex(geoid, Sketcher::PointPos::mid);
                }
            }
        }

        geoid++;
    }

    if (!elementSubNames.empty()) {
        Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), elementSubNames);
    }
}

bool CmdSketcherSelectElementsWithDoFs::isActive()
{
    return isCommandActive(getActiveGuiDocument(), false);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherRestoreInternalAlignmentGeometry)

CmdSketcherRestoreInternalAlignmentGeometry::CmdSketcherRestoreInternalAlignmentGeometry()
    : Command("Sketcher_RestoreInternalAlignmentGeometry")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Show/hide internal geometry");
    sToolTipText = QT_TR_NOOP("Show all internal geometry or hide unused internal geometry");
    sWhatsThis = "Sketcher_RestoreInternalAlignmentGeometry";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Element_Ellipse_All";
    sAccel = "Z, I";
    eType = ForEdit;
}

void CmdSketcherRestoreInternalAlignmentGeometry::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // Cancel any in-progress operation
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    SketcherGui::ReleaseHandler(doc);

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        Gui::TranslatedUserWarning(doc->getDocument(),
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Select elements from a single sketch."));

        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    getSelection().clearSelection();

    // Return GeoId of the SubName only if it is an edge
    auto getEdgeGeoId = [&Obj](const std::string& SubName) {
        int GeoId;
        Sketcher::PointPos PosId;
        getIdsFromName(SubName, Obj, GeoId, PosId);
        if (PosId == Sketcher::PointPos::none)
            return GeoId;
        else
            return (int)GeoEnum::GeoUndef;
    };

    // Tells if the geometry with given GeoId has internal geometry
    auto noInternalGeo = [&Obj](const auto& GeoId) {
        const Part::Geometry* geo = Obj->getGeometry(GeoId);
        bool hasInternalGeo = geo
            && (geo->is<Part::GeomEllipse>()
                || geo->is<Part::GeomArcOfEllipse>()
                || geo->is<Part::GeomArcOfHyperbola>()
                || geo->is<Part::GeomArcOfParabola>()
                || geo->is<Part::GeomBSplineCurve>());
        return !hasInternalGeo;// so it's removed
    };

    std::vector<int> SubGeoIds(SubNames.size());
    std::transform(SubNames.begin(), SubNames.end(), SubGeoIds.begin(), getEdgeGeoId);

    // Handle highest GeoIds first to minimize GeoIds changing
    // TODO: this might not completely resolve GeoIds changing
    std::sort(SubGeoIds.begin(), SubGeoIds.end(), std::greater<>());
    // Keep unique
    SubGeoIds.erase(std::unique(SubGeoIds.begin(), SubGeoIds.end()), SubGeoIds.end());

    // Only for supported types and keep unique
    SubGeoIds.erase(std::remove_if(SubGeoIds.begin(), SubGeoIds.end(), noInternalGeo),
                    SubGeoIds.end());

    // go through the selected subelements
    for (const auto& GeoId : SubGeoIds) {
        int currentgeoid = Obj->getHighestCurveIndex();

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Exposing Internal Geometry"));
            Gui::cmdAppObjectArgs(Obj, "exposeInternalGeometry(%d)", GeoId);

            int aftergeoid = Obj->getHighestCurveIndex();

            if (aftergeoid == currentgeoid) {// if we did not expose anything, deleteunused
                Gui::cmdAppObjectArgs(Obj, "deleteUnusedInternalGeometry(%d)", GeoId);
            }
        }
        catch (const Base::Exception& e) {
            Gui::NotifyUserError(
                Obj, QT_TRANSLATE_NOOP("Notifications", "Invalid Constraint"), e.what());
            Gui::Command::abortCommand();

            tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject*>(Obj));

            return;
        }

        Gui::Command::commitCommand();
        tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject*>(Obj));
    }
}

bool CmdSketcherRestoreInternalAlignmentGeometry::isActive()
{
    return isCommandActive(getActiveGuiDocument(), true);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherSymmetry)

CmdSketcherSymmetry::CmdSketcherSymmetry()
    : Command("Sketcher_Symmetry")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Symmetry");
    sToolTipText =
        QT_TR_NOOP("Creates symmetric of selected geometry. After starting the tool select the reference line or point.");
    sWhatsThis = "Sketcher_Symmetry";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Symmetry";
    sAccel = "Z, S";
    eType = ForEdit;
}

void CmdSketcherSymmetry::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<int> listOfGeoIds = getListOfSelectedGeoIds(true);

    if (!listOfGeoIds.empty()) {
        ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerSymmetry>(listOfGeoIds));
    }
    getSelection().clearSelection();
}

bool CmdSketcherSymmetry::isActive()
{
    return isCommandActive(getActiveGuiDocument(), true);
}

// ================================================================================

class SketcherCopy: public Gui::Command
{
public:
    enum Op
    {
        Copy,
        Clone,
        Move
    };
    explicit SketcherCopy(const char* name);
    void activate(SketcherCopy::Op op);
    virtual void activate() = 0;
};

// TODO: replace XPM cursor with SVG file
static const char* cursor_createcopy[] = {"32 32 3 1",
                                          "+ c white",
                                          "# c red",
                                          ". c None",
                                          "................................",
                                          ".......+........................",
                                          ".......+........................",
                                          ".......+........................",
                                          ".......+........................",
                                          ".......+........................",
                                          "................................",
                                          ".+++++...+++++..................",
                                          "................................",
                                          ".......+........................",
                                          ".......+..............###.......",
                                          ".......+..............###.......",
                                          ".......+..............###.......",
                                          ".......+..............###.......",
                                          "......................###.......",
                                          ".....###..............###.......",
                                          ".....###..............###.......",
                                          ".....###..............###.......",
                                          ".....###..............###.......",
                                          ".....###..............###.......",
                                          ".....###..............###.......",
                                          ".....###..............###.......",
                                          ".....###..............###.......",
                                          ".....###..............###.......",
                                          ".....###........................",
                                          ".....###........................",
                                          ".....###........................",
                                          ".....###........................",
                                          "................................",
                                          "................................",
                                          "................................",
                                          "................................"};

class DrawSketchHandlerCopy: public DrawSketchHandler
{
public:
    DrawSketchHandlerCopy(string geoidlist, int origingeoid, Sketcher::PointPos originpos,
                          int nelements, SketcherCopy::Op op)
        : Mode(STATUS_SEEK_First)
        , snapMode(SnapMode::Free)
        , geoIdList(geoidlist)
        , Origin()
        , OriginGeoId(origingeoid)
        , OriginPos(originpos)
        , nElements(nelements)
        , Op(op)
        , EditCurve(2)
    {}

    ~DrawSketchHandlerCopy() override
    {}
    /// mode table
    enum SelectMode
    {
        STATUS_SEEK_First,
        STATUS_End
    };

    enum class SnapMode
    {
        Free,
        Snap5Degree
    };

    void mouseMove(Base::Vector2d onSketchPos) override
    {
        if (Mode == STATUS_SEEK_First) {

            if (QApplication::keyboardModifiers() == Qt::ControlModifier)
                snapMode = SnapMode::Snap5Degree;
            else
                snapMode = SnapMode::Free;

            float length = (onSketchPos - EditCurve[0]).Length();
            float angle = (onSketchPos - EditCurve[0]).Angle();

            Base::Vector2d endpoint = onSketchPos;

            if (snapMode == SnapMode::Snap5Degree) {
                angle = round(angle / (M_PI / 36)) * M_PI / 36;
                endpoint = EditCurve[0] + length * Base::Vector2d(cos(angle), sin(angle));
            }

            if (showCursorCoords()) {
                SbString text;
                std::string lengthString = lengthToDisplayFormat(length, 1);
                std::string angleString = angleToDisplayFormat(angle * 180.0 / M_PI, 1);
                text.sprintf(" (%s, %s)", lengthString.c_str(), angleString.c_str());
                setPositionText(endpoint, text);
            }

            EditCurve[1] = endpoint;
            drawEdit(EditCurve);
        }
        applyCursor();
    }

    bool pressButton(Base::Vector2d) override
    {
        if (Mode == STATUS_SEEK_First) {
            drawEdit(EditCurve);
            Mode = STATUS_End;
        }
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        if (Mode == STATUS_End) {
            Base::Vector2d vector = EditCurve[1] - EditCurve[0];
            unsetCursor();
            resetPositionText();

            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Copy/clone/move geometry"));

            try {
                if (Op != SketcherCopy::Move) {
                    Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                          "addCopy(%s, App.Vector(%f, %f, 0), %s)",
                                          geoIdList.c_str(),
                                          vector.x,
                                          vector.y,
                                          (Op == SketcherCopy::Clone ? "True" : "False"));
                }
                else {
                    Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                          "addMove(%s, App.Vector(%f, %f, 0))",
                                          geoIdList.c_str(),
                                          vector.x,
                                          vector.y);
                }
                Gui::Command::commitCommand();
            }
            catch (const Base::Exception& e) {
                Gui::NotifyUserError(
                    sketchgui->getObject(), QT_TRANSLATE_NOOP("Notifications", "Error"), e.what());
                Gui::Command::abortCommand();
            }

            tryAutoRecomputeIfNotSolve(
                static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));
            EditCurve.clear();
            drawEdit(EditCurve);

            // no code after this line, Handler gets deleted in ViewProvider
            sketchgui->purgeHandler();
        }
        return true;
    }

private:
    void activated() override
    {
        setCursor(QPixmap(cursor_createcopy), 7, 7);
        Origin = static_cast<Sketcher::SketchObject*>(sketchgui->getObject())
                     ->getPoint(OriginGeoId, OriginPos);
        EditCurve[0] = Base::Vector2d(Origin.x, Origin.y);
    }

protected:
    SelectMode Mode;
    SnapMode snapMode;
    string geoIdList;
    Base::Vector3d Origin;
    int OriginGeoId;
    Sketcher::PointPos OriginPos;
    int nElements;
    SketcherCopy::Op Op;
    std::vector<Base::Vector2d> EditCurve;
    std::vector<AutoConstraint> sugConstr1;
};

/*---- SketcherCopy definition ----*/
SketcherCopy::SketcherCopy(const char* name)
    : Command(name)
{}

void SketcherCopy::activate(SketcherCopy::Op op)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        Gui::TranslatedUserWarning(getActiveGuiDocument()->getDocument(),
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Select elements from a single sketch."));

        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    if (SubNames.empty()) {
        Gui::TranslatedUserWarning(getActiveGuiDocument()->getDocument(),
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Select elements from a single sketch."));

        return;
    }

    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
    getSelection().clearSelection();

    int LastGeoId = 0;
    Sketcher::PointPos LastPointPos = Sketcher::PointPos::none;
    const Part::Geometry* LastGeo = nullptr;

    // create python command with list of elements
    std::stringstream stream;
    int geoids = 0;
    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
         ++it) {
        // only handle non-external edges
        if (it->size() > 4 && it->substr(0, 4) == "Edge") {
            LastGeoId = std::atoi(it->substr(4, 4000).c_str()) - 1;
            LastPointPos = Sketcher::PointPos::none;
            LastGeo = Obj->getGeometry(LastGeoId);
            // lines to copy
            if (LastGeoId >= 0) {
                geoids++;
                stream << LastGeoId << ",";
            }
        }
        else if (it->size() > 6 && it->substr(0, 6) == "Vertex") {
            // only if it is a GeomPoint
            int VtId = std::atoi(it->substr(6, 4000).c_str()) - 1;
            int GeoId;
            Sketcher::PointPos PosId;
            Obj->getGeoVertexIndex(VtId, GeoId, PosId);
            if (Obj->getGeometry(GeoId)->is<Part::GeomPoint>()) {
                LastGeoId = GeoId;
                LastPointPos = Sketcher::PointPos::start;
                // points to copy
                if (LastGeoId >= 0) {
                    geoids++;
                    stream << LastGeoId << ",";
                }
            }
        }
    }

    // check if last selected element is a Vertex, not being a GeomPoint
    if (SubNames.rbegin()->size() > 6 && SubNames.rbegin()->substr(0, 6) == "Vertex") {
        int VtId = std::atoi(SubNames.rbegin()->substr(6, 4000).c_str()) - 1;
        int GeoId;
        Sketcher::PointPos PosId;
        Obj->getGeoVertexIndex(VtId, GeoId, PosId);
        if (!Obj->getGeometry(GeoId)->is<Part::GeomPoint>()) {
            LastGeoId = GeoId;
            LastPointPos = PosId;
        }
    }

    if (geoids < 1) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("A copy requires at least one selected non-external geometric element"));

        return;
    }

    std::string geoIdList = stream.str();

    // remove the last added comma and brackets to make the python list
    int index = geoIdList.rfind(',');
    geoIdList.resize(index);
    geoIdList.insert(0, 1, '[');
    geoIdList.append(1, ']');

    // if the last element is not a point serving as a reference for the copy process
    // then make the start point of the last element the copy reference (if it exists, if not the
    // center point)
    if (LastPointPos == Sketcher::PointPos::none) {
        if (LastGeo->is<Part::GeomCircle>()
            || LastGeo->is<Part::GeomEllipse>()) {
            LastPointPos = Sketcher::PointPos::mid;
        }
        else {
            LastPointPos = Sketcher::PointPos::start;
        }
    }

    // Ask the user if they want to clone or to simple copy
    /*
    int ret = QMessageBox::question(Gui::getMainWindow(), QObject::tr("Dimensional/Geometric
    constraints"), QObject::tr("Do you want to clone the object, i.e. substitute dimensional
    constraints by geometric constraints?"), QMessageBox::Yes, QMessageBox::No,
    QMessageBox::Cancel);
    // use an equality constraint
    if (ret == QMessageBox::Yes) {
        clone = true;
    }
    else if (ret == QMessageBox::Cancel) {
    // do nothing
    return;
    }
*/

    ActivateHandler(getActiveGuiDocument(),
                    std::make_unique<DrawSketchHandlerCopy>(geoIdList, LastGeoId, LastPointPos, geoids, op));
}


class CmdSketcherCopy: public SketcherCopy
{
public:
    CmdSketcherCopy();
    ~CmdSketcherCopy() override
    {}
    const char* className() const override
    {
        return "CmdSketcherCopy";
    }
    void activate() override;

protected:
    void activated(int iMsg) override;
    bool isActive() override;
};

CmdSketcherCopy::CmdSketcherCopy()
    : SketcherCopy("Sketcher_Copy")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Copy");
    sToolTipText = QT_TR_NOOP(
        "Creates a simple copy of the geometry taking as reference the last selected point");
    sWhatsThis = "Sketcher_Copy";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Copy";
    sAccel = "Z, C";
    eType = ForEdit;
}

void CmdSketcherCopy::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    SketcherCopy::activate(SketcherCopy::Copy);
}


void CmdSketcherCopy::activate()
{
    SketcherCopy::activate(SketcherCopy::Copy);
}

bool CmdSketcherCopy::isActive()
{
    return isCommandActive(getActiveGuiDocument(), true);
}

// ================================================================================

class CmdSketcherClone: public SketcherCopy
{
public:
    CmdSketcherClone();
    ~CmdSketcherClone() override
    {}
    const char* className() const override
    {
        return "CmdSketcherClone";
    }
    void activate() override;

protected:
    void activated(int iMsg) override;
    bool isActive() override;
};

CmdSketcherClone::CmdSketcherClone()
    : SketcherCopy("Sketcher_Clone")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Clone");
    sToolTipText =
        QT_TR_NOOP("Creates a clone of the geometry taking as reference the last selected point");
    sWhatsThis = "Sketcher_Clone";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Clone";
    sAccel = "Z, L";
    eType = ForEdit;
}

void CmdSketcherClone::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    SketcherCopy::activate(SketcherCopy::Clone);
}

void CmdSketcherClone::activate()
{
    SketcherCopy::activate(SketcherCopy::Clone);
}

bool CmdSketcherClone::isActive()
{
    return isCommandActive(getActiveGuiDocument(), true);
}

class CmdSketcherMove: public SketcherCopy
{
public:
    CmdSketcherMove();
    ~CmdSketcherMove() override
    {}
    const char* className() const override
    {
        return "CmdSketcherMove";
    }
    void activate() override;

protected:
    void activated(int iMsg) override;
    bool isActive() override;
};

CmdSketcherMove::CmdSketcherMove()
    : SketcherCopy("Sketcher_Move")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Move");
    sToolTipText = QT_TR_NOOP("Moves the geometry taking as reference the last selected point");
    sWhatsThis = "Sketcher_Move";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Move";
    sAccel = "Z, M";
    eType = ForEdit;
}

void CmdSketcherMove::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    SketcherCopy::activate(SketcherCopy::Move);
}

void CmdSketcherMove::activate()
{
    SketcherCopy::activate(SketcherCopy::Move);
}

bool CmdSketcherMove::isActive()
{
    return isCommandActive(getActiveGuiDocument(), true);
}

// ================================================================================

DEF_STD_CMD_ACL(CmdSketcherCompCopy)

CmdSketcherCompCopy::CmdSketcherCompCopy()
    : Command("Sketcher_CompCopy")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Clone");
    sToolTipText =
        QT_TR_NOOP("Creates a clone of the geometry taking as reference the last selected point");
    sWhatsThis = "Sketcher_CompCopy";
    sStatusTip = sToolTipText;
    sAccel = "";
    eType = ForEdit;
}

void CmdSketcherCompCopy::activated(int iMsg)
{
    if (iMsg < 0 || iMsg > 2)
        return;

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());

    if (iMsg == 0) {
        CmdSketcherClone sc;
        sc.activate();
        pcAction->setShortcut(QString::fromLatin1(this->getAccel()));
    }
    else if (iMsg == 1) {
        CmdSketcherCopy sc;
        sc.activate();
        pcAction->setShortcut(QString::fromLatin1(this->getAccel()));
    }
    else if (iMsg == 2) {
        CmdSketcherMove sc;
        sc.activate();
        pcAction->setShortcut(QString::fromLatin1(""));
    }
}

Gui::Action* CmdSketcherCompCopy::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* clone = pcAction->addAction(QString());
    clone->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Clone"));
    QAction* copy = pcAction->addAction(QString());
    copy->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Copy"));
    QAction* move = pcAction->addAction(QString());
    move->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Move"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(clone->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    pcAction->setShortcut(QString::fromLatin1(getAccel()));

    return pcAction;
}

void CmdSketcherCompCopy::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* clone = a[0];
    clone->setText(QApplication::translate("Sketcher_CompCopy", "Clone"));
    clone->setToolTip(QApplication::translate(
        "Sketcher_Clone",
        "Creates a clone of the geometry taking as reference the last selected point"));
    clone->setStatusTip(QApplication::translate(
        "Sketcher_Clone",
        "Creates a clone of the geometry taking as reference the last selected point"));
    QAction* copy = a[1];
    copy->setText(QApplication::translate("Sketcher_CompCopy", "Copy"));
    copy->setToolTip(QApplication::translate(
        "Sketcher_Copy",
        "Creates a simple copy of the geometry taking as reference the last selected point"));
    copy->setStatusTip(QApplication::translate(
        "Sketcher_Copy",
        "Creates a simple copy of the geometry taking as reference the last selected point"));
    QAction* move = a[2];
    move->setText(QApplication::translate("Sketcher_CompCopy", "Move"));
    move->setToolTip(QApplication::translate(
        "Sketcher_Move", "Moves the geometry taking as reference the last selected point"));
    move->setStatusTip(QApplication::translate(
        "Sketcher_Move", "Moves the geometry taking as reference the last selected point"));
}

bool CmdSketcherCompCopy::isActive()
{
    return isCommandActive(getActiveGuiDocument(), true);
}

// ================================================================================

// TODO: replace XPM cursor with SVG file
/* XPM */
static const char* cursor_createrectangulararray[] = {"32 32 3 1",
                                                      "+ c white",
                                                      "# c red",
                                                      ". c None",
                                                      "................................",
                                                      ".......+........................",
                                                      ".......+........................",
                                                      ".......+........................",
                                                      ".......+........................",
                                                      ".......+........................",
                                                      "................................",
                                                      ".+++++...+++++..................",
                                                      ".......................###......",
                                                      ".......+...............###......",
                                                      ".......+...............###......",
                                                      ".......+...............###......",
                                                      ".......+......###......###......",
                                                      ".......+......###......###......",
                                                      "..............###......###......",
                                                      "..............###......###......",
                                                      ".....###......###......###......",
                                                      ".....###......###......###......",
                                                      ".....###......###......###......",
                                                      ".....###......###......###......",
                                                      ".....###......###......###......",
                                                      ".....###......###......###......",
                                                      ".....###......###...............",
                                                      ".....###......###...............",
                                                      ".....###......###...............",
                                                      ".....###......###...............",
                                                      ".....###........................",
                                                      ".....###........................",
                                                      ".....###........................",
                                                      ".....###........................",
                                                      "................................",
                                                      "................................"};

class DrawSketchHandlerRectangularArray: public DrawSketchHandler
{
public:
    DrawSketchHandlerRectangularArray(string geoidlist, int origingeoid,
                                      Sketcher::PointPos originpos, int nelements, bool clone,
                                      int rows, int cols, bool constraintSeparation,
                                      bool equalVerticalHorizontalSpacing)
        : Mode(STATUS_SEEK_First)
        , snapMode(SnapMode::Free)
        , geoIdList(geoidlist)
        , OriginGeoId(origingeoid)
        , OriginPos(originpos)
        , nElements(nelements)
        , Clone(clone)
        , Rows(rows)
        , Cols(cols)
        , ConstraintSeparation(constraintSeparation)
        , EqualVerticalHorizontalSpacing(equalVerticalHorizontalSpacing)
        , EditCurve(2)
    {}

    ~DrawSketchHandlerRectangularArray() override
    {}
    /// mode table
    enum SelectMode
    {
        STATUS_SEEK_First,
        STATUS_End
    };

    enum class SnapMode
    {
        Free,
        Snap5Degree
    };

    void mouseMove(Base::Vector2d onSketchPos) override
    {
        if (Mode == STATUS_SEEK_First) {

            if (QApplication::keyboardModifiers() == Qt::ControlModifier)
                snapMode = SnapMode::Snap5Degree;
            else
                snapMode = SnapMode::Free;

            float length = (onSketchPos - EditCurve[0]).Length();
            float angle = (onSketchPos - EditCurve[0]).Angle();

            Base::Vector2d endpoint = onSketchPos;

            if (snapMode == SnapMode::Snap5Degree) {
                angle = round(angle / (M_PI / 36)) * M_PI / 36;
                endpoint = EditCurve[0] + length * Base::Vector2d(cos(angle), sin(angle));
            }

            if (showCursorCoords()) {
                SbString text;
                std::string lengthString = lengthToDisplayFormat(length, 1);
                std::string angleString = angleToDisplayFormat(angle * 180.0 / M_PI, 1);
                text.sprintf(" (%s, %s)", lengthString.c_str(), angleString.c_str());
                setPositionText(endpoint, text);
            }

            EditCurve[1] = endpoint;
            drawEdit(EditCurve);
            if (seekAutoConstraint(
                    sugConstr1, endpoint, Base::Vector2d(0.0, 0.0), AutoConstraint::VERTEX)) {
                renderSuggestConstraintsCursor(sugConstr1);
                return;
            }
        }
        applyCursor();
    }

    bool pressButton(Base::Vector2d) override
    {
        if (Mode == STATUS_SEEK_First) {
            drawEdit(EditCurve);
            Mode = STATUS_End;
        }
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        if (Mode == STATUS_End) {
            Base::Vector2d vector = EditCurve[1] - EditCurve[0];
            unsetCursor();
            resetPositionText();

            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create copy of geometry"));

            try {
                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addRectangularArray(%s, App.Vector(%f, %f, 0), %s, %d, %d, %s, %f)",
                    geoIdList.c_str(),
                    vector.x,
                    vector.y,
                    (Clone ? "True" : "False"),
                    Cols,
                    Rows,
                    (ConstraintSeparation ? "True" : "False"),
                    (EqualVerticalHorizontalSpacing ? 1.0 : 0.5));
                Gui::Command::commitCommand();
            }
            catch (const Base::Exception& e) {
                Gui::NotifyUserError(
                    sketchgui, QT_TRANSLATE_NOOP("Notifications", "Error"), e.what());
                Gui::Command::abortCommand();
            }

            // add auto constraints for the destination copy
            if (!sugConstr1.empty()) {
                createAutoConstraints(sugConstr1, OriginGeoId + nElements, OriginPos);
                sugConstr1.clear();
            }
            tryAutoRecomputeIfNotSolve(
                static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));

            EditCurve.clear();
            drawEdit(EditCurve);

            // no code after this line, Handler is deleted in ViewProvider
            sketchgui->purgeHandler();
        }
        return true;
    }

private:
    void activated() override
    {
        setCursor(QPixmap(cursor_createrectangulararray), 7, 7);
        Origin = static_cast<Sketcher::SketchObject*>(sketchgui->getObject())
                     ->getPoint(OriginGeoId, OriginPos);
        EditCurve[0] = Base::Vector2d(Origin.x, Origin.y);
    }

protected:
    SelectMode Mode;
    SnapMode snapMode;
    string geoIdList;
    Base::Vector3d Origin;
    int OriginGeoId;
    Sketcher::PointPos OriginPos;
    int nElements;
    bool Clone;
    int Rows;
    int Cols;
    bool ConstraintSeparation;
    bool EqualVerticalHorizontalSpacing;
    std::vector<Base::Vector2d> EditCurve;
    std::vector<AutoConstraint> sugConstr1;
};

DEF_STD_CMD_A(CmdSketcherRectangularArray)

CmdSketcherRectangularArray::CmdSketcherRectangularArray()
    : Command("Sketcher_RectangularArray")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Rectangular array");
    sToolTipText = QT_TR_NOOP("Creates a rectangular array pattern of the geometry taking as "
                              "reference the last selected point");
    sWhatsThis = "Sketcher_RectangularArray";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_RectangularArray";
    sAccel = "Z, A";
    eType = ForEdit;
}

void CmdSketcherRectangularArray::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        Gui::TranslatedUserWarning(getActiveGuiDocument()->getDocument(),
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Select elements from a single sketch."));

        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    if (SubNames.empty()) {
        Gui::TranslatedUserWarning(getActiveGuiDocument()->getDocument(),
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Select elements from a single sketch."));
        return;
    }

    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    getSelection().clearSelection();

    int LastGeoId = 0;
    Sketcher::PointPos LastPointPos = Sketcher::PointPos::none;
    const Part::Geometry* LastGeo = nullptr;

    // create python command with list of elements
    std::stringstream stream;
    int geoids = 0;

    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
         ++it) {
        // only handle non-external edges
        if (it->size() > 4 && it->substr(0, 4) == "Edge") {
            LastGeoId = std::atoi(it->substr(4, 4000).c_str()) - 1;
            LastPointPos = Sketcher::PointPos::none;
            LastGeo = Obj->getGeometry(LastGeoId);

            // lines to copy
            if (LastGeoId >= 0) {
                geoids++;
                stream << LastGeoId << ",";
            }
        }
        else if (it->size() > 6 && it->substr(0, 6) == "Vertex") {
            // only if it is a GeomPoint
            int VtId = std::atoi(it->substr(6, 4000).c_str()) - 1;
            int GeoId;
            Sketcher::PointPos PosId;
            Obj->getGeoVertexIndex(VtId, GeoId, PosId);
            if (Obj->getGeometry(GeoId)->is<Part::GeomPoint>()) {
                LastGeoId = GeoId;
                LastPointPos = Sketcher::PointPos::start;
                // points to copy
                if (LastGeoId >= 0) {
                    geoids++;
                    stream << LastGeoId << ",";
                }
            }
        }
    }

    // check if last selected element is a Vertex, not being a GeomPoint
    if (SubNames.rbegin()->size() > 6 && SubNames.rbegin()->substr(0, 6) == "Vertex") {
        int VtId = std::atoi(SubNames.rbegin()->substr(6, 4000).c_str()) - 1;
        int GeoId;
        Sketcher::PointPos PosId;
        Obj->getGeoVertexIndex(VtId, GeoId, PosId);
        if (!Obj->getGeometry(GeoId)->is<Part::GeomPoint>()) {
            LastGeoId = GeoId;
            LastPointPos = PosId;
        }
    }

    if (geoids < 1) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("A copy requires at least one selected non-external geometric element"));

        return;
    }

    std::string geoIdList = stream.str();

    // remove the last added comma and brackets to make the python list
    int index = geoIdList.rfind(',');
    geoIdList.resize(index);
    geoIdList.insert(0, 1, '[');
    geoIdList.append(1, ']');

    // if the last element is not a point serving as a reference for the copy process
    // then make the start point of the last element the copy reference (if it exists, if not the
    // center point)
    if (LastPointPos == Sketcher::PointPos::none) {
        if (LastGeo->is<Part::GeomCircle>()
            || LastGeo->is<Part::GeomEllipse>()) {
            LastPointPos = Sketcher::PointPos::mid;
        }
        else {
            LastPointPos = Sketcher::PointPos::start;
        }
    }

    // Pop-up asking for values
    SketchRectangularArrayDialog slad;

    if (slad.exec() == QDialog::Accepted) {
        ActivateHandler(getActiveGuiDocument(),
                        std::make_unique<DrawSketchHandlerRectangularArray>(geoIdList,
                                                              LastGeoId,
                                                              LastPointPos,
                                                              geoids,
                                                              slad.Clone,
                                                              slad.Rows,
                                                              slad.Cols,
                                                              slad.ConstraintSeparation,
                                                              slad.EqualVerticalHorizontalSpacing));
    }
}

bool CmdSketcherRectangularArray::isActive()
{
    return isCommandActive(getActiveGuiDocument(), true);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherDeleteAllGeometry)

CmdSketcherDeleteAllGeometry::CmdSketcherDeleteAllGeometry()
    : Command("Sketcher_DeleteAllGeometry")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Delete all geometry");
    sToolTipText = QT_TR_NOOP("Delete all geometry and constraints in the current sketch, "
                              "with the exception of external geometry");
    sWhatsThis = "Sketcher_DeleteAllGeometry";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_DeleteGeometry";
    sAccel = "";
    eType = ForEdit;
}

void CmdSketcherDeleteAllGeometry::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    int ret = QMessageBox::question(
        Gui::getMainWindow(),
        QObject::tr("Delete All Geometry"),
        QObject::tr("Are you really sure you want to delete all geometry and constraints?"),
        QMessageBox::Yes,
        QMessageBox::Cancel);
    // use an equality constraint
    if (ret == QMessageBox::Yes) {
        getSelection().clearSelection();
        Sketcher::SketchObject* Obj = getSketchObject();

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Delete all geometry"));
            Gui::cmdAppObjectArgs(Obj, "deleteAllGeometry()");
            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Gui::NotifyUserError(
                Obj, QT_TRANSLATE_NOOP("Notifications", "Failed to delete all geometry"), e.what());
            Gui::Command::abortCommand();
        }

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool autoRecompute = hGrp->GetBool("AutoRecompute", false);

        if (autoRecompute)
            Gui::Command::updateActive();
        else
            Obj->solve();
    }
    else if (ret == QMessageBox::Cancel) {
        // do nothing
        return;
    }
}

bool CmdSketcherDeleteAllGeometry::isActive()
{
    return isCommandActive(getActiveGuiDocument(), false);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherDeleteAllConstraints)

CmdSketcherDeleteAllConstraints::CmdSketcherDeleteAllConstraints()
    : Command("Sketcher_DeleteAllConstraints")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Delete all constraints");
    sToolTipText = QT_TR_NOOP("Delete all constraints in the sketch");
    sWhatsThis = "Sketcher_DeleteAllConstraints";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_DeleteConstraints";
    sAccel = "";
    eType = ForEdit;
}

void CmdSketcherDeleteAllConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    int ret = QMessageBox::question(
        Gui::getMainWindow(),
        QObject::tr("Delete All Constraints"),
        QObject::tr("Are you really sure you want to delete all the constraints?"),
        QMessageBox::Yes,
        QMessageBox::Cancel);

    if (ret == QMessageBox::Yes) {
        getSelection().clearSelection();
        Sketcher::SketchObject* Obj = getSketchObject();

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Delete All Constraints"));
            Gui::cmdAppObjectArgs(Obj, "deleteAllConstraints()");
            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Gui::NotifyUserError(
                Obj,
                QT_TRANSLATE_NOOP("Notifications", "Failed to delete all constraints"),
                e.what());
            Gui::Command::abortCommand();
        }

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool autoRecompute = hGrp->GetBool("AutoRecompute", false);

        if (autoRecompute)
            Gui::Command::updateActive();
        else
            Obj->solve();
    }
    else if (ret == QMessageBox::Cancel) {
        // do nothing
        return;
    }
}

bool CmdSketcherDeleteAllConstraints::isActive()
{
    return isCommandActive(getActiveGuiDocument(), false);
}

// ================================================================================


DEF_STD_CMD_A(CmdSketcherRemoveAxesAlignment)

CmdSketcherRemoveAxesAlignment::CmdSketcherRemoveAxesAlignment()
    : Command("Sketcher_RemoveAxesAlignment")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Remove axes alignment");
    sToolTipText = QT_TR_NOOP("Modifies constraints to remove axes alignment while trying to "
                              "preserve the constraint relationship of the selection");
    sWhatsThis = "Sketcher_RemoveAxesAlignment";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_RemoveAxesAlignment";
    sAccel = "Z, R";
    eType = ForEdit;
}

void CmdSketcherRemoveAxesAlignment::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        Gui::TranslatedUserWarning(getActiveGuiDocument()->getDocument(),
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Select elements from a single sketch."));

        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    if (SubNames.empty()) {
        Gui::TranslatedUserWarning(getActiveGuiDocument()->getDocument(),
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("Select elements from a single sketch."));
        return;
    }

    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    getSelection().clearSelection();

    int LastGeoId = 0;

    // create python command with list of elements
    std::stringstream stream;
    int geoids = 0;

    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
         ++it) {
        // only handle non-external edges
        if (it->size() > 4 && it->substr(0, 4) == "Edge") {
            LastGeoId = std::atoi(it->substr(4, 4000).c_str()) - 1;

            // lines to copy
            if (LastGeoId >= 0) {
                geoids++;
                stream << LastGeoId << ",";
            }
        }
        else if (it->size() > 6 && it->substr(0, 6) == "Vertex") {
            // only if it is a GeomPoint
            int VtId = std::atoi(it->substr(6, 4000).c_str()) - 1;
            int GeoId;
            Sketcher::PointPos PosId;
            Obj->getGeoVertexIndex(VtId, GeoId, PosId);
            if (Obj->getGeometry(GeoId)->is<Part::GeomPoint>()) {
                LastGeoId = GeoId;
                // points to copy
                if (LastGeoId >= 0) {
                    geoids++;
                    stream << LastGeoId << ",";
                }
            }
        }
    }

    if (geoids < 1) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Removal of axes alignment requires at least one selected "
                        "non-external geometric element"));

        return;
    }

    std::string geoIdList = stream.str();

    // remove the last added comma and brackets to make the python list
    int index = geoIdList.rfind(',');
    geoIdList.resize(index);
    geoIdList.insert(0, 1, '[');
    geoIdList.append(1, ']');

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Remove Axes Alignment"));

    try {
        Gui::cmdAppObjectArgs(Obj, "removeAxesAlignment(%s)", geoIdList.c_str());
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        Gui::NotifyUserError(Obj, QT_TRANSLATE_NOOP("Notifications", "Error"), e.what());
        Gui::Command::abortCommand();
    }

    tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject*>(Obj));
}

bool CmdSketcherRemoveAxesAlignment::isActive()
{
    return isCommandActive(getActiveGuiDocument(), true);
}


// Offset tool =====================================================================
DEF_STD_CMD_A(CmdSketcherOffset)

CmdSketcherOffset::CmdSketcherOffset()
    : Command("Sketcher_Offset")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Offset geometry");
    sToolTipText = QT_TR_NOOP("Offset selected geometries. A positive offset length makes the offset go outward, a negative length inward.");
    sWhatsThis = "Sketcher_Offset";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Offset";
    sAccel = "Z, T";
    eType = ForEdit;
}

void CmdSketcherOffset::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<int> listOfGeoIds = {};

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        Gui::TranslatedUserWarning(
            getActiveGuiDocument(),
            QObject::tr("Wrong selection"),
            QObject::tr("Select elements from a single sketch."));
        return;
    }

    // get the needed lists and objects
    auto* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
    const std::vector<std::string>& subNames = selection[0].getSubNames();
    if (!subNames.empty()) {
        for (auto& name : subNames) {
            int geoId;
            if (name.size() > 4 && name.substr(0, 4) == "Edge") {
                geoId = std::atoi(name.substr(4, 4000).c_str()) - 1;
            }
            else if (name.size() > 12 && name.substr(0, 12) == "ExternalEdge") {
                geoId = -std::atoi(name.substr(12, 4000).c_str()) - 2;
            }
            else {
                continue;
            }

            const Part::Geometry* geo = Obj->getGeometry(geoId);
            if (!isPoint(*geo)
                && !isBSplineCurve(*geo)
                && !isEllipse(*geo)
                && !isArcOfEllipse(*geo)
                && !isArcOfHyperbola(*geo)
                && !isArcOfParabola(*geo)
                && !GeometryFacade::isInternalAligned(geo)) {
                // Currently ellipse/parabola/hyperbola/bspline are not handled correctly.
                // Occ engine gives offset of those as set of lines and arcs and does not seem to work consistently.
                listOfGeoIds.push_back(geoId);
            }
        }
    }

    if (listOfGeoIds.size() != 0) {
        ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerOffset>(listOfGeoIds));
    }
    else {
        getSelection().clearSelection();
        Gui::NotifyUserError(Obj,
            QT_TRANSLATE_NOOP("Notifications", "Invalid selection"),
            QT_TRANSLATE_NOOP("Notifications", "Selection has no valid geometries. B-splines and points are not supported yet."));
    }
}

bool CmdSketcherOffset::isActive()
{
    return isCommandActive(getActiveGuiDocument(), true);
}

// Rotate tool =====================================================================

DEF_STD_CMD_A(CmdSketcherRotate)

CmdSketcherRotate::CmdSketcherRotate()
    : Command("Sketcher_Rotate")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Rotate / Polar transform");
    sToolTipText = QT_TR_NOOP("Rotate selected geometries, making n copies, enable creation of circular patterns.");
    sWhatsThis = "Sketcher_Rotate";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Rotate";
    sAccel = "Z, P";
    eType = ForEdit;
}

void CmdSketcherRotate::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<int> listOfGeoIds = getListOfSelectedGeoIds(true);

    if (!listOfGeoIds.empty()) {
        ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerRotate>(listOfGeoIds));
    }
    getSelection().clearSelection();
}

bool CmdSketcherRotate::isActive()
{
    return isCommandActive(getActiveGuiDocument(), true);
}

// Scale tool =====================================================================

DEF_STD_CMD_A(CmdSketcherScale)

CmdSketcherScale::CmdSketcherScale()
    : Command("Sketcher_Scale")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Scale transform");
    sToolTipText = QT_TR_NOOP("Scale selected geometries. After selecting the center point you can either enter the scale factor, or select two reference points then scale factor = length(p2-center) / length(p1-center).");
    sWhatsThis = "Sketcher_Scale";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Scale";
    sAccel = "Z, P, S";
    eType = ForEdit;
}

void CmdSketcherScale::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<int> listOfGeoIds = getListOfSelectedGeoIds(true);

    if (!listOfGeoIds.empty()) {
        ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerScale>(listOfGeoIds));
    }
    getSelection().clearSelection();
}

bool CmdSketcherScale::isActive()
{
    return isCommandActive(getActiveGuiDocument(), true);
}

// Translate / rectangular pattern tool =======================================================

DEF_STD_CMD_A(CmdSketcherTranslate)

CmdSketcherTranslate::CmdSketcherTranslate()
    : Command("Sketcher_Translate")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Move / Array transform");
    sToolTipText = QT_TR_NOOP("Translate selected geometries. Enable creation of i * j copies.");
    sWhatsThis = "Sketcher_Translate";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Translate";
    sAccel = "W";
    eType = ForEdit;
}

void CmdSketcherTranslate::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<int> listOfGeoIds = getListOfSelectedGeoIds(true);

    if (!listOfGeoIds.empty()) {
        ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerTranslate>(listOfGeoIds));
    }
    getSelection().clearSelection();
}

bool CmdSketcherTranslate::isActive()
{
    return isCommandActive(getActiveGuiDocument(), true);
}

void CreateSketcherCommandsConstraintAccel()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdSketcherSelectConstraints());
    rcCmdMgr.addCommand(new CmdSketcherSelectOrigin());
    rcCmdMgr.addCommand(new CmdSketcherSelectVerticalAxis());
    rcCmdMgr.addCommand(new CmdSketcherSelectHorizontalAxis());
    rcCmdMgr.addCommand(new CmdSketcherSelectRedundantConstraints());
    rcCmdMgr.addCommand(new CmdSketcherSelectConflictingConstraints());
    rcCmdMgr.addCommand(new CmdSketcherSelectMalformedConstraints());
    rcCmdMgr.addCommand(new CmdSketcherSelectPartiallyRedundantConstraints());
    rcCmdMgr.addCommand(new CmdSketcherSelectElementsAssociatedWithConstraints());
    rcCmdMgr.addCommand(new CmdSketcherSelectElementsWithDoFs());
    rcCmdMgr.addCommand(new CmdSketcherRestoreInternalAlignmentGeometry());
    rcCmdMgr.addCommand(new CmdSketcherTranslate());
    rcCmdMgr.addCommand(new CmdSketcherOffset());
    rcCmdMgr.addCommand(new CmdSketcherRotate());
    rcCmdMgr.addCommand(new CmdSketcherScale());
    rcCmdMgr.addCommand(new CmdSketcherSymmetry());
    rcCmdMgr.addCommand(new CmdSketcherCopy());
    rcCmdMgr.addCommand(new CmdSketcherClone());
    rcCmdMgr.addCommand(new CmdSketcherMove());
    rcCmdMgr.addCommand(new CmdSketcherCompCopy());
    rcCmdMgr.addCommand(new CmdSketcherRectangularArray());
    rcCmdMgr.addCommand(new CmdSketcherDeleteAllGeometry());
    rcCmdMgr.addCommand(new CmdSketcherDeleteAllConstraints());
    rcCmdMgr.addCommand(new CmdSketcherRemoveAxesAlignment());
    rcCmdMgr.addCommand(new CmdSketcherCopyClipboard());
    rcCmdMgr.addCommand(new CmdSketcherCut());
    rcCmdMgr.addCommand(new CmdSketcherPaste());
}
// clang-format on
