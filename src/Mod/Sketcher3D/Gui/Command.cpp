// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include <memory>

#include <App/Document.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>

#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>

#include <Mod/Part/App/TopoShape.h>
#include <Mod/Sketcher3D/App/Constraint3D.h>
#include <Mod/Sketcher3D/App/GeoEnum3D.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "DrawSketchHandlerLine3D.h"
#include "DrawSketchHandlerPoint3D.h"
#include "Utils.h"
#include "ViewProviderSketch3D.h"


namespace Sketcher3DGui
{

DEF_STD_CMD_A(CmdSketcher3DCreateSketch)

CmdSketcher3DCreateSketch::CmdSketcher3DCreateSketch()
    : Command("Sketcher3D_CreateSketch")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Create 3D sketch");
    sToolTipText = QT_TR_NOOP("Creates a new 3D sketch");
    sWhatsThis = "Sketcher3D_CreateSketch";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_NewSketch";
}

void CmdSketcher3DCreateSketch::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    std::string FeatName = getUniqueObjectName("Sketch3D");

    openCommand(QT_TRANSLATE_NOOP("Command", "Create a new 3D sketch"));
    doCommand(
        Doc,
        "App.activeDocument().addObject('Sketcher3D::Sketch3DObject', '%s')",
        FeatName.c_str()
    );
    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
    commitCommand();
}

bool CmdSketcher3DCreateSketch::isActive()
{
    return hasActiveDocument();
}

// Editmode tool

DEF_STD_CMD_A(CmdSketcher3DCreatePoint)

CmdSketcher3DCreatePoint::CmdSketcher3DCreatePoint()
    : Command("Sketcher3D_CreatePoint")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Point");
    sToolTipText = QT_TR_NOOP("Create a 3D point");
    sWhatsThis = "Sketcher3D_CreatePoint";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreatePoint";
    eType = ForEdit;
}

void CmdSketcher3DCreatePoint::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ViewProviderSketch3D* vp = getActiveSketch3DVP();
    if (!vp) {
        return;
    }
    vp->activateHandler(std::make_unique<DrawSketchHandlerPoint3D>());
}

bool CmdSketcher3DCreatePoint::isActive()
{
    return isSketch3DInEdit();
}

DEF_STD_CMD_A(CmdSketcher3DCreateLine)

CmdSketcher3DCreateLine::CmdSketcher3DCreateLine()
    : Command("Sketcher3D_CreateLine")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Line");
    sToolTipText = QT_TR_NOOP("Create a 3D line segment");
    sWhatsThis = "Sketcher3D_CreateLine";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateLine";
    eType = ForEdit;
}

void CmdSketcher3DCreateLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ViewProviderSketch3D* vp = getActiveSketch3DVP();
    if (!vp) {
        return;
    }
    vp->activateHandler(std::make_unique<DrawSketchHandlerLine3D>());
}

bool CmdSketcher3DCreateLine::isActive()
{
    return isSketch3DInEdit();
}


DEF_STD_CMD_A(CmdSketcher3DCreatePolyline)

CmdSketcher3DCreatePolyline::CmdSketcher3DCreatePolyline()
    : Command("Sketcher3D_CreatePolyline")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Polyline");
    sToolTipText = QT_TR_NOOP("Create a 3D polyline");
    sWhatsThis = "Sketcher3D_CreatePolyline";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreatePolyline";
    eType = ForEdit;
}

void CmdSketcher3DCreatePolyline::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // stub
}

bool CmdSketcher3DCreatePolyline::isActive()
{
    return isSketch3DInEdit();
}

// ---------------------------------------------------------------------------
// Constraints
// ---------------------------------------------------------------------------

DEF_STD_CMD_A(CmdSketcher3DConstrainCoincident)

CmdSketcher3DConstrainCoincident::CmdSketcher3DConstrainCoincident()
    : Command("Sketcher3D_ConstrainCoincident")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Constrain coincident");
    sToolTipText = QT_TR_NOOP("Force two 3D points to share the same location");
    sWhatsThis = "Sketcher3D_ConstrainCoincident";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_PointOnPoint";
    eType = ForEdit;
}

void CmdSketcher3DConstrainCoincident::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    ViewProviderSketch3D* vp = getActiveSketch3DVP();
    if (!vp) {
        return;
    }
    Sketcher3D::Sketch3DObject* sketch = vp->getSketch3DObject();
    if (!sketch) {
        return;
    }

    // Collect selected vertex refs.
    std::vector<Sketcher3D::GeoElementId3D> refs;
    const auto sels = Gui::Selection().getSelectionEx(
        nullptr,
        Sketcher3D::Sketch3DObject::getClassTypeId()
    );
    const Part::TopoShape& shape = sketch->Shape.getShape();
    for (const auto& s : sels) {
        if (s.getObject() != sketch) {
            continue;
        }
        for (const std::string& subname : s.getSubNames()) {
            TopoDS_Shape sub;
            try {
                sub = shape.getSubShape(subname.c_str(), /*silent=*/true);
            }
            catch (const Standard_Failure&) {
                continue;
            }
            if (sub.IsNull() || sub.ShapeType() != TopAbs_VERTEX) {
                continue;
            }
            auto id = sketch->resolvePickedVertex(TopoDS::Vertex(sub));
            if (id.isValid()) {
                refs.push_back(id);
            }
        }
    }

    if (refs.size() != 2) {
        Base::Console().warning(
            "Sketcher3D: select exactly two 3D sketch points for Coincident.\n"
        );
        return;
    }
    if (refs[0] == refs[1]) {
        Base::Console().warning("Sketcher3D: Coincident needs two distinct points.\n");
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Constrain coincident"));
    Sketcher3D::Constraint3D c;
    c.Type = Sketcher3D::Constraint3D::Coincident3D;
    c.setElements({refs[0], refs[1]});
    sketch->addConstraint(c);
    sketch->recomputeFeature();
    commitCommand();

    Gui::Selection().clearSelection();
}

bool CmdSketcher3DConstrainCoincident::isActive()
{
    return isSketch3DInEdit();
}


}  // namespace Sketcher3DGui


void CreateSketcher3DCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DCreateSketch());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DCreatePoint());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DCreateLine());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DCreatePolyline());
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DConstrainCoincident());
}
