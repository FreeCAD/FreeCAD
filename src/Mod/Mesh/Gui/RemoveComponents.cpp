/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <QPushButton>
#endif

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>

#include "RemoveComponents.h"
#include "ui_RemoveComponents.h"


using namespace MeshGui;

RemoveComponents::RemoveComponents(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
    , ui(new Ui_RemoveComponents)
{
    ui->setupUi(this);
    setupConnections();
    ui->spSelectComp->setRange(1, INT_MAX);
    ui->spSelectComp->setValue(10);
    ui->spDeselectComp->setRange(1, INT_MAX);
    ui->spDeselectComp->setValue(10);

    Gui::Selection().clearSelection();
    meshSel.setCheckOnlyVisibleTriangles(ui->visibleTriangles->isChecked());
    meshSel.setCheckOnlyPointToUserTriangles(ui->screenTriangles->isChecked());
    meshSel.setEnabledViewerSelection(false);
}

RemoveComponents::~RemoveComponents()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void RemoveComponents::setupConnections()
{
    // clang-format off
    connect(ui->selectRegion, &QPushButton::clicked,
            this, &RemoveComponents::onSelectRegionClicked);
    connect(ui->selectAll, &QPushButton::clicked,
            this, &RemoveComponents::onSelectAllClicked);
    connect(ui->selectComponents, &QPushButton::clicked,
            this, &RemoveComponents::onSelectComponentsClicked);
    connect(ui->selectTriangle, &QPushButton::clicked,
            this, &RemoveComponents::onSelectTriangleClicked);
    connect(ui->deselectRegion, &QPushButton::clicked,
            this, &RemoveComponents::onDeselectRegionClicked);
    connect(ui->deselectAll, &QPushButton::clicked,
            this, &RemoveComponents::onDeselectAllClicked);
    connect(ui->deselectComponents, &QPushButton::clicked,
            this, &RemoveComponents::onDeselectComponentsClicked);
    connect(ui->deselectTriangle, &QPushButton::clicked,
            this, &RemoveComponents::onDeselectTriangleClicked);
    connect(ui->visibleTriangles, &QCheckBox::toggled,
            this, &RemoveComponents::onVisibleTrianglesToggled);
    connect(ui->screenTriangles, &QCheckBox::toggled,
            this, &RemoveComponents::onScreenTrianglesToggled);
    connect(ui->cbSelectComp, &QCheckBox::toggled,
            this, &RemoveComponents::onSelectCompToggled);
    connect(ui->cbDeselectComp, &QCheckBox::toggled,
            this, &RemoveComponents::onDeselectCompToggled);
    // clang-format on
}

void RemoveComponents::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

void RemoveComponents::onSelectRegionClicked()
{
    meshSel.startSelection();
}

void RemoveComponents::onDeselectRegionClicked()
{
    meshSel.startDeselection();
}

void RemoveComponents::onSelectAllClicked()
{
    // select the complete meshes
    meshSel.fullSelection();
}

void RemoveComponents::onDeselectAllClicked()
{
    // deselect all meshes
    meshSel.clearSelection();
}

void RemoveComponents::onSelectComponentsClicked()
{
    // select components up to a certain size
    int size = ui->spSelectComp->value();
    meshSel.selectComponent(size);
}

void RemoveComponents::onDeselectComponentsClicked()
{
    // deselect components from a certain size on
    int size = ui->spDeselectComp->value();
    meshSel.deselectComponent(size);
}

void RemoveComponents::onVisibleTrianglesToggled(bool on)
{
    meshSel.setCheckOnlyVisibleTriangles(on);
}

void RemoveComponents::onScreenTrianglesToggled(bool on)
{
    meshSel.setCheckOnlyPointToUserTriangles(on);
}

void RemoveComponents::onSelectCompToggled(bool on)
{
    meshSel.setAddComponentOnClick(on);
}

void RemoveComponents::onDeselectCompToggled(bool on)
{
    meshSel.setRemoveComponentOnClick(on);
}

void RemoveComponents::deleteSelection()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (!doc) {
        return;
    }
    // delete all selected faces
    doc->openCommand(QT_TRANSLATE_NOOP("Command", "Delete selection"));
    bool ok = meshSel.deleteSelection();
    if (!ok) {
        doc->abortCommand();
    }
    else {
        doc->commitCommand();
    }
}

void RemoveComponents::invertSelection()
{
    meshSel.invertSelection();
}

void RemoveComponents::onSelectTriangleClicked()
{
    meshSel.selectTriangle();
    meshSel.setAddComponentOnClick(ui->cbSelectComp->isChecked());
}

void RemoveComponents::onDeselectTriangleClicked()
{
    meshSel.deselectTriangle();
    meshSel.setRemoveComponentOnClick(ui->cbDeselectComp->isChecked());
}

void RemoveComponents::reject()
{
    // deselect all meshes
    meshSel.clearSelection();
    meshSel.setEnabledViewerSelection(true);
}

// -------------------------------------------------

RemoveComponentsDialog::RemoveComponentsDialog(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    widget = new RemoveComponents(this);  // NOLINT
    this->setWindowTitle(widget->windowTitle());

    QVBoxLayout* hboxLayout = new QVBoxLayout(this);
    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Close | QDialogButtonBox::Ok);
    QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setText(MeshGui::TaskRemoveComponents::tr("Delete"));
    buttonBox->addButton(MeshGui::TaskRemoveComponents::tr("Invert"), QDialogButtonBox::ActionRole);

    connect(buttonBox, &QDialogButtonBox::clicked, this, &RemoveComponentsDialog::clicked);

    hboxLayout->addWidget(widget);
    hboxLayout->addWidget(buttonBox);
}

RemoveComponentsDialog::~RemoveComponentsDialog() = default;

void RemoveComponentsDialog::reject()
{
    widget->reject();
    QDialog::reject();
}

void RemoveComponentsDialog::clicked(QAbstractButton* btn)
{
    QDialogButtonBox* buttonBox = qobject_cast<QDialogButtonBox*>(sender());
    QDialogButtonBox::StandardButton id = buttonBox->standardButton(btn);
    if (id == QDialogButtonBox::Ok) {
        widget->deleteSelection();
    }
    else if (id == QDialogButtonBox::Close) {
        this->reject();
    }
    else if (id == QDialogButtonBox::NoButton) {
        widget->invertSelection();
    }
}

// ---------------------------------------

/* TRANSLATOR MeshGui::TaskRemoveComponents */

TaskRemoveComponents::TaskRemoveComponents()
{
    widget = new RemoveComponents();  // NOLINT
    taskbox = new Gui::TaskView::TaskBox(QPixmap(), widget->windowTitle(), false, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

void TaskRemoveComponents::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btn = box->button(QDialogButtonBox::Ok);
    btn->setText(tr("Delete"));
    box->addButton(tr("Invert"), QDialogButtonBox::ActionRole);
}

bool TaskRemoveComponents::accept()
{
    return false;
}

void TaskRemoveComponents::clicked(int id)
{
    if (id == QDialogButtonBox::Ok) {
        widget->deleteSelection();
    }
    else if (id == QDialogButtonBox::Close) {
        widget->reject();
    }
    else if (id == QDialogButtonBox::NoButton) {
        widget->invertSelection();
    }
}

#include "moc_RemoveComponents.cpp"
