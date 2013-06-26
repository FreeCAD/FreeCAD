/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QButtonGroup>
# include <QDialogButtonBox>
#endif

#include "DlgSmoothing.h"
#include "ui_DlgSmoothing.h"
#include "Selection.h"

#include <Gui/WaitCursor.h>
#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/App/Core/Smoothing.h>

using namespace MeshGui;

/* TRANSLATOR MeshGui::DlgSmoothing */

DlgSmoothing::DlgSmoothing(QWidget* parent)
    : QWidget(parent), ui(new Ui_DlgSmoothing())
{
    ui->setupUi(this);
    bg = new QButtonGroup(this);
    bg->addButton(ui->radioButtonTaubin, 0);
    bg->addButton(ui->radioButtonLaplace, 1);
    connect(bg, SIGNAL(buttonClicked(int)),
            this, SLOT(method_clicked(int)));

    ui->labelLambda->setText(QString::fromUtf8("\xce\xbb"));
    ui->labelMu->setText(QString::fromUtf8("\xce\xbc"));
    this->resize(this->sizeHint());
}

/*
 *  Destroys the object and frees any allocated resources
 */
DlgSmoothing::~DlgSmoothing()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void DlgSmoothing::method_clicked(int id)
{
    if (bg->button(id) == ui->radioButtonTaubin) {
        ui->labelMu->setEnabled(true);
        ui->spinMicro->setEnabled(true);
    }
    else {
        ui->labelMu->setEnabled(false);
        ui->spinMicro->setEnabled(false);
    }
}

int DlgSmoothing::iterations() const
{
    return ui->iterations->value();
}

double DlgSmoothing::lambdaStep() const
{
    return ui->spinLambda->value();
}

double DlgSmoothing::microStep() const
{
    return ui->spinMicro->value();
}

DlgSmoothing::Smooth DlgSmoothing::method() const
{
    if (ui->radioButtonTaubin->isChecked())
        return DlgSmoothing::Taubin;
    else if (ui->radioButtonLaplace->isChecked())
        return DlgSmoothing::Laplace;
    return DlgSmoothing::None;
}

bool DlgSmoothing::smoothSelection() const
{
    return ui->checkBoxSelection->isChecked();
}

void DlgSmoothing::on_checkBoxSelection_toggled(bool on)
{
    /*emit*/ toggledSelection(on);
}

// ------------------------------------------------

SmoothingDialog::SmoothingDialog(QWidget* parent, Qt::WFlags fl)
  : QDialog(parent, fl)
{
    widget = new DlgSmoothing(this);
    this->setWindowTitle(widget->windowTitle());

    QVBoxLayout* hboxLayout = new QVBoxLayout(this);
    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    
    connect(buttonBox, SIGNAL(accepted()),
            this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()),
            this, SLOT(reject()));

    hboxLayout->addWidget(widget);
    hboxLayout->addWidget(buttonBox);
}

SmoothingDialog::~SmoothingDialog()
{
}

// ---------------------------------------

/* TRANSLATOR MeshGui::TaskSmoothing */

TaskSmoothing::TaskSmoothing()
{
    widget = new DlgSmoothing();
    Gui::TaskView::TaskBox* taskbox = new Gui::TaskView::TaskBox(
        QPixmap(), widget->windowTitle(), false, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);

    selection = new Selection();
    selection->setObjects(Gui::Selection().getSelectionEx(0, Mesh::Feature::getClassTypeId()));
    Gui::TaskView::TaskGroup* tasksel = new Gui::TaskView::TaskGroup();
    tasksel->groupLayout()->addWidget(selection);
    tasksel->hide();
    Content.push_back(tasksel);

    connect(widget, SIGNAL(toggledSelection(bool)),
            tasksel, SLOT(setVisible(bool)));
}

TaskSmoothing::~TaskSmoothing()
{
    // automatically deleted in the sub-class
}

bool TaskSmoothing::accept()
{
    std::vector<App::DocumentObject*> meshes = selection->getObjects();
    if (meshes.empty())
        return true;

    Gui::WaitCursor wc;
    Gui::Command::openCommand("Mesh Smoothing");

    bool hasSelection = false;
    for (std::vector<App::DocumentObject*>::const_iterator it = meshes.begin(); it != meshes.end(); ++it) {
        Mesh::Feature* mesh = static_cast<Mesh::Feature*>(*it);
        std::vector<unsigned long> selection;
        if (widget->smoothSelection()) {
            // clear the selection before editing the mesh to avoid
            // to have coloured triangles when doing an 'undo'
            const Mesh::MeshObject* mm = mesh->Mesh.getValuePtr();
            mm->getFacetsFromSelection(selection);
            selection = mm->getPointsFromFacets(selection);
            mm->clearFacetSelection();
            if (!selection.empty())
                hasSelection = true;
        }
        Mesh::MeshObject* mm = mesh->Mesh.startEditing();
        switch (widget->method()) {
            case MeshGui::DlgSmoothing::Taubin:
                {
                    MeshCore::TaubinSmoothing s(mm->getKernel());
                    s.SetLambda(widget->lambdaStep());
                    s.SetMicro(widget->microStep());
                    if (widget->smoothSelection()) {
                        s.SmoothPoints(widget->iterations(), selection);
                    }
                    else {
                        s.Smooth(widget->iterations());
                    }
                }   break;
            case MeshGui::DlgSmoothing::Laplace:
                {
                    MeshCore::LaplaceSmoothing s(mm->getKernel());
                    s.SetLambda(widget->lambdaStep());
                    if (widget->smoothSelection()) {
                        s.SmoothPoints(widget->iterations(), selection);
                    }
                    else {
                        s.Smooth(widget->iterations());
                    }
                }   break;
            default:
                break;
        }
        mesh->Mesh.finishEditing();
    }

    if (widget->smoothSelection() && !hasSelection) {
        Gui::Command::abortCommand();
        return false;
    }

    Gui::Command::commitCommand();
    return true;
}

#include "moc_DlgSmoothing.cpp"
