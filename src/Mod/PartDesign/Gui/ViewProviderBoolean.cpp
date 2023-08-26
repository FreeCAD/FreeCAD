/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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
# include <QMenu>
# include <QMessageBox>
#endif

#include "ViewProviderBoolean.h"
#include "TaskBooleanParameters.h"
#include <Mod/PartDesign/App/FeatureBoolean.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Document.h>


using namespace PartDesignGui;

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesignGui::ViewProviderBoolean,PartDesignGui::ViewProvider)

const char* PartDesignGui::ViewProviderBoolean::DisplayEnum[] = {"Result","Tools",nullptr};


ViewProviderBoolean::ViewProviderBoolean()
{
    sPixmap = "PartDesign_Boolean.svg";
    Gui::ViewProviderGeoFeatureGroupExtension::initExtension(this);

    ADD_PROPERTY(Display,((long)0));
    Display.setEnums(DisplayEnum);
}

ViewProviderBoolean::~ViewProviderBoolean() = default;


void ViewProviderBoolean::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit boolean"));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
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
            booleanDlg = nullptr; // another pad left open its task panel
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
        return PartGui::ViewProviderPart::setEdit(ModNum); // clazy:exclude=skipped-base-method
    }
}

bool ViewProviderBoolean::onDelete(const std::vector<std::string> &s)
{
    PartDesign::Boolean* pcBoolean = static_cast<PartDesign::Boolean*>(getObject());

    // if abort command deleted the object the bodies are visible again
    std::vector<App::DocumentObject*> bodies = pcBoolean->Group.getValues();
    for (auto body : bodies) {
        if (auto vp = Gui::Application::Instance->getViewProvider(body)) {
            vp->show();
        }
    }

    return ViewProvider::onDelete(s);
}

void ViewProviderBoolean::attach(App::DocumentObject* obj) {
    PartGui::ViewProviderPartExt::attach(obj);

    //set default display mode to override the "Group" display mode
    setDisplayMode("Flat Lines");
}

void ViewProviderBoolean::onChanged(const App::Property* prop) {

    PartDesignGui::ViewProvider::onChanged(prop);

    if(prop == &Display) {

        if(Display.getValue() == 0) {
            auto vp = getBodyViewProvider();
            if(vp)
                setDisplayMode(vp->DisplayMode.getValueAsString());
            else
                setDisplayMode("Flat Lines");
        } else {
            setDisplayMode("Group");
        }
    }
}
