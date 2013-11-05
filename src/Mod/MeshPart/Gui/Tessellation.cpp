/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <TopExp_Explorer.hxx>
#endif

#include "Tessellation.h"
#include "ui_Tessellation.h"
#include <Base/Console.h>
#include <Base/Exception.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Mod/Part/App/PartFeature.h>

using namespace MeshPartGui;

/* TRANSLATOR MeshPartGui::Tessellation */

Tessellation::Tessellation(QWidget* parent)
  : QWidget(parent), ui(new Ui_Tessellation)
{
    ui->setupUi(this);
    Gui::Command::doCommand(Gui::Command::Doc, "import Mesh, MeshPart");
    findShapes();
}

Tessellation::~Tessellation()
{
}

void Tessellation::on_checkSimpleMethod_toggled(bool on)
{
    if (!on) {
        if (ui->checkMaxEdgeLength->isChecked()) {
            ui->checkMaxEdgeLength->setEnabled(true);
            ui->spinMaxEdgeLength->setEnabled(true);
        }
    }
    else {
        ui->checkMaxEdgeLength->setEnabled(false);
        ui->spinMaxEdgeLength->setEnabled(false);
    }
}

void Tessellation::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

void Tessellation::findShapes()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc) return;
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(activeDoc);
    if (!activeGui) return;

    this->document = QString::fromAscii(activeDoc->getName());
    std::vector<Part::Feature*> objs = activeDoc->getObjectsOfType<Part::Feature>();

    double edgeLen = 0;
    bool foundSelection = false;
    for (std::vector<Part::Feature*>::iterator it = objs.begin(); it!=objs.end(); ++it) {
        const TopoDS_Shape& shape = (*it)->Shape.getValue();
        if (shape.IsNull()) continue;
        bool hasfaces = false;
        TopExp_Explorer xp(shape,TopAbs_FACE);
        while (xp.More()) {
            hasfaces = true;
            break;
        }

        if (hasfaces) {
            Base::BoundBox3d bbox = (*it)->Shape.getBoundingBox();
            edgeLen = std::max<double>(edgeLen, bbox.LengthX());
            edgeLen = std::max<double>(edgeLen, bbox.LengthY());
            edgeLen = std::max<double>(edgeLen, bbox.LengthZ());
            QString label = QString::fromUtf8((*it)->Label.getValue());
            QString name = QString::fromAscii((*it)->getNameInDocument());
            
            QTreeWidgetItem* child = new QTreeWidgetItem();
            child->setText(0, label);
            child->setToolTip(0, label);
            child->setData(0, Qt::UserRole, name);
            Gui::ViewProvider* vp = activeGui->getViewProvider(*it);
            if (vp) child->setIcon(0, vp->getIcon());
            ui->treeWidget->addTopLevelItem(child);
            if (Gui::Selection().isSelected(*it)) {
                child->setSelected(true);
                foundSelection = true;
            }
        }
    }

    ui->spinMaxEdgeLength->setValue(edgeLen/10);
    if (foundSelection)
        ui->treeWidget->hide();
}

bool Tessellation::accept()
{
    if (ui->treeWidget->selectedItems().isEmpty()) {
        QMessageBox::critical(this, windowTitle(),
            tr("Select a shape for meshing, first."));
        return false;
    }

    App::Document* activeDoc = App::GetApplication().getDocument((const char*)this->document.toAscii());
    if (!activeDoc) {
        QMessageBox::critical(this, windowTitle(),
            tr("No such document '%1'.").arg(this->document));
        return false;
    }

    try {
        QString shape, label;
        Gui::WaitCursor wc;

        bool simple = ui->checkSimpleMethod->isChecked();
        double devFace = ui->spinDeviation->value();
        if (!ui->spinDeviation->isEnabled()) {
            if (simple)
                devFace = 0.1;
            else
                devFace = 0;
        }
        double maxEdge = ui->spinMaxEdgeLength->value();
        if (!ui->spinMaxEdgeLength->isEnabled())
            maxEdge = 0;

        activeDoc->openTransaction("Meshing");
        QList<QTreeWidgetItem *> items = ui->treeWidget->selectedItems();
        std::vector<Part::Feature*> shapes = Gui::Selection().getObjectsOfType<Part::Feature>();
        for (QList<QTreeWidgetItem *>::iterator it = items.begin(); it != items.end(); ++it) {
            shape = (*it)->data(0, Qt::UserRole).toString();
            label = (*it)->text(0);

            QString cmd;
            if (simple) {
                cmd = QString::fromAscii(
                    "__doc__=FreeCAD.getDocument(\"%1\")\n"
                    "__mesh__=__doc__.addObject(\"Mesh::Feature\",\"Mesh\")\n"
                    "__mesh__.Mesh=Mesh.Mesh(__doc__.getObject(\"%2\").Shape.tessellate(%3))\n"
                    "__mesh__.Label=\"%4 (Meshed)\"\n"
                    "del __doc__, __mesh__\n")
                    .arg(this->document)
                    .arg(shape)
                    .arg(devFace)
                    .arg(label);
            }
            else {
                cmd = QString::fromAscii(
                    "__doc__=FreeCAD.getDocument(\"%1\")\n"
                    "__mesh__=__doc__.addObject(\"Mesh::Feature\",\"Mesh\")\n"
                    "__mesh__.Mesh=MeshPart.meshFromShape(__doc__.getObject(\"%2\").Shape,%3,0,0,%4)\n"
                    "__mesh__.Label=\"%5 (Meshed)\"\n"
                    "__mesh__.ViewObject.CreaseAngle=25.0\n"
                    "del __doc__, __mesh__\n")
                    .arg(this->document)
                    .arg(shape)
                    .arg(maxEdge)
                    .arg(devFace)
                    .arg(label);
            }
            Gui::Command::doCommand(Gui::Command::Doc, (const char*)cmd.toAscii());
        }
        activeDoc->commitTransaction();
    }
    catch (const Base::Exception& e) {
        Base::Console().Error(e.what());
    }

    return true;
}

// ---------------------------------------

TaskTessellation::TaskTessellation()
{
    widget = new Tessellation();
    Gui::TaskView::TaskBox* taskbox = new Gui::TaskView::TaskBox(
        QPixmap()/*Gui::BitmapFactory().pixmap("MeshPart_Mesher")*/,
        widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskTessellation::~TaskTessellation()
{
    // automatically deleted in the sub-class
}

void TaskTessellation::open()
{
}

void TaskTessellation::clicked(int)
{
}

bool TaskTessellation::accept()
{
    return widget->accept();
}

bool TaskTessellation::reject()
{
    return true;
}

#include "moc_Tessellation.cpp"
