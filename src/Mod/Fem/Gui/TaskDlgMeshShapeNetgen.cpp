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


#include <QMessageBox>


#include <Base/Console.h>
#include <Base/Exception.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/WaitCursor.h>
#include <Mod/Fem/App/FemMeshShapeNetgenObject.h>

#include "TaskDlgMeshShapeNetgen.h"
#include "TaskTetParameter.h"
#include "ViewProviderFemMeshShapeNetgen.h"


using namespace FemGui;

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgMeshShapeNetgen::TaskDlgMeshShapeNetgen(FemGui::ViewProviderFemMeshShapeNetgen* obj)
    : TaskDialog()
    , param(nullptr)
    , ViewProviderFemMeshShapeNetgen(obj)
{
    FemMeshShapeNetgenObject = obj->getObject<Fem::FemMeshShapeNetgenObject>();
    if (FemMeshShapeNetgenObject) {
        param = new TaskTetParameter(FemMeshShapeNetgenObject);
        Content.push_back(param);
    }
}

TaskDlgMeshShapeNetgen::~TaskDlgMeshShapeNetgen() = default;

//==== calls from the TaskView ===============================================================


void TaskDlgMeshShapeNetgen::open()
{
    // a transaction is already open at creation time of the mesh
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = tr("Edit FEM mesh");
        Gui::Command::openCommand((const char*)msg.toUtf8());
    }
}

void TaskDlgMeshShapeNetgen::clicked(int button)
{
    try {
        if (QDialogButtonBox::Apply == button && param->touched) {
            Gui::WaitCursor wc;
            // May throw an exception which we must handle here
            FemMeshShapeNetgenObject->execute();
            FemMeshShapeNetgenObject->purgeTouched();
            param->setInfo();
            param->touched = false;
        }
    }
    catch (const Base::Exception& e) {
        Base::Console().warning("FemMeshShapeNetgenObject::execute(): %s\n", e.what());
    }
}

bool TaskDlgMeshShapeNetgen::accept()
{
    try {
        if (param->touched) {
            Gui::WaitCursor wc;
            bool ret = FemMeshShapeNetgenObject->recomputeFeature();
            if (!ret) {
                wc.restoreCursor();
                QMessageBox::critical(
                    Gui::getMainWindow(),
                    tr("Meshing failure"),
                    QString::fromStdString(FemMeshShapeNetgenObject->getStatusString())
                );
                return true;
            }
        }

        // hide the input object
        App::DocumentObject* obj = FemMeshShapeNetgenObject->Shape.getValue();
        if (obj) {
            Gui::Application::Instance->hideViewProvider(obj);
        }

        // FemSetNodesObject->Label.setValue(name->name);
        App::Document* doc = FemMeshShapeNetgenObject->getDocument();
        Gui::cmdAppDocument(doc, "recompute()");
        Gui::cmdGuiDocument(doc, "resetEdit()");
        Gui::Command::commitCommand();

        return true;
    }
    catch (const Base::Exception& e) {
        Base::Console().warning("TaskDlgMeshShapeNetgen::accept(): %s\n", e.what());
    }

    return false;
}

bool TaskDlgMeshShapeNetgen::reject()
{
    // FemSetNodesObject->execute();
    //     //Gui::Document* doc = Gui::Application::Instance->activeDocument();
    //     //if(doc)
    //     //    doc->resetEdit();
    // param->MeshViewProvider->resetHighlightNodes();
    Gui::Command::abortCommand();
    App::Document* doc = FemMeshShapeNetgenObject->getDocument();
    Gui::cmdGuiDocument(doc, "resetEdit()");
    Gui::cmdAppDocument(doc, "recompute()");

    return true;
}

void TaskDlgMeshShapeNetgen::helpRequested()
{}

#include "moc_TaskDlgMeshShapeNetgen.cpp"
