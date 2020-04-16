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

#ifndef _PreComp_
# include <QPushButton>
#endif

#include "SegmentationManual.h"
#include "ui_SegmentationManual.h"
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>

#include <Mod/Mesh/App/Core/Approximation.h>
#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/Segmentation.h>
#include <Mod/Mesh/App/Core/Curvature.h>
#include <Mod/Mesh/App/Core/Smoothing.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/MeshFeature.h>

using namespace ReverseEngineeringGui;


SegmentationManual::SegmentationManual(QWidget* parent, Qt::WindowFlags fl)
  : QWidget(parent, fl)
  , ui(new Ui_SegmentationManual)
{
    ui->setupUi(this);
    ui->spSelectComp->setRange(1, INT_MAX);
    ui->spSelectComp->setValue(10);

    Gui::Selection().clearSelection();
    meshSel.setCheckOnlyVisibleTriangles(ui->visibleTriangles->isChecked());
    meshSel.setCheckOnlyPointToUserTriangles(ui->screenTriangles->isChecked());
    meshSel.setEnabledViewerSelection(false);
}

SegmentationManual::~SegmentationManual()
{
}

void SegmentationManual::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

void SegmentationManual::on_selectRegion_clicked()
{
    meshSel.startSelection();
}

void SegmentationManual::on_selectAll_clicked()
{
    // select the complete meshes
    meshSel.fullSelection();
}

void SegmentationManual::on_deselectAll_clicked()
{
    // deselect all meshes
    meshSel.clearSelection();
}

void SegmentationManual::on_selectComponents_clicked()
{
    // select components up to a certain size
    int size = ui->spSelectComp->value();
    meshSel.selectComponent(size);
}

void SegmentationManual::on_visibleTriangles_toggled(bool on)
{
    meshSel.setCheckOnlyVisibleTriangles(on);
}

void SegmentationManual::on_screenTriangles_toggled(bool on)
{
    meshSel.setCheckOnlyPointToUserTriangles(on);
}

void SegmentationManual::on_cbSelectComp_toggled(bool on)
{
    meshSel.setAddComponentOnClick(on);
}

void SegmentationManual::createSegment()
{
    Gui::Document* gdoc = Gui::Application::Instance->activeDocument();
    if (!gdoc)
        return;
    // delete all selected faces
    App::Document* adoc = gdoc->getDocument();
    gdoc->openCommand("Segmentation");

    std::vector<Mesh::Feature*> meshes = adoc->getObjectsOfType<Mesh::Feature>();
    bool selected = false;
    for (auto it : meshes) {
        const Mesh::MeshObject& mesh = it->Mesh.getValue();
        const MeshCore::MeshKernel& kernel = mesh.getKernel();

        MeshCore::MeshAlgorithm algo(kernel);
        unsigned long ct = algo.CountFacetFlag(MeshCore::MeshFacet::SELECTED);
        if (ct > 0) {
            selected = true;

            std::vector<unsigned long> facets;
            algo.GetFacetsFlag(facets, MeshCore::MeshFacet::SELECTED);

            std::unique_ptr<Mesh::MeshObject> segment(mesh.meshFromSegment(facets));
            Mesh::Feature* feaSegm = static_cast<Mesh::Feature*>(adoc->addObject("Mesh::Feature", "Segment"));
            Mesh::MeshObject* feaMesh = feaSegm->Mesh.startEditing();
            feaMesh->swap(*segment);
            feaSegm->Mesh.finishEditing();
        }
    }

    if (!selected)
        gdoc->abortCommand();
    else
        gdoc->commitCommand();
    meshSel.clearSelection();
}

void SegmentationManual::on_selectTriangle_clicked()
{
    meshSel.selectTriangle();
    meshSel.setAddComponentOnClick(ui->cbSelectComp->isChecked());
}

void SegmentationManual::reject()
{
    // deselect all meshes
    meshSel.clearSelection();
    meshSel.setEnabledViewerSelection(true);
}

// -------------------------------------------------

/* TRANSLATOR ReverseEngineeringGui::TaskSegmentationManual */

TaskSegmentationManual::TaskSegmentationManual()
{
    widget = new SegmentationManual();
    taskbox = new Gui::TaskView::TaskBox(
        QPixmap(), widget->windowTitle(), false, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskSegmentationManual::~TaskSegmentationManual()
{
    // automatically deleted in the sub-class
}

void TaskSegmentationManual::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btn = box->button(QDialogButtonBox::Ok);
    btn->setText(tr("Create"));
}

bool TaskSegmentationManual::accept()
{
    return false;
}

void TaskSegmentationManual::clicked(int id)
{
    if (id == QDialogButtonBox::Ok) {
        widget->createSegment();
    }
    else if (id == QDialogButtonBox::Close) {
        widget->reject();
    }
}

#include "moc_SegmentationManual.cpp"
