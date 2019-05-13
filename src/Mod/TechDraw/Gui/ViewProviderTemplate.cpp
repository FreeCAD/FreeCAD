/***************************************************************************
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
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
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
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
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/ViewProvider.h>

#include <Mod/TechDraw/App/DrawTemplate.h>
#include <Mod/TechDraw/App/DrawSVGTemplate.h>
#include <Mod/TechDraw/App/DrawPage.h>

#include "QGITemplate.h"
#include "QGISVGTemplate.h"
#include "QGVPage.h"
#include "MDIViewPage.h"
#include "TemplateTextField.h"
#include "ViewProviderPage.h"
#include "ViewProviderTemplate.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderTemplate, Gui::ViewProviderDocumentObject)

//**************************************************************************
// Construction/Destruction

ViewProviderTemplate::ViewProviderTemplate()
{
    sPixmap = "TechDraw_Tree_PageTemplate";

    DisplayMode.setStatus(App::Property::Hidden,true);
}

ViewProviderTemplate::~ViewProviderTemplate()
{
}

void ViewProviderTemplate::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDocumentObject::attach(pcFeat);
}

void ViewProviderTemplate::setDisplayMode(const char* ModeName)
{
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderTemplate::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDocumentObject::getDisplayModes();

    return StrList;
}

void ViewProviderTemplate::updateData(const App::Property* prop)
{
    if (getTemplate()->isDerivedFrom(TechDraw::DrawSVGTemplate::getClassTypeId())) {
        auto t = static_cast<TechDraw::DrawSVGTemplate*>(getTemplate());
        if (prop == &(t->Template)) {
            MDIViewPage* mdi = getMDIViewPage();
            if (mdi != nullptr) {
                mdi->attachTemplate(t);
                mdi->viewAll();
            }
       }
    }
    Gui::ViewProviderDocumentObject::updateData(prop);
}

void ViewProviderTemplate::onChanged(const App::Property *prop)
{
    App::DocumentObject* obj = getObject();
    if (!obj || obj->isRestoring()) {
        Gui::ViewProviderDocumentObject::onChanged(prop);
        return;
    }

    if (prop == &Visibility) {
        if(Visibility.getValue()) {
            show();
        } else {
            hide();
        }
    }
    
    Gui::ViewProviderDocumentObject::onChanged(prop);
}

void ViewProviderTemplate::show(void)
{
    QGITemplate* qTemplate = getQTemplate();
    if (qTemplate != nullptr) {
        qTemplate->show();
    }

    ViewProviderDocumentObject::show();
}

void ViewProviderTemplate::hide(void)
{
    QGITemplate* qTemplate = getQTemplate();
    if (qTemplate != nullptr) {
        qTemplate->hide();
    }
    
    ViewProviderDocumentObject::hide();
}

bool ViewProviderTemplate::isShow(void) const
{
    return Visibility.getValue();
}

QGITemplate* ViewProviderTemplate::getQTemplate(void)
{
    QGITemplate *result = nullptr;
    TechDraw::DrawTemplate* dt = getTemplate();
    if (dt) {
        MDIViewPage* mdi = getMDIViewPage();
        if (mdi != nullptr) {
            result = mdi->getQGVPage()->getTemplate();
        }
    }
    return result;
}

void ViewProviderTemplate::setMarkers(bool state)
{
//    Base::Console().Message("VPT::setMarkers(%d)\n",state);
    QGITemplate* qTemplate = getQTemplate();
    QGISVGTemplate* qSvgTemplate = dynamic_cast<QGISVGTemplate*> (qTemplate);
    if (qSvgTemplate != nullptr) {
        std::vector<TemplateTextField *> textFields = qSvgTemplate->getTextFields();
        for (auto& t:textFields) {
            if (state) {
                t->show();
            } else {
                t->hide();
            }
        }
        qSvgTemplate->updateView(true);
    }
}

MDIViewPage* ViewProviderTemplate::getMDIViewPage(void)
{
    MDIViewPage* myMdi = nullptr;
    auto t = getTemplate();
    auto page = t->getParentPage();
    Gui::ViewProvider* vp = Gui::Application::Instance->getDocument(t->getDocument())->getViewProvider(page);
    TechDrawGui::ViewProviderPage* dvp = dynamic_cast<TechDrawGui::ViewProviderPage*>(vp);
    if (dvp) {
        myMdi = dvp->getMDIViewPage();
    }
    return myMdi;
}

TechDraw::DrawTemplate* ViewProviderTemplate::getTemplate() const
{
    return dynamic_cast<TechDraw::DrawTemplate*>(pcObject);
}
