/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <QCheckBox>
#include <QMessageBox>
#endif

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Part/App/PartFeature.h>

#include "TaskDialog.h"


using namespace DrawingGui;

/* TRANSLATOR DrawingGui::TaskProjection */

TaskProjection::TaskProjection()
{
    QString texts[10] = {tr("Visible sharp edges"),
                         tr("Visible smooth edges"),
                         tr("Visible sewn edges"),
                         tr("Visible outline edges"),
                         tr("Visible isoparameters"),
                         tr("Hidden sharp edges"),
                         tr("Hidden smooth edges"),
                         tr("Hidden sewn edges"),
                         tr("Hidden outline edges"),
                         tr("Hidden isoparameters")};
    widget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout;

    for (int i = 0; i < 10; i++) {
        QCheckBox* cb = new QCheckBox();
        if (i < 5) {
            cb->setChecked(true);
        }
        cb->setText(texts[i]);
        mainLayout->addWidget(cb);
        boxes.push_back(cb);
    }

    widget->setLayout(mainLayout);

    taskbox = new Gui::TaskView::TaskBox(QPixmap(), tr("Project shapes"), false, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskProjection::~TaskProjection()
{
    // automatically deleted in the sub-class
}

bool TaskProjection::accept()
{
    Gui::Document* document = Gui::Application::Instance->activeDocument();
    if (!document) {
        QMessageBox::warning(widget,
                             tr("No active document"),
                             tr("There is currently no active document to complete the operation"));
        return true;
    }
    std::list<Gui::MDIView*> mdis =
        document->getMDIViewsOfType(Gui::View3DInventor::getClassTypeId());
    if (mdis.empty()) {
        QMessageBox::warning(widget,
                             tr("No active view"),
                             tr("There is currently no active view to complete the operation"));
        return false;
    }

    Gui::View3DInventorViewer* viewer =
        static_cast<Gui::View3DInventor*>(mdis.front())->getViewer();
    SbVec3f pnt, dir;
    viewer->getNearPlane(pnt, dir);
    float x = 0, y = 1, z = 1;
    dir.getValue(x, y, z);

    std::vector<Part::Feature*> shapes = Gui::Selection().getObjectsOfType<Part::Feature>();
    Gui::Command::openCommand("Project shape");
    Gui::Command::addModule(Gui::Command::Doc, "Drawing");
    for (std::vector<Part::Feature*>::iterator it = shapes.begin(); it != shapes.end(); ++it) {
        const char* object = (*it)->getNameInDocument();
        Gui::Command::doCommand(
            Gui::Command::Doc,
            "FreeCAD.ActiveDocument.addObject('Drawing::FeatureProjection','%s_proj')",
            object);
        Gui::Command::doCommand(
            Gui::Command::Doc,
            "FreeCAD.ActiveDocument.ActiveObject.Direction=FreeCAD.Vector(%f,%f,%f)",
            x,
            y,
            z);
        Gui::Command::doCommand(
            Gui::Command::Doc,
            "FreeCAD.ActiveDocument.ActiveObject.Source=FreeCAD.ActiveDocument.%s",
            object);
        Gui::Command::doCommand(Gui::Command::Doc,
                                "FreeCAD.ActiveDocument.ActiveObject.VCompound=%s",
                                (boxes[0]->isChecked() ? "True" : "False"));
        Gui::Command::doCommand(Gui::Command::Doc,
                                "FreeCAD.ActiveDocument.ActiveObject.Rg1LineVCompound=%s",
                                (boxes[1]->isChecked() ? "True" : "False"));
        Gui::Command::doCommand(Gui::Command::Doc,
                                "FreeCAD.ActiveDocument.ActiveObject.RgNLineVCompound=%s",
                                (boxes[2]->isChecked() ? "True" : "False"));
        Gui::Command::doCommand(Gui::Command::Doc,
                                "FreeCAD.ActiveDocument.ActiveObject.OutLineVCompound=%s",
                                (boxes[3]->isChecked() ? "True" : "False"));
        Gui::Command::doCommand(Gui::Command::Doc,
                                "FreeCAD.ActiveDocument.ActiveObject.IsoLineVCompound=%s",
                                (boxes[4]->isChecked() ? "True" : "False"));
        Gui::Command::doCommand(Gui::Command::Doc,
                                "FreeCAD.ActiveDocument.ActiveObject.HCompound=%s",
                                (boxes[5]->isChecked() ? "True" : "False"));
        Gui::Command::doCommand(Gui::Command::Doc,
                                "FreeCAD.ActiveDocument.ActiveObject.Rg1LineHCompound=%s",
                                (boxes[6]->isChecked() ? "True" : "False"));
        Gui::Command::doCommand(Gui::Command::Doc,
                                "FreeCAD.ActiveDocument.ActiveObject.RgNLineHCompound=%s",
                                (boxes[7]->isChecked() ? "True" : "False"));
        Gui::Command::doCommand(Gui::Command::Doc,
                                "FreeCAD.ActiveDocument.ActiveObject.OutLineHCompound=%s",
                                (boxes[8]->isChecked() ? "True" : "False"));
        Gui::Command::doCommand(Gui::Command::Doc,
                                "FreeCAD.ActiveDocument.ActiveObject.IsoLineHCompound=%s",
                                (boxes[9]->isChecked() ? "True" : "False"));
    }
    Gui::Command::updateActive();
    Gui::Command::commitCommand();
    return true;
}

#include "moc_TaskDialog.cpp"
