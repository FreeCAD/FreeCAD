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
    , deleteAction(nullptr)
    , addAllEdgesAction(nullptr)
    , allowFaces(selectFaces)
    , allowEdges(selectEdges)
    , DressUpView(DressUpView)
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
    if (DressUpView.expired())
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

    PartDesign::DressUp* pcDressUp = DressUpView->getObject<PartDesign::DressUp>();
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
    Q_UNUSED(widget)

    if (DressUpView.expired()) {
        return;
    }

    PartDesign::DressUp* pcDressUp = DressUpView->getObject<PartDesign::DressUp>();
    App::DocumentObject* base = pcDressUp->Base.getValue();
    if (!base) {
        return;
    }
    int count = Part::Feature::getTopoShape(base).countSubShapes(TopAbs_EDGE);
    auto subValues = pcDressUp->Base.getSubValues(false);
    std::size_t len = subValues.size();
    for (int i = 0; i < count; ++i) {
        std::string name = "Edge" + std::to_string(i+1);
        if (std::find(subValues.begin(), subValues.begin() + len, name)
            == subValues.begin() + len) {
            subValues.push_back(name);
        }
    }
    if (subValues.size() == len) {
        return;
    }
    try {
        setupTransaction();
        pcDressUp->Base.setValue(base, subValues);
    }
    catch (Base::Exception& e) {
        e.ReportException();
    }
}

void TaskDressUpParameters::deleteRef(QListWidget* widget)
{
    // delete any selections since the reference(s) being deleted might be highlighted
    Gui::Selection().clearSelection();

    // get the list of items to be deleted
    QList<QListWidgetItem*> selectedList = widget->selectedItems();

    PartDesign::DressUp* pcDressUp = DressUpView->getObject<PartDesign::DressUp>();
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

    if (current == nullptr){
        setSelectionMode(none);
        return;
    }

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
            tryAddSelection(docName, objName, subName);
            this->blockSelection(block);
        }
    }
}

void TaskDressUpParameters::tryAddSelection(const std::string& doc,
                                            const std::string& obj,
                                            const std::string& sub)
{
    try {
        Gui::Selection().addSelection(doc.c_str(), obj.c_str(), sub.c_str(), 0, 0, 0);
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
    catch (const Standard_Failure& e) {
        Base::Console().Error("OCC error: %s\n", e.GetMessageString());
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
    {
        auto& rcCmdMgr = Gui::Application::Instance->commandManager();
        auto shortcut = rcCmdMgr.getCommandByName("Std_Delete")->getShortcut();
        deleteAction->setShortcut(QKeySequence(shortcut));
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    // display shortcut behind the context menu entry
    deleteAction->setShortcutVisibleInContextMenu(true);
#endif
    parentList->addAction(deleteAction);
    parentList->setContextMenuPolicy(Qt::ActionsContextMenu);
}

bool TaskDressUpParameters::event(QEvent* event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(event);  // NOLINT
        if (deleteAction && Gui::QtTools::matches(kevent, deleteAction->shortcut())) {
            kevent->accept();
            return true;
        }
        if (addAllEdgesAction && Gui::QtTools::matches(kevent, addAllEdgesAction->shortcut())) {
            kevent->accept();
            return true;
        }
    }

    return TaskBox::event(event);
}

void TaskDressUpParameters::keyPressEvent(QKeyEvent* ke)
{
    if (deleteAction && deleteAction->isEnabled() &&
        Gui::QtTools::matches(ke, deleteAction->shortcut())) {
        deleteAction->trigger();
        return;
    }
    if (addAllEdgesAction && addAllEdgesAction->isEnabled() &&
        Gui::QtTools::matches(ke, addAllEdgesAction->shortcut())) {
        addAllEdgesAction->trigger();
        return;
    }

    TaskBox::keyPressEvent(ke);
}

const std::vector<std::string> TaskDressUpParameters::getReferences() const
{
    PartDesign::DressUp* pcDressUp = DressUpView->getObject<PartDesign::DressUp>();
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

void TaskDressUpParameters::setDressUpVisibility(bool visible)
{
    App::DocumentObject* base = getBase();
    if (base) {
        App::DocumentObject* duv = DressUpView->getObject();
        if (duv->Visibility.getValue() != visible) {
            duv->Visibility.setValue(visible);
        }
        if (base->Visibility.getValue() == visible) {
            base->Visibility.setValue(!visible);
        }
    }
}

void TaskDressUpParameters::hideObject()
{
    setDressUpVisibility(false);
}

void TaskDressUpParameters::showObject()
{
    setDressUpVisibility(true);
}

ViewProviderDressUp* TaskDressUpParameters::getDressUpView() const
{
    return DressUpView.expired() ? nullptr : DressUpView.get();
}

Part::Feature* TaskDressUpParameters::getBase() const
{
    if (ViewProviderDressUp* vp = getDressUpView()) {
        auto dressUp = vp->getObject<PartDesign::DressUp>();
        // Unlikely but this may throw an exception in case we are started to edit an object which
        // base feature was deleted. This exception will be likely unhandled inside the dialog and
        // pass upper. But an error message inside the report view is better than a SEGFAULT.
        // Generally this situation should be prevented in ViewProviderDressUp::setEdit()
        return dressUp->getBaseObject();
    }

    return nullptr;
}

void TaskDressUpParameters::setSelectionMode(selectionModes mode)
{
    if (DressUpView.expired()) {
        return;
    }

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
    auto pcDressUp = DressUpView->getObject<PartDesign::DressUp>();
    auto base = pcDressUp->Base.getValue();
    std::vector<std::string> newSubList;
    bool changed = false;
    auto& shadowSubs = pcDressUp->Base.getShadowSubs();
    for ( auto &shadowSub : shadowSubs ) {
        auto displayName = shadowSub.oldName;
        // If there is a missing tag on the shadow sub, take a guess at a new name.
        if ( boost::starts_with(shadowSub.oldName,Data::MISSING_PREFIX)) {
            Part::Feature::guessNewLink(displayName, base, shadowSub.newName.c_str());
            newSubList.emplace_back(displayName);
            changed = true;
        }
    }
    if ( changed )
        pcDressUp->Base.setValue(base, newSubList);
}

TaskDlgDressUpParameters::~TaskDlgDressUpParameters() = default;

//==== calls from the TaskView ===============================================================

bool TaskDlgDressUpParameters::accept()
{
    getViewObject<ViewProviderDressUp>()->highlightReferences(false);
    std::vector<std::string> refs = parameter->getReferences();
    std::stringstream str;
    str << Gui::Command::getObjectCmd(getObject()) << ".Base = ("
        << Gui::Command::getObjectCmd(parameter->getBase()) << ",[";
    for (const auto & ref : refs)
        str << "\"" << ref << "\",";
    str << "])";
    Gui::Command::runCommand(Gui::Command::Doc,str.str().c_str());
    return TaskDlgFeatureParameters::accept();
}

bool TaskDlgDressUpParameters::reject()
{
    getViewObject<ViewProviderDressUp>()->highlightReferences(false);
    return TaskDlgFeatureParameters::reject();
}

#include "moc_TaskDressUpParameters.cpp"
