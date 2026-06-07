// SPDX-License-Identifier: LGPL-2.0-or-later

/***************************************************************************
 *   Copyright (c) 2026 meaqua9420                                        *
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

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>

#include <App/Document.h>
#include <Base/Console.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>

#include <Mod/TechDraw/App/DrawAuxiliaryView.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewPart.h>

#include "TaskAuxiliaryView.h"
#include "ui_TaskAuxiliaryView.h"


using namespace TechDrawGui;

TaskAuxiliaryView::TaskAuxiliaryView(TechDraw::DrawViewPart* baseFeat,
                                     Base::Vector3d referenceStart,
                                     Base::Vector3d referenceEnd)
    : ui(new Ui_TaskAuxiliaryView)
    , m_auxiliaryFeat(nullptr)
    , m_baseFeat(baseFeat)
    , m_basePage(baseFeat ? baseFeat->findParentPage() : nullptr)
    , m_doc(baseFeat ? baseFeat->getDocument() : nullptr)
    , m_referenceStart(referenceStart)
    , m_referenceEnd(referenceEnd)
    , m_saveReverse(false)
    , m_editMode(false)
{
    ui->setupUi(this);

    if (!m_baseFeat || !m_basePage || !m_doc) {
        Base::Console().error("TaskAuxiliaryView - bad base view. Cannot proceed.\n");
        return;
    }

    m_baseName = m_baseFeat->getNameInDocument();
    m_pageName = m_basePage->getNameInDocument();
    ui->leBaseView->setText(QString::fromStdString(m_baseName));
    ui->leAuxiliaryView->setText(QObject::tr("New auxiliary view"));
    ui->cbOrientation->setCurrentIndex(1);
}

TaskAuxiliaryView::TaskAuxiliaryView(TechDraw::DrawAuxiliaryView* auxiliaryFeat)
    : ui(new Ui_TaskAuxiliaryView)
    , m_auxiliaryFeat(auxiliaryFeat)
    , m_baseFeat(auxiliaryFeat ? auxiliaryFeat->getBaseDVP() : nullptr)
    , m_basePage(m_baseFeat ? m_baseFeat->findParentPage() : nullptr)
    , m_doc(auxiliaryFeat ? auxiliaryFeat->getDocument() : nullptr)
    , m_referenceStart(auxiliaryFeat ? auxiliaryFeat->ReferenceStart.getValue()
                                     : Base::Vector3d(0.0, 0.0, 0.0))
    , m_referenceEnd(auxiliaryFeat ? auxiliaryFeat->ReferenceEnd.getValue()
                                   : Base::Vector3d(1.0, 0.0, 0.0))
    , m_saveReverse(false)
    , m_editMode(true)
{
    ui->setupUi(this);

    if (!m_auxiliaryFeat || !m_baseFeat || !m_basePage || !m_doc) {
        Base::Console().error("TaskAuxiliaryView - bad auxiliary view. Cannot proceed.\n");
        return;
    }

    m_auxiliaryName = m_auxiliaryFeat->getNameInDocument();
    m_baseName = m_baseFeat->getNameInDocument();
    m_pageName = m_basePage->getNameInDocument();

    saveAuxiliaryState();
    setUiFromFeat();
}

TaskAuxiliaryView::~TaskAuxiliaryView() = default;

void TaskAuxiliaryView::setUiFromFeat()
{
    ui->leBaseView->setText(QString::fromStdString(m_baseName));

    if (!m_auxiliaryFeat) {
        return;
    }

    ui->leAuxiliaryView->setText(QString::fromStdString(m_auxiliaryName));
    ui->leReferenceLabel->setText(QString::fromUtf8(m_auxiliaryFeat->ReferenceLabel.getValue()));
    std::string orientation = m_auxiliaryFeat->AuxiliaryOrientation.getValueAsString();
    ui->cbOrientation->setCurrentIndex(orientation == "Across" ? 1 : 0);
    ui->cbReverse->setChecked(m_auxiliaryFeat->ReverseDirection.getValue());
}

Base::Vector3d TaskAuxiliaryView::referenceDirection() const
{
    Base::Vector3d direction = m_referenceEnd - m_referenceStart;
    direction.z = 0.0;
    if (direction.Length() < 1.0e-7) {
        direction = Base::Vector3d(1.0, 0.0, 0.0);
    }
    direction.Normalize();
    return direction;
}

void TaskAuxiliaryView::createAuxiliaryView()
{
    if (!m_baseFeat || !m_basePage || !m_doc) {
        return;
    }

    int tid = Gui::Command::openActiveDocumentCommand(
        QT_TRANSLATE_NOOP("Command", "Create Auxiliary View"));

    const std::string objectName{"Auxiliary"};
    m_auxiliaryName = m_doc->getUniqueObjectName(objectName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.activeDocument().addObject('TechDraw::DrawAuxiliaryView', '%s')",
                            m_auxiliaryName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.activeDocument().%s.translateLabel('DrawAuxiliaryView', 'Auxiliary', '%s')",
                            m_auxiliaryName.c_str(),
                            m_auxiliaryName.c_str());

    App::DocumentObject* docObj = m_doc->getObject(m_auxiliaryName.c_str());
    m_auxiliaryFeat = dynamic_cast<TechDraw::DrawAuxiliaryView*>(docObj);
    if (!m_auxiliaryFeat) {
        Base::Console().error("TaskAuxiliaryView - new auxiliary view not found.\n");
        Gui::Command::abortCommand(tid);
        return;
    }

    m_auxiliaryFeat->Source.setValues(m_baseFeat->Source.getValues());
    m_auxiliaryFeat->XSource.setValues(m_baseFeat->XSource.getValues());
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.activeDocument().%s.BaseView = App.activeDocument().%s",
                            m_auxiliaryName.c_str(),
                            m_baseName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.activeDocument().%s.Scale = App.activeDocument().%s.Scale",
                            m_auxiliaryName.c_str(),
                            m_baseName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.activeDocument().%s.ScaleType = 2",
                            m_auxiliaryName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.activeDocument().%s.addView(App.activeDocument().%s)",
                            m_pageName.c_str(),
                            m_auxiliaryName.c_str());

    updateAuxiliaryView();

    Gui::Command::updateActive();
    Gui::Command::commitCommand(tid);

    m_auxiliaryFeat->recomputeFeature();
    m_baseFeat->requestPaint();
}

void TaskAuxiliaryView::updateAuxiliaryView()
{
    auto* auxiliaryFeat = getAuxiliaryFeat();
    if (!auxiliaryFeat) {
        return;
    }

    QString ref = ui->leReferenceLabel->text();
    auxiliaryFeat->AuxiliaryDirection.setValue(referenceDirection());
    auxiliaryFeat->AuxiliaryOrientation.setValue(ui->cbOrientation->currentIndex());
    auxiliaryFeat->ReferenceLabel.setValue(ref.toStdString());
    auxiliaryFeat->ReferenceStart.setValue(m_referenceStart);
    auxiliaryFeat->ReferenceEnd.setValue(m_referenceEnd);
    auxiliaryFeat->ReverseDirection.setValue(ui->cbReverse->isChecked());

    if (m_baseFeat) {
        m_baseFeat->requestPaint();
    }
}

bool TaskAuxiliaryView::accept()
{
    if (!m_editMode) {
        createAuxiliaryView();
        return true;
    }

    int tid = Gui::Command::openActiveDocumentCommand(
        QT_TRANSLATE_NOOP("Command", "Update Auxiliary View"));
    updateAuxiliaryView();
    Gui::Command::updateActive();
    Gui::Command::commitCommand(tid);

    if (m_auxiliaryFeat) {
        m_auxiliaryFeat->recomputeFeature();
    }
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    return true;
}

bool TaskAuxiliaryView::reject()
{
    if (m_editMode) {
        restoreAuxiliaryState();
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    }
    return true;
}

TechDraw::DrawAuxiliaryView* TaskAuxiliaryView::getAuxiliaryFeat()
{
    if (m_auxiliaryFeat) {
        return m_auxiliaryFeat;
    }
    if (!m_doc || m_auxiliaryName.empty()) {
        return nullptr;
    }
    return dynamic_cast<TechDraw::DrawAuxiliaryView*>(m_doc->getObject(m_auxiliaryName.c_str()));
}

void TaskAuxiliaryView::saveAuxiliaryState()
{
    if (!m_auxiliaryFeat) {
        return;
    }

    m_saveReferenceStart = m_auxiliaryFeat->ReferenceStart.getValue();
    m_saveReferenceEnd = m_auxiliaryFeat->ReferenceEnd.getValue();
    m_saveDirection = m_auxiliaryFeat->AuxiliaryDirection.getValue();
    m_saveLabel = m_auxiliaryFeat->ReferenceLabel.getValue();
    m_saveOrientation = m_auxiliaryFeat->AuxiliaryOrientation.getValueAsString();
    m_saveReverse = m_auxiliaryFeat->ReverseDirection.getValue();
}

void TaskAuxiliaryView::restoreAuxiliaryState()
{
    if (!m_auxiliaryFeat) {
        return;
    }

    m_auxiliaryFeat->ReferenceStart.setValue(m_saveReferenceStart);
    m_auxiliaryFeat->ReferenceEnd.setValue(m_saveReferenceEnd);
    m_auxiliaryFeat->AuxiliaryDirection.setValue(m_saveDirection);
    m_auxiliaryFeat->ReferenceLabel.setValue(m_saveLabel);
    m_auxiliaryFeat->AuxiliaryOrientation.setValue(m_saveOrientation.c_str());
    m_auxiliaryFeat->ReverseDirection.setValue(m_saveReverse);
    m_auxiliaryFeat->recomputeFeature();
    if (m_baseFeat) {
        m_baseFeat->requestPaint();
    }
}

void TaskAuxiliaryView::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

TaskDlgAuxiliaryView::TaskDlgAuxiliaryView(TechDraw::DrawViewPart* baseFeat,
                                           Base::Vector3d referenceStart,
                                           Base::Vector3d referenceEnd)
    : TaskDialog()
{
    widget = new TaskAuxiliaryView(baseFeat, referenceStart, referenceEnd);
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("actions/TechDraw_AuxiliaryView"),
        widget->windowTitle(),
        true,
        nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgAuxiliaryView::TaskDlgAuxiliaryView(TechDraw::DrawAuxiliaryView* auxiliaryFeat)
    : TaskDialog()
{
    widget = new TaskAuxiliaryView(auxiliaryFeat);
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("actions/TechDraw_AuxiliaryView"),
        widget->windowTitle(),
        true,
        nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgAuxiliaryView::~TaskDlgAuxiliaryView() = default;

void TaskDlgAuxiliaryView::open()
{}

bool TaskDlgAuxiliaryView::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgAuxiliaryView::reject()
{
    widget->reject();
    return true;
}

void TaskDlgAuxiliaryView::modifyStandardButtons(QDialogButtonBox* box)
{
    Q_UNUSED(box);
}

std::string TaskDlgAuxiliaryView::getAuxiliaryName() const
{
    auto* auxiliaryObj = widget->getAuxiliaryFeat();
    if (!auxiliaryObj) {
        return {"not found"};
    }

    return auxiliaryObj->getNameInDocument();
}

#include <Mod/TechDraw/Gui/moc_TaskAuxiliaryView.cpp>
