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
#include <QButtonGroup>
#include <QDialogButtonBox>
#endif

#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Gui/WaitCursor.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/App/Core/Smoothing.h>

#include "DlgSmoothing.h"
#include "ui_DlgSmoothing.h"
#include "Selection.h"


using namespace MeshGui;

/* TRANSLATOR MeshGui::DlgSmoothing */

DlgSmoothing::DlgSmoothing(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_DlgSmoothing())
{
    // clang-format off
    ui->setupUi(this);
    bg = new QButtonGroup(this); //NOLINT
    bg->addButton(ui->radioButtonTaubin, 0);
    bg->addButton(ui->radioButtonLaplace, 1);

    connect(ui->checkBoxSelection, &QCheckBox::toggled,
            this, &DlgSmoothing::onCheckBoxSelectionToggled);
#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
    connect(bg, qOverload<int>(&QButtonGroup::buttonClicked),
            this, &DlgSmoothing::methodClicked);
#else
    connect(bg, qOverload<int>(&QButtonGroup::idClicked),
            this, &DlgSmoothing::methodClicked);
#endif

    ui->labelLambda->setText(QString::fromUtf8("\xce\xbb"));
    ui->labelMu->setText(QString::fromUtf8("\xce\xbc"));
    this->resize(this->sizeHint());
    // clang-format on
}

/*
 *  Destroys the object and frees any allocated resources
 */
DlgSmoothing::~DlgSmoothing()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void DlgSmoothing::methodClicked(int id)
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
    if (ui->radioButtonTaubin->isChecked()) {
        return DlgSmoothing::Taubin;
    }
    if (ui->radioButtonLaplace->isChecked()) {
        return DlgSmoothing::Laplace;
    }
    return DlgSmoothing::None;
}

bool DlgSmoothing::smoothSelection() const
{
    return ui->checkBoxSelection->isChecked();
}

void DlgSmoothing::onCheckBoxSelectionToggled(bool on)
{
    Q_EMIT toggledSelection(on);
}

// ------------------------------------------------

SmoothingDialog::SmoothingDialog(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    widget = new DlgSmoothing(this);
    this->setWindowTitle(widget->windowTitle());

    QVBoxLayout* hboxLayout = new QVBoxLayout(this);
    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    hboxLayout->addWidget(widget);
    hboxLayout->addWidget(buttonBox);
}

SmoothingDialog::~SmoothingDialog() = default;

// ---------------------------------------

/* TRANSLATOR MeshGui::TaskSmoothing */

TaskSmoothing::TaskSmoothing()
{
    widget = new DlgSmoothing();  // NOLINT
    Gui::TaskView::TaskBox* taskbox =
        new Gui::TaskView::TaskBox(QPixmap(), widget->windowTitle(), false, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);

    selection = new Selection();  // NOLINT
    selection->setObjects(
        Gui::Selection().getSelectionEx(nullptr, Mesh::Feature::getClassTypeId()));
    Gui::Selection().clearSelection();
    Gui::TaskView::TaskBox* tasksel = new Gui::TaskView::TaskBox();
    tasksel->groupLayout()->addWidget(selection);
    tasksel->hide();
    Content.push_back(tasksel);

    connect(widget, &DlgSmoothing::toggledSelection, tasksel, &QWidget::setVisible);
}

bool TaskSmoothing::accept()
{
    std::vector<App::DocumentObject*> meshes = selection->getObjects();
    if (meshes.empty()) {
        return true;
    }

    Gui::WaitCursor wc;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Mesh Smoothing"));

    bool hasSelection = false;
    for (auto it : meshes) {
        Mesh::Feature* mesh = static_cast<Mesh::Feature*>(it);
        std::vector<Mesh::FacetIndex> selection;
        if (widget->smoothSelection()) {
            // clear the selection before editing the mesh to avoid
            // to have coloured triangles when doing an 'undo'
            const Mesh::MeshObject* mm = mesh->Mesh.getValuePtr();
            mm->getFacetsFromSelection(selection);
            selection = mm->getPointsFromFacets(selection);
            mm->clearFacetSelection();
            if (!selection.empty()) {
                hasSelection = true;
            }
        }
        Mesh::MeshObject* mm = mesh->Mesh.startEditing();
        switch (widget->method()) {
            case MeshGui::DlgSmoothing::Taubin: {
                MeshCore::TaubinSmoothing s(mm->getKernel());
                s.SetLambda(widget->lambdaStep());
                s.SetMicro(widget->microStep());
                if (widget->smoothSelection()) {
                    s.SmoothPoints(widget->iterations(), selection);
                }
                else {
                    s.Smooth(widget->iterations());
                }
            } break;
            case MeshGui::DlgSmoothing::Laplace: {
                MeshCore::LaplaceSmoothing s(mm->getKernel());
                s.SetLambda(widget->lambdaStep());
                if (widget->smoothSelection()) {
                    s.SmoothPoints(widget->iterations(), selection);
                }
                else {
                    s.Smooth(widget->iterations());
                }
            } break;
            case MeshGui::DlgSmoothing::MedianFilter: {
                MeshCore::MedianFilterSmoothing s(mm->getKernel());
                if (widget->smoothSelection()) {
                    s.SmoothPoints(widget->iterations(), selection);
                }
                else {
                    s.Smooth(widget->iterations());
                }
            } break;
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
