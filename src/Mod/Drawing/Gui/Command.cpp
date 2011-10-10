/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   for detail see the LICENCE text file.                                 *
 *   Jürgen Riegel 2002                                                    *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QMessageBox>
#endif

#include <vector>

#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Selection.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Drawing/App/FeaturePage.h>


#include "DrawingView.h"
#include "TaskDialog.h"

using namespace DrawingGui;
using namespace std;


//===========================================================================
// CmdDrawingOpen
//===========================================================================

DEF_STD_CMD(CmdDrawingOpen);

CmdDrawingOpen::CmdDrawingOpen()
  : Command("Drawing_Open")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Open SVG...");
    sToolTipText    = QT_TR_NOOP("Open a scalable vector graphic");
    sWhatsThis      = "Drawing_Open";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/document-new";
}


void CmdDrawingOpen::activated(int iMsg)
{
    // Reading an image
    QString filename = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(), QObject::tr("Choose an SVG file to open"), QString::null, 
                                           QObject::tr("Scalable Vector Graphics (*.svg *.svgz)"));
    if (!filename.isEmpty())
    {
        // load the file with the module
        Command::doCommand(Command::Gui, "import Drawing, DrawingGui");
        Command::doCommand(Command::Gui, "DrawingGui.open(\"%s\")", (const char*)filename.toUtf8());
    }
}

//===========================================================================
// Drawing_NewPage
//===========================================================================

DEF_STD_CMD_ACL(CmdDrawingNewPage);

CmdDrawingNewPage::CmdDrawingNewPage()
  : Command("Drawing_NewPage")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert new drawing");
    sToolTipText    = QT_TR_NOOP("Insert new drawing");
    sWhatsThis      = "Drawing_NewPage";
    sStatusTip      = sToolTipText;
}

void CmdDrawingNewPage::activated(int iMsg)
{
    std::string FeatName = getUniqueObjectName("Page");

    if (iMsg == 3) {
        openCommand("Drawing create page");
        doCommand(Doc,"App.activeDocument().addObject('Drawing::FeaturePage','%s')",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Template = 'A3_Landscape.svg'",FeatName.c_str());
        commitCommand();
    }
    else {
        QMessageBox::critical(Gui::getMainWindow(),
            QLatin1String("No template"),
            QLatin1String("No template available for this page size"));
    }
}

Gui::Action * CmdDrawingNewPage::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(pcAction);

    QAction* a0 = pcAction->addAction(QString());
    a0->setIcon(Gui::BitmapFactory().pixmap("actions/drawing-landscape-A0"));

    QAction* a1 = pcAction->addAction(QString());
    a1->setIcon(Gui::BitmapFactory().pixmap("actions/drawing-landscape-A1"));

    QAction* a2 = pcAction->addAction(QString());
    a2->setIcon(Gui::BitmapFactory().pixmap("actions/drawing-landscape-A2"));

    QAction* a3 = pcAction->addAction(QString());
    a3->setIcon(Gui::BitmapFactory().pixmap("actions/drawing-landscape-A3"));

    QAction* a4 = pcAction->addAction(QString());
    a4->setIcon(Gui::BitmapFactory().pixmap("actions/drawing-landscape-A4"));

    _pcAction = pcAction;
    languageChange();
    pcAction->setIcon(a3->icon());
    pcAction->setProperty("defaultAction", QVariant(3));

    return pcAction;
}

void CmdDrawingNewPage::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    a[0]->setText(QCoreApplication::translate(
        "Drawing_NewPage", "A0 landscape", 0,
        QCoreApplication::CodecForTr));
    a[0]->setToolTip(QCoreApplication::translate(
        "Drawing_NewPage", "Insert new A0 landscape drawing", 0,
        QCoreApplication::CodecForTr));

    a[1]->setText(QCoreApplication::translate(
        "Drawing_NewPage", "A1 landscape", 0,
        QCoreApplication::CodecForTr));
    a[1]->setToolTip(QCoreApplication::translate(
        "Drawing_NewPage", "Insert new A1 landscape drawing", 0,
        QCoreApplication::CodecForTr));

    a[2]->setText(QCoreApplication::translate(
        "Drawing_NewPage", "A2 landscape", 0,
        QCoreApplication::CodecForTr));
    a[2]->setToolTip(QCoreApplication::translate(
        "Drawing_NewPage", "Insert new A2 landscape drawing", 0,
        QCoreApplication::CodecForTr));

    a[3]->setText(QCoreApplication::translate(
        "Drawing_NewPage", "A3 landscape", 0,
        QCoreApplication::CodecForTr));
    a[3]->setToolTip(QCoreApplication::translate(
        "Drawing_NewPage", "Insert new A3 landscape drawing", 0,
        QCoreApplication::CodecForTr));

    a[4]->setText(QCoreApplication::translate(
        "Drawing_NewPage", "A4 landscape", 0,
        QCoreApplication::CodecForTr));
    a[4]->setToolTip(QCoreApplication::translate(
        "Drawing_NewPage", "Insert new A4 landscape drawing", 0,
        QCoreApplication::CodecForTr));
}

bool CmdDrawingNewPage::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Drawing_NewA3Landscape
//===========================================================================

DEF_STD_CMD_A(CmdDrawingNewA3Landscape);

CmdDrawingNewA3Landscape::CmdDrawingNewA3Landscape()
  : Command("Drawing_NewA3Landscape")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert new A3 landscape drawing");
    sToolTipText    = QT_TR_NOOP("Insert new A3 landscape drawing");
    sWhatsThis      = "Drawing_NewA3Landscape";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/drawing-landscape-A3";
}

void CmdDrawingNewA3Landscape::activated(int iMsg)
{
    std::string FeatName = getUniqueObjectName("Page");

    openCommand("Drawing create page");
    doCommand(Doc,"App.activeDocument().addObject('Drawing::FeaturePage','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Template = 'A3_Landscape.svg'",FeatName.c_str());
    commitCommand();
}

bool CmdDrawingNewA3Landscape::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}


//===========================================================================
// Drawing_NewView
//===========================================================================

DEF_STD_CMD(CmdDrawingNewView);

CmdDrawingNewView::CmdDrawingNewView()
  : Command("Drawing_NewView")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert view in drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new View of a Part in the active drawing");
    sWhatsThis      = "Drawing_NewView";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/drawing-view";
}

void CmdDrawingNewView::activated(int iMsg)
{
    std::vector<App::DocumentObject*> shapes = getSelection().getObjectsOfType(Part::Feature::getClassTypeId());
    if (shapes.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select a Part object."));
        return;
    }

    std::vector<App::DocumentObject*> pages = this->getDocument()->getObjectsOfType(Drawing::FeaturePage::getClassTypeId());
    if (pages.empty()){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No page to insert"),
            QObject::tr("Create a page to insert."));
        return;
    }

    std::string FeatName = getUniqueObjectName("View");
    std::string PageName = pages.front()->getNameInDocument();

    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();

    openCommand("Create view");
    doCommand(Doc,"App.activeDocument().addObject('Drawing::FeatureViewPart','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Source = App.activeDocument().%s",FeatName.c_str(),shapes.front()->getNameInDocument());
    doCommand(Doc,"App.activeDocument().%s.Direction = (0.0,0.0,1.0)",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.X = 10.0",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Y = 10.0",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Scale = 1.0",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    updateActive();
    commitCommand();
}

//===========================================================================
// Drawing_ExportPage
//===========================================================================

DEF_STD_CMD_A(CmdDrawingExportPage);

CmdDrawingExportPage::CmdDrawingExportPage()
  : Command("Drawing_ExportPage")
{
    // seting the
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("&Export page...");
    sToolTipText  = QT_TR_NOOP("Export a page to an SVG file");
    sWhatsThis    = "Drawing_ExportPage";
    sStatusTip    = QT_TR_NOOP("Export a page to an SVG file");
    sPixmap       = "document-save";
}

void CmdDrawingExportPage::activated(int iMsg)
{
    unsigned int n = getSelection().countObjectsOfType(Drawing::FeaturePage::getClassTypeId());
    if (n != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select one Page object."));
        return;
    }

    QStringList filter;
    filter << QObject::tr("SVG(*.svg)");
    filter << QObject::tr("All Files (*.*)");

    QString fn = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(), QObject::tr("Export page"), QString(), filter.join(QLatin1String(";;")));
    if (!fn.isEmpty()) {
        std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();
        openCommand("Drawing export page");

        doCommand(Doc,"PageFile = open(App.activeDocument().%s.PageResult,'r')",Sel[0].FeatName);
        std::string fname = (const char*)fn.toAscii();
        doCommand(Doc,"OutFile = open('%s','w')",fname.c_str());
        doCommand(Doc,"OutFile.write(PageFile.read())");
        doCommand(Doc,"del OutFile,PageFile");

        commitCommand();
    }
}

bool CmdDrawingExportPage::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}

//===========================================================================
// Drawing_ProjectShape
//===========================================================================

DEF_STD_CMD_A(CmdDrawingProjectShape);

CmdDrawingProjectShape::CmdDrawingProjectShape()
  : Command("Drawing_ProjectShape")
{
    // seting the
    sGroup        = QT_TR_NOOP("Drawing");
    sMenuText     = QT_TR_NOOP("Project shape...");
    sToolTipText  = QT_TR_NOOP("Project shape onto a user-defined plane");
    sStatusTip    = QT_TR_NOOP("Project shape onto a user-defined plane");
    sWhatsThis    = "Drawing_ProjectShape";
}

void CmdDrawingProjectShape::activated(int iMsg)
{
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg) {
        dlg = new DrawingGui::TaskProjection();
        dlg->setButtonPosition(Gui::TaskView::TaskDialog::South);
    }
    Gui::Control().showDialog(dlg);
}

bool CmdDrawingProjectShape::isActive(void)
{
    int ct = Gui::Selection().countObjectsOfType(Part::Feature::getClassTypeId());
    return (ct > 0 && !Gui::Control().activeDialog());
}



void CreateDrawingCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdDrawingOpen());
    rcCmdMgr.addCommand(new CmdDrawingNewPage());
    rcCmdMgr.addCommand(new CmdDrawingNewA3Landscape());
    rcCmdMgr.addCommand(new CmdDrawingNewView());
    rcCmdMgr.addCommand(new CmdDrawingExportPage());
    rcCmdMgr.addCommand(new CmdDrawingProjectShape());
}
