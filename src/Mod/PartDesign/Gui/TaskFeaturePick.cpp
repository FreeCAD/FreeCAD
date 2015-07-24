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
# include <QListIterator>
#endif

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/MainWindow.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderOrigin.h>
#include <App/Document.h>
#include <App/Origin.h>
#include <App/OriginFeature.h>
#include <App/Part.h>
#include <Base/Tools.h>
#include <Base/Reader.h>

#include <Mod/PartDesign/App/Body.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "Utils.h"

#include "ui_TaskFeaturePick.h"
#include "TaskFeaturePick.h"
#include <Mod/PartDesign/App/ShapeBinder.h>
#include <Mod/PartDesign/App/DatumPoint.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include <Mod/Part/App/DatumFeature.h>

using namespace PartDesignGui;

// TODO Do ve should snap here to App:Part or GeoFeatureGroup/DocumentObjectGroup ? (2015-09-04, Fat-Zer)
const QString TaskFeaturePick::getFeatureStatusString(const featureStatus st)
{
    switch (st) {
        case validFeature: return tr("Valid");
        case invalidShape: return tr("Invalid shape");
        case noWire: return tr("No wire in sketch");
        case isUsed: return tr("Sketch already used by other feature");
        case otherBody: return tr("Belongs to another body");
        case otherPart: return tr("Belongs to another part");
        case notInBody: return tr("Doesn't belongs to any body");
        case basePlane: return tr("Base plane");
        case afterTip: return tr("Feature is located after the tip feature");
    }

    return tr("");
}

TaskFeaturePick::TaskFeaturePick(std::vector<App::DocumentObject*>& objects,
                                     const std::vector<featureStatus>& status,
                                     QWidget* parent)
  : TaskBox(Gui::BitmapFactory().pixmap("edit-select-box"),
            QString::fromAscii("Select feature"), true, parent), ui(new Ui_TaskFeaturePick)
{

    proxy = new QWidget(this);
    ui->setupUi(proxy);

    connect(ui->checkUsed, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->checkOtherBody, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->bodyRadioIndependent, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->bodyRadioXRef, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->checkOtherPart, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->partRadioIndependent, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->partRadioDependent, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->partRadioXRef, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->checkNoBody, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->nobodyRadioIndependent, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->nobodyRadioXRef, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));

    enum { axisBit=0, planeBit = 1};

    // NOTE: generally there shouldn't be more then one origin
    std::map <App::Origin*, std::bitset<2> > originVisStatus;

    auto statusIt = status.cbegin();
    auto objIt = objects.begin();
    assert(status.size() == objects.size());
    for (; statusIt != status.end(); ++statusIt, ++objIt) {
        QListWidgetItem* item = new QListWidgetItem(
                QString::fromAscii((*objIt)->getNameInDocument()) +
                QString::fromAscii(" (") + getFeatureStatusString(*statusIt) + QString::fromAscii(")") );
        ui->listWidget->addItem(item);

        //check if we need to set any origin in temporary visibility mode
        if (*statusIt != invalidShape && (*objIt)->isDerivedFrom ( App::OriginFeature::getClassTypeId () )) {
            App::Origin *origin = static_cast<App::OriginFeature*> (*objIt)->getOrigin ();
            if (origin) {
                if ((*objIt)->isDerivedFrom ( App::Plane::getClassTypeId () )) {
                    originVisStatus[ origin ].set (planeBit, true);
                } else if ( (*objIt)->isDerivedFrom ( App::Line::getClassTypeId () ) ) {
                    originVisStatus[ origin ].set (axisBit, true);
                }
            }
        }
    }

    // Setup the origin's temporary visability
    for ( const auto & originPair: originVisStatus ) {
        const auto &origin = originPair.first;
        const auto &status = originPair.second;

        Gui::ViewProviderOrigin* vpo = static_cast<Gui::ViewProviderOrigin*> (
                Gui::Application::Instance->getViewProvider ( origin ) );
        if (vpo) {
            vpo->setTemporaryVisibility( originVisStatus[origin][axisBit],
                    originVisStatus[origin][planeBit]);
            origins.push_back(vpo);
        }
    }

    // TODO may be update origin API to show only some objects (2015-08-31, Fat-Zer)

    groupLayout()->addWidget(proxy);
    statuses = status;
    updateList();
}

TaskFeaturePick::~TaskFeaturePick()
{
    for(Gui::ViewProviderOrigin* vpo : origins)
        vpo->resetTemporaryVisibility();

}

void TaskFeaturePick::updateList()
{
    int index = 0;

    //get all origins in temporary mode


    for (std::vector<featureStatus>::const_iterator st = statuses.begin(); st != statuses.end(); st++) {
        QListWidgetItem* item = ui->listWidget->item(index);

        switch (*st) {
            case validFeature: item->setHidden(false); break;
            case invalidShape: item->setHidden(true); break;
            case isUsed: item->setHidden(!ui->checkUsed->isChecked()); break;
            case noWire: item->setHidden(true); break;
            case otherBody: item->setHidden(!ui->checkOtherBody->isChecked()); break;
            case otherPart: item->setHidden(!ui->checkOtherPart->isChecked()); break;
            case notInBody: item->setHidden(!ui->checkNoBody->isChecked()); break;
            case basePlane: item->setHidden(false); break;
            case afterTip:  item->setHidden(true); break;
        }

        index++;
    }
}

void TaskFeaturePick::onUpdate(bool)
{
    updateList();
}

std::vector<App::DocumentObject*> TaskFeaturePick::getFeatures() {
    
    features.clear();
    QListIterator<QListWidgetItem*> i(ui->listWidget->selectedItems());
    while (i.hasNext()) {
        
        auto item = i.next();
        if(item->isHidden())
            continue;
        
        QString t = item->text();
        t = t.left(t.indexOf(QString::fromAscii("(")) - 1);
        features.push_back(t);
    }
    
    std::vector<App::DocumentObject*> result;

    for (std::vector<QString>::const_iterator s = features.begin(); s != features.end(); s++)
        result.push_back(App::GetApplication().getActiveDocument()->getObject(s->toAscii().data()));

    return result;
}

std::vector<App::DocumentObject*> TaskFeaturePick::buildFeatures() {

    int index = 0;
    std::vector<App::DocumentObject*> result;
    auto activeBody = PartDesignGui::getBody(false);
    auto activePart = PartDesignGui::getPartFor(activeBody, false);

    for (std::vector<featureStatus>::const_iterator st = statuses.begin(); st != statuses.end(); st++) {
        QListWidgetItem* item = ui->listWidget->item(index);

        if(item->isSelected() && !item->isHidden()) {

            QString t = item->text();
            t = t.left(t.indexOf(QString::fromAscii("(")) - 1);
            auto obj = App::GetApplication().getActiveDocument()->getObject(t.toAscii().data());

            //build the dependend copy if wanted by the user
            if(*st == otherBody) {

                if(ui->bodyRadioIndependent->isChecked()) {
                    auto copy = makeCopy(obj, "", true);
                    activeBody->addFeature(copy);
                    result.push_back(copy);
                } else {
                    result.push_back(obj);
                }
            }
            else if(*st == otherPart) {

                if(!ui->partRadioXRef->isChecked()) {
                    auto copy = makeCopy(obj, "", ui->partRadioIndependent->isChecked());

                    auto oBody = PartDesignGui::getBodyFor(obj, false);
                    if(oBody)
                        activeBody->addFeature(copy);
                    else
                        activePart->addObject(copy);

                    result.push_back(copy);
                }
                else
                    result.push_back(obj);
            } else if(*st == notInBody) {
                if(ui->bodyRadioIndependent->isChecked()) {
                    auto copy = makeCopy(obj, "", true);
                    activeBody->addFeature(copy);
                    // doesn't supposed to get here anything but sketch but to be on the safe side better to check
                    if (copy->getTypeId().isDerivedFrom(Sketcher::SketchObject::getClassTypeId())) {
                        Sketcher::SketchObject *sketch = static_cast<Sketcher::SketchObject*>(copy);
                        PartDesignGui::fixSketchSupport(sketch);
                    }
                    result.push_back(copy);
                }
                else
                    result.push_back(obj);
            }
            else
                result.push_back(obj);

            break;
        }

        index++;
    }

    return result;
}

App::DocumentObject* TaskFeaturePick::makeCopy(App::DocumentObject* obj, std::string sub, bool independent) {
    App::DocumentObject* copy = nullptr;
    if( independent &&
        (obj->isDerivedFrom(Sketcher::SketchObject::getClassTypeId()) ||
        obj->isDerivedFrom(PartDesign::FeaturePrimitive::getClassTypeId()))) {

        //we do know that the created instance is a document object, as obj is one. But we do not know which
        //exact type
        auto name =  std::string("Copy") + std::string(obj->getNameInDocument());
        copy = App::GetApplication().getActiveDocument()->addObject(obj->getTypeId().getName(), name.c_str());

        //copy over all properties
        std::vector<App::Property*> props;
        std::vector<App::Property*> cprops;
        obj->getPropertyList(props);
        copy->getPropertyList(cprops);

        auto it = cprops.begin();
        for( App::Property* prop : props ) {

            //independent copys dont have links and are not attached
            if(independent && (
                prop->getTypeId() == App::PropertyLink::getClassTypeId() ||
                prop->getTypeId() == App::PropertyLinkList::getClassTypeId() ||
                prop->getTypeId() == App::PropertyLinkSub::getClassTypeId() ||
                prop->getTypeId() == App::PropertyLinkSubList::getClassTypeId()||
                ( prop->getGroup() && strcmp(prop->getGroup(),"Attachment")==0) ))    {

                ++it;
                continue;
            }

            App::Property* cprop = *it++;

            if( strcmp(prop->getName(), "Label") == 0 ) {
                static_cast<App::PropertyString*>(cprop)->setValue(name.c_str());
                continue;
            }

            cprop->Paste(*prop);

            //we are a independent copy, therefore no external geometry was copied. WE therefore can delete all
            //contraints
            if(obj->isDerivedFrom(Sketcher::SketchObject::getClassTypeId()))
                static_cast<Sketcher::SketchObject*>(copy)->delConstraintsToExternal();
        }
    }
    else {

        auto name =  std::string("Reference") + std::string(obj->getNameInDocument());
        const char* entity = std::string("").c_str();
        if(!sub.empty())
            entity = sub.c_str();

        Part::PropertyPartShape* shapeProp = nullptr;

        // TODO Replace it with commands (2015-09-11, Fat-Zer)
        if(obj->isDerivedFrom(Part::Datum::getClassTypeId())) {
            copy = App::GetApplication().getActiveDocument()->addObject(
                    obj->getClassTypeId().getName(), name.c_str() );

            //we need to reference the individual datums and make again datums. This is important as
            //datum adjust their size dependend on the part size, hence simply copying the shape is
            //not enough
            long int mode = mmDeactivated;
            Part::Datum *datumCopy = static_cast<Part::Datum*>(copy);

            if(obj->getTypeId() == PartDesign::Point::getClassTypeId()) {
                mode = mm0Vertex;
            }
            else if(obj->getTypeId() == PartDesign::Line::getClassTypeId()) {
                mode = mm1TwoPoints;
            }
            else if(obj->getTypeId() == PartDesign::Plane::getClassTypeId()) {
                mode = mmFlatFace;
            }
            else
                return copy;

            // TODO Recheck this. This looks strange in case of independent copy (2015-10-31, Fat-Zer)
            if(!independent) {
                datumCopy->Support.setValue(obj, entity);
                datumCopy->MapMode.setValue(mode);
            }
            else if(strcmp(entity,"") != 0) {
                datumCopy->Shape.setValue(static_cast<Part::Datum*>(obj)->Shape.getShape().getSubShape(entity));
            } else {
                datumCopy->Shape.setValue(static_cast<Part::Datum*>(obj)->Shape.getValue());
            }
        }
        else if(obj->isDerivedFrom(Part::Part2DObject::getClassTypeId()) ||
                obj->getTypeId() == PartDesign::ShapeBinder2D::getClassTypeId()) {
            copy = App::GetApplication().getActiveDocument()->addObject("PartDesign::ShapeBinder2D", name.c_str());

            if(!independent)
                static_cast<PartDesign::ShapeBinder2D*>(copy)->Support.setValue(obj, entity);
            else
                shapeProp = &static_cast<PartDesign::ShapeBinder*>(copy)->Shape;
        }
        else if(obj->getTypeId() == PartDesign::ShapeBinder::getClassTypeId() ||
                obj->isDerivedFrom(Part::Feature::getClassTypeId())) {

            copy = App::GetApplication().getActiveDocument()->addObject("PartDesign::ShapeBinder", name.c_str());

            if(!independent)
                static_cast<PartDesign::ShapeBinder*>(copy)->Support.setValue(obj, entity);
            else
                shapeProp = &static_cast<PartDesign::ShapeBinder*>(copy)->Shape;
        }
    }

    return copy;
}


void TaskFeaturePick::onSelectionChanged(const Gui::SelectionChanges& msg)
{    
    ui->listWidget->clearSelection();
    for(Gui::SelectionSingleton::SelObj obj :  Gui::Selection().getSelection()) {
        
        for(int row = 0; row < ui->listWidget->count(); row++) {
            
            QListWidgetItem *item = ui->listWidget->item(row);
            QString t = item->text();
            t = t.left(t.indexOf(QString::fromAscii("(")) - 1);
            if(t.compare(QString::fromAscii(obj.FeatName))==0) {
                ui->listWidget->setItemSelected(item, true);
            }
        }
    }
}

void TaskFeaturePick::showExternal(bool val) {

    ui->checkOtherBody->setChecked(val);
    ui->checkOtherPart->setChecked(val);
    ui->checkNoBody->setChecked(val);
    updateList();
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFeaturePick::TaskDlgFeaturePick(std::vector<App::DocumentObject*> &objects, 
                                        const std::vector<TaskFeaturePick::featureStatus> &status,
                                        boost::function<bool (std::vector<App::DocumentObject*>)> afunc,
                                        boost::function<void (std::vector<App::DocumentObject*>)> wfunc)
    : TaskDialog(), accepted(false)
{
    pick  = new TaskFeaturePick(objects, status);
    Content.push_back(pick);
    
    acceptFunction = afunc;
    workFunction = wfunc;
}

TaskDlgFeaturePick::~TaskDlgFeaturePick()
{
    //do the work now as before in accept() the dialog is still open, hence the work 
    //function could not open annother dialog    
    if(accepted)
        workFunction(pick->buildFeatures());
}

//==== calls from the TaskView ===============================================================


void TaskDlgFeaturePick::open()
{
    
}

void TaskDlgFeaturePick::clicked(int)
{
    
}

bool TaskDlgFeaturePick::accept()
{
    accepted = acceptFunction(pick->getFeatures());
     
    return accepted;
}

bool TaskDlgFeaturePick::reject()
{
    accepted = false;
    return true;
}

void TaskDlgFeaturePick::showExternal(bool val) {
    pick->showExternal(val);
}


#include "moc_TaskFeaturePick.cpp"
