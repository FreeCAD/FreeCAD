/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#include <QApplication>
#include <QStatusBar>
#include <QGraphicsScene>
#include <QMessageBox>

#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSection.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "DrawGuiStd.h"
#include "Rez.h"
#include "MDIViewPage.h"
#include "QGVPage.h"
#include "QGIView.h"

//#include "ViewProviderPage.h"
//#include "ViewProviderViewPart.h"

#include <Mod/TechDraw/Gui/ui_TaskSectionView.h>

#include "TaskSectionView.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

//ctor for create 
TaskSectionView::TaskSectionView(TechDraw::DrawViewPart* base) :
    ui(new Ui_TaskSectionView),
    m_base(base),
    m_section(nullptr),
    m_dirName(""),
    m_createMode(true),
    m_saved(false)
{
//    Base::Console().Message("TSV::TSV() - create mode\n");
    if  (m_base == nullptr)  {
        //should be caught in CMD caller
        std::string msg = Base::Tools::toStdString(tr("TaskSectionView - bad parameters.  Can not proceed."));
        Base::Console().Error((msg + "\n").c_str());
        return;
    }
 
   ui->setupUi(this);

    connect(ui->pbUp, SIGNAL(clicked(bool)),
            this, SLOT(onUpClicked(bool)));
    connect(ui->pbDown, SIGNAL(clicked(bool)),
            this, SLOT(onDownClicked(bool)));
    connect(ui->pbRight, SIGNAL(clicked(bool)),
            this, SLOT(onRightClicked(bool)));
    connect(ui->pbLeft, SIGNAL(clicked(bool)),
            this, SLOT(onLeftClicked(bool)));

    setUiPrimary();
}


//ctor for edit
TaskSectionView::TaskSectionView(TechDraw::DrawViewSection* section) :
    ui(new Ui_TaskSectionView),
    m_base(nullptr),
    m_section(section),
    m_createMode(false),
    m_saved(false)
{
//    Base::Console().Message("TS::TS() - edit mode\n");

    if  (m_section == nullptr)  {
        //should be caught in CMD caller
        std::string msg = Base::Tools::toStdString(tr("TaskSectionView - bad parameters.  Can not proceed."));
        Base::Console().Error((msg + "\n").c_str());
        return;
    }

    App::DocumentObject* newObj = m_section->BaseView.getValue();
    m_base = dynamic_cast<TechDraw::DrawViewPart*>(newObj);
    if ( (newObj == nullptr) ||
         (m_base == nullptr) ) {
        throw Base::RuntimeError("TaskSectionView - BaseView not found");
    }

    ui->setupUi(this);

    connect(ui->pbUp, SIGNAL(clicked(bool)),
            this, SLOT(onUpClicked(bool)));
    connect(ui->pbDown, SIGNAL(clicked(bool)),
            this, SLOT(onDownClicked(bool)));
    connect(ui->pbRight, SIGNAL(clicked(bool)),
            this, SLOT(onRightClicked(bool)));
    connect(ui->pbLeft, SIGNAL(clicked(bool)),
            this, SLOT(onLeftClicked(bool)));

    m_dirName = m_section->SectionDirection.getValue();
    saveSectionState();
    setUiEdit();
}

TaskSectionView::~TaskSectionView()
{
    delete ui;
}

void TaskSectionView::setUiPrimary()
{
//    Base::Console().Message("TSV::setUiPrimary()\n");
    setWindowTitle(QObject::tr("Create SectionView"));
    std::string temp = m_base->getNameInDocument();
    QString qTemp    = Base::Tools::fromStdString(temp);
    ui->leBaseView->setText(qTemp);

    //TODO: get next symbol from page
//    ui->leSymbol->setText();

    Base::Vector3d origin = m_base->getCentroid();
    ui->sbOrgX->setValue(origin.x);
    ui->sbOrgY->setValue(origin.y);
    ui->sbOrgZ->setValue(origin.z);
}

void TaskSectionView::setUiEdit()
{
//    Base::Console().Message("TSV::setUiEdit()\n");
    setWindowTitle(QObject::tr("Edit SectionView"));

    std::string temp = m_base->getNameInDocument();
    QString qTemp    = Base::Tools::fromStdString(temp);
    ui->leBaseView->setText(qTemp);

    temp = m_section->SectionSymbol.getValue();
    qTemp    = Base::Tools::fromStdString(temp);
    ui->leSymbol->setText(qTemp);
    
    Base::Vector3d origin = m_section->SectionOrigin.getValue();
    ui->sbOrgX->setValue(origin.x);
    ui->sbOrgY->setValue(origin.y);
    ui->sbOrgZ->setValue(origin.z);
}

//save the start conditions
void TaskSectionView::saveSectionState()
{
//    Base::Console().Message("TSV::saveSectionState()\n");
    if (m_section != nullptr) {
        m_saveSymbol = m_section->SectionSymbol.getValue();
        m_saveNormal = m_section->SectionNormal.getValue();
        m_saveDirection = m_section->Direction.getValue();
        m_saveOrigin    = m_section->SectionOrigin.getValue();
        m_saveDirName   = m_section->SectionDirection.getValue();
        m_saved = true;
    }
}

//restore the start conditions
void TaskSectionView::restoreSectionState()
{
//    Base::Console().Message("TSV::restoreSectionState()\n");
    if (m_section != nullptr) {
        m_section->SectionSymbol.setValue(m_saveSymbol);
        m_section->SectionNormal.setValue(m_saveNormal);
        m_section->Direction.setValue(m_saveDirection);
        m_section->SectionOrigin.setValue(m_saveOrigin);
        m_section->SectionDirection.setValue(m_saveDirName.c_str());
    }
}

void TaskSectionView::blockButtons(bool b)
{
    Q_UNUSED(b);
}

// cardinal: 0 - left, 1 - right, 2 - up, 3 - down
void TaskSectionView::onUpClicked(bool b)
{
//    Base::Console().Message("TSV::onUpClicked()\n");
    Q_UNUSED(b);
    checkAll(false);
    ui->pbUp->setChecked(true);
    applyQuick("Up");
}

void TaskSectionView::onDownClicked(bool b)
{
//    Base::Console().Message("TSV::onDownClicked()\n");
    Q_UNUSED(b);
    checkAll(false);
    ui->pbDown->setChecked(true);
    applyQuick("Down");
}

void TaskSectionView::onLeftClicked(bool b)
{
//    Base::Console().Message("TSV::onLeftClicked()\n");
    checkAll(false);
    ui->pbLeft->setChecked(true);
    Q_UNUSED(b);
    applyQuick("Left");
}

void TaskSectionView::onRightClicked(bool b)
{
//    Base::Console().Message("TSV::onRightClicked()\n");
    Q_UNUSED(b);
    checkAll(false);
    ui->pbRight->setChecked(true);
    applyQuick("Right");
}

bool TaskSectionView::apply()
{
//    Base::Console().Message("TSV::apply()\n");
    if (m_dirName.empty()) {
        std::string msg = Base::Tools::toStdString(tr("TSV::apply - No section direction picked yet"));
        Base::Console().Error((msg + "\n").c_str());
    } else {
        checkAll(false);
        applyQuick(m_dirName);
    }
    return true;
}

void TaskSectionView::checkAll(bool b)
{
    ui->pbUp->setChecked(b);
    ui->pbDown->setChecked(b);
    ui->pbRight->setChecked(b);
    ui->pbLeft->setChecked(b);
}

//******************************************************************************
void TaskSectionView::applyQuick(std::string dir)
{
//    Base::Console().Message("TSV::applyQuick(%s)\n", dir.c_str());
    m_dirName = dir;
    Gui::Command::openCommand("Apply Quick");
    m_dirName = dir;
    if (m_section == nullptr) {
        m_section = createSectionView();
    }
    updateSectionView();
    m_section->recomputeFeature();
//    m_section->requestPaint();
    m_base->requestPaint();
}

void TaskSectionView::applyAligned(void) 
{
    Base::Console().Message("TSV::applyAligned() - not implemented yet\n");
    Gui::Command::openCommand("Apply Aligned");
    m_dirName = "Aligned";
    //fiddle with directions here

//    m_section->recomputeFeature(); //????
    m_section->requestPaint();
    m_base->requestPaint();
}

TechDraw::DrawViewSection* TaskSectionView::createSectionView(void)
{
//    Base::Console().Message("TSV::createSectionView()\n");

    std::string sectionName;
    std::string baseName = m_base->getNameInDocument();

    Gui::Command::openCommand("Create SectionView");
    TechDraw::DrawViewSection* newSection = nullptr;
    if (m_section == nullptr) {
        sectionName = m_base->getDocument()->getUniqueObjectName("DrawViewSection");
        std::string sectionType = "TechDraw::DrawViewSection";

        TechDraw::DrawPage* page = m_base->findParentPage();
        std::string pageName = page->getNameInDocument();

        Command::doCommand(Command::Doc,"App.activeDocument().addObject('%s','%s')",
                           sectionType.c_str(),sectionName.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",
                           pageName.c_str(), sectionName.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.BaseView = App.activeDocument().%s",
                           sectionName.c_str(),baseName.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.Source = App.activeDocument().%s.Source",
                           sectionName.c_str(),baseName.c_str());
        App::DocumentObject* newObj = m_base->getDocument()->getObject(sectionName.c_str());
        newSection = dynamic_cast<TechDraw::DrawViewSection*>(newObj);
        if ( (newObj == nullptr) ||
             (newSection == nullptr) ) {
            throw Base::RuntimeError("TaskSectionView - new section object not found");
         }
    }
    return newSection;
}

void TaskSectionView::updateSectionView(void)
{
//    Base::Console().Message("TSV::updateSectionView()\n");
    if (m_section != nullptr) {
        std::string sectionName = m_section->getNameInDocument();
        Command::doCommand(Command::Doc,"App.activeDocument().%s.SectionDirection = '%s'",
                           sectionName.c_str(),m_dirName.c_str());
        Command::doCommand(Command::Doc,
                           "App.activeDocument().%s.SectionOrigin = FreeCAD.Vector(%.3f,%.3f,%.3f)",
                           sectionName.c_str(), 
                           ui->sbOrgX->value().getValue(),
                           ui->sbOrgY->value().getValue(),
                           ui->sbOrgZ->value().getValue());
        QString qTemp    = ui->leSymbol->text();
        std::string temp = Base::Tools::toStdString(qTemp);
        Command::doCommand(Command::Doc,"App.activeDocument().%s.SectionSymbol = '%s'",
                           sectionName.c_str(),
                           temp.c_str());
        m_section->setNormalFromBase(m_dirName.c_str());
    }
}

void TaskSectionView::saveButtons(QPushButton* btnOK,
                             QPushButton* btnCancel,
                             QPushButton* btnApply)
{
    m_btnOK = btnOK;
    m_btnCancel = btnCancel;
    m_btnApply = btnApply;
}

//std::string TaskSectionView::prefViewSection()
//{
////    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
////                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/Section");
////                                    
////    std::string prefString = hGrp->GetASCII("SectionPref", "default");
////    return prefString;
//}

//******************************************************************************

bool TaskSectionView::accept()
{
//    Base::Console().Message("TSV::accept()\n");
    if (m_createMode) {
        if (m_section == nullptr) {
            apply();
        }
        Gui::Command::updateActive();
        Gui::Command::commitCommand();
    } else {
        Gui::Command::openCommand("Edit SectionView");
        try {
            updateSectionView();
        }
        catch (...) {
            Base::Console().Error("TSV::accept - failed to update section\n");
        }

        Gui::Command::updateActive();
        Gui::Command::commitCommand();
    }
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
    if (m_section != nullptr) {
//        m_section->recomputeFeature();
        m_section->requestPaint();
    }
    if (m_base != nullptr) {
        m_base->requestPaint();
    }
    return true;
}

bool TaskSectionView::reject()
{
//    Base::Console().Message("TSV::reject()\n");
    std::string PageName = m_base->findParentPage()->getNameInDocument();
    if (m_section != nullptr) {
        if (m_createMode) {
            std::string SectionName = m_section->getNameInDocument();
            Gui::Command::doCommand(Gui::Command::Gui,
                                    "App.activeDocument().%s.removeView(App.activeDocument().%s)",
                                    PageName.c_str(),SectionName.c_str());
            Gui::Command::doCommand(Gui::Command::Gui,
                                    "App.activeDocument().removeObject('%s')",
                                    SectionName.c_str());
        } else {
            Base::Console().Message("TSV::reject() - edit mode\n");
            restoreSectionState();
            //check undo stack?
            m_section->requestPaint();
            m_base->requestPaint();
        }

    }

    Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return false;
}

void TaskSectionView::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgSectionView::TaskDlgSectionView(TechDraw::DrawViewPart* base) :
    TaskDialog()
{
    widget  = new TaskSectionView(base);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-viewsection"),
                                         widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgSectionView::TaskDlgSectionView(TechDraw::DrawViewSection* section) :
    TaskDialog()
{
    widget  = new TaskSectionView(section);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-ViewSection"),
                                         widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}
TaskDlgSectionView::~TaskDlgSectionView()
{
}

void TaskDlgSectionView::update()
{
    //widget->updateTask();
}

void TaskDlgSectionView::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btnOK = box->button(QDialogButtonBox::Ok);
    QPushButton* btnCancel = box->button(QDialogButtonBox::Cancel);
    QPushButton* btnApply = box->button(QDialogButtonBox::Apply);
    widget->saveButtons(btnOK, btnCancel, btnApply);
}


//==== calls from the TaskView ===============================================================
void TaskDlgSectionView::open()
{
}

void TaskDlgSectionView::clicked(int i)
{
//    Q_UNUSED(i);
//    Base::Console().Message("TDSV::clicked(%X)\n",i);
    if (i == QMessageBox::Apply) {
        widget->apply();
    }
}

bool TaskDlgSectionView::accept()
{
    widget->accept();
    return true;
}

//bool TaskDlgSectionView::apply()
//{
//    Base::Console().Message("TDSV::apply()\n");
//    widget->apply();
//    return true;
//}

bool TaskDlgSectionView::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskSectionView.cpp>
