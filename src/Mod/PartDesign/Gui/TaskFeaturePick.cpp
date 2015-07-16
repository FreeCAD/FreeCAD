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
# include <QPixmap>
# include <QDialog>
# include <QListIterator>
#endif

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/MainWindow.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderOrigin.h>
#include <App/Document.h>
#include <App/Part.h>
#include <Base/Tools.h>
#include <Base/Reader.h>

#include "ui_TaskFeaturePick.h"
#include "TaskFeaturePick.h"
#include "Workbench.h"
#include <Mod/PartDesign/App/Body.h>

using namespace PartDesignGui;

const QString TaskFeaturePick::getFeatureStatusString(const featureStatus st)
{
    switch (st) {
        case validFeature: return tr("Valid");
        case invalidShape: return tr("Invalid shape");
        case noWire: return tr("No wire in sketch");
        case isUsed: return tr("Sketch already used by other feature");
        case otherBody: return tr("Belongs to another body");
        case otherPart: return tr("Belongs to another part");
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

    connect(ui->checkOtherBody, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->bodyRadioIndependent, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->bodyRadioXRef, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->checkOtherPart, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->partRadioIndependent, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->partRadioDependent, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    connect(ui->partRadioXRef, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
    
    auto guidoc = Gui::Application::Instance->activeDocument();
    auto origin_obj = App::GetApplication().getActiveDocument()->getObjectsOfType<App::Origin>();

    assert(status.size() == objects.size());
    std::vector<featureStatus>::const_iterator st = status.begin();
    for (std::vector<App::DocumentObject*>::const_iterator o = objects.begin(); o != objects.end(); o++) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromAscii((*o)->getNameInDocument()) +
                                                    QString::fromAscii(" (") + getFeatureStatusString(*st) + QString::fromAscii(")"));
        ui->listWidget->addItem(item);
        
        //check if we need to set any origin in temporary visibility mode
        for(App::Origin* obj : origin_obj) {
            if(obj->hasObject(*o) && (*st != invalidShape)) {
                Gui::ViewProviderOrigin* vpo = static_cast<Gui::ViewProviderOrigin*>(guidoc->getViewProvider(obj));
                if(!vpo->isTemporaryVisibilityMode())
                    vpo->setTemporaryVisibilityMode(true, guidoc);
                
                vpo->setTemporaryVisibility(*o, true);
                origins.push_back(vpo);
                break;
            }
        }
        
        st++;
    }

    groupLayout()->addWidget(proxy);
    statuses = status;
    updateList();
}

TaskFeaturePick::~TaskFeaturePick()
{
    for(Gui::ViewProviderOrigin* vpo : origins)
        vpo->setTemporaryVisibilityMode(false, NULL);

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
            case noWire: item->setHidden(true); break;
            case otherBody: item->setHidden(ui->checkOtherBody->isChecked() ? false : true); break;
            case otherPart: item->setHidden(ui->checkOtherPart->isChecked() ? false : true); break;
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
                    auto copy = makeCopy(obj, true);
                    activeBody->addFeature(copy);
                    result.push_back(copy);
                }
                else 
                    result.push_back(obj);
            }
            else if(*st == otherPart) {
            
                if(!ui->partRadioXRef->isChecked()) {                    
                    auto copy = makeCopy(obj, ui->partRadioIndependent->isChecked());
                    
                    auto oBody = PartDesignGui::getBodyFor(obj, false);
                    if(oBody)
                        activeBody->addFeature(copy);
                    else 
                        activePart->addObject(copy);
                    
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

App::DocumentObject* TaskFeaturePick::makeCopy(App::DocumentObject* obj, bool independent) {

    //we do know that the created instance is a document object, as obj is one. But we do not know which 
    //exact type
    auto name =  std::string("Copy") + std::string(obj->getNameInDocument());
    auto copy = App::GetApplication().getActiveDocument()->addObject(obj->getTypeId().getName(), name.c_str());
    
    if(copy) {
        //copy over all properties
        std::vector<App::Property*> props;
        std::vector<App::Property*> cprops;
        obj->getPropertyList(props);
        copy->getPropertyList(cprops);        
        try{
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
                    static_cast<App::PropertyString*>(cprop)->setValue("wuhahahahah");
                    continue;
                }
                   
                cprop->Paste(*prop);
            }   
        }
        catch(const Base::Exception& e) {
         
            Base::Console().Message("Exception: %s\n", e.what());
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

#include "moc_TaskFeaturePick.cpp"
