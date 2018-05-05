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

#include <Base/Console.h>
#include <Base/Vector3D.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Mod/Part/App/PartFeature.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "TaskSectionView.h"
#include <Mod/TechDraw/Gui/ui_TaskSectionView.h>

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

void _printVect(char* label, Base::Vector3d v);

TaskSectionView::TaskSectionView(TechDraw::DrawViewPart* base, TechDraw::DrawViewSection* section) :
    ui(new Ui_TaskSectionView),
    m_base(base),
    m_section(section)
{
    ui->setupUi(this);

    connect(ui->pb_Up, SIGNAL(clicked(bool)),
            this, SLOT(onUpClicked(bool)));
    connect(ui->pb_Down, SIGNAL(clicked(bool)),
            this, SLOT(onDownClicked(bool)));
    connect(ui->pb_Right, SIGNAL(clicked(bool)),
            this, SLOT(onRightClicked(bool)));
    connect(ui->pb_Left, SIGNAL(clicked(bool)),
            this, SLOT(onLeftClicked(bool)));
    connect(ui->pbReset, SIGNAL(clicked(bool)),
            this, SLOT(onResetClicked(bool)));

    sectionDir = "unset";
    saveInitialValues();
    resetValues();
}

TaskSectionView::~TaskSectionView()
{
    delete ui;
}


void TaskSectionView::saveInitialValues()
{
    saveSym              = m_section->SectionSymbol.getValue();
    saveSectionOrigin    = m_base->getCentroid();
    saveSectionProjDir   = m_section->Direction.getValue();
    saveSectionNormal    = m_section->SectionNormal.getValue();
    saveLabel            = m_section->Label.getValue();
}

//set the screen back to original values
void TaskSectionView::resetValues()
{
    ui->leSymbol->setText(QString::fromUtf8(saveSym.data(), saveSym.size()));

    checkAll(false);
    enableAll(true);

    sectionDir = "unset";
    sectionProjDir = saveSectionProjDir;
    sectionNormal = saveSectionNormal;

    ui->sb_OrgX->setValue(saveSectionOrigin.x);
    ui->sb_OrgY->setValue(saveSectionOrigin.y);
    ui->sb_OrgZ->setValue(saveSectionOrigin.z);

    ui->leProjDir->setReadOnly(true);
    ui->leProjDir->setText(formatVector(saveSectionProjDir));
    ui->leNormal->setReadOnly(true);
    ui->leNormal->setText(formatVector(saveSectionNormal));

    m_section->Label.setValue(saveLabel.c_str());
    Base::Console().Message("");
}

//calculate good starting points from base view and push buttons
bool TaskSectionView::calcValues()
{
    bool result = true;

    if (ui->pb_Up->isChecked()) {
        sectionDir = "Up";
        sectionProjDir = m_section->getSectionVector(sectionDir);
    } else if (ui->pb_Down->isChecked()) {
        sectionDir = "Down";
        sectionProjDir = m_section->getSectionVector(sectionDir);
    } else if (ui->pb_Left->isChecked()) {
        sectionDir = "Left";
        sectionProjDir = m_section->getSectionVector(sectionDir);
    } else if (ui->pb_Right->isChecked()) {
        sectionDir = "Right";
        sectionProjDir = m_section->getSectionVector(sectionDir);
    } else {
        Base::Console().Message("Select a direction\n");
        result = false;
    }

    sectionNormal = sectionProjDir;
    if (result) {
        ui->leNormal->setText(formatVector(sectionNormal));
        ui->leProjDir->setText(formatVector(sectionProjDir));

        Base::Console().Message("Press Reset, OK or Cancel to continue \n");
    }
    return result;
}

//move values from screen to DocObjs
void TaskSectionView::updateValues()
{
    if (strcmp(sectionDir,"unset") != 0) {
        m_section->SectionDirection.setValue(sectionDir);
    }
    m_section->Direction.setValue(sectionProjDir);
    m_section->SectionNormal.setValue(sectionNormal);
    Base::Vector3d origin(ui->sb_OrgX->value().getValue(),
                          ui->sb_OrgY->value().getValue(),
                          ui->sb_OrgZ->value().getValue());
    m_section->SectionOrigin.setValue(origin);
    m_section->SectionSymbol.setValue(ui->leSymbol->text().toUtf8().constData());

    m_base->getDocument()->recompute();
}

void TaskSectionView::blockButtons(bool b)
{
    ui->pb_Up->blockSignals(b);
    ui->pb_Down->blockSignals(b);
    ui->pb_Left->blockSignals(b);
    ui->pb_Right->blockSignals(b);
}

void TaskSectionView::turnOnUp()
{
    blockButtons(true);
    checkAll(false);
    enableAll(false);
    ui->pb_Up->setChecked(true);
    ui->pb_Up->setEnabled(true);
    blockButtons(false);
    if (calcValues()) {
        updateValues();
    }
}

void TaskSectionView::turnOnDown()
{
    blockButtons(true);
    checkAll(false);
    enableAll(false);
    ui->pb_Down->setChecked(true);
    ui->pb_Down->setEnabled(true);
    blockButtons(false);
    if (calcValues()) {
        updateValues();
    }
}

void TaskSectionView::turnOnLeft()
{
    blockButtons(true);
    checkAll(false);
    enableAll(false);
    ui->pb_Left->setChecked(true);
    ui->pb_Left->setEnabled(true);
    blockButtons(false);
    if (calcValues()) {
        updateValues();
    }
}

void TaskSectionView::turnOnRight()
{
    blockButtons(true);
    checkAll(false);
    enableAll(false);
    ui->pb_Right->setChecked(true);
    ui->pb_Right->setEnabled(true);
    blockButtons(false);
    if (calcValues()) {
        updateValues();
    }
}

void TaskSectionView::checkAll(bool b)
{
    blockButtons(true);
    ui->pb_Up->setChecked(b);
    ui->pb_Down->setChecked(b);
    ui->pb_Left->setChecked(b);
    ui->pb_Right->setChecked(b);
    blockButtons(false);
}

void TaskSectionView::enableAll(bool b)
{
    blockButtons(true);
    ui->pb_Up->setEnabled(b);
    ui->pb_Down->setEnabled(b);
    ui->pb_Left->setEnabled(b);
    ui->pb_Right->setEnabled(b);
    blockButtons(false);
}


void TaskSectionView::onUpClicked(bool b)
{
    Q_UNUSED(b);
    turnOnUp();
}

void TaskSectionView::onDownClicked(bool b)
{
    Q_UNUSED(b);
    turnOnDown();
}

void TaskSectionView::onLeftClicked(bool b)
{
    Q_UNUSED(b);
    turnOnLeft();
}

void TaskSectionView::onRightClicked(bool b)
{
    Q_UNUSED(b);
    turnOnRight();
}

void TaskSectionView::onResetClicked(bool b)
{
    Q_UNUSED(b);
    resetValues();
    updateValues();
    m_section->Label.setValue(saveLabel.c_str());
}


bool TaskSectionView::accept()
{
    if (strcmp(sectionDir,"unset") == 0) {
        Base::Console().Message("No direction selected!\n");
        return reject();
    } else {
        updateValues();
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
        return true;
    }
}

bool TaskSectionView::reject()
{
    std::string BaseName = m_base->getNameInDocument();
    std::string PageName = m_base->findParentPage()->getNameInDocument();
    std::string SectionName = m_section->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().%s.removeView(App.activeDocument().%s)",
                            PageName.c_str(),SectionName.c_str());
    Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().removeObject('%s')",SectionName.c_str());
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
    m_base->findParentPage()->requestPaint();
    return false;
}

void TaskSectionView::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

QString TaskSectionView::formatVector(Base::Vector3d v)
{
    QString data = QString::fromLatin1("[%1 %2 %3]")
        .arg(QLocale::system().toString(v.x, 'f', 2))
        .arg(QLocale::system().toString(v.y, 'f', 2))
        .arg(QLocale::system().toString(v.z, 'f', 2));
    return data;
}

void _printVect(char* label, Base::Vector3d v)
{
    Base::Console().Message("printVect: %s (%3f,%.3f,%.3f)\n",label,v.x,v.y,v.z);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgSectionView::TaskDlgSectionView(TechDraw::DrawViewPart* base, TechDraw::DrawViewSection* section) :
    TaskDialog()
{
    widget  = new TaskSectionView(base,section);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("TechDraw_Tree_View"),
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

void TaskDlgSectionView::clicked(int i)
{
    Q_UNUSED(i);
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
