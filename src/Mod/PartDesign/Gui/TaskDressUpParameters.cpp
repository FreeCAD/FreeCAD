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

#include <boost/algorithm/string/predicate.hpp>
#include "TaskDressUpParameters.h"
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

FC_LOG_LEVEL_INIT("PartDesign",true,true)

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskDressUpParameters */

TaskDressUpParameters::TaskDressUpParameters(ViewProviderDressUp *DressUpView, bool selectEdges, bool selectFaces, QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap((std::string("PartDesign_") + DressUpView->featureName()).c_str()),
              QString::fromLatin1((DressUpView->featureName() + " parameters").c_str()),
              true,
              parent)
    , proxy(0)
    , DressUpView(DressUpView)
    , allowFaces(selectFaces)
    , allowEdges(selectEdges)
{
    selectionMode = none;
    showObject();
}

TaskDressUpParameters::~TaskDressUpParameters()
{
    // make sure to remove selection gate in all cases
    Gui::Selection().rmvSelectionGate();
}

void TaskDressUpParameters::setupTransaction() {
    int tid = 0;
    const char *name = App::GetApplication().getActiveTransaction(&tid);
    std::string n("Edit ");
    n += DressUpView->getObject()->Label.getValue();
    if(!name || n != name)
        App::GetApplication().setActiveTransaction(n.c_str());
}

void TaskDressUpParameters::setup(QListWidget *widget) {
    auto* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    if(!pcDressUp || !pcDressUp->Base.getValue())
        return;
    auto base = pcDressUp->Base.getValue();
    const auto &subs = pcDressUp->Base.getShadowSubs();
    const auto &baseShape = pcDressUp->getTopoShape(base);
    std::set<std::string> subSet;
    for(auto &sub : subs) 
        subSet.insert(sub.first.empty()?sub.second:sub.first);
    bool touched = false;
    std::vector<std::string> refs;
    for(auto &sub : subs) {
        refs.push_back(sub.second);
        if(sub.first.empty() || baseShape.isNull()) {
            widget->addItem(QString::fromStdString(sub.second));
            continue;
        }
        auto &ref = sub.first;
        Part::TopoShape edge;
        try {
            edge = baseShape.getSubShape(ref.c_str());
        }catch(...) {}
        if(!edge.isNull())  {
            widget->addItem(QString::fromStdString(sub.second));
            continue;
        }
        FC_WARN("missing element reference: " << pcDressUp->getFullName() << "." << ref);
        bool popped = false;
        for(auto &name : Part::Feature::getRelatedElements(base,ref.c_str())) {
            if(!subSet.insert(name.second).second || !subSet.insert(name.first).second)
                continue;
            FC_WARN("guess element reference: " << ref << " -> " << name.first);
            widget->addItem(QString::fromStdString(name.second));
            if(!popped) {
                refs.pop_back();
                touched = true;
                popped = true;
            }
            refs.push_back(name.second);
        }
        if(!popped) {
            if(!boost::starts_with(refs.back(),Data::ComplexGeoData::missingPrefix()))
                refs.back() = Data::ComplexGeoData::missingPrefix()+refs.back();
            auto item = new QListWidgetItem(widget);
            item->setText(QString::fromStdString(refs.back()));
            item->setForeground(Qt::red);
        }
    }
    if(touched){
        setupTransaction();
        pcDressUp->Base.setValue(base,refs);
        pcDressUp->getDocument()->recomputeFeature(pcDressUp);
    }
}

bool TaskDressUpParameters::referenceSelected(const Gui::SelectionChanges& msg)
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
        setupTransaction();
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
    QList<QListWidgetItem*> items = widget->findItems(QString::fromLatin1(itemstr), Qt::MatchExactly);
    if (!items.empty()) {
        for (QList<QListWidgetItem*>::const_iterator i = items.begin(); i != items.end(); i++) {
            QListWidgetItem* it = widget->takeItem(widget->row(*i));
            delete it;
        }
    }
}

void TaskDressUpParameters::hideObject()
{
    App::DocumentObject* base = getBase();
    if(base) {
        DressUpView->getObject()->Visibility.setValue(false);
        base->Visibility.setValue(true);
    }
}

void TaskDressUpParameters::showObject()
{
    DressUpView->getObject()->Visibility.setValue(true);
    App::DocumentObject* base = getBase();
    if (base) 
        base->Visibility.setValue(false);
}

Part::Feature* TaskDressUpParameters::getBase(void) const
{
    PartDesign::DressUp* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    // Unlikely but this may throw an exception in case we are started to edit an object which base feature
    // was deleted. This exception will be likely unhandled inside the dialog and pass upper, But an error
    // message inside the report view is better than a SEGFAULT.
    // Generally this situation should be prevented in ViewProviderDressUp::setEdit()
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
    , parameter(0)
{
    assert(DressUpView);
}

TaskDlgDressUpParameters::~TaskDlgDressUpParameters()
{

}

//==== calls from the TaskView ===============================================================

bool TaskDlgDressUpParameters::accept()
{
    getDressUpView()->highlightReferences(false);

    std::vector<std::string> refs = parameter->getReferences();
    std::stringstream str;
    str << Gui::Command::getObjectCmd(vp->getObject()) << ".Base = (" 
        << Gui::Command::getObjectCmd(parameter->getBase()) << ",[";
    for (std::vector<std::string>::const_iterator it = refs.begin(); it != refs.end(); ++it)
        str << "\"" << *it << "\",";
    str << "])";
    Gui::Command::runCommand(Gui::Command::Doc,str.str().c_str());
    return TaskDlgFeatureParameters::accept();
}

bool TaskDlgDressUpParameters::reject()
{
    getDressUpView()->highlightReferences(false);
    return TaskDlgFeatureParameters::reject();
}

#include "moc_TaskDressUpParameters.cpp"
