/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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
#include <QAction>
#include <QCoreApplication>
#include <QMenu>
#include <QMessageBox>
#endif

#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>

#include "TaskDlgMeshShapeNetgen.h"
#include "ViewProviderFemMeshShapeNetgen.h"


using namespace FemGui;

/* TRANSLATOR FemGui::ViewProviderFemMeshShapeNetgen */

PROPERTY_SOURCE(FemGui::ViewProviderFemMeshShapeNetgen, FemGui::ViewProviderFemMeshShape)


ViewProviderFemMeshShapeNetgen::ViewProviderFemMeshShapeNetgen()
{
    sPixmap = "FEM_MeshNetgenFromShape";
}

ViewProviderFemMeshShapeNetgen::~ViewProviderFemMeshShapeNetgen() = default;

void ViewProviderFemMeshShapeNetgen::setupContextMenu(QMenu* menu,
                                                      QObject* receiver,
                                                      const char* member)
{
    QAction* act = menu->addAction(QObject::tr("Meshing"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
}

bool ViewProviderFemMeshShapeNetgen::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // clear the selection (convenience)
        Gui::Selection().clearSelection();
#ifdef FCWithNetgen
        Gui::Control().showDialog(new TaskDlgMeshShapeNetgen(this));
        return true;
#else
        QMessageBox::critical(
            Gui::getMainWindow(),
            QCoreApplication::translate("FemGui::ViewProviderFemMeshShapeNetgen",
                                        "Meshing failure"),
            QCoreApplication::translate(
                "FemGui::ViewProviderFemMeshShapeNetgen",
                "The FEM module is built without NETGEN support. Meshing will not work!!!"));
        return false;
#endif
    }
    else {
        return ViewProviderDocumentObject::setEdit(ModNum);
    }
}

void ViewProviderFemMeshShapeNetgen::updateData(const App::Property* prop)
{
    ViewProviderFemMeshShape::updateData(prop);
}
