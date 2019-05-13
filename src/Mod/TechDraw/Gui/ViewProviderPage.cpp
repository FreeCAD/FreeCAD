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
#include <QList>
#include <QPointer>
#include <boost/signals2.hpp>
#include <boost/signals2/connection.hpp>
#include <boost/bind.hpp>

#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/ViewProviderDocumentObject.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawRichAnno.h>
#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "MDIViewPage.h"
#include "QGVPage.h"
#include "QGITemplate.h"
#include "ViewProviderTemplate.h"
#include "ViewProviderPage.h"


using namespace TechDrawGui;

#define _SHOWDRAWING 10
#define _TOGGLEUPDATE 11

PROPERTY_SOURCE(TechDrawGui::ViewProviderPage, Gui::ViewProviderDocumentObject)


//**************************************************************************
// Construction/Destruction

ViewProviderPage::ViewProviderPage()
  : m_mdiView(0),
    m_docReady(true),
    m_pageName("")
{
    sPixmap = "TechDraw_Tree_Page";
    static const char *group = "Frames";

    ADD_PROPERTY_TYPE(ShowFrames ,(true),group,App::Prop_None,"Show or hide View frames and Labels on this Page");

    ShowFrames.setStatus(App::Property::ReadOnly,true);
    Visibility.setStatus(App::Property::Hidden,true);
    DisplayMode.setStatus(App::Property::Hidden,true);
}

ViewProviderPage::~ViewProviderPage()
{
    removeMDIView();                    //if the MDIViewPage is still in MainWindow, remove it.
}

void ViewProviderPage::attach(App::DocumentObject *pcFeat)
{
    ViewProviderDocumentObject::attach(pcFeat);

    auto bnd = boost::bind(&ViewProviderPage::onGuiRepaint, this, _1);
    auto feature = getDrawPage();
    if (feature != nullptr) {
        connectGuiRepaint = feature->signalGuiPaint.connect(bnd);
        m_pageName = feature->getNameInDocument();
    } else {
        Base::Console().Log("VPP::attach has no Feature!\n");
    }

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
    Visibility.setValue(true);
    showMDIViewPage();
}

void ViewProviderPage::hide(void)
{
    Visibility.setValue(false);
    removeMDIView();
    ViewProviderDocumentObject::hide();
}

void ViewProviderPage::removeMDIView(void)
{
    if (!m_mdiView.isNull()) {                                //m_mdiView is a QPointer
        // https://forum.freecadweb.org/viewtopic.php?f=3&t=22797&p=182614#p182614
        //Gui::getMainWindow()->activatePreviousWindow();
        QList<QWidget*> wList= Gui::getMainWindow()->windows();
        bool found = wList.contains(m_mdiView);
        if (found) {
            Gui::getMainWindow()->removeWindow(m_mdiView);
            Gui::MDIView* aw = Gui::getMainWindow()->activeWindow();  //WF: this bit should be in the remove window logic, not here.
            if (aw != nullptr) {
                aw->showMaximized();
            }
        }
    }
}

void ViewProviderPage::updateData(const App::Property* prop)
{
    if (prop == &(getDrawPage()->KeepUpdated)) {
       if (getDrawPage()->KeepUpdated.getValue()) {
           sPixmap = "TechDraw_Tree_Page";
       } else {
           sPixmap = "TechDraw_Tree_Page_Unsync";
       }
    //if the template is changed, rebuild the visual
    } else if (prop == &(getDrawPage()->Template)) {
       if(m_mdiView && 
          !getDrawPage()->isUnsetting()) {
            m_mdiView->matchSceneRectToTemplate();
            m_mdiView->updateTemplate();
        }
    } else if (prop == &(getDrawPage()->Label)) {
       if(m_mdiView && 
          !getDrawPage()->isUnsetting()) {
           m_mdiView->setTabText(getDrawPage()->Label.getValue());
       }
    }
    Gui::ViewProviderDocumentObject::updateData(prop);
}

bool ViewProviderPage::onDelete(const std::vector<std::string> &items)
{
    bool rc = ViewProviderDocumentObject::onDelete(items);
    removeMDIView();
    return rc;
}

void ViewProviderPage::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Gui::ViewProviderDocumentObject::setupContextMenu(menu, receiver, member);
    QAction* act = menu->addAction(QObject::tr("Show drawing"), receiver, member);
    act->setData(QVariant((int) _SHOWDRAWING));
    QAction* act2 = menu->addAction(QObject::tr("Toggle KeepUpdated"), receiver, member);
    act2->setData(QVariant((int) _TOGGLEUPDATE));
}

bool ViewProviderPage::setEdit(int ModNum)
{
    bool rc = true;
    if (ModNum == _SHOWDRAWING) {
        Visibility.setValue(true);
        showMDIViewPage();   // show the drawing
        rc = false;  //finished editing
    } else if (ModNum == _TOGGLEUPDATE) {
         auto page = getDrawPage();
         if (page != nullptr) {
             page->KeepUpdated.setValue(!page->KeepUpdated.getValue());
             page->recomputeFeature();
         }
         rc = false;
    } else {
        rc = Gui::ViewProviderDocumentObject::setEdit(ModNum);
    }
    return rc;
}

bool ViewProviderPage::doubleClicked(void)
{
    Visibility.setValue(true);
    showMDIViewPage();
    Gui::getMainWindow()->setActiveWindow(m_mdiView);
    return true;
}

bool ViewProviderPage::showMDIViewPage()
{
   if (isRestoring()) {
       return true;
   }
   if (!Visibility.getValue())   {
       return true;
   }

    if (m_mdiView.isNull()){
        Gui::Document* doc = Gui::Application::Instance->getDocument
            (pcObject->getDocument());
        m_mdiView = new MDIViewPage(this, doc, Gui::getMainWindow());
//        QString tabTitle = QString::fromUtf8(getDrawPage()->getNameInDocument());
        QString tabTitle = QString::fromUtf8(getDrawPage()->Label.getValue());

        m_mdiView->setDocumentObject(getDrawPage()->getNameInDocument());
        m_mdiView->setDocumentName(pcObject->getDocument()->getName());

        m_mdiView->setWindowTitle(tabTitle + QString::fromLatin1("[*]"));
        m_mdiView->setWindowIcon(Gui::BitmapFactory().pixmap("TechDraw_Tree_Page"));
        m_mdiView->updateDrawing();
        Gui::getMainWindow()->addWindow(m_mdiView);
        m_mdiView->viewAll();  //this is empty function
        m_mdiView->showMaximized();
    } else {
        m_mdiView->updateDrawing();
        m_mdiView->redrawAllViews();
        m_mdiView->updateTemplate(true);
    }
    return true;
}

std::vector<App::DocumentObject*> ViewProviderPage::claimChildren(void) const
{
    std::vector<App::DocumentObject*> temp;

    App::DocumentObject *templateFeat = 0;
    templateFeat = getDrawPage()->Template.getValue();

    if(templateFeat) {
        temp.push_back(templateFeat);
    }

    // Collect any child views
    // for Page, valid children are any View except: DrawProjGroupItem
    //                                               DrawViewDimension
    //                                               DrawViewBalloon
    //                                               DrawLeader
    //                                               any FeatuerView in a DrawViewClip
    //                                               DrawHatch

    const std::vector<App::DocumentObject *> &views = getDrawPage()->Views.getValues();

    try {
      for(std::vector<App::DocumentObject *>::const_iterator it = views.begin(); it != views.end(); ++it) {
          TechDraw::DrawView* featView = dynamic_cast<TechDraw::DrawView*> (*it);
          App::DocumentObject *docObj = *it;
          //DrawRichAnno with no parent is child of Page
          TechDraw::DrawRichAnno* dra = dynamic_cast<TechDraw::DrawRichAnno*> (*it);
          if (dra != nullptr) {
              if (dra->AnnoParent.getValue() == nullptr) {
                  temp.push_back(*it);
                  continue;
              }
          }

          // Don't collect if dimension, projection group item, hatch or member of ClipGroup as these should be grouped elsewhere
          if(docObj->isDerivedFrom(TechDraw::DrawProjGroupItem::getClassTypeId())    ||
             docObj->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId())    ||
             docObj->isDerivedFrom(TechDraw::DrawHatch::getClassTypeId())            ||
             docObj->isDerivedFrom(TechDraw::DrawViewBalloon::getClassTypeId())      ||
             docObj->isDerivedFrom(TechDraw::DrawRichAnno::getClassTypeId())         ||
             docObj->isDerivedFrom(TechDraw::DrawLeaderLine::getClassTypeId())       ||
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


void ViewProviderPage::onChanged(const App::Property *prop)
{
//    if (prop == &(getDrawPage()->Template)) {
//       if(m_mdiView) {
//            m_mdiView->updateTemplate();
//        }
//    }

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
    //control drawing opening on restore based on Preference
    //mantis #2967 ph2 - don't even show blank page
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    bool autoUpdate = hGrp->GetBool("KeepPagesUpToDate", 1l);
    if (autoUpdate) {
        static_cast<void>(showMDIViewPage());
    }
    Gui::ViewProviderDocumentObject::finishRestoring();
}

bool ViewProviderPage::isShow(void) const
{
    return Visibility.getValue();
}

bool ViewProviderPage::getFrameState(void)
{
    bool result = ShowFrames.getValue();
    return result;
}

void ViewProviderPage::setFrameState(bool state)
{
    ShowFrames.setValue(state);
}

void ViewProviderPage::toggleFrameState(void)
{
//    Base::Console().Message("VPP::toggleFrameState()\n");
    if (m_graphicsView != nullptr) {
        setFrameState(!getFrameState());
        m_graphicsView->refreshViews();
        setTemplateMarkers(getFrameState());
    }
}

void ViewProviderPage::setTemplateMarkers(bool state)
{
//    Base::Console().Message("VPP::setTemplateMarkers(%d)\n",state);
    App::DocumentObject *templateFeat = nullptr;
    templateFeat = getDrawPage()->Template.getValue();
    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(templateFeat->getDocument());
    Gui::ViewProvider* vp = guiDoc->getViewProvider(templateFeat);
    ViewProviderTemplate* vpt = dynamic_cast<ViewProviderTemplate*>(vp);
    vpt->setMarkers(state);
    QGITemplate* t = vpt->getQTemplate();
    if (t != nullptr) {
        t->updateView(true);
    }
}

void ViewProviderPage::setGraphicsView(QGVPage* gv)
{
    m_graphicsView = gv;
}

//! Redo the whole visual page
void ViewProviderPage::onGuiRepaint(const TechDraw::DrawPage* dp) 
{
    if (dp == getDrawPage()) {
        if(!m_mdiView.isNull() &&
           !getDrawPage()->isUnsetting()) {
            m_mdiView->updateDrawing();
        }
    }
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
