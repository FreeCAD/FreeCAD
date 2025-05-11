/***************************************************************************
 *   Copyright (c) 2023 Peter McB                                          *
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <boost/algorithm/string.hpp>

#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoEventCallback.h>

#include <QAction>
#include <QApplication>
#include <QMessageBox>
#include <SMESH_Mesh.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMESH_MeshEditor.hxx>
#endif

#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Utilities.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>

#include <Mod/Fem/App/FemMeshObject.h>
#include <Mod/Fem/App/FemSetElementNodesObject.h>

#include "FemSelectionGate.h"
#include "TaskCreateElementSet.h"
#include "ViewProviderFemMesh.h"
#include "ui_TaskCreateElementSet.h"

#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>


using namespace Gui;
// using namespace Fem;
using namespace FemGui;
using namespace std;

std::string TaskCreateElementSet::currentProject = "";

namespace
{
std::string inp_file = "abaqusOutputFileEraseElements.inp";

std::string createdMesh = "cDUMMY";
std::string startResultMesh = "StartResultMesh";  // StartResultMesh";
std::string newResultMesh = "NewResultMesh";
std::string newFemMesh = "NewFemMesh";

std::string uniqueMesh = "";
std::string newProject = "";
std::string resultMesh = "Result";
std::string actualResultMesh = "XXXXXX";
std::string lastName = "";
std::string highLightMesh;  //  name of highlighted mesh
std::string meshType;       // type of - either 'result' or 'femmesh'
int passResult = 0;
int passFemMesh = 0;
double** nodeCoords;  // these are node coords
int* nodeNumbers;     // these are node numbers - probably not required[100];

void addFaceToMesh(const std::vector<const SMDS_MeshNode*> nodes, SMESHDS_Mesh* MeshDS, int EID)
{
    int nbNodes = nodes.size();
    switch (nbNodes) {
        case 3:  // 3 node triangle
            MeshDS->AddFaceWithID(nodes[0], nodes[1], nodes[2], EID);
            break;
        case 4:  // 4 node quadrilateral
            MeshDS->AddFaceWithID(nodes[0], nodes[1], nodes[2], nodes[3], EID);
            break;
        case 6:  // 6 node triangle
            MeshDS->AddFaceWithID(nodes[0], nodes[1], nodes[2], nodes[3], nodes[4], nodes[5], EID);
            break;
        case 8:  // 8 node quadrilateral
            MeshDS->AddFaceWithID(nodes[0],
                                  nodes[1],
                                  nodes[2],
                                  nodes[3],
                                  nodes[4],
                                  nodes[5],
                                  nodes[6],
                                  nodes[7],
                                  EID);
            break;
    }
}  // addFaceToMesh

void addVolumeToMesh(const std::vector<const SMDS_MeshNode*> nodes, SMESHDS_Mesh* MeshDS, int EID)
{

    int nbNodes = nodes.size();

    switch (nbNodes) {
        case 4:  // 4 node tetrahedron
            MeshDS->AddVolumeWithID(nodes[0], nodes[1], nodes[2], nodes[3], EID);
            break;
        case 5:  // 5 node pyramid
            MeshDS->AddVolumeWithID(nodes[0], nodes[1], nodes[2], nodes[3], nodes[4], EID);
            break;
        case 6:  // 6 node pentahedron
            MeshDS
                ->AddVolumeWithID(nodes[0], nodes[1], nodes[2], nodes[3], nodes[4], nodes[5], EID);
            break;
        case 8:  // 8 node hexahedron
            MeshDS->AddVolumeWithID(nodes[0],
                                    nodes[1],
                                    nodes[2],
                                    nodes[3],
                                    nodes[4],
                                    nodes[5],
                                    nodes[6],
                                    nodes[7],
                                    EID);
            break;
        case 10:  // 10 node tetrahedron
            MeshDS->AddVolumeWithID(nodes[0],
                                    nodes[1],
                                    nodes[2],
                                    nodes[3],
                                    nodes[4],
                                    nodes[5],
                                    nodes[6],
                                    nodes[7],
                                    nodes[8],
                                    nodes[9],
                                    EID);
            break;
        case 13:  // 13 node pyramid
            MeshDS->AddVolumeWithID(nodes[0],
                                    nodes[1],
                                    nodes[2],
                                    nodes[3],
                                    nodes[4],
                                    nodes[5],
                                    nodes[6],
                                    nodes[7],
                                    nodes[8],
                                    nodes[9],
                                    nodes[10],
                                    nodes[11],
                                    nodes[12],
                                    EID);
            break;
        case 15:  // 15 node pentahedron
            MeshDS->AddVolumeWithID(nodes[0],
                                    nodes[1],
                                    nodes[2],
                                    nodes[3],
                                    nodes[4],
                                    nodes[5],
                                    nodes[6],
                                    nodes[7],
                                    nodes[8],
                                    nodes[9],
                                    nodes[10],
                                    nodes[11],
                                    nodes[12],
                                    nodes[13],
                                    nodes[14],
                                    EID);
            break;
        case 20:  // 20 node hexahedron
            MeshDS->AddVolumeWithID(nodes[0],
                                    nodes[1],
                                    nodes[2],
                                    nodes[3],
                                    nodes[4],
                                    nodes[5],
                                    nodes[6],
                                    nodes[7],
                                    nodes[8],
                                    nodes[9],
                                    nodes[10],
                                    nodes[11],
                                    nodes[12],
                                    nodes[13],
                                    nodes[14],
                                    nodes[15],
                                    nodes[16],
                                    nodes[17],
                                    nodes[18],
                                    nodes[19],
                                    EID);
            break;
    }  // default: {}

}  // addVolumeToMesh

void myCopyResultsMesh(std::string oldName, std::string newName)
{
    int error = 0;

    Base::Console().warning("copy: %s and %s\n", oldName.c_str(), newName.c_str());
    if (oldName.compare(newName) == 0 && error == 0) {
        error = 1;
        Base::Console().warning("Can't copy ResultMesh to ResultMesh: %s and %s\n",
                                oldName.c_str(),
                                newName.c_str());
        QMessageBox::warning(
            Gui::getMainWindow(),
            //        QMessageBox::warning(Gui::MainWindow(),
            qApp->translate("CmdFemCreateElementsSet", "Wrong selection"),
            qApp->translate("CmdFemCreateElementsSet", "Can't copy ResultMesh to ResultMesh"));
    }
    if ((oldName.find("Result") == std::string::npos || newName.find("Result") == std::string::npos)
        && error == 0) {
        error = 1;
        Base::Console().warning("Mesh must be results: %s\n", oldName.c_str());
        QMessageBox::warning(
            Gui::getMainWindow(),
            //        QMessageBox::warning(Gui::MainWindow(),
            qApp->translate("CmdFemCreateElementsSet", "Wrong selection"),
            qApp->translate("CmdFemCreateElementsSet", "Mesh must be a Results mesh"));
    }
    if (error == 0) {
        Gui::Command::doCommand(Gui::Command::Doc,
                                "c = FreeCADGui.ActiveDocument.getObject(\'%s\')",
                                oldName.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "FreeCAD.ActiveDocument.%s.FemMesh = c.Object.FemMesh",
                                newName.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "Gui.ActiveDocument.getObject(\'%s\').BackfaceCulling = False",
                                newName.c_str());
    }
}  // copyresultsmesh

void generateMesh(std::string meshType)
{
    if (passResult + passFemMesh == 0) {
        Gui::Command::doCommand(Gui::Command::Doc, "import Fem,os");
    }

    if (strcmp(meshType.c_str(), "result") == 0) {
        if (passResult == 0) {
            string xstr(startResultMesh.c_str());
            createdMesh = newResultMesh.c_str();
            Gui::Command::doCommand(
                Gui::Command::Doc,
                "obj1 = App.ActiveDocument.addObject('Fem::FemMeshObject', \'%s\')",
                startResultMesh.c_str());
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "Gui.ActiveDocument.getObject(\'%s\').Visibility = False",
                                    startResultMesh.c_str());
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "ne = Gui.ActiveDocument.getObject(\'%s\')",
                                    actualResultMesh.c_str());
            Gui::Command::doCommand(Gui::Command::Doc, "obj1.FemMesh = ne.Object.FemMesh");
        }
        else if (passResult > 0) {
            createdMesh = newResultMesh.c_str();
        }
        passResult += 1;
    }
    else if (strcmp(meshType.c_str(), "femmesh") == 0) {
        if (passFemMesh == 0) {
            string xstr("no rename required");
            createdMesh = newFemMesh.c_str();
        }
        else if (passFemMesh > 0) {
            createdMesh = newFemMesh.c_str();
        }
        passFemMesh += 1;
    }
    App::Document* doc = App::GetApplication().getActiveDocument();
    uniqueMesh = doc->getUniqueObjectName(createdMesh.c_str());

    Gui::Command::doCommand(Gui::Command::Doc,
                            "Gui.ActiveDocument.getObject(\'%s\').Visibility = False",
                            highLightMesh.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "newermesh = Fem.read(\'%s\')", inp_file.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,
                            "obj = App.ActiveDocument.addObject('Fem::FemMeshObject', \'%s\')",
                            uniqueMesh.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "obj.FemMesh = newermesh");
    Gui::Command::doCommand(Gui::Command::Doc,
                            "Gui.ActiveDocument.getObject(\'%s\').BackfaceCulling = False",
                            uniqueMesh.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,
                            "Gui.ActiveDocument.getObject(\'%s\').Visibility = True",
                            uniqueMesh.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "os.remove(\'%s\')", inp_file.c_str());

    if (strcmp(meshType.c_str(), "result") == 0) {
        Gui::Command::doCommand(Gui::Command::Doc,
                                "c = FreeCADGui.ActiveDocument.getObject(\'%s\')",
                                uniqueMesh.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "FreeCAD.ActiveDocument.%s.FemMesh = c.Object.FemMesh",
                                actualResultMesh.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "Gui.ActiveDocument.getObject(\'%s\').BackfaceCulling = False",
                                actualResultMesh.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "Gui.ActiveDocument.getObject(\'%s\').Visibility = True",
                                actualResultMesh.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "Gui.ActiveDocument.getObject(\'%s\').Visibility = False",
                                uniqueMesh.c_str());
    }
}  // generate mesh

void writeToFile(std::string fileName,
                 SMESHDS_Mesh* newMesh,
                 int* nodeNumbers,
                 double** nodeCoords,
                 int rows,
                 int requiredType)
{
    std::map<int, std::string> elType2D;
    std::map<int, std::string> elType3D;

    elType2D[3] = "S3";  // 3 node triangle
    elType2D[6] = "S6";  // 4 node quadrilateral
    elType2D[4] = "S4";  // 6 node triangle
    elType2D[8] = "S8";  // 8 node quadrilateral

    elType3D[4] = "C3D4";    // 4 node tetrahedron
    elType3D[6] = "C3D6";    // 6 node pentahedron
    elType3D[8] = "C3D8";    // 8 node hexahedron
    elType3D[10] = "C3D10";  // 10 node tetrahedron
    elType3D[15] = "C3D15";  // 15 node pentahedron
    elType3D[20] = "C3D20";  // 20 node hexahedron
                             // no pyramid elements
    FILE* fptr = fopen(fileName.c_str(), "w");
    if (fptr == NULL) {
        return;
    }
    fprintf(fptr, "** written by Erase Elements inp file writer for CalculiX,Abaqus meshes\n");
    fprintf(fptr, "** all mesh elements.\n");

    fprintf(fptr, "\n");
    fprintf(fptr, "\n");
    fprintf(fptr, "** Nodes\n");
    fprintf(fptr, "*Node, NSET=Nall\n");

    for (int i = 1; i < rows + 1; i++) {
        if (nodeNumbers[i] > 0) {
            fprintf(fptr,
                    "%d, %e, %e, %e\n",
                    nodeNumbers[i],
                    nodeCoords[i][0],
                    nodeCoords[i][1],
                    nodeCoords[i][2]);
        }
    }

    SMDS_ElemIteratorPtr srcElemIt;
    SMDS_NodeIteratorPtr srcNodeIt;
    srcElemIt = newMesh->elementsIterator();
    srcNodeIt = newMesh->nodesIterator();
    const SMDS_MeshNode* nSrc;
    int NID, EID;
    while (srcNodeIt->more()) {
        const SMDS_MeshNode* node = srcNodeIt->next();
        NID = node->GetID();
    }
    int numberNodes = -1;
    while (srcElemIt->more()) {
        const SMDS_MeshElement* elem = srcElemIt->next();
        EID = elem->GetID();
        if (elem->GetType() != requiredType) {
            continue;
        }

        if (numberNodes != elem->NbNodes()) {
            if (requiredType == 4) {
                fprintf(fptr, "\n");
                fprintf(fptr, "\n");
                fprintf(fptr, "%s", "** Volume elements\n");
                fprintf(fptr,
                        "*Element, TYPE=%s, ELSET=Evolumes\n",
                        elType3D[elem->NbNodes()].c_str());
            }
            else if (requiredType == 3) {
                fprintf(fptr, "%s", "** Face elements\n");
                fprintf(fptr,
                        "*Element, TYPE=%s, ELSET=Efaces\n",
                        elType2D[elem->NbNodes()].c_str());
            }
            numberNodes = elem->NbNodes();
        }
        SMDS_ElemIteratorPtr nIt = elem->nodesIterator();
        fprintf(fptr, "%d", EID);
        while (nIt->more()) {
            nSrc = static_cast<const SMDS_MeshNode*>(nIt->next());
            NID = nSrc->GetID();
            fprintf(fptr, ", %d", NID);
        }
        fprintf(fptr, "\n");
    }  // while print
    if (requiredType == 4) {
        fprintf(fptr, "\n");
        fprintf(fptr, "\n");
        fprintf(fptr, "%s", "** Define element set Eall\n");
        fprintf(fptr, "%s", "*ELSET, ELSET=Eall\n");
        fprintf(fptr, "%s", "Evolumes\n");
    }
    else if (requiredType == 3) {
        fprintf(fptr, "%s", "** Define element set Eall\n");
        fprintf(fptr, "%s", "*ELSET, ELSET=Eall\n");
        fprintf(fptr, "%s", "Efaces\n");
    }

    fclose(fptr);
    return;
}
}  // namespace

TaskCreateElementSet::TaskCreateElementSet(Fem::FemSetElementNodesObject* pcObject, QWidget* parent)
    : TaskBox(Gui::BitmapFactory().pixmap("FEM_CreateElementsSet"),
              tr("Elements set"),
              true,
              parent)
    , pcObject(pcObject)
    , selectionMode(none)
{
    proxy = new QWidget(this);
    ui = new Ui_TaskCreateElementSet();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);
    this->groupLayout()->addWidget(proxy);
    QObject::connect(ui->toolButton_Poly, SIGNAL(clicked()), this, SLOT(Poly()));
    QObject::connect(ui->toolButton_Restore, SIGNAL(clicked()), this, SLOT(Restore()));
    QObject::connect(ui->toolButton_Rename, SIGNAL(clicked()), this, SLOT(CopyResultsMesh()));
    // check if the Link to the FemMesh is defined
    assert(pcObject->FemMesh.getValue<Fem::FemMeshObject*>());
    MeshViewProvider =
        freecad_cast<ViewProviderFemMesh*>(Gui::Application::Instance->getViewProvider(
            pcObject->FemMesh.getValue<Fem::FemMeshObject*>()));
    assert(MeshViewProvider);

    elementTempSet = pcObject->Elements.getValues();
    std::set<long int>::iterator it;
    std::string info;
    info = "Delete the generated data in the other project: " + std::string(currentProject);
    App::Document* doc = App::GetApplication().getActiveDocument();
    newProject = doc->Label.getValue();
    if (strcmp(currentProject.c_str(), newProject.c_str()) != 0
        && (passResult + passFemMesh != 0)) {
        QMessageBox::warning(Gui::getMainWindow(),
                             //        QMessageBox::warning(Gui::MainWindow(),
                             qApp->translate("CmdFemCreateElementsSet", "Wrong selection"),
                             qApp->translate("CmdFemCreateElementsSet", info.c_str()));
        return;
    }
}

void TaskCreateElementSet::Poly(void)
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::MDIView* view = doc->getActiveView();
    if (view->isDerivedFrom<Gui::View3DInventor>()) {
        Gui::View3DInventorViewer* viewer = ((Gui::View3DInventor*)view)->getViewer();
        viewer->setEditing(true);
        viewer->startSelection(Gui::View3DInventorViewer::Clip);
        viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(),
                                 DefineElementsCallback,
                                 this);
    }
}

void TaskCreateElementSet::CopyResultsMesh(void)
{
    std::vector<Gui::SelectionSingleton::SelObj> selection =
        Gui::Selection().getSelection();  // [0];
    highLightMesh = selection[0].FeatName;
    myCopyResultsMesh(highLightMesh, actualResultMesh);
    Gui::Command::doCommand(Gui::Command::Doc, "Gui.activeDocument().resetEdit()");
}

void TaskCreateElementSet::Restore(void)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    const std::vector<App::DocumentObject*>& all = doc->getObjects();
    int number = 0, xpos = 0;
    int elList = 0;
    // put reverse here

    std::vector<string> STR;
    for (std::vector<App::DocumentObject*>::const_iterator it = all.begin(); it != all.end();
         ++it) {
        std::string objectN = all[xpos]->getNameInDocument();
        STR.push_back(objectN);
        xpos++;
    }

    // iterate through in reverse order
    for (std::vector<string>::reverse_iterator it = STR.rbegin(); it != STR.rend(); ++it) {
        std::string objectN = (*it);
        if (objectN.find(startResultMesh) != std::string::npos) {
            number++;
            myCopyResultsMesh(objectN, actualResultMesh);
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "App.ActiveDocument.removeObject(\'%s\')",
                                    objectN.c_str());
            Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        }
        else if (objectN.find(newResultMesh) != std::string::npos) {
            number++;
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "App.ActiveDocument.removeObject(\'%s\')",
                                    objectN.c_str());
            Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        }
        else if (objectN.find(actualResultMesh) != std::string::npos) {}
        else if (objectN.find(newFemMesh) != std::string::npos) {
            number++;
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "App.ActiveDocument.removeObject(\'%s\')",
                                    objectN.c_str());
            Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        }
        else if (objectN.find(Fem::FemSetElementNodesObject::getElementName())
                 != std::string::npos) {
            if (elList > 0) {
                Gui::Command::doCommand(Gui::Command::Doc,
                                        "App.ActiveDocument.removeObject(\'%s\')",
                                        objectN.c_str());
                Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
            }
            else if (elList == 0) {
                elList++;
                lastName = objectN;
            }
        }
    }  // for
    if (strcmp(lastName.c_str(), "") != 0) {
        // blank last name - no action
    }
    else if (number == 0) {
        QMessageBox::warning(Gui::getMainWindow(),
                             //        QMessageBox::warning(Gui::MainWindow(),
                             qApp->translate("CmdFemCreateElementsSet", "Wrong selection"),
                             qApp->translate("CmdFemCreateElementsSet", "No Data To Restore\n"));
        return;
    }
    passResult = 0;
    passFemMesh = 0;
    currentProject = "";
    Gui::Command::doCommand(Gui::Command::Doc, "Gui.activeDocument().resetEdit()");
    return;
}  // restore

void TaskCreateElementSet::DefineElementsCallback(void* ud, SoEventCallback* n)
{
    Gui::WaitCursor wc;
    TaskCreateElementSet* taskBox = static_cast<TaskCreateElementSet*>(ud);
    // When this callback function is invoked we must in either case leave the edit mode
    Gui::View3DInventorViewer* view =
        reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());
    view->setEditing(false);
    view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), DefineElementsCallback, ud);
    n->setHandled();

    Gui::SelectionRole role;
    std::vector<SbVec2f> clPoly = view->getGLPolygon(&role);
    if (clPoly.size() < 3) {
        return;
    }
    if (clPoly.front() != clPoly.back()) {
        clPoly.push_back(clPoly.front());
    }

    SoCamera* cam = view->getSoRenderManager()->getCamera();
    SbViewVolume vv = cam->getViewVolume();
    Gui::ViewVolumeProjection proj(vv);
    Base::Polygon2d polygon;
    for (std::vector<SbVec2f>::const_iterator it = clPoly.begin(); it != clPoly.end(); ++it) {
        polygon.Add(Base::Vector2d((*it)[0], (*it)[1]));
    }

    taskBox->DefineNodes(polygon, proj, role == Gui::SelectionRole::Inner ? true : false);
}  // DefineElementsCallback

void TaskCreateElementSet::DefineNodes(const Base::Polygon2d& polygon,
                                       const Gui::ViewVolumeProjection& proj,
                                       bool inner)
{
    const SMESHDS_Mesh* srcMeshDS =
        const_cast<SMESH_Mesh*>(
            pcObject->FemMesh.getValue<Fem::FemMeshObject*>()->FemMesh.getValue().getSMesh())
            ->GetMeshDS();

    std::vector<Gui::SelectionSingleton::SelObj> selection =
        Gui::Selection().getSelection();  // [0];
    highLightMesh = selection[0].FeatName;

    meshType = "NULL";
    std::size_t found = boost::to_upper_copy(highLightMesh).find(boost::to_upper_copy(resultMesh));
    actualResultMesh = highLightMesh;
    // highLightMesh.find(myToUpper(resultMesh));

    if (found != std::string::npos) {
        meshType = "result";
    }
    else {
        meshType = "femmesh";
    }
    //        std::string lightMesh = selection[0].FeatID;

    elementTempSet.clear();
    int nElements = srcMeshDS->GetMeshInfo().NbElements();
    int nVolumes = srcMeshDS->GetMeshInfo().NbVolumes();
    int requiredType = nVolumes > 0 ? 4 : 3;  // type = 4 - 3D, type = 3 - 2D

    double cOfGX, cOfGY, cOfGZ;
    const SMDS_MeshNode* nSrc;
    int EID;
    currentProject = newProject;

    SMESHDS_Mesh* newMeshDS = new SMESHDS_Mesh(nElements, true);
    //    FemMesh femMesh = FemMesh(); // *getFemMeshPtr();
    Base::Vector3f pt2d;

    SMDS_ElemIteratorPtr srcElemIt = srcMeshDS->elementsIterator();
    SMDS_NodeIteratorPtr srcNode = srcMeshDS->nodesIterator();

    const SMDS_MeshNode* nTgt;
    std::vector<const SMDS_MeshNode*> nodes;
    int keepElement = 0, maxNode = -1;
    while (srcNode->more()) {
        const SMDS_MeshNode* aNode = srcNode->next();
        if (aNode->GetID() > maxNode) {
            maxNode = aNode->GetID();
        }
    }

    nodeNumbers = new int[maxNode + 2];
    nodeCoords = new double*[maxNode + 2];  // these are node coords
    for (int i = 0; i < maxNode + 2; i++) {
        nodeCoords[i] = new double[3];  // x,y,z
        nodeNumbers[i] = 0;
    }

    elementTempSet.insert(requiredType * -1);  // the type of elements
    int pNodes;                                // the first pnodes are used in the cofg calc

    while (srcElemIt->more()) {

        pNodes = 4;  // the first pnodes are used in the cofg calc
        const SMDS_MeshElement* elem = srcElemIt->next();
        nodes.resize(elem->NbNodes());
        EID = elem->GetID();
        if (elem->GetType() != requiredType) {
            continue;
        }
        SMDS_ElemIteratorPtr nIt = elem->nodesIterator();

        if (requiredType == 4)  // 3D
        {
            if (elem->NbNodes() == 8 || elem->NbNodes() == 20) {  // 8 or 20 node brick
                pNodes = 8;
            }
            if (elem->NbNodes() == 6 || elem->NbNodes() == 15) {  // 6 or 15 node penta
                pNodes = 8;
            }
        }
        else if (requiredType == 3)  // 2D
        {
            if (elem->NbNodes() == 3 || elem->NbNodes() == 6) {  // 3 or 6 node triangles
                pNodes = 3;
            }
        }
        cOfGX = 0.0;
        cOfGY = 0.0;
        cOfGZ = 0.0;
        for (int iN = 0; nIt->more(); ++iN) {
            nSrc = static_cast<const SMDS_MeshNode*>(nIt->next());
            nTgt = srcMeshDS->FindNode(nSrc->GetID());
            nodes[iN] = nTgt;
            newMeshDS->AddNodeWithID(nSrc->X(), nSrc->Y(), nSrc->Z(), nSrc->GetID());
            if (nodeNumbers[nSrc->GetID()] == 0) {
                nodeCoords[nSrc->GetID()][0] = nSrc->X();
                nodeCoords[nSrc->GetID()][1] = nSrc->Y();
                nodeCoords[nSrc->GetID()][2] = nSrc->Z();
                // write all nodes if result mesh
                if (strcmp(meshType.c_str(), "result") == 0) {
                    nodeNumbers[nSrc->GetID()] = nSrc->GetID();
                }
            }
            if (iN < pNodes) {
                cOfGX += nSrc->X() / pNodes;
                cOfGY += nSrc->Y() / pNodes;
                cOfGZ += nSrc->Z() / pNodes;
            }
        }  // for iN

        SMESH_MeshEditor::ElemFeatures elemFeat(elem->GetType(), elem->IsPoly());
        elemFeat.SetID(EID);

        /* add the bit to determine which elements are outside the poly */
        Base::Vector3f vec(cOfGX, cOfGY, cOfGZ);
        pt2d = proj(vec);
        if (polygon.Contains(Base::Vector2d(pt2d.x, pt2d.y)) != inner) {
            if (strcmp(meshType.c_str(), "femmesh") == 0) {
                for (long unsigned int i = 0; i < nodes.size(); i++) {
                    nodeNumbers[nodes[i]->GetID()] = nodes[i]->GetID();
                }
            }

            elementTempSet.insert(EID);
            keepElement += 1;
            if (requiredType == 4) {
                addVolumeToMesh(nodes, newMeshDS, EID);
            }
            else if (requiredType == 3) {
                addFaceToMesh(nodes, newMeshDS, EID);
            }
        }
    }  // while
    int erase;
    if (nVolumes != 0) {
        erase = nVolumes - keepElement;
    }
    else {
        erase = nElements - keepElement;
    }
    if (keepElement > 0) {
        Base::Console().warning("Number of Elements Kept: %d, Number of Elements Erased: %d\n",
                                keepElement,
                                erase);
        writeToFile(inp_file, newMeshDS, nodeNumbers, nodeCoords, maxNode, requiredType);
        generateMesh(meshType);
    }
    else {
        QMessageBox::warning(
            Gui::getMainWindow(),
            //        QMessageBox::warning(Gui::MainWindow(),
            qApp->translate("CmdFemCreateElementsSet", "Erased Elements"),
            qApp->translate("CmdFemCreateElementsSet", "All Elements Erased - no mesh generated."));
    }
    newMeshDS->Modified();
    Gui::Command::doCommand(Gui::Command::Doc, "Gui.activeDocument().resetEdit()");

}  // void TaskCreateElementSet::DefineNodes

void TaskCreateElementSet::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == none) {
        return;
    }

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        std::string subName(msg.pSubName);
        unsigned int i = 0;
        for (; i < subName.size(); i++) {
            if (msg.pSubName[i] == 'F') {
                break;
            }
        }
        int elem = atoi(subName.substr(4).c_str());
        int face = atoi(subName.substr(i + 1).c_str());
        elementTempSet.clear();
        std::set<long> tmp =
            pcObject->FemMesh.getValue<Fem::FemMeshObject*>()->FemMesh.getValue().getSurfaceNodes(
                elem,
                face);
        elementTempSet.insert(tmp.begin(), tmp.end());

        selectionMode = none;
        Gui::Selection().rmvSelectionGate();
    }
}

/*********************************************************/

TaskCreateElementSet::~TaskCreateElementSet()
{
    // delete last elementsset
    if (strcmp(lastName.c_str(), "") != 0) {
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.removeObject(\'%s\')",
                                lastName.c_str());
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        lastName = "";
    }
    delete ui;
}

#include "moc_TaskCreateElementSet.cpp"
