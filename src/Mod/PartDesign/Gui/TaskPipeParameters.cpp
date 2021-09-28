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
# include <QGenericReturnArgument>
# include <QMetaObject>
# include <QKeyEvent>
# include <Precision.hxx>
#endif

#include <boost/algorithm/string/predicate.hpp>

#include "ui_TaskPipeParameters.h"
#include "ui_TaskPipeOrientation.h"
#include "ui_TaskPipeScaling.h"
#include <ui_DlgReference.h>
#include "TaskPipeParameters.h"
#include <App/Application.h>
#include <App/Document.h>
#include <App/Origin.h>
#include <App/Part.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/MetaTypes.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Selection.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Mod/PartDesign/App/FeaturePipe.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/Body.h>
#include "TaskSketchBasedParameters.h"
#include "ReferenceSelection.h"
#include "Utils.h"
#include "TaskFeaturePick.h"

FC_LOG_LEVEL_INIT("PartDesign", true, true)

using namespace PartDesignGui;
using namespace Gui;

static bool _Busy;

class SpineSelectionGate : public Gui::SelectionGate
{
public:
    SpineSelectionGate(TaskPipeParameters *master, const Gui::ViewProviderDocumentObject *vp)
        :parameter(master)
    {
        init(vp);
    }

    SpineSelectionGate(TaskPipeOrientation *master, const Gui::ViewProviderDocumentObject *vp)
        :orientation(master)
    {
        init(vp);
    }

    ~SpineSelectionGate()
    {
        if (parameter)
            parameter->exitSelectionMode();
        else
            orientation->exitSelectionMode();
    }

    void init(const Gui::ViewProviderDocumentObject *vp)
    {
        if (!vp)
            return;
        objT = App::DocumentObjectT(vp->getObject());
        auto prop = getProperty();
        if (prop && !prop->getValue()) {
            inList = vp->getObject()->getInListEx(true);
            inList.insert(vp->getObject());
        }
    }

    App::PropertyLinkSub *getProperty()
    {
        auto pipe = static_cast<PartDesign::Pipe*>(objT.getObject());
        if (!pipe)
            return nullptr;
        return parameter ? &pipe->Spine : &pipe->AuxillerySpine;
    }

    bool allow(App::Document*, App::DocumentObject* pObj, const char* sSubName)
    {
        auto prop = getProperty();
        if (!prop) {
            this->notAllowedReason = QT_TR_NOOP("Pipe feature not found.");
            return false;
        }
        if (prop->getValue() && pObj != prop->getValue()) {
            this->notAllowedReason = QT_TR_NOOP("Not from pipe spine object.");
            return false;
        }
        if (inList.count(pObj)) {
            this->notAllowedReason = QT_TR_NOOP("Selecting this will cause circular dependency.");
            return false;
        }
        if (!boost::starts_with(sSubName, "Edge")
                && !boost::starts_with(sSubName, "Wire")
                && !boost::starts_with(sSubName, "Face"))
        {
            if (!Part::Feature::getTopoShape(pObj, sSubName, true).hasSubShape(TopAbs_EDGE)) {
                this->notAllowedReason = QT_TR_NOOP("No edge found in selection.");
                return false;
            }
        }
        return true;
    }

private:
    App::DocumentObjectT objT;
    std::set<App::DocumentObject*> inList;
    TaskPipeParameters *parameter = nullptr;
    TaskPipeOrientation *orientation = nullptr;
};

class PipeProfileSelectionGate : public Gui::SelectionGate
{
public:
    PipeProfileSelectionGate(TaskPipeParameters *master,
                         const Gui::ViewProviderDocumentObject *vp)
        :master(master)
    {
        if (!vp)
            return;
        objT = App::DocumentObjectT(vp->getObject());
        inList = vp->getObject()->getInListEx(true);
        inList.insert(vp->getObject());
    }

    ~PipeProfileSelectionGate()
    {
        master->exitSelectionMode();
    }

    bool allow(App::Document*, App::DocumentObject* pObj, const char *)
    {
        auto pipe = static_cast<PartDesign::Pipe*>(objT.getObject());
        if (!pipe) {
            this->notAllowedReason = QT_TR_NOOP("Pipe feature not found.");
            return false;
        }
        if (inList.count(pObj)) {
            this->notAllowedReason = QT_TR_NOOP("Selecting this will cause circular dependency.");
            return false;
        }
        if (pipe->Sections.find(pObj->getNameInDocument())) {
            this->notAllowedReason = QT_TR_NOOP("Section object cannot be used as profile.");
            return false;
        }
        return true;
    }

private:
    App::DocumentObjectT objT;
    std::set<App::DocumentObject*> inList;
    TaskPipeParameters *master;
};

class PipeSectionSelectionGate : public Gui::SelectionGate
{
public:
    PipeSectionSelectionGate(TaskPipeScaling *master,
                         const Gui::ViewProviderDocumentObject *vp)
        :master(master)
    {
        if (vp) {
            objT = App::DocumentObjectT(vp->getObject());
            inList = vp->getObject()->getInListEx(true);
            inList.insert(vp->getObject());
        }
    }

    ~PipeSectionSelectionGate()
    {
        master->exitSelectionMode();
    }

    bool allow(App::Document*, App::DocumentObject* pObj, const char*)
    {
        auto pipe = static_cast<PartDesign::Pipe*>(objT.getObject());
        if (!pipe) {
            this->notAllowedReason = QT_TR_NOOP("Pipe feature not found.");
            return false;
        }
        if (inList.count(pObj)) {
            this->notAllowedReason = QT_TR_NOOP("Selecting this will cause circular dependency.");
            return false;
        }
        if (pipe->Profile.getValue() == pObj) {
            this->notAllowedReason = QT_TR_NOOP("Profile object cannot be used as section.");
            return false;
        }
        if (pipe->Sections.find(pObj->getNameInDocument())) {
            this->notAllowedReason = QT_TR_NOOP("Section object already selected.");
            return false;
        }
        if (pipe->Profile.getValue()) {
            if (!wireCount) {
                wireCount = Part::Feature::getTopoShape(
                        pipe->Profile.getValue()).countSubShapes(TopAbs_WIRE);
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
    TaskPipeScaling *master;
    unsigned wireCount = 0;
};

/* TRANSLATOR PartDesignGui::TaskPipeParameters */


//**************************************************************************
//**************************************************************************
// Task Parameter
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskPipeParameters::TaskPipeParameters(ViewProviderPipe *PipeView, bool /*newObj*/, QWidget *parent)
    : TaskSketchBasedParameters(PipeView, parent, "PartDesign_AdditivePipe", tr("Pipe parameters"))
    , ui(new Ui_TaskPipeParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    ui->profileBaseEdit->installEventFilter(this);
    ui->profileBaseEdit->setMouseTracking(true);
    ui->spineBaseEdit->installEventFilter(this);
    ui->spineBaseEdit->setMouseTracking(true);
    ui->listWidgetReferences->installEventFilter(this);
    ui->listWidgetReferences->setMouseTracking(true);

    ui->profileBaseEdit->setReadOnly(true);
    ui->spineBaseEdit->setReadOnly(true);

    connect(ui->buttonProfileBase, SIGNAL(toggled(bool)),
            this, SLOT(onProfileButton(bool)));
    connect(ui->comboBoxTransition, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onTransitionChanged(int)));
    connect(ui->buttonRefAdd, SIGNAL(clicked(bool)),
            this, SLOT(onButtonRefAdd(bool)));
    connect(ui->buttonSpineBase, SIGNAL(clicked(bool)),
            this, SLOT(onBaseButton(bool)));
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
    connect(remove, SIGNAL(triggered()), this, SLOT(onDeleteEdge()));
    ui->listWidgetReferences->setContextMenuPolicy(Qt::ActionsContextMenu);

    this->initUI(proxy);
    this->groupLayout()->addWidget(proxy);

    Base::StateLocker locker(initing);
    refresh();

    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());
    connProfile = pipe->Profile.signalChanged.connect(
        [this](const App::Property &) {toggleShowOnTop(vp, lastProfile, "Profile", true);});
    connSpine = pipe->Spine.signalChanged.connect(
        [this](const App::Property &) {toggleShowOnTop(vp, lastSpine, "Spine");});

    QMetaObject::invokeMethod(this, "updateUI", Qt::QueuedConnection);
}

TaskPipeParameters::~TaskPipeParameters()
{
    exitSelectionMode();
}

void TaskPipeParameters::updateUI()
{
    toggleShowOnTop(vp, lastProfile, "Profile", true);
}

void TaskPipeParameters::onItemEntered(QListWidgetItem *item)
{
    if (!vp)
        return;
    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());
    App::SubObjectT objT(pipe->Spine.getValue(), item->text().toLatin1().constData());
    PartDesignGui::highlightObjectOnTop(objT);
}

void TaskPipeParameters::onItemSelectionChanged()
{
    if (!vp)
        return;
    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());
    App::SubObjectT objT(pipe->Spine.getValue());
    auto items = ui->listWidgetReferences->selectedItems();

    Base::StateLocker lock(_Busy);
    if (items.isEmpty() || items.size() == 1) {
        Gui::Selection().selStackPush();
        Gui::Selection().clearSelection();
    }
    for (auto item : items) {
        objT.setSubName(item->text().toLatin1().constData());
        PartDesignGui::selectObjectOnTop(objT, true);
    }
}

bool TaskPipeParameters::eventFilter(QObject *o, QEvent *ev)
{
    switch(ev->type()) {
    case QEvent::Leave:
        Gui::Selection().rmvPreselect();
        break;
    case QEvent::ShortcutOverride:
    case QEvent::KeyPress: {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(ev);
        if (o == ui->listWidgetReferences && kevent->modifiers() == Qt::NoModifier) {
            if (kevent->key() == Qt::Key_Delete) {
                kevent->accept();
                if (ev->type() == QEvent::KeyPress)
                    onDeleteEdge();
            }
        }
        break;
    }
    case QEvent::Enter:
        if (vp) {
            PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());
            App::DocumentObject *obj = nullptr;
            if (o == ui->profileBaseEdit)
                obj = pipe->Profile.getValue();
            else if (o == ui->spineBaseEdit)
                obj = pipe->Spine.getValue();
            if (obj)
                PartDesignGui::highlightObjectOnTop(obj);
        }
        break;
    default:
        break;
    }
    return false;
}

void TaskPipeParameters::refresh()
{
    if (!vp || !vp->getObject())
        return;

    // Temporarily prevent unnecessary feature recomputes
    for (QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(true);

    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());

    //add initial values
    if (pipe->Profile.getValue())
        ui->profileBaseEdit->setText(QString::fromUtf8(pipe->Profile.getValue()->Label.getValue()));
    if (pipe->Spine.getValue())
        ui->spineBaseEdit->setText(QString::fromUtf8(pipe->Spine.getValue()->Label.getValue()));

    ui->comboBoxTransition->setCurrentIndex(pipe->Transition.getValue());

    PartDesignGui::populateGeometryReferences(ui->listWidgetReferences, pipe->Spine, !initing);

    for (QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(false);

    TaskSketchBasedParameters::refresh();
}

void TaskPipeParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
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

    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());

    switch(selectionMode) {
    case refProfile: {
        if (obj == pipe->Profile.getValue())
            break;
        App::SubObjectT ref(msg.pOriginalMsg ? msg.pOriginalMsg->Object : msg.Object);
        ref = PartDesignGui::importExternalElement(ref);
        auto refObj = ref.getSubObject();
        if (refObj) {
            ui->profileBaseEdit->setText(QString::fromUtf8(refObj->Label.getValue()));
            try {
                setupTransaction();
                pipe->Profile.setValue(refObj);
                recomputeFeature();
            } catch (Base::Exception &e) {
                e.ReportException();
            }
            exitSelectionMode();
        }
        break;
    }
    case refObjAdd:
    case refAdd: {
        if (!pipe->Spine.getValue() || selectionMode == refObjAdd) {
            if (obj == pipe->Spine.getValue())
                break;
            App::SubObjectT ref(msg.pOriginalMsg ? msg.pOriginalMsg->Object : msg.Object);
            ref = PartDesignGui::importExternalElement(ref);
            auto refObj = ref.getSubObject();
            if (!refObj)
                break;
            try {
                setupTransaction();
                pipe->Spine.setValue(refObj);
                recomputeFeature();
            } catch (Base::Exception &e) {
                e.ReportException();
            }
            ui->spineBaseEdit->setText(QString::fromUtf8(refObj->Label.getValue()));
            ui->listWidgetReferences->clear();
        }
        if (!boost::starts_with(msg.pSubName, "Edge")
                && !boost::starts_with(msg.pSubName, "Face")
                && !boost::starts_with(msg.pSubName, "Wire"))
            break;

        QString element = QString::fromLatin1(msg.pSubName);
        auto items = ui->listWidgetReferences->findItems(element, Qt::MatchExactly);
        if (items.isEmpty()) {
            auto item = new QListWidgetItem(element, ui->listWidgetReferences);
            QSignalBlocker blocker(ui->listWidgetReferences);
            ui->listWidgetReferences->setCurrentItem(item);
            auto subs = pipe->Spine.getSubValues();
            subs.push_back(msg.pSubName);
            try {
                setupTransaction();
                pipe->Spine.setValue(pipe->Spine.getValue(), std::move(subs));
                recomputeFeature();
            } catch (Base::Exception &e) {
                e.ReportException();
            }
        } else {
            QSignalBlocker blocker(ui->listWidgetReferences);
            ui->listWidgetReferences->setCurrentItem(items[0]);
        }
        break;
    }
    default:
        break;
    }
}

void TaskPipeParameters::onTransitionChanged(int idx) {

    try {
        setupTransaction();
        static_cast<PartDesign::Pipe*>(vp->getObject())->Transition.setValue(idx);
        recomputeFeature();
    } catch (Base::Exception &e) {
        e.ReportException();
    }
}

void TaskPipeParameters::onButtonRefAdd(bool checked) {
    if (!vp)
        return;
    if (checked) {
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new SpineSelectionGate(this, vp));
        selectionMode = refAdd;
    } else {
        exitSelectionMode();
    }
}

void TaskPipeParameters::onBaseButton(bool checked) {
    if (!vp)
        return;
    if (checked) {
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new SpineSelectionGate(this, vp));
        selectionMode = refObjAdd;
        toggleShowOnTop(vp, lastSpine, nullptr);
    }
    else {
        exitSelectionMode();
    }
}

void TaskPipeParameters::onProfileButton(bool checked)
{
    if (checked) {
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new PipeProfileSelectionGate(this, vp));
        selectionMode = refProfile;
    }
    else {
        exitSelectionMode();
    }
}

void TaskPipeParameters::onTangentChanged(bool checked) {
    try {
        setupTransaction();
        static_cast<PartDesign::Pipe*>(vp->getObject())->SpineTangent.setValue(checked);
        recomputeFeature();
    } catch (Base::Exception &e) {
        e.ReportException();
    }
}

void TaskPipeParameters::onDeleteEdge()
{
    if (!vp)
        return;
    auto pipe = static_cast<PartDesign::Pipe*>(vp->getObject());
    auto subs = pipe->Spine.getSubValues(false);

    // Delete the selected path edge
    for (auto item : ui->listWidgetReferences->selectedItems()) {
        auto element = item->text().toLatin1();
        subs.erase(std::remove(subs.begin(), subs.end(), element.constData()), subs.end());
        delete item;
    }

    if (subs.size() != pipe->Spine.getSubValues().size()) {
        try {
            setupTransaction();
            pipe->Spine.setValue(pipe->Spine.getValue(), std::move(subs));
            recomputeFeature();
        } catch (Base::Exception &e) {
            e.ReportException();
        }
    }
}

void TaskPipeParameters::exitSelectionMode() {
    if (selectionMode == none)
        return;
    selectionMode = none;
    ui->buttonProfileBase->setChecked(false);
    ui->buttonRefAdd->setChecked(false);
    ui->buttonSpineBase->setChecked(false);
    Gui::Selection().clearSelection();
    Gui::Selection().rmvSelectionGate();
    toggleShowOnTop(vp, lastSpine, nullptr);
}

//**************************************************************************
//**************************************************************************
// Task Orientation
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskPipeOrientation::TaskPipeOrientation(ViewProviderPipe* PipeView, bool /*newObj*/, QWidget* parent)
    : TaskSketchBasedParameters(PipeView, parent, "PartDesign_AdditivePipe", tr("Section orientation")),
    ui(new Ui_TaskPipeOrientation)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    ui->profileBaseEdit->installEventFilter(this);
    ui->profileBaseEdit->setMouseTracking(true);
    ui->listWidgetReferences->installEventFilter(this);
    ui->listWidgetReferences->setMouseTracking(true);
    ui->profileBaseEdit->setReadOnly(true);

    connect(ui->comboBoxMode, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onOrientationChanged(int)));
    connect(ui->buttonRefAdd, SIGNAL(clicked(bool)),
            this, SLOT(onButtonRefAdd(bool)));
    connect(ui->buttonProfileBase, SIGNAL(clicked(bool)),
            this, SLOT(onBaseButton(bool)));
    connect(ui->buttonProfileClear, SIGNAL(clicked()),
            this, SLOT(onClearButton()));
    connect(ui->stackedWidget, SIGNAL(currentChanged(int)),
            this, SLOT(updateUI(int)));
    connect(ui->curvelinear, SIGNAL(toggled(bool)),
            this, SLOT(onCurvelinearChanged(bool)));
    connect(ui->doubleSpinBoxX, SIGNAL(valueChanged(double)),
            this, SLOT(onBinormalChanged(double)));
    connect(ui->doubleSpinBoxY, SIGNAL(valueChanged(double)),
            this, SLOT(onBinormalChanged(double)));
    connect(ui->doubleSpinBoxZ, SIGNAL(valueChanged(double)),
            this, SLOT(onBinormalChanged(double)));
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
    connect(remove, SIGNAL(triggered()), this, SLOT(onDeleteItem()));
    ui->listWidgetReferences->setContextMenuPolicy(Qt::ActionsContextMenu);

    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());

    this->groupLayout()->addWidget(proxy);
    // should be called after panel has become visible
    QMetaObject::invokeMethod(this, "updateUI", Qt::QueuedConnection,
        QGenericReturnArgument(), Q_ARG(int,pipe->Mode.getValue()));

    Base::StateLocker lock(initing);
    refresh();

    connAuxSpine = pipe->AuxillerySpine.signalChanged.connect(
        [this](const App::Property &) {toggleShowOnTop(vp, lastAuxSpine, "AuxillerySpine");});

}

TaskPipeOrientation::~TaskPipeOrientation()
{
    exitSelectionMode();
}

void TaskPipeOrientation::onItemSelectionChanged()
{
    if (!vp)
        return;
    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());
    App::SubObjectT objT(pipe->AuxillerySpine.getValue());
    auto items = ui->listWidgetReferences->selectedItems();

    Base::StateLocker lock(_Busy);
    if (items.isEmpty() || items.size() == 1) {
        Gui::Selection().selStackPush();
        Gui::Selection().clearSelection();
    }
    for (auto item : items) {
        objT.setSubName(item->text().toLatin1().constData());
        PartDesignGui::selectObjectOnTop(objT, true);
    }
}

void TaskPipeOrientation::onItemEntered(QListWidgetItem *item)
{
    if (!vp)
        return;
    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());
    App::SubObjectT objT(pipe->AuxillerySpine.getValue(), item->text().toLatin1().constData());
    PartDesignGui::highlightObjectOnTop(objT);
}

bool TaskPipeOrientation::eventFilter(QObject *o, QEvent *ev)
{
    switch(ev->type()) {
    case QEvent::ShortcutOverride:
    case QEvent::KeyPress: {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(ev);
        if (o == ui->listWidgetReferences && kevent->modifiers() == Qt::NoModifier) {
            if (kevent->key() == Qt::Key_Delete) {
                kevent->accept();
                if (ev->type() == QEvent::KeyPress)
                    onDeleteItem();
            }
        }
        break;
    }
    case QEvent::FocusIn:
        if (o == ui->profileBaseEdit || o == ui->listWidgetReferences)
            toggleShowOnTop(vp, lastAuxSpine, "AuxillerySpine", true);
        break;
    case QEvent::FocusOut:
        if (o == ui->profileBaseEdit || o == ui->listWidgetReferences)
            toggleShowOnTop(vp, lastAuxSpine, nullptr);
        break;
    case QEvent::Leave:
        Gui::Selection().rmvPreselect();
        break;
    case QEvent::Enter:
        if (vp) {
            if (o == ui->profileBaseEdit) {
                PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());
                PartDesignGui::highlightObjectOnTop(pipe->AuxillerySpine.getValue());
            }
        }
        break;
    default:
        break;
    }
    return false;
}

void TaskPipeOrientation::refresh()
{
    if (!vp || !vp->getObject())
        return;

    // Temporarily prevent unnecessary feature recomputes
    for (QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(true);

    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());

    //add initial values
    if (pipe->AuxillerySpine.getValue())
        ui->profileBaseEdit->setText(QString::fromUtf8(pipe->AuxillerySpine.getValue()->Label.getValue()));

    PartDesignGui::populateGeometryReferences(ui->listWidgetReferences, pipe->AuxillerySpine, !initing);

    ui->curvelinear->setChecked(pipe->AuxilleryCurvelinear.getValue());

    int idx = pipe->Mode.getValue();
    ui->comboBoxMode->setCurrentIndex(idx);
    ui->stackedWidget->setCurrentIndex(idx);
    updateUI(idx);

    for (QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(false);

    TaskSketchBasedParameters::refresh();
}

void TaskPipeOrientation::onOrientationChanged(int idx) {
    if (!vp)
        return;
    ui->stackedWidget->setCurrentIndex(idx);
    try {
        setupTransaction();
        auto pipe = static_cast<PartDesign::Pipe*>(vp->getObject());
        pipe->Mode.setValue(idx);
        if (idx != 3 || pipe->AuxillerySpine.getValue())
            recomputeFeature();
    } catch (Base::Exception &e) {
        e.ReportException();
    }
    if (idx == 3) {
        ui->buttonRefAdd->setChecked(true);
        onButtonRefAdd(true);
    } else
        exitSelectionMode();
}

void TaskPipeOrientation::exitSelectionMode() {
    if (selectionMode == none)
        return;
    selectionMode = none;
    ui->buttonRefAdd->setChecked(false);
    ui->buttonProfileBase->setChecked(false);
    Gui::Selection().clearSelection();
    Gui::Selection().rmvSelectionGate();
    toggleShowOnTop(vp, lastAuxSpine, nullptr);
}

void TaskPipeOrientation::onButtonRefAdd(bool checked) {

    if (checked) {
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new SpineSelectionGate(this, vp));
        selectionMode = refAdd;
        toggleShowOnTop(vp, lastAuxSpine, "AuxillerySpine", true);
    }
    else {
        exitSelectionMode();
    }
}

void TaskPipeOrientation::onBaseButton(bool checked)
{
    if (checked) {
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new SpineSelectionGate(this, vp));
        selectionMode = refObjAdd;
    }
    else {
        exitSelectionMode();
    }
}

void TaskPipeOrientation::onClearButton()
{
    ui->listWidgetReferences->clear();
    ui->profileBaseEdit->clear();
    try {
        setupTransaction();
        static_cast<PartDesign::Pipe*>(vp->getObject())->AuxillerySpine.setValue(nullptr);
        recomputeFeature();
    } catch (Base::Exception &e) {
        e.ReportException();
    }
    exitSelectionMode();
}

void TaskPipeOrientation::onCurvelinearChanged(bool checked)
{
    try {
        setupTransaction();
        static_cast<PartDesign::Pipe*>(vp->getObject())->AuxilleryCurvelinear.setValue(checked);
        recomputeFeature();
    } catch (Base::Exception &e) {
        e.ReportException();
    }
}

void TaskPipeOrientation::onBinormalChanged(double)
{
    Base::Vector3d vec(ui->doubleSpinBoxX->value(),
                       ui->doubleSpinBoxY->value(),
                       ui->doubleSpinBoxZ->value());

    try {
        setupTransaction();
        static_cast<PartDesign::Pipe*>(vp->getObject())->Binormal.setValue(vec);
        recomputeFeature();
    } catch (Base::Exception &e) {
        e.ReportException();
    }
}

void TaskPipeOrientation::onSelectionChanged(const SelectionChanges& msg) {
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

    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());

    switch(selectionMode) {
    case refObjAdd:
    case refAdd: {
        if (!pipe->AuxillerySpine.getValue() || selectionMode == refObjAdd) {
            if (obj == pipe->AuxillerySpine.getValue())
                break;
            App::SubObjectT ref(msg.pOriginalMsg ? msg.pOriginalMsg->Object : msg.Object);
            ref = PartDesignGui::importExternalElement(ref);
            auto refObj = ref.getSubObject();
            if (!refObj)
                break;
            try {
                setupTransaction();
                pipe->AuxillerySpine.setValue(refObj);
                recomputeFeature();
            } catch (Base::Exception &e) {
                e.ReportException();
            }
            ui->profileBaseEdit->setText(QString::fromUtf8(refObj->Label.getValue()));
            ui->listWidgetReferences->clear();
        }
        if (!boost::starts_with(msg.pSubName, "Edge")
                && !boost::starts_with(msg.pSubName, "Face")
                && !boost::starts_with(msg.pSubName, "Wire"))
            break;

        QString element = QString::fromLatin1(msg.pSubName);
        auto items = ui->listWidgetReferences->findItems(element, Qt::MatchExactly);
        if (items.isEmpty()) {
            auto item = new QListWidgetItem(element, ui->listWidgetReferences);
            QSignalBlocker blocker(ui->listWidgetReferences);
            ui->listWidgetReferences->setCurrentItem(item);
            auto subs = pipe->AuxillerySpine.getSubValues();
            subs.push_back(msg.pSubName);
            try {
                setupTransaction();
                pipe->AuxillerySpine.setValue(pipe->AuxillerySpine.getValue(), std::move(subs));
                recomputeFeature();
            } catch (Base::Exception &e) {
                e.ReportException();
            }
        } else {
            QSignalBlocker blocker(ui->listWidgetReferences);
            ui->listWidgetReferences->setCurrentItem(items[0]);
        }
        break;
    }
    default:
        break;
    }
}

void TaskPipeOrientation::onDeleteItem()
{
    if (!vp)
        return;

    auto pipe = static_cast<PartDesign::Pipe*>(vp->getObject());
    auto subs = pipe->AuxillerySpine.getSubValues(false);

    // Delete the selected path edge
    for (auto item : ui->listWidgetReferences->selectedItems()) {
        auto element = item->text().toLatin1();
        subs.erase(std::remove(subs.begin(), subs.end(), element.constData()), subs.end());
        delete item;
    }

    if (subs.size() != pipe->AuxillerySpine.getSubValues().size()) {
        try {
            setupTransaction();
            pipe->AuxillerySpine.setValue(pipe->AuxillerySpine.getValue(), std::move(subs));
            recomputeFeature();
        } catch (Base::Exception &e) {
            e.ReportException();
        }
    }
}

void TaskPipeOrientation::updateUI(int idx) {
    //make sure we resize to the size of the current page
    for (int i=0; i<ui->stackedWidget->count(); ++i)
        ui->stackedWidget->widget(i)->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    if (idx < ui->stackedWidget->count())
        ui->stackedWidget->widget(idx)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}



//**************************************************************************
//**************************************************************************
// Task Scaling
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
TaskPipeScaling::TaskPipeScaling(ViewProviderPipe* PipeView, bool /*newObj*/, QWidget* parent)
    : TaskSketchBasedParameters(PipeView, parent, "PartDesign_AdditivePipe", tr("Section transformation")),
    ui(new Ui_TaskPipeScaling)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    ui->listWidgetReferences->installEventFilter(this);
    ui->listWidgetReferences->setMouseTracking(true);

    connect(ui->comboBoxScaling, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onScalingChanged(int)));
    connect(ui->buttonRefAdd, SIGNAL(clicked(bool)),
            this, SLOT(onButtonRefAdd(bool)));
    connect(ui->stackedWidget, SIGNAL(currentChanged(int)),
            this, SLOT(updateUI(int)));
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

    this->groupLayout()->addWidget(proxy);

    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(PipeView->getObject());

    // should be called after panel has become visible
    QMetaObject::invokeMethod(this, "updateUI", Qt::QueuedConnection,
        QGenericReturnArgument(), Q_ARG(int,pipe->Transformation.getValue()));

    refresh();

    connSections = pipe->Sections.signalChanged.connect(
        [this](const App::Property &) {toggleShowOnTop(vp, lastSections, "Sections", true);});
}

TaskPipeScaling::~TaskPipeScaling()
{
    exitSelectionMode();
}

void TaskPipeScaling::refresh()
{
    if (!vp || !vp->getObject())
        return;

    // Temporarily prevent unnecessary feature recomputes
    for (QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(true);

    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());
    ui->listWidgetReferences->clear();
    for (auto obj : pipe->Sections.getValues())
        addItem(obj);

    int idx = pipe->Transformation.getValue();
    ui->comboBoxScaling->setCurrentIndex(idx);
    ui->stackedWidget->setCurrentIndex(idx);
    updateUI(idx);

    for (QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(false);

    TaskSketchBasedParameters::refresh();
}

void TaskPipeScaling::exitSelectionMode() {
    if (selectionMode == none)
        return;
    selectionMode = none;
    ui->buttonRefAdd->setChecked(false);
    Gui::Selection().clearSelection();
    Gui::Selection().rmvSelectionGate();
}

void TaskPipeScaling::onButtonRefAdd(bool checked) {

    if (checked) {
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new PipeSectionSelectionGate(this, vp));
        selectionMode = refAdd;
    }
    else {
        exitSelectionMode();
    }
}

void TaskPipeScaling::onScalingChanged(int idx) {
    if (!vp)
        return;
    ui->stackedWidget->setCurrentIndex(idx);
    updateUI(idx);
    try {
        setupTransaction();
        static_cast<PartDesign::Pipe*>(vp->getObject())->Transformation.setValue(idx);
        recomputeFeature();
    } catch (Base::Exception &e) {
        e.ReportException();
    }
    if (idx == 1) {
        ui->buttonRefAdd->setChecked(true);
        onButtonRefAdd(true);
    } else
        exitSelectionMode();
}

void TaskPipeScaling::addItem(App::DocumentObject *obj, bool select)
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

void TaskPipeScaling::onSelectionChanged(const Gui::SelectionChanges& msg)
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

    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());

    switch(selectionMode) {
    case refAdd: {
        App::SubObjectT ref(msg.pOriginalMsg ? msg.pOriginalMsg->Object : msg.Object);
        ref = PartDesignGui::importExternalElement(ref);
        auto refObj = ref.getSubObject();
        if (refObj) {
            auto sections = pipe->Sections.getValues();
            sections.push_back(refObj);
            addItem(refObj, true);
            try {
                setupTransaction();
                pipe->Sections.setValues(sections);
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

void TaskPipeScaling::onDeleteSection()
{
    if (!vp)
        return;
    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());
    std::vector<App::DocumentObject*> refs = pipe->Sections.getValues();

    // Delete the selected profile
    for(auto item : ui->listWidgetReferences->selectedItems()) {
        auto objT = qvariant_cast<App::SubObjectT>(item->data(Qt::UserRole));
        refs.erase(std::remove(refs.begin(), refs.end(), objT.getSubObject()), refs.end());
        delete item;
    }

    if (refs.size() != pipe->Sections.getValues().size()) {
        try {
            setupTransaction();
            pipe->Sections.setValues(refs);
            recomputeFeature();
        } catch (Base::Exception &e) {
            e.ReportException();
        }
    }
}

void TaskPipeScaling::updateUI(int idx) {

    //make sure we resize to the size of the current page
    for(int i=0; i<ui->stackedWidget->count(); ++i)
        ui->stackedWidget->widget(i)->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    if (idx < ui->stackedWidget->count())
        ui->stackedWidget->widget(idx)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    toggleShowOnTop(vp, lastSections, "Sections", true);
}

void TaskPipeScaling::onItemEntered(QListWidgetItem *item)
{
    if (!vp)
        return;
    auto objT = qvariant_cast<App::SubObjectT>(item->data(Qt::UserRole));
    PartDesignGui::highlightObjectOnTop(objT);
}

bool TaskPipeScaling::eventFilter(QObject *, QEvent *ev)
{
    switch(ev->type()) {
    case QEvent::Leave:
        Gui::Selection().rmvPreselect();
        break;
    default:
        break;
    }
    return false;
}

void TaskPipeScaling::onItemSelectionChanged()
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

TaskDlgPipeParameters::TaskDlgPipeParameters(ViewProviderPipe *PipeView,bool newObj)
   : TaskDlgSketchBasedParameters(PipeView)
{
    assert(PipeView);
    parameter    = new TaskPipeParameters(PipeView,newObj);
    orientation  = new TaskPipeOrientation(PipeView,newObj);
    scaling      = new TaskPipeScaling(PipeView,newObj);

    Content.push_back(parameter);
    Content.push_back(orientation);
    Content.push_back(scaling);

    if (PipeView->getObject()->isTouched() || PipeView->getObject()->mustExecute())
        parameter->recomputeFeature();
}

TaskDlgPipeParameters::~TaskDlgPipeParameters()
{
    Gui::Selection().rmvSelectionGate();
}

//==== calls from the TaskView ===============================================================


bool TaskDlgPipeParameters::accept()
{
    PartDesign::Pipe* pcPipe = static_cast<PartDesign::Pipe*>(getPipeView()->getObject());
    try {
        parameter->setupTransaction();
        Gui::cmdGuiDocument(pcPipe, "resetEdit()");
        Gui::cmdAppDocument(pcPipe, "recompute()");
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

#include "moc_TaskPipeParameters.cpp"
