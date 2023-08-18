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
# include <QListWidget>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureAddSub.h>
#include <Mod/PartDesign/App/FeatureTransformed.h>

#include "TaskTransformedParameters.h"
#include "TaskMultiTransformParameters.h"
#include "ReferenceSelection.h"


FC_LOG_LEVEL_INIT("PartDesign",true,true)

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskTransformedParameters */

TaskTransformedParameters::TaskTransformedParameters(ViewProviderTransformed *TransformedView, QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap(TransformedView->featureIcon().c_str()),
              TransformedView->menuName, true, parent)
    , proxy(nullptr)
    , TransformedView(TransformedView)
    , parentTask(nullptr)
    , insideMultiTransform(false)
    , blockUpdate(false)
{
    selectionMode = none;

    if (TransformedView) {
        Gui::Document* doc = TransformedView->getDocument();
        this->attachDocument(doc);
    }

    // remember initial transaction ID
    App::GetApplication().getActiveTransaction(&transactionID);
}

TaskTransformedParameters::TaskTransformedParameters(TaskMultiTransformParameters *parentTask)
    : TaskBox(QPixmap(), tr(""), true, parentTask),
      proxy(nullptr),
      TransformedView(nullptr),
      parentTask(parentTask),
      insideMultiTransform(true),
      blockUpdate(false)
{
    // Original feature selection makes no sense inside a MultiTransform
    selectionMode = none;
}

TaskTransformedParameters::~TaskTransformedParameters()
{
    // make sure to remove selection gate in all cases
    Gui::Selection().rmvSelectionGate();
}

void TaskTransformedParameters::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    if (TransformedView == &Obj)
        TransformedView = nullptr;
}

bool TaskTransformedParameters::isViewUpdated() const
{
    return (blockUpdate == false);
}

int TaskTransformedParameters::getUpdateViewTimeout() const
{
    return 500;
}

void TaskTransformedParameters::addObject(App::DocumentObject*)
{
}

void TaskTransformedParameters::removeObject(App::DocumentObject*)
{
}

bool TaskTransformedParameters::originalSelected(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection && (
                (selectionMode == addFeature) || (selectionMode == removeFeature))) {

        if (strcmp(msg.pDocName, getObject()->getDocument()->getName()) != 0)
            return false;

        PartDesign::Transformed* pcTransformed = getObject();
        App::DocumentObject* selectedObject = pcTransformed->getDocument()->getObject(msg.pObjectName);
        if (selectedObject->isDerivedFrom(PartDesign::FeatureAddSub::getClassTypeId())) {

            // Do the same like in TaskDlgTransformedParameters::accept() but without doCommand
            std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();
            std::vector<App::DocumentObject*>::iterator o = std::find(originals.begin(), originals.end(), selectedObject);
            if (selectionMode == addFeature) {
                if (o == originals.end()) {
                    originals.push_back(selectedObject);
                    addObject(selectedObject);
                }
                else {
                    return false; // duplicate selection
                }
            } else {
                if (o != originals.end()) {
                    originals.erase(o);
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
    if (!isEnabledTransaction())
        return;

    auto obj = getObject();
    if (!obj)
        return;

    int tid = 0;
    App::GetApplication().getActiveTransaction(&tid);
    if (tid && tid == transactionID)
        return;

    // open a transaction if none is active
    std::string n("Edit ");
    n += obj->Label.getValue();
    transactionID = App::GetApplication().setActiveTransaction(n.c_str());
}

void TaskTransformedParameters::setEnabledTransaction(bool on)
{
    enableTransaction = on;
}

bool TaskTransformedParameters::isEnabledTransaction() const
{
    return enableTransaction;
}

void TaskTransformedParameters::onButtonAddFeature(bool checked)
{
    if (checked) {
        hideObject();
        showBase();
        selectionMode = addFeature;
        Gui::Selection().clearSelection();
    } else {
        exitSelectionMode();
    }
}

// Make sure only some feature before the given one is visible
void TaskTransformedParameters::checkVisibility() {
    auto feat = getObject();
    auto body = feat->getFeatureBody();
    if(!body)
        return;
    auto inset = feat->getInListEx(true);
    inset.emplace(feat);
    for(auto o : body->Group.getValues()) {
        if(!o->Visibility.getValue()
                || !o->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
            continue;
        if(inset.count(o))
            break;
        return;
    }
    FCMD_OBJ_SHOW(getBaseObject());
}

void TaskTransformedParameters::onButtonRemoveFeature(bool checked)
{
    if (checked) {
        checkVisibility();
        selectionMode = removeFeature;
        Gui::Selection().clearSelection();
    } else {
        exitSelectionMode();
    }
}

void TaskTransformedParameters::removeItemFromListWidget(QListWidget* widget, const QString& itemstr)
{
    QList<QListWidgetItem*> items = widget->findItems(itemstr, Qt::MatchExactly);
    if (!items.empty()) {
        for (auto item : items) {
            QListWidgetItem* it = widget->takeItem(widget->row(item));
            delete it;
        }
    }
}

void TaskTransformedParameters::fillAxisCombo(ComboLinks &combolinks,
                                              Part::Part2DObject* sketch)
{
    combolinks.clear();

    //add sketch axes
    if (sketch){
        combolinks.addLink(sketch, "N_Axis",tr("Normal sketch axis"));
        combolinks.addLink(sketch,"V_Axis",tr("Vertical sketch axis"));
        combolinks.addLink(sketch,"H_Axis",tr("Horizontal sketch axis"));
        for (int i=0; i < sketch->getAxisCount(); i++) {
            QString itemText = tr("Construction line %1").arg(i+1);
            std::stringstream sub;
            sub << "Axis" << i;
            combolinks.addLink(sketch,sub.str(),itemText);
        }
    }

    //add part axes
    App::DocumentObject* obj = getObject();
    PartDesign::Body * body = PartDesign::Body::findBodyOf ( obj );

    if (body) {
        try {
            App::Origin* orig = body->getOrigin();
            combolinks.addLink(orig->getX(),"",tr("Base X axis"));
            combolinks.addLink(orig->getY(),"",tr("Base Y axis"));
            combolinks.addLink(orig->getZ(),"",tr("Base Z axis"));
        } catch (const Base::Exception &ex) {
            Base::Console().Error ("%s\n", ex.what() );
        }
    }

    //add "Select reference"
    combolinks.addLink(nullptr,std::string(),tr("Select reference..."));
}

void TaskTransformedParameters::fillPlanesCombo(ComboLinks &combolinks,
                                                Part::Part2DObject* sketch)
{
    combolinks.clear();

    //add sketch axes
    if (sketch){
        combolinks.addLink(sketch,"V_Axis",QObject::tr("Vertical sketch axis"));
        combolinks.addLink(sketch,"H_Axis",QObject::tr("Horizontal sketch axis"));
        for (int i=0; i < sketch->getAxisCount(); i++) {
            QString itemText = tr("Construction line %1").arg(i+1);
            std::stringstream sub;
            sub << "Axis" << i;
            combolinks.addLink(sketch,sub.str(),itemText);
        }
    }

    //add part baseplanes
    App::DocumentObject* obj = getObject();
    PartDesign::Body * body = PartDesign::Body::findBodyOf ( obj );

    if (body) {
        try {
            App::Origin* orig = body->getOrigin();
            combolinks.addLink(orig->getXY(),"",tr("Base XY plane"));
            combolinks.addLink(orig->getYZ(),"",tr("Base YZ plane"));
            combolinks.addLink(orig->getXZ(),"",tr("Base XZ plane"));
        } catch (const Base::Exception &ex) {
            Base::Console().Error ("%s\n", ex.what() );
        }
    }

    //add "Select reference"
    combolinks.addLink(nullptr,std::string(),tr("Select reference..."));
}

void TaskTransformedParameters::recomputeFeature() {
    getTopTransformedView()->recomputeFeature();
}

PartDesignGui::ViewProviderTransformed *TaskTransformedParameters::getTopTransformedView() const {
    PartDesignGui::ViewProviderTransformed *rv;

    if (insideMultiTransform) {
        rv = parentTask->TransformedView;
    } else {
        rv = TransformedView;
    }
    return rv;
}

PartDesign::Transformed *TaskTransformedParameters::getTopTransformedObject() const {
    ViewProviderTransformed* vp = getTopTransformedView();
    if (!vp)
        return nullptr;

    App::DocumentObject *transform = vp->getObject();
    assert (transform->isDerivedFrom(PartDesign::Transformed::getClassTypeId()));
    return static_cast<PartDesign::Transformed*>(transform);
}

PartDesign::Transformed *TaskTransformedParameters::getObject() const {
    if (insideMultiTransform)
        return parentTask->getSubFeature();
    else if (TransformedView)
        return static_cast<PartDesign::Transformed*>(TransformedView->getObject());
    else
        return nullptr;
}

App::DocumentObject *TaskTransformedParameters::getBaseObject() const {
    PartDesign::Feature* feature = getTopTransformedObject ();
    if (!feature)
        return nullptr;

    // NOTE: getBaseObject() throws if there is no base; shouldn't happen here.
    App::DocumentObject *base = feature->getBaseObject(true);
    if(!base) {
        auto body = feature->getFeatureBody();
        if(body)
            base = body->getPrevSolidFeature(feature);
    }
    return base;
}

App::DocumentObject* TaskTransformedParameters::getSketchObject() const {
    PartDesign::Transformed* feature = getTopTransformedObject();
    return feature ? feature->getSketchObject() : nullptr;
}

void TaskTransformedParameters::hideObject()
{
    try {
        FCMD_OBJ_HIDE(getTopTransformedObject());
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
}

void TaskTransformedParameters::showObject()
{
    try {
        FCMD_OBJ_SHOW(getTopTransformedObject());
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
}

void TaskTransformedParameters::hideBase()
{
    try {
        FCMD_OBJ_HIDE(getBaseObject());
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
}

void TaskTransformedParameters::showBase()
{
    try {
        FCMD_OBJ_SHOW(getBaseObject());
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
}

void TaskTransformedParameters::exitSelectionMode()
{
    try {
        clearButtons();
        selectionMode = none;
        Gui::Selection().rmvSelectionGate();
        showObject();
    } catch(Base::Exception &e) {
        e.ReportException();
    }
}

void TaskTransformedParameters::addReferenceSelectionGate(AllowSelectionFlags allow)
{
    std::unique_ptr<Gui::SelectionFilterGate> gateRefPtr(new ReferenceSelection(getBaseObject(), allow));
    std::unique_ptr<Gui::SelectionFilterGate> gateDepPtr(new NoDependentsSelection(getTopTransformedObject()));
    Gui::Selection().addSelectionGate(new CombineSelectionFilterGates(gateRefPtr, gateDepPtr));
}

void TaskTransformedParameters::indexesMoved()
{
    QAbstractItemModel* model = qobject_cast<QAbstractItemModel*>(sender());
    if (!model)
        return;

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

TaskDlgTransformedParameters::TaskDlgTransformedParameters(ViewProviderTransformed *TransformedView_)
    : TaskDlgFeatureParameters(TransformedView_), parameter(nullptr)
{
    assert(vp);
    message = new TaskTransformedMessages(getTransformedView());

    Content.push_back(message);
}

//==== calls from the TaskView ===============================================================

bool TaskDlgTransformedParameters::accept()
{
    parameter->exitSelectionMode();

    // Continue (usually in virtual method accept())
    return TaskDlgFeatureParameters::accept ();
}

bool TaskDlgTransformedParameters::reject()
{
    // ensure that we are not in selection mode
    parameter->exitSelectionMode();
    return TaskDlgFeatureParameters::reject ();
}


#include "moc_TaskTransformedParameters.cpp"


ComboLinks::ComboLinks(QComboBox &combo)
    : doc(nullptr)
{
    this->_combo = &combo;
    _combo->clear();
}

int ComboLinks::addLink(const App::PropertyLinkSub &lnk, QString itemText)
{
    if(!_combo)
        return 0;
    _combo->addItem(itemText);
    this->linksInList.push_back(new App::PropertyLinkSub());
    App::PropertyLinkSub &newitem = *(linksInList[linksInList.size()-1]);
    newitem.Paste(lnk);
    if (newitem.getValue() && !this->doc)
        this->doc = newitem.getValue()->getDocument();
    return linksInList.size()-1;
}

int ComboLinks::addLink(App::DocumentObject *linkObj, std::string linkSubname, QString itemText)
{
    if(!_combo)
        return 0;
    _combo->addItem(itemText);
    this->linksInList.push_back(new App::PropertyLinkSub());
    App::PropertyLinkSub &newitem = *(linksInList[linksInList.size()-1]);
    newitem.setValue(linkObj,std::vector<std::string>(1,linkSubname));
    if (newitem.getValue() && !this->doc)
        this->doc = newitem.getValue()->getDocument();
    return linksInList.size()-1;
}

void ComboLinks::clear()
{
    for(size_t i = 0  ;  i < this->linksInList.size()  ;  i++){
        delete linksInList[i];
    }
    if(this->_combo)
        _combo->clear();
}

App::PropertyLinkSub &ComboLinks::getLink(int index) const
{
    if (index < 0 || index > static_cast<int>(linksInList.size())-1)
        throw Base::IndexError("ComboLinks::getLink:Index out of range");
    if (linksInList[index]->getValue() && doc && !(doc->isIn(linksInList[index]->getValue())))
        throw Base::ValueError("Linked object is not in the document; it may have been deleted");
    return *(linksInList[index]);
}

App::PropertyLinkSub &ComboLinks::getCurrentLink() const
{
    assert(_combo);
    return getLink(_combo->currentIndex());
}

int ComboLinks::setCurrentLink(const App::PropertyLinkSub &lnk)
{
    for(size_t i = 0  ;  i < linksInList.size()  ;  i++) {
        App::PropertyLinkSub &it = *(linksInList[i]);
        if(lnk.getValue() == it.getValue() && lnk.getSubValues() == it.getSubValues()){
            bool wasBlocked = _combo->signalsBlocked();
            _combo->blockSignals(true);
            _combo->setCurrentIndex(i);
            _combo->blockSignals(wasBlocked);
            return i;
        }
    }
    return -1;
}
