/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#endif
// clang-format off
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
// clang-format on
#include <App/Application.h>
#include <App/Document.h>
#include <Base/Converter.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Mod/Mesh/App/MeshFeature.h>

#include "CrossSections.h"
#include "TaskCurveOnMesh.h"
#include "Tessellation.h"


using namespace std;

//===========================================================================
// MeshPart_Mesher
//===========================================================================
DEF_STD_CMD_A(CmdMeshPartMesher)

CmdMeshPartMesher::CmdMeshPartMesher()
    : Command("MeshPart_Mesher")
{
    sAppModule = "MeshPart";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Create mesh from shape...");
    sToolTipText = QT_TR_NOOP("Tessellate shape");
    sWhatsThis = "MeshPart_Mesher";
    sStatusTip = sToolTipText;
}

void CmdMeshPartMesher::activated(int)
{
    Gui::Control().showDialog(new MeshPartGui::TaskTessellation());
}

bool CmdMeshPartMesher::isActive()
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshPartTrimByPlane)

CmdMeshPartTrimByPlane::CmdMeshPartTrimByPlane()
    : Command("MeshPart_TrimByPlane")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Trim mesh with a plane");
    sToolTipText = QT_TR_NOOP("Trims a mesh with a plane");
    sStatusTip = QT_TR_NOOP("Trims a mesh with a plane");
}

void CmdMeshPartTrimByPlane::activated(int)
{
    Base::Type partType = Base::Type::fromName("Part::Plane");
    std::vector<App::DocumentObject*> plane = getSelection().getObjectsOfType(partType);
    if (plane.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             qApp->translate("MeshPart_TrimByPlane", "Select plane"),
                             qApp->translate("MeshPart_TrimByPlane",
                                             "Please select a plane at which you trim the mesh."));
        return;
    }

    QMessageBox msgBox(Gui::getMainWindow());
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle(qApp->translate("MeshPart_TrimByPlane", "Trim by plane"));
    msgBox.setText(qApp->translate("MeshPart_TrimByPlane", "Select the side you want to keep."));
    QPushButton* inner =
        msgBox.addButton(qApp->translate("MeshPart_TrimByPlane", "Below"), QMessageBox::ActionRole);
    QPushButton* outer =
        msgBox.addButton(qApp->translate("MeshPart_TrimByPlane", "Above"), QMessageBox::ActionRole);
    QPushButton* split =
        msgBox.addButton(qApp->translate("MeshPart_TrimByPlane", "Split"), QMessageBox::ActionRole);
    msgBox.addButton(QMessageBox::Cancel);
    msgBox.setDefaultButton(inner);
    msgBox.exec();
    QAbstractButton* click = msgBox.clickedButton();

    Gui::SelectionRole role;
    if (inner == click) {
        role = Gui::SelectionRole::Inner;
    }
    else if (outer == click) {
        role = Gui::SelectionRole::Outer;
    }
    else if (split == click) {
        role = Gui::SelectionRole::Split;
    }
    else {
        // abort
        return;
    }

    Base::Placement plnPlacement =
        static_cast<App::GeoFeature*>(plane.front())->Placement.getValue();

    openCommand(QT_TRANSLATE_NOOP("Command", "Trim with plane"));
    std::vector<App::DocumentObject*> docObj =
        Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (auto it : docObj) {
        Base::Vector3d normal(0, 0, 1);
        plnPlacement.getRotation().multVec(normal, normal);
        Base::Vector3d base = plnPlacement.getPosition();

        Mesh::MeshObject* mesh = static_cast<Mesh::Feature*>(it)->Mesh.startEditing();

        Base::Vector3f plnBase = Base::convertTo<Base::Vector3f>(base);
        Base::Vector3f plnNormal = Base::convertTo<Base::Vector3f>(normal);

        if (role == Gui::SelectionRole::Inner) {
            mesh->trimByPlane(plnBase, plnNormal);
            static_cast<Mesh::Feature*>(it)->Mesh.finishEditing();
        }
        else if (role == Gui::SelectionRole::Outer) {
            mesh->trimByPlane(plnBase, -plnNormal);
            static_cast<Mesh::Feature*>(it)->Mesh.finishEditing();
        }
        else if (role == Gui::SelectionRole::Split) {
            Mesh::MeshObject copy(*mesh);
            mesh->trimByPlane(plnBase, plnNormal);
            static_cast<Mesh::Feature*>(it)->Mesh.finishEditing();

            copy.trimByPlane(plnBase, -plnNormal);
            App::Document* doc = it->getDocument();
            Mesh::Feature* fea = static_cast<Mesh::Feature*>(doc->addObject("Mesh::Feature"));
            fea->Label.setValue(it->Label.getValue());
            Mesh::MeshObject* feamesh = fea->Mesh.startEditing();
            feamesh->swap(copy);
            fea->Mesh.finishEditing();
        }
        it->purgeTouched();
    }
    commitCommand();
}

bool CmdMeshPartTrimByPlane::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) != 1) {
        return false;
    }

    return true;
}

//===========================================================================
// MeshPart_Section
//===========================================================================
DEF_STD_CMD_A(CmdMeshPartSection)

CmdMeshPartSection::CmdMeshPartSection()
    : Command("MeshPart_SectionByPlane")
{
    sAppModule = "MeshPart";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Create section from mesh and plane");
    sToolTipText = QT_TR_NOOP("Section");
    sWhatsThis = "MeshPart_Section";
    sStatusTip = sToolTipText;
}

void CmdMeshPartSection::activated(int)
{
    Base::Type partType = Base::Type::fromName("Part::Plane");
    std::vector<App::DocumentObject*> plane = getSelection().getObjectsOfType(partType);
    if (plane.empty()) {
        QMessageBox::warning(
            Gui::getMainWindow(),
            qApp->translate("MeshPart_Section", "Select plane"),
            qApp->translate("MeshPart_Section",
                            "Please select a plane at which you section the mesh."));
        return;
    }

    Base::Placement plm = static_cast<App::GeoFeature*>(plane.front())->Placement.getValue();
    Base::Vector3d normal(0, 0, 1);
    plm.getRotation().multVec(normal, normal);
    Base::Vector3d base = plm.getPosition();

    openCommand(QT_TRANSLATE_NOOP("Command", "Section with plane"));
    std::vector<App::DocumentObject*> docObj =
        Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    Mesh::MeshObject::TPlane tplane;
    tplane.first = Base::convertTo<Base::Vector3f>(base);
    tplane.second = Base::convertTo<Base::Vector3f>(normal);
    std::vector<Mesh::MeshObject::TPlane> sections;
    sections.push_back(tplane);

    Py::Module partModule(PyImport_ImportModule("Part"), true);
    Py::Callable makeWire(partModule.getAttr("makePolygon"));
    Py::Module appModule(PyImport_ImportModule("FreeCAD"), true);
    Py::Callable addObject(appModule.getAttr("ActiveDocument").getAttr("addObject"));
    for (auto it : docObj) {
        const Mesh::MeshObject* mesh = static_cast<Mesh::Feature*>(it)->Mesh.getValuePtr();
        std::vector<Mesh::MeshObject::TPolylines> polylines;
        mesh->crossSections(sections, polylines);

        for (const auto& it2 : polylines) {
            for (const auto& it3 : it2) {
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

bool CmdMeshPartSection::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) != 1) {
        return false;
    }

    return true;
}

//===========================================================================
// MeshPart_CrossSections
//===========================================================================
DEF_STD_CMD_A(CmdMeshPartCrossSections)

CmdMeshPartCrossSections::CmdMeshPartCrossSections()
    : Command("MeshPart_CrossSections")
{
    sAppModule = "MeshPart";
    sGroup = QT_TR_NOOP("MeshPart");
    sMenuText = QT_TR_NOOP("Cross-sections...");
    sToolTipText = QT_TR_NOOP("Cross-sections");
    sWhatsThis = "MeshPart_CrossSections";
    sStatusTip = sToolTipText;
    // sPixmap       = "MeshPart_CrossSections";
}

void CmdMeshPartCrossSections::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg) {
        std::vector<App::DocumentObject*> obj =
            Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
        Base::BoundBox3d bbox;
        for (auto it : obj) {
            bbox.Add(static_cast<Mesh::Feature*>(it)->Mesh.getBoundingBox());
        }
        dlg = new MeshPartGui::TaskCrossSections(bbox);
    }
    Gui::Control().showDialog(dlg);
}

bool CmdMeshPartCrossSections::isActive()
{
    return (Gui::Selection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0
            && !Gui::Control().activeDialog());
}

DEF_STD_CMD_A(CmdMeshPartCurveOnMesh)

CmdMeshPartCurveOnMesh::CmdMeshPartCurveOnMesh()
    : Command("MeshPart_CurveOnMesh")
{
    sAppModule = "MeshPart";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Curve on mesh...");
    sToolTipText = QT_TR_NOOP("Creates an approximated curve on top of a mesh.\n"
                              "This command only works with a 'mesh' object.");
    sWhatsThis = "MeshPart_CurveOnMesh";
    sStatusTip = sToolTipText;
    sPixmap = "MeshPart_CurveOnMesh";
}

void CmdMeshPartCurveOnMesh::activated(int)
{
    Gui::Document* doc = getActiveGuiDocument();
    std::list<Gui::MDIView*> mdis = doc->getMDIViewsOfType(Gui::View3DInventor::getClassTypeId());
    if (mdis.empty()) {
        return;
    }

    Gui::Control().showDialog(
        new MeshPartGui::TaskCurveOnMesh(static_cast<Gui::View3DInventor*>(mdis.front())));
}

bool CmdMeshPartCurveOnMesh::isActive()
{
    if (Gui::Control().activeDialog()) {
        return false;
    }

    // Check for the selected mesh feature (all Mesh types)
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (doc && doc->countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0) {
        return true;
    }

    return false;
}


void CreateMeshPartCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdMeshPartMesher());
    rcCmdMgr.addCommand(new CmdMeshPartTrimByPlane());
    rcCmdMgr.addCommand(new CmdMeshPartSection());
    rcCmdMgr.addCommand(new CmdMeshPartCrossSections());
    rcCmdMgr.addCommand(new CmdMeshPartCurveOnMesh());
}
