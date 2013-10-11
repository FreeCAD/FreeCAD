/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender@users.sourceforge.net>        *
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
# include <sstream>
# include <QRegExp>
# include <QTextStream>
# include <QMessageBox>
# include <Precision.hxx>
# include <TopoDS.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <Geom_Plane.hxx>
# include <gp_Pln.hxx>
# include <gp_Ax1.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <Geom_Line.hxx>
# include <gp_Lin.hxx>
#endif

#include "ui_TaskFemConstraintFixed.h"
#include "TaskFemConstraintFixed.h"
#include <App/Application.h>
#include <App/Document.h>
#include <App/PropertyGeo.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/Fem/App/FemConstraintFixed.h>
#include <Mod/Part/App/PartFeature.h>

#include <Base/Console.h>

using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintFixed */

TaskFemConstraintFixed::TaskFemConstraintFixed(ViewProviderFemConstraintFixed *ConstraintView,QWidget *parent)
    : TaskFemConstraint(ConstraintView, parent, "Fem_ConstraintFixed")
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskFemConstraintFixed();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // Create a context menu for the listview of the references
    QAction* action = new QAction(tr("Delete"), ui->listReferences);
    action->connect(action, SIGNAL(triggered()),
                    this, SLOT(onReferenceDeleted()));
    ui->listReferences->addAction(action);
    ui->listReferences->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(ui->buttonReference, SIGNAL(pressed()),
            this, SLOT(onButtonReference()));

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    ui->listReferences->blockSignals(true);
    ui->buttonReference->blockSignals(true);

    // Get the feature data
    Fem::ConstraintFixed* pcConstraint = static_cast<Fem::ConstraintFixed*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    // Fill data into dialog elements
    ui->listReferences->clear();
    for (std::size_t i = 0; i < Objects.size(); i++)
        ui->listReferences->addItem(makeRefText(Objects[i], SubElements[i]));
    if (Objects.size() > 0)
        ui->listReferences->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);

    ui->listReferences->blockSignals(false);
    ui->buttonReference->blockSignals(false);

    // Selection mode can be always on since there is nothing else in the UI
    onButtonReference(true);
}

void TaskFemConstraintFixed::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        // Don't allow selection in other document
        if (strcmp(msg.pDocName, ConstraintView->getObject()->getDocument()->getName()) != 0)
            return;

        if (!msg.pSubName || msg.pSubName[0] == '\0')
            return;
        std::string subName(msg.pSubName);

        if (selectionMode == selnone)
            return;

        std::vector<std::string> references(1,subName);
        Fem::ConstraintFixed* pcConstraint = static_cast<Fem::ConstraintFixed*>(ConstraintView->getObject());
        App::DocumentObject* obj = ConstraintView->getObject()->getDocument()->getObject(msg.pObjectName);
        Part::Feature* feat = static_cast<Part::Feature*>(obj);
        TopoDS_Shape ref = feat->Shape.getShape().getSubShape(subName.c_str());

        if (selectionMode == selref) {
            std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
            std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

            // Ensure we don't have mixed reference types
            if (SubElements.size() > 0) {
                if (subName.substr(0,4) != SubElements.front().substr(0,4)) {
                    QMessageBox::warning(this, tr("Selection error"), tr("Mixed shape types are not possible. Use a second constraint instead"));
                    return;
                }
            } else {
                if ((subName.substr(0,4) != "Face") && (subName.substr(0,4) != "Edge") && (subName.substr(0,6) != "Vertex")) {
                    QMessageBox::warning(this, tr("Selection error"), tr("Only faces, edges and vertices can be picked"));
                    return;
                }
            }

            // Avoid duplicates
            std::size_t pos = 0;
            for (; pos < Objects.size(); pos++)
                if (obj == Objects[pos])
                    break;

            if (pos != Objects.size())
                if (subName == SubElements[pos])
                    return;

            // add the new reference
            Objects.push_back(obj);
            SubElements.push_back(subName);
            pcConstraint->References.setValues(Objects,SubElements);
            ui->listReferences->addItem(makeRefText(obj, subName));
        }

        Gui::Selection().clearSelection();
    }
}

void TaskFemConstraintFixed::onReferenceDeleted() {
    int row = ui->listReferences->currentIndex().row();
    TaskFemConstraint::onReferenceDeleted(row);
    ui->listReferences->model()->removeRow(row);
    ui->listReferences->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
}

const std::string TaskFemConstraintFixed::getReferences() const
{
    int rows = ui->listReferences->model()->rowCount();

    std::vector<std::string> items;
    for (int r = 0; r < rows; r++)
        items.push_back(ui->listReferences->item(r)->text().toStdString());
    return TaskFemConstraint::getReferences(items);
}

TaskFemConstraintFixed::~TaskFemConstraintFixed()
{
    delete ui;
}

void TaskFemConstraintFixed::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintFixed::TaskDlgFemConstraintFixed(ViewProviderFemConstraintFixed *ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintFixed(ConstraintView);;

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

bool TaskDlgFemConstraintFixed::accept()
{
    return TaskDlgFemConstraint::accept();
}

#include "moc_TaskFemConstraintFixed.cpp"
