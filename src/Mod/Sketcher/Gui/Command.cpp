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

#include <QApplication>
#include <QCheckBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QWidgetAction>


#include <App/DocumentObjectGroup.h>
#include <App/Datums.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Notifications.h>
#include <Gui/PrefWidgets.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/Selection/SelectionFilter.h>
#include <Gui/Selection/SelectionObject.h>
#include <Mod/Part/App/Attacher.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Part/Gui/AttacherTexts.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "SketchMirrorDialog.h"
#include "SketchOrientationDialog.h"
#include "TaskSketcherValidation.h"
#include "Utils.h"
#include "ViewProviderSketch.h"
#include "Command.h"

// Hint: this is to prevent to re-format big parts of the file. Remove it later again.
// clang-format off
using namespace std;
using namespace SketcherGui;
using namespace Part;
using namespace Attacher;


namespace SketcherGui
{

class ExceptionWrongInput: public Base::Exception
{
public:
    ExceptionWrongInput()
        : ErrMsg(QString())
    {}

    // Pass untranslated strings, enclosed in QT_TR_NOOP()
    explicit ExceptionWrongInput(const char* ErrMsg)
    {
        this->ErrMsg = QObject::tr(ErrMsg);
        this->setMessage(ErrMsg);
    }

    ~ExceptionWrongInput() noexcept override
    {}

    QString ErrMsg;
};


Attacher::eMapMode SuggestAutoMapMode(Attacher::SuggestResult::eSuggestResult* pMsgId = nullptr,
                                      QString* message = nullptr,
                                      std::vector<Attacher::eMapMode>* allmodes = nullptr)
{
    // convert pointers into valid references, to avoid checking for null pointers everywhere
    Attacher::SuggestResult::eSuggestResult buf;
    if (!pMsgId)
        pMsgId = &buf;
    Attacher::SuggestResult::eSuggestResult& msg = *pMsgId;
    QString buf2;
    if (!message)
        message = &buf2;
    QString& msg_str = *message;

    App::PropertyLinkSubList tmpSupport;
    Gui::Selection().getAsPropertyLinkSubList(tmpSupport);

    Attacher::SuggestResult sugr;
    AttachEngine3D eng;
    eng.setUp(tmpSupport);
    eng.suggestMapModes(sugr);
    if (allmodes)
        *allmodes = sugr.allApplicableModes;
    msg = sugr.message;
    switch (msg) {
        case Attacher::SuggestResult::srOK:
            break;
        case Attacher::SuggestResult::srNoModesFit:
            msg_str = QObject::tr("There are no modes that accept the selected set of subelements");
            break;
        case Attacher::SuggestResult::srLinkBroken:
            msg_str = QObject::tr("Broken link to support subelements");
            break;
        case Attacher::SuggestResult::srUnexpectedError:
            msg_str = QObject::tr("Unexpected error");
            break;
        case Attacher::SuggestResult::srIncompatibleGeometry:
            if (tmpSupport.getSubValues()[0].substr(0, 4) == std::string("Face"))
                msg_str = QObject::tr("Face is non-planar");
            else
                msg_str = QObject::tr("Selected shapes are of wrong form (e.g., a curved edge "
                                      "where a straight one is needed)");
            break;
        default:
            msg_str = QObject::tr("Unexpected error");
            assert(0 /*no message for eSuggestResult enum item*/);
    }

    return sugr.bestFitMode;
}
}// namespace SketcherGui


/* Sketch commands =======================================================*/
DEF_STD_CMD_A(CmdSketcherNewSketch)

CmdSketcherNewSketch::CmdSketcherNewSketch()
    : Command("Sketcher_NewSketch")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("New Sketch");
    sToolTipText = QT_TR_NOOP("Creates a new sketch");
    sWhatsThis = "Sketcher_NewSketch";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_NewSketch";
}

void CmdSketcherNewSketch::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Attacher::eMapMode mapmode = Attacher::mmDeactivated;
    std::string groupName;
    bool bAttach = false;
    bool groupSelected = false;
    if (Gui::Selection().countObjectsOfType<App::DocumentObjectGroup>() > 0) {
        auto selection = Gui::Selection().getSelection();
        if (selection.size() > 1) {
            Gui::TranslatedUserWarning(
                getActiveGuiDocument(),
                QObject::tr("Invalid selection"),
                QObject::tr("Too many objects selected"));
                return;
        }

        groupName = selection[0].FeatName;
        groupSelected = true;
    }
    else if (Gui::Selection().hasSelection()) {
        Attacher::SuggestResult::eSuggestResult msgid = Attacher::SuggestResult::srOK;
        QString msg_str;
        std::vector<Attacher::eMapMode> validModes;
        mapmode = SuggestAutoMapMode(&msgid, &msg_str, &validModes);
        if (msgid == Attacher::SuggestResult::srOK)
            bAttach = true;
        if (msgid != Attacher::SuggestResult::srOK
            && msgid != Attacher::SuggestResult::srNoModesFit) {
            Gui::TranslatedUserWarning(
                getActiveGuiDocument(),
                QObject::tr("Sketch mapping"),
                QObject::tr("Cannot map the sketch to the selected object. %1.").arg(msg_str));
            return;
        }
        if (validModes.size() > 1) {
            validModes.insert(validModes.begin(), Attacher::mmDeactivated);
            bool ok;
            QStringList items;
            items.push_back(QObject::tr("Do not attach"));
            int iSugg = 0;// index of the auto-suggested mode in the list of valid modes
            for (size_t i = 0; i < validModes.size(); ++i) {
                auto uiStrings =
                    AttacherGui::getUIStrings(AttachEnginePlane::getClassTypeId(), validModes[i]);
                items.push_back(uiStrings[0]);
                if (validModes[i] == mapmode)
                    iSugg = items.size() - 1;
            }
            QString text = QInputDialog::getItem(
                Gui::getMainWindow(),
                qApp->translate("Sketcher_NewSketch", "Sketch Attachment"),
                qApp->translate("Sketcher_NewSketch",
                                "Select the method to attach this sketch to selected object"),
                items,
                iSugg,
                false,
                &ok,
                Qt::MSWindowsFixedSizeDialogHint);
            if (!ok)
                return;
            int index = items.indexOf(text);
            if (index == 0) {
                bAttach = false;
                mapmode = Attacher::mmDeactivated;
            }
            else {
                bAttach = true;
                mapmode = validModes[index - 1];
            }
        }
    }

    if (bAttach) {

        std::vector<Gui::SelectionObject> objects = Gui::Selection().getSelectionEx();
        // assert (objects.size() == 1); //should have been filtered out by SuggestAutoMapMode
        // Gui::SelectionObject &sel_support = objects[0];
        App::PropertyLinkSubList support;
        Gui::Selection().getAsPropertyLinkSubList(support);
        std::string supportString = support.getPyReprString();

        // create Sketch on Face
        std::string FeatName = getUniqueObjectName("Sketch");

        openCommand(QT_TRANSLATE_NOOP("Command", "Create a new sketch on a face"));
        doCommand(Doc,
                  "App.activeDocument().addObject('Sketcher::SketchObject', '%s')",
                  FeatName.c_str());
        if (mapmode < Attacher::mmDummy_NumberOfModes)
            doCommand(Gui,
                      "App.activeDocument().%s.MapMode = \"%s\"",
                      FeatName.c_str(),
                      AttachEngine::getModeName(mapmode).c_str());
        else
            assert(0 /* mapmode index out of range */);
        doCommand(
            Gui, "App.activeDocument().%s.AttachmentSupport = %s", FeatName.c_str(), supportString.c_str());
        doCommand(Gui, "App.activeDocument().recompute()");// recompute the sketch placement based
                                                           // on its support
        doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());

        Part::Feature* part = static_cast<Part::Feature*>(
            support.getValue());// if multi-part support, this will return 0
        if (part) {
            App::DocumentObjectGroup* grp = part->getGroup();
            if (grp) {
                doCommand(Doc,
                          "App.activeDocument().%s.addObject(App.activeDocument().%s)",
                          grp->getNameInDocument(),
                          FeatName.c_str());
            }
        }
    }
    else {
        // ask user for orientation
        SketchOrientationDialog Dlg;

        Dlg.adjustSize();
        if (Dlg.exec() != QDialog::Accepted)
            return;// canceled
        Base::Vector3d p = Dlg.Pos.getPosition();
        Base::Rotation r = Dlg.Pos.getRotation();

        std::string FeatName = getUniqueObjectName("Sketch");

        openCommand(QT_TRANSLATE_NOOP("Command", "Create a new sketch"));
        if (groupSelected) {
            doCommand(Doc,
                    "App.activeDocument().getObject('%s').addObject(App.activeDocument().addObject('Sketcher::SketchObject', '%s'))",
                    groupName.c_str(),
                    FeatName.c_str());
        }
        else {
            doCommand(Doc,
                  "App.activeDocument().addObject('Sketcher::SketchObject', '%s')",
                  FeatName.c_str());
        }

        doCommand(Doc,
                  "App.activeDocument().%s.Placement = App.Placement(App.Vector(%f, %f, %f), "
                  "App.Rotation(%f, %f, %f, %f))",
                  FeatName.c_str(),
                  p.x,
                  p.y,
                  p.z,
                  r[0],
                  r[1],
                  r[2],
                  r[3]);
        doCommand(Doc,
                  "App.activeDocument().%s.MapMode = \"%s\"",
                  FeatName.c_str(),
                  AttachEngine::getModeName(Attacher::mmDeactivated).c_str());
        doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
    }
}

bool CmdSketcherNewSketch::isActive()
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

DEF_STD_CMD_A(CmdSketcherEditSketch)

CmdSketcherEditSketch::CmdSketcherEditSketch()
    : Command("Sketcher_EditSketch")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Edit Sketch");
    sToolTipText = QT_TR_NOOP("Opens the selected sketch for editing");
    sWhatsThis = "Sketcher_EditSketch";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_EditSketch";
}

void CmdSketcherEditSketch::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::SelectionFilter SketchFilter("SELECT Sketcher::SketchObject COUNT 1");

    if (SketchFilter.match()) {
        Sketcher::SketchObject* Sketch =
            static_cast<Sketcher::SketchObject*>(SketchFilter.Result[0][0].getObject());
        doCommand(Gui, "Gui.activeDocument().setEdit('%s')", Sketch->getNameInDocument());
    }
}

bool CmdSketcherEditSketch::isActive()
{
    return Gui::Selection().countObjectsOfType<Sketcher::SketchObject>() == 1;
}

DEF_STD_CMD_A(CmdSketcherLeaveSketch)

CmdSketcherLeaveSketch::CmdSketcherLeaveSketch()
    : Command("Sketcher_LeaveSketch")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Leave Sketch");
    sToolTipText = QT_TR_NOOP("Exits the active sketch");
    sWhatsThis = "Sketcher_LeaveSketch";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_LeaveSketch";
    eType = 0;
}

void CmdSketcherLeaveSketch::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document* doc = getActiveGuiDocument();

    if (doc) {
        // checks if a Sketch Viewprovider is in Edit and is in no special mode
        SketcherGui::ViewProviderSketch* vp =
            dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
        if (vp && vp->getSketchMode() != ViewProviderSketch::STATUS_NONE)
            vp->purgeHandler();
    }

    // See also TaskDlgEditSketch::reject
    doCommand(Gui, "Gui.activeDocument().resetEdit()");
    doCommand(Doc, "App.ActiveDocument.recompute()");
}

bool CmdSketcherLeaveSketch::isActive()
{
    return isSketchInEdit(getActiveGuiDocument());
}

DEF_STD_CMD_A(CmdSketcherStopOperation)

CmdSketcherStopOperation::CmdSketcherStopOperation()
    : Command("Sketcher_StopOperation")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Stop Operation");
    sToolTipText = QT_TR_NOOP("Stops the active operation while in edit mode");


    sWhatsThis = "Sketcher_StopOperation";
    sStatusTip = sToolTipText;
    sPixmap = "process-stop";
    eType = 0;
}

void CmdSketcherStopOperation::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document* doc = getActiveGuiDocument();

    if (doc) {
        SketcherGui::ViewProviderSketch* vp =
            dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
        if (vp) {
            vp->purgeHandler();
        }
    }
}

bool CmdSketcherStopOperation::isActive()
{
    return isSketchInEdit(getActiveGuiDocument());
}

DEF_STD_CMD_A(CmdSketcherReorientSketch)

CmdSketcherReorientSketch::CmdSketcherReorientSketch()
    : Command("Sketcher_ReorientSketch")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Reorient Sketch");
    sToolTipText = QT_TR_NOOP("Places the selected sketch on one of the global coordinate planes.\n"
                              "This will clear the AttachmentSupport property.");
    sWhatsThis = "Sketcher_ReorientSketch";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_ReorientSketch";
}

void CmdSketcherReorientSketch::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Sketcher::SketchObject* sketch =
        Gui::Selection().getObjectsOfType<Sketcher::SketchObject>().front();
    if (sketch->AttachmentSupport.getValue()) {
        int ret = QMessageBox::question(
            Gui::getMainWindow(),
            qApp->translate("Sketcher_ReorientSketch", "Sketch Has Support"),
            qApp->translate("Sketcher_ReorientSketch",
                            "Sketch with a support face cannot be reoriented.\n"
                            "Detach it from the support?"),
            QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::No)
            return;
        sketch->AttachmentSupport.setValue(nullptr);
    }

    // ask user for orientation
    SketchOrientationDialog Dlg;

    if (Dlg.exec() != QDialog::Accepted)
        return;// canceled
    Base::Vector3d p = Dlg.Pos.getPosition();
    Base::Rotation r = Dlg.Pos.getRotation();

    // do the right view direction
    std::string camstring;
    switch (Dlg.DirType) {
        case 0:
            camstring = "#Inventor V2.1 ascii\\n"
                        "OrthographicCamera {\\n"
                        " viewportMapping ADJUST_CAMERA\\n"
                        "  position 0 0 87\\n"
                        "  orientation 0 0 1  0\\n"
                        "  nearDistance -112.88701\\n"
                        "  farDistance 287.28702\\n"
                        "  aspectRatio 1\\n"
                        "  focalDistance 87\\n"
                        "  height 143.52005 }";
            break;
        case 1:
            camstring = "#Inventor V2.1 ascii\\n"
                        "OrthographicCamera {\\n"
                        " viewportMapping ADJUST_CAMERA\\n"
                        "  position 0 0 -87\\n"
                        "  orientation -1 0 0  3.1415927\\n"
                        "  nearDistance -112.88701\\n"
                        "  farDistance 287.28702\\n "
                        "  aspectRatio 1\\n"
                        "  focalDistance 87\\n"
                        "  height 143.52005 }";
            break;
        case 2:
            camstring = "#Inventor V2.1 ascii\\n"
                        "OrthographicCamera {\\n"
                        " viewportMapping ADJUST_CAMERA\\n"
                        "  position 0 -87 0\\n"
                        "  orientation -1 0 0  4.712389\\n"
                        "  nearDistance -112.88701\\n"
                        "  farDistance 287.28702\\n"
                        "  aspectRatio 1\\n"
                        "  focalDistance 87\\n"
                        "  height 143.52005\\n\\n}";
            break;
        case 3:
            camstring = "#Inventor V2.1 ascii\\n"
                        "OrthographicCamera {\\n"
                        " viewportMapping ADJUST_CAMERA\\n"
                        "  position 0 87 0\\n"
                        "  orientation 0 0.70710683 0.70710683  3.1415927\\n"
                        "  nearDistance -112.88701\\n"
                        "  farDistance 287.28702\\n"
                        "  aspectRatio 1\\n"
                        "  focalDistance 87\\n"
                        "  height 143.52005\\n\\n}";
            break;
        case 4:
            camstring = "#Inventor V2.1 ascii\\n"
                        "OrthographicCamera {\\n"
                        " viewportMapping ADJUST_CAMERA\\n"
                        "  position 87 0 0\\n"
                        "  orientation 0.57735026 0.57735026 0.57735026  2.0943952\\n"
                        "  nearDistance -112.887\\n"
                        "  farDistance 287.28699\\n"
                        "  aspectRatio 1\\n"
                        "  focalDistance 87\\n"
                        "  height 143.52005\\n\\n}";
            break;
        case 5:
            camstring = "#Inventor V2.1 ascii\\n"
                        "OrthographicCamera {\\n"
                        " viewportMapping ADJUST_CAMERA\\n"
                        "  position -87 0 0\\n"
                        "  orientation -0.57735026 0.57735026 0.57735026  4.1887903\\n"
                        "  nearDistance -112.887\\n"
                        "  farDistance 287.28699\\n"
                        "  aspectRatio 1\\n"
                        "  focalDistance 87\\n"
                        "  height 143.52005\\n\\n}";
            break;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Reorient sketch"));
    Gui::cmdAppObjectArgs(
        sketch,
        "Placement = App.Placement(App.Vector(%f, %f, %f), App.Rotation(%f, %f, %f, %f))",
        p.x,
        p.y,
        p.z,
        r[0],
        r[1],
        r[2],
        r[3]);
    doCommand(Gui, "Gui.ActiveDocument.setEdit('%s')", sketch->getNameInDocument());
}

bool CmdSketcherReorientSketch::isActive()
{
    return Gui::Selection().countObjectsOfType<Sketcher::SketchObject>() == 1;
}

DEF_STD_CMD_A(CmdSketcherMapSketch)

CmdSketcherMapSketch::CmdSketcherMapSketch()
    : Command("Sketcher_MapSketch")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Attach Sketch");
    sToolTipText = QT_TR_NOOP(
        "Attaches a sketch to the selected geometry element");
    sWhatsThis = "Sketcher_MapSketch";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_MapSketch";
}

void CmdSketcherMapSketch::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QString msg_str;
    try {
        Attacher::eMapMode suggMapMode;
        std::vector<Attacher::eMapMode> validModes;

        // check that selection is valid for at least some mapping mode.
        Attacher::SuggestResult::eSuggestResult msgid = Attacher::SuggestResult::srOK;
        suggMapMode = SuggestAutoMapMode(&msgid, &msg_str, &validModes);
        bool sketchInSelection = false;
        std::vector<App::DocumentObject*> selectedSketches = Gui::Selection()
                .getObjectsOfType(Part::Part2DObject::getClassTypeId());
        App::Document* doc = App::GetApplication().getActiveDocument();
        std::vector<App::DocumentObject*> sketches =
            doc->getObjectsOfType(Part::Part2DObject::getClassTypeId());

        /** remove any sketches that are in the current selection to avoid
         *  the case where the user attaches the sketch to itself issue #17629
         *  circular dependency check happens later, but a sketch does not appear
         *  in its own outlist, so we remove it from the dialog list proactively
         *  rather than wait and generate an error after the fact.
         */
        const auto newEnd = std::ranges::remove_if(sketches,
            [&selectedSketches, &sketchInSelection](App::DocumentObject* obj) {
                if (const auto sketch = dynamic_cast<Part::Part2DObject*>(obj);
                    sketch && std::ranges::find(selectedSketches, sketch) != selectedSketches.end()) {
                    sketchInSelection = true;
                    return true;
                }
                return false;
            }).begin();
        sketches.erase(newEnd, sketches.end());

        if (sketches.empty()) {
            Gui::TranslatedUserWarning(
                doc->Label.getStrValue(),
                qApp->translate("Sketcher_MapSketch", "No sketch found"),
                sketchInSelection
                ? qApp->translate("Sketcher_MapSketch", "Cannot attach sketch to itself!")
                : qApp->translate("Sketcher_MapSketch", "The document does not contain a sketch"));

            return;
        }
        std::sort(sketches.begin(), sketches.end(), [](const auto &a, const auto &b) {
            return QString::fromUtf8(a->Label.getValue()) < QString::fromUtf8(b->Label.getValue());
        });

        bool ok;
        QStringList items;
        for (std::vector<App::DocumentObject*>::iterator it = sketches.begin();
             it != sketches.end();
             ++it)
            items.push_back(QString::fromUtf8((*it)->Label.getValue()));
        QString text = QInputDialog::getItem(
            Gui::getMainWindow(),
            qApp->translate("Sketcher_MapSketch", "Select Sketch"),
            sketchInSelection
            ? qApp->translate("Sketcher_MapSketch",
                "Select a sketch (some sketches not shown to prevent a circular dependency)")
            : qApp->translate("Sketcher_MapSketch", "Select a sketch from the list"),
            items,
            0,
            false,
            &ok,
            Qt::MSWindowsFixedSizeDialogHint);
        if (!ok)
            return;
        int index = items.indexOf(text);
        Part2DObject* sketch = static_cast<Part2DObject*>(sketches[index]);

        // check circular dependency
        std::vector<Gui::SelectionObject> selobjs = Gui::Selection().getSelectionEx();
        for (size_t i = 0; i < selobjs.size(); ++i) {
            App::DocumentObject* part = static_cast<Part::Feature*>(selobjs[i].getObject());
            if (!part) {
                assert(0);
                throw Base::ValueError(
                    "Unexpected null pointer in CmdSketcherMapSketch::activated");
            }
            if (std::vector<App::DocumentObject*> input = part->getOutListRecursive();
                std::ranges::find(input, sketch) != input.end()) {
                throw ExceptionWrongInput(
                    QT_TR_NOOP("Some of the selected objects depend on the sketch to be mapped. "
                               "Circular dependencies are not allowed."));
            }
        }

        // Ask for a new mode.
        // outline:
        //  * find out the modes that are compatible with selection.
        //  * Test if current mode is OK.
        //  * fill in the dialog
        //  * execute the dialog
        //  * collect dialog result
        //  * action

        bool bAttach = true;
        bool bCurIncompatible = false;
        // * find out the modes that are compatible with selection.
        const auto  curMapMode = eMapMode(sketch->MapMode.getValue());
        // * Test if current mode is OK.
        if (std::ranges::find(validModes, curMapMode) == validModes.end())
            bCurIncompatible = true;

        // * fill in the dialog
        validModes.insert(validModes.begin(), Attacher::mmDeactivated);
        if (bCurIncompatible)
            validModes.push_back(curMapMode);
        // bool ok; //already defined
        // QStringList items; //already defined
        items.clear();
        items.push_back(QObject::tr("Do not attach"));
        int iSugg = 0;// index of the auto-suggested mode in the list of valid modes
        int iCurr = 0;// index of current mode in the list of valid modes
        for (size_t i = 0; i < validModes.size(); ++i) {
            // Get the 2-element vector of caption, tooltip -- this class cannot use the tooltip,
            // so it is just ignored.
            auto uiStrings =
                AttacherGui::getUIStrings(AttachEnginePlane::getClassTypeId(), validModes[i]);
            items.push_back(uiStrings[0]);
            if (validModes[i] == curMapMode) {
                iCurr = items.size() - 1;
                items.back().append(
                    bCurIncompatible
                        ? qApp->translate("Sketcher_MapSketch", " (incompatible with selection)")
                        : qApp->translate("Sketcher_MapSketch", " (current)"));
            }
            if (validModes[i] == suggMapMode) {
                iSugg = items.size() - 1;
                if (iSugg == 1) {
                    iSugg = 0;// redirect deactivate to detach
                }
                else {
                    items.back().append(qApp->translate("Sketcher_MapSketch", " (suggested)"));
                }
            }
        }
        // * execute the dialog
        text = QInputDialog::getItem(
            Gui::getMainWindow(),
            qApp->translate("Sketcher_MapSketch", "Sketch Attachment"),
            bCurIncompatible
                ? qApp->translate(
                    "Sketcher_MapSketch",
                    "Current attachment mode is incompatible with the new selection.\n"
                    "Select the method to attach this sketch to selected objects.")
                : qApp->translate("Sketcher_MapSketch",
                                  "Select the method to attach this sketch to selected objects."),
            items,
            bCurIncompatible ? iSugg : iCurr,
            false,
            &ok,
            Qt::MSWindowsFixedSizeDialogHint);
        // * collect dialog result
        if (!ok)
            return;
        index = items.indexOf(text);
        if (index == 0) {
            bAttach = false;
            suggMapMode = Attacher::mmDeactivated;
        }
        else {
            bAttach = true;
            suggMapMode = validModes[index - 1];
        }

        // * action
        if (bAttach) {
            App::PropertyLinkSubList support;
            Gui::Selection().getAsPropertyLinkSubList(support);
            std::string supportString = support.getPyReprString();

            openCommand(QT_TRANSLATE_NOOP("Command", "Attach sketch"));
            Gui::cmdAppObjectArgs(
                sketch, "MapMode = \"%s\"", AttachEngine::getModeName(suggMapMode).c_str());
            Gui::cmdAppObjectArgs(sketch, "AttachmentSupport = %s", supportString.c_str());
            commitCommand();
            doCommand(Gui, "App.activeDocument().recompute()");
        }
        else {
            openCommand(QT_TRANSLATE_NOOP("Command", "Detach sketch"));
            Gui::cmdAppObjectArgs(
                sketch, "MapMode = \"%s\"", AttachEngine::getModeName(suggMapMode).c_str());
            Gui::cmdAppObjectArgs(sketch, "AttachmentSupport = None");
            commitCommand();
            doCommand(Gui, "App.activeDocument().recompute()");
        }
    }
    catch (ExceptionWrongInput& e) {
        Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                   qApp->translate("Sketcher_MapSketch", "Map sketch"),
                                   qApp->translate("Sketcher_MapSketch",
                                                   "Can't map a sketch to support:\n"
                                                   "%1")
                                       .arg(e.ErrMsg.length() ? e.ErrMsg : msg_str));
    }
}

bool CmdSketcherMapSketch::isActive()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    std::vector<Gui::SelectionObject> selobjs = Gui::Selection().getSelectionEx();
    return doc && doc->countObjectsOfType<Part::Part2DObject>() > 0 && !selobjs.empty();
}

DEF_STD_CMD_A(CmdSketcherViewSketch)

CmdSketcherViewSketch::CmdSketcherViewSketch()
    : Command("Sketcher_ViewSketch")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Align View to Sketch");
    sToolTipText = QT_TR_NOOP("Aligns the camera orientation perpendicular to the active sketch plane");
    sWhatsThis = "Sketcher_ViewSketch";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_ViewSketch";
    sAccel = "Q, P";
    eType = 0;
}

void CmdSketcherViewSketch::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document* doc = getActiveGuiDocument();
    SketcherGui::ViewProviderSketch* vp =
        dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    if (vp) {
        runCommand(Gui,
                   "Gui.ActiveDocument.ActiveView.setCameraOrientation("
                   "App.Placement(Gui.editDocument().EditingTransform).Rotation.Q)");
    }
}

bool CmdSketcherViewSketch::isActive()
{
    return isSketchInEdit(getActiveGuiDocument());
}

DEF_STD_CMD_A(CmdSketcherValidateSketch)

CmdSketcherValidateSketch::CmdSketcherValidateSketch()
    : Command("Sketcher_ValidateSketch")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Validate Sketch");
    sToolTipText = QT_TR_NOOP("Validates a sketch by checking for missing coincidences,\n"
                              "invalid constraints, and degenerate geometry");
    sWhatsThis = "Sketcher_ValidateSketch";
    sStatusTip = sToolTipText;
    eType = 0;
    sPixmap = "Sketcher_ValidateSketch";
}

void CmdSketcherValidateSketch::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> selection =
        getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());
    if (selection.size() != 1) {
        Gui::TranslatedUserWarning(
            getActiveGuiDocument(),
            qApp->translate("CmdSketcherValidateSketch", "Wrong selection"),
            qApp->translate("CmdSketcherValidateSketch", "Select only 1 sketch."));
        return;
    }

    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
    Gui::Control().showDialog(new TaskSketcherValidation(Obj));
}

bool CmdSketcherValidateSketch::isActive()
{
    if (Gui::Control().activeDialog())
        return false;
    return Gui::Selection().countObjectsOfType<Sketcher::SketchObject>() == 1;
}

DEF_STD_CMD_A(CmdSketcherMirrorSketch)

CmdSketcherMirrorSketch::CmdSketcherMirrorSketch()
    : Command("Sketcher_MirrorSketch")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Mirror Sketch");
    sToolTipText = QT_TR_NOOP("Creates a new mirrored sketch for each selected sketch\n"
                              "by using the X or Y axes, or the origin point,\n"
                              "as mirroring reference");
    sWhatsThis = "Sketcher_MirrorSketch";
    sStatusTip = sToolTipText;
    eType = 0;
    sPixmap = "Sketcher_MirrorSketch";
}

void CmdSketcherMirrorSketch::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> selection =
        getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());
    if (selection.empty()) {
        Gui::TranslatedUserWarning(
            getActiveGuiDocument(),
            qApp->translate("CmdSketcherMirrorSketch", "Wrong selection"),
            qApp->translate("CmdSketcherMirrorSketch", "Select at least 1 sketch"));
        return;
    }

    int refgeoid = -1;
    Sketcher::PointPos refposid = Sketcher::PointPos::none;
    // Ask the user the type of mirroring
    SketchMirrorDialog smd;
    if (smd.exec() != QDialog::Accepted)
        return;

    refgeoid = smd.RefGeoid;
    refposid = smd.RefPosid;

    App::Document* doc = App::GetApplication().getActiveDocument();
    openCommand(QT_TRANSLATE_NOOP("Command", "Create a mirrored sketch for each selected sketch"));

    for (std::vector<Gui::SelectionObject>::const_iterator it = selection.begin();
         it != selection.end();
         ++it) {
        // create Sketch
        std::string FeatName = getUniqueObjectName("MirroredSketch");
        doCommand(Doc,
                  "App.activeDocument().addObject('Sketcher::SketchObject', '%s')",
                  FeatName.c_str());
        Sketcher::SketchObject* mirrorsketch =
            static_cast<Sketcher::SketchObject*>(doc->getObject(FeatName.c_str()));

        const Sketcher::SketchObject* Obj =
            static_cast<const Sketcher::SketchObject*>((*it).getObject());
        Base::Placement pl = Obj->Placement.getValue();
        Base::Vector3d p = pl.getPosition();
        Base::Rotation r = pl.getRotation();

        doCommand(Doc,
                  "App.activeDocument().%s.Placement = App.Placement(App.Vector(%f, %f, %f), "
                  "App.Rotation(%f, %f, %f, %f))",
                  FeatName.c_str(),
                  p.x,
                  p.y,
                  p.z,
                  r[0],
                  r[1],
                  r[2],
                  r[3]);

        Sketcher::SketchObject* tempsketch = new Sketcher::SketchObject();
        int addedGeometries = tempsketch->addGeometry(Obj->getInternalGeometry());
        int addedConstraints = tempsketch->addConstraints(Obj->Constraints.getValues());

        std::vector<int> geoIdList;

        for (int i = 0; i <= addedGeometries; i++)
            geoIdList.push_back(i);

        tempsketch->addSymmetric(geoIdList, refgeoid, refposid);

        std::vector<Part::Geometry*> tempgeo = tempsketch->getInternalGeometry();
        std::vector<Sketcher::Constraint*> tempconstr = tempsketch->Constraints.getValues();

        // If value of addedGeometries or addedConstraints is -1, it gets added to vector begin
        // iterator and that is invalid
        std::vector<Part::Geometry*> mirrorgeo(tempgeo.begin() + (addedGeometries + 1),
                                               tempgeo.end());
        std::vector<Sketcher::Constraint*> mirrorconstr(tempconstr.begin() + (addedConstraints + 1),
                                                        tempconstr.end());

        for (std::vector<Sketcher::Constraint*>::const_iterator itc = mirrorconstr.begin();
             itc != mirrorconstr.end();
             ++itc) {

            if ((*itc)->First != Sketcher::GeoEnum::GeoUndef
                || (*itc)->First == Sketcher::GeoEnum::HAxis
                || (*itc)->First == Sketcher::GeoEnum::VAxis)
                // not x, y axes or origin
                (*itc)->First -= (addedGeometries + 1);
            if ((*itc)->Second != Sketcher::GeoEnum::GeoUndef
                || (*itc)->Second == Sketcher::GeoEnum::HAxis
                || (*itc)->Second == Sketcher::GeoEnum::VAxis)
                // not x, y axes or origin
                (*itc)->Second -= (addedGeometries + 1);
            if ((*itc)->Third != Sketcher::GeoEnum::GeoUndef
                || (*itc)->Third == Sketcher::GeoEnum::HAxis
                || (*itc)->Third == Sketcher::GeoEnum::VAxis)
                // not x, y axes or origin
                (*itc)->Third -= (addedGeometries + 1);
        }

        mirrorsketch->addGeometry(mirrorgeo);
        mirrorsketch->addConstraints(mirrorconstr);
        delete tempsketch;
    }

    doCommand(Gui, "App.activeDocument().recompute()");
}

bool CmdSketcherMirrorSketch::isActive()
{
    return Gui::Selection().countObjectsOfType<Sketcher::SketchObject>() > 0;
}

DEF_STD_CMD_A(CmdSketcherMergeSketches)

CmdSketcherMergeSketches::CmdSketcherMergeSketches()
    : Command("Sketcher_MergeSketches")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Merge Sketches");
    sToolTipText = QT_TR_NOOP("Creates a new sketch by merging at least 2 selected sketches");
    sWhatsThis = "Sketcher_MergeSketches";
    sStatusTip = sToolTipText;
    eType = 0;
    sPixmap = "Sketcher_MergeSketch";
}

void CmdSketcherMergeSketches::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> selection =
        getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());
    if (selection.size() < 2) {
        Gui::TranslatedUserWarning(
            getActiveGuiDocument(),
            qApp->translate("CmdSketcherMergeSketches", "Wrong selection"),
            qApp->translate("CmdSketcherMergeSketches", "Select at least 2 sketches"));
        return;
    }

    App::Document* doc = App::GetApplication().getActiveDocument();

    // create Sketch
    std::string FeatName = getUniqueObjectName("Sketch");

    openCommand(QT_TRANSLATE_NOOP("Command", "Merge sketches"));
    doCommand(
        Doc, "App.activeDocument().addObject('Sketcher::SketchObject', '%s')", FeatName.c_str());

    Sketcher::SketchObject* mergesketch =
        static_cast<Sketcher::SketchObject*>(doc->getObject(FeatName.c_str()));

    int baseGeometry = 0;
    int baseConstraints = 0;

    for (std::vector<Gui::SelectionObject>::const_iterator it = selection.begin();
         it != selection.end();
         ++it) {
        const Sketcher::SketchObject* Obj =
            static_cast<const Sketcher::SketchObject*>((*it).getObject());
        int addedGeometries = mergesketch->addGeometry(Obj->getInternalGeometry());

        int addedConstraints = mergesketch->addCopyOfConstraints(*Obj);

        // Coverity issue 513796: make sure the loop below can complete
        if (addedConstraints < 0) {
            continue;  // There were no constraints
        }
        if (addedConstraints - baseConstraints < 0) {
            throw Base::ValueError("Constraint error in CmdSketcherMergeSketches");
        }

        for (int i = 0; i <= (addedConstraints - baseConstraints); i++) {
            Sketcher::Constraint* constraint =
                mergesketch->Constraints.getValues()[i + baseConstraints];

            if (constraint->First != Sketcher::GeoEnum::GeoUndef
                && constraint->First != Sketcher::GeoEnum::HAxis
                && constraint->First != Sketcher::GeoEnum::VAxis)
                // not x, y axes or origin
                constraint->First += baseGeometry;
            if (constraint->Second != Sketcher::GeoEnum::GeoUndef
                && constraint->Second != Sketcher::GeoEnum::HAxis
                && constraint->Second != Sketcher::GeoEnum::VAxis)
                // not x, y axes or origin
                constraint->Second += baseGeometry;
            if (constraint->Third != Sketcher::GeoEnum::GeoUndef
                && constraint->Third != Sketcher::GeoEnum::HAxis
                && constraint->Third != Sketcher::GeoEnum::VAxis)
                // not x, y axes or origin
                constraint->Third += baseGeometry;
        }

        baseGeometry = addedGeometries + 1;
        baseConstraints = addedConstraints + 1;
    }

    // apply the placement of the first sketch in the list (#0002434)
    doCommand(Doc,
              "App.activeDocument().ActiveObject.Placement = App.activeDocument().%s.Placement",
              selection.front().getFeatName());
    doCommand(Doc, "App.activeDocument().recompute()");
}

bool CmdSketcherMergeSketches::isActive()
{
    return Gui::Selection().countObjectsOfType<Sketcher::SketchObject>() > 1;
}

// Acknowledgement of idea and original python macro goes to SpritKopf:
// https://github.com/Spritkopf/freecad-macros/blob/master/clip-sketch/clip_sketch.FCMacro
// https://forum.freecad.org/viewtopic.php?p=231481#p231085
DEF_STD_CMD_A(CmdSketcherViewSection)

CmdSketcherViewSection::CmdSketcherViewSection()
    : Command("Sketcher_ViewSection")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Toggle Section View");
    sToolTipText = QT_TR_NOOP("Toggles between section view and full view");
    sWhatsThis = "Sketcher_ViewSection";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_ViewSection";
    sAccel = "Q, S";
    eType = 0;
}

void CmdSketcherViewSection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QString cmdStr =
        QLatin1String("ActiveSketch.ViewObject.TempoVis.sketchClipPlane(ActiveSketch, None, %1)\n");
    Gui::Document* doc = getActiveGuiDocument();
    bool revert = false;
    if (doc) {
        SketcherGui::ViewProviderSketch* vp =
            dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
        if (vp) {
            revert = vp->getViewOrientationFactor() < 0 ? true : false;
        }
    }
    cmdStr = cmdStr.arg(revert ? QLatin1String("True") : QLatin1String("False"));
    doCommand(Doc, cmdStr.toLatin1());
}

bool CmdSketcherViewSection::isActive()
{
    return isSketchInEdit(getActiveGuiDocument());
}

/* Grid tool */
GridSpaceAction::GridSpaceAction(QObject* parent)
    : QWidgetAction(parent)
{
    setEnabled(false);
}

void GridSpaceAction::updateWidget()
{
    auto* sketchView = getView();

    if (sketchView) {

        auto updateCheckBox = [](QCheckBox* checkbox, bool value) {
            auto checked = checkbox->checkState() == Qt::Checked;

            if (value != checked) {
                const QSignalBlocker blocker(checkbox);
                checkbox->setChecked(value);
            }
        };

        auto updateCheckBoxFromProperty = [updateCheckBox](QCheckBox* checkbox,
                                                            App::PropertyBool& property) {
            auto propvalue = property.getValue();

            updateCheckBox(checkbox, propvalue);
        };

        updateCheckBoxFromProperty(gridShow, sketchView->ShowGrid);

        updateCheckBoxFromProperty(gridAutoSpacing, sketchView->GridAuto);

        ParameterGrp::handle hGrp = getParameterPath();
        updateCheckBox(snapToGrid, hGrp->GetBool("SnapToGrid", false));

        gridSizeBox->setValue(sketchView->GridSize.getValue());
    }
}

void GridSpaceAction::languageChange()
{
    gridShow->setText(tr("Display grid"));
    gridShow->setToolTip(tr("Toggles the visibility of the grid in the active sketch"));
    gridShow->setStatusTip(gridAutoSpacing->toolTip());

    gridAutoSpacing->setText(tr("Grid auto-spacing"));
    gridAutoSpacing->setToolTip(tr("Automatically adjusts the grid spacing based on the zoom level"));
    gridAutoSpacing->setStatusTip(gridAutoSpacing->toolTip());

    sizeLabel->setText(tr("Spacing"));
    gridSizeBox->setToolTip(tr("Distance between two subsequent grid lines"));

    snapToGrid->setText(tr("Snap to grid"));
    snapToGrid->setToolTip(
        tr("New points will snap to the nearest grid line.\nPoints must be set closer than a "
            "fifth of the grid spacing to a grid line to snap."));
    snapToGrid->setStatusTip(snapToGrid->toolTip());
}

QWidget* GridSpaceAction::createWidget(QWidget* parent)
{
    gridShow = new QCheckBox();

    gridAutoSpacing = new QCheckBox();

    snapToGrid = new QCheckBox();

    sizeLabel = new QLabel();

    gridSizeBox = new Gui::QuantitySpinBox();
    gridSizeBox->setProperty("unit", QVariant(QStringLiteral("mm")));
    gridSizeBox->setObjectName(QStringLiteral("gridSize"));
    gridSizeBox->setMaximum(99999999.0);
    gridSizeBox->setMinimum(0.001);

    QWidget* gridSizeW = new QWidget(parent);
    auto* layout = new QGridLayout(gridSizeW);
    layout->addWidget(gridShow, 0, 0, 1, 2);
    layout->addWidget(gridAutoSpacing, 1, 0, 1, 2);
    layout->addWidget(snapToGrid, 2, 0, 1, 2);
    layout->addWidget(sizeLabel, 3, 0);
    layout->addWidget(gridSizeBox, 3, 1);

    languageChange();

#if QT_VERSION >= QT_VERSION_CHECK(6,7,0)
    QObject::connect(gridShow, &QCheckBox::checkStateChanged, [this](int state) {
#else
    QObject::connect(gridShow, &QCheckBox::stateChanged, [this](int state) {
#endif
        auto* sketchView = getView();

        if (sketchView) {
            auto enable = (state == Qt::Checked);
            sketchView->ShowGrid.setValue(enable);
        }
    });

#if QT_VERSION >= QT_VERSION_CHECK(6,7,0)
    QObject::connect(gridAutoSpacing, &QCheckBox::checkStateChanged, [this](int state) {
#else
    QObject::connect(gridAutoSpacing, &QCheckBox::stateChanged, [this](int state) {
#endif
        auto* sketchView = getView();

        if (sketchView) {
            auto enable = (state == Qt::Checked);
            sketchView->GridAuto.setValue(enable);
        }
    });

#if QT_VERSION >= QT_VERSION_CHECK(6,7,0)
    QObject::connect(snapToGrid, &QCheckBox::checkStateChanged, [this](int state) {
#else
    QObject::connect(snapToGrid, &QCheckBox::stateChanged, [this](int state) {
#endif
        ParameterGrp::handle hGrp = this->getParameterPath();
        hGrp->SetBool("SnapToGrid", state == Qt::Checked);
    });

    QObject::connect(gridSizeBox,
                        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
                        [this](double val) {
                            auto* sketchView = getView();
                            if (sketchView) {
                                sketchView->GridSize.setValue(val);
                            }
                        });

    return gridSizeW;
}

ViewProviderSketch* GridSpaceAction::getView()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();

    if (doc) {
        return dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    }

    return nullptr;
}

ParameterGrp::handle GridSpaceAction::getParameterPath()
    {
        return App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher/Snap");
    }


class CmdSketcherGrid: public Gui::Command
{
public:
    CmdSketcherGrid();
    ~CmdSketcherGrid() override
    {}
    const char* className() const override
    {
        return "CmdSketcherGrid";
    }
    void languageChange() override;

protected:
    void activated(int iMsg) override;
    bool isActive() override;
    Gui::Action* createAction() override;

public:
    CmdSketcherGrid(const CmdSketcherGrid&) = delete;
    CmdSketcherGrid(CmdSketcherGrid&&) = delete;
    CmdSketcherGrid& operator=(const CmdSketcherGrid&) = delete;
    CmdSketcherGrid& operator=(CmdSketcherGrid&&) = delete;
};

CmdSketcherGrid::CmdSketcherGrid()
    : Command("Sketcher_Grid")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Toggle Grid");
    sToolTipText =
        QT_TR_NOOP("Toggles the grid display in the active sketch");
    sWhatsThis = "Sketcher_Grid";
    sStatusTip = sToolTipText;
    eType = 0;
}

void CmdSketcherGrid::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::Document* doc = getActiveGuiDocument();
    assert(doc);
    auto* sketchView = dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    assert(sketchView);

    auto value = sketchView->ShowGrid.getValue();
    sketchView->ShowGrid.setValue(!value);
}

Gui::Action* CmdSketcherGrid::createAction()
{
    auto* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    pcAction->setExclusive(false);
    applyCommandData(this->className(), pcAction);

    GridSpaceAction* gsa = new GridSpaceAction(pcAction);
    pcAction->addAction(gsa);

    _pcAction = pcAction;

    QObject::connect(pcAction, &Gui::ActionGroup::aboutToShow, [gsa](QMenu* menu) {
        Q_UNUSED(menu)
        gsa->updateWidget();
    });

    return pcAction;
}

void CmdSketcherGrid::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    auto* gsa = static_cast<GridSpaceAction*>(a[0]);
    gsa->languageChange();
}

bool CmdSketcherGrid::isActive()
{
    auto* vp = getInactiveHandlerEditModeSketchViewProvider();

    if (vp) {
        return true;
    }

    return false;
}

/* Snap tool */
SnapSpaceAction::SnapSpaceAction(QObject* parent)
        : QWidgetAction(parent)
    {
        setEnabled(false);
    }

void SnapSpaceAction::updateWidget(bool snapenabled)
{

    auto updateCheckBox = [](QCheckBox* checkbox, bool value) {
        auto checked = checkbox->checkState() == Qt::Checked;

        if (value != checked) {
            const QSignalBlocker blocker(checkbox);
            checkbox->setChecked(value);
        }
    };

    auto updateSpinBox = [](Gui::QuantitySpinBox* spinbox, double value) {
        auto currentvalue = spinbox->rawValue();

        if (currentvalue != value) {
            const QSignalBlocker blocker(spinbox);
            spinbox->setValue(value);
        }
    };

    ParameterGrp::handle hGrp = getParameterPath();

    updateCheckBox(snapToObjects, hGrp->GetBool("SnapToObjects", true));

    updateSpinBox(snapAngle, hGrp->GetFloat("SnapAngle", 5.0));

    snapToObjects->setEnabled(snapenabled);
    angleLabel->setEnabled(snapenabled);
    snapAngle->setEnabled(snapenabled);
}

void SnapSpaceAction::languageChange()
{
    snapToObjects->setText(tr("Snap to objects"));
    snapToObjects->setToolTip(tr("New points will snap to the currently preselected object. It "
                                    "will also snap to the middle of lines and arcs."));
    snapToObjects->setStatusTip(snapToObjects->toolTip());

    angleLabel->setText(tr("Snap angle"));
    snapAngle->setToolTip(
        tr("Angular step for tools that use 'Snap at angle'. Hold Ctrl to "
            "enable 'Snap at angle'. The angle starts from the positive X axis of the sketch."));
}

QWidget* SnapSpaceAction::createWidget(QWidget* parent)
{
    snapToObjects = new QCheckBox();

    angleLabel = new QLabel();

    snapAngle = new Gui::QuantitySpinBox();
    snapAngle->setProperty("unit", QVariant(QStringLiteral("deg")));
    snapAngle->setObjectName(QStringLiteral("snapAngle"));
    snapAngle->setMaximum(99999999.0);
    snapAngle->setMinimum(0);

    QWidget* snapW = new QWidget(parent);
    auto* layout = new QGridLayout(snapW);
    layout->addWidget(snapToObjects, 0, 0, 1, 2);
    layout->addWidget(angleLabel, 1, 0);
    layout->addWidget(snapAngle, 1, 1);

    languageChange();

#if QT_VERSION >= QT_VERSION_CHECK(6,7,0)
    QObject::connect(snapToObjects, &QCheckBox::checkStateChanged, [this](int state) {
#else
    QObject::connect(snapToObjects, &QCheckBox::stateChanged, [this](int state) {
#endif
        ParameterGrp::handle hGrp = this->getParameterPath();
        hGrp->SetBool("SnapToObjects", state == Qt::Checked);
    });

    QObject::connect(
        snapAngle, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), [this](double val) {
            ParameterGrp::handle hGrp = this->getParameterPath();
            hGrp->SetFloat("SnapAngle", val);
        });

    return snapW;
}

ParameterGrp::handle SnapSpaceAction::getParameterPath()
{
    return App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/Snap");
}


class CmdSketcherSnap: public Gui::Command, public ParameterGrp::ObserverType
{
public:
    CmdSketcherSnap();
    ~CmdSketcherSnap() override;
    CmdSketcherSnap(const CmdSketcherSnap&) = delete;
    CmdSketcherSnap(CmdSketcherSnap&&) = delete;
    CmdSketcherSnap& operator=(const CmdSketcherSnap&) = delete;
    CmdSketcherSnap& operator=(CmdSketcherSnap&&) = delete;
    const char* className() const override
    {
        return "CmdSketcherSnap";
    }
    void languageChange() override;

    void OnChange(Base::Subject<const char*>& rCaller, const char* sReason) override;

protected:
    void activated(int iMsg) override;
    bool isActive() override;
    Gui::Action* createAction() override;

private:
    ParameterGrp::handle getParameterPath()
    {
        return App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher/Snap");
    }

    bool snapEnabled = true;
};

CmdSketcherSnap::CmdSketcherSnap()
    : Command("Sketcher_Snap")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Toggle Snap");
    sToolTipText =
        QT_TR_NOOP("Toggles snapping");
    sWhatsThis = "Sketcher_Snap";
    sStatusTip = sToolTipText;
    eType = 0;

    ParameterGrp::handle hGrp = this->getParameterPath();
    hGrp->Attach(this);
}

CmdSketcherSnap::~CmdSketcherSnap()
{

    ParameterGrp::handle hGrp = this->getParameterPath();
    hGrp->Detach(this);
}

void CmdSketcherSnap::OnChange(Base::Subject<const char*>& rCaller, const char* sReason)
{
    Q_UNUSED(rCaller)

    if (strcmp(sReason, "Snap") == 0) {
        snapEnabled = getParameterPath()->GetBool("Snap", true);
    }
}

void CmdSketcherSnap::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    getParameterPath()->SetBool("Snap", !snapEnabled);

    // Update the widget :
    if (!_pcAction)
        return;

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    auto* ssa = static_cast<SnapSpaceAction*>(a[0]);
    ssa->updateWidget(snapEnabled);
}

Gui::Action* CmdSketcherSnap::createAction()
{
    auto* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    pcAction->setExclusive(false);
    applyCommandData(this->className(), pcAction);

    SnapSpaceAction* ssa = new SnapSpaceAction(pcAction);
    pcAction->addAction(ssa);

    _pcAction = pcAction;

    QObject::connect(pcAction, &Gui::ActionGroup::aboutToShow, [ssa, this](QMenu* menu) {
        Q_UNUSED(menu)
        ssa->updateWidget(snapEnabled);
    });

    return pcAction;
}

void CmdSketcherSnap::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    auto* ssa = static_cast<SnapSpaceAction*>(a[0]);
    ssa->languageChange();
}

bool CmdSketcherSnap::isActive()
{
    auto* vp = getInactiveHandlerEditModeSketchViewProvider();

    if (vp) {
        return true;
    }

    return false;
}


/* Rendering Order */
RenderingOrderAction::RenderingOrderAction(QObject* parent)
        : QWidgetAction(parent)
{
    setEnabled(false);
}

void RenderingOrderAction::updateWidget()
{
    auto hGrp = getParameterPath();

    // 1->Normal Geometry, 2->Construction, 3->External
    int topid = hGrp->GetInt("TopRenderGeometryId", 1);
    int midid = hGrp->GetInt("MidRenderGeometryId", 2);
    int lowid = hGrp->GetInt("LowRenderGeometryId", 3);

    auto idToText = [](int id) -> QString {
        switch (id) {
        case 1:
            return tr("Normal geometry");
        case 2:
            return tr("Construction geometry");
        case 3:
            return tr("External geometry");
        default:
            // Fallback for an unexpected ID
            return tr("Unknown geometry");
        }
    };

    {
        QSignalBlocker block(this);
        list->clear();

        QListWidgetItem* itemTop = new QListWidgetItem;
        itemTop->setData(Qt::UserRole, QVariant(topid));
        itemTop->setText(idToText(topid));
        list->insertItem(0, itemTop);

        QListWidgetItem* itemMid = new QListWidgetItem;
        itemMid->setData(Qt::UserRole, QVariant(midid));
        itemMid->setText(idToText(midid));
        list->insertItem(1, itemMid);

        QListWidgetItem* itemLow = new QListWidgetItem;
        itemLow->setData(Qt::UserRole, QVariant(lowid));
        itemLow->setText(idToText(lowid));
        list->insertItem(2, itemLow);
    }
}

void RenderingOrderAction::languageChange()
{
    updateWidget();
}

QWidget* RenderingOrderAction::createWidget(QWidget* parent)
{
    list = new QListWidget();
    list->setDragDropMode(QAbstractItemView::InternalMove);
    list->setDefaultDropAction(Qt::MoveAction);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setDragEnabled(true);
    list->setFixedSize(200, 50);


    QWidget* renderingWidget = new QWidget(parent);
    auto* label = new QLabel(tr("Rendering order"), renderingWidget);
    auto* layout = new QVBoxLayout(renderingWidget);
    layout->addWidget(label);
    layout->addWidget(list);

    languageChange();

    // Handle change in the order of the list entries
    QObject::connect(list->model(),
                        &QAbstractItemModel::rowsMoved,
                        [this](const QModelIndex& sourceParent,
                            int sourceStart,
                            int sourceEnd,
                            const QModelIndex& destinationParent,
                            int destinationRow) {
                            Q_UNUSED(sourceParent)
                            Q_UNUSED(sourceStart)
                            Q_UNUSED(sourceEnd)
                            Q_UNUSED(destinationParent)
                            Q_UNUSED(destinationRow)

                            int topid = list->item(0)->data(Qt::UserRole).toInt();
                            int midid = list->item(1)->data(Qt::UserRole).toInt();
                            int lowid = list->item(2)->data(Qt::UserRole).toInt();

                            auto hGrp = getParameterPath();

                            hGrp->SetInt("TopRenderGeometryId", topid);
                            hGrp->SetInt("MidRenderGeometryId", midid);
                            hGrp->SetInt("LowRenderGeometryId", lowid);
                        });

    return renderingWidget;
}

ParameterGrp::handle RenderingOrderAction::getParameterPath()
    {
        return App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    }


class CmdRenderingOrder: public Gui::Command, public ParameterGrp::ObserverType
{
    enum class ElementType
    {
        Normal = 1,
        Construction = 2,
        External = 3,
    };

public:
    CmdRenderingOrder();
    ~CmdRenderingOrder() override;
    CmdRenderingOrder(const CmdRenderingOrder&) = delete;
    CmdRenderingOrder(CmdRenderingOrder&&) = delete;
    CmdRenderingOrder& operator=(const CmdRenderingOrder&) = delete;
    CmdRenderingOrder& operator=(CmdRenderingOrder&&) = delete;
    const char* className() const override
    {
        return "CmdRenderingOrder";
    }
    void languageChange() override;
    void OnChange(Base::Subject<const char*>& rCaller, const char* sReason) override;

protected:
    void activated(int iMsg) override;
    bool isActive() override;
    Gui::Action* createAction() override;

private:
    ParameterGrp::handle getParameterPath()
    {
        return App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    }

    ElementType TopElement = ElementType::Normal;
};

CmdRenderingOrder::CmdRenderingOrder()
    : Command("Sketcher_RenderingOrder")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Rendering Order");
    sToolTipText = QT_TR_NOOP("Reorders items in the rendering order");
    sWhatsThis = "Sketcher_RenderingOrder";
    sStatusTip = sToolTipText;
    eType = 0;

    ParameterGrp::handle hGrp = this->getParameterPath();
    hGrp->Attach(this);

    TopElement = static_cast<ElementType>(getParameterPath()->GetInt("TopRenderGeometryId", 1));
}

CmdRenderingOrder::~CmdRenderingOrder()
{

    ParameterGrp::handle hGrp = this->getParameterPath();
    hGrp->Detach(this);
}

void CmdRenderingOrder::OnChange(Base::Subject<const char*>& rCaller, const char* sReason)
{
    Q_UNUSED(rCaller)

    if (strcmp(sReason, "TopRenderGeometryId") == 0) {
        TopElement = static_cast<ElementType>(getParameterPath()->GetInt("TopRenderGeometryId", 1));
    }
}

void CmdRenderingOrder::activated(int iMsg)
{
    Q_UNUSED(iMsg);
}

Gui::Action* CmdRenderingOrder::createAction()
{
    auto* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    pcAction->setExclusive(false);
    applyCommandData(this->className(), pcAction);

    RenderingOrderAction* roa = new RenderingOrderAction(pcAction);
    pcAction->addAction(roa);

    _pcAction = pcAction;

    QObject::connect(pcAction, &Gui::ActionGroup::aboutToShow, [roa](QMenu* menu) {
        Q_UNUSED(menu)
        roa->updateWidget();
    });

    return pcAction;
}

void CmdRenderingOrder::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    auto* roa = static_cast<RenderingOrderAction*>(a[0]);
    roa->languageChange();
}

bool CmdRenderingOrder::isActive()
{
    return isSketchInEdit(getActiveGuiDocument());
    ;
}


void CreateSketcherCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdSketcherNewSketch());
    rcCmdMgr.addCommand(new CmdSketcherEditSketch());
    rcCmdMgr.addCommand(new CmdSketcherLeaveSketch());
    rcCmdMgr.addCommand(new CmdSketcherStopOperation());
    rcCmdMgr.addCommand(new CmdSketcherReorientSketch());
    rcCmdMgr.addCommand(new CmdSketcherMapSketch());
    rcCmdMgr.addCommand(new CmdSketcherViewSketch());
    rcCmdMgr.addCommand(new CmdSketcherValidateSketch());
    rcCmdMgr.addCommand(new CmdSketcherMirrorSketch());
    rcCmdMgr.addCommand(new CmdSketcherMergeSketches());
    rcCmdMgr.addCommand(new CmdSketcherViewSection());
    rcCmdMgr.addCommand(new CmdSketcherGrid());
    rcCmdMgr.addCommand(new CmdSketcherSnap());
    rcCmdMgr.addCommand(new CmdRenderingOrder());
}
// clang-format on
