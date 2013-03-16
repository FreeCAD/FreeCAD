/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
#endif

#include "ViewProviderPocket.h"
#include "TaskPocketParameters.h"
#include <Mod/PartDesign/App/FeaturePocket.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>


using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderPocket,PartDesignGui::ViewProvider)

ViewProviderPocket::ViewProviderPocket()
{
    sPixmap = "PartDesign_Pocket.svg"; 
}

ViewProviderPocket::~ViewProviderPocket()
{
}

std::vector<App::DocumentObject*> ViewProviderPocket::claimChildren(void)const
{
    std::vector<App::DocumentObject*> temp;
    temp.push_back(static_cast<PartDesign::Pocket*>(getObject())->Sketch.getValue());

    return temp;
}

void ViewProviderPocket::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit pocket"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartGui::ViewProviderPart::setupContextMenu(menu, receiver, member);
}

bool ViewProviderPocket::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this pad the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgPocketParameters *padDlg = qobject_cast<TaskDlgPocketParameters *>(dlg);
        if (padDlg && padDlg->getPocketView() != this)
            padDlg = 0; // another pad left open its task panel
        if (dlg && !padDlg) {
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
        //if(ModNum == ViewProvider::Default)
        //    Gui::Command::openCommand("Change pocket parameters");

        // start the edit dialog
        if (padDlg)
            Gui::Control().showDialog(padDlg);
        else
            Gui::Control().showDialog(new TaskDlgPocketParameters(this));

        return true;
    }
    else {
        return PartGui::ViewProviderPart::setEdit(ModNum);
    }
}

void ViewProviderPocket::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        // and update the pad
        //getSketchObject()->getDocument()->recompute();

        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
    }
    else {
        PartGui::ViewProviderPart::unsetEdit(ModNum);
    }
}

bool ViewProviderPocket::onDelete(const std::vector<std::string> &)
{
    // get the support and Sketch
    PartDesign::Pocket* pcPocket = static_cast<PartDesign::Pocket*>(getObject()); 
    Sketcher::SketchObject *pcSketch = 0;
    App::DocumentObject    *pcSupport = 0;
    if (pcPocket->Sketch.getValue()){
        pcSketch = static_cast<Sketcher::SketchObject*>(pcPocket->Sketch.getValue()); 
        pcSupport = pcSketch->Support.getValue();
    }

    // if abort command deleted the object the support is visible again
    if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
        Gui::Application::Instance->getViewProvider(pcSketch)->show();
    if (pcSupport && Gui::Application::Instance->getViewProvider(pcSupport))
        Gui::Application::Instance->getViewProvider(pcSupport)->show();

    return true;
}


