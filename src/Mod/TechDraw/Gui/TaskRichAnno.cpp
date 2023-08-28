/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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
# include <cmath>
# include <QDialog>
#endif

#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawRichAnno.h>
#include <Mod/TechDraw/App/LineGroup.h>

#include "ui_TaskRichAnno.h"
#include "TaskRichAnno.h"
#include "mrichtextedit.h"
#include "PreferencesGui.h"
#include "QGIView.h"
#include "QGMText.h"
#include "QGSPage.h"
#include "Rez.h"
#include "ViewProviderPage.h"
#include "ViewProviderRichAnno.h"


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
    m_qgParent(nullptr),
    m_createMode(false),
    m_inProgressLock(false),
    m_btnOK(nullptr),
    m_btnCancel(nullptr),
    m_textDialog(nullptr),
    m_rte(nullptr)
{
    //existence of annoVP is guaranteed by caller being ViewProviderRichAnno.setEdit

    m_annoFeat = m_annoVP->getFeature();

    m_basePage = m_annoFeat->findParentPage();
    if (!m_basePage) {
        Base::Console().Error("TaskRichAnno - bad parameters (2).  Can not proceed.\n");
        return;
    }

    //m_baseFeat can be null
    App::DocumentObject* obj = m_annoFeat->AnnoParent.getValue();
    if (obj) {
        if ( obj->isDerivedFrom(TechDraw::DrawView::getClassTypeId()) )  {
            m_baseFeat = static_cast<TechDraw::DrawView*>(m_annoFeat->AnnoParent.getValue());
        }
    }

    Gui::Document* activeGui = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    Gui::ViewProvider* vp = activeGui->getViewProvider(m_basePage);
    m_vpp = static_cast<ViewProviderPage*>(vp);

    m_qgParent = nullptr;
    if (m_baseFeat) {
        m_qgParent = m_vpp->getQGSPage()->findQViewForDocObj(m_baseFeat);
    }

    ui->setupUi(this);

    m_title = QObject::tr("Rich text editor");
    setUiEdit();

    m_attachPoint = Rez::guiX(Base::Vector3d(m_annoFeat->X.getValue(),
                                            -m_annoFeat->Y.getValue(),
                                             0.0));

    connect(ui->pbEditor, &QPushButton::clicked,
            this, &TaskRichAnno::onEditorClicked);
}

//ctor for creation
TaskRichAnno::TaskRichAnno(TechDraw::DrawView* baseFeat,
                           TechDraw::DrawPage* page) :
    ui(new Ui_TaskRichAnno),
    m_annoVP(nullptr),
    m_baseFeat(baseFeat),
    m_basePage(page),
    m_annoFeat(nullptr),
    m_qgParent(nullptr),
    m_createMode(true),
    m_inProgressLock(false),
    m_btnOK(nullptr),
    m_btnCancel(nullptr),
    m_textDialog(nullptr),
    m_rte(nullptr)
{
    //existence of baseFeat and page guaranteed by CmdTechDrawRichTextAnnotation (CommandAnnotate.cpp)
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    Gui::ViewProvider* vp = activeGui->getViewProvider(m_basePage);
    m_vpp = static_cast<ViewProviderPage*>(vp);

    m_qgParent = nullptr;
    if (m_vpp->getQGSPage()) {
        m_qgParent = m_vpp->getQGSPage()->findQViewForDocObj(baseFeat);
    }

    ui->setupUi(this);
    m_title = QObject::tr("Rich text creator");

    setUiPrimary();

    connect(ui->pbEditor, &QPushButton::clicked,
            this, &TaskRichAnno::onEditorClicked);
}

void TaskRichAnno::updateTask()
{
//    blockUpdate = true;

//    blockUpdate = false;
}

void TaskRichAnno::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskRichAnno::setUiPrimary()
{
//    Base::Console().Message("TRA::setUiPrimary()\n");
    enableVPUi(false);
    setWindowTitle(m_title);

    if (m_baseFeat) {
        std::string baseName = m_baseFeat->getNameInDocument();
        ui->leBaseView->setText(Base::Tools::fromStdString(baseName));
    }
    ui->dsbWidth->setUnit(Base::Unit::Length);
    ui->dsbWidth->setMinimum(0);
    ui->dsbWidth->setValue(prefWeight());

    ui->cpFrameColor->setColor(prefLineColor().asValue<QColor>());
    // set a default font size, use for this the preferences setting
    MRichTextEdit mre;
    ui->teAnnoText->setFontPointSize(mre.getDefFontSizeNum());
    // set a placeholder text to inform the user
    ui->teAnnoText->setPlaceholderText(tr("Input the annotation text directly or start the rich text editor"));
}

void TaskRichAnno::enableTextUi(bool enable)
{
    ui->pbEditor->setEnabled(enable);
    ui->teAnnoText->setEnabled(enable);
}

//switch widgets related to ViewProvider on/off
//there is no ViewProvider until some time after feature is created.
void TaskRichAnno::enableVPUi(bool enable)
{
    Q_UNUSED(enable);
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

    if (m_annoFeat) {
        std::string baseName("None");
        App::DocumentObject* docObj = m_annoFeat->AnnoParent.getValue();
        if (docObj) {
            baseName = docObj->getNameInDocument();
        }
        ui->leBaseView->setText(Base::Tools::fromStdString(baseName));
        ui->teAnnoText->setHtml(QString::fromUtf8(m_annoFeat->AnnoText.getValue()));
        ui->dsbMaxWidth->setValue(m_annoFeat->MaxWidth.getValue());
        ui->cbShowFrame->setChecked(m_annoFeat->ShowFrame.getValue());
    }

    if (m_annoVP) {
        ui->cpFrameColor->setColor(m_annoVP->LineColor.getValue().asValue<QColor>());
        ui->dsbWidth->setValue(m_annoVP->LineWidth.getValue());
        ui->cFrameStyle->setCurrentIndex(m_annoVP->LineStyle.getValue());
    }
}

void TaskRichAnno::onEditorClicked(bool clicked)
{
//    Base::Console().Message("TL::onEditorClicked(%d)\n", b);
    Q_UNUSED(clicked);
    m_textDialog = new QDialog(nullptr);
    QString leadText = ui->teAnnoText->toHtml();
    QString plainText = ui->teAnnoText->toPlainText();
    if (plainText.isEmpty()) {
        m_rte = new MRichTextEdit(m_textDialog);
    } else {
        m_rte = new MRichTextEdit(m_textDialog, leadText);
    }
    QGridLayout* gl = new QGridLayout(m_textDialog);
    gl->addWidget(m_rte, 0,0, 1,1);
    m_textDialog->setWindowTitle(QObject::tr("Rich text editor"));
    m_textDialog->setMinimumWidth (400);
    m_textDialog->setMinimumHeight(400);

    connect(m_rte, &MRichTextEdit::saveText,
            this, &TaskRichAnno::onSaveAndExit);
    connect(m_rte, &MRichTextEdit::editorFinished,
            this, &TaskRichAnno::onEditorExit);

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

void TaskRichAnno::onEditorExit()
{
    m_textDialog->reject();
    m_textDialog = nullptr;
    m_rte = nullptr;
}

double TaskRichAnno::prefWeight() const
{
    return TechDraw::LineGroup::getDefaultWidth("Graphic");
}

App::Color TaskRichAnno::prefLineColor()
{
    return PreferencesGui::leaderColor();
}


//******************************************************************************
void TaskRichAnno::createAnnoFeature()
{
//    Base::Console().Message("TRA::createAnnoFeature()");
    const std::string objectName{QT_TR_NOOP("RichTextAnnotation")};
    std::string annoName = m_basePage->getDocument()->getUniqueObjectName(objectName.c_str());
    std::string generatedSuffix {annoName.substr(objectName.length())};
    std::string annoType = "TechDraw::DrawRichAnno";

    std::string PageName = m_basePage->getNameInDocument();

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Anno"));
    Command::doCommand(Command::Doc, "App.activeDocument().addObject('%s', '%s')",
                       annoType.c_str(), annoName.c_str());
    Command::doCommand(Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)",
                       PageName.c_str(), annoName.c_str());

    if (m_baseFeat) {
        Command::doCommand(Command::Doc, "App.activeDocument().%s.AnnoParent = App.activeDocument().%s",
                               annoName.c_str(), m_baseFeat->getNameInDocument());
    }
    App::DocumentObject* obj = m_basePage->getDocument()->getObject(annoName.c_str());
    if (!obj) {
        throw Base::RuntimeError("TaskRichAnno - new RichAnno object not found");
    }
    if (obj->isDerivedFrom(TechDraw::DrawRichAnno::getClassTypeId())) {
        m_annoFeat = static_cast<TechDraw::DrawRichAnno*>(obj);
        commonFeatureUpdate();
        if (m_baseFeat) {
            QPointF qTemp = calcTextStartPos(m_annoFeat->getScale());
            Base::Vector3d vTemp(qTemp.x(), qTemp.y());
            m_annoFeat->X.setValue(Rez::appX(vTemp.x));
            m_annoFeat->Y.setValue(Rez::appX(vTemp.y));
        } else {
            //if we don't have a base featrue, we can't calculate start position, so just put it mid-page
            m_annoFeat->X.setValue(m_basePage->getPageWidth()/2.0);
            m_annoFeat->Y.setValue(m_basePage->getPageHeight()/2.0);
        }
    }

    if (m_annoFeat) {
        Gui::ViewProvider* vp = QGIView::getViewProvider(m_annoFeat);
        auto annoVP = dynamic_cast<ViewProviderRichAnno*>(vp);
        if (annoVP) {
            App::Color ac;
            ac.setValue<QColor>(ui->cpFrameColor->color());
            annoVP->LineColor.setValue(ac);
            annoVP->LineWidth.setValue(ui->dsbWidth->rawValue());
            annoVP->LineStyle.setValue(ui->cFrameStyle->currentIndex());
        }
    }

    std::string translatedObjectName{tr(objectName.c_str()).toStdString()};
    obj->Label.setValue(translatedObjectName + generatedSuffix);

    Gui::Command::commitCommand();
    Gui::Command::updateActive();

    //trigger claimChildren in tree
    if (m_baseFeat) {
        m_baseFeat->touch();
    }

    m_basePage->touch();

    if (m_annoFeat) {
        m_annoFeat->requestPaint();
    }
}

void TaskRichAnno::updateAnnoFeature()
{
//    Base::Console().Message("TRA::updateAnnoFeature()\n");
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Edit Anno"));
    commonFeatureUpdate();
    App::Color ac;
    ac.setValue<QColor>(ui->cpFrameColor->color());
    m_annoVP->LineColor.setValue(ac);
    m_annoVP->LineWidth.setValue(ui->dsbWidth->rawValue());
    m_annoVP->LineStyle.setValue(ui->cFrameStyle->currentIndex());

    Gui::Command::commitCommand();
    Gui::Command::updateActive();
}

void TaskRichAnno::commonFeatureUpdate()
{
//    Base::Console().Message("TRA::commonFeatureUpdate()\n");
    m_annoFeat->setPosition(Rez::appX(m_attachPoint.x), Rez::appX(- m_attachPoint.y), true);
    m_annoFeat->AnnoText.setValue(ui->teAnnoText->toHtml().toUtf8());
    m_annoFeat->MaxWidth.setValue(ui->dsbMaxWidth->value().getValue());
    m_annoFeat->ShowFrame.setValue(ui->cbShowFrame->isChecked());
}

void TaskRichAnno::removeFeature()
{
//    Base::Console().Message("TRA::removeFeature()\n");
    if (!m_annoFeat)
        return;

    if (m_createMode) {
        try {
            // this doesn't remove the QGMText item??
            std::string PageName = m_basePage->getNameInDocument();
            Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().%s.removeView(App.activeDocument().%s)",
                                    PageName.c_str(), m_annoFeat->getNameInDocument());
            Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().removeObject('%s')",
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
        }
    }
}

//we don't know the bounding rect of the text, so we have to calculate a reasonable
//guess at the size of the text block.
QPointF TaskRichAnno::calcTextStartPos(double scale)
{
    Q_UNUSED(scale)
//    Base::Console().Message("TRA::calcTextStartPos(%.3f)\n", scale);
    double textWidth = 100.0;
    double textHeight = 20.0;
    double horizGap(20.0);
    double tPosX(0.0);
    double tPosY(0.0);

    double width = m_annoFeat->MaxWidth.getValue();
    if (width > 0 ) {
        textWidth = width;
    }

    std::vector<Base::Vector3d> points;
    if (m_baseFeat) {
        if (m_baseFeat->isDerivedFrom(TechDraw::DrawLeaderLine::getClassTypeId())) {
            TechDraw::DrawLeaderLine* dll = static_cast<TechDraw::DrawLeaderLine*>(m_baseFeat);
            points = dll->WayPoints.getValues();
        } else {
//            Base::Console().Message("TRA::calcTextPos - m_baseFeat is not Leader\n");
            return QPointF(0.0, 0.0);
        }
    } else {
//        Base::Console().Message("TRA::calcStartPos - no m_baseFeat\n");
        if (m_basePage) {
            double w = Rez::guiX(m_basePage->getPageWidth() / 2.0);
            double h = Rez::guiX(m_basePage->getPageHeight() / 2.0);
            return QPointF(w, h);
        } else {
            Base::Console().Message("TRA::calcStartPos - no m_basePage\n"); //shouldn't happen. caught elsewhere
        }
    }

    if (!points.empty()) {
        QPointF lastPoint(points.back().x, points.back().y);
        QPointF firstPoint(points.front().x, points.front().y);
        QPointF lastOffset = lastPoint - firstPoint;

        if (lastPoint.x() < firstPoint.x()) {                 //last is left of first
            tPosX = lastOffset.x() - horizGap - textWidth;    //left of last
            tPosY = lastOffset.y() - textHeight;
        } else {                                             //last is right of first
            tPosX = lastOffset.x() + horizGap;               //right of last
            tPosY = lastOffset.y() - textHeight;
        }
    }
    return QPointF(tPosX, -tPosY);
}

void TaskRichAnno::saveButtons(QPushButton* btnOK,
                             QPushButton* btnCancel)
{
    m_btnOK = btnOK;
    m_btnCancel = btnCancel;
}

void TaskRichAnno::enableTaskButtons(bool enable)
{
    m_btnOK->setEnabled(enable);
    m_btnCancel->setEnabled(enable);
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
    if (!doc)
        return false;

    if (getCreateMode())  {
        createAnnoFeature();
    } else {
        updateAnnoFeature();
    }

    m_annoFeat->requestPaint();

    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskRichAnno::reject()
{
//    Base::Console().Message("TRA::reject()\n");
    if (m_inProgressLock) {
//        Base::Console().Message("TRA::reject - edit in progress!!\n");
        return false;
    }

    if (m_basePage) {
        Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
        if (!doc) {
            return false;
        }
        if (getCreateMode() && m_annoFeat)  {
            removeFeature();
        }
    }

    //make sure any dangling objects are cleaned up
    Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().recompute()");
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgRichAnno::TaskDlgRichAnno(TechDraw::DrawView* baseFeat,
                                 TechDraw::DrawPage* page)
    : TaskDialog()
{
    widget  = new TaskRichAnno(baseFeat, page);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_RichTextAnnotation"),
                                              widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgRichAnno::TaskDlgRichAnno(TechDrawGui::ViewProviderRichAnno* annoVP)
    : TaskDialog()
{
    widget  = new TaskRichAnno(annoVP);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_RichTextAnnotation"),
                                         widget->windowTitle(), true, nullptr);
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
