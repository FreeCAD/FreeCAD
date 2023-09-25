/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <QGroupBox>
#include <QLabel>
#endif

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MenuManager.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/ToolBarManager.h>
#include <Mod/Mesh/App/MeshFeature.h>

#include "Workbench.h"


using namespace MeshGui;

#if 0  // needed for Qt's lupdate utility
    qApp->translate("Workbench", "Analyze");
    qApp->translate("Workbench", "Boolean");
    qApp->translate("Workbench", "&Meshes");
    qApp->translate("Workbench", "Cutting");
    qApp->translate("Workbench", "Mesh tools");
    qApp->translate("Workbench", "Mesh modify");
    qApp->translate("Workbench", "Mesh boolean");
    qApp->translate("Workbench", "Mesh cutting");
    qApp->translate("Workbench", "Mesh segmentation");
    qApp->translate("Workbench", "Mesh analyze");
#endif

/// @namespace MeshGui @class Workbench
TYPESYSTEM_SOURCE(MeshGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench() = default;

class MeshInfoWatcher: public Gui::TaskView::TaskWatcher, public Gui::SelectionObserver
{
public:
    MeshInfoWatcher()
        : TaskWatcher(nullptr)
    {
        // NOLINTBEGIN
        labelPoints = new QLabel();
        labelPoints->setText(tr("Number of points:"));

        labelFacets = new QLabel();
        labelFacets->setText(tr("Number of facets:"));

        numPoints = new QLabel();
        numFacets = new QLabel();

        labelMin = new QLabel();
        labelMin->setText(tr("Minimum bound:"));

        labelMax = new QLabel();
        labelMax->setText(tr("Maximum bound:"));

        numMin = new QLabel();
        numMax = new QLabel();
        // NOLINTEND

        QGroupBox* box = new QGroupBox();
        box->setTitle(tr("Mesh info box"));
        // box->setAutoFillBackground(true);
        QGridLayout* grid = new QGridLayout(box);
        grid->addWidget(labelPoints, 0, 0);
        grid->addWidget(numPoints, 0, 1);
        grid->addWidget(labelFacets, 1, 0);
        grid->addWidget(numFacets, 1, 1);

        grid->addWidget(labelMin, 2, 0);
        grid->addWidget(numMin, 2, 1);
        grid->addWidget(labelMax, 3, 0);
        grid->addWidget(numMax, 3, 1);

        Gui::TaskView::TaskBox* taskbox =
            new Gui::TaskView::TaskBox(QPixmap(), tr("Mesh info"), false, nullptr);
        taskbox->groupLayout()->addWidget(box);
        Content.push_back(taskbox);
    }
    bool shouldShow() override
    {
        return true;
    }
    void onSelectionChanged(const Gui::SelectionChanges&) override
    {
        Base::BoundBox3d bbox;
        unsigned long countPoints = 0, countFacets = 0;
        std::vector<Mesh::Feature*> mesh = Gui::Selection().getObjectsOfType<Mesh::Feature>();
        for (auto it : mesh) {
            countPoints += it->Mesh.getValue().countPoints();
            countFacets += it->Mesh.getValue().countFacets();
            bbox.Add(it->Mesh.getBoundingBox());
        }

        if (countPoints > 0) {
            numPoints->setText(QString::number(countPoints));
            numFacets->setText(QString::number(countFacets));
            numMin->setText(tr("X: %1\tY: %2\tZ: %3").arg(bbox.MinX).arg(bbox.MinY).arg(bbox.MinZ));
            numMax->setText(tr("X: %1\tY: %2\tZ: %3").arg(bbox.MaxX).arg(bbox.MaxY).arg(bbox.MaxZ));
        }
        else {
            numPoints->setText(QString::fromLatin1(""));
            numFacets->setText(QString::fromLatin1(""));
            numMin->setText(QString::fromLatin1(""));
            numMax->setText(QString::fromLatin1(""));
        }
    }

private:
    QLabel* labelPoints;
    QLabel* numPoints;
    QLabel* labelFacets;
    QLabel* numFacets;
    QLabel* labelMin;
    QLabel* numMin;
    QLabel* labelMax;
    QLabel* numMax;
};

void Workbench::activated()
{
    Gui::Workbench::activated();

    std::vector<Gui::TaskView::TaskWatcher*> Watcher;
    Watcher.push_back(new MeshInfoWatcher);
    addTaskWatcher(Watcher);
}

void Workbench::deactivated()
{
    Gui::Workbench::deactivated();
    removeTaskWatcher();
}

void Workbench::setupContextMenu(const char* recipient, Gui::MenuItem* item) const
{
    StdWorkbench::setupContextMenu(recipient, item);
    if (Gui::Selection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0) {
        *item << "Separator"
              << "Mesh_Import"
              << "Mesh_Export"
              << "Mesh_VertexCurvature";
    }
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");
    Gui::MenuItem* mesh = new Gui::MenuItem;
    root->insertItem(item, mesh);

    // analyze
    Gui::MenuItem* analyze = new Gui::MenuItem;
    analyze->setCommand("Analyze");
    *analyze << "Mesh_Evaluation"
             << "Mesh_EvaluateFacet"
             << "Mesh_CurvatureInfo"
             << "Separator"
             << "Mesh_EvaluateSolid"
             << "Mesh_BoundingBox";

    // boolean
    Gui::MenuItem* boolean = new Gui::MenuItem;
    boolean->setCommand("Boolean");
    *boolean << "Mesh_Union"
             << "Mesh_Intersection"
             << "Mesh_Difference";

    // cutting
    Gui::MenuItem* cutting = new Gui::MenuItem;
    cutting->setCommand("Cutting");
    *cutting << "Mesh_PolyCut"
             << "Mesh_PolyTrim"
             //<< "Mesh_PolySegm"
             << "Mesh_TrimByPlane"
             << "Mesh_SectionByPlane"
             << "Mesh_CrossSections";

    mesh->setCommand("&Meshes");
    *mesh << "Mesh_Import"
          << "Mesh_Export"
          << "Mesh_FromPartShape"
          << "Mesh_RemeshGmsh"
          << "Separator" << analyze << "Mesh_VertexCurvature"
          << "Mesh_HarmonizeNormals"
          << "Mesh_FlipNormals"
          << "Separator"
          << "Mesh_FillupHoles"
          << "Mesh_FillInteractiveHole"
          << "Mesh_AddFacet"
          << "Mesh_RemoveComponents"
          << "Mesh_RemoveCompByHand"
          << "Mesh_Segmentation"
          << "Mesh_SegmentationBestFit"
          << "Separator"
          << "Mesh_Smoothing"
          << "Mesh_Decimating"
          << "Mesh_Scale"
          << "Separator"
          << "Mesh_BuildRegularSolid" << boolean << cutting << "Separator"
          << "Mesh_Merge"
          << "Mesh_SplitComponents"
          << "Separator";
    Gui::CommandManager& mgr = Gui::Application::Instance->commandManager();
    if (mgr.getCommandByName("MeshPart_CreateFlatMesh")) {
        *mesh << "MeshPart_CreateFlatMesh";
    }
    if (mgr.getCommandByName("MeshPart_CreateFlatFace")) {
        *mesh << "MeshPart_CreateFlatFace";
    }
    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();

    Gui::ToolBarItem* mesh = new Gui::ToolBarItem(root);
    mesh->setCommand("Mesh tools");
    *mesh << "Mesh_Import"
          << "Mesh_Export"
          << "Mesh_FromPartShape"
          << "Mesh_BuildRegularSolid";

    Gui::ToolBarItem* modifying = new Gui::ToolBarItem(root);
    modifying->setCommand("Mesh modify");
    *modifying << "Mesh_HarmonizeNormals"
               << "Mesh_FlipNormals"
               << "Mesh_FillupHoles"
               << "Mesh_FillInteractiveHole"
               << "Mesh_AddFacet"
               << "Mesh_RemoveComponents"
               << "Mesh_Smoothing"
               << "Mesh_RemeshGmsh"
               << "Mesh_Decimating"
               << "Mesh_Scale";

    Gui::ToolBarItem* boolean = new Gui::ToolBarItem(root);
    boolean->setCommand("Mesh boolean");
    *boolean << "Mesh_Union"
             << "Mesh_Intersection"
             << "Mesh_Difference";

    Gui::ToolBarItem* cutting = new Gui::ToolBarItem(root);
    cutting->setCommand("Mesh cutting");
    *cutting << "Mesh_PolyCut"
             << "Mesh_PolyTrim"
             << "Mesh_TrimByPlane"
             << "Mesh_SectionByPlane"
             << "Mesh_CrossSections";

    Gui::ToolBarItem* compseg = new Gui::ToolBarItem(root);
    compseg->setCommand("Mesh segmentation");
    *compseg << "Mesh_Merge"
             << "Mesh_SplitComponents"
             << "Mesh_Segmentation"
             << "Mesh_SegmentationBestFit";

    Gui::ToolBarItem* analyze = new Gui::ToolBarItem(root);
    analyze->setCommand("Mesh analyze");
    *analyze << "Mesh_Evaluation"
             << "Mesh_EvaluateFacet"
             << "Mesh_VertexCurvature"
             << "Mesh_CurvatureInfo"
             << "Mesh_EvaluateSolid"
             << "Mesh_BoundingBox";


    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    // Mesh tools
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    Gui::ToolBarItem* mesh;

    mesh = new Gui::ToolBarItem(root);
    mesh->setCommand("Mesh tools");
    *mesh << "Mesh_Import"
          << "Mesh_Export"
          << "Mesh_PolyCut";

    mesh = new Gui::ToolBarItem(root);
    mesh->setCommand("Mesh test suite");
    *mesh << "Mesh_Demolding"
          << "Mesh_Transform"
          << "Separator";

    return root;
}
