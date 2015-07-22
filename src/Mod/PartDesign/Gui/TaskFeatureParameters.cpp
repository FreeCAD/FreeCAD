/***************************************************************************
 *   Copyright (C) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/PartDesign/App/Body.h>

#include "TaskFeatureParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/*********************************************************************
 *                            Task Dialog                            *
 *********************************************************************/
TaskDlgFeatureParameters::TaskDlgFeatureParameters(PartDesignGui::ViewProvider *vp)
    : TaskDialog(),vp(vp)
{
    assert(vp);
}

TaskDlgFeatureParameters::~TaskDlgFeatureParameters()
{

}

bool TaskDlgFeatureParameters::reject()
{
    PartDesign::Feature* feature = static_cast<PartDesign::Feature*>(vp->getObject());

    PartDesign::Body* body = PartDesign::Body::findBodyOf(feature);

    // Find out previous feature we won't be able to do it after abort
    // (at least in the body case)
    App::DocumentObject* previous;
    if (body) {
        // NOTE: feature->getBaseObject() should return the same for body
        previous = body->getPrevSolidFeature(feature, /*inclusive =*/ false);
    } else {
        previous = feature->getBaseObject();
    }

    // roll back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");


    // if abort command deleted the object make the previous feature visible again
    if (!Gui::Application::Instance->getViewProvider(feature)) {
        // Body housekeeping
        if (body != NULL) {
            // Make the tip or the prebious feature visiable again with preference to the previous one
            App::DocumentObject* tip = body->Tip.getValue();

            if (previous && Gui::Application::Instance->getViewProvider(previous)) {
                Gui::Application::Instance->getViewProvider(previous)->show();
            } else if (tip && Gui::Application::Instance->getViewProvider(tip)) {
                Gui::Application::Instance->getViewProvider(tip)->show();
            }
        } else {
            if (previous && Gui::Application::Instance->getViewProvider(previous))
                Gui::Application::Instance->getViewProvider(previous)->show();
        }
    }

    return true;
}

#include "moc_TaskFeatureParameters.cpp"
