/***************************************************************************
 *   Copyright (c) 2024 Martin Rodriguez Reboredo <yakoyoku@gmail.com>     *
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
#include <QApplication>
#include <QCheckBox>
#include <QGridLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QWidgetAction>
#endif

#include <App/DocumentObjectGroup.h>
#include <App/OriginFeature.h>
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
#include <Gui/SelectionFilter.h>
#include <Gui/SelectionObject.h>
#include <Mod/Part/App/Attacher.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Part/Gui/AttacherTexts.h>
#include <Mod/Text/App/ShapeText.h>

#include "TextOrientationDialog.h"
#include "ViewProviderShapeText.h"
#include "Utils.h"


using namespace TextGui;


namespace TextGui
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

    ~ExceptionWrongInput() throw() override
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
    Attacher::AttachEngine3D eng;
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
            if (tmpSupport.getSubValues().front().substr(0, 4) == "Face")
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

}// namespace TextGui


/* Sketch commands =======================================================*/
DEF_STD_CMD_A(CmdTextNewText)

CmdTextNewText::CmdTextNewText()
    : Command("Text_NewText")
{
    sAppModule = "Text";
    sGroup = "Text";
    sMenuText = QT_TR_NOOP("Create text");
    sToolTipText = QT_TR_NOOP("Create a new text.");
    sWhatsThis = "Text_NewText";
    sStatusTip = sToolTipText;
    sPixmap = "Text_NewText";
}

void CmdTextNewText::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Attacher::eMapMode mapmode = Attacher::mmDeactivated;
    bool bAttach = false;
    if (Gui::Selection().hasSelection()) {
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
                QObject::tr("Text mapping"),
                QObject::tr("Can't map the text to selected object. %1.").arg(msg_str));
            return;
        }
        if (validModes.size() > 1) {
            validModes.insert(validModes.begin(), Attacher::mmDeactivated);
            bool ok;
            QStringList items;
            items.push_back(QObject::tr("Don't attach"));
            int iSugg = 0;// index of the auto-suggested mode in the list of valid modes
            for (size_t i = 0; i < validModes.size(); ++i) {
                auto uiStrings =
                    AttacherGui::getUIStrings(Attacher::AttachEnginePlane::getClassTypeId(), validModes[i]);
                items.push_back(uiStrings[0]);
                if (validModes[i] == mapmode)
                    iSugg = items.size() - 1;
            }
            QString text = QInputDialog::getItem(
                Gui::getMainWindow(),
                qApp->translate("Text_NewText", "Text attachment"),
                qApp->translate("Text_NewText",
                                "Select the method to attach this text to selected object"),
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
        std::string FeatName = getUniqueObjectName("Text");

        openCommand(QT_TRANSLATE_NOOP("Command", "Create a new text on a face"));
        doCommand(Doc,
                  "App.activeDocument().addObject('Text::ShapeText', '%s')",
                  FeatName.c_str());
        if (mapmode < Attacher::mmDummy_NumberOfModes)
            doCommand(Gui,
                      "App.activeDocument().%s.MapMode = \"%s\"",
                      FeatName.c_str(),
                      Attacher::AttachEngine::getModeName(mapmode).c_str());
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
        TextOrientationDialog Dlg;

        Dlg.adjustSize();
        if (Dlg.exec() != QDialog::Accepted)
            return;// canceled
        Base::Vector3d p = Dlg.Pos.getPosition();
        Base::Rotation r = Dlg.Pos.getRotation();

        std::string FeatName = getUniqueObjectName("Text");

        openCommand(QT_TRANSLATE_NOOP("Command", "Create a new text"));
        doCommand(Doc,
                  "App.activeDocument().addObject('Text::ShapeText', '%s')",
                  FeatName.c_str());
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
                  Attacher::AttachEngine::getModeName(Attacher::mmDeactivated).c_str());
        doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
    }
}

bool CmdTextNewText::isActive()
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

DEF_STD_CMD_A(CmdTextEditText)

CmdTextEditText::CmdTextEditText()
    : Command("Text_EditText")
{
    sAppModule = "Text";
    sGroup = "Text";
    sMenuText = QT_TR_NOOP("Edit text");
    sToolTipText = QT_TR_NOOP("Edit the selected text.");
    sWhatsThis = "Text_EditText";
    sStatusTip = sToolTipText;
    sPixmap = "Text_EditText";
}

void CmdTextEditText::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::SelectionFilter textFilter("SELECT Text::ShapeText COUNT 1");

    if (textFilter.match()) {
        Text::ShapeText* text =
            static_cast<Text::ShapeText*>(textFilter.Result[0][0].getObject());
        doCommand(Gui, "Gui.activeDocument().setEdit('%s')", text->getNameInDocument());
    }
}

bool CmdTextEditText::isActive()
{
    return Gui::Selection().countObjectsOfType(Text::ShapeText::getClassTypeId()) == 1;
}

DEF_STD_CMD_A(CmdTextLeaveText)

CmdTextLeaveText::CmdTextLeaveText()
    : Command("Text_LeaveText")
{
    sAppModule = "Text";
    sGroup = "Text";
    sMenuText = QT_TR_NOOP("Leave text");
    sToolTipText = QT_TR_NOOP("Finish editing the active text.");
    sWhatsThis = "Text_LeaveText";
    sStatusTip = sToolTipText;
    sPixmap = "Text_LeaveText";
    eType = 0;
}

void CmdTextLeaveText::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document* doc = getActiveGuiDocument();

    if (doc) {
        // checks if a ShapeText Viewprovider is in Edit and is in no special mode
        TextGui::ViewProviderShapeText* vp =
            dynamic_cast<TextGui::ViewProviderShapeText*>(doc->getInEdit());
        // if (vp && vp->getShapeTextMode() != ViewProviderShapeText::STATUS_NONE)
            // vp->purgeHandler();
    }

    // See also TaskDlgEditSketch::reject
    doCommand(Gui, "Gui.activeDocument().resetEdit()");
    doCommand(Doc, "App.ActiveDocument.recompute()");
}

bool CmdTextLeaveText::isActive()
{
    return isShapeTextInEdit(getActiveGuiDocument());
}

DEF_STD_CMD_A(CmdTextStopOperation)

CmdTextStopOperation::CmdTextStopOperation()
    : Command("Text_StopOperation")
{
    sAppModule = "Text";
    sGroup = "Text";
    sMenuText = QT_TR_NOOP("Stop operation");
    sToolTipText = QT_TR_NOOP("When in edit mode, "
                              "stop the active operation "
                              "(drawing, constraining, etc.).");
    sWhatsThis = "Text_StopOperation";
    sStatusTip = sToolTipText;
    sPixmap = "process-stop";
    eType = 0;
}

void CmdTextStopOperation::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document* doc = getActiveGuiDocument();

    if (doc) {
        TextGui::ViewProviderShapeText* vp =
            dynamic_cast<TextGui::ViewProviderShapeText*>(doc->getInEdit());
        // if (vp) {
            // vp->purgeHandler();
        // }
    }
}

bool CmdTextStopOperation::isActive()
{
    return isShapeTextInEdit(getActiveGuiDocument());
}

DEF_STD_CMD_A(CmdTextReorientText)

CmdTextReorientText::CmdTextReorientText()
    : Command("Text_ReorientText")
{
    sAppModule = "Text";
    sGroup = "Text";
    sMenuText = QT_TR_NOOP("Reorient text...");
    sToolTipText = QT_TR_NOOP("Place the selected text on one of the global coordinate planes.\n"
                              "This will clear the 'Support' property, if any.");
    sWhatsThis = "Text_ReorientText";
    sStatusTip = sToolTipText;
    sPixmap = "Text_ReorientText";
}

void CmdTextReorientText::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Text::ShapeText* text =
        Gui::Selection().getObjectsOfType<Text::ShapeText>().front();
    if (text->AttachmentSupport.getValue()) {
        int ret = QMessageBox::question(
            Gui::getMainWindow(),
            qApp->translate("Text_ReorientText", "Text has support"),
            qApp->translate("Text_ReorientText",
                            "Text with a support face cannot be reoriented.\n"
                            "Do you want to detach it from the support?"),
            QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::No)
            return;
        text->AttachmentSupport.setValue(nullptr);
    }

    // ask user for orientation
    TextOrientationDialog Dlg;

    if (Dlg.exec() != QDialog::Accepted)
        return;// canceled
    Base::Vector3d p = Dlg.Pos.getPosition();
    Base::Rotation r = Dlg.Pos.getRotation();

    openCommand(QT_TRANSLATE_NOOP("Command", "Reorient text"));
    Gui::cmdAppObjectArgs(
        text,
        "Placement = App.Placement(App.Vector(%f, %f, %f), App.Rotation(%f, %f, %f, %f))",
        p.x,
        p.y,
        p.z,
        r[0],
        r[1],
        r[2],
        r[3]);
    doCommand(Gui, "Gui.ActiveDocument.setEdit('%s')", text->getNameInDocument());
}

bool CmdTextReorientText::isActive()
{
    return Gui::Selection().countObjectsOfType(Text::ShapeText::getClassTypeId()) == 1;
}

DEF_STD_CMD_A(CmdTextMapText)

CmdTextMapText::CmdTextMapText()
    : Command("Text_MapText")
{
    sAppModule = "Text";
    sGroup = "Text";
    sMenuText = QT_TR_NOOP("Map text to face...");
    sToolTipText = QT_TR_NOOP(
        "Set the 'Support' of a text.\n"
        "First select the supporting geometry, for example, a face or an edge of a solid object,\n"
        "then call this command, then choose the desired text.");
    sWhatsThis = "Text_MapText";
    sStatusTip = sToolTipText;
    sPixmap = "Text_MapText";
}

void CmdTextMapText::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QString msg_str;
    try {
        Attacher::eMapMode suggMapMode;
        std::vector<Attacher::eMapMode> validModes;

        // check that selection is valid for at least some mapping mode.
        Attacher::SuggestResult::eSuggestResult msgid = Attacher::SuggestResult::srOK;
        suggMapMode = SuggestAutoMapMode(&msgid, &msg_str, &validModes);

        App::Document* doc = App::GetApplication().getActiveDocument();
        std::vector<App::DocumentObject*> texts =
            doc->getObjectsOfType(Part::Part2DObject::getClassTypeId());
        if (texts.empty()) {
            Gui::TranslatedUserWarning(
                doc->Label.getStrValue(),
                qApp->translate("Text_MapText", "No sketch found"),
                qApp->translate("Text_MapText", "The document doesn't have a sketch"));
            return;
        }

        bool ok;
        QStringList items;
        for (std::vector<App::DocumentObject*>::iterator it = texts.begin();
             it != texts.end();
             ++it)
            items.push_back(QString::fromUtf8((*it)->Label.getValue()));
        QString name = QInputDialog::getItem(
            Gui::getMainWindow(),
            qApp->translate("Text_MapText", "Select text"),
            qApp->translate("Text_MapText", "Select a text from the list"),
            items,
            0,
            false,
            &ok,
            Qt::MSWindowsFixedSizeDialogHint);
        if (!ok)
            return;
        int index = items.indexOf(name);
        Part::Part2DObject* text = static_cast<Part::Part2DObject*>(texts[index]);

        // check circular dependency
        std::vector<Gui::SelectionObject> selobjs = Gui::Selection().getSelectionEx();
        for (size_t i = 0; i < selobjs.size(); ++i) {
            App::DocumentObject* part = static_cast<Part::Feature*>(selobjs[i].getObject());
            if (!part) {
                assert(0);
                throw Base::ValueError(
                    "Unexpected null pointer in CmdTextMapSketch::activated");
            }
            std::vector<App::DocumentObject*> input = part->getOutListRecursive();
            if (std::find(input.begin(), input.end(), text) != input.end()) {
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
        Attacher::eMapMode curMapMode = Attacher::eMapMode(text->MapMode.getValue());
        // * Test if current mode is OK.
        if (std::find(validModes.begin(), validModes.end(), curMapMode) == validModes.end())
            bCurIncompatible = true;

        // * fill in the dialog
        validModes.insert(validModes.begin(), Attacher::mmDeactivated);
        if (bCurIncompatible)
            validModes.push_back(curMapMode);
        // bool ok; //already defined
        // QStringList items; //already defined
        items.clear();
        items.push_back(QObject::tr("Don't attach"));
        int iSugg = 0;// index of the auto-suggested mode in the list of valid modes
        int iCurr = 0;// index of current mode in the list of valid modes
        for (size_t i = 0; i < validModes.size(); ++i) {
            // Get the 2-element vector of caption, tooltip -- this class cannot use the tooltip,
            // so it is just ignored.
            auto uiStrings =
                AttacherGui::getUIStrings(Attacher::AttachEnginePlane::getClassTypeId(), validModes[i]);
            items.push_back(uiStrings[0]);
            if (validModes[i] == curMapMode) {
                iCurr = items.size() - 1;
                items.back().append(
                    bCurIncompatible
                        ? qApp->translate("Text_MapText", " (incompatible with selection)")
                        : qApp->translate("Text_MapText", " (current)"));
            }
            if (validModes[i] == suggMapMode) {
                iSugg = items.size() - 1;
                if (iSugg == 1) {
                    iSugg = 0;// redirect deactivate to detach
                }
                else {
                    items.back().append(qApp->translate("Text_MapText", " (suggested)"));
                }
            }
        }
        // * execute the dialog
        name = QInputDialog::getItem(
            Gui::getMainWindow(),
            qApp->translate("Text_MapText", "Text attachment"),
            bCurIncompatible
                ? qApp->translate(
                    "Text_MapText",
                    "Current attachment mode is incompatible with the new selection.\n"
                    "Select the method to attach this text to selected objects.")
                : qApp->translate("Text_MapText",
                                  "Select the method to attach this text to selected objects."),
            items,
            bCurIncompatible ? iSugg : iCurr,
            false,
            &ok,
            Qt::MSWindowsFixedSizeDialogHint);
        // * collect dialog result
        if (!ok)
            return;
        index = items.indexOf(name);
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

            openCommand(QT_TRANSLATE_NOOP("Command", "Attach text"));
            Gui::cmdAppObjectArgs(
                text, "MapMode = \"%s\"", Attacher::AttachEngine::getModeName(suggMapMode).c_str());
            Gui::cmdAppObjectArgs(text, "Support = %s", supportString.c_str());
            commitCommand();
            doCommand(Gui, "App.activeDocument().recompute()");
        }
        else {
            openCommand(QT_TRANSLATE_NOOP("Command", "Detach text"));
            Gui::cmdAppObjectArgs(
                text, "MapMode = \"%s\"", Attacher::AttachEngine::getModeName(suggMapMode).c_str());
            Gui::cmdAppObjectArgs(text, "Support = None");
            commitCommand();
            doCommand(Gui, "App.activeDocument().recompute()");
        }
    }
    catch (ExceptionWrongInput& e) {
        Gui::TranslatedUserWarning(getActiveGuiDocument(),
                                   qApp->translate("Text_MapText", "Map text"),
                                   qApp->translate("Text_MapText",
                                                   "Can't map a text to support:\n"
                                                   "%1")
                                       .arg(e.ErrMsg.length() ? e.ErrMsg : msg_str));
    }
}

bool CmdTextMapText::isActive()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    Base::Type textType = Base::Type::fromName("Text::ShapeText");
    std::vector<Gui::SelectionObject> selobjs = Gui::Selection().getSelectionEx();
    if (doc && doc->countObjectsOfType(textType) > 0 && !selobjs.empty())
        return true;

    return false;
}

DEF_STD_CMD_A(CmdTextViewText)

CmdTextViewText::CmdTextViewText()
    : Command("Text_ViewText")
{
    sAppModule = "Text";
    sGroup = "Text";
    sMenuText = QT_TR_NOOP("View text");
    sToolTipText = QT_TR_NOOP("When in edit mode, "
                              "set the camera orientation perpendicular to the textplane.");
    sWhatsThis = "Text_ViewText";
    sStatusTip = sToolTipText;
    sPixmap = "Text_ViewText";
    sAccel = "Q, P";
    eType = 0;
}

void CmdTextViewText::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document* doc = getActiveGuiDocument();
    TextGui::ViewProviderShapeText* vp =
        dynamic_cast<TextGui::ViewProviderShapeText*>(doc->getInEdit());
    if (vp) {
        runCommand(Gui,
                   "Gui.ActiveDocument.ActiveView.setCameraOrientation("
                   "App.Placement(Gui.editDocument().EditingTransform).Rotation.Q)");
    }
}

bool CmdTextViewText::isActive()
{
    return isShapeTextInEdit(getActiveGuiDocument());
}

// Acknowledgement of idea and original python macro goes to SpritKopf:
// https://github.com/Spritkopf/freecad-macros/blob/master/clip-sketch/clip_sketch.FCMacro
// https://forum.freecad.org/viewtopic.php?p=231481#p231085
DEF_STD_CMD_A(CmdTextViewSection)

CmdTextViewSection::CmdTextViewSection()
    : Command("Text_ViewSection")
{
    sAppModule = "Text";
    sGroup = "Text";
    sMenuText = QT_TR_NOOP("View section");
    sToolTipText = QT_TR_NOOP("When in edit mode, "
                              "switch between section view and full view.");
    sWhatsThis = "Text_ViewSection";
    sStatusTip = sToolTipText;
    sPixmap = "Text_ViewSection";
    sAccel = "Q, S";
    eType = 0;
}

void CmdTextViewSection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QString cmdStr =
        QLatin1String("ActiveText.ViewObject.TempoVis.sketchClipPlane(ActiveText, None, %1)\n");
    Gui::Document* doc = getActiveGuiDocument();
    bool revert = false;
    if (doc) {
        TextGui::ViewProviderShapeText* vp =
            dynamic_cast<TextGui::ViewProviderShapeText*>(doc->getInEdit());
        if (vp) {
            revert = vp->getViewOrientationFactor() < 0 ? true : false;
        }
    }
    cmdStr = cmdStr.arg(revert ? QLatin1String("True") : QLatin1String("False"));
    doCommand(Doc, cmdStr.toLatin1());
}

bool CmdTextViewSection::isActive()
{
    return isShapeTextInEdit(getActiveGuiDocument());
}


void CreateTextCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTextNewText());
    rcCmdMgr.addCommand(new CmdTextEditText());
    rcCmdMgr.addCommand(new CmdTextLeaveText());
    rcCmdMgr.addCommand(new CmdTextStopOperation());
    rcCmdMgr.addCommand(new CmdTextReorientText());
    rcCmdMgr.addCommand(new CmdTextMapText());
    rcCmdMgr.addCommand(new CmdTextViewText());
    rcCmdMgr.addCommand(new CmdTextViewSection());
    // rcCmdMgr.addCommand(new CmdTextGrid());
    // rcCmdMgr.addCommand(new CmdTextSnap());
    // rcCmdMgr.addCommand(new CmdRenderingOrder());
}
