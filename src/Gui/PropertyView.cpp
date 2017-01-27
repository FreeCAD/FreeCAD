/***************************************************************************
 *   Copyright (c) 2002 Juergen Riegel <juergen.riegel@web.de>             *
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
# include <QGridLayout>
# include <QHeaderView>
# include <QEvent>
#endif

#include <boost/bind.hpp>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Parameter.h>
#include <App/PropertyStandard.h>
#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <App/PropertyContainer.h>
#include <App/DocumentObject.h>
#include <App/Document.h>

#include "PropertyView.h"
#include "Application.h"
#include "Document.h"
#include "BitmapFactory.h"
#include "ViewProvider.h"

#include "propertyeditor/PropertyEditor.h"

using namespace std;
using namespace Gui;
using namespace Gui::DockWnd;
using namespace Gui::PropertyEditor;


/* TRANSLATOR Gui::PropertyView */

/*! Property Editor Widget
 *
 * Provides two Gui::PropertyEditor::PropertyEditor widgets, for "View" and "Data",
 * in two tabs.
 */
PropertyView::PropertyView(QWidget *parent)
  : QWidget(parent)
{
    QGridLayout* pLayout = new QGridLayout( this ); 
    pLayout->setSpacing(0);
    pLayout->setMargin (0);

    tabs = new QTabWidget (this);
    tabs->setObjectName(QString::fromUtf8("propertyTab"));
    tabs->setTabPosition(QTabWidget::South);
#if defined(Q_OS_WIN32)
    tabs->setTabShape(QTabWidget::Triangular);
#endif
    pLayout->addWidget(tabs, 0, 0);

    propertyEditorView = new Gui::PropertyEditor::PropertyEditor();
    propertyEditorView->setAutomaticDocumentUpdate(false);
    tabs->addTab(propertyEditorView, tr("View"));

    propertyEditorData = new Gui::PropertyEditor::PropertyEditor();
    propertyEditorData->setAutomaticDocumentUpdate(true);
    tabs->addTab(propertyEditorData, tr("Data"));

    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter().
        GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("PropertyView");
    if ( hGrp ) {
        int preferredTab = hGrp->GetInt("LastTabIndex", 1);

        if ( preferredTab > 0 && preferredTab < tabs->count() )
            tabs->setCurrentIndex(preferredTab);
    }

    // connect after adding all tabs, so adding doesn't thrash the parameter
    connect(tabs, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

    this->connectPropData =
    App::GetApplication().signalChangedObject.connect(boost::bind
        (&PropertyView::slotChangePropertyData, this, _1, _2));
    this->connectPropView =
    Gui::Application::Instance->signalChangedObject.connect(boost::bind
        (&PropertyView::slotChangePropertyView, this, _1, _2));
    this->connectPropAppend =
    App::GetApplication().signalAppendDynamicProperty.connect(boost::bind
        (&PropertyView::slotAppendDynamicProperty, this, _1));
    this->connectPropRemove =
    App::GetApplication().signalRemoveDynamicProperty.connect(boost::bind
        (&PropertyView::slotRemoveDynamicProperty, this, _1));
    this->connectPropChange =
    App::GetApplication().signalChangePropertyEditor.connect(boost::bind
        (&PropertyView::slotChangePropertyEditor, this, _1));
}

PropertyView::~PropertyView()
{
    this->connectPropData.disconnect();
    this->connectPropView.disconnect();
    this->connectPropAppend.disconnect();
    this->connectPropRemove.disconnect();
    this->connectPropChange.disconnect();
}

void PropertyView::slotChangePropertyData(const App::DocumentObject&, const App::Property& prop)
{
    propertyEditorData->updateProperty(prop);
}

void PropertyView::slotChangePropertyView(const Gui::ViewProvider&, const App::Property& prop)
{
    propertyEditorView->updateProperty(prop);
}

void PropertyView::slotAppendDynamicProperty(const App::Property& prop)
{
    App::PropertyContainer* parent = prop.getContainer();
    if (parent->isHidden(&prop))
        return;

    if (parent->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        propertyEditorData->appendProperty(prop);
    }
    else if (parent->isDerivedFrom(Gui::ViewProvider::getClassTypeId())) {
        propertyEditorView->appendProperty(prop);
    }
}

void PropertyView::slotRemoveDynamicProperty(const App::Property& prop)
{
    App::PropertyContainer* parent = prop.getContainer();
    if (parent && parent->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        propertyEditorData->removeProperty(prop);
    }
    else if (parent && parent->isDerivedFrom(Gui::ViewProvider::getClassTypeId())) {
        propertyEditorView->removeProperty(prop);
    }
}

void PropertyView::slotChangePropertyEditor(const App::Property& prop)
{
    App::PropertyContainer* parent = prop.getContainer();
    if (parent && parent->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        propertyEditorData->updateEditorMode(prop);
    }
    else if (parent && parent->isDerivedFrom(Gui::ViewProvider::getClassTypeId())) {
        propertyEditorView->updateEditorMode(prop);
    }
}

struct PropertyView::PropInfo
{
    std::string propName;
    int propId;
    std::vector<App::Property*> propList;
};

struct PropertyView::PropFind {
    const PropInfo& item;
    PropFind(const PropInfo& item) : item(item) {}
    bool operator () (const PropInfo& elem) const
    {
        return (elem.propId == item.propId) &&
               (elem.propName == item.propName);
    }
};

void PropertyView::onSelectionChanged(const SelectionChanges& msg)
{
    if (msg.Type != SelectionChanges::AddSelection &&
        msg.Type != SelectionChanges::RmvSelection &&
        msg.Type != SelectionChanges::SetSelection &&
        msg.Type != SelectionChanges::ClrSelection)
        return;

    // group the properties by <name,id>
    std::vector<PropInfo> propDataMap;
    std::vector<PropInfo> propViewMap;
    std::vector<SelectionSingleton::SelObj> array = Gui::Selection().getCompleteSelection();
    for (std::vector<SelectionSingleton::SelObj>::const_iterator it = array.begin(); it != array.end(); ++it) {
        App::DocumentObject *ob=0;
        ViewProvider *vp=0;

        std::vector<App::Property*> dataList;
        std::map<std::string, App::Property*> viewList;
        if ((*it).pObject) {
            (*it).pObject->getPropertyList(dataList);
            ob = (*it).pObject;

            // get also the properties of the associated view provider
            Gui::Document* doc = Gui::Application::Instance->getDocument(it->pDoc);
            vp = doc->getViewProvider((*it).pObject);
            if(!vp) continue;
            // get the properties as map here because it doesn't matter to have them sorted alphabetically
            vp->getPropertyMap(viewList);
        }

        // store the properties with <name,id> as key in a map
        std::vector<App::Property*>::iterator pt;
        if (ob) {
            for (pt = dataList.begin(); pt != dataList.end(); ++pt) {
                PropInfo nameType;
                nameType.propName = ob->getPropertyName(*pt);
                nameType.propId = (*pt)->getTypeId().getKey();

                if (!ob->isHidden(*pt)) {
                    std::vector<PropInfo>::iterator pi = std::find_if(propDataMap.begin(), propDataMap.end(), PropFind(nameType));
                    if (pi != propDataMap.end()) {
                        pi->propList.push_back(*pt);
                    }
                    else {
                        nameType.propList.push_back(*pt);
                        propDataMap.push_back(nameType);
                    }
                }
            }
        }
        // the same for the view properties
        if (vp) {
            std::map<std::string, App::Property*>::iterator pt;
            for (pt = viewList.begin(); pt != viewList.end(); ++pt) {
                PropInfo nameType;
                nameType.propName = pt->first;
                nameType.propId = pt->second->getTypeId().getKey();

                if (!vp->isHidden(pt->second)) {
                    std::vector<PropInfo>::iterator pi = std::find_if(propViewMap.begin(), propViewMap.end(), PropFind(nameType));
                    if (pi != propViewMap.end()) {
                        pi->propList.push_back(pt->second);
                    }
                    else {
                        nameType.propList.push_back(pt->second);
                        propViewMap.push_back(nameType);
                    }
                }
            }
        }
    }

    // the property must be part of each selected object, i.e. the number
    // of selected objects is equal to the number of properties with same
    // name and id
    std::vector<PropInfo>::const_iterator it;
    PropertyModel::PropertyList dataProps;
    for (it = propDataMap.begin(); it != propDataMap.end(); ++it) {
        if (it->propList.size() == array.size()) {
            dataProps.push_back(std::make_pair(it->propName, it->propList));
        }
    }
    propertyEditorData->buildUp(dataProps);

    PropertyModel::PropertyList viewProps;
    for (it = propViewMap.begin(); it != propViewMap.end(); ++it) {
        if (it->propList.size() == array.size()) {
            viewProps.push_back(std::make_pair(it->propName, it->propList));
        }
    }
    propertyEditorView->buildUp(viewProps);
}

void PropertyView::tabChanged(int index)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter().
        GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("PropertyView");
    if (hGrp) {
        hGrp->SetInt("LastTabIndex", index);
    }
}

void PropertyView::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        tabs->setTabText(0, trUtf8("View"));
        tabs->setTabText(1, trUtf8("Data"));
    }

    QWidget::changeEvent(e);
}

/* TRANSLATOR Gui::DockWnd::PropertyDockView */

PropertyDockView::PropertyDockView(Gui::Document* pcDocument, QWidget *parent)
  : DockWindow(pcDocument,parent)
{
    setWindowTitle(tr("Property View"));

    PropertyView* view = new PropertyView(this);
    QGridLayout* pLayout = new QGridLayout(this);
    pLayout->setSpacing(0);
    pLayout->setMargin (0);
    pLayout->addWidget(view, 0, 0);

    resize( 200, 400 );
}

PropertyDockView::~PropertyDockView()
{
}

#include "moc_PropertyView.cpp"
