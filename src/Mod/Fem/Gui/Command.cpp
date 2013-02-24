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

#include <Mod/Fem/App/FemMeshObject.h>

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





//DEF_STD_CMD_A(CmdFemDefineNodesSet);
//
//
//void DefineNodesCallback(void * ud, SoEventCallback * n)
//{
//    // show the wait cursor because this could take quite some time
//    Gui::WaitCursor wc;
//
//    // When this callback function is invoked we must in either case leave the edit mode
//    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());
//    view->setEditing(false);
//    view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), clipMeshCallback,ud);
//    n->setHandled();
//
//    SbBool clip_inner;
//    std::vector<SbVec2f> clPoly = view->getGLPolygon(&clip_inner);
//    if (clPoly.size() < 3)
//        return;
//    if (clPoly.front() != clPoly.back())
//        clPoly.push_back(clPoly.front());
//
//    std::vector<Gui::ViewProvider*> views = view->getViewProvidersOfType(ViewProviderMesh::getClassTypeId());
//    if (!views.empty()) {
//        Gui::Application::Instance->activeDocument()->openCommand("Cut");
//        for (std::vector<Gui::ViewProvider*>::iterator it = views.begin(); it != views.end(); ++it) {
//            ViewProviderMesh* that = static_cast<ViewProviderMesh*>(*it);
//            if (that->getEditingMode() > -1) {
//                that->finishEditing();
//                that->cutMesh(clPoly, *view, clip_inner);
//            }
//        }
//
//        Gui::Application::Instance->activeDocument()->commitCommand();
//
//        view->render();
//    }
//}
//
//
//
//CmdMeshPolyCut::CmdMeshPolyCut()
//  : Command("Mesh_PolyCut")
//{
//    sAppModule    = "Mesh";
//    sGroup        = QT_TR_NOOP("Mesh");
//    sMenuText     = QT_TR_NOOP("Cut mesh");
//    sToolTipText  = QT_TR_NOOP("Cuts a mesh with a picked polygon");
//    sWhatsThis    = "Mesh_PolyCut";
//    sStatusTip    = QT_TR_NOOP("Cuts a mesh with a picked polygon");
//    sPixmap       = "mesh_cut";
//}
//
//void CmdMeshPolyCut::activated(int iMsg)
//{
//    std::vector<App::DocumentObject*> docObj = Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
//    for (std::vector<App::DocumentObject*>::iterator it = docObj.begin(); it != docObj.end(); ++it) {
//        if (it == docObj.begin()) {
//            Gui::Document* doc = getActiveGuiDocument();
//            Gui::MDIView* view = doc->getActiveView();
//            if (view->getTypeId().isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
//                Gui::View3DInventorViewer* viewer = ((Gui::View3DInventor*)view)->getViewer();
//                viewer->setEditing(true);
//                viewer->startSelection(Gui::View3DInventorViewer::Clip);
//                viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), MeshGui::ViewProviderMeshFaceSet::clipMeshCallback);
//            }
//            else {
//                return;
//            }
//        }
//
//        Gui::ViewProvider* pVP = getActiveGuiDocument()->getViewProvider(*it);
//        if (pVP->isVisible())
//            pVP->startEditing();
//    }
//}
//
//bool CmdMeshPolyCut::isActive(void)
//{
//    // Check for the selected mesh feature (all Mesh types)
//    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0)
//        return false;
//
//    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
//    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
//        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
//        return !viewer->isEditing();
//    }
//
//    return false;
//}

//--------------------------------------------------------------------------------------


void CreateFemCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdFemCreateFromShape());
}
