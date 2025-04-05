/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
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
#include <QFileInfo>
#include <QMessageBox>
#include <QCheckBox>
#include <QPushButton>
#include <vector>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Link.h>

#include <Base/Console.h>
#include <Base/Tools.h>

#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>

#include <Mod/Spreadsheet/App/Sheet.h>

#include <Mod/TechDraw/App/DrawComplexSection.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawSVGTemplate.h>
#include <Mod/TechDraw/App/DrawViewArch.h>
#include <Mod/TechDraw/App/DrawViewClip.h>
#include <Mod/TechDraw/App/DrawViewDetail.h>
#include <Mod/TechDraw/App/DrawViewDraft.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSymbol.h>
#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/App/DrawBrokenView.h>

#include "DrawGuiUtil.h"
#include "MDIViewPage.h"
#include "QGIViewPart.h"
#include "QGSPage.h"
#include "QGVPage.h"
#include "TaskActiveView.h"
#include "TaskComplexSection.h"
#include "TaskDetail.h"
#include "TaskProjGroup.h"
#include "TaskProjection.h"
#include "TaskSectionView.h"
#include "ViewProviderPage.h"
#include "ViewProviderDrawingView.h"

#include "BalloonHelpers.h"
#include "CommandHelpers.h"
#include "ViewMakers.h"
#include "SingleInsertTool.h"

void execSimpleSection(Gui::Command* cmd);
void execComplexSection(Gui::Command* cmd);

using namespace TechDrawGui;
using namespace TechDraw;
using namespace TechDraw::BalloonHelpers;
using namespace TechDraw::CommandHelpers;
using namespace TechDraw::ViewMakers;
using DU = DrawUtil;

//===========================================================================
// TechDraw_PageDefault
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawPageDefault)

CmdTechDrawPageDefault::CmdTechDrawPageDefault() : Command("TechDraw_PageDefault")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Default Page");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_PageDefault";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_PageDefault";
}

void CmdTechDrawPageDefault::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    QString templateFileName = Preferences::defaultTemplate();
    QFileInfo tfi(templateFileName);
    if (tfi.isReadable()) {
        Gui::WaitCursor wc;
        openCommand(QT_TRANSLATE_NOOP("Command", "Drawing create page"));

        auto page = getDocument()->addObject<TechDraw::DrawPage>("Page");
        if (!page) {
            throw Base::TypeError("CmdTechDrawPageDefault - page not created");
        }
        page->translateLabel("DrawPage", "Page", page->getNameInDocument());

        auto svgTemplate = getDocument()->addObject<TechDraw::DrawSVGTemplate>("Template");
        if (!svgTemplate) {
            throw Base::TypeError("CmdTechDrawPageDefault - template not created");
        }
        svgTemplate->translateLabel("DrawSVGTemplate", "Template", svgTemplate->getNameInDocument());

        page->Template.setValue(svgTemplate);
        auto filespec = DU::cleanFilespecBackslash(templateFileName.toStdString());
        svgTemplate->Template.setValue(filespec);

        updateActive();
        commitCommand();

        auto* dvp = dynamic_cast<TechDrawGui::ViewProviderPage *>
                                                 (Gui::Application::Instance->getViewProvider(page));
        if (dvp) {
            dvp->show();
        }
    }
    else {
        QMessageBox::critical(Gui::getMainWindow(), QLatin1String("No template"),
                              QLatin1String("No default template found"));
    }
}

bool CmdTechDrawPageDefault::isActive() { return hasActiveDocument(); }

//===========================================================================
// TechDraw_PageTemplate
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawPageTemplate)

CmdTechDrawPageTemplate::CmdTechDrawPageTemplate() : Command("TechDraw_PageTemplate")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Page using Template");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_PageTemplate";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_PageTemplate";
}

void CmdTechDrawPageTemplate::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QString work_dir = Gui::FileDialog::getWorkingDirectory();
    QString templateDir = Preferences::defaultTemplateDir();
    QString templateFileName = Gui::FileDialog::getOpenFileName(
        Gui::getMainWindow(), QString::fromUtf8(QT_TR_NOOP("Select a Template File")), templateDir,
        QString::fromUtf8(QT_TR_NOOP("Template (*.svg)")));
    Gui::FileDialog::setWorkingDirectory(work_dir);// Don't overwrite WD with templateDir

    if (templateFileName.isEmpty()) {
        return;
    }

    QFileInfo tfi(templateFileName);
    if (tfi.isReadable()) {
        Gui::WaitCursor wc;
        openCommand(QT_TRANSLATE_NOOP("Command", "Drawing create page"));

        auto page = getDocument()->addObject<TechDraw::DrawPage>("Page");
        if (!page) {
            throw Base::TypeError("CmdTechDrawPageTemplate - page not created");
        }
        page->translateLabel("DrawPage", "Page", page->getNameInDocument());

        auto svgTemplate = getDocument()->addObject<TechDraw::DrawSVGTemplate>("Template");
        if (!svgTemplate) {
            throw Base::TypeError("CmdTechDrawPageTemplate - template not created");
        }
        svgTemplate->translateLabel("DrawSVGTemplate", "Template", svgTemplate->getNameInDocument());

        page->Template.setValue(svgTemplate);
        auto filespec = DU::cleanFilespecBackslash(templateFileName.toStdString());
        svgTemplate->Template.setValue(filespec);

        updateActive();
        commitCommand();

        auto* dvp = dynamic_cast<TechDrawGui::ViewProviderPage *>
                                                 (Gui::Application::Instance->getViewProvider(page));
        if (dvp) {
            dvp->show();
        }
    }
    else {
        QMessageBox::critical(Gui::getMainWindow(), QLatin1String("No template"),
                              QLatin1String("Template file is invalid"));
    }
}

bool CmdTechDrawPageTemplate::isActive() { return hasActiveDocument(); }

//===========================================================================
// TechDraw_RedrawPage
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawRedrawPage)

CmdTechDrawRedrawPage::CmdTechDrawRedrawPage() : Command("TechDraw_RedrawPage")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Redraw Page");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_RedrawPage";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_RedrawPage";
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

bool CmdTechDrawRedrawPage::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_PrintAll
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawPrintAll)

CmdTechDrawPrintAll::CmdTechDrawPrintAll() : Command("TechDraw_PrintAll")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Print All Pages");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_PrintAll";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_PrintAll";
}

void CmdTechDrawPrintAll::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    MDIViewPage::printAllPages();
}

bool CmdTechDrawPrintAll::isActive() { return DrawGuiUtil::needPage(this); }

//===========================================================================
// TechDraw_View
//===========================================================================

// this is the all-in-one command using singleInsertTool()
DEF_STD_CMD_A(CmdTechDrawView)

CmdTechDrawView::CmdTechDrawView() : Command("TechDraw_View")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert View");
    sToolTipText = QT_TR_NOOP("Insert a View in current page.\n"
        "Selected objects, spreadsheets or BIM section planes will be added.\n"
        "Without a selection, a file browser lets you select a SVG or image file.");
    sWhatsThis = "TechDraw_View";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_View";
}

void CmdTechDrawView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    singleInsertTool(*this, page);
}

bool CmdTechDrawView::isActive() { return DrawGuiUtil::needPage(this); }

//===========================================================================
// TechDraw_ShapeView
//===========================================================================

// this is the merger of the original tools for inserting views or projection groups

DEF_STD_CMD_A(CmdTechDrawShapeView)

CmdTechDrawShapeView::CmdTechDrawShapeView() : Command("TechDraw_ShapeView")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Shape View");
    sToolTipText = QT_TR_NOOP("Insert a View of selected shape objects onto the current or selected page.");
    sWhatsThis = "TechDraw_ShapeView";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_ShapeView";
}

void CmdTechDrawShapeView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Create shape view view"));
    auto* newView = ViewMakers::makeShapeViewSelection(page);
    updateActive();
    commitCommand();

    // create other views
    constexpr bool CreateMode{true};
    Gui::Control().showDialog(new TaskDlgProjGroup(newView, CreateMode));
}

bool CmdTechDrawShapeView::isActive()
{
    return DrawGuiUtil::needPage(this);
}


//===========================================================================
// TechDraw_BrokenView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawBrokenView)

CmdTechDrawBrokenView::CmdTechDrawBrokenView()
  : Command("TechDraw_BrokenView")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Broken View");
    sToolTipText    = QT_TR_NOOP("Insert Broken View");
    sWhatsThis      = "TechDraw_BrokenView";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_BrokenView";
}

void CmdTechDrawBrokenView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    Gui::WaitCursor wc;
    openCommand(QT_TRANSLATE_NOOP("Command", "Create broken view"));

    // TODO: wf: compare results of wrapping object creation with
    //       getDocument()->setStatus(App::Document::Status::SkipRecompute, true);  and
    //       getDocument()->setStatus(App::Document::Status::SkipRecompute, false);
    //       does it prevent superflous recomputes?

    auto newObject = makeBrokenViewSelection(page);

    commitCommand();
    newObject->recomputeFeature();
}

bool CmdTechDrawBrokenView::isActive()
{
    return DrawGuiUtil::needPage(this);
}


//===========================================================================
// TechDraw_ActiveView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawActiveView)

CmdTechDrawActiveView::CmdTechDrawActiveView() : Command("TechDraw_ActiveView")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Active View (3D View)");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_ActiveView";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_ActiveView";
}

void CmdTechDrawActiveView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this, true);
    if (!page) {
        return;
    }
    Gui::Control().showDialog(new TaskDlgActiveView(page));
}

bool CmdTechDrawActiveView::isActive() { return DrawGuiUtil::needPage(this, true); }

//===========================================================================
// TechDraw_SectionGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawSectionGroup)

CmdTechDrawSectionGroup::CmdTechDrawSectionGroup() : Command("TechDraw_SectionGroup")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert a simple or complex Section View");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_SectionGroup";
    sStatusTip = sToolTipText;
}

void CmdTechDrawSectionGroup::activated(int iMsg)
{
    if (!CommandHelpers::guardActiveDialog()) {
        return;
    }

    auto* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch (iMsg) {
        case 0:
            execSimpleSection(this);
            break;
        case 1:
            execComplexSection(this);
            break;
        default:
            Base::Console().Message("CMD::SectionGrp - invalid iMsg: %d\n", iMsg);
    };
}

Gui::Action* CmdTechDrawSectionGroup::createAction()
{
    auto* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("actions/TechDraw_SectionView"));
    p1->setObjectName(QStringLiteral("TechDraw_SectionView"));
    p1->setWhatsThis(QStringLiteral("TechDraw_SectionView"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("actions/TechDraw_ComplexSection"));
    p2->setObjectName(QStringLiteral("TechDraw_ComplexSection"));
    p2->setWhatsThis(QStringLiteral("TechDraw_ComplexSection"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawSectionGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction) {
        return;
    }
    auto* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdTechDrawSectionGroup", "Section View"));
    arc1->setToolTip(QApplication::translate("TechDraw_SectionView", "Insert simple Section View"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdTechDrawSectionGroup", "Complex Section"));
    arc2->setToolTip(
        QApplication::translate("TechDraw_ComplexSection", "Insert complex Section View"));
    arc2->setStatusTip(arc2->toolTip());
}

bool CmdTechDrawSectionGroup::isActive()
{
    return CommandHelpers::isActiveCommon(this);
}

//===========================================================================
// TechDraw_SectionView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawSectionView)

CmdTechDrawSectionView::CmdTechDrawSectionView() : Command("TechDraw_SectionView")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Section View");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_SectionView";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_SectionView";
}

void CmdTechDrawSectionView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (!CommandHelpers::guardActiveDialog()) {
        return;
    }

    execSimpleSection(this);
}

bool CmdTechDrawSectionView::isActive()
{
    return CommandHelpers::isActiveCommon(this);
}

void execSimpleSection(Gui::Command* cmd)
{
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(cmd);
    if (!page) {
        return;
    }

    auto* baseView = CommandHelpers::findBaseViewInSelection();
    if (!baseView) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("Select at least 1 DrawViewPart object as Base."));
        return;
    }

    // TODO: wf: move the dvs creation logic out of the dialog?s
    Gui::Control().showDialog(new TaskDlgSectionView(baseView));
}

//===========================================================================
// TechDraw_ComplexSection
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawComplexSection)

CmdTechDrawComplexSection::CmdTechDrawComplexSection() : Command("TechDraw_ComplexSection")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Complex Section View");
    sToolTipText = QT_TR_NOOP("Insert a Complex Section View");
    sWhatsThis = "TechDraw_ComplexSection";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_ComplexSection";
}

void CmdTechDrawComplexSection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (!CommandHelpers::guardActiveDialog()) {
        return;
    }

    execComplexSection(this);
}

bool CmdTechDrawComplexSection::isActive() { return DrawGuiUtil::needPage(this); }

//Complex Sections can be created without a baseView, so the gathering of input
//for the dialog is more involved than simple section
void execComplexSection(Gui::Command* cmd)
{
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(cmd);
    if (!page) {
        return;
    }

    std::vector<App::DocumentObject*> shapes;
    std::vector<App::DocumentObject*> xShapes;
    App::DocumentObject* profileObject(nullptr);
    std::vector<std::string> profileSubs;

    findProfileObjectsInSelection(profileObject, profileSubs);
    if (!profileObject) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("No profile object found in selection"));
        return;
    }

    auto* baseView = findBaseViewInSelection(profileObject);
    if (!baseView) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("I do not know what base view to use."));
        return;
    }

    getShapeObjectsFromSelection(shapes, xShapes, profileObject);
    if (shapes.empty() && xShapes.empty()) {
        QMessageBox::warning(
            Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("No Base View, Shapes, Groups or Links in this selection"));
        return;
    }

    Gui::Control().showDialog(
        new TaskDlgComplexSection(page, baseView, shapes, xShapes, profileObject, profileSubs));
}


//===========================================================================
// TechDraw_DetailView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawDetailView)

CmdTechDrawDetailView::CmdTechDrawDetailView() : Command("TechDraw_DetailView")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Detail View");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_DetailView";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_DetailView";
}

void CmdTechDrawDetailView::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    auto* baseView = findBaseViewInSelection();
    if (!baseView) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("Select at least 1 DrawViewPart object as Base."));
        return;
    }
    auto* dvp = static_cast<TechDraw::DrawViewPart*>(baseView);
    if (CommandHelpers::guardActiveDialog()) {
        Gui::Control().showDialog(new TaskDlgDetail(dvp));
    }
}

bool CmdTechDrawDetailView::isActive()
{
    return CommandHelpers::isActiveCommon(this);
}

//===========================================================================
// TechDraw_ProjectionGroup
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawProjectionGroup)

CmdTechDrawProjectionGroup::CmdTechDrawProjectionGroup() : Command("TechDraw_ProjectionGroup")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Projection Group");
    sToolTipText = QT_TR_NOOP("Insert multiple linked views of drawable object(s)");
    sWhatsThis = "TechDraw_ProjectionGroup";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_ProjectionGroup";
}

void CmdTechDrawProjectionGroup::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    std::vector<App::DocumentObject*> shapes;
    std::vector<App::DocumentObject*> xShapes;
    getShapeObjectsFromSelection(shapes, xShapes);
    if (shapes.empty() && xShapes.empty()) {
        QMessageBox::warning(
            Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("No Base View, Shapes, Groups or Links in this selection"));
        return;
    }

    auto newObject = makeProjectionGroupSelection(page);
    Gui::Control().showDialog(new TaskDlgProjGroup(newObject, true));
}

bool CmdTechDrawProjectionGroup::isActive()
{
    return CommandHelpers::isActiveCommon(this);
}


DEF_STD_CMD_A(CmdTechDrawBalloon)

CmdTechDrawBalloon::CmdTechDrawBalloon() : Command("TechDraw_Balloon")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Balloon Annotation");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_Balloon";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_Balloon";
}

void CmdTechDrawBalloon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = BalloonHelpers::checkSelectionBalloon(this, 1);
    if (!result) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    auto objFeat(dynamic_cast<TechDraw::DrawView*>(selection[0].getObject()));
    if (!objFeat) {
        return;
    }

    TechDraw::DrawPage* page = objFeat->findParentPage();

    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(page->getDocument());
    auto* pageVP = dynamic_cast<ViewProviderPage*>(guiDoc->getViewProvider(page));
    auto* viewVP = dynamic_cast<ViewProviderDrawingView*>(guiDoc->getViewProvider(objFeat));


    if (pageVP && viewVP) {
        QGVPage* viewPage = pageVP->getQGVPage();
        QGSPage* scenePage = pageVP->getQGSPage();
        if (viewPage) {
            viewPage->startBalloonPlacing(objFeat);

            auto* view = viewVP->getQView();
            QPointF placement;
            if (view &&
                BalloonHelpers::checkDirectPlacement(view, selection[0].getSubNames(), placement)) {
                //this creates the balloon if something is already selected
                scenePage->createBalloon(placement, objFeat);
            }
        }
    }
}

bool CmdTechDrawBalloon::isActive()
{
    return CommandHelpers::isActiveCommon(this);
}

//===========================================================================
// TechDraw_ClipGroup
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawClipGroup)

CmdTechDrawClipGroup::CmdTechDrawClipGroup() : Command("TechDraw_ClipGroup")
{
    // setting the
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Clip Group");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_ClipGroup";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_ClipGroup";
}

void CmdTechDrawClipGroup::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    openCommand(QT_TRANSLATE_NOOP("Command", "Create Clip"));
    ViewMakers::makeClipGroup(page);
    updateActive();
    commitCommand();
}

bool CmdTechDrawClipGroup::isActive() { return DrawGuiUtil::needPage(this); }

//===========================================================================
// TechDraw_ClipGroupAdd
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawClipGroupAdd)

CmdTechDrawClipGroupAdd::CmdTechDrawClipGroupAdd() : Command("TechDraw_ClipGroupAdd")
{
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Add View to Clip Group");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_ClipGroupAdd";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_ClipGroupAdd";
}

void CmdTechDrawClipGroupAdd::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    auto clipsAll = getSelection().getObjectsOfType(TechDraw::DrawViewClip::getClassTypeId());
    TechDraw::DrawViewClip* clip = nullptr;
    if (!clipsAll.empty()) {
        clip = static_cast<TechDraw::DrawViewClip*>(clipsAll.front());
    }

    auto viewsAll = getSelection().getObjectsOfType(TechDraw::DrawView::getClassTypeId());
    TechDraw::DrawView* view = nullptr;
    for (auto& selView : viewsAll) {
        if (selView->isDerivedFrom(TechDraw::DrawViewClip::getClassTypeId())) {
            continue;
        }

        view = static_cast<TechDraw::DrawView*>(selView);
        break;
    }

    TechDraw::DrawPage* pageClip{nullptr};
    if (clip) {
        pageClip = clip->findParentPage();
    }
    TechDraw::DrawPage* pageView{nullptr};
    if (view) {
        pageView = view->findParentPage();
    }
    QString samePage{QObject::tr("Same page")};
    QString differentPages{QObject::tr("Different pages")};
    QString pageMsg = (pageClip == pageView) ? samePage : differentPages;

    if (pageClip != pageView ||
        !clip ||
        !view ||
        clipsAll.size() != 1 ||
        viewsAll.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("You need exactly 1 clip group and 1 other view from the same page to do this.\nYou have %1 clip group, %2 other view. %3.")
                .arg(QString::number(clipsAll.size())).arg(QString::number(viewsAll.size()))
                .arg(pageMsg));
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Add view to clip"));
    ViewMakers::addViewToClipGroup(clip, view);
    updateActive();
    commitCommand();
}

bool CmdTechDrawClipGroupAdd::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveClip = false;
    if (havePage) {
        auto drawClipType(TechDraw::DrawViewClip::getClassTypeId());
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

CmdTechDrawClipGroupRemove::CmdTechDrawClipGroupRemove() : Command("TechDraw_ClipGroupRemove")
{
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Remove View from Clip Group");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_ClipGroupRemove";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_ClipGroupRemove";
}

void CmdTechDrawClipGroupRemove::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto dObj(getSelection().getObjectsOfType(TechDraw::DrawView::getClassTypeId()));
    if (dObj.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("Select exactly one View to remove from Group."));
        return;
    }

    auto view(static_cast<TechDraw::DrawView*>(dObj.front()));

    TechDraw::DrawPage* page = view->findParentPage();
    const std::vector<App::DocumentObject*> pViews = page->getViews();
    TechDraw::DrawViewClip* clip(nullptr);
    for (auto& v : pViews) {
        clip = dynamic_cast<TechDraw::DrawViewClip*>(v);
        if (clip && clip->isViewInClip(view)) {
            break;
        }
        clip = nullptr;
    }

    if (!clip) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("View does not belong to a Clip"));
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Remove clip group"));
    ViewMakers::removeViewFromClipGroup(clip, view);
    updateActive();
    commitCommand();
}

bool CmdTechDrawClipGroupRemove::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveClip = false;
    if (havePage) {
        auto drawClipType(TechDraw::DrawViewClip::getClassTypeId());
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

CmdTechDrawSymbol::CmdTechDrawSymbol() : Command("TechDraw_Symbol")
{
    // setting the Gui eye-candy
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert SVG Symbol");
    sToolTipText = QT_TR_NOOP("Insert symbol from an SVG file");
    sWhatsThis = "TechDraw_Symbol";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_Symbol";
}

void CmdTechDrawSymbol::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    QString filename = Gui::FileDialog::getOpenFileName(
        Gui::getMainWindow(), QObject::tr("Choose an SVG file to open"),
        Preferences::defaultSymbolDir(),
        QStringLiteral("%1 (*.svg *.svgz);;%2 (*.*)")
            .arg(QObject::tr("Scalable Vector Graphic"), QObject::tr("All Files")));
    if (filename.isEmpty()) {
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Create symbol view"));
    ViewMakers::makeSymbolView(page, filename);
    updateActive();
    commitCommand();
}

bool CmdTechDrawSymbol::isActive() { return DrawGuiUtil::needPage(this); }

//===========================================================================
// TechDraw_DraftView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawDraftView)

CmdTechDrawDraftView::CmdTechDrawDraftView() : Command("TechDraw_DraftView")
{
    // setting the Gui eye-candy
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Draft Workbench Object");
    sToolTipText = QT_TR_NOOP("Insert a View of a Draft Workbench object");
    sWhatsThis = "TechDraw_NewDraft";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_DraftView";
}

void CmdTechDrawDraftView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    auto objects = getSelection().getObjectsOfType(App::DocumentObject::getClassTypeId());

    if (objects.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("Select at least one object."));
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Create DraftView"));
    ViewMakers::makeDraftView(page);
    updateActive();
    commitCommand();
}

bool CmdTechDrawDraftView::isActive() { return DrawGuiUtil::needPage(this); }

//===========================================================================
// TechDraw_ArchView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawArchView)

CmdTechDrawArchView::CmdTechDrawArchView() : Command("TechDraw_ArchView")
{
    // setting the Gui eye-candy
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert BIM Workbench Object");
    sToolTipText = QT_TR_NOOP("Insert a View of a BIM Workbench section plane");
    sWhatsThis = "TechDraw_NewArch";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_ArchView";
}

void CmdTechDrawArchView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    auto objects = getSelection().getObjectsOfType(App::DocumentObject::getClassTypeId());
    App::DocumentObject* archObject = nullptr;
    int archCount = 0;
    for (auto& obj : objects) {
        if (DrawGuiUtil::isArchSection(obj)) {
            archCount++;
            archObject = obj;
        }
    }
    if (archCount > 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("Please select only 1 BIM section plane."));
        return;
    }

    if (!archObject) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("No BIM section plane in selection."));
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Create BIM view"));
    ViewMakers::makeBIMView(page);
    updateActive();
    commitCommand();
}

bool CmdTechDrawArchView::isActive() { return DrawGuiUtil::needPage(this); }

//===========================================================================
// TechDraw_SpreadsheetView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawSpreadsheetView)

CmdTechDrawSpreadsheetView::CmdTechDrawSpreadsheetView() : Command("TechDraw_SpreadsheetView")
{
    // setting the
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Spreadsheet View");
    sToolTipText = QT_TR_NOOP("Insert View to a spreadsheet");
    sWhatsThis = "TechDraw_SpreadsheetView";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_SpreadsheetView";
}

void CmdTechDrawSpreadsheetView::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    const std::vector<App::DocumentObject*> spreads =
        getSelection().getObjectsOfType(Spreadsheet::Sheet::getClassTypeId());
    if (spreads.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("Select exactly one Spreadsheet object."));
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Create spreadsheet view"));
    ViewMakers::makeSpreadsheetView(page);
    updateActive();
    commitCommand();
}

bool CmdTechDrawSpreadsheetView::isActive()
{
    //need a Page and a SpreadSheet::Sheet
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveSheet = false;
    if (havePage) {
        auto spreadSheetType(Spreadsheet::Sheet::getClassTypeId());
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

CmdTechDrawExportPageSVG::CmdTechDrawExportPageSVG() : Command("TechDraw_ExportPageSVG")
{
    sGroup = QT_TR_NOOP("File");
    sMenuText = QT_TR_NOOP("Export Page as SVG");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_ExportPageSVG";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_ExportPageSVG";
}

void CmdTechDrawExportPageSVG::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    Gui::Document* activeGui = Gui::Application::Instance->getDocument(page->getDocument());
    Gui::ViewProvider* vp = activeGui->getViewProvider(page);
    auto* vpPage = dynamic_cast<ViewProviderPage*>(vp);

    if (vpPage) {
        vpPage->show();  // make sure a mdi will be available
        vpPage->getMDIViewPage()->saveSVG();
    }
    else {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("No Drawing Page"),
                             QObject::tr("FreeCAD could not find a page to export"));
        return;
    }
}

bool CmdTechDrawExportPageSVG::isActive() { return DrawGuiUtil::needPage(this); }

//===========================================================================
// TechDraw_ExportPageDXF
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExportPageDXF)

CmdTechDrawExportPageDXF::CmdTechDrawExportPageDXF() : Command("TechDraw_ExportPageDXF")
{
    sGroup = QT_TR_NOOP("File");
    sMenuText = QT_TR_NOOP("Export Page as DXF");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_ExportPageDXF";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_ExportPageDXF";
}

void CmdTechDrawExportPageDXF::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    std::vector<App::DocumentObject*> views = page->getViews();
    for (auto& v : views) {
        if (v->isDerivedFrom<TechDraw::DrawViewArch>()) {
            QMessageBox::StandardButton rc = QMessageBox::question(
                Gui::getMainWindow(), QObject::tr("Can not export selection"),
                QObject::tr("Page contains DrawViewArch which will not be exported. Continue?"),
                QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));
            if (rc == QMessageBox::No) {
                return;
            }
            break;
        }
    }

    QString defaultDir;
    QString fileName = Gui::FileDialog::getSaveFileName(
        Gui::getMainWindow(), QString::fromUtf8(QT_TR_NOOP("Save DXF file")), defaultDir,
        QStringLiteral("DXF (*.dxf)"));

    if (fileName.isEmpty()) {
        return;
    }

    std::string PageName = page->getNameInDocument();
    openCommand(QT_TRANSLATE_NOOP("Command", "Save page to DXF"));
     //NOLINTNEXTLINE
    doCommand(Doc, "import TechDraw");
    fileName = Base::Tools::escapeEncodeFilename(fileName);
    auto filespec = DU::cleanFilespecBackslash(fileName.toStdString());
    //NOLINTNEXTLINE
    doCommand(Doc, "TechDraw.writeDXFPage(App.activeDocument().%s, u\"%s\")", PageName.c_str(),
              filespec.c_str());
    commitCommand();
}


bool CmdTechDrawExportPageDXF::isActive() { return DrawGuiUtil::needPage(this); }

//===========================================================================
// TechDraw_ProjectShape
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawProjectShape)

CmdTechDrawProjectShape::CmdTechDrawProjectShape() : Command("TechDraw_ProjectShape")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Project shape...");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_ProjectShape";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_ProjectShape";
}

void CmdTechDrawProjectShape::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (!CommandHelpers::guardActiveDialog()) {
        return;
    }

    Gui::Control().showDialog(new TaskDlgProjection());
}

bool CmdTechDrawProjectShape::isActive() { return true; }

void CreateTechDrawCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawPageDefault());
    rcCmdMgr.addCommand(new CmdTechDrawPageTemplate());
    rcCmdMgr.addCommand(new CmdTechDrawRedrawPage());
    rcCmdMgr.addCommand(new CmdTechDrawPrintAll());
    rcCmdMgr.addCommand(new CmdTechDrawView());
    rcCmdMgr.addCommand(new CmdTechDrawShapeView());
    rcCmdMgr.addCommand(new CmdTechDrawActiveView());
    rcCmdMgr.addCommand(new CmdTechDrawSectionGroup());
    rcCmdMgr.addCommand(new CmdTechDrawSectionView());
    rcCmdMgr.addCommand(new CmdTechDrawComplexSection());
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
    rcCmdMgr.addCommand(new CmdTechDrawProjectShape());
    rcCmdMgr.addCommand(new CmdTechDrawBrokenView());
}


