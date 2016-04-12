/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net>        *
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
# include <QListWidgetItem>
#endif

#include "TaskDressUpParameters.h"
#include "Workbench.h"
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
#include <Gui/MainWindow.h>
#include <Mod/PartDesign/App/FeatureDressUp.h>
#include <Mod/PartDesign/Gui/ReferenceSelection.h>

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskDressUpParameters */

TaskDressUpParameters::TaskDressUpParameters(ViewProviderDressUp *DressUpView, bool selectEdges, bool selectFaces, QWidget *parent)
    : allowFaces(selectFaces), allowEdges(selectEdges), 
      TaskBox(Gui::BitmapFactory().pixmap((std::string("PartDesign_") + DressUpView->featureName()).c_str()),
              QString::fromAscii((DressUpView->featureName() + " parameters").c_str()),
              true,
              parent),
      DressUpView(DressUpView)
{
    selectionMode = none;
}

TaskDressUpParameters::~TaskDressUpParameters()
{
    // make sure to remove selection gate in all cases
    Gui::Selection().rmvSelectionGate();
}

const bool TaskDressUpParameters::referenceSelected(const Gui::SelectionChanges& msg)
{
    if ((msg.Type == Gui::SelectionChanges::AddSelection) && (
                (selectionMode == refAdd) || (selectionMode == refRemove))) {

        if (strcmp(msg.pDocName, DressUpView->getObject()->getDocument()->getName()) != 0)
            return false;

        PartDesign::DressUp* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
        App::DocumentObject* base = this->getBase();

        // TODO: Must we make a copy here instead of assigning to const char* ?
        const char* fname = base->getNameInDocument();        
        if (strcmp(msg.pObjectName, fname) != 0)
            return false;

        std::string subName(msg.pSubName);
        std::vector<std::string> refs = pcDressUp->Base.getSubValues();
        std::vector<std::string>::iterator f = std::find(refs.begin(), refs.end(), subName);

        if (selectionMode == refAdd) {
            if (f == refs.end())
                refs.push_back(subName);
            else
                return false; // duplicate selection
        } else {
            if (f != refs.end())
                refs.erase(f);
            else
                return false;
        }
        DressUpView->highlightReferences(false);
        pcDressUp->Base.setValue(base, refs);        
        pcDressUp->getDocument()->recomputeFeature(pcDressUp);

        return true;
    }

    return false;
}

void TaskDressUpParameters::onButtonRefAdd(bool checked)
{
    if (checked) {
        clearButtons(refAdd);
        hideObject();
        selectionMode = refAdd;
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new ReferenceSelection(this->getBase(), allowEdges, allowFaces, false));
        DressUpView->highlightReferences(true);
    }
}

void TaskDressUpParameters::onButtonRefRemove(const bool checked)
{
    if (checked) {
        clearButtons(refRemove);
        hideObject();
        selectionMode = refRemove;
        Gui::Selection().clearSelection();        
        Gui::Selection().addSelectionGate(new ReferenceSelection(this->getBase(), allowEdges, allowFaces, false));
        DressUpView->highlightReferences(true);
    }
}

const std::vector<std::string> TaskDressUpParameters::getReferences() const
{
    PartDesign::DressUp* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    std::vector<std::string> result = pcDressUp->Base.getSubValues();
    return result;
}

// TODO: This code is identical with TaskTransformedParameters::removeItemFromListWidget()
void TaskDressUpParameters::removeItemFromListWidget(QListWidget* widget, const char* itemstr)
{
    QList<QListWidgetItem*> items = widget->findItems(QString::fromAscii(itemstr), Qt::MatchExactly);
    if (!items.empty()) {
        for (QList<QListWidgetItem*>::const_iterator i = items.begin(); i != items.end(); i++) {
            QListWidgetItem* it = widget->takeItem(widget->row(*i));
            delete it;
        }
    }
}

void TaskDressUpParameters::hideObject()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    App::DocumentObject* base = getBase();
    if (doc != NULL && base != NULL) {
        doc->setHide(DressUpView->getObject()->getNameInDocument());
        doc->setShow(base->getNameInDocument());
    }
}

void TaskDressUpParameters::showObject()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    App::DocumentObject* base = getBase();
    if (doc != NULL && base != NULL) {
        doc->setShow(DressUpView->getObject()->getNameInDocument());
        doc->setHide(base->getNameInDocument());
    }
}

Part::Feature* TaskDressUpParameters::getBase(void) const
{
    PartDesign::DressUp* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    // Unlikely but this may throw an exception in case we are started to edit an object which base feature
    // was deleted. This exception will be likely unhandled inside the dialog and pass upper, But an error
    // message inside the report view is better than a SEGFAULT.
    // TODO: generally this situation should be prevented in ViewProviderDressUp
    return pcDressUp->getBaseObject();
}

void TaskDressUpParameters::exitSelectionMode()
{
    selectionMode = none;
    Gui::Selection().rmvSelectionGate();
    Gui::Selection().clearSelection();
    showObject();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgDressUpParameters::TaskDlgDressUpParameters(ViewProviderDressUp *DressUpView)
    : TaskDlgFeatureParameters(DressUpView)
{
    assert(DressUpView);
}

TaskDlgDressUpParameters::~TaskDlgDressUpParameters()
{

}

//==== calls from the TaskView ===============================================================

bool TaskDlgDressUpParameters::accept()
{
    std::string name = vp->getObject()->getNameInDocument();
    getDressUpView()->highlightReferences(false);

    std::vector<std::string> refs = parameter->getReferences();
    std::stringstream str;
    str << "App.ActiveDocument." << name.c_str() << ".Base = (App.ActiveDocument."
        << parameter->getBase()->getNameInDocument() << ",[";
    for (std::vector<std::string>::const_iterator it = refs.begin(); it != refs.end(); ++it)
        str << "\"" << *it << "\",";
    str << "])";
    Gui::Command::doCommand(Gui::Command::Doc,str.str().c_str());

    return TaskDlgFeatureParameters::accept();
}

#include "moc_TaskDressUpParameters.cpp"
