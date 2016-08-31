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

#include "TaskSectionView.h"
#include <Mod/TechDraw/Gui/ui_TaskSectionView.h>

using namespace Gui;
using namespace TechDrawGui;


TaskSectionView::TaskSectionView(TechDraw::DrawViewPart* base, TechDraw::DrawViewSection* section) :
    ui(new Ui_TaskSectionView),
    m_base(base),
    m_section(section)
{
    ui->setupUi(this);

    connect(ui->cbHoriz, SIGNAL(clicked(bool)),
            this, SLOT(onHorizontalClicked(bool)));
    connect(ui->cbVert, SIGNAL(clicked(bool)),
            this, SLOT(onVerticalClicked(bool)));
    connect(ui->cbNormal, SIGNAL(clicked(bool)),
            this, SLOT(onNormalClicked(bool)));
    connect(ui->cbReverse, SIGNAL(clicked(bool)),
            this, SLOT(onReverseClicked(bool)));
    connect(ui->pbCalc, SIGNAL(clicked(bool)),
            this, SLOT(onCalcClicked(bool)));
    connect(ui->pbReset, SIGNAL(clicked(bool)),
            this, SLOT(onResetClicked(bool)));

    saveInitialValues();
    resetValues();
}

TaskSectionView::~TaskSectionView()
{
    delete ui;
}


void TaskSectionView::saveInitialValues()
{
    saveSym              = m_base->SymbolSection.getValue();
    saveHorizSectionLine = m_base->HorizSectionLine.getValue();     //true(horiz)/false(vert)
    saveArrowUpSection   = m_base->ArrowUpSection.getValue();       //true(up/right)/false(down/left)
    saveSectionOrigin    = m_section->SectionOrigin.getValue();
    saveSectionXDir      = m_section->XAxisDirection.getValue();
    saveSectionDirection = m_section->Direction.getValue();
    saveSectionNormal    = m_section->SectionNormal.getValue();
    saveLabel            = m_section->Label.getValue();
}

//set the screen back to original values
void TaskSectionView::resetValues()
{
    ui->leSymbol->setText(QString::fromUtf8(saveSym.data(), saveSym.size()));

    ui->cbHoriz->setChecked(false);
    ui->cbVert->setChecked(false);
    ui->cbNormal->setChecked(false);
    ui->cbReverse->setChecked(false);
    if (saveHorizSectionLine && !saveArrowUpSection) {
        ui->cbHoriz->setChecked(true);
        ui->cbNormal->setChecked(true);
    } else if (saveHorizSectionLine && saveArrowUpSection) {
        ui->cbHoriz->setChecked(true);
        ui->cbReverse->setChecked(true);
    } else if (!saveHorizSectionLine && !saveArrowUpSection) {
        ui->cbVert->setChecked(true);
        ui->cbNormal->setChecked(true);
    } else if (!saveHorizSectionLine && saveArrowUpSection) {
        ui->cbVert->setChecked(true);
        ui->cbReverse->setChecked(true);
    } else {
        Base::Console().Error("%s Symbol Line Direction is invalid\n", m_base->getNameInDocument());
    }

    ui->sbOrgX->setValue(saveSectionOrigin.x);
    ui->sbOrgY->setValue(saveSectionOrigin.y);
    ui->sbOrgZ->setValue(saveSectionOrigin.z);

    ui->sbXX->setValue(saveSectionXDir.x);
    ui->sbXY->setValue(saveSectionXDir.y);
    ui->sbXZ->setValue(saveSectionXDir.z);

    ui->leProjDir->setReadOnly(true);
    ui->leProjDir->setText(formatVector(saveSectionDirection));
    ui->leNormal->setReadOnly(true);
    ui->leNormal->setText(formatVector(saveSectionNormal));

    m_section->Label.setValue(saveLabel.c_str());
}

//calculate good starting points from base view and push buttons
void TaskSectionView::calcValues()
{
    arrowDir = Base::Vector3d(0,-1,0);
    //section arrows should point to "remaining body" of part after sectioning.
    //so the arrow direction is (-1) * sectionPlaneNormal/sectionViewDirection
     if (ui->cbHoriz->isChecked()  &&
         ui->cbNormal->isChecked()) {
        arrowDir = -1.0 * m_base->getVDir();
     } else if (ui->cbHoriz->isChecked() &&
                ui->cbReverse->isChecked()) {
        arrowDir = m_base->getVDir();
     } else if (ui->cbVert->isChecked() &&
                ui->cbNormal->isChecked()) {
        arrowDir = -1.0 * m_base->getUDir();
     } else if (ui->cbVert->isChecked() &&
                ui->cbReverse->isChecked() ) {
        arrowDir = m_base->getUDir();
     } else {
         Base::Console().Error("%s Symbol Line Direction is invalid\n", m_base->getNameInDocument());
     }

    sectionNormal = -1.0 * arrowDir;                  //point of observer (ViewDirection) is away from direction of arrows
    ui->leNormal->setText(formatVector(sectionNormal));

    sectionProjDir = sectionNormal;                              //typical use-case is view perp to face
    ui->leProjDir->setText(formatVector(sectionProjDir));

    sectionOrigin = m_base->getCentroid();                       //middle of the object
    ui->sbOrgX->setValue(sectionOrigin.x);
    ui->sbOrgY->setValue(sectionOrigin.y);
    ui->sbOrgZ->setValue(sectionOrigin.z);

    sectionXDir = m_base->Direction.getValue();                 //rotate 90*
    ui->sbXX->setValue(sectionXDir.x);
    ui->sbXY->setValue(sectionXDir.y);
    ui->sbXZ->setValue(sectionXDir.z);
}

//move values from screen to DocObjs
void TaskSectionView::updateValues()
{
    m_section->Direction.setValue(sectionProjDir);
    m_section->SectionNormal.setValue(sectionNormal);

    Base::Vector3d origin(ui->sbOrgX->value().getValue(),
                          ui->sbOrgY->value().getValue(),
                          ui->sbOrgZ->value().getValue());
    m_section->SectionOrigin.setValue(origin);

    Base::Vector3d xDirIn(ui->sbXX->value().getValue(),
                          ui->sbXY->value().getValue(),
                          ui->sbXZ->value().getValue());
    //edit is: can't be zero, can't be same/recip as sectionProjDir.  anything else is ok.
    if ((xDirIn.Length() < FLT_EPSILON) ||
        (xDirIn == sectionProjDir)      ||
        (xDirIn == -1.0 * sectionProjDir)) {
        Base::Console().Message("XAxisDirection Invalid. Will be substituted.\n");
    }
    m_section->XAxisDirection.setValue(xDirIn);

    m_base->SymbolSection.setValue(ui->leSymbol->text().toUtf8().constData());
    m_base->HorizSectionLine.setValue(ui->cbHoriz->isChecked());
    m_base->ArrowUpSection.setValue(!ui->cbNormal->isChecked());

    m_section->SymbolSection.setValue(ui->leSymbol->text().toUtf8().constData());
    m_section->HorizSectionLine.setValue(ui->cbHoriz->isChecked());
    m_section->ArrowUpSection.setValue(!ui->cbNormal->isChecked());

    std::string symbol = m_base->SymbolSection.getValue();
    std::string symbolText = "Section " + symbol + "-" + symbol;
    if (symbolText.compare(m_section->Label.getValue())) {
        m_section->Label.setValue(symbolText.c_str());
    }

    m_base->getDocument()->recompute();
}


void TaskSectionView::onHorizontalClicked(bool b)
{
    ui->cbHoriz->blockSignals(true);
    ui->cbVert->blockSignals(true);
    ui->cbHoriz->setChecked(true);
    ui->cbVert->setChecked(false);
    ui->cbHoriz->blockSignals(false);
    ui->cbVert->blockSignals(false);
}

void TaskSectionView::onVerticalClicked(bool b)
{
    ui->cbHoriz->blockSignals(true);
    ui->cbVert->blockSignals(true);
    ui->cbVert->setChecked(true);
    ui->cbHoriz->setChecked(false);
    ui->cbHoriz->blockSignals(false);
    ui->cbVert->blockSignals(false);
}

void TaskSectionView::onNormalClicked(bool b)
{
    ui->cbNormal->blockSignals(true);
    ui->cbReverse->blockSignals(true);
    ui->cbNormal->setChecked(true);
    ui->cbReverse->setChecked(false);
    ui->cbNormal->blockSignals(false);
    ui->cbReverse->blockSignals(false);
}

void TaskSectionView::onReverseClicked(bool b)
{
    ui->cbReverse->blockSignals(true);
    ui->cbNormal->blockSignals(true);
    ui->cbReverse->setChecked(true);
    ui->cbNormal->setChecked(false);
    ui->cbReverse->blockSignals(false);
    ui->cbNormal->blockSignals(false);
}


void TaskSectionView::onCalcClicked(bool b)
{
    calcValues();
    updateValues();
}

void TaskSectionView::onResetClicked(bool b)
{
    resetValues();
    updateValues();
    m_section->Label.setValue(saveLabel.c_str());
}


bool TaskSectionView::accept()
{
    //calcValues();
    updateValues();
    return true;
}

bool TaskSectionView::reject()
{
    //TODO: remove viewSection
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
