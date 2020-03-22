/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer                                    *
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
# include <QAction>
# include <QApplication>
# include <QKeyEvent>
# include <QListWidget>
# include <QListWidgetItem>
# include <QTimer>
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
#include <Mod/PartDesign/App/Body.h>
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
    , deleteAction(nullptr)
    , allowFaces(selectFaces)
    , allowEdges(selectEdges)
{
    // remember initial transaction ID
    App::GetApplication().getActiveTransaction(&transactionID);

    selectionMode = none;
    showObject();
}

TaskDressUpParameters::~TaskDressUpParameters()
{
    // make sure to remove selection gate in all cases
    Gui::Selection().rmvSelectionGate();
}

void TaskDressUpParameters::setupTransaction()
{
    if (!DressUpView)
        return;

    int tid = 0;
    App::GetApplication().getActiveTransaction(&tid);
    if (tid && tid == transactionID)
        return;

    // open a transaction if none is active
    std::string n("Edit ");
    n += DressUpView->getObject()->Label.getValue();
    transactionID = App::GetApplication().setActiveTransaction(n.c_str());
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
    } else {
        exitSelectionMode();
        DressUpView->highlightReferences(false);
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
    else {
        exitSelectionMode();
        DressUpView->highlightReferences(false);
    }
}

void TaskDressUpParameters::doubleClicked(QListWidgetItem* item) {
    // executed when the user double-clicks on any item in the list
    // shows the fillets as they are -> useful to switch out of selection mode

    Q_UNUSED(item)
    wasDoubleClicked = true;

    // assure we are not in selection mode
    exitSelectionMode();
    clearButtons(none);

    // assure the fillets are shown
    showObject();
    // remove any highlights and selections
    DressUpView->highlightReferences(false);
    Gui::Selection().clearSelection();

    // enable next possible single-click event after double-click time passed
    QTimer::singleShot(QApplication::doubleClickInterval(), this, SLOT(itemClickedTimeout()));
}

void TaskDressUpParameters::setSelection(QListWidgetItem* current) {
    // executed when the user selected an item in the list (but double-clicked it)
    // highlights the currently selected item

    if (!wasDoubleClicked) {
        // we treat it as single-click event once the QApplication double-click time is passed
        QTimer::singleShot(QApplication::doubleClickInterval(), this, SLOT(itemClickedTimeout()));

        // name of the item
        std::string subName = current->text().toStdString();
        // get the document name
        std::string docName = DressUpView->getObject()->getDocument()->getName();
        // get the name of the body we are in
        Part::BodyBase* body = PartDesign::Body::findBodyOf(DressUpView->getObject());
        std::string objName = body->getNameInDocument();

        // hide fillet to see the original edge
        // (a fillet creates new edges so that the original one is not available)
        hideObject();
        // highlight all objects in the list
        DressUpView->highlightReferences(true);
        // clear existing selection because only the current item is highlighted, not all selected ones to keep the overview
        Gui::Selection().clearSelection();
        // highligh the selected item
        Gui::Selection().addSelection(docName.c_str(), objName.c_str(), subName.c_str(), 0, 0, 0);
    }
}

void TaskDressUpParameters::itemClickedTimeout() {
    // executed after double-click time passed
    wasDoubleClicked = false;
}

void TaskDressUpParameters::createDeleteAction(QListWidget* parentList, QWidget* parentButton)
{
    // creates a context menu, a shortcutt for it and connects it to e slot function

    deleteAction = new QAction(tr("Remove"), this);
    deleteAction->setShortcut(QKeySequence::Delete);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    // display shortcut behind the context menu entry
    deleteAction->setShortcutVisibleInContextMenu(true);
#endif
    parentList->addAction(deleteAction);
    // if there is only one item, it cannot be deleted
    if (parentList->count() == 1) {
        deleteAction->setEnabled(false);
        deleteAction->setStatusTip(tr("There must be at least one item"));
        parentButton->setEnabled(false);
        parentButton->setToolTip(tr("There must be at least one item"));
    }
    parentList->setContextMenuPolicy(Qt::ActionsContextMenu);
}

bool TaskDressUpParameters::KeyEvent(QEvent *e)
{
    // in case another instance takes key events, accept the overridden key event
    if (e && e->type() == QEvent::ShortcutOverride) {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(e);
        if (kevent->modifiers() == Qt::NoModifier) {
            if (deleteAction && kevent->key() == Qt::Key_Delete) {
                kevent->accept();
                return true;
            }
        }
    }
    // if we have a Del key, trigger the deleteAction
    else if (e && e->type() == QEvent::KeyPress) {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(e);
        if (kevent->key() == Qt::Key_Delete) {
            if (deleteAction && deleteAction->isEnabled())
                deleteAction->trigger();
            return true;
        }
    }

    return TaskDressUpParameters::event(e);
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
    App::DocumentObject* base = getBase();
    if (base) {
        DressUpView->getObject()->Visibility.setValue(true);
        base->Visibility.setValue(false);
    }
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
