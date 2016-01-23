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
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawViewClip.h>
#include <Mod/TechDraw/App/DrawViewAnnotation.h>
#include <Mod/TechDraw/App/DrawViewSymbol.h>
#include <Mod/TechDraw/Gui/QGVPage.h>


#include "MDIViewPage.h"
#include "TaskProjGroup.h"
#include "ViewProviderPage.h"

using namespace TechDrawGui;
using namespace std;

bool isDrawingPageActive(Gui::Document *doc)
{
    if (doc)
        // checks if a Sketch Viewprovider is in Edit and is in no special mode
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom(TechDrawGui::ViewProviderPage::getClassTypeId()))
            return true;
    return false;
}

//===========================================================================
// TechDraw_NewPageDef (default template)
//===========================================================================

DEF_STD_CMD(CmdTechDrawNewPageDef);

CmdTechDrawNewPageDef::CmdTechDrawNewPageDef()
  : Command("TechDraw_NewPageDef")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert new default drawing page");
    sToolTipText    = QT_TR_NOOP("Insert new default drawing page");
    sWhatsThis      = "TechDraw_NewPageDef";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-new-default";
}

void CmdTechDrawNewPageDef::activated(int iMsg)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw");

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
        TechDrawGui::ViewProviderPage* dvp = dynamic_cast<TechDrawGui::ViewProviderPage*>(vp);
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
// TechDraw_NewPage (with template choice)
//===========================================================================

DEF_STD_CMD(CmdTechDrawNewPage);

CmdTechDrawNewPage::CmdTechDrawNewPage()
  : Command("TechDraw_NewPage")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert new drawing page from template");
    sToolTipText    = QT_TR_NOOP("Insert new drawing page from template");
    sWhatsThis      = "TechDraw_NewPage";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-new-pick";
}

void CmdTechDrawNewPage::activated(int iMsg)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw");

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
        TechDrawGui::ViewProviderPage* dvp = dynamic_cast<TechDrawGui::ViewProviderPage*>(vp);
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

//===========================================================================
// TechDraw_NewView
//===========================================================================

DEF_STD_CMD(CmdTechDrawNewView);

CmdTechDrawNewView::CmdTechDrawNewView()
  : Command("TechDraw_NewView")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert view in drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new View of a Part in the active drawing");
    sWhatsThis      = "TechDraw_NewView";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-view";
}

void CmdTechDrawNewView::activated(int iMsg)
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
        doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
        //TechDraw::DrawPage *page = dynamic_cast<TechDraw::DrawPage *>(pages.front());
        //TODO: page->addView sb Python function??
        //page->addView(page->getDocument()->getObject(FeatName.c_str()));
    }
    updateActive();
    commitCommand();
}
//===========================================================================
// TechDraw_NewViewSection
//===========================================================================

DEF_STD_CMD(CmdTechDrawNewViewSection);

CmdTechDrawNewViewSection::CmdTechDrawNewViewSection()
  : Command("TechDraw_NewViewSection")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert section view in drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new Section View of a Part in the active drawing");
    sWhatsThis      = "TechDraw_NewViewSecton";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-viewsection";
}

void CmdTechDrawNewViewSection::activated(int iMsg)
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
        doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
        //TechDraw::DrawPage *page = dynamic_cast<TechDraw::DrawPage *>(pages.front());
        //page->addView(page->getDocument()->getObject(FeatName.c_str()));
    }
    updateActive();
    commitCommand();
}


//===========================================================================
// TechDraw_ProjGroup
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawProjGroup);

CmdTechDrawProjGroup::CmdTechDrawProjGroup()
  : Command("TechDraw_ProjGroup")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
    sMenuText       = QT_TR_NOOP("Insert Projection Group");
    sToolTipText    = QT_TR_NOOP("Insert 2D Projections of a 3D part into the active drawing");
    sWhatsThis      = "TechDraw_ProjGroup";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-projgroup";
}

void CmdTechDrawProjGroup::activated(int iMsg)
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
    std::string PageName = page->getNameInDocument();
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),multiViewName.c_str());
    //page->addView(getDocument()->getObject(multiViewName.c_str()));

    updateActive();
    commitCommand();
}

bool CmdTechDrawProjGroup::isActive(void)
{
    if (Gui::Control().activeDialog())
        return false;
    return true;
}


//===========================================================================
// TechDraw_Annotation
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawAnnotation);

CmdTechDrawAnnotation::CmdTechDrawAnnotation()
  : Command("TechDraw_Annotation")
{
    // setting the Gui eye-candy
    sGroup        = QT_TR_NOOP("Drawing");
    sMenuText     = QT_TR_NOOP("&Annotation");
    sToolTipText  = QT_TR_NOOP("Inserts an Annotation in the active drawing");
    sWhatsThis    = "TechDraw_Annotation";
    sStatusTip    = QT_TR_NOOP("Inserts an Annotation in the active drawing");
    sPixmap       = "actions/techdraw-annotation";
}

void CmdTechDrawAnnotation::activated(int iMsg)
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
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    updateActive();
    commitCommand();
}

bool CmdTechDrawAnnotation::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}


//===========================================================================
// TechDraw_Clip
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawClip);

CmdTechDrawClip::CmdTechDrawClip()
  : Command("TechDraw_Clip")
{
    // seting the
    sGroup        = QT_TR_NOOP("Drawing");
    sMenuText     = QT_TR_NOOP("&Clip");
    sToolTipText  = QT_TR_NOOP("Inserts a clip group in the active drawing");
    sWhatsThis    = "TechDraw_Clip";
    sStatusTip    = QT_TR_NOOP("Inserts a clip group in the active drawing");
    sPixmap       = "actions/techdraw-clip";
}

void CmdTechDrawClip::activated(int iMsg)
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
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    updateActive();
    commitCommand();
}

bool CmdTechDrawClip::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}

//===========================================================================
// TechDraw_ClipPlus
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawClipPlus);

CmdTechDrawClipPlus::CmdTechDrawClipPlus()
  : Command("TechDraw_ClipPlus")
{
    // seting the
    sGroup        = QT_TR_NOOP("Drawing");
    sMenuText     = QT_TR_NOOP("&ClipPlus");
    sToolTipText  = QT_TR_NOOP("Add a View to a clip group in the active drawing");
    sWhatsThis    = "TechDraw_ClipPlus";
    sStatusTip    = QT_TR_NOOP("Adds a View into a clip group in the active drawing");
    sPixmap       = "actions/techdraw-clipplus";
}

void CmdTechDrawClipPlus::activated(int iMsg)
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

    std::string ClipName = clip->getNameInDocument();
    std::string ViewName = view->getNameInDocument();
    openCommand("ClipPlus");
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",ClipName.c_str(),ViewName.c_str());
    updateActive();
    commitCommand();
}

bool CmdTechDrawClipPlus::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}

//===========================================================================
// TechDraw_ClipMinus
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawClipMinus);

CmdTechDrawClipMinus::CmdTechDrawClipMinus()
  : Command("TechDraw_ClipMinus")
{
    sGroup        = QT_TR_NOOP("Drawing");
    sMenuText     = QT_TR_NOOP("&ClipMinus");
    sToolTipText  = QT_TR_NOOP("Remove a View from a clip group in the active drawing");
    sWhatsThis    = "TechDraw_ClipMinus";
    sStatusTip    = QT_TR_NOOP("Remove a View from a clip group in the active drawing");
    sPixmap       = "actions/techdraw-clipminus";
}

void CmdTechDrawClipMinus::activated(int iMsg)
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

    std::string ClipName = clip->getNameInDocument();
    std::string ViewName = view->getNameInDocument();
    openCommand("ClipMinus");
    doCommand(Doc,"App.activeDocument().%s.removeView(App.activeDocument().%s)",ClipName.c_str(),ViewName.c_str());
    updateActive();
    commitCommand();
}

bool CmdTechDrawClipMinus::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}


//===========================================================================
// TechDraw_Symbol
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawSymbol);

CmdTechDrawSymbol::CmdTechDrawSymbol()
  : Command("TechDraw_Symbol")
{
    // setting the Gui eye-candy
    sGroup        = QT_TR_NOOP("Drawing");
    sMenuText     = QT_TR_NOOP("Insert SVG &Symbol");
    sToolTipText  = QT_TR_NOOP("Inserts a symbol from a svg file in the active drawing");
    sWhatsThis    = "TechDraw_Symbol";
    sStatusTip    = QT_TR_NOOP("Inserts a symbol from a svg file in the active drawing");
    sPixmap       = "actions/techdraw-symbol";
}

void CmdTechDrawSymbol::activated(int iMsg)
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
        doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
        updateActive();
        commitCommand();
    }
}

bool CmdTechDrawSymbol::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}


//===========================================================================
// TechDraw_ExportPage
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExportPage);

CmdTechDrawExportPage::CmdTechDrawExportPage()
  : Command("TechDraw_ExportPage")
{
    // seting the
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("&Export page...");
    sToolTipText  = QT_TR_NOOP("Export a page to an SVG file");
    sWhatsThis    = "TechDraw_ExportPage";
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
    ViewProviderPage* dvp = dynamic_cast<ViewProviderPage*>(vp);

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


void CreateTechDrawCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawNewPageDef());
    rcCmdMgr.addCommand(new CmdTechDrawNewPage());
    rcCmdMgr.addCommand(new CmdTechDrawNewView());
    rcCmdMgr.addCommand(new CmdTechDrawNewViewSection());
    rcCmdMgr.addCommand(new CmdTechDrawProjGroup());
    rcCmdMgr.addCommand(new CmdTechDrawAnnotation());
    rcCmdMgr.addCommand(new CmdTechDrawClip());
    rcCmdMgr.addCommand(new CmdTechDrawClipPlus());
    rcCmdMgr.addCommand(new CmdTechDrawClipMinus());
    rcCmdMgr.addCommand(new CmdTechDrawSymbol());
    rcCmdMgr.addCommand(new CmdTechDrawExportPage());
}
