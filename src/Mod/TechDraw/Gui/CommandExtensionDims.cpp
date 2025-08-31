/***************************************************************************
 *   Copyright (c) 2021 edi                                                *
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
# include <cmath>
# include <sstream>
# include <BRepGProp.hxx>
# include <GProp_GProps.hxx>
#endif

# include <App/Document.h>
# include <App/DocumentObject.h>
# include <Base/Console.h>
# include <Base/Type.h>
# include <Base/Tools.h>
# include <Gui/Action.h>
# include <Gui/Application.h>
# include <Gui/BitmapFactory.h>
# include <Gui/Command.h>
# include <Gui/Control.h>
# include <Gui/MainWindow.h>
# include <Gui/Selection/Selection.h>
# include <Gui/Selection/SelectionObject.h>

# include <Mod/TechDraw/App/Cosmetic.h>
# include <Mod/TechDraw/App/DrawViewBalloon.h>
# include <Mod/TechDraw/App/DrawViewDimension.h>
# include <Mod/TechDraw/App/DrawPage.h>
# include <Mod/TechDraw/App/DrawUtil.h>
# include <Mod/TechDraw/App/DrawViewPart.h>
# include <Mod/TechDraw/App/Preferences.h>
# include <Mod/TechDraw/App/LineGroup.h>

#include "DlgTemplateField.h"
#include "DrawGuiUtil.h"
#include "TaskCustomizeFormat.h"
#include "TaskSelectLineAttributes.h"
#include "CommandExtensionDims.h"


using namespace TechDrawGui;
using namespace TechDraw;

namespace TechDrawGui {
    //internal structures and sort functions
    struct dimVertex {
        // save a dimension defining vertex and its point
        std::string name;
        Base::Vector3d point;
    };

    struct {
        bool operator()(dimVertex a, dimVertex b) const { return a.point.x < b.point.x; }
    } sortX;

    struct {
        bool operator()(dimVertex a, dimVertex b) const { return a.point.y < b.point.y; }
    } sortY;

    //internal helper functions
    void _selectDimensionAttributes(Gui::Command* cmd);
    std::vector<TechDraw::DrawViewDimension*>_getDimensions(std::vector<Gui::SelectionObject> selection, std::string needDimType);
    std::vector<dimVertex> _getVertexInfo(TechDraw::DrawViewPart* objFeat,
        std::vector<std::string> subNames);
    TechDraw::DrawViewDimension* _createLinDimension(
        TechDraw::DrawViewPart* objFeat,
        std::string startVertex,
        std::string endVertex,
        std::string dimType);
    bool _checkSelection(Gui::Command* cmd,
        std::vector<Gui::SelectionObject>& selection,
        std::string message);
    bool _checkSelAndObj(Gui::Command* cmd,
        std::vector<Gui::SelectionObject>& selection,
        TechDraw::DrawViewPart*& objFeat,
        std::string message);
    bool _checkSelObjAndSubs(Gui::Command* cmd,
        std::vector<Gui::SelectionObject>& selection,
        TechDraw::DrawViewPart*& objFeat,
        std::string message);
}

//===========================================================================
// TechDraw_ExtensionInsertDiameter
//===========================================================================
void execInsertPrefixChar(Gui::Command* cmd, const std::string& prefixFormat) {
    // insert a prefix character into the format specifier
    std::vector<Gui::SelectionObject> selection;
    if (!_checkSelection(cmd, selection, QObject::tr("TechDraw Insert Prefix").toStdString())) {
        return;
    }

    std::string prefixText(prefixFormat);
    if (prefixFormat.find("%s") != std::string::npos) {
        DlgTemplateField ui(Gui::getMainWindow());
        ui.setFieldName(QObject::tr("Repeat count").toStdString());
        ui.setFieldContent("1");
        if (ui.exec() != QDialog::Accepted) {
            return;
        }

        QString numberFromDialog = ui.getFieldContent();
        QString qPrefixText = QStringLiteral("%1× ").arg(numberFromDialog);
        prefixText = qPrefixText.toStdString();
    }
    size_t prefixSize = prefixText.capacity();

    Gui::Command::openCommand(QObject::tr("Insert Prefix").toStdString().c_str());
    for (auto selected : selection) {
        auto object = selected.getObject();
        if (object->isDerivedFrom<TechDraw::DrawViewDimension>()) {
            auto dim = static_cast<TechDraw::DrawViewDimension*>(selected.getObject());
            std::string formatSpec = dim->FormatSpec.getStrValue();
            formatSpec.reserve(formatSpec.capacity() + prefixSize);
            formatSpec.insert(0, prefixText);
            dim->FormatSpec.setValue(formatSpec);
        }
    }
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionInsertDiameter)

CmdTechDrawExtensionInsertDiameter::CmdTechDrawExtensionInsertDiameter()
    : Command("TechDraw_ExtensionInsertDiameter")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert '⌀' Prefix");
    sToolTipText    = QT_TR_NOOP("Inserts a '⌀' symbol at the beginning of the dimension");
    sWhatsThis      = "TechDraw_ExtensionInsertDiameter";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionInsertDiameter";
}

void CmdTechDrawExtensionInsertDiameter::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execInsertPrefixChar(this, "⌀");
}

bool CmdTechDrawExtensionInsertDiameter::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionInsertSquare
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionInsertSquare)

CmdTechDrawExtensionInsertSquare::CmdTechDrawExtensionInsertSquare()
    : Command("TechDraw_ExtensionInsertSquare")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert '□' Prefix");
    sToolTipText    = QT_TR_NOOP("Inserts a '□' symbol at the beginning of the dimension");
    sWhatsThis      = "TechDraw_ExtensionInsertSquare";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionInsertSquare";
}

void CmdTechDrawExtensionInsertSquare::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execInsertPrefixChar(this, "□");    //□ white square U+25A1
}

bool CmdTechDrawExtensionInsertSquare::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionInsertRepetition
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionInsertRepetition)

CmdTechDrawExtensionInsertRepetition::CmdTechDrawExtensionInsertRepetition()
    : Command("TechDraw_ExtensionInsertRepetition")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert 'n×' Prefix");
    sToolTipText    = QT_TR_NOOP("Inserts a repeated feature count at the beginning of the dimension");
    sWhatsThis      = "TechDraw_ExtensionInsertRepetition";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionInsertRepetition";
}

void CmdTechDrawExtensionInsertRepetition::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    execInsertPrefixChar(this, "%s× "); //× Multiplication sign U+00D7
}

bool CmdTechDrawExtensionInsertRepetition::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionRemovePrefixChar
//===========================================================================

void execRemovePrefixChar(Gui::Command* cmd) {
    // remove a prefix character from the format specifier
    std::vector<Gui::SelectionObject> selection;
    if (!_checkSelection(cmd, selection, QT_TRANSLATE_NOOP("Command","TechDraw Remove Prefix"))) {
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Remove Prefix"));
    for (auto selected : selection)
    {
        auto object = selected.getObject();
        if (object->isDerivedFrom<TechDraw::DrawViewDimension>()) {
            auto dim = static_cast<TechDraw::DrawViewDimension*>(selected.getObject());
            std::string formatSpec = dim->FormatSpec.getStrValue();
            int pos = formatSpec.find("%.");
            if (pos != 0)
            {
                formatSpec = formatSpec.substr(pos);
                dim->FormatSpec.setValue(formatSpec);
            }
        }
    }
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionRemovePrefixChar)

CmdTechDrawExtensionRemovePrefixChar::CmdTechDrawExtensionRemovePrefixChar()
    : Command("TechDraw_ExtensionRemovePrefixChar")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Remove Prefix");
    sToolTipText    = QT_TR_NOOP("Removes the prefix symbols at the beginning of the dimension");
    sWhatsThis      = "TechDraw_ExtensionRemovePrefixChar";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionRemovePrefixChar";
}

void CmdTechDrawExtensionRemovePrefixChar::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execRemovePrefixChar(this);
}

bool CmdTechDrawExtensionRemovePrefixChar::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionInsertPrefixGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawExtensionInsertPrefixGroup)

CmdTechDrawExtensionInsertPrefixGroup::CmdTechDrawExtensionInsertPrefixGroup()
    : Command("TechDraw_ExtensionInsertPrefixGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert '⌀' Prefix");
    sToolTipText    = QT_TR_NOOP("Inserts a '⌀' symbol at the beginning of the dimension text");
    sWhatsThis      = "TechDraw_ExtensionInsertPrefixGroup";
    sStatusTip      = sMenuText;
}

void CmdTechDrawExtensionInsertPrefixGroup::activated(int iMsg)
{
    //    Base::Console().message("CMD::ExtensionLinePPGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task in progress"),
            QObject::tr("Close active task dialog and try again"));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch (iMsg) {
    case 0:                 //insert "⌀" as prefix
        execInsertPrefixChar(this, "⌀");
        break;
    case 1:                 //insert "□" as prefix
        execInsertPrefixChar(this, "□");
        break;
    case 2:                 //insert "n×" as prefix
        execInsertPrefixChar(this, "%s× ");
        break;
    case 3:                 //remove prefix characters
        execRemovePrefixChar(this);
        break;
    default:
        Base::Console().message("CMD::CVGrp - invalid iMsg: %d\n", iMsg);
    };
}

Gui::Action* CmdTechDrawExtensionInsertPrefixGroup::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionInsertDiameter"));
    p1->setObjectName(QStringLiteral("TechDraw_ExtensionInsertDiameter"));
    p1->setWhatsThis(QStringLiteral("TechDraw_ExtensionInsertDiameter"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionInsertSquare"));
    p2->setObjectName(QStringLiteral("TechDraw_ExtensionInsertSquare"));
    p2->setWhatsThis(QStringLiteral("TechDraw_ExtensionInsertSquare"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionInsertRepetition"));
    p3->setObjectName(QStringLiteral("TechDraw_ExtensionInsertRepetition"));
    p3->setWhatsThis(QStringLiteral("TechDraw_ExtensionInsertRepetition"));
    QAction* p4 = pcAction->addAction(QString());
    p4->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionRemovePrefixChar"));
    p4->setObjectName(QStringLiteral("TechDraw_ExtensionRemovePrefixChar"));
    p4->setWhatsThis(QStringLiteral("TechDraw_ExtensionRemovePrefixChar"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawExtensionInsertPrefixGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdTechDrawExtensionInsertDiameter", "Insert '⌀' Prefix"));
    arc1->setToolTip(QApplication::translate("CmdTechDrawExtensionInsertDiameter",
"Inserts a '⌀' symbol at the beginning of the dimension"));
    arc1->setStatusTip(arc1->text());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdTechDrawExtensionInsertSquare", "Insert '□' Prefix"));
    arc2->setToolTip(QApplication::translate("CmdTechDrawExtensionInsertSquare",
"Inserts a '□' symbol at the beginning of the dimension"));
    arc2->setStatusTip(arc2->text());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("CmdTechDrawExtensionInsertRepetition", "Insert 'n×' Prefix"));
    arc3->setToolTip(QApplication::translate("CmdTechDrawExtensionInsertRepetition",
"Inserts a repeated feature count at the beginning of the dimension"));
    arc3->setStatusTip(arc3->text());
    QAction* arc4 = a[3];
    arc4->setText(QApplication::translate("TechDraw_ExtensionremovePrefixChar", "Remove Prefix"));
    arc4->setToolTip(QApplication::translate("TechDraw_ExtensionremovePrefixChar",
"Removes the prefix symbols at the beginning of the dimension"));
    arc4->setStatusTip(arc4->text());
}

bool CmdTechDrawExtensionInsertPrefixGroup::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionIncreaseDecimal
//===========================================================================

void execIncreaseDecreaseDecimal(Gui::Command* cmd, int delta) {
    // increase or decrease number of decimal places of a measure
    std::vector<Gui::SelectionObject> selection;
    if (!_checkSelection(cmd, selection, QT_TRANSLATE_NOOP("QObject","TechDraw Increase/Decrease Decimal"))) {
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Increase/Decrease Decimal"));
    std::string numStr;
    for (auto selected : selection) {
        auto object = selected.getObject();
        if (object->isDerivedFrom<TechDraw::DrawViewDimension>()) {
            auto dim = static_cast<TechDraw::DrawViewDimension*>(selected.getObject());
            std::string formatSpec = dim->FormatSpec.getStrValue();
            std::string searchStr("%.");
            int numFound = formatSpec.find(searchStr) + 2;
            numStr = formatSpec[numFound];
            int numInt = std::stoi(numStr, nullptr);
            numInt = numInt + delta;
            if (numInt >= 0 && numInt <= 9) {
                numStr = std::to_string(numInt);
                formatSpec.replace(numFound, 1, numStr);
                dim->FormatSpec.setValue(formatSpec);
            }
        }
    }
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionIncreaseDecimal)

CmdTechDrawExtensionIncreaseDecimal::CmdTechDrawExtensionIncreaseDecimal()
    : Command("TechDraw_ExtensionIncreaseDecimal")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Increase Decimal Places");
    sToolTipText    = QT_TR_NOOP("Increases the number of decimal places of the dimension");
    sWhatsThis      = "TechDraw_ExtensionIncreaseDecimal";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionIncreaseDecimal";
}

void CmdTechDrawExtensionIncreaseDecimal::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execIncreaseDecreaseDecimal(this, 1);
}

bool CmdTechDrawExtensionIncreaseDecimal::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionDecreaseDecimal
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionDecreaseDecimal)

CmdTechDrawExtensionDecreaseDecimal::CmdTechDrawExtensionDecreaseDecimal()
    : Command("TechDraw_ExtensionDecreaseDecimal")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Decrease Decimal Places");
    sToolTipText    = QT_TR_NOOP("Decreases the number of decimal places of the dimension");
    sWhatsThis      = "TechDraw_ExtensionDecreaseDecimal";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionDecreaseDecimal";
}

void CmdTechDrawExtensionDecreaseDecimal::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execIncreaseDecreaseDecimal(this, -1);
}

bool CmdTechDrawExtensionDecreaseDecimal::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionIncreaseDecreaseGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawExtensionIncreaseDecreaseGroup)

CmdTechDrawExtensionIncreaseDecreaseGroup::CmdTechDrawExtensionIncreaseDecreaseGroup()
    : Command("TechDraw_ExtensionIncreaseDecreaseGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Increase Decimal Places");
    sToolTipText    = QT_TR_NOOP("Increases the number of decimal places of the dimension");
    sWhatsThis      = "TechDraw_ExtensionIncreaseDecreaseGroup";
    sStatusTip      = sMenuText;
}

void CmdTechDrawExtensionIncreaseDecreaseGroup::activated(int iMsg)
{
    //    Base::Console().message("CMD::ExtensionIncreaseDecreaseGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task in progress"),
            QObject::tr("Close active task dialog and try again"));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch (iMsg) {
    case 0:                 //increase decimal places
        execIncreaseDecreaseDecimal(this, 1);
        break;
    case 1:                 //decrease decimal places
        execIncreaseDecreaseDecimal(this, -1);
        break;
    default:
        Base::Console().message("CMD::CVGrp - invalid iMsg: %d\n", iMsg);
    };
}

Gui::Action* CmdTechDrawExtensionIncreaseDecreaseGroup::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionIncreaseDecimal"));
    p1->setObjectName(QStringLiteral("TechDraw_ExtensionIncreaseDecimal"));
    p1->setWhatsThis(QStringLiteral("TechDraw_ExtensionIncreaseDecimal"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionDecreaseDecimal"));
    p2->setObjectName(QStringLiteral("TechDraw_ExtensionDecreaseDecimal"));
    p2->setWhatsThis(QStringLiteral("TechDraw_ExtensionDecreaseDecimal"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawExtensionIncreaseDecreaseGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdTechDrawExtensionIncreaseDecimal", "Increase Decimal Places"));
    arc1->setToolTip(QApplication::translate("CmdTechDrawExtensionIncreaseDecimal",
"Increases the number of decimal places of the dimension"));
    arc1->setStatusTip(arc1->text());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdTechDrawExtensionDecreaseDecimal", "Decrease Decimal Places"));
    arc2->setToolTip(QApplication::translate("CmdTechDrawExtensionDecreaseDecimal",
"Decreases the number of decimal places of the dimension"));
    arc2->setStatusTip(arc2->text());
}

bool CmdTechDrawExtensionIncreaseDecreaseGroup::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionPosHorizChainDimension
//===========================================================================

void execPosHorizChainDimension(Gui::Command* cmd) {
    // position a horizontal dimension chain
    std::vector<Gui::SelectionObject> selection;
    if (!_checkSelection(cmd, selection, QT_TRANSLATE_NOOP("QObject","TechDraw PosHorizChainDimension"))) {
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Position Horizontal Chain Dimension"));
    std::vector<TechDraw::DrawViewDimension*> validDimension;
    validDimension = _getDimensions(selection, "DistanceX");
    if (validDimension.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("TechDraw PosHorizChainDimension"),
            QObject::tr("No horizontal dimensions selected"));
        return;
    }
    float yMaster = validDimension[0]->Y.getValue();
    for (auto dim : validDimension) {
        dim->Y.setValue(yMaster);
        pointPair pp = dim->getLinearPoints();
        Base::Vector3d p1 = pp.first();
        Base::Vector3d p2 = pp.second();
        dim->X.setValue((p1.x + p2.x) / 2.0);
    }
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionPosHorizChainDimension)

CmdTechDrawExtensionPosHorizChainDimension::CmdTechDrawExtensionPosHorizChainDimension()
    : Command("TechDraw_ExtensionPosHorizChainDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Align Chain Dimensions Horizontally");
    sToolTipText    = QT_TR_NOOP("Aligns the horizontal dimensions to create a chain dimension:<br>\
- Select two or more horizontal dimensions<br>\
- The first dimension defines the position<br>\
- Click this tool");
    sWhatsThis      = "TechDraw_ExtensionPosHorizChainDimension";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionPosHorizChainDimension";
}

void CmdTechDrawExtensionPosHorizChainDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execPosHorizChainDimension(this);
}

bool CmdTechDrawExtensionPosHorizChainDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionPosVertChainDimension
//===========================================================================

void execPosVertChainDimension(Gui::Command* cmd) {
    // position a vertical dimension chain
    std::vector<Gui::SelectionObject> selection;
    if (!_checkSelection(cmd, selection, QT_TRANSLATE_NOOP("QObject","TechDraw PosVertChainDimension"))) {
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Position Vert Chain Dimension"));
    std::vector<TechDraw::DrawViewDimension*> validDimension;
    validDimension = _getDimensions(selection, "DistanceY");
    if (validDimension.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("TechDraw PosVertChainDimension"),
            QObject::tr("No vertical dimensions selected"));
        return;
    }
    float xMaster = validDimension[0]->X.getValue();
    double fontSize = Preferences::dimFontSizeMM();
    for (auto dim : validDimension) {
        dim->X.setValue(xMaster);
        pointPair pp = dim->getLinearPoints();
        Base::Vector3d p1 = pp.first();
        Base::Vector3d p2 = pp.second();
        dim->Y.setValue((p1.y + p2.y) / -2.0 + 0.5 * fontSize);
    }
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionPosVertChainDimension)

CmdTechDrawExtensionPosVertChainDimension::CmdTechDrawExtensionPosVertChainDimension()
    : Command("TechDraw_ExtensionPosVertChainDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Align Chain Dimensions Vertically");
    sToolTipText    = QT_TR_NOOP("Aligns the vertical dimensions to create a chain dimension:<br>\
- Select two or more vertical dimensions<br>\
- The first dimension defines the position<br>\
- Click this tool");
    sWhatsThis      = "TechDraw_ExtensionPosVertChainDimension";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionPosVertChainDimension";
}

void CmdTechDrawExtensionPosVertChainDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execPosVertChainDimension(this);
}

bool CmdTechDrawExtensionPosVertChainDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionPosObliqueChainDimension
//===========================================================================

void execPosObliqueChainDimension(Gui::Command* cmd) {
    // position an oblique dimension chain
    std::vector<Gui::SelectionObject> selection;
    if (!_checkSelection(cmd, selection, QT_TRANSLATE_NOOP("QObject","TechDraw PosObliqueChainDimension"))) {
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Position Oblique Chain Dimension"));
    std::vector<TechDraw::DrawViewDimension*> validDimension;
    validDimension = _getDimensions(selection, "Distance");
    if (validDimension.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("TechDraw PosObliqueChainDimension"),
            QObject::tr("No oblique dimensions selected"));
        return;
    }
    float xMaster = validDimension[0]->X.getValue();
    float yMaster = validDimension[0]->Y.getValue();
    Base::Vector3d pMaster(xMaster, yMaster, 0.0);
    pointPair pp = validDimension[0]->getLinearPoints();
    Base::Vector3d dirMaster = pp.second() - pp.first();
    dirMaster.y = -dirMaster.y;
    for (auto dim : validDimension) {
        float xDim = dim->X.getValue();
        float yDim = dim->Y.getValue();
        Base::Vector3d pDim(xDim, yDim, 0.0);
        Base::Vector3d p3 = DrawUtil::getTrianglePoint(pMaster, dirMaster, pDim);
        dim->X.setValue(p3.x);
        dim->Y.setValue(p3.y);
    }
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionPosObliqueChainDimension)

CmdTechDrawExtensionPosObliqueChainDimension::CmdTechDrawExtensionPosObliqueChainDimension()
    : Command("TechDraw_ExtensionPosObliqueChainDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Align Oblique Chain Dimensions");
    sToolTipText    = QT_TR_NOOP("Aligns the oblique dimensions to create a chain dimension:<br>\
- Select two or more parallel oblique dimensions<br>\
- The first dimension defines the position<br>\
- Click this tool");
    sWhatsThis      = "TechDraw_ExtensionPosObliqueChainDimension";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionPosObliqueChainDimension";
}

void CmdTechDrawExtensionPosObliqueChainDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execPosObliqueChainDimension(this);
    ///Base::Console().message("TechDraw_ExtensionPosObliqueChainDimension started\n");
}

bool CmdTechDrawExtensionPosObliqueChainDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionPosChainDimensionGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawExtensionPosChainDimensionGroup)

CmdTechDrawExtensionPosChainDimensionGroup::CmdTechDrawExtensionPosChainDimensionGroup()
    : Command("TechDraw_ExtensionPosChainDimensionGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Align Horizontal Chain Dimensions");
    sToolTipText    = QT_TR_NOOP("Aligns the horizontal dimensions to create a chain dimension:<br>\
- Select two or more horizontal dimensions<br>\
- The first dimension defines the position<br>\
- Click this tool");
    sWhatsThis      = "TechDraw_ExtensionPosChainDimensionGroup";
    sStatusTip      = sMenuText;
}

void CmdTechDrawExtensionPosChainDimensionGroup::activated(int iMsg)
{
    //    Base::Console().message("CMD::ExtensionPosChainDimensionGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task in progress"),
            QObject::tr("Close active task dialog and try again"));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch (iMsg) {
    case 0:                 //position horizontal chain dimensions
        execPosHorizChainDimension(this);
        break;
    case 1:                 //position vertical chain dimensions
        execPosVertChainDimension(this);
        break;
    case 2:                 //position oblique chain dimensions
        execPosObliqueChainDimension(this);
        break;
    default:
        Base::Console().message("CMD::CVGrp - invalid iMsg: %d\n", iMsg);
    };
}

Gui::Action* CmdTechDrawExtensionPosChainDimensionGroup::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionPosHorizChainDimension"));
    p1->setObjectName(QStringLiteral("TechDraw_ExtensionPosHorizChainDimension"));
    p1->setWhatsThis(QStringLiteral("TechDraw_ExtensionPosHorizChainDimension"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionPosVertChainDimension"));
    p2->setObjectName(QStringLiteral("TechDraw_ExtensionPosVertChainDimension"));
    p2->setWhatsThis(QStringLiteral("sion"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionPosObliqueChainDimension"));
    p3->setObjectName(QStringLiteral("TechDraw_ExtensionPosObliqueChainDimension"));
    p3->setWhatsThis(QStringLiteral("TechDraw_ExtensionPosObliqueChainDimension"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawExtensionPosChainDimensionGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdTechDrawExtensionPosHorizChainDimension", "Position Horizontal Chain Dimensions"));
    arc1->setToolTip(QApplication::translate("CmdTechDrawExtensionPosHorizChainDimension",
"Aligns the horizontal dimensions to create a chain dimension:<br>\
- Select two or more horizontal dimensions<br>\
- The first dimension defines the position<br>\
- Click this tool"));
    arc1->setStatusTip(arc1->text());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdTechDrawExtensionPosVertChainDimension", "Position Vertical Chain Dimensions"));
    arc2->setToolTip(QApplication::translate("CmdTechDrawExtensionPosVertChainDimension",
"Aligns the vertical dimensions to create a chain dimension:<br>\
- Select two or more vertical dimensions<br>\
- The first dimension defines the position<br>\
- Click this tool"));
    arc2->setStatusTip(arc2->text());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("CmdTechDrawExtensionPosObliqueChainDimension", "Position Oblique Chain Dimensions"));
    arc3->setToolTip(QApplication::translate("CmdTechDrawExtensionPosObliqueChainDimension",
"Aligns the oblique dimensions to create a chain dimension:<br>\
- Select two or more parallel oblique dimensions<br>\
- The first dimension defines the position<br>\
- Click this tool"));
    arc3->setStatusTip(arc3->text());
}

bool CmdTechDrawExtensionPosChainDimensionGroup::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCascadeHorizDimension
//===========================================================================

void execCascadeHorizDimension(Gui::Command* cmd) {
    // cascade horizontal dimensions
    std::vector<Gui::SelectionObject> selection;
    if (!_checkSelection(cmd, selection, QT_TRANSLATE_NOOP("QObject","TechDraw CascadeHorizDimension"))) {
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cascade Horizontal Dimension"));
    std::vector<TechDraw::DrawViewDimension*> validDimension;
    validDimension = _getDimensions(selection, "DistanceX");
    if (validDimension.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("TechDraw CascadeHorizDimension"),
            QObject::tr("No horizontal dimensions selected"));
        return;
    }
    float yMaster = validDimension[0]->Y.getValue();
    float dimDistance = activeDimAttributes.getCascadeSpacing();
    if (std::signbit(yMaster))
        dimDistance = -dimDistance;
    for (auto dim : validDimension) {
        dim->Y.setValue(yMaster);
        pointPair pp = dim->getLinearPoints();
        Base::Vector3d p1 = pp.first();
        Base::Vector3d p2 = pp.second();
        dim->X.setValue((p1.x + p2.x) / 2.0);
        yMaster = yMaster + dimDistance;
    }
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionCascadeHorizDimension)

CmdTechDrawExtensionCascadeHorizDimension::CmdTechDrawExtensionCascadeHorizDimension()
    : Command("TechDraw_ExtensionCascadeHorizDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cascade Horizontal Dimensions");
    sToolTipText    = QT_TR_NOOP("Evenly spaces the selected horizontal dimensions:<br>\
- Specify the cascade spacing (optional)<br>\
- Select two or more horizontal dimensions<br>\
- The first dimension defines the position<br>\
- Click this tool");
    sWhatsThis      = "TechDraw_ExtensionCascadeHorizDimension";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionCascadeHorizDimension";
}

void CmdTechDrawExtensionCascadeHorizDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCascadeHorizDimension(this);
}

bool CmdTechDrawExtensionCascadeHorizDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCascadeVertDimension
//===========================================================================

void execCascadeVertDimension(Gui::Command* cmd) {
    // cascade vertical dimensions
    std::vector<Gui::SelectionObject> selection;
    if (!_checkSelection(cmd, selection, QT_TRANSLATE_NOOP("QObject","TechDraw CascadeVertDimension"))) {
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cascade Vertical Dimension"));
    std::vector<TechDraw::DrawViewDimension*> validDimension;
    validDimension = _getDimensions(selection, "DistanceY");
    if (validDimension.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("TechDraw CascadeVertDimension"),
            QObject::tr("No vertical dimensions selected"));
        return;
    }
    float xMaster = validDimension[0]->X.getValue();
    float dimDistance = activeDimAttributes.getCascadeSpacing();
    if (std::signbit(xMaster))
        dimDistance = -dimDistance;
    double fontSize = Preferences::dimFontSizeMM();
    for (auto dim : validDimension) {
        dim->X.setValue(xMaster);
        pointPair pp = dim->getLinearPoints();
        Base::Vector3d p1 = pp.first();
        Base::Vector3d p2 = pp.second();
        dim->Y.setValue((p1.y + p2.y) / -2.0 + 0.5 * fontSize);
        xMaster = xMaster + dimDistance;
    }
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionCascadeVertDimension)

CmdTechDrawExtensionCascadeVertDimension::CmdTechDrawExtensionCascadeVertDimension()
    : Command("TechDraw_ExtensionCascadeVertDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cascade Vertical Dimensions");
    sToolTipText    = QT_TR_NOOP("Evenly spaces the selected vertical dimensions:<br>\
- Specify the cascade spacing (optional)<br>\
- Select two or more vertical dimensions<br>\
- The first dimension defines the position<br>\
- Click this tool");
    sWhatsThis      = "TechDraw_ExtensionCascadeVertDimension";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionCascadeVertDimension";
}

void CmdTechDrawExtensionCascadeVertDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCascadeVertDimension(this);
}

bool CmdTechDrawExtensionCascadeVertDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCascadeObliqueDimension
//===========================================================================

void execCascadeObliqueDimension(Gui::Command* cmd) {
    // cascade oblique dimensions
    std::vector<Gui::SelectionObject> selection;
    if (!_checkSelection(cmd, selection, QT_TRANSLATE_NOOP("QObject","TechDraw CascadeObliqueDimension"))) {
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cascade Oblique Dimension"));
    std::vector<TechDraw::DrawViewDimension*> validDimension;
    validDimension = _getDimensions(selection, "Distance");
    if (validDimension.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("TechDraw CascadeObliqueDimension"),
            QObject::tr("No oblique dimensions selected"));
        return;
    }
    float xMaster = validDimension[0]->X.getValue();
    float yMaster = validDimension[0]->Y.getValue();
    Base::Vector3d pMaster(xMaster, yMaster, 0.0);
    pointPair pp = validDimension[0]->getLinearPoints();
    Base::Vector3d dirMaster = pp.second() - pp.first();
    dirMaster.y = -dirMaster.y;
    Base::Vector3d origin(0.0, 0.0, 0.0);
    Base::Vector3d ipDelta = DrawUtil::getTrianglePoint(pMaster, dirMaster, origin);
    float dimDistance = activeDimAttributes.getCascadeSpacing();
    Base::Vector3d delta = ipDelta.Normalize() * dimDistance;
    int i = 0;
    for (auto dim : validDimension) {
        float xDim = dim->X.getValue();
        float yDim = dim->Y.getValue();
        Base::Vector3d pDim(xDim, yDim, 0.0);
        Base::Vector3d p3 = DrawUtil::getTrianglePoint(pMaster, dirMaster, pDim);
        p3 = p3 + delta * i;
        dim->X.setValue(p3.x);
        dim->Y.setValue(p3.y);
        i = i + 1;
    }
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionCascadeObliqueDimension)

CmdTechDrawExtensionCascadeObliqueDimension::CmdTechDrawExtensionCascadeObliqueDimension()
    : Command("TechDraw_ExtensionCascadeObliqueDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cascade Oblique Dimensions");
    sToolTipText    = QT_TR_NOOP("Evenly spaces the selected oblique dimensions:<br>\
- Specify the cascade spacing (optional)<br>\
- Select two or more parallel oblique dimensions<br>\
- The first dimension defines the position<br>\
- Click this tool");
    sWhatsThis      = "TechDraw_ExtensionCascadeObliqueDimension";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionCascadeObliqueDimension";
}

void CmdTechDrawExtensionCascadeObliqueDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCascadeObliqueDimension(this);
    ///Base::Console().message("TechDraw_ExtensionPosObliqueChainDimension started\n");
}

bool CmdTechDrawExtensionCascadeObliqueDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCascadeDimensionGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawExtensionCascadeDimensionGroup)

CmdTechDrawExtensionCascadeDimensionGroup::CmdTechDrawExtensionCascadeDimensionGroup()
    : Command("TechDraw_ExtensionCascadeDimensionGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cascade Horizontal Dimensions");
    sToolTipText    = QT_TR_NOOP("Evenly spaces the selected horizontal dimensions:<br>\
- Specify the cascade spacing (optional)<br>\
- Select two or more horizontal dimensions<br>\
- The first dimension defines the position<br>\
- Click this tool");
    sWhatsThis      = "TechDraw_ExtensionCascadeDimensionGroup";
    sStatusTip      = sMenuText;
}

void CmdTechDrawExtensionCascadeDimensionGroup::activated(int iMsg)
{
    //    Base::Console().message("CMD::ExtensionCascadeDimansionGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task in progress"),
            QObject::tr("Close active task dialog and try again"));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch (iMsg) {
    case 0:                 //cascade horizontal dimensions
        execCascadeHorizDimension(this);
        break;
    case 1:                 //cascade vertical dimensions
        execCascadeVertDimension(this);
        break;
    case 2:                 //cascade oblique dimensions
        execCascadeObliqueDimension(this);
        break;
    default:
        Base::Console().message("CMD::CVGrp - invalid iMsg: %d\n", iMsg);
    };
}

Gui::Action* CmdTechDrawExtensionCascadeDimensionGroup::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCascadeHorizDimension"));
    p1->setObjectName(QStringLiteral("TechDraw_ExtensionCascadeHorizDimension"));
    p1->setWhatsThis(QStringLiteral("TechDraw_ExtensionCascadeHorizDimension"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCascadeVertDimension"));
    p2->setObjectName(QStringLiteral("TechDraw_ExtensionCascadeVertDimension"));
    p2->setWhatsThis(QStringLiteral("TechDraw_ExtensionCascadeVertDimension"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCascadeObliqueDimension"));
    p3->setObjectName(QStringLiteral("TechDraw_ExtensionCascadeObliqueDimension"));
    p3->setWhatsThis(QStringLiteral("TechDraw_ExtensionCascadeObliqueDimension"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawExtensionCascadeDimensionGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdTechDrawExtensionCascadeHorizDimension", "Cascade Horizontal Dimensions"));
    arc1->setToolTip(QApplication::translate("CmdTechDrawExtensionCascadeHorizDimension",
"Evenly spaces the selected horizontal dimensions:<br>\
- Specify the cascade spacing (optional)<br>\
- Select two or more horizontal dimensions<br>\
- The first dimension defines the position<br>\
- Click this tool"));
    arc1->setStatusTip(arc1->text());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdTechDrawExtensionCascadeVertDimension", "Cascade Vertical Dimensions"));
    arc2->setToolTip(QApplication::translate("CmdTechDrawExtensionCascadeVertDimension",
"Evenly spaces the selected vertical dimensions:<br>\
- Specify the cascade spacing (optional)<br>\
- Select two or more vertical dimensions<br>\
- The first dimension defines the position<br>\
- Click this tool"));
    arc2->setStatusTip(arc2->text());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("CmdTechDrawExtensionCascadeObliqueDimension", "Cascade Oblique Dimensions"));
    arc3->setToolTip(QApplication::translate("CmdTechDrawExtensionCascadeObliqueDimension",
"Evenly spaces the selected oblique dimensions:<br>\
- Specify the cascade spacing (optional)<br>\
- Select two or more parallel oblique dimensions<br>\
- The first dimension defines the position<br>\
- Click this tool"));
    arc3->setStatusTip(arc3->text());
}

bool CmdTechDrawExtensionCascadeDimensionGroup::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateHorizChainDimension
//===========================================================================

void execCreateHorizChainDimension(Gui::Command* cmd) {
    //create a horizontal chain dimension
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSelObjAndSubs(cmd, selection, objFeat, QT_TRANSLATE_NOOP("QObject","TechDraw Create Horizontal Chain Dimension"))) {
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Horizontal Chain Dimension"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    std::vector<dimVertex> allVertexes;
    allVertexes = _getVertexInfo(objFeat, subNames);
    if (!allVertexes.empty() && allVertexes.size() > 1) {
        std::sort(allVertexes.begin(), allVertexes.end(), sortX);
        float yMaster = 0.0;
        for (long unsigned int n = 0; n < allVertexes.size() - 1; n++) {
            TechDraw::DrawViewDimension* dim;
            dim = _createLinDimension(objFeat, allVertexes[n].name, allVertexes[n + 1].name, "DistanceX");
            TechDraw::pointPair pp = dim->getLinearPoints();
            Base::Vector3d mid = (pp.first() + pp.second()) / 2.0;
            dim->X.setValue(mid.x);
            if (n == 0)
                yMaster = -mid.y;
            dim->Y.setValue(yMaster);
        }
    }
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
    cmd->getSelection().clearSelection();
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionCreateHorizChainDimension)

CmdTechDrawExtensionCreateHorizChainDimension::CmdTechDrawExtensionCreateHorizChainDimension()
    : Command("TechDraw_ExtensionCreateHorizChainDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Horizontal Chain Dimension");
    sToolTipText    = QT_TR_NOOP("Inserts a sequence of aligned horizontal dimensions to at least "
            "three selected vertices");
    sWhatsThis      = "TechDraw_ExtensionCreateHorizChainDimension";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionCreateHorizChainDimension";
}

void CmdTechDrawExtensionCreateHorizChainDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCreateHorizChainDimension(this);
    //execSortieren(this);
}

bool CmdTechDrawExtensionCreateHorizChainDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateVertChainDimension
//===========================================================================

void execCreateVertChainDimension(Gui::Command* cmd) {
    //create a vertical chain dimension
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSelObjAndSubs(cmd, selection, objFeat, QT_TRANSLATE_NOOP("QObject","TechDraw Create Vertical Chain Dimension"))) {
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Vert Chain dimension"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    std::vector<dimVertex> allVertexes;
    allVertexes = _getVertexInfo(objFeat, subNames);
    if (!allVertexes.empty() && allVertexes.size() > 1) {
        std::sort(allVertexes.begin(), allVertexes.end(), sortY);
        float xMaster = 0.0;
        double fontSize = Preferences::dimFontSizeMM();
        for (long unsigned int n = 0; n < allVertexes.size() - 1; n++) {
            TechDraw::DrawViewDimension* dim;
            dim = _createLinDimension(objFeat, allVertexes[n].name, allVertexes[n + 1].name, "DistanceY");
            TechDraw::pointPair pp = dim->getLinearPoints();
            Base::Vector3d mid = (pp.first() + pp.second()) / 2.0;
            if (n == 0)
                xMaster = mid.x;
            dim->X.setValue(xMaster);
            dim->Y.setValue(-mid.y + 0.5 * fontSize);
        }
    }
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
    cmd->getSelection().clearSelection();
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionCreateVertChainDimension)

CmdTechDrawExtensionCreateVertChainDimension::CmdTechDrawExtensionCreateVertChainDimension()
    : Command("TechDraw_ExtensionCreateVertChainDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Vertical Chain Dimension");
    sToolTipText    = QT_TR_NOOP("Inserts a sequence of aligned vertical dimensions to at least "
            "three selected vertices");
    sWhatsThis      = "TechDraw_ExtensionCreateVertChainDimension";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionCreateVertChainDimension";
}

void CmdTechDrawExtensionCreateVertChainDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCreateVertChainDimension(this);
}

bool CmdTechDrawExtensionCreateVertChainDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateObliqueChainDimension
//===========================================================================

void execCreateObliqueChainDimension(Gui::Command* cmd) {
    // create an oblique chain dimension
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSelObjAndSubs(cmd, selection, objFeat, QT_TRANSLATE_NOOP("QObject","TechDraw Create Oblique Chain Dimension"))){
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create oblique chain dimension"));

    std::vector<TechDraw::ReferenceEntry> refs;
    for (auto& subName : selection[0].getSubNames()) {
        refs.push_back(ReferenceEntry(objFeat, subName));
    }

    auto dims = makeObliqueChainDimension(refs);
    if(dims.empty()){
        Gui::Command::abortCommand();
    }
    else {
        objFeat->refreshCEGeoms();
        objFeat->requestPaint();
        cmd->getSelection().clearSelection();
        Gui::Command::commitCommand();
    }
}

std::vector<DrawViewDimension*> TechDrawGui::makeObliqueChainDimension(std::vector<TechDraw::ReferenceEntry> refs)
{
    if (refs.empty()) {
        return {};
    }

    std::vector<std::string> subNames;
    auto* objFeat = static_cast<DrawViewPart*>(refs[0].getObject());
    for (auto& ref : refs) {
        subNames.push_back(ref.getSubName());
    }
    std::vector<DrawViewDimension*> dims;

    std::vector<dimVertex> allVertexes, carrierVertexes;
    allVertexes = _getVertexInfo(objFeat, subNames);
    if (!allVertexes.empty() && allVertexes.size() > 1) {
        Base::Vector3d pMaster = allVertexes[0].point;
        Base::Vector3d dirMaster = pMaster - allVertexes[1].point;
        Base::Vector3d origin(0.0, 0.0, 0.0);
        Base::Vector3d delta = DrawUtil::getTrianglePoint(pMaster, dirMaster, origin);
        float dimDistance = activeDimAttributes.getCascadeSpacing();
        delta = delta.Normalize() * dimDistance;
        for (dimVertex& oldVertex : allVertexes) {
            Base::Vector3d nextPoint = DrawUtil::getTrianglePoint(pMaster, dirMaster, oldVertex.point);
            // nextPoint.y = -nextPoint.y;
            // oldVertex.point.y = -oldVertex.point.y;
            if ((oldVertex.point - nextPoint).Length() > 0.01) {
                Base::Vector3d cvPoint = CosmeticVertex::makeCanonicalPointInverted(objFeat, nextPoint);
                std::string vertTag = objFeat->addCosmeticVertex(cvPoint, false);
                int vertNumber = objFeat->add1CVToGV(vertTag);
                std::stringstream ss;
                ss << "Vertex" << vertNumber;
                dimVertex newVertex;
                newVertex.name = ss.str();
                newVertex.point = nextPoint;
                carrierVertexes.push_back(newVertex);
                Base::Vector3d oldCanon = CosmeticVertex::makeCanonicalPointInverted(objFeat, oldVertex.point);
                std::string edgeTag = objFeat->addCosmeticEdge(oldCanon, cvPoint);
                auto edge = objFeat->getCosmeticEdge(edgeTag);
                edge->m_format.setStyle(1);
                edge->m_format.setLineNumber(1);
                edge->m_format.setWidth(TechDraw::LineGroup::getDefaultWidth("Thin"));
                edge->m_format.setColor(Base::Color(0.0f, 0.0f, 0.0f));
            }
            else
                carrierVertexes.push_back(oldVertex);
        }
        std::sort(carrierVertexes.begin(), carrierVertexes.end(), sortX);
        double fontSize = Preferences::dimFontSizeMM();
        for (long unsigned int n = 0; n < allVertexes.size() - 1; n++) {
            TechDraw::DrawViewDimension* dim;
            dim = _createLinDimension(objFeat, carrierVertexes[n].name, carrierVertexes[n + 1].name, "Distance");
            TechDraw::pointPair pp = dim->getLinearPoints();
            Base::Vector3d mid = (pp.first() + pp.second()) / 2.0 + delta;
            dim->X.setValue(mid.x);
            dim->Y.setValue(-mid.y + 0.5 * fontSize);
            dims.push_back(dim);
        }
    }

    return dims;
}

DEF_STD_CMD_A(CmdTechDrawExtensionCreateObliqueChainDimension)

CmdTechDrawExtensionCreateObliqueChainDimension::CmdTechDrawExtensionCreateObliqueChainDimension()
    : Command("TechDraw_ExtensionCreateObliqueChainDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Oblique Chain Dimension");
    sToolTipText    = QT_TR_NOOP("Inserts a sequence of aligned oblique dimensions to at least "
            "three selected vertices, where the first two define the direction");
    sWhatsThis      = "TechDraw_ExtensionCreateObliqueChainDimension";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionCreateObliqueChainDimension";
}

void CmdTechDrawExtensionCreateObliqueChainDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCreateObliqueChainDimension(this);
}

bool CmdTechDrawExtensionCreateObliqueChainDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateChainDimensionGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawExtensionCreateChainDimensionGroup)

CmdTechDrawExtensionCreateChainDimensionGroup::CmdTechDrawExtensionCreateChainDimensionGroup()
    : Command("TechDraw_ExtensionCreateChainDimensionGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Horizontal Chain Dimension");
    sToolTipText    = QT_TR_NOOP("Inserts a sequence of aligned horizontal dimensions to at least "
            "three selected vertices, where the first two define the direction");
    sWhatsThis      = "TechDraw_ExtensionCreateChainDimensionGroup";
    sStatusTip      = sMenuText;
}

void CmdTechDrawExtensionCreateChainDimensionGroup::activated(int iMsg)
{
    //    Base::Console().message("CMD::ExtensionCascadeDimansionGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task in progress"),
            QObject::tr("Close active task dialog and try again"));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch (iMsg) {
    case 0:                 //create horizontal chain dimensions
        execCreateHorizChainDimension(this);
        break;
    case 1:                 //create vertical chain dimensions
        execCreateVertChainDimension(this);
        break;
    case 2:                 //create oblique chain dimensions
        execCreateObliqueChainDimension(this);
        break;
    default:
        Base::Console().message("CMD::CVGrp - invalid iMsg: %d\n", iMsg);
    };
}

Gui::Action* CmdTechDrawExtensionCreateChainDimensionGroup::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCreateHorizChainDimension"));
    p1->setObjectName(QStringLiteral("TechDraw_ExtensionCreateHorizChainDimension"));
    p1->setWhatsThis(QStringLiteral("TechDraw_ExtensionCreateHorizChainDimension"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCreateVertChainDimension"));
    p2->setObjectName(QStringLiteral("TechDraw_ExtensionCreateVertChainDimension"));
    p2->setWhatsThis(QStringLiteral("TechDraw_ExtensionCreateVertChainDimension"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCreateObliqueChainDimension"));
    p3->setObjectName(QStringLiteral("TechDraw_ExtensionCreateObliqueChainDimension"));
    p3->setWhatsThis(QStringLiteral("TechDraw_ExtensionCreateObliqueChainDimension"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawExtensionCreateChainDimensionGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdTechDrawExtensionCreateHorizChainDimension", "Horizontal Chain Dimension"));
    arc1->setToolTip(QApplication::translate("CmdTechDrawExtensionCreateHorizChainDimension",
"Inserts a sequence of aligned horizontal dimensions to at least three selected vertices, where the first two define the direction"));
    arc1->setStatusTip(arc1->text());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdTechDrawExtensionCreateVertChainDimension", "Vertical Chain Dimension"));
    arc2->setToolTip(QApplication::translate("CmdTechDrawExtensionCreateVertChainDimension",
"Inserts a sequence of aligned vertical dimensions to at least three selected vertices, where the first two define the direction"));
    arc2->setStatusTip(arc2->text());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("CmdTechDrawExtensionCreateObliqueChainDimension", "Oblique Chain Dimension"));
    arc3->setToolTip(QApplication::translate("CmdTechDrawExtensionCreateObliqueChainDimension",
"Inserts a sequence of aligned oblique dimensions to at least three selected vertices, where the first two define the direction"));
    arc3->setStatusTip(arc3->text());
}

bool CmdTechDrawExtensionCreateChainDimensionGroup::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateHorizCoordDimension
//===========================================================================

void execCreateHorizCoordDimension(Gui::Command* cmd) {
    //create horizontal coordinate dimensions
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSelObjAndSubs(cmd, selection, objFeat, QT_TRANSLATE_NOOP("QObject","TechDraw Create Horizontal Coordinate Dimension"))) {
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Horizontal Coord Dimension"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    std::vector<dimVertex> allVertexes;
    allVertexes = _getVertexInfo(objFeat, subNames);
    if (!allVertexes.empty() && allVertexes.size() > 1) {
        dimVertex firstVertex = allVertexes[0];
        dimVertex secondVertex = allVertexes[1];
        std::sort(allVertexes.begin(), allVertexes.end(), sortX);
        if (firstVertex.point.x > secondVertex.point.x) {
            std::reverse(allVertexes.begin(), allVertexes.end());
        }
        float dimDistance = activeDimAttributes.getCascadeSpacing();
        float yMaster = allVertexes[0].point.y - dimDistance;
        if (std::signbit(yMaster))
            dimDistance = -dimDistance;
        for (long unsigned int n = 0; n < allVertexes.size() - 1; n++) {
            TechDraw::DrawViewDimension* dim;
            dim = _createLinDimension(objFeat, allVertexes[0].name, allVertexes[n + 1].name, "DistanceX");
            TechDraw::pointPair pp = dim->getLinearPoints();
            Base::Vector3d mid = (pp.first() + pp.second()) / 2.0;
            dim->X.setValue(mid.x);
            dim->Y.setValue(-yMaster - dimDistance * n);
        }
    }
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
    cmd->getSelection().clearSelection();
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionCreateHorizCoordDimension)

CmdTechDrawExtensionCreateHorizCoordDimension::CmdTechDrawExtensionCreateHorizCoordDimension()
    : Command("TechDraw_ExtensionCreateHorizCoordDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Horizontal Coordinate Dimension");
    sToolTipText    = QT_TR_NOOP("Adds evenly spaced horizontal dimensions between 3 or more vertices aligned to a shared baseline");
    sWhatsThis      = "TechDraw_ExtensionCreateHorizCoordDimension";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionCreateHorizCoordDimension";
}

void CmdTechDrawExtensionCreateHorizCoordDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCreateHorizCoordDimension(this);
}

bool CmdTechDrawExtensionCreateHorizCoordDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateVertCoordDimension
//===========================================================================

void execCreateVertCoordDimension(Gui::Command* cmd) {
    //create vertical coordinate dimensions
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSelObjAndSubs(cmd, selection, objFeat, QT_TRANSLATE_NOOP("QObject","TechDraw Create Vertical Coord dimension"))) {
        return;
    }
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create vert coord dimension"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    std::vector<dimVertex> allVertexes;
    allVertexes = _getVertexInfo(objFeat, subNames);
    if (!allVertexes.empty() && allVertexes.size() > 1) {
        dimVertex firstVertex = allVertexes[0];
        dimVertex secondVertex = allVertexes[1];
        std::sort(allVertexes.begin(), allVertexes.end(), sortY);
        if (firstVertex.point.y > secondVertex.point.y) {
            std::reverse(allVertexes.begin(), allVertexes.end());
        }
        float dimDistance = activeDimAttributes.getCascadeSpacing();
        float xMaster = allVertexes[0].point.x + dimDistance;
        if (std::signbit(xMaster))
            dimDistance = -dimDistance;
        double fontSize = Preferences::dimFontSizeMM();
        for (long unsigned int n = 0; n < allVertexes.size() - 1; n++) {
            TechDraw::DrawViewDimension* dim;
            dim = _createLinDimension(objFeat, allVertexes[0].name, allVertexes[n + 1].name, "DistanceY");
            TechDraw::pointPair pp = dim->getLinearPoints();
            Base::Vector3d mid = (pp.first() + pp.second()) / 2.0;
            dim->X.setValue(xMaster + dimDistance * n);
            dim->Y.setValue(-mid.y + 0.5 * fontSize);
        }
    }
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
    cmd->getSelection().clearSelection();
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionCreateVertCoordDimension)

CmdTechDrawExtensionCreateVertCoordDimension::CmdTechDrawExtensionCreateVertCoordDimension()
    : Command("TechDraw_ExtensionCreateVertCoordDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Vertical Coordinate Dimension");
    sToolTipText    = QT_TR_NOOP("Adds evenly spaced vertical dimensions between 3 or more vertices aligned to a shared baseline");
    sWhatsThis      = "TechDraw_ExtensionCreateVertCoordDimension";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionCreateVertCoordDimension";
}

void CmdTechDrawExtensionCreateVertCoordDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCreateVertCoordDimension(this);
}

bool CmdTechDrawExtensionCreateVertCoordDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateObliqueCoordDimension
//===========================================================================

void execCreateObliqueCoordDimension(Gui::Command* cmd) {
    //create oblique coordinate dimensions
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat{nullptr};
    if (!_checkSelObjAndSubs(cmd, selection, objFeat, QT_TRANSLATE_NOOP("QObject","TechDraw Create Oblique Coord Dimension"))) {
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create oblique coord dimension"));

    std::vector<TechDraw::ReferenceEntry> refs;
    for (auto& subName : selection[0].getSubNames()) {
        refs.push_back(ReferenceEntry(objFeat, subName));
    }

    auto dims = makeObliqueCoordDimension(refs);
    if (dims.empty()) {
        Gui::Command::abortCommand();
    }
    else {
        objFeat->refreshCEGeoms();
        objFeat->requestPaint();
        cmd->getSelection().clearSelection();
        Gui::Command::commitCommand();
    }
}

std::vector<DrawViewDimension*> TechDrawGui::makeObliqueCoordDimension(std::vector<TechDraw::ReferenceEntry> refs)
{
    if (refs.empty()) {
        return {};
    }

    std::vector<std::string> subNames;
    auto* objFeat = static_cast<DrawViewPart*>(refs[0].getObject());
    for (auto& ref : refs) {
        subNames.push_back(ref.getSubName());
    }
    std::vector<DrawViewDimension*> dims;

    std::vector<dimVertex> allVertexes, carrierVertexes;
    allVertexes = _getVertexInfo(objFeat, subNames);
    if (!allVertexes.empty() && allVertexes.size() > 1) {
        Base::Vector3d pMaster = allVertexes[0].point;
        Base::Vector3d dirMaster = pMaster - allVertexes[1].point;
        Base::Vector3d origin(0.0, 0.0, 0.0);
        Base::Vector3d delta = DrawUtil::getTrianglePoint(pMaster, dirMaster, origin);
        float dimDistance = activeDimAttributes.getCascadeSpacing();
        delta = delta.Normalize() * dimDistance;
        for (dimVertex& oldVertex : allVertexes) {
            Base::Vector3d nextPoint = DrawUtil::getTrianglePoint(pMaster, dirMaster, oldVertex.point);
            if ((oldVertex.point - nextPoint).Length() > 0.01) {
                Base::Vector3d cvPoint = CosmeticVertex::makeCanonicalPointInverted(objFeat, nextPoint);
                std::string vertTag = objFeat->addCosmeticVertex(cvPoint, false);
                int vertNumber = objFeat->add1CVToGV(vertTag);
                std::stringstream ss;
                ss << "Vertex" << vertNumber;
                dimVertex newVertex;
                newVertex.name = ss.str();
                newVertex.point = nextPoint;
                carrierVertexes.push_back(newVertex);
                Base::Vector3d oldCanon = CosmeticVertex::makeCanonicalPointInverted(objFeat, oldVertex.point);
                std::string edgeTag = objFeat->addCosmeticEdge(oldCanon, cvPoint);
                auto edge = objFeat->getCosmeticEdge(edgeTag);
                edge->m_format.setStyle(1);
                edge->m_format.setLineNumber(1);
                edge->m_format.setWidth(TechDraw::LineGroup::getDefaultWidth("Thin"));
                edge->m_format.setColor(Base::Color(0.0, 0.0, 0.0));
            }
            else {
                carrierVertexes.push_back(oldVertex);
            }
        }
        dimVertex firstVertex = carrierVertexes[0];
        dimVertex secondVertex = carrierVertexes[1];
        std::sort(carrierVertexes.begin(), carrierVertexes.end(), sortX);
        if (firstVertex.point.x > secondVertex.point.x) {
            std::reverse(carrierVertexes.begin(), carrierVertexes.end());
        }
        double fontSize = Preferences::dimFontSizeMM();
        for (long unsigned int n = 0; n < allVertexes.size() - 1; n++) {
            TechDraw::DrawViewDimension* dim;
            dim = _createLinDimension(objFeat, carrierVertexes[0].name, carrierVertexes[n + 1].name, "Distance");
            TechDraw::pointPair pp = dim->getLinearPoints();
            Base::Vector3d mid = (pp.first() + pp.second()) / 2.0 + delta * (n + 1);
            dim->X.setValue(mid.x);
            dim->Y.setValue(-mid.y + 0.5 * fontSize);
            dims.push_back(dim);
        }
    }

    return dims;
}

DEF_STD_CMD_A(CmdTechDrawExtensionCreateObliqueCoordDimension)

CmdTechDrawExtensionCreateObliqueCoordDimension::CmdTechDrawExtensionCreateObliqueCoordDimension()
    : Command("TechDraw_ExtensionCreateObliqueCoordDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Oblique Coordinate Dimension");
    sToolTipText    = QT_TR_NOOP("Adds evenly spaced oblique dimensions between 3 or more vertices aligned to a shared baseline");
    sWhatsThis      = "TechDraw_ExtensionCreateObliqueCoordDimension";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionCreateObliqueCoordDimension";
}

void CmdTechDrawExtensionCreateObliqueCoordDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCreateObliqueCoordDimension(this);
}

bool CmdTechDrawExtensionCreateObliqueCoordDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateCoordDimensionGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawExtensionCreateCoordDimensionGroup)

CmdTechDrawExtensionCreateCoordDimensionGroup::CmdTechDrawExtensionCreateCoordDimensionGroup()
    : Command("TechDraw_ExtensionCreateCoordDimensionGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Horizontal Coordinate Dimension");
    sToolTipText    = QT_TR_NOOP("Adds evenly spaced horizontal dimensions between 3 or more vertices aligned to a shared baseline");
    sWhatsThis      = "TechDraw_ExtensionCreateCoordDimensionGroup";
    sStatusTip      = sMenuText;
}

void CmdTechDrawExtensionCreateCoordDimensionGroup::activated(int iMsg)
{
    //    Base::Console().message("CMD::ExtensionCascadeDimansionGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task in progress"),
            QObject::tr("Close active task dialog and try again"));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch (iMsg) {
    case 0:                 //create horizontal coordinate dimensions
        execCreateHorizCoordDimension(this);
        break;
    case 1:                 //create vertical coordinate dimensions
        execCreateVertCoordDimension(this);
        break;
    case 2:                 //create oblique coordinate dimensions
        execCreateObliqueCoordDimension(this);
        break;
    default:
        Base::Console().message("CMD::CVGrp - invalid iMsg: %d\n", iMsg);
    };
}

Gui::Action* CmdTechDrawExtensionCreateCoordDimensionGroup::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCreateHorizCoordDimension"));
    p1->setObjectName(QStringLiteral("TechDraw_ExtensionCreateHorizCoordDimension"));
    p1->setWhatsThis(QStringLiteral("TechDraw_ExtensionCreateHorizCoordDimension"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCreateVertCoordDimension"));
    p2->setObjectName(QStringLiteral("TechDraw_ExtensionCreateVertCoordDimension"));
    p2->setWhatsThis(QStringLiteral("TechDraw_ExtensionCreateVertCoordDimension"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCreateObliqueCoordDimension"));
    p3->setObjectName(QStringLiteral("TechDraw_ExtensionCreateObliqueCoordDimension"));
    p3->setWhatsThis(QStringLiteral("TechDraw_ExtensionCreateObliqueCoordDimension"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawExtensionCreateCoordDimensionGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdTechDrawExtensionCreateHorizCoordDimension", "Horizontal Coordinate Dimension"));
    arc1->setToolTip(QApplication::translate("CmdTechDrawExtensionCreateHorizCoordDimension",
"Adds evenly spaced horizontal dimensions between 3 or more vertices aligned to a shared baseline"));
    arc1->setStatusTip(arc1->text());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdTechDrawExtensionCreateVertCoordDimension", "Vertical Coordinate Dimension"));
    arc2->setToolTip(QApplication::translate("CmdTechDrawExtensionCreateVertCoordDimension",
"Adds evenly spaced vertical dimensions between 3 or more vertices aligned to a shared baseline"));
    arc2->setStatusTip(arc2->text());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("CmdTechDrawExtensionCreateObliqueCoordDimension", "Oblique Coordinate Dimension"));
    arc3->setToolTip(QApplication::translate("CmdTechDrawExtensionCreateObliqueCoordDimension",
"Adds evenly spaced oblique dimensions between 3 or more vertices aligned to a shared baseline"));
    arc3->setStatusTip(arc3->text());
}

bool CmdTechDrawExtensionCreateCoordDimensionGroup::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateHorizChamferDimension
//===========================================================================

void execCreateHorizChamferDimension(Gui::Command* cmd) {
    //create a horizontal chamfer dimension
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSelObjAndSubs(cmd, selection, objFeat, QT_TRANSLATE_NOOP("QObject","TechDraw Create Horizontal Chamfer Dimension"))) {
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Horizontal Chamfer Dimension"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    std::vector<dimVertex> allVertexes;
    allVertexes = _getVertexInfo(objFeat, subNames);
    if (!allVertexes.empty() && allVertexes.size() > 1) {
        TechDraw::DrawViewDimension* dim;
        dim = _createLinDimension(objFeat, allVertexes[0].name, allVertexes[1].name, "DistanceX");
        float yMax = std::max(abs(allVertexes[0].point.y), abs(allVertexes[1].point.y)) + 7.0;
        if (std::signbit(allVertexes[0].point.y))
            yMax = -yMax;
        TechDraw::pointPair pp = dim->getLinearPoints();
        Base::Vector3d mid = (pp.first() + pp.second()) / 2.0;
        dim->X.setValue(mid.x);
        dim->Y.setValue(-yMax);
        float dx = allVertexes[0].point.x - allVertexes[1].point.x;
        float dy = allVertexes[0].point.y - allVertexes[1].point.y;
        float alpha = std::round(Base::toDegrees(std::abs<float>(std::atan(dy / dx))));
        std::string sAlpha = std::to_string((int)alpha);
        std::string formatSpec = dim->FormatSpec.getStrValue();
        formatSpec = formatSpec + " x" + sAlpha + "°";
        dim->FormatSpec.setValue(formatSpec);
        objFeat->requestPaint();
        cmd->getSelection().clearSelection();
    }
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionCreateHorizChamferDimension)

CmdTechDrawExtensionCreateHorizChamferDimension::CmdTechDrawExtensionCreateHorizChamferDimension()
    : Command("TechDraw_ExtensionCreateHorizChamferDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Horizontal Chamfer Dimension");
    sToolTipText    = QT_TR_NOOP("Inserts a horizontal size and angle dimension for a chamfer from 2 selected vertices");
    sWhatsThis      = "TechDraw_ExtensionCreateHorizChamferDimension";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionCreateHorizChamferDimension";
}

void CmdTechDrawExtensionCreateHorizChamferDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCreateHorizChamferDimension(this);
}

bool CmdTechDrawExtensionCreateHorizChamferDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateVertChamferDimension
//===========================================================================

void execCreateVertChamferDimension(Gui::Command* cmd) {
    //create a vertical chamfer dimension
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSelObjAndSubs(cmd, selection, objFeat, QT_TRANSLATE_NOOP("QObject","TechDraw Create Vertical Chamfer Dimension"))) {
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Vert Chamfer Dimension"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    std::vector<dimVertex> allVertexes;
    allVertexes = _getVertexInfo(objFeat, subNames);
    if (!allVertexes.empty() && allVertexes.size() > 1) {
        TechDraw::DrawViewDimension* dim;
        dim = _createLinDimension(objFeat, allVertexes[0].name, allVertexes[1].name, "DistanceY");
        float xMax = std::max(abs(allVertexes[0].point.x), abs(allVertexes[1].point.x)) + 7.0;
        if (std::signbit(allVertexes[0].point.x))
            xMax = -xMax;
        TechDraw::pointPair pp = dim->getLinearPoints();
        Base::Vector3d mid = (pp.first() + pp.second()) / 2.0;
        dim->X.setValue(xMax);
        dim->Y.setValue(-mid.y);
        float dx = allVertexes[0].point.x - allVertexes[1].point.x;
        float dy = allVertexes[0].point.y - allVertexes[1].point.y;
        float alpha = std::round(Base::toDegrees(std::abs<float>(std::atan(dx / dy))));
        std::string sAlpha = std::to_string((int)alpha);
        std::string formatSpec = dim->FormatSpec.getStrValue();
        formatSpec = formatSpec + " x" + sAlpha + "°";
        dim->FormatSpec.setValue(formatSpec);
        objFeat->requestPaint();
        cmd->getSelection().clearSelection();
    }
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionCreateVertChamferDimension)

CmdTechDrawExtensionCreateVertChamferDimension::CmdTechDrawExtensionCreateVertChamferDimension()
    : Command("TechDraw_ExtensionCreateVertChamferDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Vertical Chamfer Dimension");
    sToolTipText    = QT_TR_NOOP("Inserts a vertical size and angle dimension for a chamfer from 2 selected vertices");
    sWhatsThis      = "TechDraw_ExtensionCreateVertChamferDimension";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionCreateVertChamferDimension";
}

void CmdTechDrawExtensionCreateVertChamferDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCreateVertChamferDimension(this);
}

bool CmdTechDrawExtensionCreateVertChamferDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionChamferDimensionGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawExtensionChamferDimensionGroup)

CmdTechDrawExtensionChamferDimensionGroup::CmdTechDrawExtensionChamferDimensionGroup()
    : Command("TechDraw_ExtensionChamferDimensionGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Horizontal Chamfer Dimension");
    sToolTipText    = QT_TR_NOOP("Inserts a horizontal size and angle dimension for a chamfer from 2 selected vertices");
    sWhatsThis      = "TechDraw_ExtensionChamferDimensionGroup";
    sStatusTip      = sMenuText;
}

void CmdTechDrawExtensionChamferDimensionGroup::activated(int iMsg)
{
    //    Base::Console().message("CMD::ExtensionIncreaseDecreaseGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task in progress"),
            QObject::tr("Close active task dialog and try again"));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch (iMsg) {
    case 0:                 //create a horizontal chamfer dimension
        execCreateHorizChamferDimension(this);
        break;
    case 1:                 //create a vertical chamfer dimension
        execCreateVertChamferDimension(this);
        break;
    default:
        Base::Console().message("CMD::CVGrp - invalid iMsg: %d\n", iMsg);
    };
}

Gui::Action* CmdTechDrawExtensionChamferDimensionGroup::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCreateHorizChamferDimension"));
    p1->setObjectName(QStringLiteral("TechDraw_ExtensionCreateHorizChamferDimension"));
    p1->setWhatsThis(QStringLiteral("TechDraw_ExtensionCreateHorizChamferDimension"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCreateVertChamferDimension"));
    p2->setObjectName(QStringLiteral("TechDraw_ExtensionCreateVertChamferDimension"));
    p2->setWhatsThis(QStringLiteral("TechDraw_ExtensionCreateVertChamferDimension"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawExtensionChamferDimensionGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdTechDrawExtensionCreateHorizChamferDimension", "Horizontal Chamfer Dimension"));
    arc1->setToolTip(QApplication::translate("CmdTechDrawExtensionCreateHorizChamferDimension",
"Inserts a horizontal size and angle dimension for a chamfer from 2 selected vertices"));
    arc1->setStatusTip(arc1->text());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdTechDrawExtensionCreateVertChamferDimension", "Vertical Chamfer Dimension"));
    arc2->setToolTip(QApplication::translate("CmdTechDrawExtensionCreateVertChamferDimension",
"Inserts a vertical size and angle dimension for a chamfer from 2 selected vertices"));
    arc2->setStatusTip(arc2->text());
}

bool CmdTechDrawExtensionChamferDimensionGroup::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateLengthArc
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionCreateLengthArc)

CmdTechDrawExtensionCreateLengthArc::CmdTechDrawExtensionCreateLengthArc()
    : Command("TechDraw_ExtensionCreateLengthArc")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Arc Length Dimension");
    sToolTipText    = QT_TR_NOOP("Inserts an arc length dimension to the selected arc");
    sWhatsThis      = "TechDraw_ExtensionCreateLengthArc";
    sStatusTip      = sMenuText;
    sPixmap         = "TechDraw_ExtensionCreateLengthArc";
}

void CmdTechDrawExtensionCreateLengthArc::activated(int iMsg) {
    // create arc length dimension
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSelObjAndSubs(this, selection, objFeat, QT_TRANSLATE_NOOP("QObject","TechDraw Create Arc Length Dimension"))) {
        return;
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Arc Length Dimension"));
    ReferenceEntry ref(objFeat, selection[0].getSubNames()[0]);

    TechDraw::DrawViewDimension* dim = makeArcLengthDimension(ref);

    if (dim) {
        objFeat->refreshCEGeoms();
        objFeat->requestPaint();
        Gui::Command::commitCommand();
    }
    else {
        Gui::Command::abortCommand();
    }
}

bool CmdTechDrawExtensionCreateLengthArc::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCustomizeFormat
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionCustomizeFormat)

CmdTechDrawExtensionCustomizeFormat::CmdTechDrawExtensionCustomizeFormat()
  : Command("TechDraw_ExtensionCustomizeFormat")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Customize Format Label");
    sToolTipText    = QT_TR_NOOP("Customizes the format label of a selected dimension or balloon");
    sWhatsThis      = "TechDraw_ExtensionCustomizeFormat";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionCustomizeFormat";
}

void CmdTechDrawExtensionCustomizeFormat::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> selected;
    if (!_checkSelection(this, selected, QT_TRANSLATE_NOOP("QObject","TechDraw Customize Format")))
        return;
    auto object = selected[0].getObject();
    if (object->isDerivedFrom<TechDraw::DrawViewDimension>() ||
        object->isDerivedFrom<TechDraw::DrawViewBalloon>())
        Gui::Control().showDialog(new TaskDlgCustomizeFormat(object));
}

bool CmdTechDrawExtensionCustomizeFormat::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

DrawViewDimension* TechDrawGui::makeArcLengthDimension(const ReferenceEntry& ref)
{
    DrawViewDimension* dim = nullptr;
    auto* dvp = static_cast<DrawViewPart*>(ref.getObject());

    int geoId = DrawUtil::getIndexFromName(ref.getSubName());
    BaseGeomPtr geom = dvp->getGeomByIndex(geoId);

    // Find the edge length.
    TechDraw::BaseGeomPtr edge = dvp->getEdge(ref.getSubName());
    if (!edge) {
        return nullptr;
    }
    GProp_GProps edgeProps;
    BRepGProp::LinearProperties(edge->getOCCEdge(), edgeProps);
    double length = edgeProps.Mass() / dvp->getScale();

    Base::Vector3d startPt = edge->getStartPoint();
    Base::Vector3d endPt = edge->getEndPoint();
    startPt.y = -startPt.y;
    endPt.y = -endPt.y;

    std::stringstream startName, endName, formatSpec;
    Base::Vector3d cvPoint = CosmeticVertex::makeCanonicalPoint(dvp, startPt);
    std::string startVertTag = dvp->addCosmeticVertex(cvPoint);
    int startVertNumber = dvp->add1CVToGV(startVertTag);
    startName << "Vertex" << startVertNumber;
    cvPoint = CosmeticVertex::makeCanonicalPoint(dvp, endPt);
    std::string endVertTag = dvp->addCosmeticVertex(cvPoint);
    int endVertNumber = dvp->add1CVToGV(endVertTag);
    endName << "Vertex" << endVertNumber;

    dim = _createLinDimension(dvp, startName.str(), endName.str(), "Distance");
    TechDraw::pointPair pp = dim->getLinearPoints();
    Base::Vector3d mid = (pp.first() + pp.second()) / 2.0;
    dim->X.setValue(mid.x);
    dim->Y.setValue(-mid.y);

    dim->Arbitrary.setValue(true);
    formatSpec << "◠ " << length;
    dim->FormatSpec.setValue(formatSpec.str());

    return dim;
}

namespace TechDrawGui {
    //===========================================================================
    // internal helper routines
    //===========================================================================

    bool _checkSelection(Gui::Command* cmd,
        std::vector<Gui::SelectionObject>& selection,
        std::string message) {
        // check selection of getSelectionEx()
        selection = cmd->getSelection().getSelectionEx();
        if (selection.empty()) {
            QMessageBox::warning(Gui::getMainWindow(),
                QObject::tr(message.c_str()),
                QObject::tr("Selection is empty"));
            return false;
        }
        return true;
    }

    bool _checkSelAndObj(Gui::Command* cmd,
        std::vector<Gui::SelectionObject>& selection,
        TechDraw::DrawViewPart*& objFeat,
        std::string message) {
        // check selection of getSelectionEx() and selection[0].getObject()
        if (_checkSelection(cmd, selection, message)) {
            objFeat = dynamic_cast<TechDraw::DrawViewPart*>(selection[0].getObject());
            if (!objFeat) {
                QMessageBox::warning(Gui::getMainWindow(),
                    QObject::tr(message.c_str()),
                    QObject::tr("No object selected"));
                return false;
            }
        } else {
            // nothing selected
            return false;
        }
        return true;
    }

    bool _checkSelObjAndSubs(Gui::Command* cmd,
        std::vector<Gui::SelectionObject>& selection,
        TechDraw::DrawViewPart*& objFeat,
        std::string message) {
        // check selection of getSelectionEx() and selection[0].getObject()
        if (_checkSelAndObj(cmd, selection, objFeat, message)) {
            auto subs = selection[0].getSubNames();
            if (subs.empty()) {
                QMessageBox::warning(Gui::getMainWindow(),
                    QObject::tr(message.c_str()),
                    QObject::tr("No sub-elements selected"));
                return false;
            }
        } else {
            // nothing selected
            return false;
        }
        return true;
    }

    TechDraw::DrawViewDimension* _createLinDimension(
        TechDraw::DrawViewPart* objFeat,
        std::string startVertex,
        std::string endVertex,
        std::string dimType)
        // create a new linear dimension
    {
        TechDraw::DrawPage* page = objFeat->findParentPage();
        std::string PageName = page->getNameInDocument();
        std::string FeatName = objFeat->getDocument()->getUniqueObjectName("Dimension");
        std::vector<App::DocumentObject*> objs;
        std::vector<std::string> subs;
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(startVertex);
        subs.push_back(endVertex);
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('TechDraw::DrawViewDimension', '%s')", FeatName.c_str());
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Type = '%s'", FeatName.c_str(), dimType.c_str());
        auto dim = dynamic_cast<TechDraw::DrawViewDimension*>(objFeat->getDocument()->getObject(FeatName.c_str()));
        if (!dim){
            throw Base::TypeError("CmdTechDrawExtensionCreateLinDimension - dim not found\n");
        }
        dim->References2D.setValues(objs, subs);
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", PageName.c_str(), FeatName.c_str());

        // Touch the parent feature so the dimension in tree view appears as a child
        objFeat->touch();
        dim->recomputeFeature();
        return dim;
    }

    std::vector<dimVertex> _getVertexInfo(TechDraw::DrawViewPart* objFeat,
        std::vector<std::string> subNames) {
        // get subNames and coordinates of all selected vertexes
        std::vector<dimVertex> vertexes;
        dimVertex nextVertex;
        for (const std::string& name : subNames) {
            std::string geoType = TechDraw::DrawUtil::getGeomTypeFromName(name);
            if (geoType == "Vertex") {
                int geoId = TechDraw::DrawUtil::getIndexFromName(name);
                TechDraw::VertexPtr vert = objFeat->getProjVertexByIndex(geoId);
                nextVertex.name = name;
                nextVertex.point.x = vert->point().x;
                nextVertex.point.y = vert->point().y;
                nextVertex.point.z = 0.0;
                vertexes.push_back(nextVertex);
            }
        }
        return vertexes;
    }

    std::vector<TechDraw::DrawViewDimension*>_getDimensions(std::vector<Gui::SelectionObject> selection, std::string needDimType) {
        // get all selected dimensions of type needDimType
        std::vector<TechDraw::DrawViewDimension*> validDimension;
        for (auto selected : selection) {
            auto object = selected.getObject();
            if (object->isDerivedFrom<TechDraw::DrawViewDimension>()) {
                auto dim = static_cast<TechDraw::DrawViewDimension*>(selected.getObject());
                std::string dimType = dim->Type.getValueAsString();
                if (dimType == needDimType)
                    validDimension.push_back(dim);
            }
        }
        return validDimension;
    }
}

//------------------------------------------------------------------------------
void CreateTechDrawCommandsExtensionDims()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawExtensionInsertPrefixGroup());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionInsertDiameter());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionInsertSquare());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionInsertRepetition());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionRemovePrefixChar());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionIncreaseDecreaseGroup());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionIncreaseDecimal());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionDecreaseDecimal());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionPosChainDimensionGroup());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionPosHorizChainDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionPosVertChainDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionPosObliqueChainDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCascadeDimensionGroup());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCascadeHorizDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCascadeVertDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCascadeObliqueDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCreateChainDimensionGroup());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCreateHorizChainDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCreateVertChainDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCreateObliqueChainDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCreateCoordDimensionGroup());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCreateHorizCoordDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCreateVertCoordDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCreateObliqueCoordDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionChamferDimensionGroup());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCreateHorizChamferDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCreateVertChamferDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCreateLengthArc());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCustomizeFormat());
}
