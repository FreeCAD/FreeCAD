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
# include <QMessageBox>
#endif

#include <Gui/Command.h>
#include <Gui/MDIView.h>
#include <Gui/Control.h>
#include <Gui/Application.h>
#include <Base/Exception.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>

#include "TaskFeatureParameters.h"

#include "ViewProvider.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProvider, PartGui::ViewProviderPart)

ViewProvider::ViewProvider()
    :oldWb(""), oldTip(NULL)
{
}

ViewProvider::~ViewProvider()
{
}

bool ViewProvider::doubleClicked(void)
{
	PartDesign::Body* body = PartDesign::Body::findBodyOf(getObject());
    // TODO May be move to setEdit()? (2015-07-26, Fat-Zer)
	if (body != NULL) {
        // Drop into insert mode so that the user doesn't see all the geometry that comes later in the tree
        // Also, this way the user won't be tempted to use future geometry as external references for the sketch
		oldTip = body->Tip.getValue();
        if (oldTip != this->pcObject)
            Gui::Command::doCommand(Gui::Command::Gui,"FreeCADGui.runCommand('PartDesign_MoveTip')");
        else
            oldTip = NULL;
    } else {
        oldTip = NULL;
    }

    try {
        std::string Msg("Edit ");
        Msg += this->pcObject->Label.getValue();
        Gui::Command::openCommand(Msg.c_str());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().setEdit('%s',0)",
                this->pcObject->getNameInDocument());
    }
    catch (const Base::Exception&) {
        Gui::Command::abortCommand();
    }
    return true;
}

bool ViewProvider::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this feature the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgFeatureParameters *featureDlg = qobject_cast<TaskDlgFeatureParameters *>(dlg);
        // NOTE: if the dialog is not partDesigan dialog the featureDlg will be NULL
        if (featureDlg && featureDlg->viewProvider() != this) {
            featureDlg = 0; // another feature left open its task panel
        }
        if (dlg && !featureDlg) {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes) {
                Gui::Control().reject();
            } else {
                return false;
            }
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // always change to PartDesign WB, remember where we come from
        oldWb = Gui::Command::assureWorkbench("PartDesignWorkbench");

        // start the edit dialog if
        if (!featureDlg) {
            featureDlg = this->getEditDialog();
            if (!featureDlg) { // Shouldn't generally happen
                throw Base::Exception ("Failed to create new edit dialog.");
            }
        }

        Gui::Control().showDialog(featureDlg);
        return true;
    } else {
        return PartGui::ViewProviderPart::setEdit(ModNum);
    }
}


TaskDlgFeatureParameters *ViewProvider::getEditDialog() {
    throw Base::Exception("getEditDialog() not implemented");
}


void ViewProvider::unsetEdit(int ModNum)
{
    // return to the WB we were in before editing the PartDesign feature
    if (!oldWb.empty())
        Gui::Command::assureWorkbench(oldWb.c_str());

    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
		PartDesign::Body* activeBody = Gui::Application::Instance->activeView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);
        Gui::Control().closeDialog();
		if ((activeBody != NULL) && (oldTip != NULL)) {
            Gui::Selection().clearSelection();
            Gui::Selection().addSelection(oldTip->getDocument()->getName(), oldTip->getNameInDocument());
            Gui::Command::doCommand(Gui::Command::Gui,"FreeCADGui.runCommand('PartDesign_MoveTip')");
        }
        oldTip = NULL;
    }
    else {
        PartGui::ViewProviderPart::unsetEdit(ModNum);
        oldTip = NULL;
    }
}

void ViewProvider::updateData(const App::Property* prop)
{
    // TODO What's that? (2015-07-24, Fat-Zer)
    if (prop->getTypeId() == Part::PropertyPartShape::getClassTypeId() &&
        strcmp(prop->getName(),"AddSubShape") == 0) {
        return;
    }

    inherited::updateData(prop);
}

bool ViewProvider::onDelete(const std::vector<std::string> &)
{
    PartDesign::Feature* feature = static_cast<PartDesign::Feature*>(getObject());
    App::DocumentObject* previous = feature->getBaseObject(/* silent = */ true );

    // Make the tip or the previous feature visiable again with preference to the previous one
    // if the feature was visiable itself
    if (isShow()) {
        // TODO TaskDlgFeatureParameters::reject has the same code. May be this one is excess?
        //      (2015-07-24, Fat-Zer)
        if (previous && Gui::Application::Instance->getViewProvider(previous)) {
            Gui::Application::Instance->getViewProvider(previous)->show();
        } else {
            // Body feature housekeeping
            Part::BodyBase* body = PartDesign::Body::findBodyOf(getObject());
            if (body != NULL) {
                App::DocumentObject* tip = body->Tip.getValue();
                if (tip && Gui::Application::Instance->getViewProvider(tip)) {
                    Gui::Application::Instance->getViewProvider(tip)->show();
                }
            }
        }
    }
    return true;
}
