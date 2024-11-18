/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <cfloat>
#include <qmessagebox.h>
#endif

#include <App/Document.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Base/UnitsApi.h>
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
    // Box
    ui->boxLength->setMaximum(DBL_MAX);
    ui->boxLength->setMinimum(0);
    ui->boxWidth->setMaximum(DBL_MAX);
    ui->boxWidth->setMinimum(0);
    ui->boxHeight->setMaximum(DBL_MAX);
    ui->boxHeight->setMinimum(0);
    // Cylinder
    ui->cylinderRadius->setMaximum(DBL_MAX);
    ui->cylinderRadius->setMinimum(0);
    ui->cylinderLength->setMaximum(DBL_MAX);
    ui->cylinderLength->setMinimum(0);
    ui->cylinderEdgeLength->setMaximum(DBL_MAX);
    ui->cylinderEdgeLength->setMinimum(0);
    ui->cylinderCount->setMaximum(1000);
    // Cone
    ui->coneRadius1->setMaximum(DBL_MAX);
    ui->coneRadius1->setMinimum(0);
    ui->coneRadius2->setMaximum(DBL_MAX);
    ui->coneRadius2->setMinimum(0);
    ui->coneLength->setMaximum(DBL_MAX);
    ui->coneLength->setMinimum(0);
    ui->coneEdgeLength->setMaximum(DBL_MAX);
    ui->coneEdgeLength->setMinimum(0);
    ui->coneCount->setMaximum(1000);
    // Sphere
    ui->sphereRadius->setMaximum(DBL_MAX);
    ui->sphereRadius->setMinimum(0);
    ui->sphereCount->setMaximum(1000);
    // Ellipsoid
    ui->ellipsoidRadius1->setMaximum(DBL_MAX);
    ui->ellipsoidRadius1->setMinimum(0);
    ui->ellipsoidRadius2->setMaximum(DBL_MAX);
    ui->ellipsoidRadius2->setMinimum(0);
    ui->ellipsoidCount->setMaximum(1000);
    // Torus
    ui->toroidRadius1->setMaximum(DBL_MAX);
    ui->toroidRadius1->setMinimum(0);
    ui->toroidRadius2->setMaximum(DBL_MAX);
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
        QString cmd; std::string name;
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (!doc) {
            QMessageBox::warning(this, tr("Create %1").arg(ui->comboBox1->currentText()), tr("No active document"));
            return;
        }
        if (ui->comboBox1->currentIndex() == 0) {         // cube
            name = doc->getUniqueObjectName("Cube");
            cmd = QString(QLatin1String(
                "App.ActiveDocument.addObject(\"Mesh::Cube\",\"%1\")\n"
                "App.ActiveDocument.%1.Length=%2\n"
                "App.ActiveDocument.%1.Width=%3\n"
                "App.ActiveDocument.%1.Height=%4\n"))
                .arg(QLatin1String(name.c_str()),
                     Base::UnitsApi::toNumber(ui->boxLength->value()),
                     Base::UnitsApi::toNumber(ui->boxWidth->value()),
                     Base::UnitsApi::toNumber(ui->boxHeight->value()));
        }
        else if (ui->comboBox1->currentIndex() == 1) {  // cylinder
            name = doc->getUniqueObjectName("Cylinder");
            cmd = QString(QLatin1String(
                "App.ActiveDocument.addObject(\"Mesh::Cylinder\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius=%2\n"
                "App.ActiveDocument.%1.Length=%3\n"
                "App.ActiveDocument.%1.EdgeLength=%4\n"
                "App.ActiveDocument.%1.Closed=%5\n"
                "App.ActiveDocument.%1.Sampling=%6\n"))
                .arg(QLatin1String(name.c_str()),
                     Base::UnitsApi::toNumber(ui->cylinderRadius->value()),
                     Base::UnitsApi::toNumber(ui->cylinderLength->value()),
                     Base::UnitsApi::toNumber(ui->cylinderEdgeLength->value()),
                     QLatin1String((ui->cylinderClosed->isChecked()?"True":"False")))
                .arg(ui->cylinderCount->value());
        }
        else if (ui->comboBox1->currentIndex() == 2) {  // cone
            name = doc->getUniqueObjectName("Cone");
            cmd = QString(QLatin1String(
                "App.ActiveDocument.addObject(\"Mesh::Cone\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius1=%2\n"
                "App.ActiveDocument.%1.Radius2=%3\n"
                "App.ActiveDocument.%1.Length=%4\n"
                "App.ActiveDocument.%1.EdgeLength=%5\n"
                "App.ActiveDocument.%1.Closed=%6\n"
                "App.ActiveDocument.%1.Sampling=%7\n"))
                .arg(QLatin1String(name.c_str()),
                     Base::UnitsApi::toNumber(ui->coneRadius1->value()),
                     Base::UnitsApi::toNumber(ui->coneRadius2->value()),
                     Base::UnitsApi::toNumber(ui->coneLength->value()),
                     Base::UnitsApi::toNumber(ui->coneEdgeLength->value()),
                     QLatin1String((ui->coneClosed->isChecked()?"True":"False")))
                .arg(ui->coneCount->value());
        }
        else if (ui->comboBox1->currentIndex() == 3) {  // sphere
            name = doc->getUniqueObjectName("Sphere");
            cmd = QString(QLatin1String(
                "App.ActiveDocument.addObject(\"Mesh::Sphere\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius=%2\n"
                "App.ActiveDocument.%1.Sampling=%3\n"))
                .arg(QLatin1String(name.c_str()),
                     Base::UnitsApi::toNumber(ui->sphereRadius->value()))
                .arg(ui->sphereCount->value());
        }
        else if (ui->comboBox1->currentIndex() == 4) {  // ellipsoid
            name = doc->getUniqueObjectName("Ellipsoid");
            cmd = QString(QLatin1String(
                "App.ActiveDocument.addObject(\"Mesh::Ellipsoid\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius1=%2\n"
                "App.ActiveDocument.%1.Radius2=%3\n"
                "App.ActiveDocument.%1.Sampling=%4\n"))
                .arg(QLatin1String(name.c_str()),
                     Base::UnitsApi::toNumber(ui->ellipsoidRadius1->value()),
                     Base::UnitsApi::toNumber(ui->ellipsoidRadius2->value()))
                .arg(ui->ellipsoidCount->value());
        }
        else if (ui->comboBox1->currentIndex() == 5) {  // toroid
            name = doc->getUniqueObjectName("Torus");
            cmd = QString(QLatin1String(
                "App.ActiveDocument.addObject(\"Mesh::Torus\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius1=%2\n"
                "App.ActiveDocument.%1.Radius2=%3\n"
                "App.ActiveDocument.%1.Sampling=%4\n"))
                .arg(QLatin1String(name.c_str()),
                     Base::UnitsApi::toNumber(ui->toroidRadius1->value()),
                     Base::UnitsApi::toNumber(ui->toroidRadius2->value()))
                .arg(ui->toroidCount->value());
        }

        // Execute the Python block
        QString solid = tr("Create %1").arg(ui->comboBox1->currentText());
        Gui::Application::Instance->activeDocument()->openCommand(solid.toUtf8());
        Gui::Command::doCommand(Gui::Command::Doc, (const char*)cmd.toLatin1());
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
