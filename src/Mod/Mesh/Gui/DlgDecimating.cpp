/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include "DlgDecimating.h"
#include "ui_DlgDecimating.h"

#include <Gui/WaitCursor.h>
#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Mod/Mesh/App/MeshFeature.h>

using namespace MeshGui;

/* TRANSLATOR MeshGui::DlgDecimating */

DlgDecimating::DlgDecimating(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
    , numberOfTriangles(0)
    , ui(new Ui_DlgDecimating)
{
    ui->setupUi(this);
    ui->spinBoxReduction->setMinimumWidth(60);
    ui->checkAbsolueNumber->setEnabled(false);
    on_checkAbsolueNumber_toggled(false);
}

DlgDecimating::~DlgDecimating()
{
}

bool DlgDecimating::isAbsoluteNumber() const
{
    return ui->checkAbsolueNumber->isChecked();
}

int DlgDecimating::targetNumberOfTriangles() const
{
    if (ui->checkAbsolueNumber->isChecked()) {
        return ui->spinBoxReduction->value();
    }
    else {
        return numberOfTriangles * (1.0 - reduction());
    }
}

void DlgDecimating::setNumberOfTriangles(int num)
{
    numberOfTriangles = num;
    ui->checkAbsolueNumber->setEnabled(num > 0);
    if (num <= 0)
        ui->checkAbsolueNumber->setChecked(false);
}

void DlgDecimating::on_checkAbsolueNumber_toggled(bool on)
{
    ui->sliderReduction->setDisabled(on);
    ui->groupBoxTolerance->setDisabled(on);

    if (on) {
        disconnect(ui->sliderReduction, SIGNAL(valueChanged(int)), ui->spinBoxReduction, SLOT(setValue(int)));
        disconnect(ui->spinBoxReduction, SIGNAL(valueChanged(int)), ui->sliderReduction, SLOT(setValue(int)));
        ui->spinBoxReduction->setRange(1, numberOfTriangles);
        ui->spinBoxReduction->setValue(numberOfTriangles * (1.0 - reduction()));
        ui->spinBoxReduction->setSuffix(QString());
        ui->checkAbsolueNumber->setText(tr("Absolute number (Maximum: %1)").arg(numberOfTriangles));
    }
    else {
        ui->spinBoxReduction->setRange(0, 100);
        ui->spinBoxReduction->setValue(ui->sliderReduction->value());
        ui->spinBoxReduction->setSuffix(QString::fromLatin1("%"));
        ui->checkAbsolueNumber->setText(tr("Absolute number"));
        connect(ui->sliderReduction, SIGNAL(valueChanged(int)), ui->spinBoxReduction, SLOT(setValue(int)));
        connect(ui->spinBoxReduction, SIGNAL(valueChanged(int)), ui->sliderReduction, SLOT(setValue(int)));
    }
}

double DlgDecimating::tolerance() const
{
    return ui->spinBoxTolerance->value();
}

/**
 * Returns the level of reduction in the range [0, 1]. 0 means no reduction, 1 means full
 * reduction.
 */
double DlgDecimating::reduction() const
{
    double max = static_cast<double>(ui->sliderReduction->maximum());
    double min = static_cast<double>(ui->sliderReduction->minimum());
    double val = static_cast<double>(ui->sliderReduction->value());
    return (val - min)/(max - min);
}

// ---------------------------------------

/* TRANSLATOR MeshGui::TaskDecimating */

TaskDecimating::TaskDecimating()
{
    widget = new DlgDecimating();
    Gui::TaskView::TaskBox* taskbox = new Gui::TaskView::TaskBox(
        QPixmap(), widget->windowTitle(), false, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);

    std::vector<Mesh::Feature*> meshes = Gui::Selection().getObjectsOfType<Mesh::Feature>();
    if (meshes.size() == 1) {
        Mesh::Feature* mesh = meshes.front();
        const Mesh::MeshObject& mm = mesh->Mesh.getValue();
        widget->setNumberOfTriangles(static_cast<int>(mm.countFacets()));
    }
}

TaskDecimating::~TaskDecimating()
{
    // automatically deleted in the sub-class
}

bool TaskDecimating::accept()
{
    std::vector<Mesh::Feature*> meshes = Gui::Selection().getObjectsOfType<Mesh::Feature>();
    if (meshes.empty())
        return true;
    Gui::Selection().clearSelection();

    Gui::WaitCursor wc;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Mesh Decimating"));

    float tolerance = widget->tolerance();
    float reduction = widget->reduction();
    bool absolute = widget->isAbsoluteNumber();
    int targetSize = 0;
    if (absolute)
        targetSize = widget->targetNumberOfTriangles();
    for (std::vector<Mesh::Feature*>::const_iterator it = meshes.begin(); it != meshes.end(); ++it) {
        Mesh::Feature* mesh = *it;
        Mesh::MeshObject* mm = mesh->Mesh.startEditing();
        if (absolute)
            mm->decimate(targetSize);
        else
            mm->decimate(tolerance, reduction);
        mesh->Mesh.finishEditing();
    }

    Gui::Command::commitCommand();
    return true;
}

#include "moc_DlgDecimating.cpp"
