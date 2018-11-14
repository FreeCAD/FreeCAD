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
# include <QKeyEvent>
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

#include "ui_TaskFemConstraint.h"
#include "TaskFemConstraint.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/Fem/App/FemConstraint.h>
#include <Mod/Part/App/PartFeature.h>

#include <Base/Console.h>

# include <boost/lexical_cast.hpp> //OvG conversion between string and int etc.

using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraint */

TaskFemConstraint::TaskFemConstraint(ViewProviderFemConstraint *ConstraintView,QWidget *parent,const char* pixmapname)
    : TaskBox(Gui::BitmapFactory().pixmap(pixmapname),tr("FEM constraint parameters"),true, parent)
    , proxy(nullptr)
    , ConstraintView(ConstraintView)
    , buttonBox(nullptr)
    , okButton(nullptr)
    , cancelButton(nullptr)
{
    selectionMode = selref;

    // Setup the dialog inside the Shaft Wizard dialog
    if ((ConstraintView->wizardWidget != NULL) && (ConstraintView->wizardSubLayout != NULL)) {
        // Hide the shaft wizard table widget to make more space
        ConstraintView->wizardSubLayout->itemAt(0)->widget()->hide();
        QGridLayout* buttons = ConstraintView->wizardSubLayout->findChild<QGridLayout*>();
        for (int b = 0; b < buttons->count(); b++)
            buttons->itemAt(b)->widget()->hide();

        // Show this dialog for the FEM constraint
        ConstraintView->wizardWidget->addWidget(this);

        // Add buttons to finish editing the constraint without closing the shaft wizard dialog
        okButton = new QPushButton(QObject::tr("Ok"));
        cancelButton = new QPushButton(QObject::tr("Cancel"));
        buttonBox = new QDialogButtonBox();
        buttonBox->addButton(okButton, QDialogButtonBox::AcceptRole);
        buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);
        QObject::connect(okButton, SIGNAL(clicked()), this, SLOT(onButtonWizOk()));
        QObject::connect(cancelButton, SIGNAL(clicked()), this, SLOT(onButtonWizCancel()));
        ConstraintView->wizardWidget->addWidget(buttonBox);
    }
}

void TaskFemConstraint::keyPressEvent(QKeyEvent *ke)
{
    if ((ConstraintView->wizardWidget != NULL) && (ConstraintView->wizardSubLayout != NULL))
        // Prevent <Enter> from closing this dialog AND the shaft wizard dialog
        // TODO: This should trigger an update in the shaft wizard but its difficult to access a python dialog from here...
        if (ke->key() == Qt::Key_Return)
            return;

    TaskBox::keyPressEvent(ke);
}

const std::string TaskFemConstraint::getReferences(const std::vector<std::string>& items) const
{
    std::string result;
    for (std::vector<std::string>::const_iterator i = items.begin(); i != items.end(); i++) {
        int pos = i->find_last_of(":");
        std::string objStr = "App.ActiveDocument." + i->substr(0, pos);
        std::string refStr = "\"" + i->substr(pos+1) + "\"";
        result = result + (i != items.begin() ? ", " : "") + "(" + objStr + "," + refStr + ")";
    }

    return result;
}

const std::string TaskFemConstraint::getScale() const //OvG: Return pre-calculated scale for constraint display
{
    std::string result;
    Fem::Constraint* pcConstraint = static_cast<Fem::Constraint*>(ConstraintView->getObject());
    result = boost::lexical_cast<std::string>(pcConstraint->Scale.getValue());
    return result;
}

void TaskFemConstraint::onReferenceDeleted(const int row) {
    Fem::Constraint* pcConstraint = static_cast<Fem::Constraint*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    Objects.erase(Objects.begin() + row);
    SubElements.erase(SubElements.begin() + row);
    pcConstraint->References.setValues(Objects, SubElements);
}

void TaskFemConstraint::onButtonReference(const bool pressed) {
    if (pressed)
        selectionMode = selref;
    else
        selectionMode = selnone;
    Gui::Selection().clearSelection();
}

void TaskFemConstraint::onButtonWizOk()
{
    // Remove dialog elements
    buttonBox->removeButton(okButton);
    delete okButton;
    buttonBox->removeButton(cancelButton);
    delete cancelButton;
    ConstraintView->wizardWidget->removeWidget(buttonBox);
    delete buttonBox;
    ConstraintView->wizardWidget->removeWidget(this);

    // Show the wizard shaft dialog again
    ConstraintView->wizardSubLayout->itemAt(0)->widget()->show();
    QGridLayout* buttons = ConstraintView->wizardSubLayout->findChild<QGridLayout*>();
    for (int b = 0; b < buttons->count(); b++)
        buttons->itemAt(b)->widget()->show();

    Gui::Application::Instance->activeDocument()->resetEdit(); // Reaches ViewProviderFemConstraint::unsetEdit() eventually
}

void TaskFemConstraint::onButtonWizCancel()
{
    Fem::Constraint* pcConstraint = static_cast<Fem::Constraint*>(ConstraintView->getObject());
    if (pcConstraint != NULL)
        pcConstraint->getDocument()->removeObject(pcConstraint->getNameInDocument());
    onButtonWizOk();
}

const QString TaskFemConstraint::makeRefText(const App::DocumentObject* obj, const std::string& subName) const
{
    return QString::fromUtf8((std::string(obj->getNameInDocument()) + ":" + subName).c_str());
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraint::open()
{
    ConstraintView->setVisible(true);
    Gui::Command::runCommand(Gui::Command::Doc,ViewProviderFemConstraint::gethideMeshShowPartStr((static_cast<Fem::Constraint*>(ConstraintView->getObject()))->getNameInDocument()).c_str()); //OvG: Hide meshes and show parts
}

bool TaskDlgFemConstraint::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();

    try {
        std::string refs = parameter->getReferences();

        if (!refs.empty()) {
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.References = [%s]", name.c_str(), refs.c_str());
        } else {
            QMessageBox::warning(parameter, tr("Input error"), tr("You must specify at least one reference"));
            return false;
        }

        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!ConstraintView->getObject()->isValid())
            throw Base::RuntimeError(ConstraintView->getObject()->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return true;
}

bool TaskDlgFemConstraint::reject()
{
    // roll back the changes
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

    return true;
}


#include "moc_TaskFemConstraint.cpp"
