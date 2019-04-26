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
# include <QTimer>
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
#include <Base/Console.h>

#include "PropertyView.h"
#include "Application.h"
#include "MainWindow.h"
#include "Document.h"
#include "BitmapFactory.h"
#include "ViewProvider.h"
#include "ViewProviderDocumentObject.h"

#include "propertyeditor/PropertyEditor.h"

using namespace std;
using namespace Gui;
using namespace Gui::DockWnd;
using namespace Gui::PropertyEditor;

static ParameterGrp::handle _GetParam() {
    static ParameterGrp::handle hGrp;
    if(!hGrp) {
        hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/PropertyView");
    }
    return hGrp;
}

/* TRANSLATOR Gui::PropertyView */

/*! Property Editor Widget
 *
 * Provides two Gui::PropertyEditor::PropertyEditor widgets, for "View" and "Data",
 * in two tabs.
 */
PropertyView::PropertyView(QWidget *parent)
  : QWidget(parent),SelectionObserver(false) 
{
    QGridLayout* pLayout = new QGridLayout( this ); 
    pLayout->setSpacing(0);
    pLayout->setMargin (0);

    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));

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

    int preferredTab = _GetParam()->GetInt("LastTabIndex", 1);

    if ( preferredTab > 0 && preferredTab < tabs->count() )
        tabs->setCurrentIndex(preferredTab);

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
    this->connectUndoDocument =
    App::GetApplication().signalUndoDocument.connect(boost::bind
        (&PropertyView::slotRollback, this));
    this->connectRedoDocument =
    App::GetApplication().signalRedoDocument.connect(boost::bind
        (&PropertyView::slotRollback, this));
    this->connectActiveDoc =
    Application::Instance->signalActiveDocument.connect(boost::bind
        (&PropertyView::slotActiveDocument, this, _1));
}

PropertyView::~PropertyView()
{
    this->connectPropData.disconnect();
    this->connectPropView.disconnect();
    this->connectPropAppend.disconnect();
    this->connectPropRemove.disconnect();
    this->connectPropChange.disconnect();
    this->connectUndoDocument.disconnect();
    this->connectRedoDocument.disconnect();
    this->connectActiveDoc.disconnect();
}

static bool _ShowAll;

bool PropertyView::showAll() {
    return _ShowAll;
}

void PropertyView::setShowAll(bool enable) {
    if(_ShowAll != enable) {
        _ShowAll = enable;
        for(auto view : getMainWindow()->findChildren<PropertyView*>()) {
            if(view->isVisible())
                view->onTimer();
        }
    }
}

void PropertyView::hideEvent(QHideEvent *ev) {
    this->timer->stop();
    this->detachSelection();
    // clear the properties before hiding.
    propertyEditorData->buildUp();
    propertyEditorView->buildUp();
    clearPropertyItemSelection();
    QWidget::hideEvent(ev);
}

void PropertyView::showEvent(QShowEvent *ev) {
    this->attachSelection();
    this->timer->start(100);
    QWidget::showEvent(ev);
}

void PropertyView::clearPropertyItemSelection() {
    if(App::GetApplication().autoTransaction()) {
        QModelIndex index;
        propertyEditorData->clearSelection();
        propertyEditorData->setCurrentIndex(index);
        propertyEditorView->clearSelection();
        propertyEditorView->setCurrentIndex(index);
    }
}

void PropertyView::slotRollback() {
    // If auto transaction (BaseApp->Preferences->Document->AutoTransaction) is
    // enabled, PropertyItemDelegate will setup application active transaction
    // on entering edit mode, and close active transaction when exit editing.
    // But, when the user clicks undo/redo button while editing some property,
    // the current active transaction will be closed by design, which cause
    // further editing to be not recorded. Hence, we force unselect any property
    // item on undo/redo
    clearPropertyItemSelection();
}

void PropertyView::slotChangePropertyData(const App::DocumentObject&, const App::Property& prop)
{
    propertyEditorData->updateProperty(prop);
}

void PropertyView::slotChangePropertyView(const Gui::ViewProvider&, const App::Property& prop)
{
    propertyEditorView->updateProperty(prop);
}

bool PropertyView::isPropertyHidden(const App::Property *prop) {
    return prop && !showAll() &&
        ((prop->getType() & App::Prop_Hidden) || prop->testStatus(App::Property::Hidden));
}

void PropertyView::slotAppendDynamicProperty(const App::Property& prop)
{
    App::PropertyContainer* parent = prop.getContainer();
    if (isPropertyHidden(&prop)) 
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

void PropertyView::slotActiveDocument(const Gui::Document &doc)
{
    // allow to disable the auto-deactivation
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    bool enableEditor = hGrp->GetBool("EnablePropertyViewForInactiveDocument", true);
    if (enableEditor) {
        setEnabled(true);
        return;
    }

    // check if at least one selected object is part of the active document
    std::vector<SelectionSingleton::SelObj> array = Gui::Selection().getCompleteSelection();
    for (std::vector<SelectionSingleton::SelObj>::const_iterator it = array.begin(); it != array.end(); ++it) {
        if (Gui::Application::Instance->getDocument(it->pDoc) == &doc) {
            enableEditor = true;
            break;
        }
    }
    setEnabled(enableEditor || array.empty());
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

    // clear the properties.
    propertyEditorData->buildUp();
    propertyEditorView->buildUp();
    clearPropertyItemSelection();
    timer->start(100);
}

void PropertyView::onTimer() {
    timer->stop();
    std::set<App::DocumentObject *> objSet;
    // allow to disable the auto-deactivation
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    bool enableEditor = hGrp->GetBool("EnablePropertyViewForInactiveDocument", true);
    Gui::Document *activeDoc = Application::Instance->activeDocument();

    // group the properties by <name,id>
    std::vector<PropInfo> propDataMap;
    std::vector<PropInfo> propViewMap;
    bool checkLink = true;
    ViewProviderDocumentObject *vpLast = 0;
    const auto &array = Gui::Selection().getCompleteSelection(false);
    for(auto &sel : array) {
        if(!sel.pObject) continue;
        App::DocumentObject *parent = 0;
        App::DocumentObject *ob = sel.pObject->resolve(sel.SubName,&parent);
        if(!ob) continue;

        // App::Link should be able to handle special case below now, besides, the new
        // support of plain group in App::Link breaks because of the code below
#if 0
        if(parent) {
            auto parentVp = Application::Instance->getViewProvider(parent);
            if(parentVp) {
                // For special case where the SubName reference can resolve to
                // a non-child object (e.g. link array element), the tree view
                // will select the parent instead.  So we shall show the
                // property of the parent as well.
                bool found = false;
                for(auto child : parentVp->claimChildren()) {
                    if(ob == child) {
                        found = true;
                        break;
                    }
                }
                if(!found)
                    ob = parent;
            }
        }
#endif

        // Do not process an object more than once
        if(!objSet.insert(ob).second)
            continue;

        if(ob->getDocument() == activeDoc->getDocument())
            enableEditor = true;

        std::vector<App::Property*> dataList;
        std::map<std::string, App::Property*> viewList;

        auto vp = Application::Instance->getViewProvider(ob);
        if(!vp) {
            checkLink = false;
            ob->getPropertyList(dataList);
            continue;
        }

        if(vp->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) {
            auto cvp = static_cast<ViewProviderDocumentObject*>(vp);
            if(vpLast && cvp!=vpLast)
                checkLink = false;
            vpLast = cvp;
        }

        ob->getPropertyList(dataList);

        // get the properties as map here because it doesn't matter to have them sorted alphabetically
        vp->getPropertyMap(viewList);

        // store the properties with <name,id> as key in a map
        std::vector<App::Property*>::iterator pt;
        if (ob) {
            for (pt = dataList.begin(); pt != dataList.end(); ++pt) {
                if (isPropertyHidden(*pt))
                    continue;

                PropInfo nameType;
                nameType.propName = (*pt)->getName();
                nameType.propId = (*pt)->getTypeId().getKey();

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
        // the same for the view properties
        if (vp) {
            std::map<std::string, App::Property*>::iterator pt;
            for (pt = viewList.begin(); pt != viewList.end(); ++pt) {
                if (isPropertyHidden(pt->second))
                    continue;

                PropInfo nameType;
                nameType.propName = pt->first;
                nameType.propId = pt->second->getTypeId().getKey();

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

    // the property must be part of each selected object, i.e. the number
    // of selected objects is equal to the number of properties with same
    // name and id
    std::vector<PropInfo>::const_iterator it;
    PropertyModel::PropertyList dataProps;
    PropertyModel::PropertyList viewProps;

    if(checkLink && vpLast) {
        // In case the only selected object is a link, insert the link's own
        // property before the linked object
        App::DocumentObject *obj = vpLast->getObject();
        auto linked = obj;
        if(obj && obj->canLinkProperties() && (linked=obj->getLinkedObject(true))!=obj && linked) {
            std::vector<App::Property*> dataList;
            std::map<std::string, App::Property*> propMap;
            obj->getPropertyMap(propMap);
            linked->getPropertyList(dataList);
            for(auto prop : dataList) {
                if(isPropertyHidden(prop))
                    continue;
                std::string name(prop->getName());
                auto it = propMap.find(name);
                if(it!=propMap.end() && !isPropertyHidden(it->second))
                    continue;
                std::vector<App::Property*> v(1,prop);
                dataProps.emplace_back(name+"*", v);
            }
            auto vpLinked = Application::Instance->getViewProvider(linked);
            if(vpLinked) {
                propMap.clear();
                vpLast->getPropertyMap(propMap);
                dataList.clear();
                vpLinked->getPropertyList(dataList);
                for(auto prop : dataList) {
                    if(isPropertyHidden(prop))
                        continue;
                    std::string name(prop->getName());
                    auto it = propMap.find(name);
                    if(it!=propMap.end() && !isPropertyHidden(it->second))
                        continue;
                    std::vector<App::Property*> v(1,prop);
                    viewProps.emplace_back(name+"*", v);
                }
            }
        }
    }

    for (it = propDataMap.begin(); it != propDataMap.end(); ++it) {
        if (it->propList.size() == array.size())
            dataProps.push_back(std::make_pair(it->propName, it->propList));
    }

    propertyEditorData->buildUp(std::move(dataProps));

    for (it = propViewMap.begin(); it != propViewMap.end(); ++it) {
        if (it->propList.size() == array.size())
            viewProps.push_back(std::make_pair(it->propName, it->propList));
    }

    propertyEditorView->buildUp(std::move(viewProps));

    // make sure the editors are enabled/disabled properly
    setEnabled(enableEditor || array.empty());
}

void PropertyView::tabChanged(int index)
{
    _GetParam()->SetInt("LastTabIndex",index);
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
