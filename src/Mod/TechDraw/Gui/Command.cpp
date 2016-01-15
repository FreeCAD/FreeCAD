/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   for detail see the LICENCE text file.                                 *
 *   Jürgen Riegel 2002                                                    *
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <sstream>
# include <QCoreApplication>
# include <QDir>
# include <QFile>
# include <QFileInfo>
# include <QMessageBox>
# include <QRegExp>
#endif

#include <QStringBuilder>

#include <QGraphicsView>
#include <QPainter>
#include <QSvgRenderer>
#include <QSvgGenerator>

#include <vector>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyGeo.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Drawing/App/DrawPage.h>
#include <Mod/Drawing/App/DrawViewPart.h>
#include <Mod/Drawing/App/DrawProjGroupItem.h>
#include <Mod/Drawing/App/DrawProjGroup.h>
#include <Mod/Drawing/App/DrawViewDimension.h>
#include <Mod/Drawing/App/DrawViewClip.h>
#include <Mod/Drawing/App/DrawViewAnnotation.h>
#include <Mod/Drawing/App/DrawViewSymbol.h>
#include <Mod/Drawing/Gui/QGVPage.h>


#include "MDIViewPage.h"
#include "TaskDialog.h"
#include "TaskProjGroup.h"
#include "ViewProviderPage.h"

using namespace TechDrawGui;
using namespace std;

bool isDrawingPageActive(Gui::Document *doc)
{
    if (doc)
        // checks if a Sketch Viewprovider is in Edit and is in no special mode
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom(TechDrawGui::ViewProviderDrawingPage::getClassTypeId()))
            return true;
    return false;
}

//TODO: check if obsolete.
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
        QString::fromLatin1("%1 (*.svg *.svgz)").arg(QObject::tr("Scalable Vector Graphic")));
    if (!filename.isEmpty())
    {
        // load the file with the module
        Command::doCommand(Command::Gui, "import Drawing, TechDrawGui");
        Command::doCommand(Command::Gui, "TechDrawGui.open(unicode(\"%s\",\"utf-8\"))", (const char*)filename.toUtf8());
    }
}

//===========================================================================
// Drawing_NewPageDef (default template)
//===========================================================================

DEF_STD_CMD(CmdDrawingNewPageDef);

CmdDrawingNewPageDef::CmdDrawingNewPageDef()
  : Command("Drawing_NewPageDef")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert new default drawing page");
    sToolTipText    = QT_TR_NOOP("Insert new default drawing page");
    sWhatsThis      = "Drawing_NewPageDef";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-new-default";
}

void CmdDrawingNewPageDef::activated(int iMsg)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Drawing");

    std::string defaultDir = App::Application::getResourceDir() + "Mod/Drawing/Templates";
    QString templateDir = QString::fromStdString(hGrp->GetASCII("TemplateDir", defaultDir.c_str()));
    if (templateDir.isEmpty()) {                                         //preference key can be present, but have null value
        templateDir = QString::fromStdString(defaultDir);
    }
    std::string defaultFileName = "A4_Landscape.svg";
    QString templateFileName = QString::fromStdString(hGrp->GetASCII("TemplateFile",defaultFileName.c_str()));
    if (templateFileName.isEmpty()) {
        templateFileName = QString::fromStdString(defaultFileName);
    }
    templateFileName = templateDir + QString::fromUtf8("/")  + templateFileName;

    std::string PageName = getUniqueObjectName("Page");
    std::string TemplateName = getUniqueObjectName("Template");

    QFileInfo tfi(templateFileName);
    if (tfi.isReadable()) {
        Gui::WaitCursor wc;
        openCommand("Drawing create page");
        doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawPage','%s')",PageName.c_str());

        // Create the Template Object to attach to the page
        doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawSVGTemplate','%s')",TemplateName.c_str());

        //TODO: why is "Template" property set twice?
        doCommand(Doc,"App.activeDocument().%s.Template = '%s'",TemplateName.c_str(), templateFileName.toStdString().c_str());
        doCommand(Doc,"App.activeDocument().%s.Template = App.activeDocument().%s",PageName.c_str(),TemplateName.c_str());

        commitCommand();
        TechDraw::DrawPage* fp = dynamic_cast<TechDraw::DrawPage*>(getDocument()->getObject(PageName.c_str()));
        Gui::ViewProvider* vp = Gui::Application::Instance->getDocument(getDocument())->getViewProvider(fp);
        TechDrawGui::ViewProviderDrawingPage* dvp = dynamic_cast<TechDrawGui::ViewProviderDrawingPage*>(vp);
        if (dvp) {
            dvp->show();
        }
        else {
            Base::Console().Log("INFO - Template: %s for Page: %s NOT Found\n", PageName.c_str(),TemplateName.c_str());
        }
    }
    else {
        QMessageBox::critical(Gui::getMainWindow(),
            QLatin1String("No template"),
            QLatin1String("No default template found"));
    }
}

//===========================================================================
// Drawing_NewPage (with template choice)
//===========================================================================

DEF_STD_CMD(CmdDrawingNewPage);

CmdDrawingNewPage::CmdDrawingNewPage()
  : Command("Drawing_NewPage")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert new drawing page from template");
    sToolTipText    = QT_TR_NOOP("Insert new drawing page from template");
    sWhatsThis      = "Drawing_NewPage";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-new-pick";
}

void CmdDrawingNewPage::activated(int iMsg)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Drawing");

    std::string defaultDir = App::Application::getResourceDir() + "Mod/Drawing/Templates";
    QString templateDir = QString::fromStdString(hGrp->GetASCII("TemplateDir", defaultDir.c_str()));

    //TODO: Some templates are too complex for regex?
    QString templateFileName = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(),
                                                   QString::fromUtf8(QT_TR_NOOP("Select a Template File")),
                                                   templateDir,
                                                   QString::fromUtf8(QT_TR_NOOP("Template (*.svg *.dxf)")));

    if (templateFileName.isEmpty()) {
        QMessageBox::critical(Gui::getMainWindow(),
            QLatin1String("No template"),
            QLatin1String("Must select a valid template file"));
        return;
    }

    std::string PageName = getUniqueObjectName("Page");
    std::string TemplateName = getUniqueObjectName("Template");

    QFileInfo tfi(templateFileName);
    if (tfi.isReadable()) {
        Gui::WaitCursor wc;
        openCommand("Drawing create page");
        doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawPage','%s')",PageName.c_str());

        // Create the Template Object to attach to the page
        doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawSVGTemplate','%s')",TemplateName.c_str());

        //why is "Template" property set twice? -wf
        // once to set DrawSVGTemplate.Template to OS template file name
        doCommand(Doc,"App.activeDocument().%s.Template = '%s'",TemplateName.c_str(), templateFileName.toStdString().c_str());
        // once to set Page.Template to DrawSVGTemplate.Name
        doCommand(Doc,"App.activeDocument().%s.Template = App.activeDocument().%s",PageName.c_str(),TemplateName.c_str());
        // consider renaming DrawSVGTemplate.Template property?

        commitCommand();
        TechDraw::DrawPage* fp = dynamic_cast<TechDraw::DrawPage*>(getDocument()->getObject(PageName.c_str()));
        Gui::ViewProvider* vp = Gui::Application::Instance->getDocument(getDocument())->getViewProvider(fp);
        TechDrawGui::ViewProviderDrawingPage* dvp = dynamic_cast<TechDrawGui::ViewProviderDrawingPage*>(vp);
        if (dvp) {
            dvp->show();
        }
        else {
            Base::Console().Log("INFO - Template: %s for Page: %s NOT Found\n", PageName.c_str(),TemplateName.c_str());
        }
    }
    else {
        QMessageBox::critical(Gui::getMainWindow(),
            QLatin1String("No template"),
            QLatin1String("Template file is invalid"));
    }
}

//TODO: check if obsolete
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
    sPixmap         = "actions/techdraw-landscape-A3";
}

void CmdDrawingNewA3Landscape::activated(int iMsg)
{
    std::string FeatName = getUniqueObjectName("Page");

    openCommand("Create page");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawPage','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Template = 'A3_Landscape.svg'",FeatName.c_str());
//    doCommand(Doc,"App.activeDocument().recompute()");
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
    sPixmap         = "actions/techdraw-view";
}

void CmdDrawingNewView::activated(int iMsg)
{
    std::vector<App::DocumentObject*> shapes = getSelection().getObjectsOfType(Part::Feature::getClassTypeId());
    if (shapes.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select a Part object."));
        return;
    }

//    std::vector<App::DocumentObject*> pages = getSelection().getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    std::vector<App::DocumentObject*> pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()) {
        //pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
        //if (pages.empty()){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No page found"),
            QObject::tr("Create a page first."));
        return;
        //}
    }

    Gui::WaitCursor wc;
    const std::vector<App::DocumentObject*> selectedProjections = getSelection().getObjectsOfType(TechDraw::DrawView::getClassTypeId());
    float newX = 10.0;
    float newY = 10.0;
    float newScale = 1.0;
    float newRotation = 0.0;
    Base::Vector3d newDirection(0.0, 0.0, 1.0);
    if (!selectedProjections.empty()) {
        const TechDraw::DrawView* const myView = dynamic_cast<TechDraw::DrawView*>(selectedProjections.front());

        newX = myView->X.getValue();
        newY = myView->Y.getValue();
        newScale = myView->Scale.getValue();
        newRotation = myView->Rotation.getValue();

        // The "Direction" property does not belong to TechDraw::DrawView, but to one of the
        // many child classes that are projecting objects into the drawing. Therefore, we get the
        // property by name.
        const App::PropertyVector* const propDirection = dynamic_cast<App::PropertyVector*>(myView->getPropertyByName("Direction"));
        if (propDirection) {
            newDirection = propDirection->getValue();
        }
    }

    std::string PageName = pages.front()->getNameInDocument();

    openCommand("Create view");
    for (std::vector<App::DocumentObject*>::iterator it = shapes.begin(); it != shapes.end(); ++it) {
        std::string FeatName = getUniqueObjectName("View");
        doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewPart','%s')",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Source = App.activeDocument().%s",FeatName.c_str(),(*it)->getNameInDocument());
        doCommand(Doc,"App.activeDocument().%s.Direction = (%e,%e,%e)",FeatName.c_str(), newDirection.x, newDirection.y, newDirection.z);
        doCommand(Doc,"App.activeDocument().%s.X = %e",FeatName.c_str(), newX);
        doCommand(Doc,"App.activeDocument().%s.Y = %e",FeatName.c_str(), newY);
        doCommand(Doc,"App.activeDocument().%s.Scale = %e",FeatName.c_str(), newScale);
        doCommand(Doc,"App.activeDocument().%s.Rotation = %e",FeatName.c_str(), newRotation);
        //doCommand(Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
        TechDraw::DrawPage *page = dynamic_cast<TechDraw::DrawPage *>(pages.front());
        //TODO: page->addView sb Python function??
        page->addView(page->getDocument()->getObject(FeatName.c_str()));
    }
    updateActive();
    commitCommand();
}
//===========================================================================
// Drawing_NewViewSection
//===========================================================================

DEF_STD_CMD(CmdDrawingNewViewSection);

CmdDrawingNewViewSection::CmdDrawingNewViewSection()
  : Command("Drawing_NewViewSection")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert section view in drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new Section View of a Part in the active drawing");
    sWhatsThis      = "Drawing_NewViewSecton";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-viewsection";
}

void CmdDrawingNewViewSection::activated(int iMsg)
{
    std::vector<App::DocumentObject*> shapes = getSelection().getObjectsOfType(Part::Feature::getClassTypeId());
    if (shapes.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select a Part object."));
        return;
    }

    std::vector<App::DocumentObject*> pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No page to insert"),
            QObject::tr("Create a page to insert."));
        return;
    }

    std::string PageName = pages.front()->getNameInDocument();

    openCommand("Create view");
    for (std::vector<App::DocumentObject*>::iterator it = shapes.begin(); it != shapes.end(); ++it) {
        std::string FeatName = getUniqueObjectName("View");
        doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewSection','%s')",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Source = App.activeDocument().%s",FeatName.c_str(),(*it)->getNameInDocument());
        doCommand(Doc,"App.activeDocument().%s.Direction = (0.0,0.0,1.0)",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.X = 10.0",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Y = 10.0",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Scale = 1.0",FeatName.c_str());
//         doCommand(Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)",PageName.c_str(),);
        TechDraw::DrawPage *page = dynamic_cast<TechDraw::DrawPage *>(pages.front());
        page->addView(page->getDocument()->getObject(FeatName.c_str()));
    }
    updateActive();
    commitCommand();
}


//===========================================================================
// Drawing_ProjGroup
//===========================================================================

DEF_STD_CMD_A(CmdDrawingProjGroup);

CmdDrawingProjGroup::CmdDrawingProjGroup()
  : Command("Drawing_ProjGroup")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert Projection Group");
    sToolTipText    = QT_TR_NOOP("Insert 2D Projections of a 3D part into the active drawing");
    sWhatsThis      = "Drawing_ProjGroup";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-projgroup";
}

void CmdDrawingProjGroup::activated(int iMsg)
{
    // Check that a Part::Feature is in the Selection
    std::vector<App::DocumentObject*> shapes = getSelection().getObjectsOfType(Part::Feature::getClassTypeId());
    if (shapes.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one Part object."));
        return;
    }

    // Check if a Drawing Page is in the Selection.
    TechDraw::DrawPage *page;
    const std::vector<App::DocumentObject*> selPages = getSelection().getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (!selPages.empty()) {
        page = dynamic_cast<TechDraw::DrawPage *>(selPages.front());
    } else {
        // Check that any page object exists in Document
        const std::vector<App::DocumentObject*> docPages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
        if (!docPages.empty()) {
            page = dynamic_cast<TechDraw::DrawPage *>(docPages.front());
        } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No Drawing Page found"),
                                 QObject::tr("Create a Drawing Page first."));
            return;
        }
    }
// TODO: is there a way to use "Active Page" instead of pages.front? if a second page is in the document, we will always
//       use page#1 if there isn't a page in the selection.

    openCommand("Create Projection Group");
    std::string multiViewName = getUniqueObjectName("cView");
    std::string SourceName = (*shapes.begin())->getNameInDocument();
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawProjGroup','%s')",multiViewName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Source = App.activeDocument().%s",multiViewName.c_str(),SourceName.c_str());
    doCommand(Doc,"App.activeDocument().%s.X = %f",     multiViewName.c_str(), page->getPageWidth() / 2);
    doCommand(Doc,"App.activeDocument().%s.Y = %f",     multiViewName.c_str(), page->getPageHeight() / 2);
    doCommand(Doc,"App.activeDocument().%s.Scale = 1.0",multiViewName.c_str());

    App::DocumentObject *docObj = getDocument()->getObject(multiViewName.c_str());
    TechDraw::DrawProjGroup *multiView = dynamic_cast<TechDraw::DrawProjGroup *>(docObj);

    // set the anchor
    App::DocumentObject* anchorView = multiView->addProjection("Front");
    std::string anchorName = anchorView->getNameInDocument();
    doCommand(Doc,"App.activeDocument().%s.Anchor = App.activeDocument().%s",multiViewName.c_str(),anchorName.c_str());

    // create the rest of the desired views
    Gui::Control().showDialog(new TaskDlgProjGroup(multiView));

    // add the multiView to the page
    page->addView(getDocument()->getObject(multiViewName.c_str()));

    updateActive();
    commitCommand();
}

bool CmdDrawingProjGroup::isActive(void)
{
    if (Gui::Control().activeDialog())
        return false;
    return true;
}


//===========================================================================
// Drawing_OpenBrowserView
//===========================================================================

DEF_STD_CMD_A(CmdDrawingOpenBrowserView);

CmdDrawingOpenBrowserView::CmdDrawingOpenBrowserView()
  : Command("Drawing_OpenBrowserView")
{
    // seting the
    sGroup        = QT_TR_NOOP("Drawing");
    sMenuText     = QT_TR_NOOP("Open &browser view");
    sToolTipText  = QT_TR_NOOP("Opens the selected page in a browser view");
    sWhatsThis    = "Drawing_OpenBrowserView";
    sStatusTip    = QT_TR_NOOP("Opens the selected page in a browser view");
    sPixmap       = "actions/techdraw-openbrowser";
}

void CmdDrawingOpenBrowserView::activated(int iMsg)
{
    unsigned int n = getSelection().countObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (n != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select one Page object."));
        return;
    }
    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();
    doCommand(Doc,"PageName = App.activeDocument().%s.PageResult",Sel[0].FeatName);
    doCommand(Doc,"import WebGui");
    doCommand(Doc,"WebGui.openBrowser(PageName)");
}

bool CmdDrawingOpenBrowserView::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}

//===========================================================================
// Drawing_Annotation
//===========================================================================

DEF_STD_CMD_A(CmdDrawingAnnotation);

CmdDrawingAnnotation::CmdDrawingAnnotation()
  : Command("Drawing_Annotation")
{
    // setting the Gui eye-candy
    sGroup        = QT_TR_NOOP("Drawing");
    sMenuText     = QT_TR_NOOP("&Annotation");
    sToolTipText  = QT_TR_NOOP("Inserts an Annotation in the active drawing");
    sWhatsThis    = "Drawing_Annotation";
    sStatusTip    = QT_TR_NOOP("Inserts an Annotation in the active drawing");
    sPixmap       = "actions/techdraw-annotation";
}

void CmdDrawingAnnotation::activated(int iMsg)
{
//    std::vector<App::DocumentObject*> pages = getSelection().getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    std::vector<App::DocumentObject*> pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()) {
          QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No page found"),
              QObject::tr("Create a page first."));
          return;
    }
    std::string PageName = pages.front()->getNameInDocument();
    std::string FeatName = getUniqueObjectName("Annotation");
    openCommand("Create Annotation");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewAnnotation','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.X = 10.0",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Y = 10.0",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Scale = 7.0",FeatName.c_str());
    //doCommand(Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    TechDraw::DrawPage *page = dynamic_cast<TechDraw::DrawPage *>(pages.front());
    page->addView(page->getDocument()->getObject(FeatName.c_str()));
    updateActive();
    commitCommand();
}

bool CmdDrawingAnnotation::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}


//===========================================================================
// Drawing_Clip
//===========================================================================

DEF_STD_CMD_A(CmdDrawingClip);

CmdDrawingClip::CmdDrawingClip()
  : Command("Drawing_Clip")
{
    // seting the
    sGroup        = QT_TR_NOOP("Drawing");
    sMenuText     = QT_TR_NOOP("&Clip");
    sToolTipText  = QT_TR_NOOP("Inserts a clip group in the active drawing");
    sWhatsThis    = "Drawing_Clip";
    sStatusTip    = QT_TR_NOOP("Inserts a clip group in the active drawing");
    sPixmap       = "actions/techdraw-clip";
}

void CmdDrawingClip::activated(int iMsg)
{

//    std::vector<App::DocumentObject*> pages = getSelection().getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    std::vector<App::DocumentObject*> pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No page found"),
            QObject::tr("Create a page first."));
        return;
    }

    std::string PageName = pages.front()->getNameInDocument();
    std::string FeatName = getUniqueObjectName("Clip");
    openCommand("Create Clip");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewClip','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.ShowFrame = True",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Height = 30.0",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Width = 30.0",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.ShowLabels = False",FeatName.c_str());
    TechDraw::DrawPage *page = dynamic_cast<TechDraw::DrawPage *>(pages.front());
    page->addView(page->getDocument()->getObject(FeatName.c_str()));
    updateActive();
    commitCommand();
}

bool CmdDrawingClip::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}

//===========================================================================
// Drawing_ClipPlus
//===========================================================================

DEF_STD_CMD_A(CmdDrawingClipPlus);

CmdDrawingClipPlus::CmdDrawingClipPlus()
  : Command("Drawing_ClipPlus")
{
    // seting the
    sGroup        = QT_TR_NOOP("Drawing");
    sMenuText     = QT_TR_NOOP("&ClipPlus");
    sToolTipText  = QT_TR_NOOP("Add a View to a clip group in the active drawing");
    sWhatsThis    = "Drawing_ClipPlus";
    sStatusTip    = QT_TR_NOOP("Adds a View into a clip group in the active drawing");
    sPixmap       = "actions/techdraw-clipplus";
}

void CmdDrawingClipPlus::activated(int iMsg)
{

    std::vector<App::DocumentObject*> pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No page found"),
            QObject::tr("Create a page first."));
        return;
    }

    std::vector<App::DocumentObject*> views = getSelection().getObjectsOfType(TechDraw::DrawView::getClassTypeId());
    if (views.size() != 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one Clip and one View object."));
        return;
    }

    TechDraw::DrawViewClip* clip;
    TechDraw::DrawView* view;
    std::vector<App::DocumentObject*>::iterator it = views.begin();
    for (; it != views.end(); it++) {
        if ((*it)->isDerivedFrom(TechDraw::DrawViewClip::getClassTypeId())) {
            clip = dynamic_cast<TechDraw::DrawViewClip*> ((*it));
        } else {
            view = dynamic_cast<TechDraw::DrawView*> ((*it));
        }
    }

    if (!view) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one Drawing View object."));
        return;
    }
    if (!clip) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one Clip object."));
        return;
    }

    openCommand("ClipPlus");
    clip->addView(view);
    updateActive();
    commitCommand();
}

bool CmdDrawingClipPlus::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}

//===========================================================================
// Drawing_ClipMinus
//===========================================================================

DEF_STD_CMD_A(CmdDrawingClipMinus);

CmdDrawingClipMinus::CmdDrawingClipMinus()
  : Command("Drawing_ClipMinus")
{
    sGroup        = QT_TR_NOOP("Drawing");
    sMenuText     = QT_TR_NOOP("&ClipMinus");
    sToolTipText  = QT_TR_NOOP("Remove a View from a clip group in the active drawing");
    sWhatsThis    = "Drawing_ClipMinus";
    sStatusTip    = QT_TR_NOOP("Remove a View from a clip group in the active drawing");
    sPixmap       = "actions/techdraw-clipminus";
}

void CmdDrawingClipMinus::activated(int iMsg)
{

    std::vector<App::DocumentObject*> pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No page found"),
            QObject::tr("Create a page first."));
        return;
    }

    std::vector<App::DocumentObject*> views = getSelection().getObjectsOfType(TechDraw::DrawView::getClassTypeId());
    if (views.size() != 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one Clip and one View object."));
        return;
    }

    TechDraw::DrawViewClip* clip;
    TechDraw::DrawView* view;
    std::vector<App::DocumentObject*>::iterator it = views.begin();
    for (; it != views.end(); it++) {
        if ((*it)->isDerivedFrom(TechDraw::DrawViewClip::getClassTypeId())) {
            clip = dynamic_cast<TechDraw::DrawViewClip*> ((*it));
        } else {
            view = dynamic_cast<TechDraw::DrawView*> ((*it));
        }
    }

    if (!view) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one Drawing View object."));
        return;
    }
    if (!clip) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one Clip object."));
        return;
    }

    openCommand("ClipMinus");
    clip->removeView(view);
    updateActive();
    commitCommand();
}

bool CmdDrawingClipMinus::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}


//===========================================================================
// Drawing_Symbol
//===========================================================================

DEF_STD_CMD_A(CmdDrawingSymbol);

CmdDrawingSymbol::CmdDrawingSymbol()
  : Command("Drawing_Symbol")
{
    // setting the Gui eye-candy
    sGroup        = QT_TR_NOOP("Drawing");
    sMenuText     = QT_TR_NOOP("Insert SVG &Symbol");
    sToolTipText  = QT_TR_NOOP("Inserts a symbol from a svg file in the active drawing");
    sWhatsThis    = "Drawing_Symbol";
    sStatusTip    = QT_TR_NOOP("Inserts a symbol from a svg file in the active drawing");
    sPixmap       = "actions/techdraw-symbol";
}

void CmdDrawingSymbol::activated(int iMsg)
{
//    std::vector<App::DocumentObject*> pages = getSelection().getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    std::vector<App::DocumentObject*> pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No page found"),
            QObject::tr("Create a page first."));
        return;
    }
    // Reading an image
    QString filename = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(), QObject::tr("Choose an SVG file to open"), QString::null,
        QString::fromLatin1("%1 (*.svg *.svgz)").arg(QObject::tr("Scalable Vector Graphic")));
    if (!filename.isEmpty())
    {
        std::string PageName = pages.front()->getNameInDocument();
        std::string FeatName = getUniqueObjectName("Symbol");
        openCommand("Create Symbol");
        doCommand(Doc,"import Drawing");
        doCommand(Doc,"f = open(unicode(\"%s\",'utf-8'),'r')",(const char*)filename.toUtf8());
        doCommand(Doc,"svg = f.read()");
        doCommand(Doc,"f.close()");
        doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewSymbol','%s')",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.X = 10.0",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Y = 10.0",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Symbol = svg",FeatName.c_str());
        //doCommand(Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
        TechDraw::DrawPage *page = dynamic_cast<TechDraw::DrawPage *>(pages.front());
        page->addView(page->getDocument()->getObject(FeatName.c_str()));
        updateActive();
        commitCommand();
    }
}

bool CmdDrawingSymbol::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}


//===========================================================================
// Drawing_ExportPage
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExportPage);

CmdTechDrawExportPage::CmdTechDrawExportPage()
  : Command("Drawing_ExportPage")
{
    // seting the
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("&Export page...");
    sToolTipText  = QT_TR_NOOP("Export a page to an SVG file");
    sWhatsThis    = "Drawing_ExportPage";
    sStatusTip    = QT_TR_NOOP("Export a page to an SVG file");
    sPixmap       = "actions/saveSVG";
}

void CmdTechDrawExportPage::activated(int iMsg)
{
    std::vector<App::DocumentObject*> pages = getSelection().getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()) {                                   // no Pages in Selection
        pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
        if (pages.empty()) {                               // no Pages in Document
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No pages found"),
                                 QObject::tr("Create a drawing page first."));
            return;
        }
    }

    unsigned int n = getSelection().countObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (n > 1) {                                          // too many Pages
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select only one Page object."));
        return;
    }

    TechDraw::DrawPage* page = dynamic_cast<TechDraw::DrawPage*>(pages.front());

    Gui::Document* activeGui = Gui::Application::Instance->getDocument(page->getDocument());
    Gui::ViewProvider* vp = activeGui->getViewProvider(page);
    ViewProviderDrawingPage* dvp = dynamic_cast<ViewProviderDrawingPage*>(vp);

    if (dvp  && dvp->getMDIViewPage()) {
        dvp->getMDIViewPage()->saveSVG();
    } else {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No Drawing View"),
            QObject::tr("Open Drawing View before attempting export to SVG."));
        return;
    }

}

bool CmdTechDrawExportPage::isActive(void)
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
        dlg = new TechDrawGui::TaskProjection();
        dlg->setButtonPosition(Gui::TaskView::TaskDialog::South);
    }
    Gui::Control().showDialog(dlg);
}

bool CmdDrawingProjectShape::isActive(void)
{
    int ct = Gui::Selection().countObjectsOfType(Part::Feature::getClassTypeId());
    return (ct > 0 && !Gui::Control().activeDialog());
}



//===========================================================================
// Drawing_Draft_View
//===========================================================================

DEF_STD_CMD_A(CmdDrawingDraftView);

CmdDrawingDraftView::CmdDrawingDraftView()
  : Command("Drawing_DraftView")
{
    // seting the
    sGroup        = QT_TR_NOOP("Drawing");
    sMenuText     = QT_TR_NOOP("&Draft View");
    sToolTipText  = QT_TR_NOOP("Inserts a Draft view of the selected object(s) in the active drawing");
    sWhatsThis    = "Drawing_DraftView";
    sStatusTip    = QT_TR_NOOP("Inserts a Draft view of the selected object(s) in the active drawing");
    sPixmap       = "actions/techdraw-draft-view";
}

void CmdDrawingDraftView::activated(int iMsg)
{
    addModule(Gui,"Draft");
    doCommand(Gui,"Gui.runCommand(\"Draft_Drawing\")");
}

bool CmdDrawingDraftView::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}

void CreateDrawingCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

//    rcCmdMgr.addCommand(new CmdDrawingOpen());
    rcCmdMgr.addCommand(new CmdDrawingNewPageDef());
    rcCmdMgr.addCommand(new CmdDrawingNewPage());
    rcCmdMgr.addCommand(new CmdDrawingNewA3Landscape());
    rcCmdMgr.addCommand(new CmdDrawingNewView());
    rcCmdMgr.addCommand(new CmdDrawingNewViewSection());
    rcCmdMgr.addCommand(new CmdDrawingProjGroup());
//    rcCmdMgr.addCommand(new CmdDrawingOpenBrowserView());
    rcCmdMgr.addCommand(new CmdDrawingAnnotation());
    rcCmdMgr.addCommand(new CmdDrawingClip());
    rcCmdMgr.addCommand(new CmdDrawingClipPlus());
    rcCmdMgr.addCommand(new CmdDrawingClipMinus());
    rcCmdMgr.addCommand(new CmdDrawingSymbol());
    rcCmdMgr.addCommand(new CmdTechDrawExportPage());
    rcCmdMgr.addCommand(new CmdDrawingProjectShape());
    rcCmdMgr.addCommand(new CmdDrawingDraftView());
}
