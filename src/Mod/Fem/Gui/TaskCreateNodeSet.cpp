/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
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
# include <QString>
# include <QSlider>

# include <Standard_math.hxx>

# include <Inventor/nodes/SoEventCallback.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/events/SoMouseButtonEvent.h>

# include <SMESH_Mesh.hxx>
# include <SMESHDS_Mesh.hxx>
# include <SMDSAbs_ElementType.hxx>
#endif

#include "ui_TaskCreateNodeSet.h"
#include "TaskCreateNodeSet.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/Utilities.h>

#include <Mod/Fem/App/FemMeshObject.h>
#include <Mod/Fem/App/FemSetNodesObject.h>
#include "ViewProviderFemMesh.h"
#include "FemSelectionGate.h"


using namespace FemGui;
using namespace Gui;


TaskCreateNodeSet::TaskCreateNodeSet(Fem::FemSetNodesObject *pcObject,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("fem-femmesh-create-node-by-poly"),
      tr("Nodes set"),
      true,
      parent),
      pcObject(pcObject),
      selectionMode(none)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskCreateNodeSet();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    QObject::connect(ui->toolButton_Poly,SIGNAL(clicked()),this,SLOT(Poly()));
    QObject::connect(ui->toolButton_Pick,SIGNAL(clicked()),this,SLOT(Pick()));
    QObject::connect(ui->comboBox,SIGNAL(activated  (int)),this,SLOT(SwitchMethod(int)));

    // check if the Link to the FemMesh is defined
    assert(pcObject->FemMesh.getValue<Fem::FemMeshObject*>());
    MeshViewProvider = dynamic_cast<ViewProviderFemMesh*>(Gui::Application::Instance->getViewProvider( pcObject->FemMesh.getValue<Fem::FemMeshObject*>()));
    assert(MeshViewProvider);

    tempSet = pcObject->Nodes.getValues();

    MeshViewProvider->setHighlightNodes(tempSet);

    ui->groupBox_AngleSearch->setEnabled(false);

}

void TaskCreateNodeSet::Poly(void)
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::MDIView* view = doc->getActiveView();
    if (view->getTypeId().isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = ((Gui::View3DInventor*)view)->getViewer();
        viewer->setEditing(true);
        viewer->startSelection(Gui::View3DInventorViewer::Clip);
        viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), DefineNodesCallback,this);
    }
}

void TaskCreateNodeSet::Pick(void)
{
    if (selectionMode == none){
        selectionMode = PickElement;
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new FemSelectionGate(FemSelectionGate::Element));
    }
}

void TaskCreateNodeSet::SwitchMethod(int Value)
{
    if(Value == 1){
        ui->groupBox_AngleSearch->setEnabled(true);
        ui->toolButton_Pick->setEnabled(true);
        ui->toolButton_Poly->setEnabled(false);
    }else{
        ui->groupBox_AngleSearch->setEnabled(false);
        ui->toolButton_Pick->setEnabled(false);
        ui->toolButton_Poly->setEnabled(true);
    }
}



void TaskCreateNodeSet::DefineNodesCallback(void * ud, SoEventCallback * n)
{
    // show the wait cursor because this could take quite some time
    Gui::WaitCursor wc;

    TaskCreateNodeSet *taskBox = static_cast<TaskCreateNodeSet *>(ud);


    // When this callback function is invoked we must in either case leave the edit mode
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());
    view->setEditing(false);
    view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), DefineNodesCallback,ud);
    n->setHandled();

    Gui::SelectionRole role;
    std::vector<SbVec2f> clPoly = view->getGLPolygon(&role);
    if (clPoly.size() < 3)
        return;
    if (clPoly.front() != clPoly.back())
        clPoly.push_back(clPoly.front());

    SoCamera* cam = view->getSoRenderManager()->getCamera();
    SbViewVolume vv = cam->getViewVolume();
    Gui::ViewVolumeProjection proj(vv);
    Base::Polygon2d polygon;
    for (std::vector<SbVec2f>::const_iterator it = clPoly.begin(); it != clPoly.end(); ++it)
        polygon.Add(Base::Vector2d((*it)[0],(*it)[1]));

    taskBox->DefineNodes(polygon,proj,role == Gui::SelectionRole::Inner ? true : false);
}

void TaskCreateNodeSet::DefineNodes(const Base::Polygon2d &polygon,const Gui::ViewVolumeProjection &proj,bool inner)
{
    const SMESHDS_Mesh* data = const_cast<SMESH_Mesh*>(pcObject->FemMesh.getValue<Fem::FemMeshObject*>()->FemMesh.getValue().getSMesh())->GetMeshDS();

    SMDS_NodeIteratorPtr aNodeIter = data->nodesIterator();
    Base::Vector3f pt2d;

    if(! ui->checkBox_Add->isChecked())
        tempSet.clear();

    while (aNodeIter->more()) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        Base::Vector3f vec(aNode->X(),aNode->Y(),aNode->Z());
        pt2d = proj(vec);
        if (polygon.Contains(Base::Vector2d(pt2d.x, pt2d.y)) == inner)
            tempSet.insert(aNode->GetID());
    }

    MeshViewProvider->setHighlightNodes(tempSet);
}

void TaskCreateNodeSet::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == none)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        std::string subName(msg.pSubName);
        unsigned int i=0;
        for(;i<subName.size();i++)
            if(msg.pSubName[i]=='F')
                break;

        int elem = atoi(subName.substr(4).c_str());
        int face = atoi(subName.substr(i+1).c_str() );

            tempSet.clear();


        Base::Console().Message("Picked Element:%i Face:%i\n",elem,face);


        if(! ui->checkBox_Add->isChecked()){
            std::set<long> tmp = pcObject->FemMesh.getValue<Fem::FemMeshObject*>()->FemMesh.getValue().getSurfaceNodes(elem,face);
            tempSet.insert(tmp.begin(),tmp.end());
        }else
            tempSet = pcObject->FemMesh.getValue<Fem::FemMeshObject*>()->FemMesh.getValue().getSurfaceNodes(elem,face);

        selectionMode = none;
        Gui::Selection().rmvSelectionGate();

        MeshViewProvider->setHighlightNodes(tempSet);

    }
}


TaskCreateNodeSet::~TaskCreateNodeSet()
{
    delete ui;
}


#include "moc_TaskCreateNodeSet.cpp"
