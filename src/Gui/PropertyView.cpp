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
# include <QGridLayout>
# include <QHeaderView>
# include <QEvent>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......

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

PropertyView::PropertyView(QWidget *parent)
  : QWidget(parent)
{
    QGridLayout* pLayout = new QGridLayout( this ); 
    pLayout->setSpacing(0);
    pLayout->setMargin (0);

    tabs = new QTabWidget (this);
    tabs->setObjectName(QString::fromUtf8("propertyTab"));
    tabs->setTabPosition(QTabWidget::South);
    tabs->setTabShape(QTabWidget::Triangular);
    pLayout->addWidget(tabs, 0, 0);

    propertyEditorView = new Gui::PropertyEditor::PropertyEditor();
    propertyEditorView->setAutomaticDocumentUpdate(false);
    tabs->addTab(propertyEditorView, tr("View"));
    propertyEditorData = new Gui::PropertyEditor::PropertyEditor();
    propertyEditorData->setAutomaticDocumentUpdate(true);
    tabs->addTab(propertyEditorData, tr("Data"));
}

PropertyView::~PropertyView()
{
}

void PropertyView::onSelectionChanged(const SelectionChanges& msg)
{
    if (msg.Type != SelectionChanges::AddSelection &&
        msg.Type != SelectionChanges::RmvSelection &&
        msg.Type != SelectionChanges::SetSelection &&
        msg.Type != SelectionChanges::ClrSelection)
        return;
    // group the properties by <name,id>
    std::map<std::pair<std::string, int>, std::vector<App::Property*> > propDataMap;
    std::map<std::pair<std::string, int>, std::vector<App::Property*> > propViewMap;
    std::vector<SelectionSingleton::SelObj> array = Gui::Selection().getCompleteSelection();
    for (std::vector<SelectionSingleton::SelObj>::const_iterator it = array.begin(); it != array.end(); ++it) {
        App::DocumentObject *ob=0;
        ViewProvider *vp=0;

        std::map<std::string,App::Property*> dataMap;
        std::map<std::string,App::Property*> viewMap;
        if ((*it).pObject) {
            (*it).pObject->getPropertyMap(dataMap);
            ob = (*it).pObject;

            // get also the properties of the associated view provider
            Gui::Document* doc = Gui::Application::Instance->getDocument(it->pDoc);
            vp = doc->getViewProvider((*it).pObject);
            if(!vp) continue;
            vp->getPropertyMap(viewMap);
        }

        // store the properties with <name,id> as key in a map
        std::map<std::string,App::Property*>::iterator pt;
        if (ob) {
            for (pt = dataMap.begin(); pt != dataMap.end(); ++pt) {
                std::pair<std::string, int> nameType = std::make_pair
                    <std::string, int>(pt->first, pt->second->getTypeId().getKey());
                if (!ob->isHidden(pt->second))
                    propDataMap[nameType].push_back(pt->second);
            }
        }
        // the same for the view properties
        if (vp) {
            for(pt = viewMap.begin(); pt != viewMap.end(); ++pt) {
                std::pair<std::string, int> nameType = std::make_pair
                    <std::string, int>( pt->first, pt->second->getTypeId().getKey());
                if (!vp->isHidden(pt->second))
                    propViewMap[nameType].push_back(pt->second);
            }
        }
    }

    // the property must be part of each selected object, i.e. the number
    // of selected objects is equal to the number of properties with same
    // name and id
    std::map<std::pair<std::string, int>, std::vector<App::Property*> >
        ::const_iterator it;
    std::map<std::string, std::vector<App::Property*> > dataProps;
    for (it = propDataMap.begin(); it != propDataMap.end(); ++it) {
        if (it->second.size() == array.size()) {
            dataProps[it->first.first] = it->second;
        }
    }
    propertyEditorData->buildUp(dataProps);

    std::map<std::string, std::vector<App::Property*> > viewProps;
    for (it = propViewMap.begin(); it != propViewMap.end(); ++it) {
        if (it->second.size() == array.size()) {
            viewProps[it->first.first] = it->second;
        }
    }
    propertyEditorView->buildUp(viewProps);
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
