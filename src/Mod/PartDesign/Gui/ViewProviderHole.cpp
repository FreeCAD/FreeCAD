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
# include <QMenu>
# include <QMessageBox>
#endif

#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/FeatureHole.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "ViewProviderHole.h"
#include "TaskHoleParameters.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderHole,PartDesignGui::ViewProvider)

ViewProviderHole::ViewProviderHole()
{
    sPixmap = "PartDesign_Hole.svg";
}

ViewProviderHole::~ViewProviderHole() = default;

std::vector<App::DocumentObject*> ViewProviderHole::claimChildren()const
{
    std::vector<App::DocumentObject*> temp;
    temp.push_back(static_cast<PartDesign::Hole*>(getObject())->Profile.getValue());

    return temp;
}

void ViewProviderHole::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit hole"));
    PartGui::ViewProviderPart::setupContextMenu(menu, receiver, member); // clazy:exclude=skipped-base-method
}

bool ViewProviderHole::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this hole the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgHoleParameters *holeDlg = qobject_cast<TaskDlgHoleParameters *>(dlg);
        if (holeDlg && holeDlg->getHoleView() != this)
            holeDlg = nullptr; // another hole left open its task panel
        if (dlg && !holeDlg) {
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
        if (holeDlg)
            Gui::Control().showDialog(holeDlg);
        else
            Gui::Control().showDialog(new TaskDlgHoleParameters(this));

        return true;
    }
    else {
        return PartGui::ViewProviderPart::setEdit(ModNum); // clazy:exclude=skipped-base-method
    }
}

bool ViewProviderHole::onDelete(const std::vector<std::string> &s)
{
    // get the Sketch
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(getObject());
    Sketcher::SketchObject *pcSketch = nullptr;
    if (pcHole->Profile.getValue())
        pcSketch = static_cast<Sketcher::SketchObject*>(pcHole->Profile.getValue());

    // if abort command deleted the object the sketch is visible again
    if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
        Gui::Application::Instance->getViewProvider(pcSketch)->show();

    return ViewProvider::onDelete(s);
}
