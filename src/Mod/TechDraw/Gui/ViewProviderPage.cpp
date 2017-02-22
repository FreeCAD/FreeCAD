/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2012 Luke Parry    <l.parry@warwick.ac.uk>              *
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
# include <QAction>
# include <QMenu>
# include <QTimer>
#include <QPointer>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/ViewProviderDocumentObjectGroup.h>


#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawUtil.h>

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderPage, Gui::ViewProviderDocumentObject)


//**************************************************************************
// Construction/Destruction

ViewProviderPage::ViewProviderPage()
  : m_mdiView(0),
    m_docReady(true)
{
    sPixmap = "TechDraw_Tree_Page";

//    ADD_PROPERTY(HintScale,(10.0));
//    ADD_PROPERTY(HintOffsetX,(10.0));
//    ADD_PROPERTY(HintOffsetY,(10.0));

    // do not show this in the property editor
    //Visibility.StatusBits.set(3, true);
    //DisplayMode.StatusBits.set(3, true);
    Visibility.setStatus(App::Property::Hidden,true);
    DisplayMode.setStatus(App::Property::Hidden,true);
}

ViewProviderPage::~ViewProviderPage()
{
}

void ViewProviderPage::attach(App::DocumentObject *pcFeat)
{
    ViewProviderDocumentObject::attach(pcFeat);
}

void ViewProviderPage::setDisplayMode(const char* ModeName)
{
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderPage::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDocumentObject::getDisplayModes();
    StrList.push_back("Drawing");
    return StrList;
}

void ViewProviderPage::show(void)
{
    showMDIViewPage();
}

void ViewProviderPage::hide(void)
{
    // hiding the drawing page should not affect its children but closes the MDI m_mdiView
    // therefore do not call the method of its direct base class
    ViewProviderDocumentObject::hide();
    if (m_mdiView) {
        m_mdiView->parentWidget()->deleteLater();
    }
}

void ViewProviderPage::updateData(const App::Property* prop)
{
    if (prop == &(getDrawPage()->Views)) {
        if(m_mdiView &&
           !getDrawPage()->isDeleting()) {
            m_mdiView->updateDrawing();
        }
    } else if (prop == &(getDrawPage()->Template)) {
       if(m_mdiView && 
          !getDrawPage()->isDeleting()) {
            m_mdiView->updateTemplate();
        }
    }

    Gui::ViewProviderDocumentObject::updateData(prop);
}

bool ViewProviderPage::onDelete(const std::vector<std::string> &items)
{
    if (!m_mdiView.isNull()) {
        Gui::getMainWindow()->removeWindow(m_mdiView);
        Gui::getMainWindow()->activatePreviousWindow();
        m_mdiView->deleteLater(); // Delete the drawing m_mdiView;
    } else {
        // MDIViewPage is not displayed yet so don't try to delete it!
        Base::Console().Log("INFO - ViewProviderPage::onDelete - Page object deleted when viewer not displayed\n");
    }
    Gui::Selection().clearSelection();
    return ViewProviderDocumentObject::onDelete(items);
}

void ViewProviderPage::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Gui::ViewProviderDocumentObject::setupContextMenu(menu, receiver, member);
    QAction* act = menu->addAction(QObject::tr("Show drawing"), receiver, member);
//    act->setData(QVariant(1));  // Removed to resolve compile after cb16fec6bb67cec15be3fc2aeb251ab524134073   //this is edit ModNum
    act->setData(QVariant((int) ViewProvider::Default));
}

bool ViewProviderPage::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        showMDIViewPage();   // show the drawing
        Gui::getMainWindow()->setActiveWindow(m_mdiView);
        return false;
    } else {
        Gui::ViewProviderDocumentObject::setEdit(ModNum);
    }
    return true;
}

bool ViewProviderPage::doubleClicked(void)
{
    showMDIViewPage();
    Gui::getMainWindow()->setActiveWindow(m_mdiView);
    return true;
}

bool ViewProviderPage::showMDIViewPage()
{
    if (isRestoring()) {
        return true;
    }

    if (m_mdiView.isNull()){
        Gui::Document* doc = Gui::Application::Instance->getDocument
            (pcObject->getDocument());
        m_mdiView = new MDIViewPage(this, doc, Gui::getMainWindow());
        m_mdiView->setWindowTitle(QObject::tr("Drawing viewer") + QString::fromLatin1("[*]"));
        m_mdiView->setWindowIcon(Gui::BitmapFactory().pixmap("TechDraw_Tree_Page"));
        m_mdiView->updateDrawing(true);
     //   m_mdiView->updateTemplate(true);   //TODO: I don't think this is necessary?  Ends up triggering a reload of SVG template, but the MDIViewPage constructor does too.
        Gui::getMainWindow()->addWindow(m_mdiView);
        m_mdiView->viewAll();
    } else {
        m_mdiView->updateDrawing(true);
        m_mdiView->updateTemplate(true);
    }
    return true;
}

std::vector<App::DocumentObject*> ViewProviderPage::claimChildren(void) const
{
    std::vector<App::DocumentObject*> temp;

    // Attach the template if it exists
    App::DocumentObject *templateFeat = 0;
    templateFeat = getDrawPage()->Template.getValue();

    if(templateFeat) {
        temp.push_back(templateFeat);
    }

    // Collect any child views
    // for Page, valid children are any View except: DrawProjGroupItem
    //                                               DrawViewDimension
    //                                               any FeatuerView in a DrawViewClip
    //                                               DrawHatch

    const std::vector<App::DocumentObject *> &views = getDrawPage()->Views.getValues();

    try {
      for(std::vector<App::DocumentObject *>::const_iterator it = views.begin(); it != views.end(); ++it) {
          TechDraw::DrawView* featView = dynamic_cast<TechDraw::DrawView*> (*it);
          App::DocumentObject *docObj = *it;
          // Don't collect if dimension, projection group item, hatch or member of ClipGroup as these should be grouped elsewhere
          if(docObj->isDerivedFrom(TechDraw::DrawProjGroupItem::getClassTypeId())    ||
             docObj->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId())    ||
             docObj->isDerivedFrom(TechDraw::DrawHatch::getClassTypeId())            ||
             (featView && featView->isInClip()) )
              continue;
          else
              temp.push_back(*it);
      }
      return temp;
    } catch (...) {
        std::vector<App::DocumentObject*> tmp;
        return tmp;
    }
}

void ViewProviderPage::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    static_cast<void>(showMDIViewPage());
    return;
}


MDIViewPage* ViewProviderPage::getMDIViewPage()
{
    if (m_mdiView.isNull()) {
        Base::Console().Log("INFO - ViewProviderPage::getMDIViewPage has no m_mdiView!\n");
        return 0;
    } else {
        return m_mdiView;
    }
}

void ViewProviderPage::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if(!m_mdiView.isNull()) {
        if(msg.Type == Gui::SelectionChanges::SetSelection) {
            m_mdiView->clearSelection();
            std::vector<Gui::SelectionSingleton::SelObj> objs = Gui::Selection().getSelection(msg.pDocName);

            for (std::vector<Gui::SelectionSingleton::SelObj>::iterator it = objs.begin(); it != objs.end(); ++it) {
                Gui::SelectionSingleton::SelObj selObj = *it;
                if(selObj.pObject == getDrawPage())
                    continue;

                std::string str = msg.pSubName;
                // If it's a subfeature, don't select feature
                if (!str.empty()) {
                    if (TechDraw::DrawUtil::getGeomTypeFromName(str) == "Face" ||
                        TechDraw::DrawUtil::getGeomTypeFromName(str) == "Edge" ||
                        TechDraw::DrawUtil::getGeomTypeFromName(str) == "Vertex") {
                        // TODO implement me   wf: don't think this is ever executed
                    }
                } else {
                        m_mdiView->selectFeature(selObj.pObject, true);
                }
            }
        } else {
            bool selectState = (msg.Type == Gui::SelectionChanges::AddSelection) ? true : false;
            Gui::Document* doc = Gui::Application::Instance->getDocument(pcObject->getDocument());
            App::DocumentObject *obj = doc->getDocument()->getObject(msg.pObjectName);
            if(obj) {
                std::string str = msg.pSubName;
                // If it's a subfeature, don't select feature
                if (!str.empty()) {
                    if (TechDraw::DrawUtil::getGeomTypeFromName(str) == "Face" ||
                        TechDraw::DrawUtil::getGeomTypeFromName(str) == "Edge" ||
                        TechDraw::DrawUtil::getGeomTypeFromName(str) == "Vertex") {
                        // TODO implement me
                    } else {
                        m_mdiView->selectFeature(obj, selectState);
                    }
                }
            }
        }  //else (Gui::SelectionChanges::SetPreselect)
    }
}

void ViewProviderPage::onChanged(const App::Property *prop)
{
    if (prop == &(getDrawPage()->Views)) {
        if(m_mdiView) {
            m_mdiView->updateDrawing();
        }
    } else if (prop == &(getDrawPage()->Template)) {
       if(m_mdiView) {
            m_mdiView->updateTemplate();
        }
    }

    Gui::ViewProviderDocumentObject::onChanged(prop);
}

void ViewProviderPage::startRestoring()
{
    m_docReady = false;
    Gui::ViewProviderDocumentObject::startRestoring();
}

void ViewProviderPage::finishRestoring()
{
    m_docReady = true;
    static_cast<void>(showMDIViewPage());
    Gui::ViewProviderDocumentObject::finishRestoring();
}


TechDraw::DrawPage* ViewProviderPage::getDrawPage() const
{
    //during redo, pcObject can become invalid, but non-zero??
    if (!pcObject) {
        Base::Console().Message("TROUBLE - VPPage::getDrawPage - no Page Object!\n");
        return nullptr;
    }
    return dynamic_cast<TechDraw::DrawPage*>(pcObject);
}
