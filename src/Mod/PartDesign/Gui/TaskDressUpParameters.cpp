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
# include <QListWidgetItem>
# include <QTimer>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Gui/Tools.h>
#include <Gui/WaitCursor.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/Gui/ReferenceSelection.h>

#include "TaskDressUpParameters.h"


FC_LOG_LEVEL_INIT("PartDesign",true,true)

using namespace PartDesignGui;
using namespace Gui;


/* TRANSLATOR PartDesignGui::TaskDressUpParameters */

TaskDressUpParameters::TaskDressUpParameters(ViewProviderDressUp *DressUpView, bool selectEdges, bool selectFaces, QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap(DressUpView->featureIcon().c_str()),
              DressUpView->menuName,
              true,
              parent)
    , proxy(nullptr)
    , DressUpView(DressUpView)
    , deleteAction(nullptr)
    , addAllEdgesAction(nullptr)
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

const QString TaskDressUpParameters::btnPreviewStr()
{
    const QString text{ tr("Preview") };
    return text;
}

const QString TaskDressUpParameters::btnSelectStr()
{
    const QString text{ tr("Select") };
    return text;
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

void TaskDressUpParameters::referenceSelected(const Gui::SelectionChanges& msg, QListWidget* widget)
{
    if (strcmp(msg.pDocName, DressUpView->getObject()->getDocument()->getName()) != 0)
        return;

    Gui::Selection().clearSelection();

    PartDesign::DressUp* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    App::DocumentObject* base = this->getBase();

    // TODO: Must we make a copy here instead of assigning to const char* ?
    const char* fname = base->getNameInDocument();
    if (strcmp(msg.pObjectName, fname) != 0)
        return;

    std::string subName(msg.pSubName);
    std::vector<std::string> refs = pcDressUp->Base.getSubValues();
    std::vector<std::string>::iterator f = std::find(refs.begin(), refs.end(), subName);

    if (f != refs.end()) { //If it's found then it's in the list so we remove it.
        refs.erase(f);
        removeItemFromListWidget(widget, msg.pSubName);
    }
    else { //if it's not found then it's not yet in the list so we add it.
        refs.push_back(subName);
        widget->addItem(QString::fromStdString(msg.pSubName));
    }

    updateFeature(pcDressUp, refs);
}

void TaskDressUpParameters::addAllEdges(QListWidget* widget)
{
    PartDesign::DressUp* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());

    Gui::WaitCursor wait;
    int count = pcDressUp->getBaseTopoShape().countSubElements("Edge");
    std::vector<std::string> edgeNames;
    for (int ii = 0; ii < count; ii++){
        std::ostringstream edgeName;
        edgeName << "Edge" << ii+1;
        edgeNames.push_back(edgeName.str());
    }

    //First we need to clear the widget in case the user had faces selected. Else the faces will still be in widget but not in the feature refs!
    QSignalBlocker block(widget);
    widget->clear();

    for (const auto & it : edgeNames){
        widget->addItem(QLatin1String(it.c_str()));
    }

    updateFeature(pcDressUp, edgeNames);
}

void TaskDressUpParameters::deleteRef(QListWidget* widget)
{
    // delete any selections since the reference(s) being deleted might be highlighted
    Gui::Selection().clearSelection();

    // get the list of items to be deleted
    QList<QListWidgetItem*> selectedList = widget->selectedItems();

    PartDesign::DressUp* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    std::vector<std::string> refs = pcDressUp->Base.getSubValues();

    // delete the selection backwards to assure the list index keeps valid for the deletion
    QSignalBlocker block(widget);
    for (int i = selectedList.count() - 1; i > -1; i--) {
        // the ref index is the same as the listWidgetReferences index
        // so we can erase using the row number of the element to be deleted
        int rowNumber = widget->row(selectedList.at(i));
        refs.erase(refs.begin() + rowNumber);
        widget->model()->removeRow(rowNumber);
    }

    updateFeature(pcDressUp, refs);
}

void TaskDressUpParameters::updateFeature(PartDesign::DressUp* pcDressUp, const std::vector<std::string>& refs)
{
    if (selectionMode == refSel)
        DressUpView->highlightReferences(false);

    setupTransaction();
    pcDressUp->Base.setValue(pcDressUp->Base.getValue(), refs);
    pcDressUp->recomputeFeature();
    if (selectionMode == refSel)
        DressUpView->highlightReferences(true);
    else
        hideOnError();
}

void TaskDressUpParameters::onButtonRefSel(bool checked)
{
    setSelectionMode(checked ? refSel : none);
}

void TaskDressUpParameters::doubleClicked(QListWidgetItem* item) {
    // executed when the user double-clicks on any item in the list
    // shows the fillets as they are -> useful to switch out of selection mode

    Q_UNUSED(item)
    wasDoubleClicked = true;

    // assure we are not in selection mode
    setSelectionMode(none);

    // enable next possible single-click event after double-click time passed
    QTimer::singleShot(QApplication::doubleClickInterval(), this, &TaskDressUpParameters::itemClickedTimeout);
}

void TaskDressUpParameters::setSelection(QListWidgetItem* current) {
    // executed when the user selected an item in the list (but double-clicked it)
    // highlights the currently selected item

    if (!wasDoubleClicked) {
        // we treat it as single-click event once the QApplication double-click time is passed
        QTimer::singleShot(QApplication::doubleClickInterval(), this, &TaskDressUpParameters::itemClickedTimeout);

        // name of the item
        std::string subName = current->text().toStdString();
        // get the document name
        std::string docName = DressUpView->getObject()->getDocument()->getName();
        // get the name of the body we are in
        Part::BodyBase* body = PartDesign::Body::findBodyOf(DressUpView->getObject());
        if (body) {
            std::string objName = body->getNameInDocument();

            // Enter selection mode
            if (selectionMode == none)
                setSelectionMode(refSel);
            else
                Gui::Selection().clearSelection();

            // highlight the selected item
            bool block = this->blockSelection(true);
            Gui::Selection().addSelection(docName.c_str(), objName.c_str(), subName.c_str(), 0, 0, 0);
            this->blockSelection(block);
        }
    }
}

void TaskDressUpParameters::itemClickedTimeout() {
    // executed after double-click time passed
    wasDoubleClicked = false;
}

void TaskDressUpParameters::createAddAllEdgesAction(QListWidget* parentList)
{
    // creates a context menu, a shortcut for it and connects it to a slot function

    addAllEdgesAction = new QAction(tr("Add all edges"), this);
    addAllEdgesAction->setShortcut(QKeySequence(QString::fromLatin1("Ctrl+Shift+A")));
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    // display shortcut behind the context menu entry
    addAllEdgesAction->setShortcutVisibleInContextMenu(true);
#endif
    parentList->addAction(addAllEdgesAction);
    addAllEdgesAction->setStatusTip(tr("Adds all edges to the list box (active only when in add selection mode)."));
    parentList->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void TaskDressUpParameters::createDeleteAction(QListWidget* parentList)
{
    // creates a context menu, a shortcut for it and connects it to a slot function

    deleteAction = new QAction(tr("Remove"), this);
    deleteAction->setShortcut(QKeySequence::Delete);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    // display shortcut behind the context menu entry
    deleteAction->setShortcutVisibleInContextMenu(true);
#endif
    parentList->addAction(deleteAction);
    parentList->setContextMenuPolicy(Qt::ActionsContextMenu);
}

bool TaskDressUpParameters::KeyEvent(QEvent *e)
{
    // in case another instance takes key events, accept the overridden key event
    if (e && e->type() == QEvent::ShortcutOverride) {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(e);
        if (deleteAction && Gui::QtTools::matches(kevent, deleteAction->shortcut())) {
            kevent->accept();
            return true;
        }
        if (addAllEdgesAction && Gui::QtTools::matches(kevent, addAllEdgesAction->shortcut())) {
            kevent->accept();
            return true;
        }
    }
    // if we have a Del key, trigger the deleteAction
    else if (e && e->type() == QEvent::KeyPress) {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(e);
        if (deleteAction && deleteAction->isEnabled() &&
            Gui::QtTools::matches(kevent, deleteAction->shortcut())) {
            deleteAction->trigger();
            return true;
        }
        if (addAllEdgesAction && addAllEdgesAction->isEnabled() &&
            Gui::QtTools::matches(kevent, addAllEdgesAction->shortcut())) {
            addAllEdgesAction->trigger();
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
        for (auto item : items) {
            QListWidgetItem* it = widget->takeItem(widget->row(item));
            delete it;
        }
    }
}

void TaskDressUpParameters::hideOnError()
{
    App::DocumentObject* dressup = DressUpView->getObject();
    if (dressup->isError())
        hideObject();
    else
        showObject();
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

Part::Feature* TaskDressUpParameters::getBase() const
{
    PartDesign::DressUp* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    // Unlikely but this may throw an exception in case we are started to edit an object which base feature
    // was deleted. This exception will be likely unhandled inside the dialog and pass upper, But an error
    // message inside the report view is better than a SEGFAULT.
    // Generally this situation should be prevented in ViewProviderDressUp::setEdit()
    return pcDressUp->getBaseObject();
}

void TaskDressUpParameters::setSelectionMode(selectionModes mode)
{
    selectionMode = mode;
    setButtons(mode);

    if (mode == none) {
        showObject();

        Gui::Selection().rmvSelectionGate();

        // remove any highlights and selections
        DressUpView->highlightReferences(false);
    }
    else {
        hideObject();

        AllowSelectionFlags allow;
        allow.setFlag(AllowSelection::EDGE, allowEdges);
        allow.setFlag(AllowSelection::FACE, allowFaces);
        Gui::Selection().addSelectionGate(new ReferenceSelection(this->getBase(), allow));

        DressUpView->highlightReferences(true);
    }

    Gui::Selection().clearSelection();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgDressUpParameters::TaskDlgDressUpParameters(ViewProviderDressUp *DressUpView)
    : TaskDlgFeatureParameters(DressUpView)
    , parameter(nullptr)
{
    assert(DressUpView);
}

TaskDlgDressUpParameters::~TaskDlgDressUpParameters() = default;

//==== calls from the TaskView ===============================================================

bool TaskDlgDressUpParameters::accept()
{
    getDressUpView()->highlightReferences(false);
    std::vector<std::string> refs = parameter->getReferences();
    std::stringstream str;
    str << Gui::Command::getObjectCmd(vp->getObject()) << ".Base = ("
        << Gui::Command::getObjectCmd(parameter->getBase()) << ",[";
    for (const auto & ref : refs)
        str << "\"" << ref << "\",";
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
