/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <sstream>
# include <Python.h>
# include <QString>
# include <QDir>
# include <QFileInfo>
# include <QLineEdit>
# include <QPointer>
# include <Standard_math.hxx>
# include <TopoDS_Shape.hxx>
# include <TopExp_Explorer.hxx>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Standard_Version.hxx>
# include <TopoDS_TCompound.hxx>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/DocumentObserver.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/WaitCursor.h>

#include "../App/PartFeature.h"
#include <Mod/Part/App/Part2DObject.h>
#include "DlgPartImportStepImp.h"
#include "DlgBooleanOperation.h"
#include "DlgExtrusion.h"
#include "DlgRevolution.h"
#include "DlgFilletEdges.h"
#include "DlgPrimitives.h"
#include "DlgProjectionOnSurface.h"
#include "CrossSections.h"
#include "Mirroring.h"
#include "ViewProvider.h"
#include "TaskShapeBuilder.h"
#include "TaskLoft.h"
#include "TaskSweep.h"
#include "TaskDimension.h"
#include "TaskCheckGeometry.h"
#include "BoxSelection.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//===========================================================================
// Part_PickCurveNet
//===========================================================================
DEF_STD_CMD(CmdPartPickCurveNet);

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
DEF_STD_CMD(CmdPartNewDoc);

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
DEF_STD_CMD_A(CmdPartBox2);

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
    openCommand("Part Box Create");
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

bool CmdPartBox2::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Part_Box3
//===========================================================================
DEF_STD_CMD_A(CmdPartBox3);

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
    openCommand("Part Box Create");
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

bool CmdPartBox3::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Part_Primitives
//===========================================================================
DEF_STD_CMD_A(CmdPartPrimitives);

CmdPartPrimitives::CmdPartPrimitives()
  :Command("Part_Primitives")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Create primitives...");
    sToolTipText  = QT_TR_NOOP("Creation of parametrized geometric primitives");
    sWhatsThis    = "Part_Primitives";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_CreatePrimitives";
}

void CmdPartPrimitives::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    PartGui::TaskPrimitives* dlg = new PartGui::TaskPrimitives();
    Gui::Control().showDialog(dlg);
}

bool CmdPartPrimitives::isActive(void)
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
}

//===========================================================================
// Part_Cut
//===========================================================================
DEF_STD_CMD_A(CmdPartCut);

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
    std::vector<Gui::SelectionObject> Sel = getSelection().getSelectionEx(0, Part::Feature::getClassTypeId());
    if (Sel.size() != 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two shapes please."));
        return;
    }

    bool askUser = false;
    for (std::vector<Gui::SelectionObject>::iterator it = Sel.begin(); it != Sel.end(); ++it) {
        App::DocumentObject* obj = it->getObject();
        if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            const TopoDS_Shape& shape = static_cast<Part::Feature*>(obj)->Shape.getValue();
            if (!PartGui::checkForSolids(shape) && !askUser) {
                int ret = QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Non-solids selected"),
                    QObject::tr("The use of non-solids for boolean operations may lead to unexpected results.\n"
                                "Do you want to continue?"), QMessageBox::Yes, QMessageBox::No);
                if (ret == QMessageBox::No)
                    return;
                askUser = true;
            }
        }
    }

    std::string FeatName = getUniqueObjectName("Cut");

    openCommand("Part Cut");
    doCommand(Doc,"App.activeDocument().addObject(\"Part::Cut\",\"%s\")",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Base = App.activeDocument().%s",FeatName.c_str(),Sel[0].getFeatName());
    doCommand(Doc,"App.activeDocument().%s.Tool = App.activeDocument().%s",FeatName.c_str(),Sel[1].getFeatName());

    // hide the input objects and remove them from the parent group
    App::DocumentObjectGroup* targetGroup = 0;
    for (std::vector<Gui::SelectionObject>::iterator it = Sel.begin(); it != Sel.end(); ++it) {
        doCommand(Gui,"Gui.activeDocument().%s.Visibility=False",it->getFeatName());
        App::DocumentObjectGroup* group = it->getObject()->getGroup();
        if (group) {
            targetGroup = group;
            doCommand(Doc, "App.activeDocument().%s.removeObject(App.activeDocument().%s)",
                group->getNameInDocument(), it->getFeatName());
        }
    }

    if (targetGroup) {
        doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)",
            targetGroup->getNameInDocument(), FeatName.c_str());
    }

    copyVisual(FeatName.c_str(), "ShapeColor", Sel[0].getFeatName());
    copyVisual(FeatName.c_str(), "DisplayMode", Sel[0].getFeatName());
    updateActive();
    commitCommand();
}

bool CmdPartCut::isActive(void)
{
    return getSelection().countObjectsOfType(Part::Feature::getClassTypeId())==2;
}

//===========================================================================
// Part_Common
//===========================================================================
DEF_STD_CMD_A(CmdPartCommon);

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
    std::vector<Gui::SelectionObject> Sel = getSelection().getSelectionEx(0, Part::Feature::getClassTypeId());

    //test if selected object is a compound, and if it is, look how many children it has...
    std::size_t numShapes = 0;
    if (Sel.size() == 1){
        numShapes = 1; //to be updated later in code, if
        Gui::SelectionObject selobj = Sel[0];
        if (selobj.getObject()->isDerivedFrom(Part::Feature::getClassTypeId())){
            TopoDS_Shape sh = static_cast<Part::Feature*>(selobj.getObject())->Shape.getValue();
            if (sh.ShapeType() == TopAbs_COMPOUND) {
                numShapes = 0;
                TopoDS_Iterator it(sh);
                for (; it.More(); it.Next()) {
                    ++numShapes;
                }
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
    std::string FeatName = getUniqueObjectName("Common");
    std::stringstream str;
    std::vector<Gui::SelectionObject> partObjects;

    str << "App.activeDocument()." << FeatName << ".Shapes = [";
    for (std::vector<Gui::SelectionObject>::iterator it = Sel.begin(); it != Sel.end(); ++it) {
        App::DocumentObject* obj = it->getObject();
        if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            const TopoDS_Shape& shape = static_cast<Part::Feature*>(obj)->Shape.getValue();
            if (!PartGui::checkForSolids(shape) && !askUser) {
                int ret = QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Non-solids selected"),
                    QObject::tr("The use of non-solids for boolean operations may lead to unexpected results.\n"
                                "Do you want to continue?"), QMessageBox::Yes, QMessageBox::No);
                if (ret == QMessageBox::No)
                    return;
                askUser = true;
            }
            str << "App.activeDocument()." << it->getFeatName() << ",";
            partObjects.push_back(*it);
        }
    }
    str << "]";

    openCommand("Common");
    doCommand(Doc,"App.activeDocument().addObject(\"Part::MultiCommon\",\"%s\")",FeatName.c_str());
    runCommand(Doc,str.str().c_str());

    // hide the input objects and remove them from the parent group
    App::DocumentObjectGroup* targetGroup = 0;
    for (std::vector<Gui::SelectionObject>::iterator it = partObjects.begin(); it != partObjects.end(); ++it) {
        doCommand(Gui,"Gui.activeDocument().%s.Visibility=False",it->getFeatName());
        App::DocumentObjectGroup* group = it->getObject()->getGroup();
        if (group) {
            targetGroup = group;
            doCommand(Doc, "App.activeDocument().%s.removeObject(App.activeDocument().%s)",
                group->getNameInDocument(), it->getFeatName());
        }
    }

    if (targetGroup) {
        doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)",
            targetGroup->getNameInDocument(), FeatName.c_str());
    }

    copyVisual(FeatName.c_str(), "ShapeColor", partObjects.front().getFeatName());
    copyVisual(FeatName.c_str(), "DisplayMode", partObjects.front().getFeatName());
    updateActive();
    commitCommand();
}

bool CmdPartCommon::isActive(void)
{
    return getSelection().countObjectsOfType(Part::Feature::getClassTypeId())>=1;
}

//===========================================================================
// Part_Fuse
//===========================================================================
DEF_STD_CMD_A(CmdPartFuse);

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
    std::vector<Gui::SelectionObject> Sel = getSelection().getSelectionEx(0, Part::Feature::getClassTypeId());

    //test if selected object is a compound, and if it is, look how many children it has...
    std::size_t numShapes = 0;
    if (Sel.size() == 1){
        numShapes = 1; //to be updated later in code
        Gui::SelectionObject selobj = Sel[0];
        if (selobj.getObject()->isDerivedFrom(Part::Feature::getClassTypeId())){
            TopoDS_Shape sh = static_cast<Part::Feature*>(selobj.getObject())->Shape.getValue();
            if (sh.ShapeType() == TopAbs_COMPOUND) {
                numShapes = 0;
                TopoDS_Iterator it(sh);
                for (; it.More(); it.Next()) {
                    ++numShapes;
                }
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
    std::string FeatName = getUniqueObjectName("Fusion");
    std::stringstream str;
    std::vector<Gui::SelectionObject> partObjects;

    str << "App.activeDocument()." << FeatName << ".Shapes = [";
    for (std::vector<Gui::SelectionObject>::iterator it = Sel.begin(); it != Sel.end(); ++it) {
        App::DocumentObject* obj = it->getObject();
        if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            const TopoDS_Shape& shape = static_cast<Part::Feature*>(obj)->Shape.getValue();
            if (!PartGui::checkForSolids(shape) && !askUser) {
                int ret = QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Non-solids selected"),
                    QObject::tr("The use of non-solids for boolean operations may lead to unexpected results.\n"
                                "Do you want to continue?"), QMessageBox::Yes, QMessageBox::No);
                if (ret == QMessageBox::No)
                    return;
                askUser = true;
            }
            str << "App.activeDocument()." << it->getFeatName() << ",";
            partObjects.push_back(*it);
        }
    }
    str << "]";

    openCommand("Fusion");
    doCommand(Doc,"App.activeDocument().addObject(\"Part::MultiFuse\",\"%s\")",FeatName.c_str());
    runCommand(Doc,str.str().c_str());

    // hide the input objects and remove them from the parent group
    App::DocumentObjectGroup* targetGroup = 0;
    for (std::vector<Gui::SelectionObject>::iterator it = partObjects.begin(); it != partObjects.end(); ++it) {
        doCommand(Gui,"Gui.activeDocument().%s.Visibility=False",it->getFeatName());
        App::DocumentObjectGroup* group = it->getObject()->getGroup();
        if (group) {
            targetGroup = group;
            doCommand(Doc, "App.activeDocument().%s.removeObject(App.activeDocument().%s)",
                group->getNameInDocument(), it->getFeatName());
        }
    }

    if (targetGroup) {
        doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)",
            targetGroup->getNameInDocument(), FeatName.c_str());
    }

    copyVisual(FeatName.c_str(), "ShapeColor", partObjects.front().getFeatName());
    copyVisual(FeatName.c_str(), "DisplayMode", partObjects.front().getFeatName());
    updateActive();
    commitCommand();
}

bool CmdPartFuse::isActive(void)
{
    return getSelection().countObjectsOfType(Part::Feature::getClassTypeId())>=1;
}

//===========================================================================
// Part_CompJoinFeatures (dropdown toolbar button for Connect, Embed and Cutout)
//===========================================================================

DEF_STD_CMD_ACL(CmdPartCompJoinFeatures);

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

    // Since the default icon is reset when enabing/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdPartCompJoinFeatures::createAction(void)
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

bool CmdPartCompJoinFeatures::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Part_CompSplitFeatures (dropdown toolbar button for BooleanFragments, Slice)
//===========================================================================

DEF_STD_CMD_ACL(CmdPartCompSplitFeatures);

CmdPartCompSplitFeatures::CmdPartCompSplitFeatures()
  : Command("Part_CompSplitFeatures")
{
    sAppModule      = "Part";
    sGroup          = QT_TR_NOOP("Part");
    sMenuText       = QT_TR_NOOP("Split objects...");
    sToolTipText    = QT_TR_NOOP("Shape splitting tools. Compsolid creation tools. OCC 6.9.0 or later is required.");
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

    // Since the default icon is reset when enabing/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdPartCompSplitFeatures::createAction(void)
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

bool CmdPartCompSplitFeatures::isActive(void)
{
    if (getActiveGuiDocument())
#if OCC_VERSION_HEX < 0x060900
        return false;
#else
        return true;
#endif
    else
        return false;
}

//===========================================================================
// Part_CompCompoundTools (dropdown toolbar button for BooleanFragments, Slice)
//===========================================================================

DEF_STD_CMD_ACL(CmdPartCompCompoundTools);

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

    // Since the default icon is reset when enabing/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdPartCompCompoundTools::createAction(void)
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

bool CmdPartCompCompoundTools::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}



//===========================================================================
// Part_Compound
//===========================================================================
DEF_STD_CMD_A(CmdPartCompound);

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
    unsigned int n = getSelection().countObjectsOfType(Part::Feature::getClassTypeId());
    if (n < 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select one shape or more, please."));
        return;
    }

    std::string FeatName = getUniqueObjectName("Compound");

    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();
    std::stringstream str;
    std::vector<std::string> tempSelNames;
    str << "App.activeDocument()." << FeatName << ".Links = [";
    for (std::vector<Gui::SelectionSingleton::SelObj>::iterator it = Sel.begin(); it != Sel.end(); ++it){
        str << "App.activeDocument()." << it->FeatName << ",";
        tempSelNames.push_back(it->FeatName);
    }
    str << "]";

    openCommand("Compound");
    doCommand(Doc,"App.activeDocument().addObject(\"Part::Compound\",\"%s\")",FeatName.c_str());
    runCommand(Doc,str.str().c_str());
    updateActive();
    commitCommand();
}

bool CmdPartCompound::isActive(void)
{
    return getSelection().countObjectsOfType(Part::Feature::getClassTypeId())>=1;
}

//===========================================================================
// Part_Section
//===========================================================================
DEF_STD_CMD_A(CmdPartSection);

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
    std::vector<Gui::SelectionObject> Sel = getSelection().getSelectionEx(0, Part::Feature::getClassTypeId());
    if (Sel.size() != 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two shapes please."));
        return;
    }

    std::string FeatName = getUniqueObjectName("Section");
    std::string BaseName  = Sel[0].getFeatName();
    std::string ToolName  = Sel[1].getFeatName();

    openCommand("Section");
    doCommand(Doc,"App.activeDocument().addObject(\"Part::Section\",\"%s\")",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Base = App.activeDocument().%s",FeatName.c_str(),BaseName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Tool = App.activeDocument().%s",FeatName.c_str(),ToolName.c_str());
    doCommand(Gui,"Gui.activeDocument().hide('%s')",BaseName.c_str());
    doCommand(Gui,"Gui.activeDocument().hide('%s')",ToolName.c_str());
    doCommand(Gui,"Gui.activeDocument().%s.LineColor = Gui.activeDocument().%s.ShapeColor", FeatName.c_str(),BaseName.c_str());
    updateActive();
    commitCommand();
}

bool CmdPartSection::isActive(void)
{
    return getSelection().countObjectsOfType(Part::Feature::getClassTypeId())==2;
}

//===========================================================================
// CmdPartImport
//===========================================================================
DEF_STD_CMD_A(CmdPartImport);

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
        if (!pDoc) return; // no document
        openCommand("Import Part");
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
        for (std::list<Gui::MDIView*>::iterator it = views.begin(); it != views.end(); ++it) {
            (*it)->viewAll();
        }
    }
}

bool CmdPartImport::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// CmdPartExport
//===========================================================================
DEF_STD_CMD_A(CmdPartExport);

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
        if (!pDoc) return; // no document
        if (select == filter[1] ||
            select == filter[3]) {
            Gui::Application::Instance->exportTo((const char*)fn.toUtf8(),pDoc->getName(),"ImportGui");
        }
        else {
            Gui::Application::Instance->exportTo((const char*)fn.toUtf8(),pDoc->getName(),"Part");
        }
    }
}

bool CmdPartExport::isActive(void)
{
    return Gui::Selection().countObjectsOfType(Part::Feature::getClassTypeId()) > 0;
}

//===========================================================================
// PartImportCurveNet
//===========================================================================
DEF_STD_CMD_A(CmdPartImportCurveNet);

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
        openCommand("Part Import Curve Net");
        doCommand(Doc,"f = App.activeDocument().addObject(\"Part::CurveNet\",\"%s\")", (const char*)fi.baseName().toLatin1());
        doCommand(Doc,"f.FileName = \"%s\"",(const char*)fn.toLatin1());
        commitCommand();
        updateActive();
    }
}

bool CmdPartImportCurveNet::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Part_MakeSolid
//===========================================================================
DEF_STD_CMD_A(CmdPartMakeSolid);

CmdPartMakeSolid::CmdPartMakeSolid()
  :Command("Part_MakeSolid")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Convert to solid");
    sToolTipText  = QT_TR_NOOP("Create solid from a shell or compound");
    sWhatsThis    = "Part_MakeSolid";
    sStatusTip    = sToolTipText;
}

void CmdPartMakeSolid::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<App::DocumentObject*> objs = Gui::Selection().getObjectsOfType
        (Part::Feature::getClassTypeId());
    runCommand(Doc, "import Part");
    for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it != objs.end(); ++it) {
        const TopoDS_Shape& shape = static_cast<Part::Feature*>(*it)->Shape.getValue();
        if (!shape.IsNull()) {
            TopAbs_ShapeEnum type = shape.ShapeType();
            QString str;
            if (type == TopAbs_SOLID) {
                Base::Console().Message("%s is ignored because it is already a solid.\n",
                    (*it)->Label.getValue());
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
                    .arg(QLatin1String((*it)->getNameInDocument()))
                    .arg(QLatin1String((*it)->Label.getValue()));
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
                    .arg(QLatin1String((*it)->getNameInDocument()))
                    .arg(QLatin1String((*it)->Label.getValue()));
            }
            else {
                Base::Console().Message("%s is ignored because it is neither a shell nor a compound.\n",
                    (*it)->Label.getValue());
            }

            try {
                if (!str.isEmpty())
                    runCommand(Doc, str.toLatin1());
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("Cannot convert %s because %s.\n",
                    (*it)->Label.getValue(), e.what());
            }
        }
    }
}

bool CmdPartMakeSolid::isActive(void)
{
    return Gui::Selection().countObjectsOfType
        (Part::Feature::getClassTypeId()) > 0;
}

//===========================================================================
// Part_ReverseShape
//===========================================================================
DEF_STD_CMD_A(CmdPartReverseShape);

CmdPartReverseShape::CmdPartReverseShape()
  :Command("Part_ReverseShape")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Reverse shapes");
    sToolTipText  = QT_TR_NOOP("Reverse orientation of shapes");
    sWhatsThis    = "Part_ReverseShape";
    sStatusTip    = sToolTipText;
}

void CmdPartReverseShape::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<App::DocumentObject*> objs = Gui::Selection().getObjectsOfType
        (Part::Feature::getClassTypeId());
    runCommand(Doc, "import Part");
    for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it != objs.end(); ++it) {
        const TopoDS_Shape& shape = static_cast<Part::Feature*>(*it)->Shape.getValue();
        if (!shape.IsNull()) {
            QString str = QString::fromLatin1(
                "__s__=App.ActiveDocument.%1.Shape.copy()\n"
                "__s__.reverse()\n"
                "__o__=App.ActiveDocument.addObject(\"Part::Feature\",\"%1_rev\")\n"
                "__o__.Label=\"%2 (Rev)\"\n"
                "__o__.Shape=__s__\n"
                "del __s__, __o__"
                )
                .arg(QLatin1String((*it)->getNameInDocument()))
                .arg(QLatin1String((*it)->Label.getValue()));

            try {
                if (!str.isEmpty())
                    runCommand(Doc, str.toLatin1());
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("Cannot convert %s because %s.\n",
                    (*it)->Label.getValue(), e.what());
            }
        }
    }
}

bool CmdPartReverseShape::isActive(void)
{
    return Gui::Selection().countObjectsOfType
        (Part::Feature::getClassTypeId()) > 0;
}

//===========================================================================
// Part_Boolean
//===========================================================================
DEF_STD_CMD_A(CmdPartBoolean);

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

bool CmdPartBoolean::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Extrude
//===========================================================================
DEF_STD_CMD_A(CmdPartExtrude);

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

bool CmdPartExtrude::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_MakeFace
//===========================================================================
DEF_STD_CMD_A(CmdPartMakeFace);

CmdPartMakeFace::CmdPartMakeFace()
  : Command("Part_MakeFace")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Make face from wires");
    sToolTipText  = QT_TR_NOOP("Make face from set of wires (e.g. from a sketch)");
    sWhatsThis    = "Part_MakeFace";
    sStatusTip    = sToolTipText;
}

void CmdPartMakeFace::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<Part::Feature*> sketches = Gui::Selection().getObjectsOfType<Part::Feature>();
    openCommand("Make face");

    try {
        App::DocumentT doc(sketches.front()->getDocument());
        std::stringstream str;
        str << doc.getDocumentPython()
            << ".addObject(\"Part::Face\", \"Face\").Sources = (";
        for (std::vector<Part::Feature*>::iterator it = sketches.begin(); it != sketches.end(); ++it) {
            App::DocumentObjectT obj(*it);
            str << obj.getObjectPython() << ", ";
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

bool CmdPartMakeFace::isActive(void)
{
    return (Gui::Selection().countObjectsOfType(Part::Feature::getClassTypeId()) > 0 &&
            !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Revolve
//===========================================================================
DEF_STD_CMD_A(CmdPartRevolve);

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

bool CmdPartRevolve::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Fillet
//===========================================================================
DEF_STD_CMD_A(CmdPartFillet);

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
    Gui::Control().showDialog(new PartGui::TaskFilletEdges(0));
}

bool CmdPartFillet::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Chamfer
//===========================================================================
DEF_STD_CMD_A(CmdPartChamfer);

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
    Gui::Control().showDialog(new PartGui::TaskChamferEdges(0));
}

bool CmdPartChamfer::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Mirror
//===========================================================================
DEF_STD_CMD_A(CmdPartMirror);

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

bool CmdPartMirror::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_CrossSections
//===========================================================================
DEF_STD_CMD_A(CmdPartCrossSections);

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
        std::vector<App::DocumentObject*> obj = Gui::Selection().getObjectsOfType
            (Part::Feature::getClassTypeId());
        Base::BoundBox3d bbox;
        for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it) {
            bbox.Add(static_cast<Part::Feature*>(*it)->Shape.getBoundingBox());
        }
        dlg = new PartGui::TaskCrossSections(bbox);
    }
    Gui::Control().showDialog(dlg);
}

bool CmdPartCrossSections::isActive(void)
{
    return (Gui::Selection().countObjectsOfType(Part::Feature::getClassTypeId()) > 0 &&
            !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Builder
//===========================================================================

DEF_STD_CMD_A(CmdPartBuilder);

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

bool CmdPartBuilder::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Loft
//===========================================================================

DEF_STD_CMD_A(CmdPartLoft);

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

bool CmdPartLoft::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Sweep
//===========================================================================

DEF_STD_CMD_A(CmdPartSweep);

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

bool CmdPartSweep::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Offset
//===========================================================================

DEF_STD_CMD_A(CmdPartOffset);

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
    App::DocumentObject* shape = getSelection().getObjectsOfType(Part::Feature::getClassTypeId()).front();
    std::string offset = getUniqueObjectName("Offset");

    openCommand("Make Offset");
    doCommand(Doc,"App.ActiveDocument.addObject(\"Part::Offset\",\"%s\")",offset.c_str());
    doCommand(Doc,"App.ActiveDocument.%s.Source = App.ActiveDocument.%s" ,offset.c_str(), shape->getNameInDocument());
    doCommand(Doc,"App.ActiveDocument.%s.Value = 1.0",offset.c_str());
    updateActive();
    doCommand(Gui,"Gui.ActiveDocument.%s.DisplayMode = 'Wireframe'", shape->getNameInDocument());
    //if (isActiveObjectValid())
    //    doCommand(Gui,"Gui.ActiveDocument.hide(\"%s\")",shape->getNameInDocument());
    doCommand(Gui,"Gui.ActiveDocument.setEdit('%s')",offset.c_str());

    //commitCommand();
    adjustCameraPosition();

    copyVisual(offset.c_str(), "ShapeColor", shape->getNameInDocument());
    copyVisual(offset.c_str(), "LineColor" , shape->getNameInDocument());
    copyVisual(offset.c_str(), "PointColor", shape->getNameInDocument());
}

bool CmdPartOffset::isActive(void)
{
    Base::Type partid = Base::Type::fromName("Part::Feature");
    bool objectsSelected = Gui::Selection().countObjectsOfType(partid) == 1;
    return (objectsSelected && !Gui::Control().activeDialog());
}


//===========================================================================
// Part_Offset2D
//===========================================================================

DEF_STD_CMD_A(CmdPartOffset2D);

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
    App::DocumentObject* shape = getSelection().getObjectsOfType(Part::Feature::getClassTypeId()).front();
    std::string offset = getUniqueObjectName("Offset2D");

    openCommand("Make 2D Offset");
    doCommand(Doc,"App.ActiveDocument.addObject(\"Part::Offset2D\",\"%s\")",offset.c_str());
    doCommand(Doc,"App.ActiveDocument.%s.Source = App.ActiveDocument.%s" ,offset.c_str(), shape->getNameInDocument());
    doCommand(Doc,"App.ActiveDocument.%s.Value = 1.0",offset.c_str());
    updateActive();
    //if (isActiveObjectValid())
    //    doCommand(Gui,"Gui.ActiveDocument.hide(\"%s\")",shape->getNameInDocument());
    doCommand(Gui,"Gui.ActiveDocument.setEdit('%s')",offset.c_str());

    //commitCommand();
    adjustCameraPosition();

    copyVisual(offset.c_str(), "ShapeColor", shape->getNameInDocument());
    copyVisual(offset.c_str(), "LineColor" , shape->getNameInDocument());
    copyVisual(offset.c_str(), "PointColor", shape->getNameInDocument());
}

bool CmdPartOffset2D::isActive(void)
{
    Base::Type partid = Base::Type::fromName("Part::Feature");
    bool objectsSelected = Gui::Selection().countObjectsOfType(partid) == 1;
    return (objectsSelected && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_CompOffset (dropdown toolbar button for Offset features)
//===========================================================================

DEF_STD_CMD_ACL(CmdPartCompOffset);

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

    // Since the default icon is reset when enabing/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdPartCompOffset::createAction(void)
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
        cmd0->setText(QApplication::translate("Part_Offset", cmdOffset->getMenuText()));
        cmd0->setToolTip(QApplication::translate("Part_Offset", cmdOffset->getToolTipText()));
        cmd0->setStatusTip(QApplication::translate("Part_Offset", cmdOffset->getStatusTip()));
    }

    Gui::Command* cmdOffset2D = rcCmdMgr.getCommandByName("Part_Offset2D");
    if (cmdOffset2D) {
        QAction* cmd1 = a[1];
        cmd1->setText(QApplication::translate("Part_Offset", cmdOffset2D->getMenuText()));
        cmd1->setToolTip(QApplication::translate("Part_Offset", cmdOffset2D->getToolTipText()));
        cmd1->setStatusTip(QApplication::translate("Part_Offset", cmdOffset2D->getStatusTip()));
    }
}

bool CmdPartCompOffset::isActive(void)
{
    Base::Type partid = Base::Type::fromName("Part::Feature");
    bool objectsSelected = Gui::Selection().countObjectsOfType(partid) == 1;
    return (objectsSelected && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_Thickness
//===========================================================================

DEF_STD_CMD_A(CmdPartThickness);

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
    Gui::SelectionFilter faceFilter  ("SELECT Part::Feature SUBELEMENT Face COUNT 1..");
    if (!faceFilter.match()) {
        QMessageBox::warning(Gui::getMainWindow(),
            QApplication::translate("CmdPartThickness", "Wrong selection"),
            QApplication::translate("CmdPartThickness", "Selected one or more faces of a shape"));
        return;
    }

    // get the selected object
    const std::vector<Gui::SelectionObject>& result = faceFilter.Result[0];
    std::string selection = result.front().getAsPropertyLinkSubString();

    const Part::Feature* shape = static_cast<const Part::Feature*>(result.front().getObject());
    if (shape->Shape.getValue().IsNull())
        return;
    int countSolids = 0;
    TopExp_Explorer xp;
    xp.Init(shape->Shape.getValue(),TopAbs_SOLID);
    for (;xp.More(); xp.Next()) {
        countSolids++;
    }
    if (countSolids != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
            QApplication::translate("CmdPartThickness", "Wrong selection"),
            QApplication::translate("CmdPartThickness", "Selected shape is not a solid"));
        return;
    }

    std::string thick = getUniqueObjectName("Thickness");

    openCommand("Make Thickness");
    doCommand(Doc,"App.ActiveDocument.addObject(\"Part::Thickness\",\"%s\")",thick.c_str());
    doCommand(Doc,"App.ActiveDocument.%s.Faces = %s" ,thick.c_str(), selection.c_str());
    doCommand(Doc,"App.ActiveDocument.%s.Value = 1.0",thick.c_str());
    updateActive();
    if (isActiveObjectValid())
        doCommand(Gui,"Gui.ActiveDocument.hide(\"%s\")",shape->getNameInDocument());
    doCommand(Gui,"Gui.ActiveDocument.setEdit('%s')",thick.c_str());

    //commitCommand();
    adjustCameraPosition();

    copyVisual(thick.c_str(), "ShapeColor", shape->getNameInDocument());
    copyVisual(thick.c_str(), "LineColor" , shape->getNameInDocument());
    copyVisual(thick.c_str(), "PointColor", shape->getNameInDocument());
}

bool CmdPartThickness::isActive(void)
{
    Base::Type partid = Base::Type::fromName("Part::Feature");
    bool objectsSelected = Gui::Selection().countObjectsOfType(partid) > 0;
    return (objectsSelected && !Gui::Control().activeDialog());
}

//===========================================================================
// Part_ShapeInfo
//===========================================================================

DEF_STD_CMD_A(CmdShapeInfo);

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
#if 0
    static const char * const part_pipette[]={
        "32 32 17 1",
        "# c #000000",
        "j c #080808",
        "b c #101010",
        "f c #1c1c1c",
        "g c #4c4c4c",
        "c c #777777",
        "a c #848484",
        "i c #9c9c9c",
        "l c #b9b9b9",
        "e c #cacaca",
        "n c #d6d6d6",
        "k c #dedede",
        "d c #e7e7e7",
        "m c #efefef",
        "h c #f7f7f7",
        "w c #ffffff",
        ". c None",
        "................................",
        ".....................#####......",
        "...................#######......",
        "...................#########....",
        "..................##########....",
        "..................##########....",
        "..................##########....",
        ".................###########....",
        "...............#############....",
        ".............###############....",
        ".............#############......",
        ".............#############......",
        "...............ab######.........",
        "..............cdef#####.........",
        ".............ghdacf####.........",
        "............#ehiacj####.........",
        "............awiaaf####..........",
        "...........iheacf##.............",
        "..........#kdaag##..............",
        ".........gedaacb#...............",
        ".........lwiac##................",
        ".......#amlaaf##................",
        ".......cheaag#..................",
        "......#ndaag##..................",
        ".....#imaacb#...................",
        ".....iwlacf#....................",
        "....#nlaag##....................",
        "....feaagj#.....................",
        "....caag##......................",
        "....ffbj##......................",
        "................................",
        "................................"};

    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
#endif
    //if (view) {
    //    Gui::View3DInventorViewer* viewer = view->getViewer();
    //    viewer->setEditing(true);
    //    viewer->getWidget()->setCursor(QCursor(QPixmap(part_pipette),4,29));
    //    viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), PartGui::ViewProviderPart::shapeInfoCallback);
    // }
}

bool CmdShapeInfo::isActive(void)
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

DEF_STD_CMD_A(CmdPartRuledSurface);

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
    bool ok = false;
    TopoDS_Shape curve1, curve2;
    std::string link1, link2, obj1, obj2;
    Gui::SelectionFilter edgeFilter  ("SELECT Part::Feature SUBELEMENT Edge COUNT 1..2");
    Gui::SelectionFilter wireFilter  ("SELECT Part::Feature SUBELEMENT Wire COUNT 1..2");
    Gui::SelectionFilter partFilter  ("SELECT Part::Feature COUNT 2");
    bool matchEdge = edgeFilter.match();
    bool matchWire = wireFilter.match();
    if (matchEdge || matchWire) {
        // get the selected object
        const std::vector<Gui::SelectionObject>& result = matchEdge
            ? edgeFilter.Result[0] : wireFilter.Result[0];
        // two edges from one object
        if (result.size() == 1) {
            const Part::Feature* part = static_cast<const Part::Feature*>(result[0].getObject());
            const std::vector<std::string>& edges = result[0].getSubNames();
            if (edges.size() != 2) {
                ok = false;
            }
            else {
                ok = true;
                // get the selected sub-shapes
                const Part::TopoShape& shape = part->Shape.getValue();
                curve1 = shape.getSubShape(edges[0].c_str());
                curve2 = shape.getSubShape(edges[1].c_str());
                obj1 = result[0].getObject()->getNameInDocument();
                link1 = edges[0];
                obj2 = result[0].getObject()->getNameInDocument();
                link2 = edges[1];
            }
        }
        // two objects and one edge per object
        else if (result.size() == 2) {
            const Part::Feature* part1 = static_cast<const Part::Feature*>(result[0].getObject());
            const std::vector<std::string>& edges1 = result[0].getSubNames();
            const Part::Feature* part2 = static_cast<const Part::Feature*>(result[1].getObject());
            const std::vector<std::string>& edges2 = result[1].getSubNames();
            if (edges1.size() != 1 || edges2.size() != 1) {
                ok = false;
            }
            else {
                ok = true;
                const Part::TopoShape& shape1 = part1->Shape.getValue();
                curve1 = shape1.getSubShape(edges1[0].c_str());
                const Part::TopoShape& shape2 = part2->Shape.getValue();
                curve2 = shape2.getSubShape(edges2[0].c_str());
                obj1 = result[0].getObject()->getNameInDocument();
                link1 = edges1[0];
                obj2 = result[1].getObject()->getNameInDocument();
                link2 = edges2[0];
            }
        }
    }
    else if (partFilter.match()) {
        const std::vector<Gui::SelectionObject>& result = partFilter.Result[0];
        const Part::Feature* part1 = static_cast<const Part::Feature*>(result[0].getObject());
        const Part::Feature* part2 = static_cast<const Part::Feature*>(result[1].getObject());
        const Part::TopoShape& shape1 = part1->Shape.getValue();
        curve1 = shape1.getShape();
        const Part::TopoShape& shape2 = part2->Shape.getValue();
        curve2 = shape2.getShape();
        obj1 = part1->getNameInDocument();
        obj2 = part2->getNameInDocument();

        if (!curve1.IsNull() && !curve2.IsNull()) {
            if (curve1.ShapeType() == TopAbs_EDGE &&
                curve2.ShapeType() == TopAbs_EDGE)
                ok = true;
            if (curve1.ShapeType() == TopAbs_WIRE &&
                curve2.ShapeType() == TopAbs_WIRE)
                ok = true;
        }
    }

    if (!ok) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("You have to select either two edges or two wires."));
        return;
    }

    openCommand("Create ruled surface");
    doCommand(Doc, "FreeCAD.ActiveDocument.addObject('Part::RuledSurface', 'Ruled Surface')");
    doCommand(Doc, "FreeCAD.ActiveDocument.ActiveObject.Curve1=(FreeCAD.ActiveDocument.%s,['%s'])"
                 ,obj1.c_str(), link1.c_str());
    doCommand(Doc, "FreeCAD.ActiveDocument.ActiveObject.Curve2=(FreeCAD.ActiveDocument.%s,['%s'])"
                 ,obj2.c_str(), link2.c_str());
    commitCommand();
    updateActive();
}

bool CmdPartRuledSurface::isActive(void)
{
    return getActiveGuiDocument();
}

//===========================================================================
// Part_CheckGeometry
//===========================================================================

DEF_STD_CMD_A(CmdCheckGeometry);

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

bool CmdCheckGeometry::isActive(void)
{
    Base::Type partid = Base::Type::fromName("Part::Feature");
    bool objectsSelected = Gui::Selection().countObjectsOfType(partid) > 0;
    return (hasActiveDocument() && !Gui::Control().activeDialog() && objectsSelected);
}

//===========================================================================
// Part_ColorPerFace
//===========================================================================

DEF_STD_CMD_A(CmdColorPerFace);

CmdColorPerFace::CmdColorPerFace()
  : Command("Part_ColorPerFace")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Color per face");
    sToolTipText  = QT_TR_NOOP("Set color per face");
    sStatusTip    = sToolTipText;
    sWhatsThis    = "Part_ColorPerFace";
}

void CmdColorPerFace::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (getActiveGuiDocument()->getInEdit())
        getActiveGuiDocument()->resetEdit();
    std::vector<App::DocumentObject*> sel = Gui::Selection().getObjectsOfType(Part::Feature::getClassTypeId());
    Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(sel.front());
    // FIXME: Need a way to force 'Color' edit mode
    // #0000477: Proper interface for edit modes of view provider
    if (vp)
        getActiveGuiDocument()->setEdit(vp, Gui::ViewProvider::Color);
}

bool CmdColorPerFace::isActive(void)
{
    Base::Type partid = Base::Type::fromName("Part::Feature");
    bool objectSelected = Gui::Selection().countObjectsOfType(partid) == 1;
    return (hasActiveDocument() && !Gui::Control().activeDialog() && objectSelected);
}

//===========================================================================
// Part_Measure_Linear
//===========================================================================

DEF_STD_CMD_A(CmdMeasureLinear);

CmdMeasureLinear::CmdMeasureLinear()
  : Command("Part_Measure_Linear")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Measure Linear");
    sToolTipText  = QT_TR_NOOP("Measure Linear");
    sWhatsThis    = "Part_Measure_Linear";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Measure_Linear";
}

void CmdMeasureLinear::activated(int iMsg)
{
  Q_UNUSED(iMsg);
  PartGui::goDimensionLinearRoot();
}

bool CmdMeasureLinear::isActive(void)
{
  return hasActiveDocument();
}

//===========================================================================
// Part_Measure_Angular
//===========================================================================

DEF_STD_CMD_A(CmdMeasureAngular);

CmdMeasureAngular::CmdMeasureAngular()
  : Command("Part_Measure_Angular")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Measure Angular");
    sToolTipText  = QT_TR_NOOP("Measure Angular");
    sWhatsThis    = "Part_Measure_Angular";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Measure_Angular";
}

void CmdMeasureAngular::activated(int iMsg)
{
  Q_UNUSED(iMsg);
  PartGui::goDimensionAngularRoot();
}

bool CmdMeasureAngular::isActive(void)
{
  return hasActiveDocument();
}

//===========================================================================
// Part_Measure_Clear_All
//===========================================================================

DEF_STD_CMD_A(CmdMeasureClearAll);

CmdMeasureClearAll::CmdMeasureClearAll()
  : Command("Part_Measure_Clear_All")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Clear All");
    sToolTipText  = QT_TR_NOOP("Clear All");
    sWhatsThis    = "Part_Measure_Clear_All";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Measure_Clear_All";
}

void CmdMeasureClearAll::activated(int iMsg)
{
  Q_UNUSED(iMsg);
  PartGui::eraseAllDimensions();
}

bool CmdMeasureClearAll::isActive(void)
{
  return hasActiveDocument();
}

//===========================================================================
// Part_Measure_Toggle_All
//===========================================================================

DEF_STD_CMD_A(CmdMeasureToggleAll);

CmdMeasureToggleAll::CmdMeasureToggleAll()
  : Command("Part_Measure_Toggle_All")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Toggle All");
    sToolTipText  = QT_TR_NOOP("Toggle All");
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

bool CmdMeasureToggleAll::isActive(void)
{
  return hasActiveDocument();
}

//===========================================================================
// Part_Measure_Toggle_3d
//===========================================================================

DEF_STD_CMD_A(CmdMeasureToggle3d);

CmdMeasureToggle3d::CmdMeasureToggle3d()
  : Command("Part_Measure_Toggle_3d")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Toggle 3d");
    sToolTipText  = QT_TR_NOOP("Toggle 3d");
    sWhatsThis    = "Part_Measure_Toggle_3d";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Measure_Toggle_3d";
}

void CmdMeasureToggle3d::activated(int iMsg)
{
  Q_UNUSED(iMsg);
  PartGui::toggle3d();
}

bool CmdMeasureToggle3d::isActive(void)
{
  return hasActiveDocument();
}

//===========================================================================
// Part_Measure_Toggle_Delta
//===========================================================================

DEF_STD_CMD_A(CmdMeasureToggleDelta);

CmdMeasureToggleDelta::CmdMeasureToggleDelta()
  : Command("Part_Measure_Toggle_Delta")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Toggle Delta");
    sToolTipText  = QT_TR_NOOP("Toggle Delta");
    sWhatsThis    = "Part_Measure_Toggle_Delta";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Measure_Toggle_Delta";
}

void CmdMeasureToggleDelta::activated(int iMsg)
{
  Q_UNUSED(iMsg);
  PartGui::toggleDelta();
}

bool CmdMeasureToggleDelta::isActive(void)
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
    sel->start();
}

bool CmdBoxSelection::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// Part_projectionOnSurface
//===========================================================================
DEF_STD_CMD_A(CmdPartProjectionOnSurface);

CmdPartProjectionOnSurface::CmdPartProjectionOnSurface()
  :Command("Part_projectionOnSurface")
{
  sAppModule = "Part";
  sGroup = QT_TR_NOOP("Part");
  sMenuText = QT_TR_NOOP("Create projection on surface...");
  sToolTipText = QT_TR_NOOP("Create projection on surface...");
  sWhatsThis = "Part_projectionOnSurface";
  sStatusTip = sToolTipText;
  sPixmap = "Part_ProjectionOnSurface";
}

void CmdPartProjectionOnSurface::activated(int iMsg)
{
  Q_UNUSED(iMsg);
  PartGui::TaskProjectionOnSurface* dlg = new PartGui::TaskProjectionOnSurface();
  Gui::Control().showDialog(dlg);
}

bool CmdPartProjectionOnSurface::isActive(void)
{
  return (hasActiveDocument() && !Gui::Control().activeDialog());
}

void CreatePartCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdPartMakeSolid());
    rcCmdMgr.addCommand(new CmdPartReverseShape());
    rcCmdMgr.addCommand(new CmdPartBoolean());
    rcCmdMgr.addCommand(new CmdPartExtrude());
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
    //rcCmdMgr.addCommand(new CmdPartBox2());
    //rcCmdMgr.addCommand(new CmdPartBox3());
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
    rcCmdMgr.addCommand(new CmdMeasureClearAll());
    rcCmdMgr.addCommand(new CmdMeasureToggleAll());
    rcCmdMgr.addCommand(new CmdMeasureToggle3d());
    rcCmdMgr.addCommand(new CmdMeasureToggleDelta());
    rcCmdMgr.addCommand(new CmdBoxSelection());
    rcCmdMgr.addCommand(new CmdPartProjectionOnSurface());
}
