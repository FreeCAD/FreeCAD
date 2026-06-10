/***************************************************************************
 *   Copyright (c) 2026 FreeCAD contributors                               *
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
#include <QEvent>
#include <QFormLayout>
#include <QLineEdit>

#include <Base/Tools.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Mod/TechDraw/App/DrawAuxiliaryView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>

#include "TaskAuxiliaryView.h"

using namespace TechDrawGui;

TaskAuxiliaryView::TaskAuxiliaryView(TechDraw::DrawAuxiliaryView* auxiliaryView)
    : m_auxiliaryView(auxiliaryView)
    , m_baseViewEdit(new QLineEdit(this))
    , m_referenceLabelEdit(new QLineEdit(this))
    , m_projectionModeCombo(new QComboBox(this))
    , m_reverseDirectionCheck(new QCheckBox(this))
    , m_keepAlignedCheck(new QCheckBox(this))
{
    initUi();
}

void TaskAuxiliaryView::initUi()
{
    auto* form = new QFormLayout(this);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_baseViewEdit->setReadOnly(true);
    if (auto baseView = getBaseView()) {
        m_baseViewEdit->setText(QString::fromUtf8(baseView->getNameInDocument()));
    }

    m_projectionModeCombo->addItem(tr("Across"), QStringLiteral("Across"));
    m_projectionModeCombo->addItem(tr("Along"), QStringLiteral("Along"));

    if (m_auxiliaryView) {
        m_referenceLabelEdit->setText(QString::fromUtf8(m_auxiliaryView->ReferenceLabel.getValue()));
        m_projectionModeCombo->setCurrentIndex(m_auxiliaryView->ProjectionMode.isValue("Along") ? 1 : 0);
        m_reverseDirectionCheck->setChecked(m_auxiliaryView->ReverseDirection.getValue());
        m_keepAlignedCheck->setChecked(m_auxiliaryView->KeepAligned.getValue());
    }

    form->addRow(tr("Base view"), m_baseViewEdit);
    form->addRow(tr("Reference label"), m_referenceLabelEdit);
    form->addRow(tr("Projection mode"), m_projectionModeCombo);
    form->addRow(tr("Reverse direction"), m_reverseDirectionCheck);
    form->addRow(tr("Keep aligned"), m_keepAlignedCheck);

    setLayout(form);
    setWindowTitle(tr("Auxiliary View"));
}

bool TaskAuxiliaryView::accept()
{
    if (!m_auxiliaryView) {
        return false;
    }

    const std::string auxiliaryName = m_auxiliaryView->getNameInDocument();
    const std::string label =
        Base::Tools::escapeEncodeString(m_referenceLabelEdit->text().toStdString());
    const std::string mode = m_projectionModeCombo->currentData().toString().toStdString();
    const char* reverse = m_reverseDirectionCheck->isChecked() ? "True" : "False";
    const char* keepAligned = m_keepAlignedCheck->isChecked() ? "True" : "False";

    int transaction =
        Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Edit Auxiliary View"));
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.ReferenceLabel = '%s'",
                            auxiliaryName.c_str(),
                            label.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.ProjectionMode = '%s'",
                            auxiliaryName.c_str(),
                            mode.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.ReverseDirection = %s",
                            auxiliaryName.c_str(),
                            reverse);
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.KeepAligned = %s",
                            auxiliaryName.c_str(),
                            keepAligned);
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.recompute()",
                            auxiliaryName.c_str());
    Gui::Command::updateActive();
    Gui::Command::commitCommand(transaction);

    if (auto baseView = getBaseView()) {
        baseView->requestPaint();
    }
    m_auxiliaryView->requestPaint();

    return true;
}

bool TaskAuxiliaryView::reject()
{
    return true;
}

std::string TaskAuxiliaryView::getAuxiliaryName() const
{
    if (!m_auxiliaryView) {
        return {};
    }

    return m_auxiliaryView->getNameInDocument();
}

void TaskAuxiliaryView::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        setWindowTitle(tr("Auxiliary View"));
    }
    QWidget::changeEvent(event);
}

TechDraw::DrawViewPart* TaskAuxiliaryView::getBaseView() const
{
    if (!m_auxiliaryView) {
        return nullptr;
    }

    return m_auxiliaryView->getBaseDVP();
}

TaskDlgAuxiliaryView::TaskDlgAuxiliaryView(TechDraw::DrawAuxiliaryView* auxiliaryView)
    : TaskDialog()
    , widget(new TaskAuxiliaryView(auxiliaryView))
    , taskbox(new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_View"),
                                         widget->windowTitle(),
                                         true,
                                         nullptr))
{
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

void TaskDlgAuxiliaryView::open()
{}

void TaskDlgAuxiliaryView::clicked(int)
{}

bool TaskDlgAuxiliaryView::accept()
{
    return widget->accept();
}

bool TaskDlgAuxiliaryView::reject()
{
    return widget->reject();
}

std::string TaskDlgAuxiliaryView::getAuxiliaryName() const
{
    return widget->getAuxiliaryName();
}

#include <Mod/TechDraw/Gui/moc_TaskAuxiliaryView.cpp>
