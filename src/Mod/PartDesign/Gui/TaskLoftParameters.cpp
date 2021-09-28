/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <sstream>
# include <QAction>
# include <QRegExp>
# include <QTextStream>
# include <QMessageBox>
# include <Precision.hxx>
# include <QKeyEvent>
#endif

#include "ui_TaskLoftParameters.h"
#include "TaskLoftParameters.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Selection.h>
#include <Gui/CommandT.h>
#include <Gui/MetaTypes.h>
#include <Mod/PartDesign/App/FeatureLoft.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/Body.h>
#include "TaskSketchBasedParameters.h"
#include "ReferenceSelection.h"
#include "Utils.h"

using namespace PartDesignGui;
using namespace Gui;

static bool _Busy;

class LoftProfileSelectionGate : public Gui::SelectionGate
{
public:
    LoftProfileSelectionGate(TaskLoftParameters *master,
                         const Gui::ViewProviderDocumentObject *vp)
        :master(master)
    {
        if (!vp)
            return;
        objT = App::DocumentObjectT(vp->getObject());
        inList = vp->getObject()->getInListEx(true);
        inList.insert(vp->getObject());
    }

    ~LoftProfileSelectionGate()
    {
        master->exitSelectionMode();
    }

    bool allow(App::Document*, App::DocumentObject* pObj, const char *)
    {
        auto loft = static_cast<PartDesign::Loft*>(objT.getObject());
        if (!loft) {
            this->notAllowedReason = QT_TR_NOOP("Loft feature not found.");
            return false;
        }
        if (inList.count(pObj)) {
            this->notAllowedReason = QT_TR_NOOP("Selecting this will cause circular dependency.");
            return false;
        }
        if (loft->Sections.find(pObj->getNameInDocument())) {
            this->notAllowedReason = QT_TR_NOOP("Section object cannot be used as profile.");
            return false;
        }
        return true;
    }

private:
    App::DocumentObjectT objT;
    std::set<App::DocumentObject*> inList;
    TaskLoftParameters *master;
};

class LoftSectionSelectionGate : public Gui::SelectionGate
{
public:
    LoftSectionSelectionGate(TaskLoftParameters *master,
                         const Gui::ViewProviderDocumentObject *vp)
        :master(master)
    {
        if (vp) {
            objT = App::DocumentObjectT(vp->getObject());
            inList = vp->getObject()->getInListEx(true);
            inList.insert(vp->getObject());
        }
    }

    ~LoftSectionSelectionGate()
    {
        master->exitSelectionMode();
    }

    bool allow(App::Document*, App::DocumentObject* pObj, const char*)
    {
        auto loft = static_cast<PartDesign::Loft*>(objT.getObject());
        if (!loft) {
            this->notAllowedReason = QT_TR_NOOP("Loft feature not found.");
            return false;
        }
        if (inList.count(pObj)) {
            this->notAllowedReason = QT_TR_NOOP("Selecting this will cause circular dependency.");
            return false;
        }
        if (loft->Profile.getValue() == pObj) {
            this->notAllowedReason = QT_TR_NOOP("Profile object cannot be used as section.");
            return false;
        }
        if (loft->Profile.getValue()) {
            if (!wireCount) {
                wireCount = Part::Feature::getTopoShape(
                        loft->Profile.getValue()).countSubShapes(TopAbs_WIRE);
            }
            if (wireCount>0 && Part::Feature::getTopoShape(
                                    pObj).countSubShapes(TopAbs_WIRE) != wireCount)
            {
                this->notAllowedReason = QT_TR_NOOP("Section object must have the same number of wires.");
                return false;
            }
        } else
            wireCount = 0;
        return true;
    }

private:
    App::DocumentObjectT objT;
    std::set<App::DocumentObject*> inList;
    TaskLoftParameters *master;
    unsigned wireCount = 0;
};


/* TRANSLATOR PartDesignGui::TaskLoftParameters */

TaskLoftParameters::TaskLoftParameters(ViewProviderLoft *LoftView,bool /*newObj*/, QWidget *parent)
    : TaskSketchBasedParameters(LoftView, parent, "PartDesign_AdditiveLoft", tr("Loft parameters"))
    , ui(new Ui_TaskLoftParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    if (LoftView) {
        PartDesign::Loft* pcLoft = static_cast<PartDesign::Loft*>(LoftView->getObject());
        ui->checkBoxRuled->setToolTip(QApplication::translate(
                    "Property", pcLoft->Ruled.getDocumentation()));
        ui->checkBoxClosed->setToolTip(QApplication::translate(
                    "Property", pcLoft->Closed.getDocumentation()));
        ui->checkBoxSplitProfile->setToolTip(QApplication::translate(
                    "Property", pcLoft->SplitProfile.getDocumentation()));
    }

    ui->profileBaseEdit->installEventFilter(this);
    ui->profileBaseEdit->setMouseTracking(true);
    ui->listWidgetReferences->installEventFilter(this);
    ui->listWidgetReferences->setMouseTracking(true);

    ui->profileBaseEdit->setReadOnly(true);

    connect(ui->buttonProfileBase, SIGNAL(clicked(bool)),
            this, SLOT(onProfileButton(bool)));
    connect(ui->buttonRefAdd, SIGNAL(clicked(bool)),
            this, SLOT(onRefButtonAdd(bool)));
    connect(ui->checkBoxRuled, SIGNAL(toggled(bool)),
            this, SLOT(onRuled(bool)));
    connect(ui->checkBoxClosed, SIGNAL(toggled(bool)),
            this, SLOT(onClosed(bool)));
    connect(ui->checkBoxSplitProfile, SIGNAL(toggled(bool)),
            this, SLOT(onSplitProfile(bool)));
    connect(ui->listWidgetReferences, SIGNAL(itemEntered(QListWidgetItem*)),
            this, SLOT(onItemEntered(QListWidgetItem*)));
    connect(ui->listWidgetReferences, SIGNAL(itemSelectionChanged()),
            this, SLOT(onItemSelectionChanged()));

    // Create context menu
    QAction* remove = new QAction(tr("Remove"), this);
    remove->setShortcut(QKeySequence::Delete);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    // display shortcut behind the context menu entry
    remove->setShortcutVisibleInContextMenu(true);
#endif
    ui->listWidgetReferences->addAction(remove);
    ui->listWidgetReferences->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(remove, SIGNAL(triggered()), this, SLOT(onDeleteSection()));

    connect(ui->listWidgetReferences->model(),
        SIGNAL(rowsMoved(QModelIndex, int, int, QModelIndex, int)), this, SLOT(indexesMoved()));

    this->initUI(proxy);
    this->groupLayout()->addWidget(proxy);
    refresh();

    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
    connProfile = loft->Profile.signalChanged.connect(
        [this](const App::Property &) {toggleShowOnTop(vp, lastProfile, "Profile", true);});
    connSections = loft->Sections.signalChanged.connect(
        [this](const App::Property &) {toggleShowOnTop(vp, lastSections, "Sections", true);});

    // call updateUI until Gui::Document::setEdit() finishes;
    QMetaObject::invokeMethod(this, "updateUI", Qt::QueuedConnection);
}

TaskLoftParameters::~TaskLoftParameters()
{
    exitSelectionMode();
}

void TaskLoftParameters::updateUI()
{
    toggleShowOnTop(vp, lastProfile, "Profile", true);
    toggleShowOnTop(vp, lastSections, "Sections", true);
}

void TaskLoftParameters::addItem(App::DocumentObject *obj, bool select)
{
    QString label = QString::fromUtf8(obj->Label.getValue());
    QListWidgetItem* item = new QListWidgetItem();
    auto vp = Gui::Application::Instance->getViewProvider(obj);
    if (vp)
        item->setIcon(vp->getIcon());
    item->setText(label);
    item->setData(Qt::UserRole, QVariant::fromValue(App::SubObjectT(obj)));
    ui->listWidgetReferences->addItem(item);
    if (select) {
        QSignalBlocker blocker(ui->listWidgetReferences);
        item->setSelected(true);
        ui->listWidgetReferences->scrollToItem(item);
    }
}

void TaskLoftParameters::refresh()
{
    if (!vp || !vp->getObject())
        return;

    // Temporarily prevent unnecessary feature recomputes
    for (QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(true);

    //add the profiles
    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
    App::DocumentObject* profile = loft->Profile.getValue();
    if (profile) {
        QString label = QString::fromUtf8(profile->Label.getValue());
        ui->profileBaseEdit->setText(label);
    }

    for (auto obj : loft->Sections.getValues())
        addItem(obj);

    // get options
    ui->checkBoxRuled->setChecked(loft->Ruled.getValue());
    ui->checkBoxClosed->setChecked(loft->Closed.getValue());
    ui->checkBoxSplitProfile->setChecked(loft->SplitProfile.getValue());

    for (QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(false);

}

void TaskLoftParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (!vp || _Busy)
        return;

    if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        QSignalBlocker blocker(ui->listWidgetReferences);
        ui->listWidgetReferences->selectionModel()->clearSelection();
        return;
    }

    if (msg.Type != Gui::SelectionChanges::AddSelection)
        return;

    auto obj = msg.Object.getObject();
    if (!obj)
        return;

    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());

    switch(selectionMode) {
    case refProfile: {
        if (obj == loft->Profile.getValue())
            break;
        App::SubObjectT ref(msg.pOriginalMsg ? msg.pOriginalMsg->Object : msg.Object);
        ref = PartDesignGui::importExternalElement(ref);
        auto refObj = ref.getSubObject();
        if (refObj) {
            ui->profileBaseEdit->setText(QString::fromUtf8(refObj->Label.getValue()));
            try {
                setupTransaction();
                loft->Profile.setValue(refObj);
                recomputeFeature();
            } catch (Base::Exception &e) {
                e.ReportException();
            }
            exitSelectionMode();
        }
        break;
    }
    case refAdd: {
        App::SubObjectT ref(msg.pOriginalMsg ? msg.pOriginalMsg->Object : msg.Object);
        ref = PartDesignGui::importExternalElement(ref);
        auto refObj = ref.getSubObject();
        if (refObj) {
            auto sections = loft->Sections.getValues();
            sections.push_back(refObj);
            addItem(refObj, true);
            try {
                setupTransaction();
                loft->Sections.setValues(sections);
                recomputeFeature();
            } catch (Base::Exception &e) {
                e.ReportException();
            }
        }
        break;
    }
    default:
        break;
    }
}

void TaskLoftParameters::onDeleteSection()
{
    if (!vp)
        return;

    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
    std::vector<App::DocumentObject*> refs = loft->Sections.getValues();

    // Delete the selected profile
    for(auto item : ui->listWidgetReferences->selectedItems()) {
        auto objT = qvariant_cast<App::SubObjectT>(item->data(Qt::UserRole));
        refs.erase(std::remove(refs.begin(), refs.end(), objT.getSubObject()), refs.end());
        delete item;
    }

    if (refs.size() != loft->Sections.getValues().size()) {
        try {
            setupTransaction();
            loft->Sections.setValues(refs);
            recomputeFeature();
        } catch (Base::Exception &e) {
            e.ReportException();
        }
    }
}

void TaskLoftParameters::indexesMoved()
{
    if (!vp)
        return;

    QAbstractItemModel* model = qobject_cast<QAbstractItemModel*>(sender());
    if (!model)
        return;

    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
    std::vector<App::DocumentObject*> originals = loft->Sections.getValues();

    QByteArray name;
    int rows = model->rowCount();
    for (int i = 0; i < rows; i++) {
        QModelIndex index = model->index(i, 0);
        name = index.data(Qt::UserRole).toByteArray().constData();
        originals[i] = loft->getDocument()->getObject(name.constData());
    }

    try {
        setupTransaction();
        loft->Sections.setValues(originals);
        recomputeFeature();
    } catch (Base::Exception &e) {
        e.ReportException();
    }
}

void TaskLoftParameters::exitSelectionMode() {

    if (selectionMode == none)
        return;
    selectionMode = none;
    ui->buttonProfileBase->setChecked(false);
    ui->buttonRefAdd->setChecked(false);
    Gui::Selection().clearSelection();
    Gui::Selection().rmvSelectionGate();
}

void TaskLoftParameters::changeEvent(QEvent * /*e*/)
{
}

void TaskLoftParameters::onClosed(bool val) {
    if (!vp)
        return;
    try {
        setupTransaction();
        static_cast<PartDesign::Loft*>(vp->getObject())->Closed.setValue(val);
        recomputeFeature();
    } catch (Base::Exception &e) {
        e.ReportException();
    }
}

void TaskLoftParameters::onRuled(bool val) {
    if (!vp)
        return;
    try {
        setupTransaction();
        static_cast<PartDesign::Loft*>(vp->getObject())->Ruled.setValue(val);
        recomputeFeature();
    } catch (Base::Exception &e) {
        e.ReportException();
    }
}

void TaskLoftParameters::onSplitProfile(bool val) {
    if (!vp)
        return;
    try {
        setupTransaction();
        static_cast<PartDesign::Loft*>(vp->getObject())->SplitProfile.setValue(val);
        recomputeFeature();
    } catch (Base::Exception &e) {
        e.ReportException();
    }
}

void TaskLoftParameters::onProfileButton(bool checked)
{
    if (checked) {
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new LoftProfileSelectionGate(this, vp));
        selectionMode = refProfile;
    }
    else {
        exitSelectionMode();
    }
}

void TaskLoftParameters::onRefButtonAdd(bool checked) {
    if (!vp)
        return;
    if (checked) {
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new LoftSectionSelectionGate(this, vp));
        selectionMode = refAdd;
    } else {
        exitSelectionMode();
    }
}

void TaskLoftParameters::onItemEntered(QListWidgetItem *item)
{
    if (!vp)
        return;
    auto objT = qvariant_cast<App::SubObjectT>(item->data(Qt::UserRole));
    PartDesignGui::highlightObjectOnTop(objT);
}

bool TaskLoftParameters::eventFilter(QObject *o, QEvent *ev)
{
    switch(ev->type()) {
    case QEvent::Leave:
        Gui::Selection().rmvPreselect();
        break;
    case QEvent::Enter:
        if (vp && o == ui->profileBaseEdit) {
            auto loft = static_cast<PartDesign::Loft*>(vp->getObject());
            auto obj = loft->Profile.getValue();
            if (obj)
                PartDesignGui::highlightObjectOnTop(obj);
        }
        break;
    case QEvent::ShortcutOverride:
    case QEvent::KeyPress: {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(ev);
        if (o == ui->listWidgetReferences && kevent->modifiers() == Qt::NoModifier) {
            if (kevent->key() == Qt::Key_Delete) {
                kevent->accept();
                if (ev->type() == QEvent::KeyPress)
                    onDeleteSection();
            }
        }
        break;
    }
    default:
        break;
    }
    return false;
}

void TaskLoftParameters::onItemSelectionChanged()
{
    if (!vp)
        return;
    auto items = ui->listWidgetReferences->selectedItems();
    Base::StateLocker lock(_Busy);
    if (items.isEmpty() || items.size() == 1) {
        Gui::Selection().selStackPush();
        Gui::Selection().clearSelection();
    }
    for (auto item : items) {
        PartDesignGui::selectObjectOnTop(
                qvariant_cast<App::SubObjectT>(item->data(Qt::UserRole)), true);
    }
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgLoftParameters::TaskDlgLoftParameters(ViewProviderLoft *LoftView,bool newObj)
   : TaskDlgSketchBasedParameters(LoftView)
{
    assert(LoftView);
    parameter  = new TaskLoftParameters(LoftView,newObj);

    Content.push_back(parameter);
}

TaskDlgLoftParameters::~TaskDlgLoftParameters()
{
}


bool TaskDlgLoftParameters::accept()
{
    PartDesign::Loft* pcLoft = static_cast<PartDesign::Loft*>(getLoftView()->getObject());
    try {
        parameter->setupTransaction();
        Gui::cmdGuiDocument(pcLoft, "resetEdit()");
        Gui::cmdAppDocument(pcLoft, "recompute()");
        if (!vp->getObject()->isValid())
            throw Base::RuntimeError(vp->getObject()->getStatusString());
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromUtf8(e.what()));
        return false;
    }

    return true;
}

//==== calls from the TaskView ===============================================================


#include "moc_TaskLoftParameters.cpp"
