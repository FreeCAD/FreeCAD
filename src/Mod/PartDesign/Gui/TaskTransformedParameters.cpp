/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
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
#include <QAction>
#include <QListWidget>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Command.h>
#include <Gui/Tools.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureAddSub.h>
#include <Mod/PartDesign/App/FeatureTransformed.h>

#include "ui_TaskTransformedParameters.h"
#include "TaskTransformedParameters.h"
#include "TaskMultiTransformParameters.h"
#include "ReferenceSelection.h"


FC_LOG_LEVEL_INIT("PartDesign", true, true)

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskTransformedParameters */

TaskTransformedParameters::TaskTransformedParameters(ViewProviderTransformed* TransformedView,
                                                     QWidget* parent)
    : TaskBox(Gui::BitmapFactory().pixmap(TransformedView->featureIcon().c_str()),
              TransformedView->menuName,
              true,
              parent)
    , TransformedView(TransformedView)
    , ui(new Ui_TaskTransformedParameters)
{
    Gui::Document* doc = TransformedView->getDocument();
    this->attachDocument(doc);

    // remember initial transaction ID
    App::GetApplication().getActiveTransaction(&transactionID);
}

TaskTransformedParameters::TaskTransformedParameters(TaskMultiTransformParameters* parentTask)
    : TaskBox(QPixmap(), tr(""), true, parentTask)
    , parentTask(parentTask)
    , insideMultiTransform(true)
{}

TaskTransformedParameters::~TaskTransformedParameters()
{
    // make sure to remove selection gate in all cases
    Gui::Selection().rmvSelectionGate();

    delete proxy;
}

void TaskTransformedParameters::setupUI()
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->buttonAddFeature,
            &QToolButton::toggled,
            this,
            &TaskTransformedParameters::onButtonAddFeature);
    connect(ui->buttonRemoveFeature,
            &QToolButton::toggled,
            this,
            &TaskTransformedParameters::onButtonRemoveFeature);

    // Create context menu
    auto action = new QAction(tr("Remove"), this);
    action->setShortcut(Gui::QtTools::deleteKeySequence());

    // display shortcut behind the context menu entry
    action->setShortcutVisibleInContextMenu(true);
    ui->listWidgetFeatures->addAction(action);
    connect(action, &QAction::triggered, this, &TaskTransformedParameters::onFeatureDeleted);
    ui->listWidgetFeatures->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(ui->listWidgetFeatures->model(),
            &QAbstractListModel::rowsMoved,
            this,
            &TaskTransformedParameters::indexesMoved);

    connect(ui->checkBoxUpdateView,
            &QCheckBox::toggled,
            this,
            &TaskTransformedParameters::onUpdateView);

    // Get the feature data
    auto pcTransformed = getObject<PartDesign::Transformed>();

    using Mode = PartDesign::Transformed::Mode;

    ui->buttonGroupMode->setId(ui->radioTransformBody, static_cast<int>(Mode::TransformBody));
    ui->buttonGroupMode->setId(ui->radioTransformToolShapes, static_cast<int>(Mode::TransformToolShapes));

    connect(ui->buttonGroupMode,
            &QButtonGroup::idClicked,
            this,
            &TaskTransformedParameters::onModeChanged);

    auto const mode = static_cast<Mode>(pcTransformed->TransformMode.getValue());
    ui->groupFeatureList->setEnabled(mode == Mode::TransformToolShapes);
    switch (mode) {
        case Mode::TransformBody:
            ui->radioTransformBody->setChecked(true);
            break;
        case Mode::TransformToolShapes:
            ui->radioTransformToolShapes->setChecked(true);
            break;
    }

    std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();
    // Fill data into dialog elements
    for (auto obj : originals) {
        if (obj) {
            auto item = new QListWidgetItem();
            item->setText(QString::fromUtf8(obj->Label.getValue()));
            item->setData(Qt::UserRole, QString::fromLatin1(obj->getNameInDocument()));
            ui->listWidgetFeatures->addItem(item);
        }
    }

    setupParameterUI(ui->featureUI);  // create parameter UI widgets
    this->groupLayout()->addWidget(proxy);
}

void TaskTransformedParameters::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    if (TransformedView == &Obj) {
        TransformedView = nullptr;
    }
}

void TaskTransformedParameters::changeEvent(QEvent* event)
{
    TaskBox::changeEvent(event);
    if (event->type() == QEvent::LanguageChange && proxy) {
        ui->retranslateUi(proxy);
        retranslateParameterUI(ui->featureUI);
    }
}

void TaskTransformedParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (originalSelected(msg)) {
        exitSelectionMode();
    }
}

void TaskTransformedParameters::clearButtons()
{
    if (insideMultiTransform) {
        parentTask->clearButtons();
    }
    else {
        ui->buttonAddFeature->setChecked(false);
        ui->buttonRemoveFeature->setChecked(false);
    }
}

int TaskTransformedParameters::getUpdateViewTimeout() const
{
    return 500;
}

void TaskTransformedParameters::addObject(App::DocumentObject* obj)
{
    QString label = QString::fromUtf8(obj->Label.getValue());
    QString objectName = QString::fromLatin1(obj->getNameInDocument());

    auto item = new QListWidgetItem();
    item->setText(label);
    item->setData(Qt::UserRole, objectName);
    ui->listWidgetFeatures->addItem(item);
}

void TaskTransformedParameters::removeObject(App::DocumentObject* obj)
{
    QString label = QString::fromUtf8(obj->Label.getValue());
    removeItemFromListWidget(ui->listWidgetFeatures, label);
}

bool TaskTransformedParameters::originalSelected(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection
        && ((selectionMode == SelectionMode::AddFeature)
            || (selectionMode == SelectionMode::RemoveFeature))) {

        if (strcmp(msg.pDocName, getObject()->getDocument()->getName()) != 0) {
            return false;
        }

        PartDesign::Transformed* pcTransformed = getObject();
        App::DocumentObject* selectedObject =
            pcTransformed->getDocument()->getObject(msg.pObjectName);
        if (selectedObject->isDerivedFrom<PartDesign::FeatureAddSub>()) {

            // Do the same like in TaskDlgTransformedParameters::accept() but without doCommand
            std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();
            const auto or_iter = std::ranges::find(originals, selectedObject);
            if (selectionMode == SelectionMode::AddFeature) {
                if (or_iter == originals.end()) {
                    originals.push_back(selectedObject);
                    addObject(selectedObject);
                }
                else {
                    return false;  // duplicate selection
                }
            }
            else {
                if (or_iter != originals.end()) {
                    originals.erase(or_iter);
                    removeObject(selectedObject);
                }
                else {
                    return false;
                }
            }
            setupTransaction();
            pcTransformed->Originals.setValues(originals);
            recomputeFeature();

            return true;
        }
    }

    return false;
}

void TaskTransformedParameters::setupTransaction()
{
    if (!isEnabledTransaction()) {
        return;
    }

    auto obj = getObject();
    if (!obj) {
        return;
    }

    int tid = 0;
    App::GetApplication().getActiveTransaction(&tid);
    if (tid != 0 && tid == transactionID) {
        return;
    }

    // open a transaction if none is active
    std::string name("Edit ");
    name += obj->Label.getValue();
    transactionID = App::GetApplication().setActiveTransaction(name.c_str());
}

void TaskTransformedParameters::setEnabledTransaction(bool on)
{
    enableTransaction = on;
}

bool TaskTransformedParameters::isEnabledTransaction() const
{
    return enableTransaction;
}

void TaskTransformedParameters::onModeChanged(int mode_id)
{
    if (mode_id < 0) {
        return;
    }

    auto pcTransformed = getObject<PartDesign::Transformed>();
    pcTransformed->TransformMode.setValue(mode_id);

    using Mode = PartDesign::Transformed::Mode;
    Mode const mode = static_cast<Mode>(mode_id);

    ui->groupFeatureList->setEnabled(mode == Mode::TransformToolShapes);
    if (mode == Mode::TransformBody) {
        ui->listWidgetFeatures->clear();
    }
    setupTransaction();
    recomputeFeature();
}

void TaskTransformedParameters::onButtonAddFeature(bool checked)
{
    if (checked) {
        hideObject();
        showBase();
        selectionMode = SelectionMode::AddFeature;
        Gui::Selection().clearSelection();
    }
    else {
        exitSelectionMode();
    }

    ui->buttonRemoveFeature->setDisabled(checked);
}

// Make sure only some feature before the given one is visible
void TaskTransformedParameters::checkVisibility()
{
    auto feat = getObject();
    auto body = feat->getFeatureBody();
    if (!body) {
        return;
    }
    auto inset = feat->getInListEx(true);
    inset.emplace(feat);
    for (auto obj : body->Group.getValues()) {
        if (!obj->Visibility.getValue() || !obj->isDerivedFrom<PartDesign::Feature>()) {
            continue;
        }
        if (inset.count(obj) > 0) {
            break;
        }
        return;
    }
    FCMD_OBJ_SHOW(getBaseObject());
}

void TaskTransformedParameters::onButtonRemoveFeature(bool checked)
{
    if (checked) {
        checkVisibility();
        selectionMode = SelectionMode::RemoveFeature;
        Gui::Selection().clearSelection();
    }
    else {
        exitSelectionMode();
    }

    ui->buttonAddFeature->setDisabled(checked);
}

void TaskTransformedParameters::onFeatureDeleted()
{
    PartDesign::Transformed* pcTransformed = getObject();
    std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();
    int currentRow = ui->listWidgetFeatures->currentRow();
    if (currentRow < 0) {
        Base::Console().error("PartDesign Pattern: No feature selected for removing.\n");
        return;  // no current row selected
    }
    originals.erase(originals.begin() + currentRow);
    setupTransaction();
    pcTransformed->Originals.setValues(originals);
    ui->listWidgetFeatures->model()->removeRow(currentRow);
    recomputeFeature();
}

void TaskTransformedParameters::removeItemFromListWidget(QListWidget* widget,
                                                         const QString& itemstr)
{
    QList<QListWidgetItem*> items = widget->findItems(itemstr, Qt::MatchExactly);
    if (!items.empty()) {
        for (auto item : items) {
            delete widget->takeItem(widget->row(item));
        }
    }
}

void TaskTransformedParameters::fillAxisCombo(ComboLinks& combolinks, Part::Part2DObject* sketch)
{
    combolinks.clear();

    // add sketch axes
    if (sketch) {
        combolinks.addLink(sketch, "N_Axis", tr("Normal sketch axis"));
        combolinks.addLink(sketch, "V_Axis", tr("Vertical sketch axis"));
        combolinks.addLink(sketch, "H_Axis", tr("Horizontal sketch axis"));
        for (int i = 0; i < sketch->getAxisCount(); i++) {
            QString itemText = tr("Construction line %1").arg(i + 1);
            std::stringstream sub;
            sub << "Axis" << i;
            combolinks.addLink(sketch, sub.str(), itemText);
        }
    }

    // add part axes
    App::DocumentObject* obj = getObject();
    PartDesign::Body* body = PartDesign::Body::findBodyOf(obj);

    if (body) {
        try {
            App::Origin* orig = body->getOrigin();
            combolinks.addLink(orig->getX(), "", tr("Base x-axis"));
            combolinks.addLink(orig->getY(), "", tr("Base y-axis"));
            combolinks.addLink(orig->getZ(), "", tr("Base z-axis"));
        }
        catch (const Base::Exception& ex) {
            Base::Console().error("%s\n", ex.what());
        }
    }

    // add "Select reference"
    combolinks.addLink(nullptr, std::string(), tr("Select reference…"));
}

void TaskTransformedParameters::fillPlanesCombo(ComboLinks& combolinks, Part::Part2DObject* sketch)
{
    combolinks.clear();

    // add sketch axes
    if (sketch) {
        combolinks.addLink(sketch, "V_Axis", QObject::tr("Vertical sketch axis"));
        combolinks.addLink(sketch, "H_Axis", QObject::tr("Horizontal sketch axis"));
        for (int i = 0; i < sketch->getAxisCount(); i++) {
            QString itemText = tr("Construction line %1").arg(i + 1);
            std::stringstream sub;
            sub << "Axis" << i;
            combolinks.addLink(sketch, sub.str(), itemText);
        }
    }

    // add part baseplanes
    App::DocumentObject* obj = getObject();
    PartDesign::Body* body = PartDesign::Body::findBodyOf(obj);

    if (body) {
        try {
            App::Origin* orig = body->getOrigin();
            combolinks.addLink(orig->getXY(), "", tr("Base XY plane"));
            combolinks.addLink(orig->getYZ(), "", tr("Base YZ plane"));
            combolinks.addLink(orig->getXZ(), "", tr("Base XZ plane"));
        }
        catch (const Base::Exception& ex) {
            Base::Console().error("%s\n", ex.what());
        }
    }

    // add "Select reference"
    combolinks.addLink(nullptr, std::string(), tr("Select reference…"));
}

void TaskTransformedParameters::recomputeFeature()
{
    getTopTransformedView()->recomputeFeature();
}

PartDesignGui::ViewProviderTransformed* TaskTransformedParameters::getTopTransformedView() const
{
    return insideMultiTransform ? parentTask->TransformedView : TransformedView;
}

PartDesign::Transformed* TaskTransformedParameters::getTopTransformedObject() const
{
    ViewProviderTransformed* vp = getTopTransformedView();
    if (!vp) {
        return nullptr;
    }

    App::DocumentObject* transform = vp->getObject();
    assert(transform->isDerivedFrom<PartDesign::Transformed>());
    return static_cast<PartDesign::Transformed*>(transform);
}

PartDesign::Transformed* TaskTransformedParameters::getObject() const
{
    if (insideMultiTransform) {
        return parentTask->getSubFeature();
    }
    if (TransformedView) {
        return TransformedView->getObject<PartDesign::Transformed>();
    }
    return nullptr;
}

App::DocumentObject* TaskTransformedParameters::getBaseObject() const
{
    PartDesign::Feature* feature = getTopTransformedObject();
    if (!feature) {
        return nullptr;
    }

    // NOTE: getBaseObject() throws if there is no base; shouldn't happen here.
    App::DocumentObject* base = feature->getBaseObject(true);
    if (!base) {
        auto body = feature->getFeatureBody();
        if (body) {
            base = body->getPrevSolidFeature(feature);
        }
    }
    return base;
}

App::DocumentObject* TaskTransformedParameters::getSketchObject() const
{
    PartDesign::Transformed* feature = getTopTransformedObject();
    return feature ? feature->getSketchObject() : nullptr;
}

void TaskTransformedParameters::hideObject()
{
    try {
        FCMD_OBJ_HIDE(getTopTransformedObject());
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }
}

void TaskTransformedParameters::showObject()
{
    try {
        FCMD_OBJ_SHOW(getTopTransformedObject());
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }
}

void TaskTransformedParameters::hideBase()
{
    try {
        FCMD_OBJ_HIDE(getBaseObject());
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }
}

void TaskTransformedParameters::showBase()
{
    try {
        FCMD_OBJ_SHOW(getBaseObject());
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }
}

void TaskTransformedParameters::exitSelectionMode()
{
    try {
        clearButtons();
        selectionMode = SelectionMode::None;
        Gui::Selection().rmvSelectionGate();
        showObject();
    }
    catch (Base::Exception& exc) {
        exc.reportException();
    }
}

void TaskTransformedParameters::addReferenceSelectionGate(AllowSelectionFlags allow)
{
    std::unique_ptr<Gui::SelectionFilterGate> gateRefPtr(
        new ReferenceSelection(getBaseObject(), allow));
    std::unique_ptr<Gui::SelectionFilterGate> gateDepPtr(
        new NoDependentsSelection(getTopTransformedObject()));
    Gui::Selection().addSelectionGate(new CombineSelectionFilterGates(gateRefPtr, gateDepPtr));
}

void TaskTransformedParameters::indexesMoved()
{
    auto model = qobject_cast<QAbstractItemModel*>(sender());
    if (!model) {
        return;
    }

    PartDesign::Transformed* pcTransformed = getObject();
    std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();

    QByteArray name;
    int rows = model->rowCount();
    for (int i = 0; i < rows; i++) {
        QModelIndex index = model->index(i, 0);
        name = index.data(Qt::UserRole).toByteArray().constData();
        originals[i] = pcTransformed->getDocument()->getObject(name.constData());
    }

    setupTransaction();
    pcTransformed->Originals.setValues(originals);
    recomputeFeature();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgTransformedParameters::TaskDlgTransformedParameters(ViewProviderTransformed* viewProvider)
    : TaskDlgFeatureParameters(viewProvider)
{
    message = new TaskTransformedMessages(viewProvider);
    Content.push_back(message);
}

//==== calls from the TaskView ===============================================================

bool TaskDlgTransformedParameters::accept()
{
    parameter->exitSelectionMode();
    parameter->apply();

    return TaskDlgFeatureParameters::accept();
}

bool TaskDlgTransformedParameters::reject()
{
    // ensure that we are not in selection mode
    parameter->exitSelectionMode();
    return TaskDlgFeatureParameters::reject();
}


#include "moc_TaskTransformedParameters.cpp"


ComboLinks::ComboLinks(QComboBox& combo)
    : _combo(&combo)
{
    _combo->clear();
}

int ComboLinks::addLink(const App::PropertyLinkSub& lnk, QString const& itemText)
{
    if (!_combo) {
        return 0;
    }
    _combo->addItem(itemText);
    this->linksInList.push_back(new App::PropertyLinkSub());
    App::PropertyLinkSub& newitem = *(linksInList[linksInList.size() - 1]);
    newitem.Paste(lnk);
    if (newitem.getValue() && !this->doc) {
        this->doc = newitem.getValue()->getDocument();
    }
    return linksInList.size() - 1;
}

int ComboLinks::addLink(App::DocumentObject* linkObj,
                        std::string const& linkSubname,
                        QString const& itemText)
{
    if (!_combo) {
        return 0;
    }
    _combo->addItem(itemText);
    this->linksInList.push_back(new App::PropertyLinkSub());
    App::PropertyLinkSub& newitem = *(linksInList[linksInList.size() - 1]);
    newitem.setValue(linkObj, std::vector<std::string>(1, linkSubname));
    if (newitem.getValue() && !this->doc) {
        this->doc = newitem.getValue()->getDocument();
    }
    return linksInList.size() - 1;
}

void ComboLinks::clear()
{
    for (size_t i = 0; i < this->linksInList.size(); i++) {
        delete linksInList[i];
    }
    if (this->_combo) {
        _combo->clear();
    }
}

App::PropertyLinkSub& ComboLinks::getLink(int index) const
{
    if (index < 0 || index > static_cast<int>(linksInList.size()) - 1) {
        throw Base::IndexError("ComboLinks::getLink:Index out of range");
    }
    if (linksInList[index]->getValue() && doc && !(doc->isIn(linksInList[index]->getValue()))) {
        throw Base::ValueError("Linked object is not in the document; it may have been deleted");
    }
    return *(linksInList[index]);
}

App::PropertyLinkSub& ComboLinks::getCurrentLink() const
{
    assert(_combo);
    return getLink(_combo->currentIndex());
}

int ComboLinks::setCurrentLink(const App::PropertyLinkSub& lnk)
{
    for (size_t i = 0; i < linksInList.size(); i++) {
        App::PropertyLinkSub& it = *(linksInList[i]);
        if (lnk.getValue() == it.getValue() && lnk.getSubValues() == it.getSubValues()) {
            bool wasBlocked = _combo->signalsBlocked();
            _combo->blockSignals(true);
            _combo->setCurrentIndex(i);
            _combo->blockSignals(wasBlocked);
            return i;
        }
    }
    return -1;
}
