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
    m_saveScale(0.0),
    m_dirName(""),
    m_doc(nullptr),
    m_createMode(true),
    m_saved(false),
    m_abort(false)
{
    //existence of base is guaranteed by CmdTechDrawSectionView (Command.cpp)

    m_sectionName = std::string();
    m_doc         = m_base->getDocument();

    m_saveBaseName = m_base->getNameInDocument();
    m_savePageName = m_base->findParentPage()->getNameInDocument();

    ui->setupUi(this);

    connect(ui->pbUp, SIGNAL(clicked(bool)), this, SLOT(onUpClicked()));
    connect(ui->pbDown, SIGNAL(clicked(bool)), this, SLOT(onDownClicked()));
    connect(ui->pbRight, SIGNAL(clicked(bool)), this, SLOT(onRightClicked()));
    connect(ui->pbLeft, SIGNAL(clicked(bool)), this, SLOT(onLeftClicked()));

    setUiPrimary();
}


//ctor for edit
TaskSectionView::TaskSectionView(TechDraw::DrawViewSection* section) :
    ui(new Ui_TaskSectionView),
    m_base(nullptr),
    m_section(section),
    m_saveScale(0.0),
    m_doc(nullptr),
    m_createMode(false),
    m_saved(false),
    m_abort(false)
{
    //existence of section is guaranteed by ViewProviderViewSection.setEdit

    m_doc = m_section->getDocument();
    m_sectionName = m_section->getNameInDocument();
    App::DocumentObject* newObj = m_section->BaseView.getValue();
    m_base = dynamic_cast<TechDraw::DrawViewPart*>(newObj);
    if ( (newObj == nullptr) ||
         (m_base == nullptr) ) {
        throw Base::RuntimeError("TaskSectionView - BaseView not found");
    }

    m_saveBaseName = m_base->getNameInDocument();
    m_savePageName = m_base->findParentPage()->getNameInDocument();

    ui->setupUi(this);

    connect(ui->pbUp, SIGNAL(clicked(bool)), this, SLOT(onUpClicked()));
    connect(ui->pbDown, SIGNAL(clicked(bool)), this, SLOT(onDownClicked()));
    connect(ui->pbRight, SIGNAL(clicked(bool)), this, SLOT(onRightClicked()));
    connect(ui->pbLeft, SIGNAL(clicked(bool)), this, SLOT(onLeftClicked()));

    m_dirName = m_section->SectionDirection.getValueAsString();
    saveSectionState();
    setUiEdit();
}

TaskSectionView::~TaskSectionView()
{
}

void TaskSectionView::setUiPrimary()
{
//    Base::Console().Message("TSV::setUiPrimary()\n");
    setWindowTitle(QObject::tr("Create Section View"));
    std::string temp = m_base->getNameInDocument();
    QString qTemp    = Base::Tools::fromStdString(temp);
    ui->leBaseView->setText(qTemp);

    //TODO: get next symbol from page
//    ui->leSymbol->setText();

    ui->sbScale->setValue(m_base->getScale());
    Base::Vector3d origin = m_base->getOriginalCentroid();
    ui->sbOrgX->setUnit(Base::Unit::Length);
    ui->sbOrgX->setValue(origin.x);
    ui->sbOrgY->setUnit(Base::Unit::Length);
    ui->sbOrgY->setValue(origin.y);
    ui->sbOrgZ->setUnit(Base::Unit::Length);
    ui->sbOrgZ->setValue(origin.z);

    // before the user did not select an orientation,
    // the section properties cannot be changed
    this->setToolTip(QObject::tr("Select at first an orientation"));
    enableAll(false);

    connect(ui->leSymbol, SIGNAL(editingFinished()), this, SLOT(onIdentifierChanged()));

    // the UI file uses keyboardTracking = false so that a recomputation
    // will only be triggered when the arrow keys of the spinboxes are used
    connect(ui->sbScale, SIGNAL(valueChanged(double)), this, SLOT(onScaleChanged()));
    connect(ui->sbOrgX, SIGNAL(valueChanged(double)), this, SLOT(onXChanged()));
    connect(ui->sbOrgY, SIGNAL(valueChanged(double)), this, SLOT(onYChanged()));
    connect(ui->sbOrgZ, SIGNAL(valueChanged(double)), this, SLOT(onZChanged()));
}

void TaskSectionView::setUiEdit()
{
//    Base::Console().Message("TSV::setUiEdit()\n");
    setWindowTitle(QObject::tr("Edit Section View"));

    std::string temp = m_base->getNameInDocument();
    QString qTemp    = Base::Tools::fromStdString(temp);
    ui->leBaseView->setText(qTemp);

    temp = m_section->SectionSymbol.getValue();
    qTemp = Base::Tools::fromStdString(temp);
    ui->leSymbol->setText(qTemp);
    ui->sbScale->setValue(m_section->getScale());
    
    Base::Vector3d origin = m_section->SectionOrigin.getValue();
    ui->sbOrgX->setUnit(Base::Unit::Length);
    ui->sbOrgX->setValue(origin.x);
    ui->sbOrgY->setUnit(Base::Unit::Length);
    ui->sbOrgY->setValue(origin.y);
    ui->sbOrgZ->setUnit(Base::Unit::Length);
    ui->sbOrgZ->setValue(origin.z);

    connect(ui->leSymbol, SIGNAL(editingFinished()), this, SLOT(onIdentifierChanged()));

    // the UI file uses keyboardTracking = false so that a recomputation
    // will only be triggered when the arrow keys of the spinboxes are used
    connect(ui->sbScale, SIGNAL(valueChanged(double)), this, SLOT(onScaleChanged()));
    connect(ui->sbOrgX, SIGNAL(valueChanged(double)), this, SLOT(onXChanged()));
    connect(ui->sbOrgY, SIGNAL(valueChanged(double)), this, SLOT(onYChanged()));
    connect(ui->sbOrgZ, SIGNAL(valueChanged(double)), this, SLOT(onZChanged()));
}

//save the start conditions
void TaskSectionView::saveSectionState()
{
//    Base::Console().Message("TSV::saveSectionState()\n");
    if (m_section != nullptr) {
        m_saveSymbol = m_section->SectionSymbol.getValue();
        m_saveScale  = m_section->getScale();
        m_saveNormal = m_section->SectionNormal.getValue();
        m_saveDirection = m_section->Direction.getValue();
        m_saveOrigin    = m_section->SectionOrigin.getValue();
        m_saveDirName   = m_section->SectionDirection.getValueAsString();
        m_saved = true;
    }
}

//restore the start conditions
void TaskSectionView::restoreSectionState()
{
//    Base::Console().Message("TSV::restoreSectionState()\n");
    if (m_section != nullptr) {
        m_section->SectionSymbol.setValue(m_saveSymbol);
        m_section->Scale.setValue(m_saveScale);
        m_section->SectionNormal.setValue(m_saveNormal);
        m_section->Direction.setValue(m_saveDirection);
        m_section->SectionOrigin.setValue(m_saveOrigin);
        m_section->SectionDirection.setValue(m_saveDirName.c_str());
    }
}

void TaskSectionView::onUpClicked()
{
//    Base::Console().Message("TSV::onUpClicked()\n");
    checkAll(false);
    ui->pbUp->setChecked(true);
    applyQuick("Up");
}

void TaskSectionView::onDownClicked()
{
//    Base::Console().Message("TSV::onDownClicked()\n");
    checkAll(false);
    ui->pbDown->setChecked(true);
    applyQuick("Down");
}

void TaskSectionView::onLeftClicked()
{
//    Base::Console().Message("TSV::onLeftClicked()\n");
    checkAll(false);
    ui->pbLeft->setChecked(true);
    applyQuick("Left");
}

void TaskSectionView::onRightClicked()
{
//    Base::Console().Message("TSV::onRightClicked()\n");
    checkAll(false);
    ui->pbRight->setChecked(true);
    applyQuick("Right");
}

void TaskSectionView::onIdentifierChanged()
{
    checkAll(false);
    apply();
}

void TaskSectionView::onScaleChanged()
{
    checkAll(false);
    apply();
}

void TaskSectionView::onXChanged()
{
    checkAll(false);
    apply();
}

void TaskSectionView::onYChanged()
{
    checkAll(false);
    apply();
}

void TaskSectionView::onZChanged()
{
    checkAll(false);
    apply();
}

void TaskSectionView::checkAll(bool b)
{
    ui->pbUp->setChecked(b);
    ui->pbDown->setChecked(b);
    ui->pbRight->setChecked(b);
    ui->pbLeft->setChecked(b);
}

void TaskSectionView::enableAll(bool b)
{
    ui->leSymbol->setEnabled(b);
    ui->sbScale->setEnabled(b);
    ui->sbOrgX->setEnabled(b);
    ui->sbOrgY->setEnabled(b);
    ui->sbOrgZ->setEnabled(b);
}

//******************************************************************************
bool TaskSectionView::apply(void)
{
//    Base::Console().Message("TSV::apply() - m_dirName: %s\n", m_dirName.c_str());
    if (m_dirName.empty()) {
        std::string msg = 
            Base::Tools::toStdString(tr("Nothing to apply. No section direction picked yet"));
        Base::Console().Error((msg + "\n").c_str());
        return false;
    }
    if (m_section == nullptr) {          //didn't create the feature yet
        return false;
    }

    checkAll(false);
    applyQuick(m_dirName);
    return true;
}

void TaskSectionView::applyQuick(std::string dir)
{
//    Base::Console().Message("TSV::applyQuick(%s)\n", dir.c_str());
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Apply Quick"));
    m_dirName = dir;
    if (m_section == nullptr) {
        createSectionView();
    }
    if (isSectionValid()) {
        updateSectionView();
        m_section->recomputeFeature();
        this->setToolTip(QObject::tr("Select at first an orientation"));
        // we can in any case enable all objects in the dialog
        // and remove the dialog-wide tooltip if there was one
        enableAll(true);
        this->setToolTip(QString());
    } else {
        failNoObject(m_sectionName);
    }

    if (isBaseValid()) {
        m_base->requestPaint();
        return;
    }
}

void TaskSectionView::applyAligned(void) 
{
    Base::Console().Message("TSV::applyAligned() - not implemented yet\n");
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Apply Aligned"));
    m_dirName = "Aligned";
    //fiddle with directions here

    m_section->requestPaint();
    m_base->requestPaint();
}

//*********************************************************************

//pointer to created view is not returned, but stored in m_section
void TaskSectionView::createSectionView(void)
{
//    Base::Console().Message("TSV::createSectionView()\n");
    if (!isBaseValid()) {
        failNoObject(m_baseName);
        return;
    }

    std::string sectionName;
    std::string baseName = m_base->getNameInDocument();
    double baseScale = m_base->getScale();

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create SectionView"));
    if (m_section == nullptr) {
        m_sectionName = m_base->getDocument()->getUniqueObjectName("SectionView");
        std::string sectionType = "TechDraw::DrawViewSection";

        Command::doCommand(Command::Doc,"App.activeDocument().addObject('%s','%s')",
                           sectionType.c_str(),m_sectionName.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",
                           m_savePageName.c_str(), m_sectionName.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.BaseView = App.activeDocument().%s",
                           m_sectionName.c_str(),baseName.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.Source = App.activeDocument().%s.Source",
                           m_sectionName.c_str(),baseName.c_str());
        Command::doCommand(Command::Doc,
                           "App.activeDocument().%s.SectionOrigin = FreeCAD.Vector(%.3f,%.3f,%.3f)",
                           m_sectionName.c_str(), 
                           ui->sbOrgX->value().getValue(),
                           ui->sbOrgY->value().getValue(),
                           ui->sbOrgZ->value().getValue());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.Scale = %0.6f",
                           m_sectionName.c_str(), baseScale);

        App::DocumentObject* newObj = m_base->getDocument()->getObject(m_sectionName.c_str());
        m_section = dynamic_cast<TechDraw::DrawViewSection*>(newObj);
        if ( (newObj == nullptr) ||
             (m_section == nullptr) ) {
            throw Base::RuntimeError("TaskSectionView - new section object not found");
         }
    }
    Gui::Command::updateActive();
    Gui::Command::commitCommand();

    return;
}

void TaskSectionView::updateSectionView(void)
{
//    Base::Console().Message("TSV::updateSectionView() - m_sectionName: %s\n", m_sectionName.c_str());
    if (!isSectionValid()) {
        failNoObject(m_sectionName);
        return;
    }

    if (m_section != nullptr) {
        Command::doCommand(Command::Doc,"App.activeDocument().%s.SectionDirection = '%s'",
                           m_sectionName.c_str(),m_dirName.c_str());
        Command::doCommand(Command::Doc,
                           "App.activeDocument().%s.SectionOrigin = FreeCAD.Vector(%.3f,%.3f,%.3f)",
                           m_sectionName.c_str(), 
                           ui->sbOrgX->value().getValue(),
                           ui->sbOrgY->value().getValue(),
                           ui->sbOrgZ->value().getValue());
        QString qTemp    = ui->leSymbol->text();
        std::string temp = Base::Tools::toStdString(qTemp);
        Command::doCommand(Command::Doc,"App.activeDocument().%s.SectionSymbol = '%s'",
                           m_sectionName.c_str(),
                           temp.c_str());
        std::string lblText = "Section " +
                              temp + 
                              " - " +
                              temp;
        Command::doCommand(Command::Doc,"App.activeDocument().%s.Label = '%s'",
                           m_sectionName.c_str(),
                           lblText.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.Scale = %0.6f",
                           m_sectionName.c_str(),
                           ui->sbScale->value().getValue());
        m_section->setCSFromBase(m_dirName.c_str());
    }
}

void TaskSectionView::failNoObject(std::string objectName) 
{
    QString qObjectName = Base::Tools::fromStdString(objectName);
    QString msg = tr("Can not continue. Object * %1 * not found.").arg(qObjectName);
    QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Operation Failed"), msg);
    Gui::Control().closeDialog();
    m_abort = true;
}

bool TaskSectionView::isBaseValid(void)
{
    bool result = true;
    if (m_base == nullptr) {
        result = false;
    } else {
        App::DocumentObject* baseObj = m_doc->getObject(m_saveBaseName.c_str());
        if (baseObj == nullptr) {
            result = false;
        }
    }
    return result;
}

bool TaskSectionView::isSectionValid(void)
{
    bool result = true;
    if (m_section == nullptr) {
        result = false;
    } else {
        App::DocumentObject* sectionObj = m_doc->getObject(m_sectionName.c_str());
        if (sectionObj == nullptr) {
            result = false;
        }
    }
    return result;
}

//******************************************************************************

bool TaskSectionView::accept()
{
//    Base::Console().Message("TSV::accept()\n");
    if (m_abort) {
        return true;
    }
    apply();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
    return true;
}

bool TaskSectionView::reject()
{
//    Base::Console().Message("TSV::reject()\n");
    if (m_section == nullptr) {                 //no section created, nothing to undo
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
        return false;
    }

    if (!isSectionValid()) {                    //section !exist. nothing to undo 
        if (isBaseValid()) {
            m_base->requestPaint();
        }
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
        return false;
    }

    if (m_createMode) {
        std::string SectionName = m_section->getNameInDocument();
        Gui::Command::doCommand(Gui::Command::Gui,
                                "App.activeDocument().%s.removeView(App.activeDocument().%s)",
                                m_savePageName.c_str(),SectionName.c_str());
        Gui::Command::doCommand(Gui::Command::Gui,
                                "App.activeDocument().removeObject('%s')",
                                SectionName.c_str());
    } else {
        restoreSectionState();
        m_section->recomputeFeature();
        m_section->requestPaint();
    }

    if (isBaseValid()) {
        m_base->requestPaint();
    }

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
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_SectionView"),
                                         widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgSectionView::TaskDlgSectionView(TechDraw::DrawViewSection* section) :
    TaskDialog()
{
    widget  = new TaskSectionView(section);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_SectionView"),
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

//==== calls from the TaskView ===============================================================
void TaskDlgSectionView::open()
{
}

bool TaskDlgSectionView::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgSectionView::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskSectionView.cpp>
