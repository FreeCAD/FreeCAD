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
# include <QMessageBox>
# include <iostream>
# include <string>
# include <sstream>
# include <cstdlib>
# include <exception>
#endif  //#ifndef _PreComp_

#include <QGraphicsView>

# include <App/DocumentObject.h>
# include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Type.h>
# include <Gui/Action.h>
# include <Gui/Application.h>
# include <Gui/BitmapFactory.h>
# include <Gui/Command.h>
# include <Gui/Control.h>
# include <Gui/Document.h>
# include <Gui/Selection.h>
# include <Gui/MainWindow.h>
# include <Gui/FileDialog.h>
# include <Gui/ViewProvider.h>

# include <Mod/Part/App/PartFeature.h>

# include <Mod/TechDraw/App/DrawViewPart.h>
# include <Mod/TechDraw/App/DrawProjGroupItem.h>
# include <Mod/TechDraw/App/DrawProjGroup.h>
# include <Mod/TechDraw/App/DrawViewDimension.h>
# include <Mod/TechDraw/App/DrawDimHelper.h>
# include <Mod/TechDraw/App/LandmarkDimension.h>
# include <Mod/TechDraw/App/DrawPage.h>
# include <Mod/TechDraw/App/DrawUtil.h>
# include <Mod/TechDraw/App/Geometry.h>

#include <Mod/TechDraw/Gui/QGVPage.h> //needed ?


#include "DrawGuiUtil.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include "TaskLinkDim.h"

#include "TaskSelectLineAttributes.h"

/////////////////////////////
#include <Mod/TechDraw/App/DrawViewSection.h>  // needed
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
/////////////////////////////

using namespace TechDrawGui;
using namespace TechDraw;
using namespace std;

//internal structures and sort functions
struct dimVertex{
    // save a dimension defining vertex and its point
    std::string name;
    Base::Vector3d point;
};

struct {
    bool operator()(dimVertex a, dimVertex b) const {return a.point.x < b.point.x;}
} sortX;

struct {
    bool operator()(dimVertex a, dimVertex b) const {return a.point.y < b.point.y;}
} sortY;

//internal helper functions
void _selectDimensionAttributes(Gui::Command* cmd);
std::vector<TechDraw::DrawViewDimension*>_getDimensions(std::vector<Gui::SelectionObject> selection,std::string needDimType);
Base::Vector3d _getTrianglePoint(Base::Vector3d p1, Base::Vector3d d, Base::Vector3d p2);
std::vector<dimVertex> _getVertexInfo(TechDraw::DrawViewPart* objFeat,
                                      std::vector<std::string> subNames);
TechDraw::DrawViewDimension* _createLinDimension(Gui::Command* cmd,
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

//===========================================================================
// TechDraw_ExtensionInsertDiameter
//===========================================================================

void execInsertPraefixChar(Gui::Command* cmd,std::string praefixChar){
    // insert a praefix character into the format specifier
    std::vector<Gui::SelectionObject> selection;
    if (_checkSelection(cmd,selection,"TechDraw Insert Praefix Character")){
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Insert Praefix Character"));
        for (auto selected : selection){
            auto object = selected.getObject();
            if (object->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId())){
                auto dim = dynamic_cast<TechDraw::DrawViewDimension*>(selected.getObject());
                std::string formatSpec = dim->FormatSpec.getStrValue();
                formatSpec = praefixChar+formatSpec;
                dim->FormatSpec.setValue(formatSpec);
            }
        }
        Gui::Command::commitCommand();
    }
}

DEF_STD_CMD_A(CmdTechDrawExtensionInsertDiameter)

CmdTechDrawExtensionInsertDiameter::CmdTechDrawExtensionInsertDiameter()
  : Command("TechDraw_ExtensionInsertDiameter")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert '⌀' sign");
    sToolTipText    = QT_TR_NOOP("Insert '⌀' sign in a measure\n\
    - select many measures\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionInsertDiameter";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionInsertDiameter";
}

void CmdTechDrawExtensionInsertDiameter::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execInsertPraefixChar(this,"⌀");
}

bool CmdTechDrawExtensionInsertDiameter::isActive(void)
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
    sMenuText       = QT_TR_NOOP("Insert '〼' sign");
    sToolTipText    = QT_TR_NOOP("Insert '〼' sign in a measure\n\
    - select many measures\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionInsertSquare";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionInsertSquare";
}

void CmdTechDrawExtensionInsertSquare::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execInsertPraefixChar(this,"〼");
}

bool CmdTechDrawExtensionInsertSquare::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionInsertPraefixGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawExtensionInsertPraefixGroup)

CmdTechDrawExtensionInsertPraefixGroup::CmdTechDrawExtensionInsertPraefixGroup()
  : Command("TechDraw_ExtensionInsertPraefixGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert a '⌀' sign in a measure");
    sToolTipText    = QT_TR_NOOP("Insert a '⌀' sign in a measure:\n\
    - select many measures\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionInsertPraefixGroup";
    sStatusTip      = sToolTipText;
}

void CmdTechDrawExtensionInsertPraefixGroup::activated(int iMsg)
{
//    Base::Console().Message("CMD::ExtensionLinePPGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch(iMsg) {
        case 0:                 //insert "⌀" as praefix
            execInsertPraefixChar(this,"⌀");
            break;
        case 1:                 //insert "〼" as praefix
            execInsertPraefixChar(this,"〼");
            break;
        default:
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n",iMsg);
    };
}

Gui::Action * CmdTechDrawExtensionInsertPraefixGroup::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionInsertDiameter"));
    p1->setObjectName(QString::fromLatin1("TechDraw_ExtensionInsertDiameter"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionInsertDiameter"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionInsertSquare"));
    p2->setObjectName(QString::fromLatin1("TechDraw_ExtensionInsertSquare"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionInsertSquare"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawExtensionInsertPraefixGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("TechDraw_Extension","Insert '⌀' sign in a measure"));
    arc1->setToolTip(QApplication::translate("TechDraw_Extension","Insert a '⌀' sign in a measure:\n\
    - select many measures\n\
    - click this button"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("TechDraw_Extension","Insert a '〼' sign in a measure"));
    arc2->setToolTip(QApplication::translate("TechDraw_Extension","Insert a '〼' sign in a measure:\n\
    - select many measures\n\
    - click this button"));
    arc2->setStatusTip(arc2->toolTip());
}

bool CmdTechDrawExtensionInsertPraefixGroup::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionIncreaseDecimal
//===========================================================================

void execIncreaseDecreaseDecimal(Gui::Command* cmd, int delta){
    // increase or decrease number of decimal places of a measure
    std::vector<Gui::SelectionObject> selection;
    if (_checkSelection(cmd,selection,"TechDraw Increase/Decrease Decimal")){
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Increase/Decrease Decimal"));
        std::string numStr;
        for (auto selected : selection){
            auto object = selected.getObject();
            if (object->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId())){
                auto dim = dynamic_cast<TechDraw::DrawViewDimension*>(selected.getObject());
                std::string formatSpec = dim->FormatSpec.getStrValue();
                std::string searchStr ("%.");
                int numFound = formatSpec.find(searchStr)+2;
                numStr = formatSpec[numFound];
                int numInt = std::stoi(numStr,nullptr);
                numInt = numInt+delta;
                if (numInt >= 0 && numInt <= 9){
                    numStr = std::to_string(numInt);
                    formatSpec.replace(numFound,1,numStr);
                    dim->FormatSpec.setValue(formatSpec);
                }
            }
        }
        Gui::Command::commitCommand();
    }
}

DEF_STD_CMD_A(CmdTechDrawExtensionIncreaseDecimal)

CmdTechDrawExtensionIncreaseDecimal::CmdTechDrawExtensionIncreaseDecimal()
  : Command("TechDraw_ExtensionIncreaseDecimal")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Increase Decimal Places");
    sToolTipText    = QT_TR_NOOP("Increase Decimal Places of a Measure\n\
    - select many measures\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionIncreaseDecimal";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionIncreaseDecimal";
}

void CmdTechDrawExtensionIncreaseDecimal::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execIncreaseDecreaseDecimal(this,1);
}

bool CmdTechDrawExtensionIncreaseDecimal::isActive(void)
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
    sToolTipText    = QT_TR_NOOP("Decrease Decimal Places of a Measure\n\
    - select many measures\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionDecreaseDecimal";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionDecreaseDecimal";
}

void CmdTechDrawExtensionDecreaseDecimal::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execIncreaseDecreaseDecimal(this,-1);
}

bool CmdTechDrawExtensionDecreaseDecimal::isActive(void)
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
    sToolTipText    = QT_TR_NOOP("Increase Decimal Places of a Measure\n\
    - select many measures\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionIncreaseDecreaseGroup";
    sStatusTip      = sToolTipText;
}

void CmdTechDrawExtensionIncreaseDecreaseGroup::activated(int iMsg)
{
//    Base::Console().Message("CMD::ExtensionIncreaseDecreaseGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch(iMsg) {
        case 0:                 //increase decimal places
            execIncreaseDecreaseDecimal(this,1);
            break;
        case 1:                 //decrease decimal places
            execIncreaseDecreaseDecimal(this,-1);
            break;
        default:
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n",iMsg);
    };
}

Gui::Action * CmdTechDrawExtensionIncreaseDecreaseGroup::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionIncreaseDecimal"));
    p1->setObjectName(QString::fromLatin1("TechDraw_ExtensionIncreaseDecimal"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionIncreaseDecimal"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionDecreaseDecimal"));
    p2->setObjectName(QString::fromLatin1("TechDraw_ExtensionDecreaseDecimal"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionDecreaseDecimal"));

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
    arc1->setText(QApplication::translate("TechDraw_Extension","Increase Decimal Places"));
    arc1->setToolTip(QApplication::translate("TechDraw_Extension","Increase Decimal Places of a Measure\n\
    - select many measures\n\
    - click this button"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("TechDraw_Extension","Decrease Decimal Places"));
    arc2->setToolTip(QApplication::translate("TechDraw_Extension","Decrease Decimal Places of a Measure\n\
    - select many measures\n\
    - click this button"));
    arc2->setStatusTip(arc2->toolTip());
}

bool CmdTechDrawExtensionIncreaseDecreaseGroup::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionPosHorizChainDimension
//===========================================================================

void execPosHorizChainDimension(Gui::Command* cmd){
    // position a horizontal dimension chain
    std::vector<Gui::SelectionObject> selection;
    if (_checkSelection(cmd,selection,"TechDraw PosHorizChainDimension")){
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Pos Horiz Chain Dim"));
        std::vector<TechDraw::DrawViewDimension*> validDimension;
        validDimension = _getDimensions(selection,"DistanceX");
        if( validDimension.empty() ) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("TechDraw PosHorizChainDimension"),
                                 QObject::tr("No horizontal dimensions selected"));
            return;
        }
        float yMaster = validDimension[0]->Y.getValue();
        for (auto dim : validDimension){
            dim->Y.setValue(yMaster);
            pointPair pp = dim->getLinearPoints();
            Base::Vector3d p1 = pp.first;
            Base::Vector3d p2 = pp.second;
            dim->X.setValue((p1.x+p2.x)/2.0);
        }
        Gui::Command::commitCommand();
    }
}

DEF_STD_CMD_A(CmdTechDrawExtensionPosHorizChainDimension)

CmdTechDrawExtensionPosHorizChainDimension::CmdTechDrawExtensionPosHorizChainDimension()
  : Command("TechDraw_ExtensionPosHorizChainDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Position Horizontal Chain Dimension");
    sToolTipText    = QT_TR_NOOP("Arranges the selected horizontal dimensions on the same line:\n\
    - select two or more measures\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionPosHorizChainDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionPosHorizChainDimension";
}

void CmdTechDrawExtensionPosHorizChainDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execPosHorizChainDimension(this);
}

bool CmdTechDrawExtensionPosHorizChainDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionPosVertChainDimension
//===========================================================================

void execPosVertChainDimension(Gui::Command* cmd){
    // position a vertical dimension chain
    std::vector<Gui::SelectionObject> selection;
    if (_checkSelection(cmd,selection,"TechDraw PosVertChainDimension")){
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Pos Vert Chain Dim"));
        std::vector<TechDraw::DrawViewDimension*> validDimension;
        validDimension = _getDimensions(selection,"DistanceY");
        if( validDimension.empty() ) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("TechDraw PosVertChainDimension"),
                                 QObject::tr("No vertical dimensions selected"));
            return;
        }  
        float xMaster = validDimension[0]->X.getValue();
        for (auto dim : validDimension){
            dim->X.setValue(xMaster);
            pointPair pp = dim->getLinearPoints();
            Base::Vector3d p1 = pp.first;
            Base::Vector3d p2 = pp.second;
            dim->Y.setValue((p1.y+p2.y)/-2.0);
        }
        Gui::Command::commitCommand();
    }
}

DEF_STD_CMD_A(CmdTechDrawExtensionPosVertChainDimension)

CmdTechDrawExtensionPosVertChainDimension::CmdTechDrawExtensionPosVertChainDimension()
  : Command("TechDraw_ExtensionPosVertChainDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Position Vertical Chain Dimension");
    sToolTipText    = QT_TR_NOOP("Arranges the selected vertical dimensions on the same line:\n\
    - select two or more measures\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionPosVertChainDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionPosVertChainDimension";
}

void CmdTechDrawExtensionPosVertChainDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execPosVertChainDimension(this);
}

bool CmdTechDrawExtensionPosVertChainDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionPosObliqueChainDimension
//===========================================================================

void execPosObliqueChainDimension(Gui::Command* cmd){
    // position an oblique dimension chain
    std::vector<Gui::SelectionObject> selection;
    if (_checkSelection(cmd,selection,"TechDraw PosObliqueChainDimension")){
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Pos Oblique Chain Dim"));
        std::vector<TechDraw::DrawViewDimension*> validDimension;
        validDimension = _getDimensions(selection,"Distance");
        if( validDimension.empty() ) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("TechDraw PosObliqueChainDimension"),
                                 QObject::tr("No oblique dimensions selected"));
            return;
        }
        float xMaster = validDimension[0]->X.getValue();
        float yMaster = validDimension[0]->Y.getValue();
        Base::Vector3d pMaster(xMaster,yMaster,0.0);
        pointPair pp = validDimension[0]->getLinearPoints();
        Base::Vector3d dirMaster = pp.second-pp.first;
        dirMaster.y = -dirMaster.y;
        for (auto dim : validDimension){
            float xDim = dim->X.getValue();
            float yDim = dim->Y.getValue();
            Base::Vector3d pDim(xDim,yDim,0.0);
            Base::Vector3d p3 = _getTrianglePoint(pMaster,dirMaster,pDim);
            dim->X.setValue(p3.x);
            dim->Y.setValue(p3.y);
        }
        Gui::Command::commitCommand();
    }
}

DEF_STD_CMD_A(CmdTechDrawExtensionPosObliqueChainDimension)

CmdTechDrawExtensionPosObliqueChainDimension::CmdTechDrawExtensionPosObliqueChainDimension()
  : Command("TechDraw_ExtensionPosObliqueChainDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Position Oblique Chain Dimension");
    sToolTipText    = QT_TR_NOOP("Arranges the selected oblique dimensions on the same line:\n\
    - select two or more measures\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionPosObliqueChainDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionPosObliqueChainDimension";
}

void CmdTechDrawExtensionPosObliqueChainDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execPosObliqueChainDimension(this);
    ///Base::Console().Message("TechDraw_ExtensionPosObliqueChainDimension started\n");
}

bool CmdTechDrawExtensionPosObliqueChainDimension::isActive(void)
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
    sMenuText       = QT_TR_NOOP("Position Horizontal Chain Dimension");
    sToolTipText    = QT_TR_NOOP("Arranges the selected horizontal dimensions on the same line:\n\
    - select two or more measures\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionPosChainDimensionGroup";
    sStatusTip      = sToolTipText;
}

void CmdTechDrawExtensionPosChainDimensionGroup::activated(int iMsg)
{
//    Base::Console().Message("CMD::ExtensionPosChainDimensionGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch(iMsg) {
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
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n",iMsg);
    };
}

Gui::Action * CmdTechDrawExtensionPosChainDimensionGroup::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionPosHorizChainDimension"));
    p1->setObjectName(QString::fromLatin1("TechDraw_ExtensionPosHorizChainDimension"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionPosHorizChainDimension"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionPosVertChainDimension"));
    p2->setObjectName(QString::fromLatin1("TechDraw_ExtensionPosVertChainDimension"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionPosVertChainDimension"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionPosObliqueChainDimension"));
    p3->setObjectName(QString::fromLatin1("TechDraw_ExtensionPosObliqueChainDimension"));
    p3->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionPosObliqueChainDimension"));

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
    arc1->setText(QApplication::translate("TechDraw_Extension","Position Horizontal Chain Dimensions"));
    arc1->setToolTip(QApplication::translate("TechDraw_Extension",
    "Arranges the selected horizontal dimensions on the same line:\n\
    - select two or more measures\n\
    - click this button"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("TechDraw_Extension","Position Vertical Chain Dimensions"));
    arc2->setToolTip(QApplication::translate("TechDraw_Extension",
    "Arranges the selected vertical dimensions on the same line:\n\
    - select two or more measures\n\
    - click this button"));
    arc2->setStatusTip(arc2->toolTip());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("TechDraw_Extension","Position Oblique Chain Dimensions"));
    arc3->setToolTip(QApplication::translate("TechDraw_Extension",
    "Arranges the selected oblique dimensions on the same line:\n\
    - select two or more measures\n\
    - click this button"));
    arc3->setStatusTip(arc3->toolTip());
}

bool CmdTechDrawExtensionPosChainDimensionGroup::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCascadeHorizDimension
//===========================================================================

void execCascadeHorizDimension(Gui::Command* cmd){
    // cascade horizontal dimensions
    std::vector<Gui::SelectionObject> selection;
    if (_checkSelection(cmd,selection,"TechDraw CascadeHorizDimension")){
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cascade Horiz Dim"));
        std::vector<TechDraw::DrawViewDimension*> validDimension;
        validDimension = _getDimensions(selection,"DistanceX");
        if( validDimension.empty() ) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("TechDraw CascadeHorizDimension"),
                                 QObject::tr("No horizontal dimensions selected"));
            return;
        }
        float yMaster = validDimension[0]->Y.getValue();
        float dimDistance = activeDimAttributes.getCascadeSpacing();
        if (signbit(yMaster))
            dimDistance = -dimDistance;
        for (auto dim : validDimension){
            dim->Y.setValue(yMaster);
            pointPair pp = dim->getLinearPoints();
            Base::Vector3d p1 = pp.first;
            Base::Vector3d p2 = pp.second;
            dim->X.setValue((p1.x+p2.x)/2.0);
            yMaster = yMaster+dimDistance;
        }
        Gui::Command::commitCommand();
    }
}

DEF_STD_CMD_A(CmdTechDrawExtensionCascadeHorizDimension)

CmdTechDrawExtensionCascadeHorizDimension::CmdTechDrawExtensionCascadeHorizDimension()
  : Command("TechDraw_ExtensionCascadeHorizDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cascade Horizontal Dimensions");
    sToolTipText    = QT_TR_NOOP("Spaces (vertically), the selected horizontal dimensions:\n\
    - select two or more horizontal dimensions\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCascadeHorizDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionCascadeHorizDimension";
}

void CmdTechDrawExtensionCascadeHorizDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCascadeHorizDimension(this);
}

bool CmdTechDrawExtensionCascadeHorizDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCascadeVertDimension
//===========================================================================

void execCascadeVertDimension(Gui::Command* cmd){
    // cascade horizontal dimensions
    std::vector<Gui::SelectionObject> selection;
    if (_checkSelection(cmd,selection,"TechDraw CascadeVertDimension")){
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cascade Vert Dim"));
        std::vector<TechDraw::DrawViewDimension*> validDimension;
        validDimension = _getDimensions(selection,"DistanceY");
        if( validDimension.empty() ) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("TechDraw CascadeVertDimension"),
                                 QObject::tr("No vertical dimensions selected"));
            return;
        }
        float xMaster = validDimension[0]->X.getValue();
        float dimDistance = activeDimAttributes.getCascadeSpacing();
        if (signbit(xMaster))
            dimDistance = -dimDistance;
        for (auto dim : validDimension){
            dim->X.setValue(xMaster);
            pointPair pp = dim->getLinearPoints();
            Base::Vector3d p1 = pp.first;
            Base::Vector3d p2 = pp.second;
            dim->Y.setValue((p1.y+p2.y)/-2.0);
            xMaster = xMaster+dimDistance;
        }
        Gui::Command::commitCommand();
    }
}

DEF_STD_CMD_A(CmdTechDrawExtensionCascadeVertDimension)

CmdTechDrawExtensionCascadeVertDimension::CmdTechDrawExtensionCascadeVertDimension()
  : Command("TechDraw_ExtensionCascadeVertDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cascade Vertical Dimensions");
    sToolTipText    = QT_TR_NOOP("Spaces (horizontally), the selected vertical dimensions:\n\
    - select two or more vertical dimensions\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCascadeVertDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionCascadeVertDimension";
}

void CmdTechDrawExtensionCascadeVertDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCascadeVertDimension(this);
}

bool CmdTechDrawExtensionCascadeVertDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCascadeObliqueDimension
//===========================================================================

void execCascadeObliqueDimension(Gui::Command* cmd){
    // cascade oblique dimensions
    std::vector<Gui::SelectionObject> selection;
    if (_checkSelection(cmd,selection,"TechDraw CascadeObliqueDimension")){
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cascade Oblique Dim"));
        std::vector<TechDraw::DrawViewDimension*> validDimension;
        validDimension = _getDimensions(selection,"Distance");
        if( validDimension.empty() ) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("TechDraw CascadeObliqueDimension"),
                                 QObject::tr("No oblique dimensions selected"));
            return;
        }
        float xMaster = validDimension[0]->X.getValue();
        float yMaster = validDimension[0]->Y.getValue();
        Base::Vector3d pMaster(xMaster,yMaster,0.0);
        pointPair pp = validDimension[0]->getLinearPoints();
        Base::Vector3d dirMaster = pp.second-pp.first;
        dirMaster.y = -dirMaster.y;
        Base::Vector3d origin(0.0,0.0,0.0);
        Base::Vector3d ipDelta = _getTrianglePoint(pMaster,dirMaster,origin);
        float dimDistance = activeDimAttributes.getCascadeSpacing();
        Base::Vector3d delta = ipDelta.Normalize()*dimDistance;
        int i = 0;
        for (auto dim : validDimension){
            float xDim = dim->X.getValue();
            float yDim = dim->Y.getValue();
            Base::Vector3d pDim(xDim,yDim,0.0);
            Base::Vector3d p3 = _getTrianglePoint(pMaster,dirMaster,pDim);
            p3 = p3+delta*i;
            dim->X.setValue(p3.x);
            dim->Y.setValue(p3.y);
            i = i+1;
        }
        Gui::Command::commitCommand();
    }
}

DEF_STD_CMD_A(CmdTechDrawExtensionCascadeObliqueDimension)

CmdTechDrawExtensionCascadeObliqueDimension::CmdTechDrawExtensionCascadeObliqueDimension()
  : Command("TechDraw_ExtensionCascadeObliqueDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cascade Oblique Dimensions");
    sToolTipText    = QT_TR_NOOP("Cascade oblique dimensions\n\
    - select some parallel oblique measures\n\
    - the first selected maesure defines the position\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCascadeObliqueDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionCascadeObliqueDimension";
}

void CmdTechDrawExtensionCascadeObliqueDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCascadeObliqueDimension(this);
    ///Base::Console().Message("TechDraw_ExtensionPosObliqueChainDimension started\n");
}

bool CmdTechDrawExtensionCascadeObliqueDimension::isActive(void)
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
    sToolTipText    = QT_TR_NOOP("Cascade Horizontal Dimensions\n\
    - select many horizontal dimensions\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCascadeDimensionGroup";
    sStatusTip      = sToolTipText;
}

void CmdTechDrawExtensionCascadeDimensionGroup::activated(int iMsg)
{
//    Base::Console().Message("CMD::ExtensionCascadeDimansionGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch(iMsg) {
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
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n",iMsg);
    };
}

Gui::Action * CmdTechDrawExtensionCascadeDimensionGroup::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCascadeHorizDimension"));
    p1->setObjectName(QString::fromLatin1("TechDraw_ExtensionCascadeHorizDimension"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionCascadeHorizDimension"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCascadeVertDimension"));
    p2->setObjectName(QString::fromLatin1("TechDraw_ExtensionCascadeVertDimension"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionCascadeVertDimension"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCascadeObliqueDimension"));
    p3->setObjectName(QString::fromLatin1("TechDraw_ExtensionCascadeObliqueDimension"));
    p3->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionCascadeObliqueDimension"));

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
    arc1->setText(QApplication::translate("TechDraw_Extension","Cascade Horizontal Dimensions"));
    arc1->setToolTip(QApplication::translate("TechDraw_Extension",
    "Spaces (vertically), the selected horizontal dimensions:\n\
    - select two or more horizontal dimensions\n\
    - click this button"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("TechDraw_Extension","Cascade Vertical Dimensions"));
    arc2->setToolTip(QApplication::translate("TechDraw_Extension",
    "Spaces (horizontally), the selected vertical dimensions:\n\
    - select two or more vertical dimensions\n\
    - click this button"));
    arc2->setStatusTip(arc2->toolTip());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("TechDraw_Extension","Cascade Oblique Dimensions"));
    arc3->setToolTip(QApplication::translate("TechDraw_Extension",
    "Cascade oblique dimensions\n\
    - select some parallel oblique measures\n\
    - the first selected maesure defines the position\n\
    - click this button"));
    arc3->setStatusTip(arc3->toolTip());
}

bool CmdTechDrawExtensionCascadeDimensionGroup::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateHorizChainDimension
//===========================================================================

void execCreateHorizChainDimension(Gui::Command* cmd){
    //create a horizontal chain dimension
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSelAndObj(cmd,selection,objFeat,"TechDraw Create Horizontal Chain Dimension"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Horiz Chain Dim"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    std::vector<dimVertex> allVertexes;
    allVertexes = _getVertexInfo(objFeat,subNames);
    if (!allVertexes.empty()){
        if (allVertexes.size() > 1){
            std::sort(allVertexes.begin(),allVertexes.end(),sortX);
            float yMaster = 0.0;
            for(long unsigned int n=0;n<allVertexes.size()-1;n++){
                TechDraw::DrawViewDimension* dim;
                dim = _createLinDimension(cmd,objFeat,allVertexes[n].name,allVertexes[n+1].name,"DistanceX");
                TechDraw::pointPair pp = dim->getLinearPoints();
                Base::Vector3d mid = (pp.first + pp.second)/2.0;
                dim->X.setValue(mid.x);
                if(n == 0)
                    yMaster = -mid.y;
                dim->Y.setValue(yMaster);
            }
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
    sMenuText       = QT_TR_NOOP("Create Horizontal Chain Dimension");
    sToolTipText    = QT_TR_NOOP("Create a horizontal chain dimension\n\
    - Select three or more vertexes\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCreateHorizChainDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionCreateHorizChainDimension";
}

void CmdTechDrawExtensionCreateHorizChainDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCreateHorizChainDimension(this);
    //execSortieren(this);
}

bool CmdTechDrawExtensionCreateHorizChainDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateVertChainDimension
//===========================================================================

void execCreateVertChainDimension(Gui::Command* cmd){
    //create a vertical chain dimension
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSelAndObj(cmd,selection,objFeat,"TechDraw Create Vertical Chain Dimension"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Vert Chain Dim"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    std::vector<dimVertex> allVertexes;
    allVertexes = _getVertexInfo(objFeat,subNames);
    if (!allVertexes.empty()){
        if (allVertexes.size() > 1){
            std::sort(allVertexes.begin(),allVertexes.end(),sortY);
            float xMaster = 0.0;
            for(long unsigned int n=0;n<allVertexes.size()-1;n++){
                TechDraw::DrawViewDimension* dim;
                dim = _createLinDimension(cmd,objFeat,allVertexes[n].name,allVertexes[n+1].name,"DistanceY");
                TechDraw::pointPair pp = dim->getLinearPoints();
                Base::Vector3d mid = (pp.first + pp.second)/2.0;
                if(n == 0)
                    xMaster = mid.x;
                dim->X.setValue(xMaster);
                dim->Y.setValue(-mid.y);
            }
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
    sMenuText       = QT_TR_NOOP("Create Vertical Chain Dimension");
    sToolTipText    = QT_TR_NOOP("Create vertical chain dimension\n\
    - Select three or more vertexes\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCreateVertChainDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionCreateVertChainDimension";
}

void CmdTechDrawExtensionCreateVertChainDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCreateVertChainDimension(this);
}

bool CmdTechDrawExtensionCreateVertChainDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateObliqueChainDimension
//===========================================================================

void execCreateObliqueChainDimension(Gui::Command* cmd){
    // create an oblique chain dimension
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSelAndObj(cmd,selection,objFeat,"TechDraw Create Oblique Chain Dimension"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Oblique Chain Dim"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    std::vector<dimVertex> allVertexes, carrierVertexes;
    allVertexes = _getVertexInfo(objFeat,subNames);
    if (!allVertexes.empty()){
        if (allVertexes.size() > 1){
            Base::Vector3d pMaster = allVertexes[0].point;
            Base::Vector3d dirMaster = pMaster-allVertexes[1].point;
            Base::Vector3d origin(0.0,0.0,0.0);
            Base::Vector3d delta = _getTrianglePoint(pMaster,dirMaster,origin);
            delta = delta.Normalize()*7.0;
            double scale = objFeat->getScale();
            for (dimVertex oldVertex : allVertexes){
                Base::Vector3d nextPoint = _getTrianglePoint(pMaster,dirMaster,oldVertex.point);
                nextPoint.y = -nextPoint.y;
                oldVertex.point.y = -oldVertex.point.y;
                if ((oldVertex.point-nextPoint).Length() > 0.01){
                    std::string vertTag = objFeat->addCosmeticVertex(nextPoint/scale);
                    int vertNumber = objFeat->add1CVToGV(vertTag);
                    std::stringstream ss;
                    ss << "Vertex" << vertNumber;
                    dimVertex newVertex;
                    newVertex.name = ss.str();
                    newVertex.point = nextPoint;
                    carrierVertexes.push_back(newVertex);
                    std::string edgeTag = objFeat->addCosmeticEdge(oldVertex.point/scale, nextPoint/scale);
                    auto edge = objFeat->getCosmeticEdge(edgeTag);  
                    edge->m_format.m_style = 1;
                    edge->m_format.m_weight = 0.15;
                    edge->m_format.m_color = App::Color(0.0f,0.0f,0.0f);
                } else
                    carrierVertexes.push_back(oldVertex);
            }
            std::sort(carrierVertexes.begin(),carrierVertexes.end(),sortX);
            for(long unsigned int n=0;n<allVertexes.size()-1;n++){
                TechDraw::DrawViewDimension* dim;
                dim = _createLinDimension(cmd,objFeat,carrierVertexes[n].name,carrierVertexes[n+1].name,"Distance");
                TechDraw::pointPair pp = dim->getLinearPoints();
                Base::Vector3d mid = (pp.first + pp.second)/2.0+delta;
                dim->X.setValue(mid.x);
                dim->Y.setValue(-mid.y);
            }
        }
        objFeat->refreshCEGeoms();
        objFeat->requestPaint();
        cmd->getSelection().clearSelection();
        Gui::Command::commitCommand();
    }
}

DEF_STD_CMD_A(CmdTechDrawExtensionCreateObliqueChainDimension)

CmdTechDrawExtensionCreateObliqueChainDimension::CmdTechDrawExtensionCreateObliqueChainDimension()
  : Command("TechDraw_ExtensionCreateObliqueChainDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Create Oblique Chain Dimension");
    sToolTipText    = QT_TR_NOOP("Create an oblique dimension chain\n\
    - select three or more  vertexes\n\
    - the first two define the direction\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCreateObliqueChainDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionCreateObliqueChainDimension";
}

void CmdTechDrawExtensionCreateObliqueChainDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCreateObliqueChainDimension(this);
}

bool CmdTechDrawExtensionCreateObliqueChainDimension::isActive(void)
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
    sMenuText       = QT_TR_NOOP("Create Chain Dimensions");
    sToolTipText    = QT_TR_NOOP("Create a horizontal chain dimension\n\
    - Select three or more vertexes\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCreateChainDimensionGroup";
    sStatusTip      = sToolTipText;
}

void CmdTechDrawExtensionCreateChainDimensionGroup::activated(int iMsg)
{
//    Base::Console().Message("CMD::ExtensionCascadeDimansionGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch(iMsg) {
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
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n",iMsg);
    };
}

Gui::Action * CmdTechDrawExtensionCreateChainDimensionGroup::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCreateHorizChainDimension"));
    p1->setObjectName(QString::fromLatin1("TechDraw_ExtensionCreateHorizChainDimension"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionCreateHorizChainDimension"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCreateVertChainDimension"));
    p2->setObjectName(QString::fromLatin1("TechDraw_ExtensionCreateVertChainDimension"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionCreateVertChainDimension"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCreateObliqueChainDimension"));
    p3->setObjectName(QString::fromLatin1("TechDraw_ExtensionCreateObliqueChainDimension"));
    p3->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionCreateObliqueChainDimension"));

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
    arc1->setText(QApplication::translate("TechDraw_Extension","Create Horizontal Chain Dimensions"));
    arc1->setToolTip(QApplication::translate("TechDraw_Extension",
    "Create a horizontal chain dimension\n\
    - Select three or more vertexes\n\
    - click this button"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("TechDraw_Extension","Create Vertical Chain Dimensions"));
    arc2->setToolTip(QApplication::translate("TechDraw_Extension",
    "Create vertical chain dimension\n\
    - Select three or more vertexes\n\
    - click this button"));
    arc2->setStatusTip(arc2->toolTip());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("TechDraw_Extension","Create Oblique Chain Dimensions"));
    arc3->setToolTip(QApplication::translate("TechDraw_Extension",
    "Create an oblique dimension chain\n\
    - select three or more  vertexes\n\
    - the first two define the direction\n\
    - click this button"));
    arc3->setStatusTip(arc3->toolTip());
}

bool CmdTechDrawExtensionCreateChainDimensionGroup::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateHorizCoordDimension
//===========================================================================

void execCreateHorizCoordDimension(Gui::Command* cmd){
    //create horizontal coordinate dimensions
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSelAndObj(cmd,selection,objFeat,"TechDraw Create Horizontal Coord Dimension"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Horiz Coord Dim"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    std::vector<dimVertex> allVertexes;
    allVertexes = _getVertexInfo(objFeat,subNames);
    if (!allVertexes.empty()){
        if (allVertexes.size() > 1){
            std::sort(allVertexes.begin(),allVertexes.end(),sortX);
            float dimDistance = activeDimAttributes.getCascadeSpacing();
            float yMaster = allVertexes[0].point.y-dimDistance;
            if (signbit(yMaster))
                dimDistance = -dimDistance;
            for(long unsigned int n=0;n<allVertexes.size()-1;n++){
                TechDraw::DrawViewDimension* dim;
                dim = _createLinDimension(cmd,objFeat,allVertexes[0].name,allVertexes[n+1].name,"DistanceX");
                TechDraw::pointPair pp = dim->getLinearPoints();
                Base::Vector3d mid = (pp.first + pp.second)/2.0;
                dim->X.setValue(mid.x);
                dim->Y.setValue(-yMaster-dimDistance*n);
            }
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
    sMenuText       = QT_TR_NOOP("Create Horizontal Coordinate Dimension");
    sToolTipText    = QT_TR_NOOP("Create horizontal coordinate dimensions\n\
    - select three or more vertexes\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCreateHorizCoordDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionCreateHorizCoordDimension";
}

void CmdTechDrawExtensionCreateHorizCoordDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCreateHorizCoordDimension(this);
}

bool CmdTechDrawExtensionCreateHorizCoordDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateVertCoordDimension
//===========================================================================

void execCreateVertCoordDimension(Gui::Command* cmd){
    //create vertical coordinate dimensions
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSelAndObj(cmd,selection,objFeat,"TechDraw Create Vertical Coord Dimension"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Vert Coord Dim"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    std::vector<dimVertex> allVertexes;
    allVertexes = _getVertexInfo(objFeat,subNames);
    if (!allVertexes.empty()){
        if (allVertexes.size() > 1){
            std::sort(allVertexes.begin(),allVertexes.end(),sortY);
            std::reverse(allVertexes.begin(),allVertexes.end());
            float dimDistance = activeDimAttributes.getCascadeSpacing();
            float xMaster = allVertexes[0].point.x+dimDistance;
            if (signbit(xMaster))
                dimDistance = -dimDistance;
            for(long unsigned int n=0;n<allVertexes.size()-1;n++){
                TechDraw::DrawViewDimension* dim;
                dim = _createLinDimension(cmd,objFeat,allVertexes[0].name,allVertexes[n+1].name,"DistanceY");
                TechDraw::pointPair pp = dim->getLinearPoints();
                Base::Vector3d mid = (pp.first + pp.second)/2.0;
                dim->X.setValue(xMaster+dimDistance*n);
                dim->Y.setValue(-mid.y);
            }
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
    sMenuText       = QT_TR_NOOP("Create Vertical Coordinate Dimension");
    sToolTipText    = QT_TR_NOOP("Create vertical coordinate dimensions\n\
    - select three or more vertexes\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCreateVertCoordDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionCreateVertCoordDimension";
}

void CmdTechDrawExtensionCreateVertCoordDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCreateVertCoordDimension(this);
}

bool CmdTechDrawExtensionCreateVertCoordDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateObliqueCoordDimension
//===========================================================================

void execCreateObliqueCoordDimension(Gui::Command* cmd){
    //create oblique coordinate dimensions
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSelAndObj(cmd,selection,objFeat,"TechDraw Create Oblique Coord Dimension"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Oblique Coord Dim"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    std::vector<dimVertex> allVertexes, carrierVertexes;
    allVertexes = _getVertexInfo(objFeat,subNames);
    if (!allVertexes.empty()){
        if (allVertexes.size() > 1){
            Base::Vector3d pMaster = allVertexes[0].point;
            Base::Vector3d dirMaster = pMaster-allVertexes[1].point;
            Base::Vector3d origin(0.0,0.0,0.0);
            Base::Vector3d delta = _getTrianglePoint(pMaster,dirMaster,origin);
            float dimDistance = activeDimAttributes.getCascadeSpacing();
            delta = delta.Normalize()*dimDistance;
            double scale = objFeat->getScale();
            for (dimVertex oldVertex : allVertexes){
                Base::Vector3d nextPoint = _getTrianglePoint(pMaster,dirMaster,oldVertex.point);
                nextPoint.y = -nextPoint.y;
                oldVertex.point.y = -oldVertex.point.y;
                if ((oldVertex.point-nextPoint).Length() > 0.01){
                    std::string vertTag = objFeat->addCosmeticVertex(nextPoint/scale);
                    int vertNumber = objFeat->add1CVToGV(vertTag);
                    std::stringstream ss;
                    ss << "Vertex" << vertNumber;
                    dimVertex newVertex;
                    newVertex.name = ss.str();
                    newVertex.point = nextPoint;
                    carrierVertexes.push_back(newVertex);
                    std::string edgeTag = objFeat->addCosmeticEdge(oldVertex.point/scale, nextPoint/scale);
                    auto edge = objFeat->getCosmeticEdge(edgeTag);  
                    edge->m_format.m_style = 1;
                    edge->m_format.m_weight = 0.15;
                    edge->m_format.m_color = App::Color(0.0f,0.0f,0.0f);
                } else
                    carrierVertexes.push_back(oldVertex);
            }
            std::sort(carrierVertexes.begin(),carrierVertexes.end(),sortX);
            for(long unsigned int n=0;n<allVertexes.size()-1;n++){
                TechDraw::DrawViewDimension* dim;
                dim = _createLinDimension(cmd,objFeat,carrierVertexes[0].name,carrierVertexes[n+1].name,"Distance");
                TechDraw::pointPair pp = dim->getLinearPoints();
                Base::Vector3d mid = (pp.first + pp.second)/2.0+delta*(n+1);
                dim->X.setValue(mid.x);
                dim->Y.setValue(-mid.y);
            }
        }
        objFeat->refreshCEGeoms();
        objFeat->requestPaint();
        cmd->getSelection().clearSelection();
        Gui::Command::commitCommand();
    }
}

DEF_STD_CMD_A(CmdTechDrawExtensionCreateObliqueCoordDimension)

CmdTechDrawExtensionCreateObliqueCoordDimension::CmdTechDrawExtensionCreateObliqueCoordDimension()
  : Command("TechDraw_ExtensionCreateObliqueCoordDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Create Oblique Coordinate Dimension");
    sToolTipText    = QT_TR_NOOP("Create an oblique coordinate dimension\n\
    - select three or more  vertexes\n\
    - the first two define the direction\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCreateObliqueCoordDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionCreateObliqueCoordDimension";
}

void CmdTechDrawExtensionCreateObliqueCoordDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCreateObliqueCoordDimension(this);
}

bool CmdTechDrawExtensionCreateObliqueCoordDimension::isActive(void)
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
    sMenuText       = QT_TR_NOOP("Create Coordinate Dimensions");
    sToolTipText    = QT_TR_NOOP("Create horizontal coordinate dimensions\n\
    - select three or more vertexes\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCreateCoordDimensionGroup";
    sStatusTip      = sToolTipText;
}

void CmdTechDrawExtensionCreateCoordDimensionGroup::activated(int iMsg)
{
//    Base::Console().Message("CMD::ExtensionCascadeDimansionGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch(iMsg) {
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
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n",iMsg);
    };
}

Gui::Action * CmdTechDrawExtensionCreateCoordDimensionGroup::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCreateHorizCoordDimension"));
    p1->setObjectName(QString::fromLatin1("TechDraw_ExtensionCreateHorizCoordDimension"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionCreateHorizCoordDimension"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCreateVertCoordDimension"));
    p2->setObjectName(QString::fromLatin1("TechDraw_ExtensionCreateVertCoordDimension"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionCreateVertCoordDimension"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCreateObliqueCoordDimension"));
    p3->setObjectName(QString::fromLatin1("TechDraw_ExtensionCreateObliqueCoordDimension"));
    p3->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionCreateObliqueCoordDimension"));

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
    arc1->setText(QApplication::translate("TechDraw_Extension","Create Horizontal Coordinate Dimensions"));
    arc1->setToolTip(QApplication::translate("TechDraw_Extension",
    "Create horizontal coordinate dimensions\n\
    - select three or more vertexes\n\
    - click this button"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("TechDraw_Extension","Create Vertical Coordinate Dimensions"));
    arc2->setToolTip(QApplication::translate("TechDraw_Extension",
    "Create vertical coordinate dimensions\n\
    - select three or more vertexes\n\
    - click this button"));
    arc2->setStatusTip(arc2->toolTip());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("TechDraw_Extension","Create Oblique Coordinate Dimensions"));
    arc3->setToolTip(QApplication::translate("TechDraw_Extension",
    "Create an oblique coordinate dimension\n\
    - select three or more  vertexes\n\
    - the first two define the direction\n\
    - click this button"));
    arc3->setStatusTip(arc3->toolTip());
}

bool CmdTechDrawExtensionCreateCoordDimensionGroup::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateHorizChamferDimension
//===========================================================================

void execCreateHorizChamferDimension(Gui::Command* cmd){
    //create a horizontal chamfer dimension
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSelAndObj(cmd,selection,objFeat,"TechDraw Create Horizontal Chamfer Dimension"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Horiz Chamfer Dim"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    std::vector<dimVertex> allVertexes;
    allVertexes = _getVertexInfo(objFeat,subNames);
    if (!allVertexes.empty()){
        if (allVertexes.size() > 1){
            const float Pi180 = 180.0/3.14159;
            TechDraw::DrawViewDimension* dim;
            dim = _createLinDimension(cmd,objFeat,allVertexes[0].name,allVertexes[1].name,"DistanceX");
            float yMax = std::max(abs(allVertexes[0].point.y),abs(allVertexes[1].point.y))+7.0;
            if (signbit(allVertexes[0].point.y))
                yMax = -yMax;
            TechDraw::pointPair pp = dim->getLinearPoints();
            Base::Vector3d mid = (pp.first + pp.second)/2.0;
            dim->X.setValue(mid.x);
            dim->Y.setValue(-yMax);
            float dx = allVertexes[0].point.x-allVertexes[1].point.x;
            float dy = allVertexes[0].point.y-allVertexes[1].point.y;
            float alpha = round(abs(atan(dy/dx))*Pi180);
            std::string sAlpha = std::to_string((int) alpha);
            std::string formatSpec = dim->FormatSpec.getStrValue();
            formatSpec = formatSpec+"x"+sAlpha+"°";
            dim->FormatSpec.setValue(formatSpec);
            objFeat->requestPaint();
            cmd->getSelection().clearSelection();
        }
    }
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionCreateHorizChamferDimension)

CmdTechDrawExtensionCreateHorizChamferDimension::CmdTechDrawExtensionCreateHorizChamferDimension()
  : Command("TechDraw_ExtensionCreateHorizChamferDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Create Horizontal Chamfer Dimension");
    sToolTipText    = QT_TR_NOOP("Create Horizontal Chamfer Dimension\n\
    - select two vertexes\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCreateHorizChamferDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionCreateHorizChamferDimension";
}

void CmdTechDrawExtensionCreateHorizChamferDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCreateHorizChamferDimension(this);
}

bool CmdTechDrawExtensionCreateHorizChamferDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCreateVertChamferDimension
//===========================================================================

void execCreateVertChamferDimension(Gui::Command* cmd){
    //create a vertical chamfer dimension
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSelAndObj(cmd,selection,objFeat,"TechDraw Create Vertical Chamfer Dimension"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Vert Chamfer Dim"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    std::vector<dimVertex> allVertexes;
    allVertexes = _getVertexInfo(objFeat,subNames);
    if (!allVertexes.empty()){
        if (allVertexes.size() > 1){
            const float Pi180 = 180.0/3.14159;
            TechDraw::DrawViewDimension* dim;
            dim = _createLinDimension(cmd,objFeat,allVertexes[0].name,allVertexes[1].name,"DistanceY");
            float xMax = std::max(abs(allVertexes[0].point.x),abs(allVertexes[1].point.x))+7.0;
            if (signbit(allVertexes[0].point.x))
                xMax = -xMax;
            TechDraw::pointPair pp = dim->getLinearPoints();
            Base::Vector3d mid = (pp.first + pp.second)/2.0;
            dim->X.setValue(xMax);
            dim->Y.setValue(-mid.y);
            float dx = allVertexes[0].point.x-allVertexes[1].point.x;
            float dy = allVertexes[0].point.y-allVertexes[1].point.y;
            float alpha = round(abs(atan(dx/dy))*Pi180);
            std::string sAlpha = std::to_string((int) alpha);
            std::string formatSpec = dim->FormatSpec.getStrValue();
            formatSpec = formatSpec+"x"+sAlpha+"°";
            dim->FormatSpec.setValue(formatSpec);
            objFeat->requestPaint();
            cmd->getSelection().clearSelection();
        }
    }
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionCreateVertChamferDimension)

CmdTechDrawExtensionCreateVertChamferDimension::CmdTechDrawExtensionCreateVertChamferDimension()
  : Command("TechDraw_ExtensionCreateVertChamferDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Create Vertical Chamfer Dimension");
    sToolTipText    = QT_TR_NOOP("Create Vertical Chamfer Dimension\n\
    - select two vertexes\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCreateVertChamferDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionCreateVertChamferDimension";
}

void CmdTechDrawExtensionCreateVertChamferDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCreateVertChamferDimension(this);
}

bool CmdTechDrawExtensionCreateVertChamferDimension::isActive(void)
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
    sMenuText       = QT_TR_NOOP("Create Horizontal Chamfer Dimension");
    sToolTipText    = QT_TR_NOOP("Create Horizontal Chamfer Dimension\n\
    - select two vertexes\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionChamferDimensionGroup";
    sStatusTip      = sToolTipText;
}

void CmdTechDrawExtensionChamferDimensionGroup::activated(int iMsg)
{
//    Base::Console().Message("CMD::ExtensionIncreaseDecreaseGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch(iMsg) {
        case 0:                 //create a horizontal chamfer dimension
            execCreateHorizChamferDimension(this);
            break;
        case 1:                 //create a vertical chamfer dimension
            execCreateVertChamferDimension(this);
            break;
        default:
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n",iMsg);
    };
}

Gui::Action * CmdTechDrawExtensionChamferDimensionGroup::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCreateHorizChamferDimension"));
    p1->setObjectName(QString::fromLatin1("TechDraw_ExtensionCreateHorizChamferDimension"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionCreateHorizChamferDimension"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCreateVertChamferDimension"));
    p2->setObjectName(QString::fromLatin1("TechDraw_ExtensionCreateVertChamferDimension"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionCreateVertChamferDimension"));

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
    arc1->setText(QApplication::translate("TechDraw_Extension","Create Horizontal Chamfer Dimension"));
    arc1->setToolTip(QApplication::translate("TechDraw_Extension","Create Horizontal Chamfer Dimension\n\
    - select two vertexes\n\
    - click this button"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("TechDraw_Extension","Create Vertical Chamfer Dimension"));
    arc2->setToolTip(QApplication::translate("TechDraw_Extension","Create Vertical Chamfer Dimension\n\
    - select two vertexes\n\
    - click this button"));
    arc2->setStatusTip(arc2->toolTip());
}

bool CmdTechDrawExtensionChamferDimensionGroup::isActive(void)
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
    sMenuText       = QT_TR_NOOP("Create Development/length arc");
    sToolTipText    = QT_TR_NOOP("Create Development/length arc\n\
    - select one arc\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCreateLengthArc";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionCreateLengthArc";
}

void CmdTechDrawExtensionCreateLengthArc::activated(int iMsg){
    // create a development/length arc dimension
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSelAndObj(this,selection,objFeat,"TechDraw Create Development/length arc"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Length Arc"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    int geoId = TechDraw::DrawUtil::getIndexFromName(subNames[0]);
    TechDraw::BaseGeom* geom = objFeat->getGeomByIndex(geoId);
    std::string geoType = TechDraw::DrawUtil::getGeomTypeFromName(subNames[0]);
    if (geoType == "Edge"){
        if (geom->geomType == TechDraw::ARCOFCIRCLE){
            TechDraw::AOC* arcTag = static_cast<TechDraw::AOC *>(geom);
            float radius = arcTag->radius;
            Base::Vector3d centerPt = arcTag->center;
            centerPt.y = -centerPt.y;
            Base::Vector3d startPt = arcTag->startPnt;
            startPt.y = -startPt.y;
            Base::Vector3d endPt = arcTag->endPnt;
            endPt.y = -endPt.y;
            std::stringstream startName, endName, formatSpec;
            double scale = objFeat->getScale();
            std::string startVertTag = objFeat->addCosmeticVertex(startPt/scale);
            int startVertNumber = objFeat->add1CVToGV(startVertTag);
            startName << "Vertex" << startVertNumber;
            std::string endVertTag = objFeat->addCosmeticVertex(endPt/scale);
            int endVertNumber = objFeat->add1CVToGV(endVertTag);
            endName << "Vertex" << endVertNumber;
            TechDraw::DrawViewDimension* dim;
            dim = _createLinDimension(this,objFeat,startName.str(),endName.str(),"Distance");
            TechDraw::pointPair pp = dim->getLinearPoints();
            Base::Vector3d mid = (pp.first + pp.second)/2.0;
            dim->X.setValue(mid.x);
            dim->Y.setValue(-mid.y);            
            Base::Vector3d radVec1 = startPt-centerPt;
            Base::Vector3d radVec2 = endPt-centerPt;
            float alpha = acos((radVec1*radVec2)/(radVec1.Length()*radVec2.Length()));
            float arcLength = alpha*radius/scale;
            dim->Arbitrary.setValue(true);
            formatSpec << "∩ " << arcLength;
            dim->FormatSpec.setValue(formatSpec.str());
            objFeat->refreshCEGeoms();
            objFeat->requestPaint();
            getSelection().clearSelection();
        }
    }
    Gui::Command::commitCommand();
}

bool CmdTechDrawExtensionCreateLengthArc::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// internal helper routines
//===========================================================================

bool _checkSelection(Gui::Command* cmd,
                    std::vector<Gui::SelectionObject>& selection,
                    std::string message){
    // check selection of getSelectionEx()
    bool OK = true;
    selection = cmd->getSelection().getSelectionEx();
    if( selection.empty() ) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr(message.c_str()),
                             QObject::tr("Selection is empty"));
        OK = false;
    }
    return OK;
}

bool _checkSelAndObj(Gui::Command* cmd,
                    std::vector<Gui::SelectionObject>& selection,
                    TechDraw::DrawViewPart*& objFeat,
                    std::string message){
    // check selection of getSelectionEx() and selection[0].getObject()
    bool OK = true;
    OK = _checkSelection(cmd,selection,message);
    if (OK) {
        objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
        if( objFeat == nullptr ) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr(message.c_str()),
                                 QObject::tr("No object selected"));
            OK = false;
        }
    }
    return OK;
}

TechDraw::DrawViewDimension* _createLinDimension(Gui::Command* cmd,
                                                 TechDraw::DrawViewPart* objFeat,
                                                 std::string startVertex,
                                                 std::string endVertex,
                                                 std::string dimType)
    // create a new linear dimension
{
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();
    TechDraw::DrawViewDimension* dim = 0;
    std::string FeatName = cmd->getUniqueObjectName("Dimension");
    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;
    objs.push_back(objFeat);
    objs.push_back(objFeat);
    subs.push_back(startVertex);
    subs.push_back(endVertex);
    cmd->doCommand(cmd->Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')",FeatName.c_str());
    cmd->doCommand(cmd->Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str(),dimType.c_str());
    dim = dynamic_cast<TechDraw::DrawViewDimension *>(cmd->getDocument()->getObject(FeatName.c_str()));
    if (!dim)
        throw Base::TypeError("CmdTechDrawExtensionCreateLinDimension - dim not found\n");
    dim->References2D.setValues(objs, subs);
    cmd->doCommand(cmd->Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    dim->recomputeFeature();
    return dim;
}

std::vector<dimVertex> _getVertexInfo(TechDraw::DrawViewPart* objFeat,
                                      std::vector<std::string> subNames){
    // get subNames and coordinates of all selected vertexes
    std::vector<dimVertex> vertexes;
    dimVertex nextVertex;
    for (std::string name : subNames){    
        std::string geoType = TechDraw::DrawUtil::getGeomTypeFromName(name);
        if (geoType == "Vertex"){
            int geoId = TechDraw::DrawUtil::getIndexFromName(name);
            //TechDraw::Vertex* vert = objFeat->getProjVertexByIndex(geoId);
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

Base::Vector3d _getTrianglePoint(Base::Vector3d p1, Base::Vector3d dir, Base::Vector3d p2){
    // get third point of a perpendicular triangle
    // p1, p2 ...vertexes of hypothenusis, dir ...direction of one kathete, p3 ...3rd vertex
    float a = -dir.y;
    float b = dir.x;
    float c1 = p1.x*a+p1.y*b;
    float c2 = -p2.x*b+p2.y*a;
    float ab = a*a+b*b;
    float x = (c1*a-c2*b)/ab;
    float y = (c2*a+c1*b)/ab;
    Base::Vector3d p3(x,y,0.0);
    return p3;
}

std::vector<TechDraw::DrawViewDimension*>_getDimensions(std::vector<Gui::SelectionObject> selection,std::string needDimType){
    // get all selected dimensions of type needDimType
    std::vector<TechDraw::DrawViewDimension*> validDimension;
    for (auto selected : selection){
        auto object = selected.getObject();
        if (object->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId())){
            auto dim = dynamic_cast<TechDraw::DrawViewDimension*>(selected.getObject());
            std::string dimType = dim->Type.getValueAsString();
            if (dimType == needDimType)
                validDimension.push_back(dim);
        }
    }
    return validDimension;
}

//------------------------------------------------------------------------------
void CreateTechDrawCommandsExtensionDims(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawExtensionInsertPraefixGroup());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionInsertDiameter());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionInsertSquare());
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
}