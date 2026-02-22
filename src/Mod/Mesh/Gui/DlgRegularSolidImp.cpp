// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include <qmessagebox.h>

#include <App/Document.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/WaitCursor.h>

#include "DlgRegularSolidImp.h"
#include "ui_DlgRegularSolid.h"


using namespace MeshGui;

/* TRANSLATOR MeshGui::DlgRegularSolidImp */

// clang-format off
DlgRegularSolidImp::DlgRegularSolidImp(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , ui(new Ui_DlgRegularSolid)
{
    ui->setupUi(this);
    connect(ui->createSolidButton, &QPushButton::clicked,
            this, &DlgRegularSolidImp::onCreateSolidButtonClicked);
    Gui::Command::doCommand(Gui::Command::Doc, "import Mesh,BuildRegularGeoms");

    // set limits
    constexpr double doubleMax = std::numeric_limits<double>::max();
    // Box
    ui->boxLength->setMaximum(doubleMax);
    ui->boxLength->setMinimum(0);
    ui->boxWidth->setMaximum(doubleMax);
    ui->boxWidth->setMinimum(0);
    ui->boxHeight->setMaximum(doubleMax);
    ui->boxHeight->setMinimum(0);
    // Cylinder
    ui->cylinderRadius->setMaximum(doubleMax);
    ui->cylinderRadius->setMinimum(0);
    ui->cylinderLength->setMaximum(doubleMax);
    ui->cylinderLength->setMinimum(0);
    ui->cylinderEdgeLength->setMaximum(doubleMax);
    ui->cylinderEdgeLength->setMinimum(0);
    ui->cylinderCount->setMaximum(1000);
    // Cone
    ui->coneRadius1->setMaximum(doubleMax);
    ui->coneRadius1->setMinimum(0);
    ui->coneRadius2->setMaximum(doubleMax);
    ui->coneRadius2->setMinimum(0);
    ui->coneLength->setMaximum(doubleMax);
    ui->coneLength->setMinimum(0);
    ui->coneEdgeLength->setMaximum(doubleMax);
    ui->coneEdgeLength->setMinimum(0);
    ui->coneCount->setMaximum(1000);
    // Sphere
    ui->sphereRadius->setMaximum(doubleMax);
    ui->sphereRadius->setMinimum(0);
    ui->sphereCount->setMaximum(1000);
    // Ellipsoid
    ui->ellipsoidRadius1->setMaximum(doubleMax);
    ui->ellipsoidRadius1->setMinimum(0);
    ui->ellipsoidRadius2->setMaximum(doubleMax);
    ui->ellipsoidRadius2->setMinimum(0);
    ui->ellipsoidCount->setMaximum(1000);
    // Torus
    ui->toroidRadius1->setMaximum(doubleMax);
    ui->toroidRadius1->setMinimum(0);
    ui->toroidRadius2->setMaximum(doubleMax);
    ui->toroidRadius2->setMinimum(0);
    ui->toroidCount->setMaximum(1000);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgRegularSolidImp::~DlgRegularSolidImp() = default;

void DlgRegularSolidImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QDialog::changeEvent(e);
}

/**
 * Builds a mesh solid from the currently active solid type.
 */
void DlgRegularSolidImp::onCreateSolidButtonClicked()
{
    try {
        Gui::WaitCursor wc;
        std::string cmd, name;
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (!doc) {
            QMessageBox::warning(this, tr("Create %1").arg(ui->comboBox1->currentText()), tr("No active document"));
            return;
        }
        switch (ui->comboBox1->currentIndex()) {
            case 0:
                name = doc->getUniqueObjectName("Cube");
                cmd = fmt::format(
                    "App.ActiveDocument.addObject(\"Mesh::Cube\",\"{0}\")\n"
                    "App.ActiveDocument.{0}.Length={1}\n"
                    "App.ActiveDocument.{0}.Width={2}\n"
                    "App.ActiveDocument.{0}.Height={3}\n", name,
                        ui->boxLength->value().toNumber(),
                        ui->boxWidth->value().toNumber(),
                        ui->boxHeight->value().toNumber());
                break;
            case 1:
                name = doc->getUniqueObjectName("Cylinder");
                cmd = fmt::format(
                    "App.ActiveDocument.addObject(\"Mesh::Cylinder\",\"{0}\")\n"
                    "App.ActiveDocument.{0}.Radius={1}\n"
                    "App.ActiveDocument.{0}.Length={2}\n"
                    "App.ActiveDocument.{0}.EdgeLength={3}\n"
                    "App.ActiveDocument.{0}.Closed={4}\n"
                    "App.ActiveDocument.{0}.Sampling={5}\n", name,
                        ui->cylinderRadius->value().toNumber(),
                        ui->cylinderLength->value().toNumber(),
                        ui->cylinderEdgeLength->value().toNumber(),
                        ui->cylinderClosed->isChecked() ? "True" : "False",
                        ui->cylinderCount->value());
                break;
            case 2:
                name = doc->getUniqueObjectName("Cone");
                cmd = fmt::format(
                    "App.ActiveDocument.addObject(\"Mesh::Cone\",\"{0}\")\n"
                    "App.ActiveDocument.{0}.Radius1={1}\n"
                    "App.ActiveDocument.{0}.Radius2={2}\n"
                    "App.ActiveDocument.{0}.Length={3}\n"
                    "App.ActiveDocument.{0}.EdgeLength={4}\n"
                    "App.ActiveDocument.{0}.Closed={5}\n"
                    "App.ActiveDocument.{0}.Sampling={6}\n", name,
                        ui->coneRadius1->value().toNumber(),
                        ui->coneRadius2->value().toNumber(),
                        ui->coneLength->value().toNumber(),
                        ui->coneEdgeLength->value().toNumber(),
                        ui->coneClosed->isChecked() ? "True" : "False",
                        ui->coneCount->value());
                break;
            case 3:
                name = doc->getUniqueObjectName("Sphere");
                cmd = fmt::format(
                    "App.ActiveDocument.addObject(\"Mesh::Sphere\",\"{0}\")\n"
                    "App.ActiveDocument.{0}.Radius={1}\n"
                    "App.ActiveDocument.{0}.Sampling={2}\n", name,
                        ui->sphereRadius->value().toNumber(),
                        ui->sphereCount->value());
                break;
            case 4:
                name = doc->getUniqueObjectName("Ellipsoid");
                cmd = fmt::format(
                    "App.ActiveDocument.addObject(\"Mesh::Ellipsoid\",\"{0}\")\n"
                    "App.ActiveDocument.{0}.Radius1={1}\n"
                    "App.ActiveDocument.{0}.Radius2={2}\n"
                    "App.ActiveDocument.{0}.Sampling={3}\n", name,
                        ui->ellipsoidRadius1->value().toNumber(),
                        ui->ellipsoidRadius2->value().toNumber(),
                        ui->ellipsoidCount->value());
                break;
            case 5:
                name = doc->getUniqueObjectName("Torus");
                cmd = fmt::format(
                    "App.ActiveDocument.addObject(\"Mesh::Torus\",\"{0}\")\n"
                    "App.ActiveDocument.{0}.Radius1={1}\n"
                    "App.ActiveDocument.{0}.Radius2={2}\n"
                    "App.ActiveDocument.{0}.Sampling={3}\n", name,
                        ui->toroidRadius1->value().toNumber(),
                        ui->toroidRadius2->value().toNumber(),
                        ui->toroidCount->value());
                break;
        }

        // Execute the Python block
        QString solid = tr("Create %1").arg(ui->comboBox1->currentText());
        Gui::Application::Instance->activeDocument()->openCommand(solid.toUtf8());
        Gui::Command::doCommand(Gui::Command::Doc, cmd.c_str());
        Gui::Application::Instance->activeDocument()->commitCommand();
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().recompute()");
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
    }
    catch (const Base::PyException& e) {
        QMessageBox::warning(this, tr("Create %1").arg(ui->comboBox1->currentText()),
            QString::fromLatin1(e.what()));
    }
}
// clang-format on

#include "moc_DlgRegularSolidImp.cpp"
