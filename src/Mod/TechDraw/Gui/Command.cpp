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

#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>
#include <App/FeaturePython.h>
#include <App/PropertyGeo.h>
#include <App/GeoFeature.h>
#include <Base/Console.h>
#include <Base/Exception.h>
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
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Spreadsheet/App/Sheet.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawViewClip.h>
#include <Mod/TechDraw/App/DrawViewSymbol.h>
#include <Mod/TechDraw/App/DrawViewDraft.h>
#include <Mod/TechDraw/App/DrawViewMulti.h>
#include <Mod/TechDraw/App/DrawViewDetail.h>
#include <Mod/TechDraw/App/DrawViewArch.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/Gui/QGVPage.h>

#include "DrawGuiUtil.h"
#include "MDIViewPage.h"
#include "TaskProjGroup.h"
#include "TaskSectionView.h"
#include "TaskActiveView.h"
#include "ViewProviderPage.h"

using namespace TechDrawGui;
using namespace std;


//===========================================================================
// TechDraw_PageDefault
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawPageDefault)

CmdTechDrawPageDefault::CmdTechDrawPageDefault()
  : Command("TechDraw_PageDefault")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Default Page");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_PageDefault";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-PageDefault";
}

void CmdTechDrawPageDefault::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Files");

    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Templates/";
    std::string defaultFileName = defaultDir + "A4_LandscapeTD.svg";
    QString templateFileName = QString::fromStdString(hGrp->GetASCII("TemplateFile",defaultFileName.c_str()));
    if (templateFileName.isEmpty()) {
        templateFileName = QString::fromStdString(defaultFileName);
    }

    std::string PageName = getUniqueObjectName("Page");
    std::string TemplateName = getUniqueObjectName("Template");

    QFileInfo tfi(templateFileName);
    if (tfi.isReadable()) {
        Gui::WaitCursor wc;
        openCommand("Drawing create page");
        doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawPage','%s')",PageName.c_str());
        doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawSVGTemplate','%s')",TemplateName.c_str());

        doCommand(Doc,"App.activeDocument().%s.Template = '%s'",TemplateName.c_str(), templateFileName.toStdString().c_str());
        doCommand(Doc,"App.activeDocument().%s.Template = App.activeDocument().%s",PageName.c_str(),TemplateName.c_str());

        commitCommand();
        TechDraw::DrawPage* fp = dynamic_cast<TechDraw::DrawPage*>(getDocument()->getObject(PageName.c_str()));
        if (!fp) {
            throw Base::TypeError("CmdTechDrawPageDefault fp not found\n");
        }

        Gui::ViewProvider* vp = Gui::Application::Instance->getDocument(getDocument())->getViewProvider(fp);
        TechDrawGui::ViewProviderPage* dvp = dynamic_cast<TechDrawGui::ViewProviderPage*>(vp);
        if (dvp) {
            dvp->show();
        }
        else {
            Base::Console().Log("INFO - Template: %s for Page: %s NOT Found\n", PageName.c_str(),TemplateName.c_str());
        }
    } else {
        QMessageBox::critical(Gui::getMainWindow(),
            QLatin1String("No template"),
            QLatin1String("No default template found"));
    }
}

bool CmdTechDrawPageDefault::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// TechDraw_PageTemplate
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawPageTemplate)

CmdTechDrawPageTemplate::CmdTechDrawPageTemplate()
  : Command("TechDraw_PageTemplate")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Page using Template");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_PageTemplate";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-PageTemplate";
}

void CmdTechDrawPageTemplate::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Files");

    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Templates";
    QString templateDir = QString::fromStdString(hGrp->GetASCII("TemplateDir", defaultDir.c_str()));
    QString templateFileName = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(),
                                                   QString::fromUtf8(QT_TR_NOOP("Select a Template File")),
                                                   templateDir,
                                                   QString::fromUtf8(QT_TR_NOOP("Template (*.svg *.dxf)")));

    if (templateFileName.isEmpty()) {
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
        templateFileName = Base::Tools::escapeEncodeFilename(templateFileName);
        doCommand(Doc,"App.activeDocument().%s.Template = \"%s\"",TemplateName.c_str(), templateFileName.toUtf8().constData());
        // once to set Page.Template to DrawSVGTemplate.Name
        doCommand(Doc,"App.activeDocument().%s.Template = App.activeDocument().%s",PageName.c_str(),TemplateName.c_str());
        // consider renaming DrawSVGTemplate.Template property?

        commitCommand();
        TechDraw::DrawPage* fp = dynamic_cast<TechDraw::DrawPage*>(getDocument()->getObject(PageName.c_str()));
        if (!fp) {
            throw Base::TypeError("CmdTechDrawNewPagePick fp not found\n");
        }
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

bool CmdTechDrawPageTemplate::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// TechDraw_RedrawPage
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawRedrawPage)

CmdTechDrawRedrawPage::CmdTechDrawRedrawPage()
  : Command("TechDraw_RedrawPage")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Redraw Page");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_RedrawPage";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-RedrawPage";
}

void CmdTechDrawRedrawPage::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    Gui::WaitCursor wc;

    page->redrawCommand();
}

bool CmdTechDrawRedrawPage::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this,false);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_View
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawView)

CmdTechDrawView::CmdTechDrawView()
  : Command("TechDraw_View")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert View");
    sToolTipText    = QT_TR_NOOP("Insert a View");
    sWhatsThis      = "TechDraw_View";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-View";
}

void CmdTechDrawView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    std::string PageName = page->getNameInDocument();

    //set projection direction from selected Face
    //use first object with a face selected
    std::vector<App::DocumentObject*> shapes;
    App::DocumentObject* partObj = nullptr;
    std::string faceName;
    int resolve = 1;                                //mystery
    bool single = false;                            //mystery
    auto selection = getSelection().getSelectionEx(0,
                                                   App::DocumentObject::getClassTypeId(),
                                                   resolve,
                                                   single);
    for (auto& sel: selection) {
        auto obj = sel.getObject();
        if (obj->isDerivedFrom(TechDraw::DrawPage::getClassTypeId()) ) {
            continue;
        }
        if (obj != nullptr) {                       //can this happen?
            shapes.push_back(obj);
        }
        if(partObj != nullptr) {
            continue;
        }
        for(auto& sub : sel.getSubNames()) {
            if (TechDraw::DrawUtil::getGeomTypeFromName(sub) == "Face") {
                faceName = sub;
                partObj = obj;
                break;
            }
        }
    }

    if ((shapes.empty())) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("No Shapes or Groups in this selection"));
        return;
    }

    Base::Vector3d projDir;

    Gui::WaitCursor wc;
    openCommand("Create view");
    std::string FeatName = getUniqueObjectName("View");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewPart','%s')",FeatName.c_str());
    App::DocumentObject *docObj = getDocument()->getObject(FeatName.c_str());
    TechDraw::DrawViewPart* dvp = dynamic_cast<TechDraw::DrawViewPart *>(docObj);
    if (!dvp) {
        throw Base::TypeError("CmdTechDrawView DVP not found\n");
    }
    dvp->Source.setValues(shapes);
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    if (faceName.size()) {
        std::pair<Base::Vector3d,Base::Vector3d> dirs = DrawGuiUtil::getProjDirFromFace(partObj,faceName);
        projDir = dirs.first;
        getDocument()->setStatus(App::Document::Status::SkipRecompute, true);
        doCommand(Doc,"App.activeDocument().%s.Direction = FreeCAD.Vector(%.3f,%.3f,%.3f)",
                  FeatName.c_str(), projDir.x,projDir.y,projDir.z);
        doCommand(Doc,"App.activeDocument().%s.recompute()", FeatName.c_str());
        getDocument()->setStatus(App::Document::Status::SkipRecompute, false);
   } else {
        std::pair<Base::Vector3d,Base::Vector3d> dirs = DrawGuiUtil::get3DDirAndRot();
        projDir = dirs.first;
        getDocument()->setStatus(App::Document::Status::SkipRecompute, true);
        doCommand(Doc,"App.activeDocument().%s.Direction = FreeCAD.Vector(%.3f,%.3f,%.3f)",
                  FeatName.c_str(), projDir.x,projDir.y,projDir.z);
        getDocument()->setStatus(App::Document::Status::SkipRecompute, false);
        doCommand(Doc,"App.activeDocument().%s.recompute()", FeatName.c_str());
    }
    commitCommand();
}

bool CmdTechDrawView::isActive(void)
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// TechDraw_ActiveView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawActiveView)

CmdTechDrawActiveView::CmdTechDrawActiveView()
  : Command("TechDraw_ActiveView")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Active View (3D View)");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_ActiveView";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-ActiveView";
}

void CmdTechDrawActiveView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    std::string PageName = page->getNameInDocument();
    Gui::Control().showDialog(new TaskDlgActiveView(page));
}

bool CmdTechDrawActiveView::isActive(void)
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// TechDraw_SectionView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawSectionView)

CmdTechDrawSectionView::CmdTechDrawSectionView()
  : Command("TechDraw_SectionView")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Section View");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_SectionView";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-SectionView";
}

void CmdTechDrawSectionView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    std::vector<App::DocumentObject*> baseObj = getSelection().getObjectsOfType(TechDraw::DrawViewPart::getClassTypeId());
    if (baseObj.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select at least 1 DrawViewPart object as Base."));
        return;
    }
    TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(*baseObj.begin());
//    std::string BaseName = dvp->getNameInDocument();
//    std::string PageName = page->getNameInDocument();
//    double baseScale = dvp->getScale();

//    Gui::WaitCursor wc;
//    openCommand("Create view");
//    std::string FeatName = getUniqueObjectName("Section");

//    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewSection','%s')",FeatName.c_str());

//    App::DocumentObject *docObj = getDocument()->getObject(FeatName.c_str());
//    TechDraw::DrawViewSection* dsv = dynamic_cast<TechDraw::DrawViewSection *>(docObj);
//    if (!dsv) {
//        throw Base::TypeError("CmdTechDrawSectionView DVS not found\n");
//    }
//    dsv->Source.setValues(dvp->Source.getValues());
//    doCommand(Doc,"App.activeDocument().%s.BaseView = App.activeDocument().%s",FeatName.c_str(),BaseName.c_str());
//    doCommand(Doc,"App.activeDocument().%s.ScaleType = App.activeDocument().%s.ScaleType",FeatName.c_str(),BaseName.c_str());
//    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
//    doCommand(Doc,"App.activeDocument().%s.Scale = %0.6f",FeatName.c_str(),baseScale);
    Gui::Control().showDialog(new TaskDlgSectionView(dvp));

    updateActive();             //ok here since dialog doesn't call doc.recompute()
    commitCommand();
}

bool CmdTechDrawSectionView::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    bool taskInProgress = false;
    if (havePage) {
        taskInProgress = Gui::Control().activeDialog();
    }
    return (havePage && haveView && !taskInProgress);
}

//===========================================================================
// TechDraw_DetailView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawDetailView)

CmdTechDrawDetailView::CmdTechDrawDetailView()
  : Command("TechDraw_DetailView")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Detail View");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_DetailView";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-DetailView";
}

void CmdTechDrawDetailView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    std::vector<App::DocumentObject*> baseObj = getSelection().getObjectsOfType(TechDraw::DrawViewPart::getClassTypeId());
    if (baseObj.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select at least 1 DrawViewPart object as Base."));
        return;
    }
    TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(*(baseObj.begin()));

    std::string PageName = page->getNameInDocument();

    Gui::WaitCursor wc;
    openCommand("Create view");

    std::string FeatName = getUniqueObjectName("Detail");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDetail','%s')",FeatName.c_str());
    App::DocumentObject *docObj = getDocument()->getObject(FeatName.c_str());
    TechDraw::DrawViewDetail* dvd = dynamic_cast<TechDraw::DrawViewDetail *>(docObj);
    if (!dvd) {
        throw Base::TypeError("CmdTechDrawDetailView DVD not found\n");
    }
    dvd->Source.setValues(dvp->Source.getValues());

    doCommand(Doc,"App.activeDocument().%s.BaseView = App.activeDocument().%s",FeatName.c_str(),dvp->getNameInDocument());
    doCommand(Doc,"App.activeDocument().%s.Direction = App.activeDocument().%s.Direction",FeatName.c_str(),dvp->getNameInDocument());
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());

    updateActive();            //ok here, no preceding recompute
    commitCommand();
}

bool CmdTechDrawDetailView::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    bool taskInProgress = false;
    if (havePage) {
        taskInProgress = Gui::Control().activeDialog();
    }
    return (havePage && haveView && !taskInProgress);
}

//===========================================================================
// TechDraw_ProjectionGroup
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawProjectionGroup)

CmdTechDrawProjectionGroup::CmdTechDrawProjectionGroup()
  : Command("TechDraw_ProjectionGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Projection Group");
    sToolTipText    = QT_TR_NOOP("Insert multiple linked views of drawable object(s)");
    sWhatsThis      = "TechDraw_ProjectionGroup";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-ProjectionGroup";
}

void CmdTechDrawProjectionGroup::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    std::string PageName = page->getNameInDocument();
//    auto inlist = page->getInListEx(true);
//    inlist.insert(page);

    //set projection direction from selected Face
    //use first object with a face selected
    std::vector<App::DocumentObject*> shapes;
    App::DocumentObject* partObj = nullptr;
    std::string faceName;
    int resolve = 1;                                //mystery
    bool single = false;                            //mystery
    auto selection = getSelection().getSelectionEx(0,
                                                   App::DocumentObject::getClassTypeId(),
                                                   resolve,
                                                   single);
    for (auto& sel: selection) {
//    for(auto &sel : getSelection().getSelectionEx(0,App::DocumentObject::getClassTypeId(),false)) {
        auto obj = sel.getObject();
        if (obj->isDerivedFrom(TechDraw::DrawPage::getClassTypeId()) ) {
            continue;
        }
//        if(!obj || inlist.count(obj))             //??????
//            continue;
        if (obj != nullptr) {                       //can this happen?
            shapes.push_back(obj);
        }
        if(partObj != nullptr) {
            continue;
        }
        for(auto& sub : sel.getSubNames()) {
            if (TechDraw::DrawUtil::getGeomTypeFromName(sub) == "Face") {
                faceName = sub;
                partObj = obj;
                break;
            }
        }
    }
    if (shapes.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("No Shapes or Groups in this selection"));
        return;
    }

    Base::Vector3d projDir;
    Gui::WaitCursor wc;

    openCommand("Create Projection Group");

    std::string multiViewName = getUniqueObjectName("ProjGroup");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawProjGroup','%s')",
                        multiViewName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",
                        PageName.c_str(),multiViewName.c_str());

    App::DocumentObject *docObj = getDocument()->getObject(multiViewName.c_str());
    auto multiView( static_cast<TechDraw::DrawProjGroup *>(docObj) );
    multiView->Source.setValues(shapes);
    doCommand(Doc,"App.activeDocument().%s.addProjection('Front')",multiViewName.c_str());

    if (faceName.size()) {
        std::pair<Base::Vector3d,Base::Vector3d> dirs = DrawGuiUtil::getProjDirFromFace(partObj,faceName);
        getDocument()->setStatus(App::Document::Status::SkipRecompute, true);
        doCommand(Doc,"App.activeDocument().%s.Anchor.Direction = FreeCAD.Vector(%.3f,%.3f,%.3f)",
                      multiViewName.c_str(), dirs.first.x,dirs.first.y,dirs.first.z);
        doCommand(Doc,"App.activeDocument().%s.Anchor.RotationVector = FreeCAD.Vector(%.3f,%.3f,%.3f)",
                      multiViewName.c_str(), dirs.second.x,dirs.second.y,dirs.second.z);
        getDocument()->setStatus(App::Document::Status::SkipRecompute, false);
    } else {
        std::pair<Base::Vector3d,Base::Vector3d> dirs = DrawGuiUtil::get3DDirAndRot();
        getDocument()->setStatus(App::Document::Status::SkipRecompute, true);
        doCommand(Doc,"App.activeDocument().%s.Anchor.Direction = FreeCAD.Vector(%.3f,%.3f,%.3f)",
                      multiViewName.c_str(), dirs.first.x,dirs.first.y,dirs.first.z);
        doCommand(Doc,"App.activeDocument().%s.Anchor.RotationVector = FreeCAD.Vector(%.3f,%.3f,%.3f)",
                      multiViewName.c_str(), dirs.second.x,dirs.second.y,dirs.second.z);
        getDocument()->setStatus(App::Document::Status::SkipRecompute, false);
    }
 
    doCommand(Doc,"App.activeDocument().%s.Anchor.recompute()", multiViewName.c_str());
    commitCommand();
    updateActive();

    // create the rest of the desired views
    Gui::Control().showDialog(new TaskDlgProjGroup(multiView,true));
}

bool CmdTechDrawProjectionGroup::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool taskInProgress = false;
    if (havePage) {
        taskInProgress = Gui::Control().activeDialog();
    }
    return (havePage  && !taskInProgress);
}

//===========================================================================
// TechDraw_NewMulti  **deprecated**
//===========================================================================

//DEF_STD_CMD_A(CmdTechDrawNewMulti);

//CmdTechDrawNewMulti::CmdTechDrawNewMulti()
//  : Command("TechDraw_NewMulti")
//{
//    sAppModule      = "TechDraw";
//    sGroup          = QT_TR_NOOP("TechDraw");
//    sMenuText       = QT_TR_NOOP("Insert multi-part view in drawing");
//    sToolTipText    = QT_TR_NOOP("Insert a new View of a multiple Parts in the active drawing");
//    sWhatsThis      = "TechDraw_NewMulti";
//    sStatusTip      = sToolTipText;
//    sPixmap         = "actions/techdraw-multiview";
//}

//void CmdTechDrawNewMulti::activated(int iMsg)
//{
//    Q_UNUSED(iMsg);
//    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
//    if (!page) {
//        return;
//    }

//    std::vector<App::DocumentObject*> shapes = getSelection().getObjectsOfType(App::DocumentObject::getClassTypeId());
//    if (shapes.empty()) {
//        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
//            QObject::tr("Can not  MultiView from this selection."));
//        return;
//    }

//    std::string PageName = page->getNameInDocument();

//    Gui::WaitCursor wc;

//    openCommand("Create view");
//    std::string FeatName = getUniqueObjectName("MultiView");
//    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewMulti','%s')",FeatName.c_str());
//    App::DocumentObject *docObj = getDocument()->getObject(FeatName.c_str());
//    auto multiView( static_cast<TechDraw::DrawViewMulti *>(docObj) );
//    multiView->Sources.setValues(shapes);
//    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
//    updateActive();
//    commitCommand();
//}

//bool CmdTechDrawNewMulti::isActive(void)
//{
//    return DrawGuiUtil::needPage(this);
//}

//===========================================================================
// TechDraw_Balloon
//===========================================================================

//! common checks of Selection for Dimension commands
//non-empty selection, no more than maxObjs selected and at least 1 DrawingPage exists
bool _checkSelectionBalloon(Gui::Command* cmd, unsigned maxObjs) {
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    if (selection.size() == 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
                             QObject::tr("Select an object first"));
        return false;
    }

    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() > maxObjs){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
            QObject::tr("Too many objects selected"));
        return false;
    }

    std::vector<App::DocumentObject*> pages = cmd->getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
            QObject::tr("Create a page first."));
        return false;
    }
    return true;
}

bool _checkDrawViewPartBalloon(Gui::Command* cmd) {
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    auto objFeat( dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject()) );
    if( !objFeat ) {
        QMessageBox::warning( Gui::getMainWindow(),
                              QObject::tr("Incorrect selection"),
                              QObject::tr("No View of a Part in selection.") );
        return false;
    }
    return true;
}

DEF_STD_CMD_A(CmdTechDrawBalloon)

CmdTechDrawBalloon::CmdTechDrawBalloon()
  : Command("TechDraw_Balloon")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Balloon Annotation");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_Balloon";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Balloon";
}

void CmdTechDrawBalloon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelectionBalloon(this,1);
    if (!result)
        return;
    result = _checkDrawViewPartBalloon(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    auto objFeat( dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject()) );
    if( objFeat == nullptr ) {
        return;
    }

    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();
    
    page->balloonParent = objFeat;
    page->balloonPlacing = true;

}

bool CmdTechDrawBalloon::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ClipGroup
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawClipGroup)

CmdTechDrawClipGroup::CmdTechDrawClipGroup()
  : Command("TechDraw_ClipGroup")
{
    // setting the
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Insert Clip Group");
    sToolTipText  = sToolTipText;
    sWhatsThis    = "TechDraw_ClipGroup";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/techdraw-ClipGroup";
}

void CmdTechDrawClipGroup::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    std::string PageName = page->getNameInDocument();

    std::string FeatName = getUniqueObjectName("Clip");
    openCommand("Create Clip");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewClip','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    updateActive();
    commitCommand();
}

bool CmdTechDrawClipGroup::isActive(void)
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// TechDraw_ClipGroupAdd
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawClipGroupAdd)

CmdTechDrawClipGroupAdd::CmdTechDrawClipGroupAdd()
  : Command("TechDraw_ClipGroupAdd")
{
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Add View to Clip Group");
    sToolTipText  = sMenuText;
    sWhatsThis    = "TechDraw_ClipGroupAdd";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/techdraw-ClipGroupAdd";
}

void CmdTechDrawClipGroupAdd::activated(int iMsg)
{
    Q_UNUSED(iMsg);
   std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
   if (selection.size() != 2) {
       QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                            QObject::tr("Select one Clip group and one View."));
       return;
   }

    TechDraw::DrawViewClip* clip = 0;
    TechDraw::DrawView* view = 0;
    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewClip::getClassTypeId())) {
            clip = static_cast<TechDraw::DrawViewClip*>((*itSel).getObject());
        } else if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
            view = static_cast<TechDraw::DrawView*>((*itSel).getObject());
        }
    }
    if (!view) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one View to add to group."));
        return;
    }
    if (!clip) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one Clip group."));
        return;
    }

    TechDraw::DrawPage* pageClip = clip->findParentPage();
    TechDraw::DrawPage* pageView = view->findParentPage();

    if (pageClip != pageView) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Clip and View must be from same Page."));
        return;
    }

    std::string PageName = pageClip->getNameInDocument();
    std::string ClipName = clip->getNameInDocument();
    std::string ViewName = view->getNameInDocument();

    openCommand("ClipGroupAdd");
    doCommand(Doc,"App.activeDocument().%s.ViewObject.Visibility = False",ViewName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",ClipName.c_str(),ViewName.c_str());
    doCommand(Doc,"App.activeDocument().%s.ViewObject.Visibility = True",ViewName.c_str());
    updateActive();
    commitCommand();
}

bool CmdTechDrawClipGroupAdd::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveClip = false;
    if (havePage) {
        auto drawClipType( TechDraw::DrawViewClip::getClassTypeId() );
        auto selClips = getDocument()->getObjectsOfType(drawClipType);
        if (!selClips.empty()) {
            haveClip = true;
        }
    }
    return (havePage && haveClip);
}

//===========================================================================
// TechDraw_ClipGroupRemove
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawClipGroupRemove)

CmdTechDrawClipGroupRemove::CmdTechDrawClipGroupRemove()
  : Command("TechDraw_ClipGroupRemove")
{
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Remove View from Clip Group");
    sToolTipText  = sMenuText;
    sWhatsThis    = "TechDraw_ClipGroupRemove";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/techdraw-ClipGroupRemove";
}

void CmdTechDrawClipGroupRemove::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto dObj( getSelection().getObjectsOfType(TechDraw::DrawView::getClassTypeId()) );
    if (dObj.empty()) {
        QMessageBox::warning( Gui::getMainWindow(),
                              QObject::tr("Wrong selection"),
                              QObject::tr("Select exactly one View to remove from Group.") );
        return;
    }

    auto view( static_cast<TechDraw::DrawView*>(dObj.front()) );

    TechDraw::DrawPage* page = view->findParentPage();
    const std::vector<App::DocumentObject*> pViews = page->Views.getValues();
    TechDraw::DrawViewClip *clip(nullptr);
    for (auto &v : pViews) {
        clip = dynamic_cast<TechDraw::DrawViewClip*>(v);
        if (clip && clip->isViewInClip(view)) {
            break;
        }
        clip = nullptr;
    }

    if (!clip) {
        QMessageBox::warning( Gui::getMainWindow(),
                              QObject::tr("Wrong selection"),
                              QObject::tr("View does not belong to a Clip") );
        return;
    }

    std::string ClipName = clip->getNameInDocument();
    std::string ViewName = view->getNameInDocument();

    openCommand("ClipGroupRemove");
    doCommand(Doc,"App.activeDocument().%s.ViewObject.Visibility = False",ViewName.c_str());
    doCommand(Doc,"App.activeDocument().%s.removeView(App.activeDocument().%s)",ClipName.c_str(),ViewName.c_str());
    doCommand(Doc,"App.activeDocument().%s.ViewObject.Visibility = True",ViewName.c_str());
    updateActive();
    commitCommand();
}

bool CmdTechDrawClipGroupRemove::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveClip = false;
    if (havePage) {
        auto drawClipType( TechDraw::DrawViewClip::getClassTypeId() );
        auto selClips = getDocument()->getObjectsOfType(drawClipType);
        if (!selClips.empty()) {
            haveClip = true;
        }
    }
    return (havePage && haveClip);
}


//===========================================================================
// TechDraw_Symbol
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawSymbol)

CmdTechDrawSymbol::CmdTechDrawSymbol()
  : Command("TechDraw_Symbol")
{
    // setting the Gui eye-candy
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Insert SVG Symbol");
    sToolTipText  = QT_TR_NOOP("Insert symbol from a SVG file");
    sWhatsThis    = "TechDraw_Symbol";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/techdraw-symbol";
}

void CmdTechDrawSymbol::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    std::string PageName = page->getNameInDocument();

    // Reading an image
    QString filename = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(), QObject::tr("Choose an SVG file to open"), QString::null,
        QString::fromLatin1("%1 (*.svg *.svgz)").arg(QObject::tr("Scalable Vector Graphic")));
    if (!filename.isEmpty())
    {
        std::string FeatName = getUniqueObjectName("Symbol");
        filename = Base::Tools::escapeEncodeFilename(filename);
        openCommand("Create Symbol");
#if PY_MAJOR_VERSION < 3
        doCommand(Doc,"f = open(unicode(\"%s\",'utf-8'),'r')",(const char*)filename.toUtf8());
#else
        doCommand(Doc,"f = open(\"%s\",'r')",(const char*)filename.toUtf8());
#endif
        doCommand(Doc,"svg = f.read()");
        doCommand(Doc,"f.close()");
        doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewSymbol','%s')",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Symbol = svg",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
        updateActive();
        commitCommand();
    }
}

bool CmdTechDrawSymbol::isActive(void)
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// TechDraw_DraftView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawDraftView)

CmdTechDrawDraftView::CmdTechDrawDraftView()
  : Command("TechDraw_DraftView")
{
    // setting the Gui eye-candy
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Insert Draft Workbench Object");
    sToolTipText  = QT_TR_NOOP("Insert a View of a Draft Workbench object");
    sWhatsThis    = "TechDraw_NewDraft";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/techdraw-DraftView";
}

void CmdTechDrawDraftView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

//TODO: shouldn't this be checking for a Draft object only?
//      there is no obvious way of check for a Draft object.  Could be App::FeaturePython, Part::Part2DObject, ???
    std::vector<App::DocumentObject*> objects = getSelection().getObjectsOfType(App::DocumentObject::getClassTypeId());
    if (objects.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select at least one object."));
        return;
    }
    std::string PageName = page->getNameInDocument();

    for (std::vector<App::DocumentObject*>::iterator it = objects.begin(); it != objects.end(); ++it) {
        std::string FeatName = getUniqueObjectName("DraftView");
        std::string SourceName = (*it)->getNameInDocument();
        openCommand("Create DraftView");
        doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDraft','%s')",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Source = App.activeDocument().%s",FeatName.c_str(),SourceName.c_str());
        doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
        updateActive();
        commitCommand();
    }
}

bool CmdTechDrawDraftView::isActive(void)
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// TechDraw_ArchView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawArchView)

CmdTechDrawArchView::CmdTechDrawArchView()
  : Command("TechDraw_ArchView")
{
    // setting the Gui eye-candy
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Insert Section Plane");
    sToolTipText  = QT_TR_NOOP("Insert a View of a Section Plane from Arch Workbench");
    sWhatsThis    = "TechDraw_NewArch";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/techdraw-ArchView";
}

void CmdTechDrawArchView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    const std::vector<App::DocumentObject*> objects = getSelection().getObjectsOfType(App::DocumentObject::getClassTypeId());
    if (objects.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one object."));
        return;
    }

    std::string PageName = page->getNameInDocument();

    std::string FeatName = getUniqueObjectName("ArchView");
    std::string SourceName = objects.front()->getNameInDocument();
    openCommand("Create ArchView");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewArch','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Source = App.activeDocument().%s",FeatName.c_str(),SourceName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    updateActive();
    commitCommand();
}

bool CmdTechDrawArchView::isActive(void)
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// TechDraw_SpreadsheetView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawSpreadsheetView)

CmdTechDrawSpreadsheetView::CmdTechDrawSpreadsheetView()
  : Command("TechDraw_SpreadsheetView")
{
    // setting the
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Insert Spreadsheet View");
    sToolTipText  = QT_TR_NOOP("Insert View to a spreadsheet");
    sWhatsThis    = "TechDraw_SpreadsheetView";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/techdraw-SpreadsheetView";
}

void CmdTechDrawSpreadsheetView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    const std::vector<App::DocumentObject*> spreads = getSelection().getObjectsOfType(Spreadsheet::Sheet::getClassTypeId());
    if (spreads.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one Spreadsheet object."));
        return;
    }
    std::string SpreadName = spreads.front()->getNameInDocument();

    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    std::string PageName = page->getNameInDocument();

    openCommand("Create spreadsheet view");
    std::string FeatName = getUniqueObjectName("Sheet");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewSpreadsheet','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Source = App.activeDocument().%s",FeatName.c_str(),SpreadName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    updateActive();
    commitCommand();
}

bool CmdTechDrawSpreadsheetView::isActive(void)
{
    //need a Page and a SpreadSheet::Sheet
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveSheet = false;
    if (havePage) {
        auto spreadSheetType( Spreadsheet::Sheet::getClassTypeId() );
        auto selSheets = getDocument()->getObjectsOfType(spreadSheetType);
        if (!selSheets.empty()) {
            haveSheet = true;
        }
    }
    return (havePage && haveSheet);
}


//===========================================================================
// TechDraw_ExportPageSVG
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExportPageSVG)

CmdTechDrawExportPageSVG::CmdTechDrawExportPageSVG()
  : Command("TechDraw_ExportPageSVG")
{
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("Export page as SVG");
    sToolTipText  = sMenuText;
    sWhatsThis    = "TechDraw_ExportPageSVG";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/techdraw-ExportPageSVG";
}

void CmdTechDrawExportPageSVG::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    std::string PageName = page->getNameInDocument();

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

bool CmdTechDrawExportPageSVG::isActive(void)
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// TechDraw_ExportPageDXF
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExportPageDXF)

CmdTechDrawExportPageDXF::CmdTechDrawExportPageDXF()
  : Command("TechDraw_ExportPageDXF")
{
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("Export page as DXF");
    sToolTipText  = sMenuText;
    sWhatsThis    = "TechDraw_ExportPageDXF";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/techdraw-ExportPageDXF";
}

void CmdTechDrawExportPageDXF::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    std::vector<App::DocumentObject*> views = page->Views.getValues();
    for (auto& v: views) {
        if (v->isDerivedFrom(TechDraw::DrawViewArch::getClassTypeId())) {
            QMessageBox::StandardButton rc =
                QMessageBox::question(Gui::getMainWindow(), QObject::tr("Can not export selection"),
                            QObject::tr("Page contains DrawViewArch which will not be exported. Continue?"),
                            QMessageBox::StandardButtons(QMessageBox::Yes| QMessageBox::No));
            if (rc == QMessageBox::No) {
                return;
            } else {
                break;
            }
        }
    }

//WF? allow more than one TD Page per Dxf file??  1 TD page = 1 DXF file = 1 drawing?
    QString defaultDir;
    QString fileName = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(),
                                                   QString::fromUtf8(QT_TR_NOOP("Save Dxf File ")),
                                                   defaultDir,
                                                   QString::fromUtf8(QT_TR_NOOP("Dxf (*.dxf)")));

    if (fileName.isEmpty()) {
        return;
    }

    std::string PageName = page->getNameInDocument();
    openCommand("Save page to dxf");
    doCommand(Doc,"import TechDraw");
    fileName = Base::Tools::escapeEncodeFilename(fileName);
    doCommand(Doc,"TechDraw.writeDXFPage(App.activeDocument().%s,u\"%s\")",PageName.c_str(),(const char*)fileName.toUtf8());
    commitCommand();
}


bool CmdTechDrawExportPageDXF::isActive(void)
{
    return DrawGuiUtil::needPage(this);
}

void CreateTechDrawCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawPageDefault());
    rcCmdMgr.addCommand(new CmdTechDrawPageTemplate());
    rcCmdMgr.addCommand(new CmdTechDrawRedrawPage());
    rcCmdMgr.addCommand(new CmdTechDrawView());
    rcCmdMgr.addCommand(new CmdTechDrawActiveView());
    rcCmdMgr.addCommand(new CmdTechDrawSectionView());
    rcCmdMgr.addCommand(new CmdTechDrawDetailView());
    rcCmdMgr.addCommand(new CmdTechDrawProjectionGroup());
    rcCmdMgr.addCommand(new CmdTechDrawClipGroup());
    rcCmdMgr.addCommand(new CmdTechDrawClipGroupAdd());
    rcCmdMgr.addCommand(new CmdTechDrawClipGroupRemove());
    rcCmdMgr.addCommand(new CmdTechDrawSymbol());
    rcCmdMgr.addCommand(new CmdTechDrawExportPageSVG());
    rcCmdMgr.addCommand(new CmdTechDrawExportPageDXF());
    rcCmdMgr.addCommand(new CmdTechDrawDraftView());
    rcCmdMgr.addCommand(new CmdTechDrawArchView());
    rcCmdMgr.addCommand(new CmdTechDrawSpreadsheetView());
    rcCmdMgr.addCommand(new CmdTechDrawBalloon());
}
