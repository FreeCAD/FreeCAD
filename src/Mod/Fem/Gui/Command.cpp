/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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
# include <QMessageBox>
#endif

#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/events/SoMouseButtonEvent.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>
#include <Gui/Selection.h>
#include <Gui/Document.h>
#include <Gui/WaitCursor.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/Utilities.h>

#include <SMESH_Mesh.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMDSAbs_ElementType.hxx>

#include <Mod/Fem/App/FemMeshObject.h>
#include <Mod/Fem/App/FemSetNodesObject.h>
#include <strstream>

#include "Hypothesis.h"

using namespace std;

DEF_STD_CMD_A(CmdFemCreateFromShape);

CmdFemCreateFromShape::CmdFemCreateFromShape()
  : Command("Fem_CreateFromShape")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Create FEM mesh");
    sToolTipText    = QT_TR_NOOP("Create FEM mesh from shape");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Fem_FemMesh";
}

void CmdFemCreateFromShape::activated(int iMsg)
{
    FemGui::TaskHypothesis* dlg = new FemGui::TaskHypothesis();
    Gui::Control().showDialog(dlg);
}

bool CmdFemCreateFromShape::isActive(void)
{
    if (Gui::Control().activeDialog())
        return false;
    Base::Type type = Base::Type::fromName("Part::Feature");
    return Gui::Selection().countObjectsOfType(type) > 0;
}


// #####################################################################################################



DEF_STD_CMD_A(CmdFemDefineNodesSet);


void DefineNodesCallback(void * ud, SoEventCallback * n)
{
    // show the wait cursor because this could take quite some time
    Gui::WaitCursor wc;

    // When this callback function is invoked we must in either case leave the edit mode
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());
    view->setEditing(false);
    view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), DefineNodesCallback,ud);
    n->setHandled();

    SbBool clip_inner;
    std::vector<SbVec2f> clPoly = view->getGLPolygon(&clip_inner);
    if (clPoly.size() < 3)
        return;
    if (clPoly.front() != clPoly.back())
        clPoly.push_back(clPoly.front());

    SoCamera* cam = view->getCamera();
    SbViewVolume vv = cam->getViewVolume();
    Gui::ViewVolumeProjection proj(vv);
    Base::Polygon2D polygon;
    for (std::vector<SbVec2f>::const_iterator it = clPoly.begin(); it != clPoly.end(); ++it)
        polygon.Add(Base::Vector2D((*it)[0],(*it)[1]));


    std::vector<App::DocumentObject*> docObj = Gui::Selection().getObjectsOfType(Fem::FemMeshObject::getClassTypeId());
    if(docObj.size() !=1)
        return;

    const SMESHDS_Mesh* data = const_cast<SMESH_Mesh*>(dynamic_cast<Fem::FemMeshObject*>(docObj[0])->FemMesh.getValue().getSMesh())->GetMeshDS();

    SMDS_NodeIteratorPtr aNodeIter = data->nodesIterator();
    Base::Vector3f pt2d;
    std::set<int> IntSet;

    for (int i=0;aNodeIter->more();) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        Base::Vector3f vec(aNode->X(),aNode->Y(),aNode->Z());
        pt2d = proj(vec);
        if (polygon.Contains(Base::Vector2D(pt2d.x, pt2d.y)) == true) 
            IntSet.insert(aNode->GetID());
    }
    
    std::stringstream  set;

    set << "[";
    for(std::set<int>::const_iterator it=IntSet.begin();it!=IntSet.end();++it)
        if(it==IntSet.begin())
            set << *it ;
        else
            set << "," << *it ;
    set << "]";

    
    Gui::Command::openCommand("Place robot");
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject('Fem::FemSetNodesObject','NodeSet')");
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.ActiveObject.Nodes = %s",set.str().c_str());
    ////Gui::Command::updateActive();
    Gui::Command::commitCommand();

    //std::vector<Gui::ViewProvider*> views = view->getViewProvidersOfType(ViewProviderMesh::getClassTypeId());
    //if (!views.empty()) {
    //    Gui::Application::Instance->activeDocument()->openCommand("Cut");
    //    for (std::vector<Gui::ViewProvider*>::iterator it = views.begin(); it != views.end(); ++it) {
    //        ViewProviderMesh* that = static_cast<ViewProviderMesh*>(*it);
    //        if (that->getEditingMode() > -1) {
    //            that->finishEditing();
    //            that->cutMesh(clPoly, *view, clip_inner);
    //        }
    //    }

    //    Gui::Application::Instance->activeDocument()->commitCommand();

    //    view->render();
    //}
}



CmdFemDefineNodesSet::CmdFemDefineNodesSet()
  : Command("Fem_DefineNodesSet")
{
    sAppModule    = "Fem";
    sGroup        = QT_TR_NOOP("Fem");
    sMenuText     = QT_TR_NOOP("Create node set by Poly");
    sToolTipText  = QT_TR_NOOP("Create node set by Poly");
    sWhatsThis    = "Create node set by Poly";
    sStatusTip    = QT_TR_NOOP("Create node set by Poly");
    sPixmap       = "Fem_FemMesh";
}

void CmdFemDefineNodesSet::activated(int iMsg)
{
    std::vector<App::DocumentObject*> docObj = Gui::Selection().getObjectsOfType(Fem::FemMeshObject::getClassTypeId());

    for (std::vector<App::DocumentObject*>::iterator it = docObj.begin(); it != docObj.end(); ++it) {
        if (it == docObj.begin()) {
            Gui::Document* doc = getActiveGuiDocument();
            Gui::MDIView* view = doc->getActiveView();
            if (view->getTypeId().isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
                Gui::View3DInventorViewer* viewer = ((Gui::View3DInventor*)view)->getViewer();
                viewer->setEditing(true);
                viewer->startSelection(Gui::View3DInventorViewer::Clip);
                viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), DefineNodesCallback);
            }
            else {
                return;
            }
        }

        //Gui::ViewProvider* pVP = getActiveGuiDocument()->getViewProvider(*it);
        //if (pVP->isVisible())
        //    pVP->startEditing();
    }
}

bool CmdFemDefineNodesSet::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Fem::FemMeshObject::getClassTypeId()) != 1)
        return false;

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}


// #####################################################################################################

DEF_STD_CMD_A(CmdFemCreateNodesSet);

CmdFemCreateNodesSet::CmdFemCreateNodesSet()
	:Command("Fem_CreateNodesSet")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Define/create a nodes set...");
    sToolTipText    = QT_TR_NOOP("Define/create a nodes set...");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Fem_FemMesh";

}


void CmdFemCreateNodesSet::activated(int iMsg)
{
     
    Gui::SelectionFilter ObjectFilter("SELECT Fem::FemSetNodesObject COUNT 1");
    Gui::SelectionFilter FemMeshFilter  ("SELECT Fem::FemMeshObject COUNT 1");

    if (ObjectFilter.match()) {
        Fem::FemSetNodesObject *NodesObj = static_cast<Fem::FemSetNodesObject*>(ObjectFilter.Result[0][0].getObject());
        openCommand("Edit nodes-set");
        doCommand(Gui,"Gui.activeDocument().setEdit('%s')",NodesObj->getNameInDocument());
    }else if (FemMeshFilter.match()) {
        Fem::FemMeshObject *MeshObj = static_cast<Fem::FemMeshObject*>(FemMeshFilter.Result[0][0].getObject());

        std::string FeatName = getUniqueObjectName("NodesSet");

        openCommand("Create a new nodes-set");
        doCommand(Doc,"App.activeDocument().addObject('Fem::FemSetNodesObject','%s')",FeatName.c_str());
        doCommand(Gui,"App.activeDocument().%s.FemMesh = App.activeDocument().%s",FeatName.c_str(),MeshObj->getNameInDocument());
        doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());

    }
}

bool CmdFemCreateNodesSet::isActive(void)
{
    return true;
}

//--------------------------------------------------------------------------------------


void CreateFemCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdFemCreateFromShape());
    rcCmdMgr.addCommand(new CmdFemCreateNodesSet());
    rcCmdMgr.addCommand(new CmdFemDefineNodesSet());
}
