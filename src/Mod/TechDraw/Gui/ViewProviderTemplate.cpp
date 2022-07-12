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
# include <QMessageBox>
# include <QTextStream>
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
#endif

#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawTemplate.h>
#include <Mod/TechDraw/App/DrawSVGTemplate.h>

#include "QGITemplate.h"
#include "QGISVGTemplate.h"
#include "QGVPage.h"
#include "QGSPage.h"
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
    sPixmap = "TechDraw_TreePageTemplate";

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
                mdi->getViewProviderPage()->setGrid();
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
        if (Visibility.getValue()) {
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
    TechDraw::DrawTemplate* dt = getTemplate();
    if (dt) {
        MDIViewPage* mdi = getMDIViewPage();
        if (mdi != nullptr) {
            return mdi->getQGSPage()->getTemplate();
        }
    }
    return nullptr;
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

bool ViewProviderTemplate::onDelete(const std::vector<std::string> &)
{
    // deleting the template will break the page view, thus warn the user

    // get the page
    auto page = getTemplate()->getParentPage();

    // If no parent page is given then just go ahead
    if (!page)
        return true;

    // generate dialog
    QString bodyMessage;
    QTextStream bodyMessageStream(&bodyMessage);
    bodyMessageStream << qApp->translate("Std_Delete",
        "The following referencing object might break:");
    bodyMessageStream << "\n\n" << QString::fromUtf8(page->Label.getValue());
    bodyMessageStream << "\n\n" << QObject::tr("Are you sure you want to continue?");

    // show and evaluate dialog
    int DialogResult = QMessageBox::warning(Gui::getMainWindow(),
        qApp->translate("Std_Delete", "Object dependencies"), bodyMessage,
        QMessageBox::Yes, QMessageBox::No);
    if (DialogResult == QMessageBox::Yes)
        return true;
    else
        return false;
}

MDIViewPage* ViewProviderTemplate::getMDIViewPage(void) const
{
    auto t = getTemplate();
    auto page = t->getParentPage();
    Gui::ViewProvider* vp = Gui::Application::Instance->getDocument(t->getDocument())->getViewProvider(page);
    TechDrawGui::ViewProviderPage* dvp = dynamic_cast<TechDrawGui::ViewProviderPage*>(vp);
    if (dvp) {
        return dvp->getMDIViewPage();
    }
    return nullptr;
}

Gui::MDIView *ViewProviderTemplate::getMDIView() const
{
    return getMDIViewPage();
}

TechDraw::DrawTemplate* ViewProviderTemplate::getTemplate() const
{
    return dynamic_cast<TechDraw::DrawTemplate*>(pcObject);
}
