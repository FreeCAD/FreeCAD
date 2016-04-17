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
# include <QMessageBox>
# include <QAction>
# include <QMenu>
#endif

#include "ViewProviderBoolean.h"
#include "TaskBooleanParameters.h"
#include <Mod/PartDesign/App/FeatureBoolean.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>


using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderBoolean,PartDesignGui::ViewProvider)

ViewProviderBoolean::ViewProviderBoolean()
{
    sPixmap = "PartDesign_Boolean.svg";
}

ViewProviderBoolean::~ViewProviderBoolean()
{
}


void ViewProviderBoolean::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit boolean"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
}

bool ViewProviderBoolean::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this fillet the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgBooleanParameters *booleanDlg = qobject_cast<TaskDlgBooleanParameters *>(dlg);
        if (booleanDlg && booleanDlg->getBooleanView() != this)
            booleanDlg = 0; // another pad left open its task panel
        if (dlg && !booleanDlg) {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes)
                Gui::Control().closeDialog();
            else
                return false;
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // always change to PartDesign WB, remember where we come from
        oldWb = Gui::Command::assureWorkbench("PartDesignWorkbench");

        // start the edit dialog
        if (booleanDlg)
            Gui::Control().showDialog(booleanDlg);
        else
            Gui::Control().showDialog(new TaskDlgBooleanParameters(this));

        return true;
    }
    else {
        return PartGui::ViewProviderPart::setEdit(ModNum);
    }
}

std::vector<App::DocumentObject*> ViewProviderBoolean::claimChildren(void)const
{
    return static_cast<PartDesign::Boolean*>(getObject())->Bodies.getValues();
}

bool ViewProviderBoolean::onDelete(const std::vector<std::string> &s)
{
    PartDesign::Boolean* pcBoolean = static_cast<PartDesign::Boolean*>(getObject());

    // if abort command deleted the object the bodies are visible again
    std::vector<App::DocumentObject*> bodies = pcBoolean->Bodies.getValues();
    for (std::vector<App::DocumentObject*>::const_iterator b = bodies.begin(); b != bodies.end(); b++) {
        if (*b && Gui::Application::Instance->getViewProvider(*b))
        Gui::Application::Instance->getViewProvider(*b)->show();
    }

    return ViewProvider::onDelete(s);
}


