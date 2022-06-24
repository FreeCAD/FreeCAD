/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2012 Luke Parry <l.parry@warwick.ac.uk>                 *
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
#include <boost_signals2.hpp>
#include <boost/signals2/connection.hpp>
#endif

#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawView.h>

#include "ViewProviderDrawingView.h"
#include "MDIViewPage.h"
#include "QGIView.h"
#include "QGSPage.h"
#include "QGVPage.h"
#include "ViewProviderPage.h"

using namespace TechDrawGui;
namespace bp = boost::placeholders;

PROPERTY_SOURCE(TechDrawGui::ViewProviderDrawingView, Gui::ViewProviderDocumentObject)

ViewProviderDrawingView::ViewProviderDrawingView()
{
//    Base::Console().Message("VPDV::VPDV\n");
    sPixmap = "TechDraw_TreeView";
    static const char *group = "Base";

    ADD_PROPERTY_TYPE(KeepLabel ,(false),group,App::Prop_None,"Keep Label on Page even if toggled off");

    // Do not show in property editor   why? wf  WF: because DisplayMode applies only to coin and we
    // don't use coin.
    DisplayMode.setStatus(App::Property::Hidden,true);
    m_docReady = true;
}

ViewProviderDrawingView::~ViewProviderDrawingView()
{
}

void ViewProviderDrawingView::attach(App::DocumentObject *pcFeat)
{
//    Base::Console().Message("VPDV::attach(%s)\n", pcFeat->getNameInDocument());
    ViewProviderDocumentObject::attach(pcFeat);

    auto bnd = boost::bind(&ViewProviderDrawingView::onGuiRepaint, this, bp::_1);
    auto feature = getViewObject();
    if (feature != nullptr) {
        connectGuiRepaint = feature->signalGuiPaint.connect(bnd);
        //TODO: would be good to start the QGIV creation process here, but no guarantee we actually have
        //      MDIVP or QGVP yet.
        // but parent page might.  we may not be part of the document yet though!
        // :( we're not part of the page yet either!
    } else {
        Base::Console().Warning("VPDV::attach has no Feature!\n");
    }
}

void ViewProviderDrawingView::setDisplayMode(const char* ModeName)
{
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderDrawingView::getDisplayModes(void) const
{
    std::vector<std::string> StrList = ViewProviderDocumentObject::getDisplayModes();
    return StrList;
}

void ViewProviderDrawingView::onChanged(const App::Property *prop)
{
    App::DocumentObject* obj = getObject();
    if (!obj || obj->isRestoring()) {
            Gui::ViewProviderDocumentObject::onChanged(prop);
            return;
    }

    if (prop == &Visibility) {
//       if(Visibility.getValue()) {
//            show();
//        } else {
//            hide();
//        }
    } else if (prop == &KeepLabel) {
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
    }

    Gui::ViewProviderDocumentObject::onChanged(prop);
}

void ViewProviderDrawingView::show(void)
{
    TechDraw::DrawView* obj = getViewObject();
    if (!obj || obj->isRestoring())
        return;

    if (obj->getTypeId().isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
        QGIView* qView = getQView();
        if (qView) {
            qView->draw();
            qView->show();
        }
    }
    ViewProviderDocumentObject::show();
}

void ViewProviderDrawingView::hide(void)
{
    TechDraw::DrawView* obj = getViewObject();
    if (!obj || obj->isRestoring())
        return;

    if (obj->getTypeId().isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
        QGIView* qView = getQView();
        if (qView) {
            //note: hiding an item in the scene clears its selection status
            //      this confuses Gui::Selection.
            //      So we block selection changes while we are hiding the qgiv
            //      in FC Tree hiding does not change selection state.
            //      block/unblock selection protects against crash in Gui::SelectionSingleton::setVisible
            MDIViewPage* mdi = getMDIViewPage();
            if (mdi != nullptr) {                  //if there is no mdivp, there is nothing to hide!
                mdi->blockSceneSelection(true);
                qView->hide();
                ViewProviderDocumentObject::hide();
                mdi->blockSceneSelection(false);
            }
        }
    }
}

QGIView* ViewProviderDrawingView::getQView(void)
{
    QGIView *qView = nullptr;
    if (m_docReady){
        TechDraw::DrawView* dv = getViewObject();
        if (dv) {
            Gui::Document* guiDoc = Gui::Application::Instance->getDocument(getViewObject()->getDocument());
            if (guiDoc != nullptr) {
                Gui::ViewProvider* vp = guiDoc->getViewProvider(getViewObject()->findParentPage());
                ViewProviderPage* dvp = dynamic_cast<ViewProviderPage*>(vp);
                if (dvp) {
                    if (dvp->getMDIViewPage()) {
                        if (dvp->getMDIViewPage()->getQGSPage()) {
                            qView = dynamic_cast<QGIView *>(dvp->getMDIViewPage()->
                                                   getQGSPage()->findQViewForDocObj(getViewObject()));
                        }
                    }
                }
            }
        }
    }
    return qView;
}

bool ViewProviderDrawingView::isShow(void) const
{
    return Visibility.getValue();
}

void ViewProviderDrawingView::startRestoring()
{
    m_docReady = false;
    Gui::ViewProviderDocumentObject::startRestoring();
}

void ViewProviderDrawingView::finishRestoring()
{
    m_docReady = true;
    if (Visibility.getValue()) {
        show();
    } else {
        hide();
    }
    Gui::ViewProviderDocumentObject::finishRestoring();
}

void ViewProviderDrawingView::updateData(const App::Property* prop)
{
    //only move the view on X,Y change
    if (prop == &(getViewObject()->X)  ||
        prop == &(getViewObject()->Y) ){
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->QGIView::updateView(true);
        }
    }

    Gui::ViewProviderDocumentObject::updateData(prop);
}

void ViewProviderDrawingView::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
    }
    else {
        Gui::ViewProviderDocumentObject::unsetEdit(ModNum);
    }
}

MDIViewPage* ViewProviderDrawingView::getMDIViewPage() const
{
    MDIViewPage* result = nullptr;
    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(getViewObject()->getDocument());
    if (guiDoc != nullptr) {
        Gui::ViewProvider* vp = guiDoc->getViewProvider(getViewObject()->findParentPage());
        ViewProviderPage* dvp = dynamic_cast<ViewProviderPage*>(vp);
        if (dvp) {
            result = dvp->getMDIViewPage();
        }
    }
    return result;
}

Gui::MDIView *ViewProviderDrawingView::getMDIView() const
{
    return getMDIViewPage();
}

void ViewProviderDrawingView::onGuiRepaint(const TechDraw::DrawView* dv) 
{
//    Base::Console().Message("VPDV::onGuiRepaint(%s) - this: %x\n", dv->getNameInDocument(), this);
    std::vector<TechDraw::DrawPage*> pages = getViewObject()->findAllParentPages();
    if (pages.size() > 1) {
        Gui::Document* guiDoc = Gui::Application::Instance->getDocument(getViewObject()->getDocument());
        if (guiDoc == nullptr) {
            return;
        }
        for (auto& p : pages) {
            std::vector<App::DocumentObject*> views = p->Views.getValues();
            for (auto& v: views) {
                if (v == getViewObject()) {
                    //view v belongs to this page p
                    Gui::ViewProvider* vp = guiDoc->getViewProvider(p);
                    ViewProviderPage* vpPage = dynamic_cast<ViewProviderPage*>(vp);
                    if (vpPage != nullptr) {
                        if (vpPage->getMDIViewPage() != nullptr) {
                            if (vpPage->getMDIViewPage()->getQGSPage()) {
                                QGIView* qView = dynamic_cast<QGIView *>(vpPage->getMDIViewPage()->
                                                           getQGSPage()->findQViewForDocObj(v));
                                if (qView != nullptr) {
                                    qView->updateView(true);
                                }
                            }
                        }
                    }
                }
            }
        }
    } else if (dv == getViewObject()) {
        //original logic for 1 view on 1 page
        if (!dv->isRemoving() &&
            !dv->isRestoring()) {
            QGIView* qgiv = getQView();
            if (qgiv) {
                qgiv->updateView(true);
            } else {                                //we are not part of the Gui page yet. ask page to add us.
                //TODO: this bit causes trouble.  Should move QGIV creation to attach?
                //      is MDIVP/QGVP available at attach time?
                // wf: mdivp/qgvp is not necessarily directly available at attach time.  It should be available
                //     via the parent DrawPage since the DP is created before any views.
//                Base::Console().Message("VPDV::onGuiRepaint - no QGIV for: %s\n",dv->getNameInDocument());
                MDIViewPage* page = getMDIViewPage();
                if (page != nullptr) {
                    page->addView(dv);
                }
            }
        }
    }
}

TechDraw::DrawView* ViewProviderDrawingView::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawView*>(pcObject);
}
