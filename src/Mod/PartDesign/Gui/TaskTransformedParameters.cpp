/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QMessageBox>
#endif

#include "TaskTransformedParameters.h"
#include "TaskMultiTransformParameters.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/FeatureTransformed.h>
#include <Mod/PartDesign/App/FeatureAdditive.h>
#include <Mod/PartDesign/App/FeatureSubtractive.h>

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskTransformedParameters */

TaskTransformedParameters::TaskTransformedParameters(ViewProviderTransformed *TransformedView, QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap((std::string("PartDesign_") + TransformedView->featureName).c_str()),
              QString::fromAscii((TransformedView->featureName + " parameters").c_str()),
              true,
              parent),
      TransformedView(TransformedView),
      parentTask(NULL),
      insideMultiTransform(false),
      updateUIinProgress(false)
{
    // Start in feature selection mode
    featureSelectionMode = true;
}

TaskTransformedParameters::TaskTransformedParameters(QWidget *parent, TaskMultiTransformParameters *parentTask)
    : TaskBox(QPixmap(), tr(""), true, parent),
      TransformedView(NULL),
      parentTask(parentTask),
      insideMultiTransform(true),
      updateUIinProgress(false)
{
    // Start in reference selection mode and stay there! Feature selection makes
    // no sense inside a MultiTransform
    featureSelectionMode = false;
}

const bool TaskTransformedParameters::originalSelected(const Gui::SelectionChanges& msg)
{
    if (featureSelectionMode && (msg.Type == Gui::SelectionChanges::AddSelection)) {
        PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(TransformedView->getObject());
        App::DocumentObject* selectedObject = pcTransformed->getDocument()->getActiveObject();
        if (!selectedObject->isDerivedFrom(PartDesign::Additive::getClassTypeId()) &&
            !selectedObject->isDerivedFrom(PartDesign::Subtractive::getClassTypeId()))
            return false;
        if (TransformedView->getObject() == pcTransformed)
            return false;

        std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();

        if (std::find(originals.begin(), originals.end(), selectedObject) == originals.end()) {
            originals.push_back(selectedObject);
            pcTransformed->Originals.setValues(originals);
            pcTransformed->getDocument()->recomputeFeature(pcTransformed);
            return true;
        }
    }

    return false;
}

void TaskTransformedParameters::onOriginalDeleted(const int row)
{
    PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(TransformedView->getObject());
    std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();
    originals.erase(originals.begin() + row);
    pcTransformed->Originals.setValues(originals);
    pcTransformed->getDocument()->recomputeFeature(pcTransformed);
}

const std::vector<App::DocumentObject*> TaskTransformedParameters::getOriginals(void) const
{
    PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(TransformedView->getObject());
    std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();

    return originals;
}

PartDesign::Transformed *TaskTransformedParameters::getObject() const
{

    if (insideMultiTransform)
        return parentTask->getSubFeature();
    else
        return static_cast<PartDesign::Transformed*>(TransformedView->getObject());
}

void TaskTransformedParameters::recomputeFeature()
{
    if (insideMultiTransform) {
        parentTask->recomputeFeature();
    } else {
        PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(TransformedView->getObject());
        pcTransformed->getDocument()->recomputeFeature(pcTransformed);
    }
}

App::DocumentObject* TaskTransformedParameters::getOriginalObject() const
{
    if (insideMultiTransform) {
        return parentTask->getOriginalObject();
    } else {
        PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(TransformedView->getObject());
        return pcTransformed->getOriginalObject();
    }
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgTransformedParameters::TaskDlgTransformedParameters(ViewProviderTransformed *TransformedView)
    : TaskDialog(),TransformedView(TransformedView)
{
    assert(TransformedView);
}

//==== calls from the TaskView ===============================================================

bool TaskDlgTransformedParameters::accept()
{
    std::string name = TransformedView->getObject()->getNameInDocument();

    try {
        //Gui::Command::openCommand(featureName + " changed");
        std::vector<App::DocumentObject*> originals = parameter->getOriginals();
        std::stringstream str;
        str << "App.ActiveDocument." << name.c_str() << ".Originals = [";
        for (std::vector<App::DocumentObject*>::const_iterator it = originals.begin(); it != originals.end(); ++it)
        {
            if ((*it) != NULL)
                str << "App.ActiveDocument." << (*it)->getNameInDocument() << ",";
        }
        str << "]";
        Gui::Command::runCommand(Gui::Command::Doc,str.str().c_str());
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromAscii(e.what()));
        return false;
    }

    // Continue (usually in virtual method accept())
    return true;
}

bool TaskDlgTransformedParameters::reject()
{
    // Get object before view is invalidated
    PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(TransformedView->getObject());
    std::vector<App::DocumentObject*> pcOriginals = pcTransformed->Originals.getValues();

    // roll back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

    // if abort command deleted the object the originals are visible again
    if (!Gui::Application::Instance->getViewProvider(pcTransformed)) {
        for (std::vector<App::DocumentObject*>::const_iterator it = pcOriginals.begin(); it != pcOriginals.end(); ++it)
        {
            if (((*it) != NULL) && (Gui::Application::Instance->getViewProvider(*it) != NULL)) {
                Gui::Application::Instance->getViewProvider(*it)->show();
            }
        }
    }

    return true;
}



#include "moc_TaskTransformedParameters.cpp"
