/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
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
# include <QMessageBox>
# include <QListWidget>
# include <QAction>
# include <QCheckBox>
# include <TopoDS_Shape.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS.hxx>
# include <BRepAdaptor_Surface.hxx>
#endif

#include <boost/algorithm/string/predicate.hpp>

#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/Origin.h>
#include <App/OriginFeature.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>

#include <Mod/PartDesign/App/FeatureTransformed.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureAddSub.h>

#include "ReferenceSelection.h"
#include "TaskMultiTransformParameters.h"
#include "Utils.h"

#include "TaskTransformedParameters.h"

FC_LOG_LEVEL_INIT("PartDesign",true,true)

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskTransformedParameters */

TaskTransformedParameters::TaskTransformedParameters(ViewProviderTransformed *TransformedView, QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap((std::string("PartDesign_") + TransformedView->featureName).c_str()),
              QString::fromLatin1((TransformedView->featureName + " parameters").c_str()),
              true,
              parent),
      proxy(nullptr),
      TransformedView(TransformedView),
      parentTask(nullptr),
      insideMultiTransform(false),
      blockUpdate(false)
{
    selectionMode = none;

    if (TransformedView) {
        Gui::Document* doc = TransformedView->getDocument();
        this->attachDocument(doc);
    }
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

bool TaskTransformedParameters::originalSelected(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection && (
                (selectionMode == addFeature) || (selectionMode == removeFeature))) {

        if (strcmp(msg.pDocName, getObject()->getDocument()->getName()) != 0)
            return false;

        PartDesign::Transformed* pcTransformed = getObject();
        auto selectedObject = Base::freecad_dynamic_cast<PartDesign::Feature>(
                pcTransformed->getDocument()->getObject(msg.pObjectName));
        if (selectedObject) {
            auto subset = pcTransformed->Originals.getSubListValues();
            std::map<App::DocumentObject*,std::pair<size_t,std::set<std::string> > > submap;
            for(auto it=subset.begin(),itNext=it;it!=subset.end();it=itNext) {
                ++itNext;
                auto &info = submap[it->first];
                if(info.second.empty()) {
                    info.first = it - subset.begin();
                    auto &subs = it->second;
                    for(auto itSub=subs.begin(),itSubNext=itSub;itSub!=subs.end();itSub=itSubNext) {
                        ++itSubNext;
                        if(!info.second.insert(*itSub).second)
                            subs.erase(itSub);
                    }
                }else{
                    auto &subs = subset[info.first].second;
                    for(auto &sub : it->second) {
                        if(info.second.insert(sub).second)
                            subs.push_back(sub);
                    }
                    subset.erase(it);
                }
            }

            auto o = submap.find(selectedObject);
            if (selectionMode == addFeature) {
                if (o == submap.end()) {
                    std::vector<std::string> subs;
                    if(selectedObject->Shape.getShape().countSubShapes(TopAbs_SOLID))
                        subs.push_back(msg.SubName);
                    subset.emplace_back(selectedObject,subs);
                } else if(o->second.second.insert(msg.SubName).second) 
                    subset[o->second.first].second.push_back(msg.SubName);
                else
                    return false; // duplicate selection
            } else {
                if (o == submap.end())
                    return false;
                if(msg.SubName.empty())
                    subset.erase(subset.begin()+o->second.first);
                else {
                    auto &subs = subset[o->second.first].second;
                    auto it = std::find(subs.begin(),subs.end(),msg.SubName);
                    if(it==subs.end())
                        return false;
                    subs.erase(it);
                }
            }
            setupTransaction();
            pcTransformed->Originals.setSubListValues(subset);
            populate();
            recomputeFeature();
            return true;
        }
    }

    return false;
}

void TaskTransformedParameters::setupTransaction() {
    int tid = 0;
    const char *name = App::GetApplication().getActiveTransaction(&tid);
    std::string n("Edit ");
    n += getObject()->Label.getValue();
    if(!name || n != name)
        App::GetApplication().setActiveTransaction(n.c_str());
}

void TaskTransformedParameters::populate() {
    if(!listWidget) 
        return;
    PartDesign::Transformed* pcTransformed = getObject();
    listWidget->clear();
    auto values = pcTransformed->Originals.getValues();
    auto itValue = values.begin();
    const auto &shadows = pcTransformed->Originals.getShadowSubs();
    auto itShadow = shadows.begin();
    PartDesign::Feature *feat = 0;
    auto subs = pcTransformed->Originals.getSubValues(false);
    bool touched = false;
    for(auto &sub : subs) {
        bool missing = false;
        auto obj = *itValue++;
        const auto &shadow = *itShadow++;
        if(feat!=obj)
            feat = Base::freecad_dynamic_cast<PartDesign::Feature>(obj);
        if(feat && shadow.first.size()) {
            try {
                feat->Shape.getShape().getSubShape(shadow.first.c_str());
            }catch(...) {
                auto names = Part::Feature::getRelatedElements(obj,shadow.first.c_str());
                if(names.size()) {
                    auto &name = names.front();
                    FC_WARN("guess element reference: " << shadow.first << " -> " << name.first);
                    touched = true;
                }else{
                    if(!boost::starts_with(sub,Data::ComplexGeoData::missingPrefix()))
                        sub = Data::ComplexGeoData::missingPrefix() + sub;
                    missing = true;
                }
            }
        }
        QString label = QString::fromUtf8(obj->Label.getValue());
        QLatin1String objectName(obj->getNameInDocument());
        QLatin1String subName(sub.c_str());
        if(sub.size()) 
            label += QString::fromLatin1(" (%1)").arg(subName);
        QListWidgetItem* item = new QListWidgetItem(listWidget);
        item->setText(label);
        item->setData(Qt::UserRole, objectName);
        item->setData(Qt::UserRole+1, subName);
        if(missing)
            item->setForeground(Qt::red);
    }

    if(touched) {
        setupTransaction();
        getObject()->Originals.setValues(values,subs);
        recomputeFeature();
    }
}

// Make sure only some feature before the given one is visible
static void checkVisibility(PartDesign::Feature *feat) {
    auto body = feat->getFeatureBody();
    if(!body) return;
    auto inset = feat->getInListEx(true);
    for(auto o : body->Group.getValues()) {
        if(!o->Visibility.getValue() 
                || !o->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
            continue;
        if(inset.count(o))
            break;
        return;
    }
    FCMD_OBJ_SHOW(feat->getBaseObject());
}

void TaskTransformedParameters::onButtonAddFeature(bool checked)
{
    if (checked) {
        checkVisibility(getObject());
        selectionMode = addFeature;
        Gui::Selection().clearSelection();
    } else {
        exitSelectionMode();
    }
}

void TaskTransformedParameters::setupListWidget(QListWidget *widget) {
    listWidget = widget;
    QAction* action = new QAction(tr("Remove"), widget);
    listWidget->addAction(action);
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(onFeatureDeleted()));
    listWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
    populate();
}

void TaskTransformedParameters::onFeatureDeleted(void) {
    PartDesign::Transformed* pcTransformed = getObject();
    auto values = pcTransformed->Originals.getValues();
    auto subs = pcTransformed->Originals.getSubValues(false);
    if(values.size()==subs.size() && listWidget->currentRow()<(int)values.size()) {
        values.erase(values.begin() + listWidget->currentRow());
        subs.erase(subs.begin() + listWidget->currentRow());

        pcTransformed->Originals.setValues(values,subs);
        recomputeFeature();
    }
    populate();
}

void TaskTransformedParameters::onButtonRemoveFeature(bool checked)
{
    if (checked) {
        checkVisibility(getObject());
        selectionMode = removeFeature;
        Gui::Selection().clearSelection();
    } else {
        exitSelectionMode();
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
    combolinks.addLink(0,std::string(),tr("Select reference..."));
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
    combolinks.addLink(0,std::string(),tr("Select reference..."));
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
    assert (rv);

    return rv;
}

PartDesign::Transformed *TaskTransformedParameters::getTopTransformedObject() const {
    App::DocumentObject *transform = getTopTransformedView()->getObject();
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

Part::Feature *TaskTransformedParameters::getBaseObject() const {
    PartDesign::Feature* feature = getTopTransformedObject ();
    // NOTE: getBaseObject() throws if there is no base; shouldn't happen here.
    return feature->getBaseObject();
}

App::DocumentObject* TaskTransformedParameters::getSketchObject() const {
    return getTopTransformedObject()->getSketchObject();
}

void TaskTransformedParameters::hideObject()
{
    FCMD_OBJ_HIDE(getTopTransformedObject());
}

void TaskTransformedParameters::showObject()
{
    FCMD_OBJ_SHOW(getTopTransformedObject());
}

void TaskTransformedParameters::hideBase()
{
    FCMD_OBJ_HIDE(getBaseObject());
}

void TaskTransformedParameters::showBase()
{
    FCMD_OBJ_SHOW(getBaseObject());
}

void TaskTransformedParameters::exitSelectionMode()
{
    clearButtons();
    selectionMode = none;
    Gui::Selection().rmvSelectionGate();
    showObject();
    hideBase();
}

void TaskTransformedParameters::addReferenceSelectionGate(bool edge, bool face)
{
    std::unique_ptr<Gui::SelectionFilterGate> gateRefPtr(new ReferenceSelection(getBaseObject(), edge, face, /*point =*/ true));
    std::unique_ptr<Gui::SelectionFilterGate> gateDepPtr(new NoDependentsSelection(getTopTransformedObject()));
    Gui::Selection().addSelectionGate(new CombineSelectionFilterGates(gateRefPtr, gateDepPtr));
}

void TaskTransformedParameters::setupCheckBox(QCheckBox *checkbox) {
    checkbox->setChecked(getObject()->SubTransform.getValue());
    QObject::connect(checkbox, SIGNAL(toggled(bool)), this, SLOT(onChangedSubTransform(bool)));
}

void TaskTransformedParameters::onChangedSubTransform(bool checked) {
    setupTransaction();
    getObject()->SubTransform.setValue(checked);
    recomputeFeature();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgTransformedParameters::TaskDlgTransformedParameters(
        ViewProviderTransformed *TransformedView_, TaskTransformedParameters *parameter)
    : TaskDlgFeatureParameters(TransformedView_), parameter(parameter)
{
    assert(vp);
    message = new TaskTransformedMessages(getTransformedView());
    Content.push_back(message);

    parameter->setupCheckBox(message->getCheckBox());
    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

bool TaskDlgTransformedParameters::accept()
{
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
    : doc(0)
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
    if (newitem.getValue() && this->doc == 0)
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
    if (newitem.getValue() && this->doc == 0)
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
    if (index < 0 || index > (ssize_t) linksInList.size()-1)
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
