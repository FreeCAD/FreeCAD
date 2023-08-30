/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#endif

#include <Base/UnitsApi.h>
#include <Gui/CommandT.h>
#include <Gui/Selection.h>
#include <Gui/WaitCursor.h>

#include "ShapeFromMesh.h"
#include "ui_ShapeFromMesh.h"


using namespace PartGui;


ShapeFromMesh::ShapeFromMesh(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , ui(new Ui_ShapeFromMesh)
{
    ui->setupUi(this);
    ui->groupBoxSew->setChecked(false);

    double STD_OCC_TOLERANCE = 1e-6;

    int decimals = Base::UnitsApi::getDecimals();
    double tolerance_from_decimals = pow(10., -decimals);

    double minimal_tolerance = tolerance_from_decimals < STD_OCC_TOLERANCE ? STD_OCC_TOLERANCE : tolerance_from_decimals;
    ui->doubleSpinBox->setRange(minimal_tolerance, 10.0);
    ui->doubleSpinBox->setValue(0.1);
    ui->doubleSpinBox->setSingleStep(0.1);
    ui->doubleSpinBox->setDecimals(decimals);
}

ShapeFromMesh::~ShapeFromMesh() = default;

void ShapeFromMesh::perform()
{
    double tolerance = ui->doubleSpinBox->value();
    bool sewShape = ui->groupBoxSew->isChecked();

    Gui::WaitCursor wc;

    Base::Type meshid = Base::Type::fromName("Mesh::Feature");
    std::vector<App::DocumentObject*> meshes;
    meshes = Gui::Selection().getObjectsOfType(meshid);

    Gui::doCommandT(Gui::Command::Doc, "import Part");
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Convert mesh"));

    for (auto it : meshes) {
        App::Document* doc = it->getDocument();
        std::string mesh = it->getNameInDocument();
        std::string name = doc->getUniqueObjectName(mesh.c_str());

        Gui::cmdAppDocumentArgs(doc, "addObject('%s', '%s')", "Part::Feature",  name);
        std::string partObj = App::DocumentObjectT(doc, name).getObjectPython();
        std::string meshObj = App::DocumentObjectT(doc, mesh).getObjectPython();

        Gui::doCommandT(Gui::Command::Doc, "__shape__ = Part.Shape()");
        Gui::doCommandT(Gui::Command::Doc, "__shape__.makeShapeFromMesh(%s.Mesh.Topology, %f, %s)", meshObj, tolerance, (sewShape ? "True" : "False"));
        Gui::doCommandT(Gui::Command::Doc, partObj + ".Shape = __shape__");
        Gui::doCommandT(Gui::Command::Doc, partObj + ".purgeTouched()");
        Gui::doCommandT(Gui::Command::Doc, "del __shape__");
    }

    Gui::Command::commitCommand();
}

void ShapeFromMesh::accept()
{
    try {
        perform();
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }

    QDialog::accept();
}

#include "moc_ShapeFromMesh.cpp"
