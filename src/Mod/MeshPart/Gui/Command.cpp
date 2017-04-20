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
# include <QApplication>
# include <QMessageBox>
#endif

#include <Mod/Mesh/App/MeshFeature.h>

#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include "Tessellation.h"
#include "TaskCurveOnMesh.h"

using namespace std;

//===========================================================================
// MeshPart_Mesher
//===========================================================================
DEF_STD_CMD_A(CmdMeshPartMesher)

CmdMeshPartMesher::CmdMeshPartMesher()
  : Command("MeshPart_Mesher")
{
    sAppModule    = "MeshPart";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Create mesh from shape...");
    sToolTipText  = QT_TR_NOOP("Tessellate shape");
    sWhatsThis    = "MeshPart_Mesher";
    sStatusTip    = sToolTipText;
}

void CmdMeshPartMesher::activated(int)
{
    Gui::Control().showDialog(new MeshPartGui::TaskTessellation());
}

bool CmdMeshPartMesher::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshPartTrimByPlane)

CmdMeshPartTrimByPlane::CmdMeshPartTrimByPlane()
  : Command("MeshPart_TrimByPlane")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Trim mesh with a plane");
    sToolTipText  = QT_TR_NOOP("Trims a mesh with a plane");
    sStatusTip    = QT_TR_NOOP("Trims a mesh with a plane");
}

void CmdMeshPartTrimByPlane::activated(int)
{
    Base::Type partType = Base::Type::fromName("Part::Plane");
    std::vector<App::DocumentObject*> plane = getSelection().getObjectsOfType(partType);
    if (plane.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("MeshPart_TrimByPlane", "Select plane"),
            qApp->translate("MeshPart_TrimByPlane", "Please select a plane at which you trim the mesh."));
        return;
    }

    Base::Placement plm = static_cast<App::GeoFeature*>(plane.front())->Placement.getValue();
    Base::Vector3d normal(0,0,1);
    plm.getRotation().multVec(normal, normal);
    Base::Vector3d up(-1,0,0);
    plm.getRotation().multVec(up, up);
    Base::Vector3d view(0,1,0);
    plm.getRotation().multVec(view, view);

    Base::Vector3d base = plm.getPosition();
    Base::Rotation rot(Base::Vector3d(0,0,1), view);
    Base::Matrix4D mat;
    rot.getValue(mat);
    Base::ViewProjMatrix proj(mat);

    openCommand("Trim with plane");
    std::vector<App::DocumentObject*> docObj = Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = docObj.begin(); it != docObj.end(); ++it) {
        Mesh::MeshObject* mesh = static_cast<Mesh::Feature*>(*it)->Mesh.startEditing();
        Base::BoundBox3d bbox = mesh->getBoundBox();
        double len = bbox.CalcDiagonalLength();
        // project center of bbox onto plane and use this as base point
        Base::Vector3d cnt = bbox.GetCenter();
        double dist = (cnt-base)*normal;
        base = cnt - normal * dist;

        Base::Vector3d p1 = base + up * len;
        Base::Vector3d p2 = base - up * len;
        Base::Vector3d p3 = p2 + normal * len;
        Base::Vector3d p4 = p1 + normal * len;
        p1 = mat * p1;
        p2 = mat * p2;
        p3 = mat * p3;
        p4 = mat * p4;

        Base::Polygon2d polygon2d;
        polygon2d.Add(Base::Vector2d(p1.x, p1.y));
        polygon2d.Add(Base::Vector2d(p2.x, p2.y));
        polygon2d.Add(Base::Vector2d(p3.x, p3.y));
        polygon2d.Add(Base::Vector2d(p4.x, p4.y));

        Mesh::MeshObject::CutType type = Mesh::MeshObject::INNER;
        mesh->trim(polygon2d, proj, type);
        static_cast<Mesh::Feature*>(*it)->Mesh.finishEditing();
        (*it)->purgeTouched();
    }
    commitCommand();
}

bool CmdMeshPartTrimByPlane::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) != 1)
        return false;

    return true;
}

//===========================================================================
// MeshPart_Section
//===========================================================================
DEF_STD_CMD_A(CmdMeshPartSection)

CmdMeshPartSection::CmdMeshPartSection()
  : Command("MeshPart_SectionByPlane")
{
    sAppModule    = "MeshPart";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Create section from mesh and plane");
    sToolTipText  = QT_TR_NOOP("Section");
    sWhatsThis    = "MeshPart_Section";
    sStatusTip    = sToolTipText;
}

void CmdMeshPartSection::activated(int)
{
    Base::Type partType = Base::Type::fromName("Part::Plane");
    std::vector<App::DocumentObject*> plane = getSelection().getObjectsOfType(partType);
    if (plane.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("MeshPart_Section", "Select plane"),
            qApp->translate("MeshPart_Section", "Please select a plane at which you section the mesh."));
        return;
    }

    Base::Placement plm = static_cast<App::GeoFeature*>(plane.front())->Placement.getValue();
    Base::Vector3d normal(0,0,1);
    plm.getRotation().multVec(normal, normal);
    Base::Vector3d base = plm.getPosition();

    openCommand("Section with plane");
    std::vector<App::DocumentObject*> docObj = Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    Mesh::MeshObject::TPlane tplane;
    tplane.first = Base::convertTo<Base::Vector3f>(base);
    tplane.second = Base::convertTo<Base::Vector3f>(normal);
    std::vector<Mesh::MeshObject::TPlane> sections;
    sections.push_back(tplane);

    Py::Module partModule(PyImport_ImportModule("Part"), true);
    Py::Callable makeWire(partModule.getAttr("makePolygon"));
    Py::Module appModule(PyImport_ImportModule("FreeCAD"), true);
    Py::Callable addObject(appModule.getAttr("ActiveDocument").getAttr("addObject"));
    for (std::vector<App::DocumentObject*>::iterator it = docObj.begin(); it != docObj.end(); ++it) {
        const Mesh::MeshObject* mesh = static_cast<Mesh::Feature*>(*it)->Mesh.getValuePtr();
        std::vector<Mesh::MeshObject::TPolylines> polylines;
        mesh->crossSections(sections, polylines);

        for (auto it2 : polylines) {
            for (auto it3 : it2) {
                Py::Tuple arg(1);
                Py::List list;
                for (auto it4 : it3) {
                    Py::Tuple pnt(3);
                    pnt.setItem(0, Py::Float(it4.x));
                    pnt.setItem(1, Py::Float(it4.y));
                    pnt.setItem(2, Py::Float(it4.z));
                    list.append(pnt);
                }
                arg.setItem(0, list);
                Py::Object wire = makeWire.apply(arg);

                Py::Tuple arg2(2);
                arg2.setItem(0, Py::String("Part::Feature"));
                arg2.setItem(1, Py::String("Section"));
                Py::Object obj = addObject.apply(arg2);
                obj.setAttr("Shape", wire);
            }
        }
    }
    updateActive();
    commitCommand();
}

bool CmdMeshPartSection::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) != 1)
        return false;

    return true;
}

DEF_STD_CMD_A(CmdMeshPartCurveOnMesh)

CmdMeshPartCurveOnMesh::CmdMeshPartCurveOnMesh()
  : Command("MeshPart_CurveOnMesh")
{
    sAppModule    = "MeshPart";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Curve on mesh...");
    sToolTipText  = QT_TR_NOOP("Curve on mesh");
    sWhatsThis    = "MeshPart_CurveOnMesh";
    sStatusTip    = sToolTipText;
}

void CmdMeshPartCurveOnMesh::activated(int)
{
    Gui::Document* doc = getActiveGuiDocument();
    std::list<Gui::MDIView*> mdis = doc->getMDIViewsOfType(Gui::View3DInventor::getClassTypeId());
    if (mdis.empty()) {
        return;
    }

    Gui::Control().showDialog(new MeshPartGui::TaskCurveOnMesh(static_cast<Gui::View3DInventor*>(mdis.front())));
}

bool CmdMeshPartCurveOnMesh::isActive(void)
{
    if (Gui::Control().activeDialog())
        return false;

    // Check for the selected mesh feature (all Mesh types)
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (doc && doc->countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0)
        return true;

    return false;
}


void CreateMeshPartCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdMeshPartMesher());
    rcCmdMgr.addCommand(new CmdMeshPartTrimByPlane());
    rcCmdMgr.addCommand(new CmdMeshPartSection());
    rcCmdMgr.addCommand(new CmdMeshPartCurveOnMesh());
}
