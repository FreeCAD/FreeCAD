/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QFileInfo>
# include <QPointer>
# include <QString>
# include <Standard_math.hxx>
# include <Standard_Version.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS_Shape.hxx>
#endif
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <App/Link.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Gui/WaitCursor.h>

#include "BoxSelection.h"
#include "CrossSections.h"
#include "DlgBooleanOperation.h"
#include "DlgExtrusion.h"
#include "DlgScale.h"
#include "DlgFilletEdges.h"
#include "DlgPrimitives.h"
#include "DlgProjectionOnSurface.h"
#include "DlgRevolution.h"
#include "Mirroring.h"
#include "SectionCutting.h"
#include "TaskCheckGeometry.h"
#include "TaskDimension.h"
#include "TaskLoft.h"
#include "TaskShapeBuilder.h"
#include "TaskSweep.h"
#include "ViewProvider.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//===========================================================================
// Part_PickCurveNet
//===========================================================================
DEF_STD_CMD(CmdPartPickCurveNet)

CmdPartPickCurveNet::CmdPartPickCurveNet()
  :Command("Part_PickCurveNet")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Pick curve network");
    sToolTipText  = QT_TR_NOOP("Pick a curve network");
    sWhatsThis    = "Part_PickCurveNet";
    sStatusTip    = sToolTipText;
    sPixmap       = "Test1";
}

void CmdPartPickCurveNet::activated(int iMsg)
{
    Q_UNUSED(iMsg);
}

//===========================================================================
// Part_NewDoc
//===========================================================================
DEF_STD_CMD(CmdPartNewDoc)

CmdPartNewDoc::CmdPartNewDoc()
  :Command("Part_NewDoc")
{
    sAppModule    = "Part";
    sGroup        = "Part";
    sMenuText     = "New document";
    sToolTipText  = "Create an empty part document";
    sWhatsThis    = "Part_NewDoc";
    sStatusTip    = sToolTipText;
    sPixmap       = "New";
}

void CmdPartNewDoc::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Doc,"d = App.New()");
    updateActive();
}

//===========================================================================
// Part_Box2
//===========================================================================
DEF_STD_CMD_A(CmdPartBox2)

CmdPartBox2::CmdPartBox2()
  :Command("Part_Box2")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Box fix 1");
    sToolTipText  = QT_TR_NOOP("Create a box solid without dialog");
    sWhatsThis    = "Part_Box2";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Box";
}

void CmdPartBox2::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    openCommand(QT_TRANSLATE_NOOP("Command", "Part Box Create"));
    doCommand(Doc,"from FreeCAD import Base");
    doCommand(Doc,"import Part");
    doCommand(Doc,"__fb__ = App.ActiveDocument.addObject(\"Part::Box\",\"PartBox\")");
    doCommand(Doc,"__fb__.Location = Base.Vector(0.0,0.0,0.0)");
    doCommand(Doc,"__fb__.Length = 100.0");
    doCommand(Doc,"__fb__.Width = 100.0");
    doCommand(Doc,"__fb__.Height = 100.0");
    doCommand(Doc,"del __fb__");
    commitCommand();
    updateActive();
}

bool CmdPartBox2::isActive()
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Part_Box3
//===========================================================================
DEF_STD_CMD_A(CmdPartBox3)

CmdPartBox3::CmdPartBox3()
  :Command("Part_Box3")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Box fix 2");
    sToolTipText  = QT_TR_NOOP("Create a box solid without dialog");
    sWhatsThis    = "Part_Box3";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Box";
}

void CmdPartBox3::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    openCommand(QT_TRANSLATE_NOOP("Command", "Part Box Create"));
    doCommand(Doc,"from FreeCAD import Base");
    doCommand(Doc,"import Part");
    doCommand(Doc,"__fb__ = App.ActiveDocument.addObject(\"Part::Box\",\"PartBox\")");
    doCommand(Doc,"__fb__.Location = Base.Vector(50.0,50.0,50.0)");
    doCommand(Doc,"__fb__.Length = 100.0");
    doCommand(Doc,"__fb__.Width = 100.0");
    doCommand(Doc,"__fb__.Height = 100.0");
    doCommand(Doc,"del __fb__");
    commitCommand();
    updateActive();
}

bool CmdPartBox3::isActive()
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Part_Primitives
//===========================================================================
DEF_STD_CMD_A(CmdPartPrimitives)

CmdPartPrimitives::CmdPartPrimitives()
  :Command("Part_Primitives")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Create primitives...");
    sToolTipText  = QT_TR_NOOP("Creation of parametrized geometric primitives");
    sWhatsThis    = "Part_Primitives";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Primitives";
}

void CmdPartPrimitives::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    PartGui::TaskPrimitives* dlg = new PartGui::TaskPrimitives();
    Gui::Control().showDialog(dlg);
}

bool CmdPartPrimitives::isActive()
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

namespace PartGui {
bool checkForSolids(const TopoDS_Shape& shape)
{
    TopExp_Explorer xp;
    xp.Init(shape, TopAbs_FACE, TopAbs_SHELL);
    if (xp.More()) {
        return false;
    }
    xp.Init(shape, TopAbs_WIRE, TopAbs_FACE);
    if (xp.More()) {
        return false;
    }
    xp.Init(shape, TopAbs_EDGE, TopAbs_WIRE);
    if (xp.More()) {
        return false;
    }
    xp.Init(shape, TopAbs_VERTEX, TopAbs_EDGE);
    if (xp.More()) {
        return false;
    }

    return true;
}
/*
 * returns vector of Part::TopoShapes from selected Part::Feature derived objects,
 * App::Links linked to Part::Features, or App::Part containers with visible Part::Features
 */
std::vector<Part::TopoShape> getShapesFromSelection()
{
    std::vector<App::DocumentObject*> objs = Gui::Selection().getObjectsOfType(App::DocumentObject::getClassTypeId());
    std::vector <Part::TopoShape> shapes;
    for (auto it : objs) {
        Part::TopoShape shp = Part::Feature::getTopoShape(it);
        if (!shp.isNull()){
            shapes.push_back(shp);
        }
    }
    return shapes;
}
/*
 * returns true if selected objects contain valid Part::TopoShapes.
 * Objects can be Part::Features, App::Links, or App::Parts
 */
bool hasShapesInSelection()
{
    bool hasShapes = false;
    std::vector<App::DocumentObject*> docobjs = Gui::Selection().getObjectsOfType(App::DocumentObject::getClassTypeId());
    for (auto it : docobjs) {
        // Only check for the existence of a shape but don't perform a transformation
        if (!Part::Feature::getTopoShape(it, nullptr, false, nullptr, nullptr, true, false, false).isNull()) {
            hasShapes = true;
            break;
        }
    }
    return hasShapes;
}
}

//===========================================================================
// Part_Cut
//===========================================================================
DEF_STD_CMD_A(CmdPartCut)

CmdPartCut::CmdPartCut()
  :Command("Part_Cut")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Cut");
    sToolTipText  = QT_TR_NOOP("Make a cut of two shapes");
    sWhatsThis    = "Part_Cut";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Cut";
}

void CmdPartCut::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> Sel =
        getSelection().getSelectionEx(nullptr, App::DocumentObject::getClassTypeId(), Gui::ResolveMode::FollowLink);
    if (Sel.size() != 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two shapes please."));
        return;
    }

    bool askUser = false;
    std::vector<std::string> names;
    for (const auto & it : Sel) {
        const App::DocumentObject* obj = it.getObject();
        const TopoDS_Shape& shape = Part::Feature::getShape(obj);
        if (!PartGui::checkForSolids(shape) && !askUser) {
            int ret = QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Non-solids selected"),
                QObject::tr("The use of non-solids for boolean operations may lead to unexpected results.\n"
                            "Do you want to continue?"), QMessageBox::Yes, QMessageBox::No);
            if (ret == QMessageBox::No)
                return;
            askUser = true;
        }

        names.push_back(Base::Tools::quoted(it.getFeatName()));
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Part Cut"));
    doCommand(Doc, "from BOPTools import BOPFeatures");
    doCommand(Doc, "bp = BOPFeatures.BOPFeatures(App.activeDocument())");
    doCommand(Doc, "bp.make_cut([%s])", Base::Tools::joinList(names).c_str());
    updateActive();
    commitCommand();
}

bool CmdPartCut::isActive()
{
    return getSelection().countObjectsOfType(
            App::DocumentObject::getClassTypeId(), nullptr, Gui::ResolveMode::FollowLink)==2;
}

//===========================================================================
// Part_Common
//===========================================================================
DEF_STD_CMD_A(CmdPartCommon)

CmdPartCommon::CmdPartCommon()
  :Command("Part_Common")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Intersection");
    sToolTipText  = QT_TR_NOOP("Make an intersection of two shapes");
    sWhatsThis    = "Part_Common";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Common";
}

void CmdPartCommon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> Sel =
        getSelection().getSelectionEx(nullptr, App::DocumentObject::getClassTypeId(), Gui::ResolveMode::FollowLink);

    //test if selected object is a compound, and if it is, look how many children it has...
    std::size_t numShapes = 0;
    if (Sel.size() == 1){
        numShapes = 1; //to be updated later in code, if
        Gui::SelectionObject selobj = Sel[0];
        TopoDS_Shape sh = Part::Feature::getShape(selobj.getObject());
        if (sh.ShapeType() == TopAbs_COMPOUND) {
            numShapes = 0;
            TopoDS_Iterator it(sh);
            for (; it.More(); it.Next()) {
                ++numShapes;
            }
        }
    } else {
        numShapes = Sel.size();
    }
    if (numShapes < 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two shapes or more, please. Or, select one compound containing two or more shapes to compute common between."));
        return;
    }

    bool askUser = false;
    std::vector<std::string> names;
    for (const auto & it : Sel) {
        const App::DocumentObject* obj = it.getObject();
        const TopoDS_Shape& shape = Part::Feature::getShape(obj);
        if (!PartGui::checkForSolids(shape) && !askUser) {
            int ret = QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Non-solids selected"),
                QObject::tr("The use of non-solids for boolean operations may lead to unexpected results.\n"
                            "Do you want to continue?"), QMessageBox::Yes, QMessageBox::No);
            if (ret == QMessageBox::No)
                return;
            askUser = true;
        }

        names.push_back(Base::Tools::quoted(it.getFeatName()));
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Common"));
    doCommand(Doc, "from BOPTools import BOPFeatures");
    doCommand(Doc, "bp = BOPFeatures.BOPFeatures(App.activeDocument())");
    doCommand(Doc, "bp.make_multi_common([%s])", Base::Tools::joinList(names).c_str());
    updateActive();
    commitCommand();
}

bool CmdPartCommon::isActive()
{
    return getSelection().countObjectsOfType(
            App::DocumentObject::getClassTypeId(), nullptr, Gui::ResolveMode::FollowLink) >= 1;
}

//===========================================================================
// Part_Fuse
//===========================================================================
DEF_STD_CMD_A(CmdPartFuse)

CmdPartFuse::CmdPartFuse()
  :Command("Part_Fuse")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Union");
    sToolTipText  = QT_TR_NOOP("Make a union of several shapes");
    sWhatsThis    = "Part_Fuse";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Fuse";
}

void CmdPartFuse::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> Sel =
        getSelection().getSelectionEx(nullptr, App::DocumentObject::getClassTypeId(), Gui::ResolveMode::FollowLink);

    //test if selected object is a compound, and if it is, look how many children it has...
    std::size_t numShapes = 0;
    if (Sel.size() == 1){
        numShapes = 1; //to be updated later in code
        Gui::SelectionObject selobj = Sel[0];
        TopoDS_Shape sh = Part::Feature::getShape(selobj.getObject());
        if (sh.ShapeType() == TopAbs_COMPOUND) {
            numShapes = 0;
            TopoDS_Iterator it(sh);
            for (; it.More(); it.Next()) {
                ++numShapes;
            }
        }
    } else {
        numShapes = Sel.size();
    }
    if (numShapes < 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two shapes or more, please. Or, select one compound containing two or more shapes to be fused."));
        return;
    }

    bool askUser = false;
    std::vector<std::string> names;
    for (const auto & it : Sel) {
        const App::DocumentObject* obj = it.getObject();
        const TopoDS_Shape& shape = Part::Feature::getShape(obj);
        if (!PartGui::checkForSolids(shape) && !askUser) {
            int ret = QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Non-solids selected"),
                QObject::tr("The use of non-solids for boolean operations may lead to unexpected results.\n"
                            "Do you want to continue?"), QMessageBox::Yes, QMessageBox::No);
            if (ret == QMessageBox::No)
                return;
            askUser = true;
        }

        names.push_back(Base::Tools::quoted(it.getFeatName()));
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Fusion"));
    doCommand(Doc, "from BOPTools import BOPFeatures");
    doCommand(Doc, "bp = BOPFeatures.BOPFeatures(App.activeDocument())");
    doCommand(Doc, "bp.make_multi_fuse([%s])", Base::Tools::joinList(names).c_str());
    updateActive();
    commitCommand();
}

bool CmdPartFuse::isActive()
{
    return getSelection().countObjectsOfType(
            App::DocumentObject::getClassTypeId(), nullptr, Gui::ResolveMode::FollowLink) >= 1;
}

//===========================================================================
// Part_CompJoinFeatures (dropdown toolbar button for Connect, Embed and Cutout)
//===========================================================================

DEF_STD_CMD_ACL(CmdPartCompJoinFeatures)

CmdPartCompJoinFeatures::CmdPartCompJoinFeatures()
  : Command("Part_CompJoinFeatures")
{
    sAppModule      = "Part";
    sGroup          = QT_TR_NOOP("Part");
    sMenuText       = QT_TR_NOOP("Join objects...");
    sToolTipText    = QT_TR_NOOP("Join walled objects");
    sWhatsThis      = "Part_CompJoinFeatures";
    sStatusTip      = sToolTipText;
}

void CmdPartCompJoinFeatures::activated(int iMsg)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    if (iMsg==0)
        rcCmdMgr.runCommandByName("Part_JoinConnect");
    else if (iMsg==1)
        rcCmdMgr.runCommandByName("Part_JoinEmbed");
    else if (iMsg==2)
        rcCmdMgr.runCommandByName("Part_JoinCutout");
    else
        return;

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdPartCompJoinFeatures::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* cmd0 = pcAction->addAction(QString());
    cmd0->setIcon(Gui::BitmapFactory().iconFromTheme("Part_JoinConnect"));
    QAction* cmd1 = pcAction->addAction(QString());
    cmd1->setIcon(Gui::BitmapFactory().iconFromTheme("Part_JoinEmbed"));
    QAction* cmd2 = pcAction->addAction(QString());
    cmd2->setIcon(Gui::BitmapFactory().iconFromTheme("Part_JoinCutout"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(cmd0->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdPartCompJoinFeatures::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;

    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    Gui::Command* joinConnect = rcCmdMgr.getCommandByName("Part_JoinConnect");
    if (joinConnect) {
        QAction* cmd0 = a[0];
        cmd0->setText(QApplication::translate("Part_JoinFeatures", joinConnect->getMenuText()));
        cmd0->setToolTip(QApplication::translate("Part_JoinFeatures", joinConnect->getToolTipText()));
        cmd0->setStatusTip(QApplication::translate("Part_JoinFeatures", joinConnect->getStatusTip()));
    }

    Gui::Command* joinEmbed = rcCmdMgr.getCommandByName("Part_JoinEmbed");
    if (joinEmbed) {
        QAction* cmd1 = a[1];
        cmd1->setText(QApplication::translate("Part_JoinFeatures", joinEmbed->getMenuText()));
        cmd1->setToolTip(QApplication::translate("Part_JoinFeatures", joinEmbed->getToolTipText()));
        cmd1->setStatusTip(QApplication::translate("Part_JoinFeatures", joinEmbed->getStatusTip()));
    }

    Gui::Command* joinCutout = rcCmdMgr.getCommandByName("Part_JoinCutout");
    if (joinCutout) {
        QAction* cmd2 = a[2];
        cmd2->setText(QApplication::translate("Part_JoinFeatures", joinCutout->getMenuText()));
        cmd2->setToolTip(QApplication::translate("Part_JoinFeatures", joinCutout->getToolTipText()));
        cmd2->setStatusTip(QApplication::translate("Part_JoinFeatures", joinCutout->getStatusTip()));
    }
}

bool CmdPartCompJoinFeatures::isActive()
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Part_CompSplitFeatures (dropdown toolbar button for BooleanFragments, Slice)
//===========================================================================

DEF_STD_CMD_ACL(CmdPartCompSplitFeatures)

CmdPartCompSplitFeatures::CmdPartCompSplitFeatures()
  : Command("Part_CompSplitFeatures")
{
    sAppModule      = "Part";
    sGroup          = QT_TR_NOOP("Part");
    sMenuText       = QT_TR_NOOP("Split objects...");
    sToolTipText    = QT_TR_NOOP("Shape splitting and Compsolid creation tools.");
    sWhatsThis      = "Part_CompSplitFeatures";
    sStatusTip      = sToolTipText;
}

void CmdPartCompSplitFeatures::activated(int iMsg)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    if (iMsg==0)
        rcCmdMgr.runCommandByName("Part_BooleanFragments");
    else if (iMsg==1)
        rcCmdMgr.runCommandByName("Part_SliceApart");
    else if (iMsg==2)
        rcCmdMgr.runCommandByName("Part_Slice");
    else if (iMsg==3)
        rcCmdMgr.runCommandByName("Part_XOR");
    else
        return;

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdPartCompSplitFeatures::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* cmd0 = pcAction->addAction(QString());
    cmd0->setIcon(Gui::BitmapFactory().iconFromTheme("Part_BooleanFragments"));
    QAction* cmd1 = pcAction->addAction(QString());
    cmd1->setIcon(Gui::BitmapFactory().iconFromTheme("Part_SliceApart"));
    QAction* cmd2 = pcAction->addAction(QString());
    cmd2->setIcon(Gui::BitmapFactory().iconFromTheme("Part_Slice"));
    QAction* cmd3 = pcAction->addAction(QString());
    cmd3->setIcon(Gui::BitmapFactory().iconFromTheme("Part_XOR"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(cmd0->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdPartCompSplitFeatures::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;

    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    Gui::Command* splitBoolFragments = rcCmdMgr.getCommandByName("Part_BooleanFragments");
    if (splitBoolFragments) {
        QAction* cmd0 = a[0];
        cmd0->setText(QApplication::translate("Part_SplitFeatures", splitBoolFragments->getMenuText()));
        cmd0->setToolTip(QApplication::translate("Part_SplitFeatures", splitBoolFragments->getToolTipText()));
        cmd0->setStatusTip(QApplication::translate("Part_SplitFeatures", splitBoolFragments->getStatusTip()));
    }

    Gui::Command* splitSliceApart = rcCmdMgr.getCommandByName("Part_SliceApart");
    if (splitSliceApart) {
        QAction* cmd1 = a[1];
        cmd1->setText(QApplication::translate("Part_SplitFeatures", splitSliceApart->getMenuText()));
        cmd1->setToolTip(QApplication::translate("Part_SplitFeatures", splitSliceApart->getToolTipText()));
        cmd1->setStatusTip(QApplication::translate("Part_SplitFeatures", splitSliceApart->getStatusTip()));
    }

    Gui::Command* splitSlice = rcCmdMgr.getCommandByName("Part_Slice");
    if (splitSlice) {
        QAction* cmd1 = a[2];
        cmd1->setText(QApplication::translate("Part_SplitFeatures", splitSlice->getMenuText()));
        cmd1->setToolTip(QApplication::translate("Part_SplitFeatures", splitSlice->getToolTipText()));
        cmd1->setStatusTip(QApplication::translate("Part_SplitFeatures", splitSlice->getStatusTip()));
    }

    Gui::Command* splitXOR = rcCmdMgr.getCommandByName("Part_XOR");
    if (splitXOR) {
        QAction* cmd2 = a[3];
        cmd2->setText(QApplication::translate("Part_SplitFeatures", splitXOR->getMenuText()));
        cmd2->setToolTip(QApplication::translate("Part_SplitFeatures", splitXOR->getToolTipText()));
        cmd2->setStatusTip(QApplication::translate("Part_SplitFeatures", splitXOR->getStatusTip()));
    }
}

bool CmdPartCompSplitFeatures::isActive()
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Part_CompCompoundTools (dropdown toolbar button for BooleanFragments, Slice)
//===========================================================================

DEF_STD_CMD_ACL(CmdPartCompCompoundTools)

CmdPartCompCompoundTools::CmdPartCompCompoundTools()
  : Command("Part_CompCompoundTools")
{
    sAppModule      = "Part";
    sGroup          = QT_TR_NOOP("Part");
    sMenuText       = QT_TR_NOOP("Compound tools");
    sToolTipText    = QT_TR_NOOP("Compound tools: working with lists of shapes.");
    sWhatsThis      = "Part_CompCompoundTools";
    sStatusTip      = sToolTipText;
}

void CmdPartCompCompoundTools::activated(int iMsg)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    if (iMsg==0)
        rcCmdMgr.runCommandByName("Part_Compound");
    else if (iMsg==1)
        rcCmdMgr.runCommandByName("Part_ExplodeCompound");
    else if (iMsg==2)
        rcCmdMgr.runCommandByName("Part_CompoundFilter");
    else
        return;

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdPartCompCompoundTools::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* cmd0 = pcAction->addAction(QString());
    cmd0->setIcon(Gui::BitmapFactory().iconFromTheme("Part_Compound"));
    QAction* cmd1 = pcAction->addAction(QString());
    cmd1->setIcon(Gui::BitmapFactory().iconFromTheme("Part_ExplodeCompound"));
    QAction* cmd2 = pcAction->addAction(QString());
    cmd2->setIcon(Gui::BitmapFactory().iconFromTheme("Part_CompoundFilter"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(cmd0->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdPartCompCompoundTools::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;

    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    Gui::Command* cmdCompound = rcCmdMgr.getCommandByName("Part_Compound");
    if (cmdCompound) {
        QAction* cmd0 = a[0];
        cmd0->setText(QApplication::translate("CmdPartCompound", cmdCompound->getMenuText()));
        cmd0->setToolTip(QApplication::translate("CmdPartCompound", cmdCompound->getToolTipText()));
        cmd0->setStatusTip(QApplication::translate("CmdPartCompound", cmdCompound->getStatusTip()));
    }

    Gui::Command* cmdExplode = rcCmdMgr.getCommandByName("Part_ExplodeCompound");
    if (cmdExplode) {
        QAction* cmd1 = a[1];
        cmd1->setText(QApplication::translate("Part_CompoundTools", cmdExplode->getMenuText()));
        cmd1->setToolTip(QApplication::translate("Part_CompoundTools", cmdExplode->getToolTipText()));
        cmd1->setStatusTip(QApplication::translate("Part_CompoundTools", cmdExplode->getStatusTip()));
    }

    Gui::Command* cmdCompoundFilter = rcCmdMgr.getCommandByName("Part_CompoundFilter");
    if (cmdCompoundFilter) {
        QAction* cmd2 = a[2];
        cmd2->setText(QApplication::translate("Part_CompoundTools", cmdCompoundFilter->getMenuText()));
        cmd2->setToolTip(QApplication::translate("Part_CompoundTools", cmdCompoundFilter->getToolTipText()));
        cmd2->setStatusTip(QApplication::translate("Part_CompoundTools", cmdCompoundFilter->getStatusTip()));
    }
}

bool CmdPartCompCompoundTools::isActive()
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}



//===========================================================================
// Part_Compound
//===========================================================================
DEF_STD_CMD_A(CmdPartCompound)

CmdPartCompound::CmdPartCompound()
  :Command("Part_Compound")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Make compound");
    sToolTipText  = QT_TR_NOOP("Make a compound of several shapes");
    sWhatsThis    = "Part_Compound";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Compound";
}

void CmdPartCompound::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    unsigned int n = getSelection().countObjectsOfType(
            App::DocumentObject::getClassTypeId(), nullptr, Gui::ResolveMode::FollowLink);
    if (n < 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select one shape or more, please."));
        return;
    }

    std::string FeatName = getUniqueObjectName("Compound");

    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();
    std::stringstream str;

    // avoid duplicates without changing the order
    std::set<std::string> tempSelNames;
    str << "App.activeDocument()." << FeatName << ".Links = [";
    for (const auto & it : Sel) {
        auto pos = tempSelNames.insert(it.FeatName);
        if (pos.second) {
            str << "App.activeDocument()." << it.FeatName << ",";
        }
    }
    str << "]";

    openCommand(QT_TRANSLATE_NOOP("Command", "Compound"));
    doCommand(Doc,"App.activeDocument().addObject(\"Part::Compound\",\"%s\")",FeatName.c_str());
    runCommand(Doc,str.str().c_str());
    updateActive();
    commitCommand();
}

bool CmdPartCompound::isActive()
{
    return getSelection().countObjectsOfType(
            App::DocumentObject::getClassTypeId(), nullptr, Gui::ResolveMode::FollowLink) >= 1;
}

//===========================================================================
// Part_Section
//===========================================================================
DEF_STD_CMD_A(CmdPartSection)

CmdPartSection::CmdPartSection()
  :Command("Part_Section")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Section");
    sToolTipText  = QT_TR_NOOP("Make a section of two shapes");
    sWhatsThis    = "Part_Section";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Section";
}

void CmdPartSection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> Sel =
        getSelection().getSelectionEx(nullptr, App::DocumentObject::getClassTypeId(), Gui::ResolveMode::FollowLink);
    if (Sel.size() != 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two shapes please."));
        return;
    }

    std::string FeatName = getUniqueObjectName("Section");
    std::string BaseName  = Sel[0].getFeatName();
    std::string ToolName  = Sel[1].getFeatName();

    openCommand(QT_TRANSLATE_NOOP("Command", "Section"));
    doCommand(Doc,"App.activeDocument().addObject(\"Part::Section\",\"%s\")",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Base = App.activeDocument().%s",FeatName.c_str(),BaseName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Tool = App.activeDocument().%s",FeatName.c_str(),ToolName.c_str());
    doCommand(Gui,"Gui.activeDocument().hide('%s')",BaseName.c_str());
    doCommand(Gui,"Gui.activeDocument().hide('%s')",ToolName.c_str());
    doCommand(Gui,"Gui.activeDocument().%s.LineColor = Gui.activeDocument().%s.ShapeColor", FeatName.c_str(),BaseName.c_str());
    updateActive();
    commitCommand();
}

bool CmdPartSection::isActive()
{
    return getSelection().countObjectsOfType(App::DocumentObject::getClassTypeId(), nullptr, Gui::ResolveMode::FollowLink) == 2;
}

//===========================================================================
// CmdPartImport
//===========================================================================
DEF_STD_CMD_A(CmdPartImport)

CmdPartImport::CmdPartImport()
  :Command("Part_Import")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Import CAD...");
    sToolTipText  = QT_TR_NOOP("Imports a CAD file");
    sWhatsThis    = "Part_Import";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Import";
}

void CmdPartImport::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QStringList filter;
    filter << QString::fromLatin1("STEP (*.stp *.step)");
    filter << QString::fromLatin1("STEP with colors (*.stp *.step)");
    filter << QString::fromLatin1("IGES (*.igs *.iges)");
    filter << QString::fromLatin1("IGES with colors (*.igs *.iges)");
    filter << QString::fromLatin1("BREP (*.brp *.brep)");

    QString select;
    QString fn = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(), QString(), QString(), filter.join(QLatin1String(";;")), &select);
    if (!fn.isEmpty()) {
        Gui::WaitCursor wc;
        App::Document* pDoc = getDocument();
        if (!pDoc) // no document
            return;

        fn = Base::Tools::escapeEncodeFilename(fn);
        openCommand(QT_TRANSLATE_NOOP("Command", "Import Part"));
        if (select == filter[1] ||
            select == filter[3]) {
            doCommand(Doc, "import ImportGui");
            doCommand(Doc, "ImportGui.insert(\"%s\",\"%s\")", (const char*)fn.toUtf8(), pDoc->getName());
        }
        else {
            doCommand(Doc, "import Part");
            doCommand(Doc, "Part.insert(\"%s\",\"%s\")", (const char*)fn.toUtf8(), pDoc->getName());
        }
        commitCommand();

        std::list<Gui::MDIView*> views = getActiveGuiDocument()->getMDIViewsOfType(Gui::View3DInventor::getClassTypeId());
        for (auto view : views) {
            view->viewAll();
        }
    }
}

bool CmdPartImport::isActive()
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// CmdPartExport
//===========================================================================
DEF_STD_CMD_A(CmdPartExport)

CmdPartExport::CmdPartExport()
  : Command("Part_Export")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Export CAD...");
    sToolTipText  = QT_TR_NOOP("Exports to a CAD file");
    sWhatsThis    = "Part_Export";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Export";
}

void CmdPartExport::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QStringList filter;
    filter << QString::fromLatin1("STEP (*.stp *.step)");
    filter << QString::fromLatin1("STEP with colors (*.stp *.step)");
    filter << QString::fromLatin1("IGES (*.igs *.iges)");
    filter << QString::fromLatin1("IGES with colors (*.igs *.iges)");
    filter << QString::fromLatin1("BREP (*.brp *.brep)");

    QString select;
    QString fn = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(), QString(), QString(), filter.join(QLatin1String(";;")), &select);
    if (!fn.isEmpty()) {
        App::Document* pDoc = getDocument();
        if (!pDoc) // no document
            return;
        if (select == filter[1] ||
            select == filter[3]) {
            Gui::Application::Instance->exportTo((const char*)fn.toUtf8(),pDoc->getName(),"ImportGui");
        }
        else {
            Gui::Application::Instance->exportTo((const char*)fn.toUtf8(),pDoc->getName(),"Part");
        }
    }
}

bool CmdPartExport::isActive()
{
    return Gui::Selection().countObjectsOfType(App::DocumentObject::getClassTypeId(), nullptr, Gui::ResolveMode::FollowLink) > 0;
}

//===========================================================================
// PartImportCurveNet
//===========================================================================
DEF_STD_CMD_A(CmdPartImportCurveNet)

CmdPartImportCurveNet::CmdPartImportCurveNet()
  :Command("Part_ImportCurveNet")
{
    sAppModule  = "Part";
    sGroup      = QT_TR_NOOP("Part");
    sMenuText   = QT_TR_NOOP("Import curve network...");
    sToolTipText= QT_TR_NOOP("Import a curve network");
    sWhatsThis  = "Part_ImportCurveNet";
    sStatusTip  = sToolTipText;
    sPixmap     = "Part_Box";
}

void CmdPartImportCurveNet::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QStringList filter;
    filter << QString::fromLatin1("%1 (*.stp *.step *.igs *.iges *.brp *.brep)")
                 .arg(QObject::tr("All CAD Files"));
    filter << QString::fromLatin1("STEP (*.stp *.step)");
    filter << QString::fromLatin1("IGES (*.igs *.iges)");
    filter << QString::fromLatin1("BREP (*.brp *.brep)");
    filter << QString::fromLatin1("%1 (*.*)")
                 .arg(QObject::tr("All Files"));

    QString fn = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(), QString(), QString(), filter.join(QLatin1String(";;")));
    if (!fn.isEmpty()) {
        QFileInfo fi; fi.setFile(fn);
        openCommand(QT_TRANSLATE_NOOP("Command", "Part Import Curve Net"));
        doCommand(Doc,"f = App.activeDocument().addObject(\"Part::CurveNet\",\"%s\")", (const char*)fi.baseName().toLatin1());
        doCommand(Doc,"f.FileName = \"%s\"",(const char*)fn.toLatin1());
        commitCommand();
        updateActive();
    }
}

bool CmdPartImportCurveNet::isActive()
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Part_MakeSolid
//===========================================================================
DEF_STD_CMD_A(CmdPartMakeSolid)

CmdPartMakeSolid::CmdPartMakeSolid()
  :Command("Part_MakeSolid")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Convert to solid");
    sToolTipText  = QT_TR_NOOP("Create solid from a shell or compound");
    sWhatsThis    = "Part_MakeSolid";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_MakeSolid";
}

void CmdPartMakeSolid::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<App::DocumentObject*> objs = Gui::Selection().getObjectsOfType
        (App::DocumentObject::getClassTypeId(), nullptr, Gui::ResolveMode::FollowLink);
    runCommand(Doc, "import Part");
    for (auto it : objs) {
        const TopoDS_Shape& shape = Part::Feature::getShape(it);
        if (!shape.IsNull()) {
            TopAbs_ShapeEnum type = shape.ShapeType();
            QString str;
            if (type == TopAbs_SOLID) {
                Base::Console().Message("%s is ignored because it is already a solid.\n",
                    it->Label.getValue());
            }
            else if (type == TopAbs_COMPOUND || type == TopAbs_COMPSOLID) {
                str = QString::fromLatin1(
                    "__s__=App.ActiveDocument.%1.Shape.Faces\n"
                    "__s__=Part.Solid(Part.Shell(__s__))\n"
                    "__o__=App.ActiveDocument.addObject(\"Part::Feature\",\"%1_solid\")\n"
                    "__o__.Label=\"%2 (Solid)\"\n"
                    "__o__.Shape=__s__\n"
                    "del __s__, __o__"
                    )
                    .arg(QLatin1String(it->getNameInDocument()),
                         QLatin1String(it->Label.getValue()));
            }
            else if (type == TopAbs_SHELL) {
                str = QString::fromLatin1(
                    "__s__=App.ActiveDocument.%1.Shape\n"
                    "__s__=Part.Solid(__s__)\n"
                    "__o__=App.ActiveDocument.addObject(\"Part::Feature\",\"%1_solid\")\n"
                    "__o__.Label=\"%2 (Solid)\"\n"
                    "__o__.Shape=__s__\n"
                    "del __s__, __o__"
                    )
                    .arg(QLatin1String(it->getNameInDocument()),
                         QLatin1String(it->Label.getValue()));
            }
            else {
                Base::Console().Message("%s is ignored because it is neither a shell nor a compound.\n",
                    it->Label.getValue());
            }

            try {
                if (!str.isEmpty())
                    runCommand(Doc, str.toLatin1());
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("Cannot convert %s because %s.\n",
                    it->Label.getValue(), e.what());
            }
        }
    }
}

bool CmdPartMakeSolid::isActive()
{
    return Gui::Selection().countObjectsOfType
        (App::DocumentObject::getClassTypeId(), nullptr, Gui::ResolveMode::FollowLink) > 0;
}

//===========================================================================
// Part_ReverseShape
//===========================================================================
DEF_STD_CMD_A(CmdPartReverseShape)

CmdPartReverseShape::CmdPartReverseShape()
  :Command("Part_ReverseShape")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Reverse shapes");
    sToolTipText  = QT_TR_NOOP("Reverse orientation of shapes");
    sWhatsThis    = "Part_ReverseShape";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Reverse_Shape";
}

void CmdPartReverseShape::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<App::DocumentObject*> objs = Gui::Selection().getObjectsOfType
        (App::DocumentObject::getClassTypeId());
    openCommand(QT_TRANSLATE_NOOP("Command", "Reverse"));
    for (auto it : objs) {
        const TopoDS_Shape& shape = Part::Feature::getShape(it);
        if (!shape.IsNull()) {
            std::string name = it->getNameInDocument();
            name += "_rev";
            name = getUniqueObjectName(name.c_str());

            QString str = QString::fromLatin1(
                "__o__=App.ActiveDocument.addObject(\"Part::Reverse\",\"%1\")\n"
                "__o__.Source=App.ActiveDocument.%2\n"
                "__o__.Label=\"%3 (Rev)\"\n"
                "del __o__"
                )
                .arg(QString::fromLatin1(name.c_str()),
                     QString::fromLatin1(it->getNameInDocument()),
                     QString::fromLatin1(it->Label.getValue()));

            try {
                runCommand(Doc, str.toLatin1());
                copyVisual(name.c_str(), "ShapeColor", it->getNameInDocument());
                copyVisual(name.c_str(), "LineColor" , it->getNameInDocument());
                copyVisual(name.c_str(), "PointColor", it->getNameInDocument());
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("Cannot convert %s because %s.\n",
                    it->Label.getValue(), e.what());
            }
        }
    }

    commitCommand();
    updateActive();
}

bool CmdPartReverseShape::isActive()
{
    return PartGui::hasShapesInSelection();
}

//===========================================================================
// Part_Boolean
//===========================================================================
DEF_STD_CMD_A(CmdPartBoolean)

CmdPartBoolean::CmdPartBoolean()
  :Command("Part_Boolean")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Boolean...");
    sToolTipText  = QT_TR_NOOP("Run a boolean operation with two shapes selected");
    sWhatsThis    = "Part_Boolean";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Booleans";
}

void CmdPartBoolean::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg)
        dlg = new PartGui::TaskBooleanOperation();
    Gui::Control().showDialog(dlg);
}

bool CmdPartBoolean::isActive()
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Extrude
//===========================================================================
DEF_STD_CMD_A(CmdPartExtrude)

CmdPartExtrude::CmdPartExtrude()
  :Command("Part_Extrude")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Extrude...");
    sToolTipText  = QT_TR_NOOP("Extrude a selected sketch");
    sWhatsThis    = "Part_Extrude";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Extrude";
}

void CmdPartExtrude::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new PartGui::TaskExtrusion());
}

bool CmdPartExtrude::isActive()
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Scale
//===========================================================================
DEF_STD_CMD_A(CmdPartScale)

CmdPartScale::CmdPartScale()
  :Command("Part_Scale")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Scale...");
    sToolTipText  = QT_TR_NOOP("Scale a selected shape");
    sWhatsThis    = "Part_Scale";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Scale";
}

void CmdPartScale::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::Control().showDialog(new PartGui::TaskScale());
}

bool CmdPartScale::isActive()
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_MakeFace
//===========================================================================
DEF_STD_CMD_A(CmdPartMakeFace)

CmdPartMakeFace::CmdPartMakeFace()
  : Command("Part_MakeFace")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Make face from wires");
    sToolTipText  = QT_TR_NOOP("Make face from set of wires (e.g. from a sketch)");
    sWhatsThis    = "Part_MakeFace";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_MakeFace";
}

void CmdPartMakeFace::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto sketches = Gui::Selection().getObjectsOfType(App::DocumentObject::getClassTypeId(), nullptr, Gui::ResolveMode::FollowLink);
    if(sketches.empty())
        return;
    openCommand(QT_TRANSLATE_NOOP("Command", "Make face"));

    try {
        App::DocumentT doc(sketches.front()->getDocument());
        std::stringstream str;
        str << doc.getDocumentPython()
            << R"(.addObject("Part::Face", "Face").Sources = ()";
        for (auto &obj : sketches) {
            str << App::DocumentObjectT(obj).getObjectPython() << ", ";
        }

        str << ")";

        runCommand(Doc,str.str().c_str());
        commitCommand();
        updateActive();
    }
    catch (...) {
        abortCommand();
        throw;
    }
}

bool CmdPartMakeFace::isActive()
{
    return (Gui::Selection().countObjectsOfType(App::DocumentObject::getClassTypeId(), nullptr, Gui::ResolveMode::FollowLink) > 0 &&
            !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Revolve
//===========================================================================
DEF_STD_CMD_A(CmdPartRevolve)

CmdPartRevolve::CmdPartRevolve()
  :Command("Part_Revolve")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Revolve...");
    sToolTipText  = QT_TR_NOOP("Revolve a selected shape");
    sWhatsThis    = "Part_Revolve";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Revolve";
}

void CmdPartRevolve::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new PartGui::TaskRevolution());
}

bool CmdPartRevolve::isActive()
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Fillet
//===========================================================================
DEF_STD_CMD_A(CmdPartFillet)

CmdPartFillet::CmdPartFillet()
  :Command("Part_Fillet")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Fillet...");
    sToolTipText  = QT_TR_NOOP("Fillet the selected edges of a shape");
    sWhatsThis    = "Part_Fillet";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Fillet";
}

void CmdPartFillet::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new PartGui::TaskFilletEdges(nullptr));
}

bool CmdPartFillet::isActive()
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Chamfer
//===========================================================================
DEF_STD_CMD_A(CmdPartChamfer)

CmdPartChamfer::CmdPartChamfer()
  :Command("Part_Chamfer")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Chamfer...");
    sToolTipText  = QT_TR_NOOP("Chamfer the selected edges of a shape");
    sWhatsThis    = "Part_Chamfer";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Chamfer";
}

void CmdPartChamfer::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new PartGui::TaskChamferEdges(nullptr));
}

bool CmdPartChamfer::isActive()
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Mirror
//===========================================================================
DEF_STD_CMD_A(CmdPartMirror)

CmdPartMirror::CmdPartMirror()
  :Command("Part_Mirror")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Mirroring...");
    sToolTipText  = QT_TR_NOOP("Mirroring a selected shape");
    sWhatsThis    = "Part_Mirror";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Mirror";
}

void CmdPartMirror::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new PartGui::TaskMirroring());
}

bool CmdPartMirror::isActive()
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_CrossSections
//===========================================================================
DEF_STD_CMD_A(CmdPartCrossSections)

CmdPartCrossSections::CmdPartCrossSections()
  :Command("Part_CrossSections")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Cross-sections...");
    sToolTipText  = QT_TR_NOOP("Cross-sections");
    sWhatsThis    = "Part_CrossSections";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_CrossSections";
}

void CmdPartCrossSections::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg) {
        std::vector<Part::TopoShape> shapes = PartGui::getShapesFromSelection();
        Base::BoundBox3d bbox;
        for (const auto & it : shapes) {
            bbox.Add(it.getBoundBox());
        }
        dlg = new PartGui::TaskCrossSections(bbox);
    }
    Gui::Control().showDialog(dlg);
}

bool CmdPartCrossSections::isActive()
{
    bool hasShapes = PartGui::hasShapesInSelection();
    return (hasShapes && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Builder
//===========================================================================

DEF_STD_CMD_A(CmdPartBuilder)

CmdPartBuilder::CmdPartBuilder()
  :Command("Part_Builder")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Shape builder...");
    sToolTipText  = QT_TR_NOOP("Advanced utility to create shapes");
    sWhatsThis    = "Part_Builder";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Shapebuilder";
}

void CmdPartBuilder::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new PartGui::TaskShapeBuilder());
}

bool CmdPartBuilder::isActive()
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Loft
//===========================================================================

DEF_STD_CMD_A(CmdPartLoft)

CmdPartLoft::CmdPartLoft()
  : Command("Part_Loft")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Loft...");
    sToolTipText  = QT_TR_NOOP("Utility to loft");
    sWhatsThis    = "Part_Loft";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Loft";
}

void CmdPartLoft::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new PartGui::TaskLoft());
}

bool CmdPartLoft::isActive()
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Sweep
//===========================================================================

DEF_STD_CMD_A(CmdPartSweep)

CmdPartSweep::CmdPartSweep()
  : Command("Part_Sweep")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Sweep...");
    sToolTipText  = QT_TR_NOOP("Utility to sweep");
    sWhatsThis    = "Part_Sweep";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Sweep";
}

void CmdPartSweep::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new PartGui::TaskSweep());
}

bool CmdPartSweep::isActive()
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Offset
//===========================================================================

DEF_STD_CMD_A(CmdPartOffset)

CmdPartOffset::CmdPartOffset()
  : Command("Part_Offset")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("3D Offset...");
    sToolTipText  = QT_TR_NOOP("Utility to offset in 3D");
    sWhatsThis    = "Part_Offset";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Offset";
}

void CmdPartOffset::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<App::DocumentObject*> docobjs = Gui::Selection().getObjectsOfType(App::DocumentObject::getClassTypeId());
    std::vector<App::DocumentObject*> shapes;
    for (auto it : docobjs) {
        if (!Part::Feature::getTopoShape(it).isNull()) {
           shapes.push_back(it);
        }
    }
    if (shapes.size() != 1) {
        return;
    }
    App::DocumentObject* shape = shapes.front();
    std::string offset = getUniqueObjectName("Offset");

    openCommand(QT_TRANSLATE_NOOP("Command", "Make Offset"));
    doCommand(Doc,"App.ActiveDocument.addObject(\"Part::Offset\",\"%s\")",offset.c_str());
    doCommand(Doc,"App.ActiveDocument.%s.Source = App.ActiveDocument.%s" ,offset.c_str(), shape->getNameInDocument());
    doCommand(Doc,"App.ActiveDocument.%s.Value = 1.0",offset.c_str());
    updateActive();

    doCommand(Gui,"Gui.ActiveDocument.setEdit('%s')",offset.c_str());

    adjustCameraPosition();

    copyVisual(offset.c_str(), "ShapeColor", shape->getNameInDocument());
    copyVisual(offset.c_str(), "LineColor" , shape->getNameInDocument());
    copyVisual(offset.c_str(), "PointColor", shape->getNameInDocument());
}

bool CmdPartOffset::isActive()
{
     bool hasShapes = PartGui::hasShapesInSelection();
     std::vector<App::DocumentObject*> docobjs = Gui::Selection().getObjectsOfType(App::DocumentObject::getClassTypeId());
     return (hasShapes && !Gui::Control().activeDialog() && docobjs.size() == 1);
}


//===========================================================================
// Part_Offset2D
//===========================================================================

DEF_STD_CMD_A(CmdPartOffset2D)

CmdPartOffset2D::CmdPartOffset2D()
  : Command("Part_Offset2D")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("2D Offset...");
    sToolTipText  = QT_TR_NOOP("Utility to offset planar shapes");
    sWhatsThis    = "Part_Offset2D";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Offset2D";
}

void CmdPartOffset2D::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<App::DocumentObject*> docobjs = Gui::Selection().getObjectsOfType(App::DocumentObject::getClassTypeId());
    std::vector<App::DocumentObject*> shapes;

    for (auto it : docobjs) {
        if (!Part::Feature::getTopoShape(it).isNull()) {
           shapes.push_back(it);
        }
    }
    if (shapes.size() != 1) {
        return;
    }
    App::DocumentObject* shape = shapes.front();
    std::string offset = getUniqueObjectName("Offset2D");

    openCommand(QT_TRANSLATE_NOOP("Command", "Make 2D Offset"));
    doCommand(Doc,"App.ActiveDocument.addObject(\"Part::Offset2D\",\"%s\")",offset.c_str());
    doCommand(Doc,"App.ActiveDocument.%s.Source = App.ActiveDocument.%s" ,offset.c_str(), shape->getNameInDocument());
    doCommand(Doc,"App.ActiveDocument.%s.Value = 1.0",offset.c_str());
    updateActive();
    doCommand(Gui,"Gui.ActiveDocument.setEdit('%s')",offset.c_str());
    adjustCameraPosition();

    copyVisual(offset.c_str(), "ShapeColor", shape->getNameInDocument());
    copyVisual(offset.c_str(), "LineColor" , shape->getNameInDocument());
    copyVisual(offset.c_str(), "PointColor", shape->getNameInDocument());
}

bool CmdPartOffset2D::isActive()
{
    bool hasShapes = PartGui::hasShapesInSelection();
    std::vector<App::DocumentObject*> docobjs = Gui::Selection().getObjectsOfType(App::DocumentObject::getClassTypeId());
    return (hasShapes && !Gui::Control().activeDialog() && docobjs.size() == 1);
}

//===========================================================================
// Part_CompOffset (dropdown toolbar button for Offset features)
//===========================================================================

DEF_STD_CMD_ACL(CmdPartCompOffset)

CmdPartCompOffset::CmdPartCompOffset()
  : Command("Part_CompOffset")
{
    sAppModule      = "Part";
    sGroup          = QT_TR_NOOP("Part");
    sMenuText       = QT_TR_NOOP("Offset:");
    sToolTipText    = QT_TR_NOOP("Tools to offset shapes (construct parallel shapes)");
    sWhatsThis      = "Part_CompOffset";
    sStatusTip      = sToolTipText;
}

void CmdPartCompOffset::activated(int iMsg)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    if (iMsg==0)
        rcCmdMgr.runCommandByName("Part_Offset");
    else if (iMsg==1)
        rcCmdMgr.runCommandByName("Part_Offset2D");
    else
        return;

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdPartCompOffset::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* cmd0 = pcAction->addAction(QString());
    cmd0->setIcon(Gui::BitmapFactory().iconFromTheme("Part_Offset"));
    QAction* cmd1 = pcAction->addAction(QString());
    cmd1->setIcon(Gui::BitmapFactory().iconFromTheme("Part_Offset2D"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(cmd0->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdPartCompOffset::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;

    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    Gui::Command* cmdOffset = rcCmdMgr.getCommandByName("Part_Offset");
    if (cmdOffset) {
        QAction* cmd0 = a[0];
        cmd0->setText(QApplication::translate(cmdOffset->className(), cmdOffset->getMenuText()));
        cmd0->setToolTip(QApplication::translate(cmdOffset->className(), cmdOffset->getToolTipText()));
        cmd0->setStatusTip(QApplication::translate(cmdOffset->className(), cmdOffset->getStatusTip()));
    }

    Gui::Command* cmdOffset2D = rcCmdMgr.getCommandByName("Part_Offset2D");
    if (cmdOffset2D) {
        QAction* cmd1 = a[1];
        cmd1->setText(QApplication::translate(cmdOffset2D->className(), cmdOffset2D->getMenuText()));
        cmd1->setToolTip(QApplication::translate(cmdOffset2D->className(), cmdOffset2D->getToolTipText()));
        cmd1->setStatusTip(QApplication::translate(cmdOffset2D->className(), cmdOffset2D->getStatusTip()));
    }
}

bool CmdPartCompOffset::isActive()
{
    bool hasShapes = PartGui::hasShapesInSelection();
    std::vector<App::DocumentObject*> docobjs = Gui::Selection().getObjectsOfType(App::DocumentObject::getClassTypeId());
    return (hasShapes && !Gui::Control().activeDialog() && docobjs.size() == 1);
}
//===========================================================================
// Part_Thickness
//===========================================================================

DEF_STD_CMD_A(CmdPartThickness)

CmdPartThickness::CmdPartThickness()
  : Command("Part_Thickness")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Thickness...");
    sToolTipText  = QT_TR_NOOP("Utility to apply a thickness");
    sWhatsThis    = "Part_Thickness";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Thickness";
}

void CmdPartThickness::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    const App::DocumentObject* obj = nullptr;
    std::string selection;
    const std::vector<Gui::SelectionObject> selobjs = Gui::Selection().getSelectionEx();
    std::vector<Part::TopoShape> subShapes;
    Part::TopoShape topoShape = Part::TopoShape();

    bool ok = true;
    if (selobjs.size() == 1) {
        selection = selobjs[0].getAsPropertyLinkSubString();
        const std::vector<std::string>& subnames = selobjs[0].getSubNames();
        obj = selobjs[0].getObject();
        topoShape = Part::Feature::getTopoShape(obj);
        if (!topoShape.isNull()) {
            for (std::vector<std::string>::const_iterator it = subnames.begin(); it != subnames.end(); ++it) {
                subShapes.emplace_back(topoShape.getSubShape(subnames[0].c_str()));
            }
            for (const auto & it : subShapes) {
                TopoDS_Shape dsShape = it.getShape();
                if (dsShape.IsNull() || dsShape.ShapeType() != TopAbs_FACE) { //only face selection allowed
                    ok = false;
                }
            }
        } else { //could be not a part::feature or app:link to non-part::feature or app::part without a visible part::feature
            ok = false;
        }

    } else { //not just one object selected
        ok = false;
    }

    int countSolids = 0;
    TopExp_Explorer xp;
    if (!topoShape.isNull()){
        xp.Init(topoShape.getShape(), TopAbs_SOLID);
        for (;xp.More(); xp.Next()) {
            countSolids++;
        }
    }
    if (countSolids != 1 || !ok) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QApplication::translate("CmdPartThickness", "Wrong selection"),
                             QApplication::translate("CmdPartThickness", "Selected shape is not a solid"));
        return;
    }

    std::string thick = getUniqueObjectName("Thickness");

    openCommand(QT_TRANSLATE_NOOP("Command", "Make Thickness"));
    doCommand(Doc,"App.ActiveDocument.addObject(\"Part::Thickness\",\"%s\")",thick.c_str());
    doCommand(Doc,"App.ActiveDocument.%s.Faces = %s" ,thick.c_str(), selection.c_str());
    doCommand(Doc,"App.ActiveDocument.%s.Value = 1.0",thick.c_str());
    updateActive();
    if (isActiveObjectValid()) {
        doCommand(App,"App.getDocument(\"%s\").getObject(\"%s\").ViewObject.Visibility = False",
                  obj->getDocument()->getName(), obj->getNameInDocument());
    }
    doCommand(Gui,"Gui.ActiveDocument.setEdit('%s')",thick.c_str());
    adjustCameraPosition();

    copyVisual(thick.c_str(), "ShapeColor", obj->getNameInDocument());
    copyVisual(thick.c_str(), "LineColor" , obj->getNameInDocument());
    copyVisual(thick.c_str(), "PointColor", obj->getNameInDocument());
}

bool CmdPartThickness::isActive()
{
    Base::Type partid = Base::Type::fromName("Part::Feature");
    bool objectsSelected = Gui::Selection().countObjectsOfType(partid, nullptr, Gui::ResolveMode::FollowLink) > 0;
    return (objectsSelected && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_ShapeInfo
//===========================================================================

DEF_STD_CMD_A(CmdShapeInfo)

CmdShapeInfo::CmdShapeInfo()
  :Command("Part_ShapeInfo")
{
    sAppModule    = "Part";
    sGroup        = "Part";
    sMenuText     = "Shape info...";
    sToolTipText  = "Info about shape";
    sWhatsThis    = "Part_ShapeInfo";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_ShapeInfo";
}

void CmdShapeInfo::activated(int iMsg)
{
    Q_UNUSED(iMsg);
}

bool CmdShapeInfo::isActive()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc || doc->countObjectsOfType(Part::Feature::getClassTypeId()) == 0)
        return false;

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//===========================================================================
// Part_RuledSurface
//===========================================================================

DEF_STD_CMD_A(CmdPartRuledSurface)

CmdPartRuledSurface::CmdPartRuledSurface()
  : Command("Part_RuledSurface")
{
    sAppModule      = "Part";
    sGroup          = QT_TR_NOOP("Part");
    sMenuText       = QT_TR_NOOP("Create ruled surface");
    sToolTipText    = QT_TR_NOOP("Create a ruled surface from either two Edges or two wires");
    sWhatsThis      = "Part_RuledSurface";
    sStatusTip      = sToolTipText;
    sPixmap         = "Part_RuledSurface";
}

void CmdPartRuledSurface::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool ok = true;
    TopoDS_Shape curve1, curve2;
    std::string link1, link2, obj1, obj2;
    const std::vector<Gui::SelectionObject> selobjs = Gui::Selection().getSelectionEx();
    const App::DocumentObject* docobj1 = nullptr;
    const App::DocumentObject* docobj2 = nullptr;

    if (selobjs.size() != 1 && selobjs.size() != 2) {
        ok = false;
    }

    if (ok && selobjs.size() <= 2) {
        if (!selobjs.empty()) {
            const std::vector<std::string>& subnames1= selobjs[0].getSubNames();
            docobj1 = selobjs[0].getObject();
            obj1 = docobj1->getNameInDocument();
            obj2 = obj1; //changed later if 2 objects were selected
            const Part::TopoShape& shape1 = Part::Feature::getTopoShape(docobj1);
            if (shape1.isNull()) {
                ok = false;
            }
            if (ok && subnames1.size() <= 2) {
                if (!subnames1.empty()) {
                    curve1 = Part::Feature::getTopoShape(docobj1, subnames1[0].c_str(), true /*need element*/).getShape();
                    link1 = subnames1[0];
                }
                if (subnames1.size() == 2) {
                    curve2 = Part::Feature::getTopoShape(docobj1, subnames1[1].c_str(), true /*need element*/).getShape();
                    link2 = subnames1[1];
                }
                if (subnames1.empty()) {
                    curve1 = shape1.getShape();
                }
            } else {
                ok = false;
            }
        }
        if (selobjs.size() == 2) {
            const std::vector<std::string>& subnames2 = selobjs[1].getSubNames();
            docobj2 = selobjs[1].getObject();
            obj2 = docobj2->getNameInDocument();

            const Part::TopoShape& shape2 = Part::Feature::getTopoShape(docobj2);
            if (shape2.isNull()) {
                ok = false;
            }
            if (ok && subnames2.size() == 1) {
                curve2 = Part::Feature::getTopoShape(docobj2, subnames2[0].c_str(), true /*need element*/).getShape();
                link2 = subnames2[0];
            } else {
                if (subnames2.empty()) {
                    curve2 = shape2.getShape();
                }
            }
        }
        if (!curve1.IsNull() && !curve2.IsNull()) {
            if ((curve1.ShapeType() == TopAbs_EDGE || curve1.ShapeType() == TopAbs_WIRE)
                    &&  (curve2.ShapeType() == TopAbs_EDGE || curve2.ShapeType() == TopAbs_WIRE)) {
                ok = true;
            }
        }
    }

    if (!ok) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("You have to select either two edges or two wires."));
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Create ruled surface"));
    doCommand(Doc, "FreeCAD.ActiveDocument.addObject('Part::RuledSurface', 'Ruled Surface')");
    doCommand(Doc, "FreeCAD.ActiveDocument.ActiveObject.Curve1=(FreeCAD.ActiveDocument.%s,['%s'])"
              ,obj1.c_str(), link1.c_str());
    doCommand(Doc, "FreeCAD.ActiveDocument.ActiveObject.Curve2=(FreeCAD.ActiveDocument.%s,['%s'])"
              ,obj2.c_str(), link2.c_str());
    commitCommand();
    updateActive();
}

bool CmdPartRuledSurface::isActive()
{
    return getActiveGuiDocument();
}

//===========================================================================
// Part_CheckGeometry
//===========================================================================

DEF_STD_CMD_A(CmdCheckGeometry)

CmdCheckGeometry::CmdCheckGeometry()
  : Command("Part_CheckGeometry")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Check Geometry");
    sToolTipText  = QT_TR_NOOP("Analyzes Geometry For Errors");
    sWhatsThis    = "Part_CheckGeometry";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_CheckGeometry";
}

void CmdCheckGeometry::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg)
        dlg = new PartGui::TaskCheckGeometryDialog();
    Gui::Control().showDialog(dlg);
}

bool CmdCheckGeometry::isActive()
{
    bool hasShapes = PartGui::hasShapesInSelection();
    return (hasShapes && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_ColorPerFace
//===========================================================================

DEF_STD_CMD_A(CmdColorPerFace)

CmdColorPerFace::CmdColorPerFace()
  : Command("Part_ColorPerFace")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Color per face");
    sToolTipText  = QT_TR_NOOP("Set the color of each individual face "
                               "of the selected object.");
    sStatusTip    = sToolTipText;
    sWhatsThis    = "Part_ColorPerFace";
    sPixmap       = "Part_ColorFace";
}

void CmdColorPerFace::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (getActiveGuiDocument()->getInEdit())
        getActiveGuiDocument()->resetEdit();
    std::vector<App::DocumentObject*> sel = Gui::Selection().getObjectsOfType(Part::Feature::getClassTypeId());
    if (sel.empty())
        return;
    PartGui::ViewProviderPartExt* vp = dynamic_cast<PartGui::ViewProviderPartExt*>(Gui::Application::Instance->getViewProvider(sel.front()));
    if (vp)
        vp->changeFaceColors();
}

bool CmdColorPerFace::isActive()
{
    Base::Type partid = Base::Type::fromName("Part::Feature");
    bool objectSelected = Gui::Selection().countObjectsOfType(partid) == 1;
    return (hasActiveDocument() && !Gui::Control().activeDialog() && objectSelected);
}

//===========================================================================
// Part_Measure_Linear
//===========================================================================

DEF_STD_CMD_A(CmdMeasureLinear)

CmdMeasureLinear::CmdMeasureLinear()
  : Command("Part_Measure_Linear")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Measure Linear");
    sToolTipText  = QT_TR_NOOP("Measure the linear distance between two points;\n"
                               "if edges or faces are picked, it will measure\n"
                               "between two vertices of them.");
    sWhatsThis    = "Part_Measure_Linear";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Measure_Linear";
}

void CmdMeasureLinear::activated(int iMsg)
{
  Q_UNUSED(iMsg);
  PartGui::goDimensionLinearRoot();
}

bool CmdMeasureLinear::isActive()
{
  return hasActiveDocument();
}

//===========================================================================
// Part_Measure_Angular
//===========================================================================

DEF_STD_CMD_A(CmdMeasureAngular)

CmdMeasureAngular::CmdMeasureAngular()
  : Command("Part_Measure_Angular")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Measure Angular");
    sToolTipText  = QT_TR_NOOP("Measure the angle between two edges.");
    sWhatsThis    = "Part_Measure_Angular";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Measure_Angular";
}

void CmdMeasureAngular::activated(int iMsg)
{
  Q_UNUSED(iMsg);
  PartGui::goDimensionAngularRoot();
}

bool CmdMeasureAngular::isActive()
{
  return hasActiveDocument();
}

//===========================================================================
// Part_Measure_Refresh
//===========================================================================

DEF_STD_CMD_A(CmdMeasureRefresh)

CmdMeasureRefresh::CmdMeasureRefresh()
  : Command("Part_Measure_Refresh")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Refresh");
    sToolTipText  = QT_TR_NOOP("Recalculate the dimensions\n"
                               "if the measured points have moved.");
    sWhatsThis    = "Part_Measure_Refresh";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Measure_Refresh";
}

void CmdMeasureRefresh::activated(int iMsg)
{
  Q_UNUSED(iMsg);
  PartGui::refreshDimensions();
}

bool CmdMeasureRefresh::isActive()
{
  return hasActiveDocument();
}

//===========================================================================
// Part_Measure_Clear_All
//===========================================================================

DEF_STD_CMD_A(CmdMeasureClearAll)

CmdMeasureClearAll::CmdMeasureClearAll()
  : Command("Part_Measure_Clear_All")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Clear All");
    sToolTipText  = QT_TR_NOOP("Clear all dimensions from the screen.");
    sWhatsThis    = "Part_Measure_Clear_All";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Measure_Clear_All";
}

void CmdMeasureClearAll::activated(int iMsg)
{
  Q_UNUSED(iMsg);
  PartGui::eraseAllDimensions();
}

bool CmdMeasureClearAll::isActive()
{
  return hasActiveDocument();
}

//===========================================================================
// Part_Measure_Toggle_All
//===========================================================================

DEF_STD_CMD_A(CmdMeasureToggleAll)

CmdMeasureToggleAll::CmdMeasureToggleAll()
  : Command("Part_Measure_Toggle_All")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Toggle All");
    sToolTipText  = QT_TR_NOOP("Toggle on and off "
                               "all currently visible dimensions,\n"
                               "direct, orthogonal, and angular.");
    sWhatsThis    = "Part_Measure_Toggle_All";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Measure_Toggle_All";
}

void CmdMeasureToggleAll::activated(int iMsg)
{
  Q_UNUSED(iMsg);
  ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("View");
  bool visibility = group->GetBool("DimensionsVisible", true);
  if (visibility)
    group->SetBool("DimensionsVisible", false);
  else
    group->SetBool("DimensionsVisible", true);
}

bool CmdMeasureToggleAll::isActive()
{
  return hasActiveDocument();
}

//===========================================================================
// Part_Measure_Toggle_3D
//===========================================================================

DEF_STD_CMD_A(CmdMeasureToggle3d)

CmdMeasureToggle3d::CmdMeasureToggle3d()
  : Command("Part_Measure_Toggle_3D")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Toggle 3D");
    sToolTipText  = QT_TR_NOOP("Toggle on and off "
                               "all direct dimensions,\n"
                               "including angular.");
    sWhatsThis    = "Part_Measure_Toggle_3D";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Measure_Toggle_3D";
}

void CmdMeasureToggle3d::activated(int iMsg)
{
  Q_UNUSED(iMsg);
  PartGui::toggle3d();
}

bool CmdMeasureToggle3d::isActive()
{
  return hasActiveDocument();
}

//===========================================================================
// Part_Measure_Toggle_Delta
//===========================================================================

DEF_STD_CMD_A(CmdMeasureToggleDelta)

CmdMeasureToggleDelta::CmdMeasureToggleDelta()
  : Command("Part_Measure_Toggle_Delta")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Toggle Delta");
    sToolTipText  = QT_TR_NOOP("Toggle on and off "
                               "all orthogonal dimensions,\n"
                               "meaning that a direct dimension will be decomposed\n"
                               "into its X, Y, and Z components.");
    sWhatsThis    = "Part_Measure_Toggle_Delta";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Measure_Toggle_Delta";
}

void CmdMeasureToggleDelta::activated(int iMsg)
{
  Q_UNUSED(iMsg);
  PartGui::toggleDelta();
}

bool CmdMeasureToggleDelta::isActive()
{
  return hasActiveDocument();
}

//===========================================================================
// Part_BoxSelection
//===========================================================================

DEF_STD_CMD_A(CmdBoxSelection)

CmdBoxSelection::CmdBoxSelection()
  : Command("Part_BoxSelection")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Box selection");
    sToolTipText  = QT_TR_NOOP("Box selection");
    sWhatsThis    = "Part_BoxSelection";
    sStatusTip    = QT_TR_NOOP("Box selection");
    sPixmap       = "Part_BoxSelection";
}

void CmdBoxSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    PartGui::BoxSelection* sel = new PartGui::BoxSelection();
    sel->setAutoDelete(true);
    sel->start(TopAbs_FACE);
}

bool CmdBoxSelection::isActive()
{
    return hasActiveDocument();
}

//===========================================================================
// Part_ProjectionOnSurface
//===========================================================================
DEF_STD_CMD_A(CmdPartProjectionOnSurface)

CmdPartProjectionOnSurface::CmdPartProjectionOnSurface()
    :Command("Part_ProjectionOnSurface")
{
    sAppModule = "Part";
    sGroup = QT_TR_NOOP("Part");
    sMenuText = QT_TR_NOOP("Create projection on surface...");
    sToolTipText = QT_TR_NOOP("Project edges, wires, or faces of one object\n"
                              "onto a face of another object.\n"
                              "The camera view determines the direction\n"
                              "of projection.");
    sWhatsThis = "Part_ProjectionOnSurface";
    sStatusTip = sToolTipText;
    sPixmap = "Part_ProjectionOnSurface";
}

void CmdPartProjectionOnSurface::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    PartGui::TaskProjectionOnSurface* dlg = new PartGui::TaskProjectionOnSurface();
    Gui::Control().showDialog(dlg);
}

bool CmdPartProjectionOnSurface::isActive()
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_SectionCut
//===========================================================================

DEF_STD_CMD_AC(CmdPartSectionCut)

CmdPartSectionCut::CmdPartSectionCut()
    : Command("Part_SectionCut")
{
    sAppModule = "Part";
    sGroup = "View";
    sMenuText = QT_TR_NOOP("Persistent section cut");
    sToolTipText = QT_TR_NOOP("Creates a persistent section cut of visible part objects");
    sWhatsThis = "Part_SectionCut";
    sStatusTip = sToolTipText;
    sPixmap = "Part_SectionCut";
    eType = AlterDoc | Alter3DView;
}

Gui::Action* CmdPartSectionCut::createAction()
{
    Gui::Action* pcAction = Gui::Command::createAction();
    return pcAction;
}

void CmdPartSectionCut::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    static QPointer<PartGui::SectionCut> sectionCut = nullptr;
    if (!sectionCut) {
        sectionCut = PartGui::SectionCut::makeDockWidget(Gui::getMainWindow());
    }
}

bool CmdPartSectionCut::isActive()
{
    return hasActiveDocument();
}

//---------------------------------------------------------------

void CreatePartCommands()
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdPartMakeSolid());
    rcCmdMgr.addCommand(new CmdPartReverseShape());
    rcCmdMgr.addCommand(new CmdPartBoolean());
    rcCmdMgr.addCommand(new CmdPartExtrude());
    rcCmdMgr.addCommand(new CmdPartScale());
    rcCmdMgr.addCommand(new CmdPartMakeFace());
    rcCmdMgr.addCommand(new CmdPartMirror());
    rcCmdMgr.addCommand(new CmdPartRevolve());
    rcCmdMgr.addCommand(new CmdPartCrossSections());
    rcCmdMgr.addCommand(new CmdPartFillet());
    rcCmdMgr.addCommand(new CmdPartChamfer());
    rcCmdMgr.addCommand(new CmdPartCommon());
    rcCmdMgr.addCommand(new CmdPartCut());
    rcCmdMgr.addCommand(new CmdPartFuse());
    rcCmdMgr.addCommand(new CmdPartCompJoinFeatures());
    rcCmdMgr.addCommand(new CmdPartCompSplitFeatures());
    rcCmdMgr.addCommand(new CmdPartCompCompoundTools());
    rcCmdMgr.addCommand(new CmdPartCompound());
    rcCmdMgr.addCommand(new CmdPartSection());
    rcCmdMgr.addCommand(new CmdPartPrimitives());

    rcCmdMgr.addCommand(new CmdPartImport());
    rcCmdMgr.addCommand(new CmdPartExport());
    rcCmdMgr.addCommand(new CmdPartImportCurveNet());
    rcCmdMgr.addCommand(new CmdPartPickCurveNet());
    rcCmdMgr.addCommand(new CmdShapeInfo());
    rcCmdMgr.addCommand(new CmdPartRuledSurface());
    rcCmdMgr.addCommand(new CmdPartBuilder());
    rcCmdMgr.addCommand(new CmdPartLoft());
    rcCmdMgr.addCommand(new CmdPartSweep());
    rcCmdMgr.addCommand(new CmdPartOffset());
    rcCmdMgr.addCommand(new CmdPartOffset2D());
    rcCmdMgr.addCommand(new CmdPartCompOffset());
    rcCmdMgr.addCommand(new CmdPartThickness());
    rcCmdMgr.addCommand(new CmdCheckGeometry());
    rcCmdMgr.addCommand(new CmdColorPerFace());
    rcCmdMgr.addCommand(new CmdMeasureLinear());
    rcCmdMgr.addCommand(new CmdMeasureAngular());
    rcCmdMgr.addCommand(new CmdMeasureRefresh());
    rcCmdMgr.addCommand(new CmdMeasureClearAll());
    rcCmdMgr.addCommand(new CmdMeasureToggleAll());
    rcCmdMgr.addCommand(new CmdMeasureToggle3d());
    rcCmdMgr.addCommand(new CmdMeasureToggleDelta());
    rcCmdMgr.addCommand(new CmdBoxSelection());
    rcCmdMgr.addCommand(new CmdPartProjectionOnSurface());
    rcCmdMgr.addCommand(new CmdPartSectionCut());
}
