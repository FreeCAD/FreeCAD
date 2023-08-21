/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QEvent>
# include <QGridLayout>
# include <QTimer>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>

#include "PropertyView.h"
#include "Application.h"
#include "Document.h"
#include "MainWindow.h"
#include "SelectionObject.h"
#include "Tree.h"
#include "ViewParams.h"
#include "ViewProvider.h"
#include "ViewProviderDocumentObject.h"
#include "propertyeditor/PropertyEditor.h"


using namespace std;
using namespace Gui;
using namespace Gui::DockWnd;
using namespace Gui::PropertyEditor;
namespace sp = std::placeholders;

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
  : QWidget(parent), SelectionObserver(false, ResolveMode::NoResolve)
{
    auto pLayout = new QGridLayout( this );
    pLayout->setSpacing(0);
    pLayout->setContentsMargins(0, 0, 0, 0);

    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &PropertyView::onTimer);

    tabs = new QTabWidget (this);
    tabs->setObjectName(QString::fromUtf8("propertyTab"));
    tabs->setTabPosition(QTabWidget::South);
    pLayout->addWidget(tabs, 0, 0);

    propertyEditorView = new Gui::PropertyEditor::PropertyEditor();
    propertyEditorView->setAutomaticDocumentUpdate(_GetParam()->GetBool("AutoTransactionView", false));
    propertyEditorView->setAutomaticExpand(_GetParam()->GetBool("AutoExpandView", false));
    tabs->addTab(propertyEditorView, tr("View"));

    propertyEditorData = new Gui::PropertyEditor::PropertyEditor();
    propertyEditorData->setAutomaticDocumentUpdate(_GetParam()->GetBool("AutoTransactionData", true));
    propertyEditorData->setAutomaticExpand(_GetParam()->GetBool("AutoExpandData", false));
    tabs->addTab(propertyEditorData, tr("Data"));

    int preferredTab = _GetParam()->GetInt("LastTabIndex", 1);

    if ( preferredTab > 0 && preferredTab < tabs->count() )
        tabs->setCurrentIndex(preferredTab);

    // connect after adding all tabs, so adding doesn't thrash the parameter
    connect(tabs, &QTabWidget::currentChanged, this, &PropertyView::tabChanged);

    //NOLINTBEGIN
    this->connectPropData =
    App::GetApplication().signalChangedObject.connect(std::bind
        (&PropertyView::slotChangePropertyData, this, sp::_2));
    this->connectPropView =
    Gui::Application::Instance->signalChangedObject.connect(std::bind
        (&PropertyView::slotChangePropertyView, this, sp::_1, sp::_2));
    this->connectPropAppend =
    App::GetApplication().signalAppendDynamicProperty.connect(std::bind
        (&PropertyView::slotAppendDynamicProperty, this, sp::_1));
    this->connectPropRemove =
    App::GetApplication().signalRemoveDynamicProperty.connect(std::bind
        (&PropertyView::slotRemoveDynamicProperty, this, sp::_1));
    this->connectPropChange =
    App::GetApplication().signalChangePropertyEditor.connect(std::bind
        (&PropertyView::slotChangePropertyEditor, this, sp::_1, sp::_2));
    this->connectUndoDocument =
    App::GetApplication().signalUndoDocument.connect(std::bind
        (&PropertyView::slotRollback, this));
    this->connectRedoDocument =
    App::GetApplication().signalRedoDocument.connect(std::bind
        (&PropertyView::slotRollback, this));
    this->connectActiveDoc =
    Application::Instance->signalActiveDocument.connect(std::bind
        (&PropertyView::slotActiveDocument, this, sp::_1));
    this->connectDelDocument =
        Application::Instance->signalDeleteDocument.connect(
                std::bind(&PropertyView::slotDeleteDocument, this, sp::_1));
    this->connectDelViewObject =
        Application::Instance->signalDeletedObject.connect(
                std::bind(&PropertyView::slotDeletedViewObject, this, sp::_1));
    this->connectDelObject =
        App::GetApplication().signalDeletedObject.connect(
                std::bind(&PropertyView::slotDeletedObject, this, sp::_1));
    this->connectChangedDocument = App::GetApplication().signalChangedDocument.connect(
            std::bind(&PropertyView::slotChangePropertyData, this, sp::_2));
    //NOLINTEND
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
    this->connectDelDocument.disconnect();
    this->connectDelObject.disconnect();
    this->connectDelViewObject.disconnect();
    this->connectChangedDocument.disconnect();
}

static bool _ShowAll;

bool PropertyView::showAll() {
    return _ShowAll;
}

void PropertyView::setShowAll(bool enable) {
    if(_ShowAll != enable) {
        _ShowAll = enable;
        const auto views = getMainWindow()->findChildren<PropertyView*>();
        for(auto view : views) {
            if(view->isVisible()) {
                view->propertyEditorData->buildUp();
                view->propertyEditorView->buildUp();
                view->onTimer();
            }
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
    this->timer->start(ViewParams::instance()->getPropertyViewTimer());
    QWidget::showEvent(ev);
}

void PropertyView::clearPropertyItemSelection() {
    QModelIndex index;
    propertyEditorData->clearSelection();
    propertyEditorData->setCurrentIndex(index);
    propertyEditorView->clearSelection();
    propertyEditorView->setCurrentIndex(index);
}

void PropertyView::slotRollback() {
    // PropertyItemDelegate will setup application active transaction on
    // entering edit mode, and close active transaction when exit editing.  But,
    // when the user clicks undo/redo button while editing some property, the
    // current active transaction will be closed by design, which cause further
    // editing to be not recorded. Hence, we force unselect any property item on
    // undo/redo
    clearPropertyItemSelection();
}

void PropertyView::slotChangePropertyData(const App::Property& prop)
{
    if (propertyEditorData->propOwners.count(prop.getContainer())) {
        propertyEditorData->updateProperty(prop);
        timer->start(ViewParams::instance()->getPropertyViewTimer());
    }
}

void PropertyView::slotChangePropertyView(const Gui::ViewProvider&, const App::Property& prop)
{
    if (propertyEditorView->propOwners.count(prop.getContainer())) {
        propertyEditorView->updateProperty(prop);
        timer->start(ViewParams::instance()->getPropertyViewTimer());
    }
}

bool PropertyView::isPropertyHidden(const App::Property *prop) {
    return prop && !showAll() &&
        ((prop->getType() & App::Prop_Hidden) || prop->testStatus(App::Property::Hidden));
}

void PropertyView::slotAppendDynamicProperty(const App::Property& prop)
{
    if (isPropertyHidden(&prop))
        return;

    App::PropertyContainer* parent = prop.getContainer();
    if (propertyEditorData->propOwners.count(parent)
            || propertyEditorView->propOwners.count(parent))
    {
        timer->start(ViewParams::instance()->getPropertyViewTimer());
    }
}

void PropertyView::slotRemoveDynamicProperty(const App::Property& prop)
{
    App::PropertyContainer* parent = prop.getContainer();
    if(propertyEditorData->propOwners.count(parent))
        propertyEditorData->removeProperty(prop);
    else if(propertyEditorView->propOwners.count(parent))
        propertyEditorView->removeProperty(prop);
    else
        return;
    timer->start(ViewParams::instance()->getPropertyViewTimer());
}

void PropertyView::slotChangePropertyEditor(const App::Document &, const App::Property& prop)
{
    App::PropertyContainer* parent = prop.getContainer();
    if (propertyEditorData->propOwners.count(parent)
            || propertyEditorView->propOwners.count(parent))
        timer->start(ViewParams::instance()->getPropertyViewTimer());
}

void PropertyView::slotDeleteDocument(const Gui::Document &doc) {
    if(propertyEditorData->propOwners.count(doc.getDocument())) {
        propertyEditorView->buildUp();
        propertyEditorData->buildUp();
        clearPropertyItemSelection();
        timer->start(ViewParams::instance()->getPropertyViewTimer());
    }
}

void PropertyView::slotDeletedViewObject(const Gui::ViewProvider &vp) {
    if(propertyEditorView->propOwners.count(&vp)) {
        propertyEditorView->buildUp();
        propertyEditorData->buildUp();
        clearPropertyItemSelection();
        timer->start(ViewParams::instance()->getPropertyViewTimer());
    }
}

void PropertyView::slotDeletedObject(const App::DocumentObject &obj) {
    if(propertyEditorData->propOwners.count(&obj)) {
        propertyEditorView->buildUp();
        propertyEditorData->buildUp();
        clearPropertyItemSelection();
        timer->start(ViewParams::instance()->getPropertyViewTimer());
    }
}

void PropertyView::slotActiveDocument(const Gui::Document &doc)
{
    checkEnable(doc.getDocument()->getName());
}

void PropertyView::checkEnable(const char *doc) {
    if(ViewParams::instance()->getEnablePropertyViewForInactiveDocument()) {
        setEnabled(true);
        return;
    }
    // check if at least one selected object is part of the active document
    setEnabled(!Selection().hasSelection()
            || Selection().hasSelection(doc, ResolveMode::NoResolve));
}

struct PropertyView::PropInfo
{
    std::string propName;
    int propId;
    std::vector<App::Property*> propList;
};

struct PropertyView::PropFind {
    const PropInfo& item;
    explicit PropFind(const PropInfo& item) : item(item) {}
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
    timer->start(ViewParams::instance()->getPropertyViewTimer());
}

void PropertyView::onTimer()
{
    // See https://forum.freecad.org/viewtopic.php?f=8&t=72526
    if (this->updating) {
        Base::Console().Log("Ignore recursive call of PropertyView::onTimer()\n");
        return;
    }
    Base::StateLocker guard(this->updating);

    timer->stop();

    if(!this->isSelectionAttached()) {
        propertyEditorData->buildUp();
        propertyEditorView->buildUp();
        clearPropertyItemSelection();
        return;
    }

    if(!Gui::Selection().hasSelection()) {
        auto gdoc = TreeWidget::selectedDocument();
        if(!gdoc || !gdoc->getDocument()) {
            propertyEditorData->buildUp();
            propertyEditorView->buildUp();
            clearPropertyItemSelection();
            return;
        }

        PropertyModel::PropertyList docProps;

        auto doc = gdoc->getDocument();
        std::map<std::string,App::Property*> props;
        doc->getPropertyMap(props);
        for(auto &v : props)
            docProps.emplace_back(v.first,
                    std::vector<App::Property*>(1,v.second));
        propertyEditorData->buildUp(std::move(docProps));
        tabs->setCurrentIndex(1);
        return;
    }

    std::set<App::DocumentObject *> objSet;

    // group the properties by <name,id>
    std::vector<PropInfo> propDataMap;
    std::vector<PropInfo> propViewMap;
    bool checkLink = true;
    ViewProviderDocumentObject *vpLast = nullptr;
    auto sels = Gui::Selection().getSelectionEx("*");
    for(auto &sel : sels) {
        App::DocumentObject *ob = sel.getObject();
        if(!ob) continue;

        // Do not process an object more than once
        if(!objSet.insert(ob).second)
            continue;

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
        {
            for (auto prop : dataList) {
                if (isPropertyHidden(prop))
                    continue;

                PropInfo nameType;
                nameType.propName = prop->getName();
                nameType.propId = prop->getTypeId().getKey();

                auto pi = std::find_if(propDataMap.begin(), propDataMap.end(), PropFind(nameType));
                if (pi != propDataMap.end()) {
                    pi->propList.push_back(prop);
                }
                else {
                    nameType.propList.push_back(prop);
                    propDataMap.push_back(nameType);
                }
            }
        }
        // the same for the view properties
        {
            std::map<std::string, App::Property*>::iterator pt;
            for (pt = viewList.begin(); pt != viewList.end(); ++pt) {
                if (isPropertyHidden(pt->second))
                    continue;

                PropInfo nameType;
                nameType.propName = pt->first;
                nameType.propId = pt->second->getTypeId().getKey();

                auto pi = std::find_if(propViewMap.begin(), propViewMap.end(), PropFind(nameType));
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
    std::map<std::string, std::vector<App::Property*> > dataPropsMap;
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
                std::vector<App::Property*> items(1,prop);
                if(prop->testStatus(App::Property::PropDynamic))
                    dataPropsMap.emplace(name+"*",std::move(items));
                else
                    dataProps.emplace_back(name+"*", std::move(items));
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
                    std::vector<App::Property*> items(1,prop);
                    viewProps.emplace_back(name+"*", std::move(items));
                }
            }
        }
    }

    for(auto &v : dataPropsMap)
        dataProps.emplace_back(v.first,std::move(v.second));

    dataPropsMap.clear();

    for (it = propDataMap.begin(); it != propDataMap.end(); ++it) {
        if (it->propList.size() == sels.size()) {
            if(it->propList[0]->testStatus(App::Property::PropDynamic))
                dataPropsMap.emplace(it->propName, std::move(it->propList));
            else
                dataProps.emplace_back(it->propName, std::move(it->propList));
        }
    }

    for(auto &v : dataPropsMap)
        dataProps.emplace_back(v.first,std::move(v.second));

    propertyEditorData->buildUp(std::move(dataProps),true);

    for (it = propViewMap.begin(); it != propViewMap.end(); ++it) {
        if (it->propList.size() == sels.size())
            viewProps.emplace_back(it->propName, std::move(it->propList));
    }

    propertyEditorView->buildUp(std::move(viewProps));

    // make sure the editors are enabled/disabled properly
    checkEnable();
}

void PropertyView::tabChanged(int index)
{
    _GetParam()->SetInt("LastTabIndex",index);
}

void PropertyView::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        tabs->setTabText(0, tr("View"));
        tabs->setTabText(1, tr("Data"));
    }

    QWidget::changeEvent(e);
}

/* TRANSLATOR Gui::DockWnd::PropertyDockView */

PropertyDockView::PropertyDockView(Gui::Document* pcDocument, QWidget *parent)
  : DockWindow(pcDocument,parent)
{
    setWindowTitle(tr("Property View"));

    auto view = new PropertyView(this);
    auto pLayout = new QGridLayout(this);
    pLayout->setSpacing(0);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->addWidget(view, 0, 0);

    resize( 200, 400 );
}

PropertyDockView::~PropertyDockView() = default;

#include "moc_PropertyView.cpp"
