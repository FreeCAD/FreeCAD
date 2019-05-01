/***************************************************************************
 *   Copyright (c) 2019 Wandererfan <wandererfan@gmail.com                 *
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
#include <cmath>
#endif // #ifndef _PreComp_

#include <QStatusBar>
#include <QGraphicsScene>
#include <QDialog>

#include <Base/Console.h>
#include <Base/Tools.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawRichAnno.h>

#include <Mod/TechDraw/Gui/ui_TaskRichAnno.h>

#include "DrawGuiStd.h"
#include "QGVPage.h"
#include "QGIView.h"
#include "QGIPrimPath.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include "ViewProviderRichAnno.h"
#include "QGMText.h"
#include "QGIRichAnno.h"
#include "Rez.h"
#include "mrichtextedit.h"
#include "mtextedit.h"

#include "TaskRichAnno.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

//ctor for edit
TaskRichAnno::TaskRichAnno(TechDrawGui::ViewProviderRichAnno* annoVP) :
    ui(new Ui_TaskRichAnno),
    m_annoVP(annoVP),
    m_baseFeat(nullptr),
    m_basePage(nullptr),
    m_annoFeat(nullptr),
    m_createMode(false),
    m_inProgressLock(false),
    m_qgAnno(nullptr),
    m_textDialog(nullptr),
    m_rte(nullptr)
{
    if (m_annoVP == nullptr)  {
        //should be caught in CMD caller
        Base::Console().Error("TaskRichAnno - bad parameters.  Can not proceed.\n");
        return;
    }
    ui->setupUi(this);
    
    m_annoFeat = m_annoVP->getFeature();

    //m_baseFeat can be null 
    App::DocumentObject* obj = m_annoFeat->AnnoParent.getValue();
    if (obj != nullptr) {
        if ( obj->isDerivedFrom(TechDraw::DrawView::getClassTypeId()) )  {
            m_baseFeat = static_cast<TechDraw::DrawView*>(m_annoFeat->AnnoParent.getValue());
        }
    }
    m_basePage = m_annoFeat->findParentPage();
    if ( m_basePage == nullptr ) {
        Base::Console().Error("TaskRichAnno - bad parameters (2).  Can not proceed.\n");
        return;
    }

    setUiEdit();
//    m_title = QObject::tr("Rich text editor");

    m_mdi = m_annoVP->getMDIViewPage();
    m_scene = m_mdi->m_scene;
    m_view = m_mdi->getQGVPage();
    if (m_baseFeat != nullptr) {
        m_qgParent = m_view->findQViewForDocObj(m_baseFeat);
    }
    
    m_saveContextPolicy = m_mdi->contextMenuPolicy();

    m_attachPoint = Rez::guiX(Base::Vector3d(m_annoFeat->X.getValue(),
                                            -m_annoFeat->Y.getValue(),
                                             0.0));

    connect(ui->pbEditor, SIGNAL(clicked(bool)),
                this, SLOT(onEditorClicked(bool)));
}

//ctor for creation
TaskRichAnno::TaskRichAnno(TechDraw::DrawView* baseFeat,
                           TechDraw::DrawPage* page) :
    ui(new Ui_TaskRichAnno),
    m_annoVP(nullptr),
    m_baseFeat(baseFeat),
    m_basePage(page),
    m_annoFeat(nullptr),
    m_createMode(true),
    m_inProgressLock(false),
    m_qgAnno(nullptr),
    m_textDialog(nullptr),
    m_rte(nullptr)
{
    if (m_basePage == nullptr)  {
        //should be caught in CMD caller
        Base::Console().Error("TaskRichAnno - bad parameters.  Can not proceed.\n");
        return;
    }

    ui->setupUi(this);
    m_title = QObject::tr("Rich text creator");

    Gui::Document* activeGui = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    Gui::ViewProvider* vp = activeGui->getViewProvider(m_basePage);
    ViewProviderPage* vpp = static_cast<ViewProviderPage*>(vp);
    m_mdi = vpp->getMDIViewPage();
    m_scene = m_mdi->m_scene;
    m_view = m_mdi->getQGVPage();
    if (baseFeat != nullptr) {
        m_qgParent = m_view->findQViewForDocObj(baseFeat);
    }

    m_saveContextPolicy = m_mdi->contextMenuPolicy();

    setUiPrimary();

    connect(ui->pbEditor, SIGNAL(clicked(bool)),
                this, SLOT(onEditorClicked(bool)));
}

TaskRichAnno::~TaskRichAnno()
{
    delete ui;
}

void TaskRichAnno::updateTask()
{
//    blockUpdate = true;

//    blockUpdate = false;
}

void TaskRichAnno::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskRichAnno::setUiPrimary()
{
//    Base::Console().Message("TRA::setUiPrimary()\n");
    enableVPUi(false);
    setWindowTitle(m_title);

    if (m_baseFeat != nullptr) {
        std::string baseName = m_baseFeat->getNameInDocument();
        ui->leBaseView->setText(Base::Tools::fromStdString(baseName));
    }
}

void TaskRichAnno::enableTextUi(bool b) 
{
    ui->pbEditor->setEnabled(b);
    ui->teAnnoText->setEnabled(b);
}

void TaskRichAnno::enableVPUi(bool b)
{
    Q_UNUSED(b);
//    ui->cpLineColor->setEnabled(b);
//    ui->dsbWeight->setEnabled(b);
//    ui->cboxStyle->setEnabled(b);
}

void TaskRichAnno::setUiEdit()
{
//    Base::Console().Message("TRA::setUiEdit());
    enableVPUi(true);
    setWindowTitle(m_title);
    enableTextUi(true);
    
    if (m_annoFeat != nullptr) {
        std::string baseName("None");
        App::DocumentObject* docObj = m_annoFeat->AnnoParent.getValue();
        if (docObj != nullptr) {
            baseName = docObj->getNameInDocument();
        }
        ui->leBaseView->setText(Base::Tools::fromStdString(baseName));
        ui->teAnnoText->setHtml(QString::fromUtf8(m_annoFeat->AnnoText.getValue()));
        ui->dsbMaxWidth->setValue(m_annoFeat->MaxWidth.getValue());
        ui->cbShowFrame->setChecked(m_annoFeat->ShowFrame.getValue());
    }
    
    if (m_annoVP != nullptr) {
//        ui->cpLineColor->setColor(m_annoVP->Color.getValue().asValue<QColor>());
//        ui->dsbWeight->setValue(m_annoVP->LineWidth.getValue());
//        ui->cboxStyle->setCurrentIndex(m_annoVP->LineStyle.getValue());
    }
}

void TaskRichAnno::onEditorClicked(bool b)
{
//    Base::Console().Message("TL::onEditorClicked(%d)\n",b);
    Q_UNUSED(b);
    m_textDialog = new QDialog(0);
    QString leadText = ui->teAnnoText->toHtml();
    m_rte = new MRichTextEdit(m_textDialog, leadText);
    //m_rte->setTextWidth(m_annoVP->MaxWidth);
    QGridLayout* gl = new QGridLayout(m_textDialog);
    gl->addWidget(m_rte,0,0,1,1);
    m_textDialog->setWindowTitle(QObject::tr("Rich text editor"));
    m_textDialog->setMinimumWidth (400);
    m_textDialog->setMinimumHeight(400);

    connect(m_rte, SIGNAL(saveText(QString)),
            this, SLOT(onSaveAndExit(QString)));
    connect(m_rte, SIGNAL(editorFinished(void)),
            this, SLOT(onEditorExit(void)));

//    m_textDialog->setFont(m_annoVP->Font.getValue());
//    m_textDialog->setFontSize(m_annoVP->FontSize.getValue());
    m_textDialog->show();
}

void TaskRichAnno::onSaveAndExit(QString qs)
{
    ui->teAnnoText->setHtml(qs);
    //dialog clean up should be handled by accept() call in dialog
    m_textDialog->accept();
    m_textDialog = nullptr;
    m_rte = nullptr;
}

void TaskRichAnno::onEditorExit(void)
{
    m_textDialog->reject();
    m_textDialog = nullptr;
    m_rte = nullptr;
}

//******************************************************************************
void TaskRichAnno::createAnnoFeature()
{
//    Base::Console().Message("TRA::createAnnoFeature()");
    std::string annoName = m_basePage->getDocument()->getUniqueObjectName("DrawRichAnno");
    std::string annoType = "TechDraw::DrawRichAnno";

    std::string PageName = m_basePage->getNameInDocument();

    Gui::Command::openCommand("Create Leader");
    Command::doCommand(Command::Doc,"App.activeDocument().addObject('%s','%s')",
                       annoType.c_str(),annoName.c_str());
    Command::doCommand(Command::Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",
                       PageName.c_str(),annoName.c_str());

    if (m_baseFeat != nullptr) {
        Command::doCommand(Command::Doc,"App.activeDocument().%s.AnnoParent = App.activeDocument().%s",
                               annoName.c_str(),m_baseFeat->getNameInDocument());
    }
    App::DocumentObject* obj = m_basePage->getDocument()->getObject(annoName.c_str());
    if (obj == nullptr) {
        throw Base::RuntimeError("TaskRichAnno - new RichAnno object not found");
    }
    if (obj->isDerivedFrom(TechDraw::DrawRichAnno::getClassTypeId())) {
        m_annoFeat = static_cast<TechDraw::DrawRichAnno*>(obj);
        commonFeatureUpdate();
        QPointF qTemp = calcTextStartPos(m_annoFeat->getScale());
        Base::Vector3d vTemp(qTemp.x(), qTemp.y());
//        m_annoFeat->TextPosition.setValue(Rez::appX(vTemp));
        m_annoFeat->X.setValue(Rez::appX(vTemp.x));
        m_annoFeat->Y.setValue(Rez::appX(vTemp.y));
    }

    Gui::Command::updateActive();
    Gui::Command::commitCommand();
    m_annoFeat->requestPaint();
}

void TaskRichAnno::updateAnnoFeature()
{
//    Base::Console().Message("TRA::updateAnnoFeature()\n");
    Gui::Command::openCommand("Edit Leader");
    commonFeatureUpdate();
//    App::Color ac;
//    ac.setValue<QColor>(ui->cpLineColor->color());
//    m_annoVP->Color.setValue(ac);

    Gui::Command::commitCommand();
    m_annoFeat->requestPaint();
}

void TaskRichAnno::commonFeatureUpdate(void)
{
//    Base::Console().Message("TRA::commonFeatureUpdate()\n");
    m_annoFeat->setPosition(Rez::appX(m_attachPoint.x),Rez::appX(- m_attachPoint.y), true);
    m_annoFeat->AnnoText.setValue(ui->teAnnoText->toHtml().toUtf8()); 
    m_annoFeat->MaxWidth.setValue(ui->dsbMaxWidth->value());
    m_annoFeat->ShowFrame.setValue(ui->cbShowFrame->isChecked());
}

void TaskRichAnno::removeFeature(void)
{
//    Base::Console().Message("TRA::removeFeature()\n");
    if (m_annoFeat != nullptr) {
        if (m_createMode) {
            try {
                // this doesn't remove the QGMText item??
                std::string PageName = m_basePage->getNameInDocument();
                Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().%s.removeView(App.activeDocument().%s)",
                                        PageName.c_str(),m_annoFeat->getNameInDocument());
                Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().removeObject('%s')",
                                         m_annoFeat->getNameInDocument());
            }
            catch (...) {
                Base::Console().Warning("TRA::removeFeature - failed to delete feature\n");
                return;
            }
        } else {
            if (Gui::Command::hasPendingCommand()) {
                std::vector<std::string> undos = Gui::Application::Instance->activeDocument()->getUndoVector();
                Gui::Application::Instance->activeDocument()->undo(1);
            } else {
                Base::Console().Log("TaskRichAnno: Edit mode - NO command is active\n");
            }
        }
    }
}

//we don't know the bounding rect of the text, so we have to calculate a reasonable
//guess at the size of the text block.
QPointF TaskRichAnno::calcTextStartPos(double scale)
{
//    Base::Console().Message("TRA::calcTextStartPos(%.3f)\n", scale);
    double textWidth = 100.0;
    double textHeight = 20.0;
    double horizGap(20.0);
    double tPosX(0.0);
    double tPosY(0.0);

    Gui::ViewProvider* vp = QGIView::getViewProvider(m_annoFeat);
    
    if (vp != nullptr) {
        ViewProviderRichAnno* vpra = dynamic_cast<ViewProviderRichAnno*>(vp);
        if (vpra != nullptr) {
            double adjust = 2.0;
            double fsize = vpra->Fontsize.getValue();
            textHeight = fsize * adjust;
            double width = m_annoFeat->MaxWidth.getValue();
            if (width > 0 ) {
                textWidth = width;
            }
        }
    }

    std::vector<Base::Vector3d> points;
    if (m_baseFeat != nullptr) {
        if (m_baseFeat->isDerivedFrom(TechDraw::DrawLeaderLine::getClassTypeId())) {
            TechDraw::DrawLeaderLine* dll = dynamic_cast<TechDraw::DrawLeaderLine*>(m_baseFeat);
            points = dll->WayPoints.getValues();
        } else {
            Base::Console().Log("TRA::calcTextPos - m_baseFeat is not Leader\n");
            QPointF result(0.0,0.0);
            return result;
        }
    } else {
        if (m_basePage != nullptr) {
            double w = Rez::guiX(m_basePage->getPageWidth() / 2.0);
            double h = Rez::guiX(m_basePage->getPageHeight() / 2.0);
            QPointF result(w,h);
            return result;
        }
    }

    if (!points.empty()) {
        QPointF lastPoint(points.back().x, points.back().y);
        QPointF firstPoint(points.front().x, points.front().y);
        QPointF lastOffset = lastPoint;
        lastPoint = m_qgParent->mapFromScene(lastPoint) * scale;
        firstPoint = m_qgParent->mapFromScene(firstPoint) * scale;

        if (lastPoint.x() < firstPoint.x()) {                 //last is left of first
            tPosX = lastOffset.x() - horizGap - textWidth;    //left of last
            tPosY = lastOffset.y() - textHeight;
        } else {                                             //last is right of first
            tPosX = lastOffset.x() + horizGap;               //right of last
            tPosY = lastOffset.y() - textHeight;
        }
    }
    QPointF result(tPosX, -tPosY);
    return result;
}

void TaskRichAnno::saveButtons(QPushButton* btnOK,
                             QPushButton* btnCancel)
{
    m_btnOK = btnOK;
    m_btnCancel = btnCancel;
}

void TaskRichAnno::enableTaskButtons(bool b)
{
    m_btnOK->setEnabled(b);
    m_btnCancel->setEnabled(b);
}

//******************************************************************************

bool TaskRichAnno::accept()
{
//    Base::Console().Message("TRA::accept()\n");
    if (m_inProgressLock) {
//        Base::Console().Message("TRA::accept - edit in progress!!\n");
        //TODO: kill MRTE dialog?
        return true;
    }

    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc) return false;

    if (!getCreateMode())  {
        updateAnnoFeature();
    } else {
        createAnnoFeature();
    }
    m_mdi->setContextMenuPolicy(m_saveContextPolicy);
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskRichAnno::reject()
{
    Base::Console().Message("TRA::reject()\n");
    if (m_inProgressLock) {
//        Base::Console().Message("TRA::reject - edit in progress!!\n");
        return false;
    }
    
    if (m_basePage != nullptr) {
        Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
        if (!doc) {
            return false;
        }

        Base::Console().Message("TRA::reject() - m_mdi: %X\n", m_mdi);
        if (m_mdi != nullptr) {
            m_mdi->setContextMenuPolicy(m_saveContextPolicy);
        }
        if (getCreateMode() &&
            (m_annoFeat != nullptr) )  {
            removeFeature();
        }
    }

    //make sure any dangling objects are cleaned up 
    Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgRichAnno::TaskDlgRichAnno(TechDraw::DrawView* baseFeat,
                                 TechDraw::DrawPage* page)
    : TaskDialog()
{
    widget  = new TaskRichAnno(baseFeat,page);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-textleader"),
                                              widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgRichAnno::TaskDlgRichAnno(TechDrawGui::ViewProviderRichAnno* leadVP)
    : TaskDialog()
{
    widget  = new TaskRichAnno(leadVP);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-textleader"),
                                         widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgRichAnno::~TaskDlgRichAnno()
{
}

void TaskDlgRichAnno::update()
{
//    widget->updateTask();
}

void TaskDlgRichAnno::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btnOK = box->button(QDialogButtonBox::Ok);
    QPushButton* btnCancel = box->button(QDialogButtonBox::Cancel);
    widget->saveButtons(btnOK, btnCancel);
}

//==== calls from the TaskView ===============================================================
void TaskDlgRichAnno::open()
{
}

void TaskDlgRichAnno::clicked(int)
{
}

bool TaskDlgRichAnno::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgRichAnno::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskRichAnno.cpp>
