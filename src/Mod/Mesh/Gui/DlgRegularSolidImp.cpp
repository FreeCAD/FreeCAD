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
# include <float.h>
# include <qcheckbox.h>
# include <qcombobox.h>
# include <qmessagebox.h>
#endif

#include <Base/PyObjectBase.h>
#include <Base/Interpreter.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/PrefWidgets.h>
#include <Gui/WaitCursor.h>

#include "DlgRegularSolidImp.h"

using namespace MeshGui;

/**
 *  Constructs a MeshGui::DlgRegularSolidImp as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
MeshGui::DlgRegularSolidImp::DlgRegularSolidImp(QWidget* parent, Qt::WFlags fl)
  : QDialog( parent, fl )
{
    this->setupUi(this);
    QList<QDoubleSpinBox*> list = this->findChildren<QDoubleSpinBox*>();
    for (QList<QDoubleSpinBox*>::iterator it = list.begin(); it != list.end(); ++it)
        (*it)->setDecimals(Base::UnitsApi::getDecimals());
    Gui::Command::doCommand(Gui::Command::Doc, "import Mesh,BuildRegularGeoms");

    // set limits
    // Box
    boxLength->setMaximum(DBL_MAX);
    boxWidth->setMaximum(DBL_MAX);
    boxHeight->setMaximum(DBL_MAX);
    // Cylinder
    cylinderRadius->setMaximum(DBL_MAX);
    cylinderLength->setMaximum(DBL_MAX);
    cylinderEdgeLength->setMaximum(DBL_MAX);
    cylinderCount->setMaximum(1000);
    // Cone
    coneRadius1->setMaximum(DBL_MAX);
    coneRadius2->setMaximum(DBL_MAX);
    coneLength->setMaximum(DBL_MAX);
    coneEdgeLength->setMaximum(DBL_MAX);
    coneCount->setMaximum(1000);
    // Sphere
    sphereRadius->setMaximum(DBL_MAX);
    sphereCount->setMaximum(1000);
    // Ellipsoid
    ellipsoidRadius1->setMaximum(DBL_MAX);
    ellipsoidRadius2->setMaximum(DBL_MAX);
    ellipsoidCount->setMaximum(1000);
    // Torus
    toroidRadius1->setMaximum(DBL_MAX);
    toroidRadius2->setMaximum(DBL_MAX);
    toroidCount->setMaximum(1000);
}

/**
 *  Destroys the object and frees any allocated resources
 */
MeshGui::DlgRegularSolidImp::~DlgRegularSolidImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void MeshGui::DlgRegularSolidImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        this->retranslateUi(this);
    }
    QDialog::changeEvent(e);
}

/**
 * Builds a mesh solid from the currently active solid type.
 */
void MeshGui::DlgRegularSolidImp::on_createSolidButton_clicked()
{
    try {
        Gui::WaitCursor wc;
        QString cmd; std::string name;
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (!doc) {
            QMessageBox::warning(this, tr("Create %1").arg(comboBox1->currentText()), tr("No active document"));
            return;
        }
        if (comboBox1->currentIndex() == 0) {         // cube
            name = doc->getUniqueObjectName("Cube");
            cmd = QString(QLatin1String(
                "App.ActiveDocument.addObject(\"Mesh::Cube\",\"%1\")\n"
                "App.ActiveDocument.%1.Length=%2\n"
                "App.ActiveDocument.%1.Width=%3\n"
                "App.ActiveDocument.%1.Height=%4\n"))
                .arg(QLatin1String(name.c_str()))
                .arg(boxLength->value(),0,'f',2)
                .arg(boxWidth->value(),0,'f',2)
                .arg(boxHeight->value(),0,'f',2);
        }
        else if (comboBox1->currentIndex() == 1) {  // cylinder
            name = doc->getUniqueObjectName("Cylinder");
            cmd = QString(QLatin1String(
                "App.ActiveDocument.addObject(\"Mesh::Cylinder\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius=%2\n"
                "App.ActiveDocument.%1.Length=%3\n"
                "App.ActiveDocument.%1.EdgeLength=%4\n"
                "App.ActiveDocument.%1.Closed=%5\n"
                "App.ActiveDocument.%1.Sampling=%6\n"))
                .arg(QLatin1String(name.c_str()))
                .arg(cylinderRadius->value(),0,'f',2)
                .arg(cylinderLength->value(),0,'f',2)
                .arg(cylinderEdgeLength->value(),0,'f',2)
                .arg(QLatin1String((cylinderClosed->isChecked()?"True":"False")))
                .arg(cylinderCount->value());
        }
        else if (comboBox1->currentIndex() == 2) {  // cone
            name = doc->getUniqueObjectName("Cone");
            cmd = QString(QLatin1String(
                "App.ActiveDocument.addObject(\"Mesh::Cone\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius1=%2\n"
                "App.ActiveDocument.%1.Radius2=%3\n"
                "App.ActiveDocument.%1.Length=%4\n"
                "App.ActiveDocument.%1.EdgeLength=%5\n"
                "App.ActiveDocument.%1.Closed=%6\n"
                "App.ActiveDocument.%1.Sampling=%7\n"))
                .arg(QLatin1String(name.c_str()))
                .arg(coneRadius1->value(),0,'f',2)
                .arg(coneRadius2->value(),0,'f',2)
                .arg(coneLength->value(),0,'f',2)
                .arg(coneEdgeLength->value(),0,'f',2)
                .arg(QLatin1String((coneClosed->isChecked()?"True":"False")))
                .arg(coneCount->value());
        }
        else if (comboBox1->currentIndex() == 3) {  // sphere
            name = doc->getUniqueObjectName("Sphere");
            cmd = QString(QLatin1String(
                "App.ActiveDocument.addObject(\"Mesh::Sphere\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius=%2\n"
                "App.ActiveDocument.%1.Sampling=%3\n"))
                .arg(QLatin1String(name.c_str()))
                .arg(sphereRadius->value(),0,'f',2)
                .arg(sphereCount->value());
        }
        else if (comboBox1->currentIndex() == 4) {  // ellipsoid
            name = doc->getUniqueObjectName("Ellipsoid");
            cmd = QString(QLatin1String(
                "App.ActiveDocument.addObject(\"Mesh::Ellipsoid\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius1=%2\n"
                "App.ActiveDocument.%1.Radius2=%3\n"
                "App.ActiveDocument.%1.Sampling=%4\n"))
                .arg(QLatin1String(name.c_str()))
                .arg(ellipsoidRadius1->value(),0,'f',2)
                .arg(ellipsoidRadius2->value(),0,'f',2)
                .arg(ellipsoidCount->value());
        }
        else if (comboBox1->currentIndex() == 5) {  // toroid
            name = doc->getUniqueObjectName("Torus");
            cmd = QString(QLatin1String(
                "App.ActiveDocument.addObject(\"Mesh::Torus\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius1=%2\n"
                "App.ActiveDocument.%1.Radius2=%3\n"
                "App.ActiveDocument.%1.Sampling=%4\n"))
                .arg(QLatin1String(name.c_str()))
                .arg(toroidRadius1->value(),0,'f',2)
                .arg(toroidRadius2->value(),0,'f',2)
                .arg(toroidCount->value());
        }

        // Execute the Python block
        QString solid = tr("Create %1").arg(comboBox1->currentText());
        Gui::Application::Instance->activeDocument()->openCommand(solid.toUtf8());
        Gui::Command::doCommand(Gui::Command::Doc, (const char*)cmd.toAscii());
        Gui::Application::Instance->activeDocument()->commitCommand();
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().recompute()");
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
    }
    catch (const Base::PyException& e) {
        QMessageBox::warning(this, tr("Create %1").arg(comboBox1->currentText()),
            QString::fromLatin1(e.what()));
    }
}

// -------------------------------------------------------------

SingleDlgRegularSolidImp* SingleDlgRegularSolidImp::_instance=0;

SingleDlgRegularSolidImp* SingleDlgRegularSolidImp::instance()
{
    // not initialized?
    if(!_instance) {
        _instance = new SingleDlgRegularSolidImp( Gui::getMainWindow());
        _instance->setAttribute(Qt::WA_DeleteOnClose);
    }

    return _instance;
}

void SingleDlgRegularSolidImp::destruct ()
{
    if (_instance != 0) {
        SingleDlgRegularSolidImp *pTmp = _instance;
        _instance = 0;
        delete pTmp;
    }
}

bool SingleDlgRegularSolidImp::hasInstance()
{
    return _instance != 0;
}

/**
 *  Constructs a SingleDlgRegularSolidImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
SingleDlgRegularSolidImp::SingleDlgRegularSolidImp(QWidget* parent, Qt::WFlags fl)
  : DlgRegularSolidImp(parent, fl)
{
}

/**
 *  Destroys the object and frees any allocated resources
 */
SingleDlgRegularSolidImp::~SingleDlgRegularSolidImp()
{
    _instance = 0;
}

#include "moc_DlgRegularSolidImp.cpp"
