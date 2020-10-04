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
# include <QApplication>
# include <QListWidget>
# include <QListWidgetItem>
# include <QAction>
# include <QKeyEvent>
# include <QPushButton>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include "TaskDressUpParameters.h"
#include <Base/Tools.h>
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
#include <Gui/ViewParams.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
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
    , allowFaces(selectFaces)
    , allowEdges(selectEdges)
{
    // remember initial transaction ID
    App::GetApplication().getActiveTransaction(&transactionID);

    selectionMode = none;
    showObject();

    onTopEnabled = Gui::ViewParams::instance()->getShowSelectionOnTop();
    if(!onTopEnabled)
        Gui::ViewParams::instance()->setShowSelectionOnTop(true);

    connUndo = App::GetApplication().signalUndo.connect(boost::bind(&TaskDressUpParameters::refresh, this));
    connRedo = App::GetApplication().signalRedo.connect(boost::bind(&TaskDressUpParameters::refresh, this));

    connDelete = Gui::Application::Instance->signalDeletedObject.connect(
        [this](const Gui::ViewProvider &Obj) {
            if(this->DressUpView == &Obj)
                this->DressUpView = nullptr;
        });

    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));
}

TaskDressUpParameters::~TaskDressUpParameters()
{
    // make sure to remove selection gate in all cases
    Gui::Selection().rmvSelectionGate();

    if(!onTopEnabled)
        Gui::ViewParams::instance()->setShowSelectionOnTop(false);

    clearButtons(none);
    exitSelectionMode();
}

void TaskDressUpParameters::setupTransaction() {
    if(!DressUpView)
        return;

    int tid = 0;
    const char *name = App::GetApplication().getActiveTransaction(&tid);
    if(tid && tid == transactionID)
        return;

    std::string n("Edit ");
    n += DressUpView->getObject()->getNameInDocument();
    if(!name || n!=name)
        tid = App::GetApplication().setActiveTransaction(n.c_str());

    if (!transactionID)
        transactionID = tid;
}

void TaskDressUpParameters::setup(QLabel *label, QListWidget *widget, QPushButton *_btnAdd, bool touched)
{
    if(!DressUpView)
        return;
    auto* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    if(!pcDressUp || !pcDressUp->Base.getValue())
        return;

    // Remember the initial transaction ID
    App::GetApplication().getActiveTransaction(&transactionID);

    messageLabel = label;
    messageLabel->hide();
    messageLabel->setWordWrap(true);

    btnAdd = _btnAdd;
    connect(btnAdd, SIGNAL(toggled(bool)), this, SLOT(onButtonRefAdd(bool)));
    btnAdd->setCheckable(true);

    listWidget = widget;
    listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    listWidget->setMouseTracking(true);
    listWidget->installEventFilter(this);

    connect(listWidget, SIGNAL(itemSelectionChanged()),
        this, SLOT(onItemSelectionChanged()));
    connect(listWidget, SIGNAL(itemEntered(QListWidgetItem*)),
        this, SLOT(onItemEntered(QListWidgetItem*)));

    if(!deleteAction) {
        // Create context menu
        deleteAction = new QAction(tr("Remove"), this);
        deleteAction->setShortcut(QKeySequence::Delete);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
        // display shortcut behind the context menu entry
        deleteAction->setShortcutVisibleInContextMenu(true);
#endif
        widget->addAction(deleteAction);
        connect(deleteAction, SIGNAL(triggered()), this, SLOT(onRefDeleted()));
        widget->setContextMenuPolicy(Qt::ActionsContextMenu);
    }

    if(populate() || touched) {
        setupTransaction();
        recompute();
    }
}

void TaskDressUpParameters::refresh()
{
    populate(true);
}

bool TaskDressUpParameters::populate(bool refresh)
{
    if(!listWidget || !DressUpView)
        return false;

    QSignalBlocker blocker(listWidget);
    listWidget->clear();

    auto* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    if(!pcDressUp || !pcDressUp->Base.getValue())
        return false;

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
        if(refresh || sub.first.empty() || baseShape.isNull()) {
            listWidget->addItem(QString::fromStdString(sub.second));
            continue;
        }
        const auto &ref = sub.first;
        Part::TopoShape element;
        try {
            element = baseShape.getSubShape(ref.c_str());
        }catch(...) {}
        if(!element.isNull())  {
            listWidget->addItem(QString::fromStdString(sub.second));
            continue;
        }
        FC_WARN("missing element reference: " << pcDressUp->getFullName() << "." << ref);
        bool popped = false;
        for(auto &name : Part::Feature::getRelatedElements(base,ref.c_str())) {
            if(!subSet.insert(name.second).second || !subSet.insert(name.first).second)
                continue;
            FC_WARN("guess element reference: " << ref << " -> " << name.first);
            listWidget->addItem(QString::fromStdString(name.second));
            if(!popped) {
                refs.pop_back();
                touched = true;
                popped = true;
            }
            refs.push_back(name.second);
        }
        if(!popped) {
            std::string missingSub = refs.back();
            if(!boost::starts_with(missingSub,Data::ComplexGeoData::missingPrefix()))
                missingSub = Data::ComplexGeoData::missingPrefix()+missingSub;
            auto item = new QListWidgetItem(listWidget);
            item->setText(QString::fromStdString(missingSub));
            item->setForeground(Qt::red);
            refs.back() = ref; // use new style name for future guessing
        }
    }

    if(touched) {
        setupTransaction();
        pcDressUp->Base.setValue(base,refs);
    }
    return touched;
}

bool TaskDressUpParameters::showOnTop(bool enable,
        std::vector<App::SubObjectT> &&objs)
{
    if(onTopObjs.size()) {
        auto doc = Gui::Application::Instance->getDocument(
                onTopObjs.front().getDocumentName().c_str());
        if(doc) {
            auto view = Base::freecad_dynamic_cast<Gui::View3DInventor>(doc->getActiveView());
            if(view) {
                auto viewer = view->getViewer();
                for(auto &obj : onTopObjs) {
                    viewer->checkGroupOnTop(Gui::SelectionChanges(SelectionChanges::RmvSelection,
                                obj.getDocumentName().c_str(),
                                obj.getObjectName().c_str(),
                                obj.getSubName().c_str()), true);
                }
            }
        }
    }
    onTopObjs.clear();
    if(!enable)
        return true;

    if(objs.empty())
        objs.push_back(getInEdit());

    auto doc = Gui::Application::Instance->getDocument(
            objs.front().getDocumentName().c_str());
    if(!doc)
        return false;
    auto view = Base::freecad_dynamic_cast<Gui::View3DInventor>(doc->getActiveView());
    if(!view)
        return false;
    auto viewer = view->getViewer();
    for(auto &obj : objs) {
        viewer->checkGroupOnTop(Gui::SelectionChanges(SelectionChanges::AddSelection,
                    obj.getDocumentName().c_str(),
                    obj.getObjectName().c_str(),
                    obj.getSubName().c_str()), true);
    }
    onTopObjs = std::move(objs);
    return true;
}

void TaskDressUpParameters::clearButtons(const selectionModes notThis)
{
    if (notThis != refToggle && btnAdd) {
        QSignalBlocker blocker(btnAdd);
        btnAdd->setChecked(false);
        showOnTop(false);
    }
    if(DressUpView)
        DressUpView->highlightReferences(false);
}

void TaskDressUpParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if(!listWidget || !DressUpView)
        return;

    bool addSel = false;
    switch(msg.Type) {
    case Gui::SelectionChanges::ClrSelection: {
        showMessage();
        QSignalBlocker blocker(listWidget);
        listWidget->selectionModel()->clearSelection();
        break;
    }
    case Gui::SelectionChanges::RmvSelection:
        break;
    case Gui::SelectionChanges::AddSelection:
        addSel = true;
        break;
    default:
        return;
    }

    App::DocumentObject* base = this->getBase();
    auto selObj = msg.Object.getObject();
    if(!base || !selObj)
        return;

    auto* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    if(selObj == pcDressUp && addSel) {
        // The dress up feature itself is selected, trace the selected element
        // back to its base
        auto history = pcDressUp->getElementHistory(pcDressUp,msg.pSubName);
        const char *element = 0;
        for(auto &hist : history) {
            if(hist.obj != base
                    || (!allowFaces && boost::starts_with(hist.element,"Face"))
                    || (!allowEdges && boost::starts_with(hist.element,"Edge")))
            {
                continue;
            }
            if(element) {
                showMessage("Ambiguious selection");
                return;
            }
            element = hist.element.c_str();
        }
        if(element) {
            std::string subname = Data::ComplexGeoData::elementMapPrefix()+element;
            std::vector<App::SubObjectT> sels;
            sels.emplace_back(base, subname.c_str());

            // base element found, check if it is already in the list widget
            auto items = listWidget->findItems(
                    QString::fromLatin1(sels.back().getOldElementName().c_str()), Qt::MatchExactly);

            if(items.size()) {
                if(selectionMode != refToggle) {
                    // element found, but we are not toggling, select and exit
                    QSignalBlocker blocker(listWidget);
                    for(auto item : items)
                        item->setSelected(true);
                    return;
                }

                // We are toggling, so delete the found item, and call syncItems() below
                if(onTopObjs.empty()) {
                    for(auto item : items)
                        delete item;
                    sels.clear();
                }
            }

            if(msg.pOriginalMsg) {
                // We about dress up the current selected element, meaning that
                // this original element will be gone soon. So remove it from
                // the selection to avoid warning.
                Gui::Selection().rmvSelection(msg.pOriginalMsg->pDocName,
                                              msg.pOriginalMsg->pObjectName,
                                              msg.pOriginalMsg->pSubName);
            }

            if(onTopObjs.empty()) {
                // If no on top display at the moment, just sync the reference
                // selection.
                syncItems(sels, false);
            } else {
                // If there is on top display, replace the current selection
                // with the base element selection, and let it trigger an
                // subsequent onSelectionChanged() event, which will be handled
                // by code below.
                std::string sub;
                auto obj = getInEdit(sub);
                if(obj) {
                    sub += subname;
                    Gui::Selection().addSelection(obj->getDocument()->getName(),
                            obj->getNameInDocument(), sub.c_str());
                }
            }
        }
        return;
    }

    showMessage();

    if(!base || base != selObj)
        return;

    bool check = true;
    auto items = listWidget->findItems(QString::fromLatin1(msg.pSubName), Qt::MatchExactly);
    if(items.size()) {
        QSignalBlocker blocker(listWidget);
        if(selectionMode == refToggle && addSel) {
            check = false;
            for(auto item : items)
                delete item;
        } else {
            for(auto item : items) {
                if(!item->isSelected())
                    item->setSelected(addSel);
            }
        }
    }

    if(addSel) {
        if(check)
            syncItems(Gui::Selection().getSelectionT());
        else
            syncItems();
    }
}

void TaskDressUpParameters::onButtonRefAdd(bool checked)
{
    if(!DressUpView)
        return;

    // We no longer use highlightReferences now, but ShowOnTopSelection
    // instead. Turning off here just to be safe.
    DressUpView->highlightReferences(false);

    if(!checked) {
        clearButtons(none);
        exitSelectionMode();
        return;
    }

    std::string subname;
    auto obj = getInEdit(subname);
    if(obj) {
        blockConnection(true);
        QSignalBlocker blocker(listWidget);
        for(auto item : listWidget->selectedItems()) {
            std::string sub = subname + item->text().toStdString();
            Gui::Selection().rmvSelection(obj->getDocument()->getName(),
                    obj->getNameInDocument(), sub.c_str());
            delete item;
        }
        blockConnection(false);
        syncItems(Gui::Selection().getSelectionT());
    }
    exitSelectionMode();
    selectionMode = refToggle;
    clearButtons(refToggle);
    Gui::Selection().clearSelection();
    Gui::Selection().addSelectionGate(
            new ReferenceSelection(this->getBase(), allowEdges, allowFaces, false));
}

void TaskDressUpParameters::onRefDeleted() {
    if(!listWidget || !DressUpView)
        return;

    QSignalBlocker blocker(listWidget);
    for(auto item : listWidget->selectedItems())
        delete item;

    DressUpView->highlightReferences(false);
    syncItems();
}

bool TaskDressUpParameters::syncItems(const std::vector<App::SubObjectT> &sels, bool select) {
    if(!DressUpView)
        return false;

    std::set<std::string> subset;
    std::vector<std::string> subs;
    for(int i=0, count=listWidget->count();i<count;++i) {
        std::string s = listWidget->item(i)->text().toStdString();
        if(subset.insert(s).second)
            subs.push_back(std::move(s));
    }

    auto* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    App::DocumentObject* base = pcDressUp->Base.getValue();

    if(sels.size()) {
        QSignalBlocker blocker(listWidget);
        for(auto &sel : sels) {
            if(sel.getObject() != base)
                continue;
            std::string element = sel.getOldElementName();
            if(subset.count(element))
                continue;
            if((allowEdges && boost::starts_with(element,"Edge"))
                    || (allowFaces && boost::starts_with(element,"Face")))
            {
                subset.insert(element);
                auto item = new QListWidgetItem(QString::fromLatin1(element.c_str()), listWidget);
                if(select)
                    item->setSelected(true);
                subs.push_back(std::move(element));
            }
        }
    }

    if(subs == pcDressUp->Base.getSubValues())
        return false;

    setupTransaction();
    pcDressUp->Base.setValue(base, subs);
    recompute();
    return true;
}

void TaskDressUpParameters::recompute() {
    if(DressUpView) {
        DressUpView->getObject()->recomputeFeature();
        showMessage();
    }
}

void TaskDressUpParameters::showMessage(const char *msg) {
    if(!messageLabel || !DressUpView)
        return;

    auto obj = DressUpView->getObject();
    if(!msg && obj->isError())
        msg = obj->getStatusString();
    if(!msg || !msg[0])
        messageLabel->hide();
    else {
        messageLabel->setText(QString::fromLatin1("<font color='red'>%1<br/>%2</font>").arg(
                tr("Recompute failed"), tr(msg)));
        messageLabel->show();
    }
}

void TaskDressUpParameters::onItemSelectionChanged()
{
    if(!listWidget)
        return;

    if(selectionMode == refToggle) {
        onRefDeleted();
        return;
    }

    std::string subname;
    auto obj = getInEdit(subname);
    if(!obj)
        return;

    std::vector<std::string> subs;
    for(auto item : listWidget->selectedItems()) 
        subs.push_back(subname + item->text().toStdString());

    if(subs.size()) {
        blockConnection(true);
        Gui::Selection().clearSelection();
        Gui::Selection().addSelections(obj->getDocument()->getName(),
                obj->getNameInDocument(), subs);
        blockConnection(false);
    }
}

App::SubObjectT TaskDressUpParameters::getInEdit(App::DocumentObject *base, const char *sub) {
    std::string subname;
    auto obj = getInEdit(subname,base);
    if(obj) {
        if(sub)
            subname += sub;
        return App::SubObjectT(obj,subname.c_str());
    }
    return App::SubObjectT();
}

App::DocumentObject *TaskDressUpParameters::getInEdit(std::string &subname, App::DocumentObject *base)
{
    if(!DressUpView)
        return nullptr;

    auto* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    if(!base) {
        base = pcDressUp->Base.getValue();
        if(!base)
            return nullptr;
    }
    auto editDoc = Gui::Application::Instance->editDocument();
    if(!editDoc)
        return nullptr;

    ViewProviderDocumentObject *editVp = nullptr;
    editDoc->getInEdit(&editVp,&subname);
    if(!editVp)
        return nullptr;

    auto sobjs = editVp->getObject()->getSubObjectList(subname.c_str());
    if(sobjs.empty())
        return nullptr;

    for(;;) {
        if(sobjs.back() == base)
            break;
        sobjs.pop_back();
        if(sobjs.empty())
            break;
        std::string sub(base->getNameInDocument());
        sub += ".";
        if(sobjs.back()->getSubObject(sub.c_str())) {
            sobjs.push_back(base);
            break;
        }
    }

    std::ostringstream ss;
    for(size_t i=1;i<sobjs.size();++i)
        ss << sobjs[i]->getNameInDocument() << '.';

    subname = ss.str();
    return sobjs.size()?sobjs.front():base;
}

void TaskDressUpParameters::onItemEntered(QListWidgetItem *)
{
    enteredObject = listWidget;
    timer->start(100);
}

void TaskDressUpParameters::onTimer() {
    if(enteredObject != listWidget || !listWidget)
        return;

    auto item = listWidget->itemAt(listWidget->viewport()->mapFromGlobal(QCursor::pos()));
    if(!item) {
        Gui::Selection().rmvPreselect();
        return;
    }
    std::string subname;
    auto obj = getInEdit(subname);
    if(obj) {
        subname +=item->text().toStdString();
        Gui::Selection().setPreselect(obj->getDocument()->getName(),
                obj->getNameInDocument(), subname.c_str(),0,0,0,2);
    }
}

std::vector<std::string> TaskDressUpParameters::getReferences() const
{
    if(!DressUpView) 
        return {};

    PartDesign::DressUp* pcDressUp = static_cast<PartDesign::DressUp*>(DressUpView->getObject());
    std::vector<std::string> result = pcDressUp->Base.getSubValues();
    return result;
}

void TaskDressUpParameters::hideObject()
{
    App::DocumentObject* base = getBase();
    if(base)
        base->Visibility.setValue(true);
}

void TaskDressUpParameters::showObject()
{
    if(DressUpView)
        DressUpView->getObject()->Visibility.setValue(true);
}

Part::Feature* TaskDressUpParameters::getBase(void) const
{
    if(!DressUpView)
        throw Base::RuntimeError("No view object");

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

bool TaskDressUpParameters::event(QEvent *e)
{
    if (e && e->type() == QEvent::ShortcutOverride) {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(e);
        if (kevent->modifiers() == Qt::NoModifier) {
            if (kevent->key() == Qt::Key_Delete) {
                kevent->accept();
                return true;
            }
        }
    }
    else if (e && e->type() == QEvent::KeyPress) {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(e);
        if (deleteAction && kevent->key() == Qt::Key_Delete) {
            if (deleteAction->isEnabled())
                deleteAction->trigger();
            return true;
        }
    }
    else if (e && e->type() == QEvent::Leave) {
        Gui::Selection().rmvPreselect();
    }

    return Gui::TaskView::TaskBox::event(e);
}

bool TaskDressUpParameters::eventFilter(QObject *o, QEvent *e)
{
    if(listWidget && o == listWidget) {
        if(e->type() == QEvent::Leave) {
            enteredObject = nullptr;
            timer->stop();
            Gui::Selection().rmvPreselect();
        }
    }
    return Gui::TaskView::TaskBox::eventFilter(o,e);
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
    auto editDoc = Gui::Application::Instance->editDocument();
    if(editDoc && parameter->getTransactionID())
        editDoc->getDocument()->undo(parameter->getTransactionID());

    return TaskDlgFeatureParameters::reject();
}

#include "moc_TaskDressUpParameters.cpp"
