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
# include <QPushButton>
#endif

#include "RemoveComponents.h"
#include "ui_RemoveComponents.h"
#include <Gui/Application.h>
#include <Gui/Document.h>

using namespace MeshGui;


RemoveComponents::RemoveComponents(QWidget* parent, Qt::WFlags fl)
  : QWidget(parent, fl)
{
    ui = new Ui_RemoveComponents;
    ui->setupUi(this);
    ui->spSelectComp->setRange(1, INT_MAX);
    ui->spSelectComp->setValue(10);
    ui->spDeselectComp->setRange(1, INT_MAX);
    ui->spDeselectComp->setValue(10);

    meshSel.setCheckOnlyVisibleTriangles(ui->visibleTriangles->isChecked());
    meshSel.setCheckOnlyPointToUserTriangles(ui->screenTriangles->isChecked());
}

RemoveComponents::~RemoveComponents()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void RemoveComponents::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

void RemoveComponents::on_selectRegion_clicked()
{
    meshSel.startSelection();
}

void RemoveComponents::on_deselectRegion_clicked()
{
    meshSel.startDeselection();
}

void RemoveComponents::on_selectAll_clicked()
{
    // select the complete meshes
    meshSel.fullSelection();
}

void RemoveComponents::on_deselectAll_clicked()
{
    // deselect all meshes
    meshSel.clearSelection();
}

void RemoveComponents::on_selectComponents_clicked()
{
    // select components upto a certain size
    int size = ui->spSelectComp->value();
    meshSel.selectComponent(size);
}

void RemoveComponents::on_deselectComponents_clicked()
{
    // deselect components from a certain size on
    int size = ui->spDeselectComp->value();
    meshSel.deselectComponent(size);
}

void RemoveComponents::on_visibleTriangles_toggled(bool on)
{
    meshSel.setCheckOnlyVisibleTriangles(on);
}

void RemoveComponents::on_screenTriangles_toggled(bool on)
{
    meshSel.setCheckOnlyPointToUserTriangles(on);
}

void RemoveComponents::on_cbSelectComp_toggled(bool on)
{
    meshSel.setAddComponentOnClick(on);
}

void RemoveComponents::on_cbDeselectComp_toggled(bool on)
{
    meshSel.setRemoveComponentOnClick(on);
}

void RemoveComponents::deleteSelection()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (!doc) return;
    // delete all selected faces
    doc->openCommand("Delete selection");
    bool ok = meshSel.deleteSelection();
    if (!ok)
        doc->abortCommand();
    else
        doc->commitCommand();
}

void RemoveComponents::invertSelection()
{
    meshSel.invertSelection();
}

void RemoveComponents::on_selectTriangle_clicked()
{
    meshSel.selectTriangle();
    meshSel.setAddComponentOnClick(ui->cbSelectComp->isChecked());
}

void RemoveComponents::on_deselectTriangle_clicked()
{
    meshSel.deselectTriangle();
    meshSel.setRemoveComponentOnClick(ui->cbDeselectComp->isChecked());
}

void RemoveComponents::reject()
{
    // deselect all meshes
    meshSel.clearSelection();
}

// -------------------------------------------------

RemoveComponentsDialog::RemoveComponentsDialog(QWidget* parent, Qt::WFlags fl)
  : QDialog(parent, fl)
{
    widget = new RemoveComponents(this);
    this->setWindowTitle(widget->windowTitle());

    QVBoxLayout* hboxLayout = new QVBoxLayout(this);
    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Close|QDialogButtonBox::Ok);
    QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setText(MeshGui::TaskRemoveComponents::tr("Delete"));
    buttonBox->addButton(MeshGui::TaskRemoveComponents::tr("Invert"),
        QDialogButtonBox::ActionRole);
    
    connect(buttonBox, SIGNAL(clicked(QAbstractButton*)),
            this, SLOT(clicked(QAbstractButton*)));

    hboxLayout->addWidget(widget);
    hboxLayout->addWidget(buttonBox);
}

RemoveComponentsDialog::~RemoveComponentsDialog()
{
}

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
    widget = new RemoveComponents();
    taskbox = new Gui::TaskView::TaskBox(
        QPixmap(), widget->windowTitle(), false, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskRemoveComponents::~TaskRemoveComponents()
{
    // automatically deleted in the sub-class
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
