/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is Drawing of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A DrawingICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include <App/DocumentObject.h>
#include <Mod/Drawing/App/FeatureClip.h>
#include <Mod/Drawing/App/FeatureView.h>

#include "ViewProviderView.h"


using namespace DrawingGui;

PROPERTY_SOURCE(DrawingGui::ViewProviderDrawingView, Gui::ViewProviderDocumentObject)

ViewProviderDrawingView::ViewProviderDrawingView()
{
    sPixmap = "Page";

    // Do not show in property editor
    DisplayMode.setStatus(App::Property::Hidden, true);
}

ViewProviderDrawingView::~ViewProviderDrawingView()
{}

void ViewProviderDrawingView::attach(App::DocumentObject* pcFeat)
{
    // call parent attach method
    ViewProviderDocumentObject::attach(pcFeat);
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

void ViewProviderDrawingView::show(void)
{
    ViewProviderDocumentObject::show();

    App::DocumentObject* obj = getObject();
    if (!obj || obj->isRestoring()) {
        return;
    }
    if (obj->getTypeId().isDerivedFrom(Drawing::FeatureView::getClassTypeId())) {
        // The 'Visible' property is marked as 'Output'. To update the drawing on recompute
        // the parent page object is touched.
        static_cast<Drawing::FeatureView*>(obj)->Visible.setValue(true);
        std::vector<App::DocumentObject*> inp = obj->getInList();
        for (std::vector<App::DocumentObject*>::iterator it = inp.begin(); it != inp.end(); ++it) {
            (*it)->touch();
        }
    }
}

void ViewProviderDrawingView::hide(void)
{
    ViewProviderDocumentObject::hide();

    App::DocumentObject* obj = getObject();
    if (!obj || obj->isRestoring()) {
        return;
    }
    if (obj->getTypeId().isDerivedFrom(Drawing::FeatureView::getClassTypeId())) {
        // The 'Visible' property is marked as 'Output'. To update the drawing on recompute
        // the parent page object is touched.
        static_cast<Drawing::FeatureView*>(obj)->Visible.setValue(false);
        std::vector<App::DocumentObject*> inp = obj->getInList();
        for (std::vector<App::DocumentObject*>::iterator it = inp.begin(); it != inp.end(); ++it) {
            (*it)->touch();
        }
    }
}

bool ViewProviderDrawingView::isShow(void) const
{
    return Visibility.getValue();
}

void ViewProviderDrawingView::startRestoring()
{
    // do nothing
}

void ViewProviderDrawingView::finishRestoring()
{
    // do nothing
}

void ViewProviderDrawingView::updateData(const App::Property*)
{}

// Python viewprovider -----------------------------------------------------------------------

namespace Gui
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(DrawingGui::ViewProviderDrawingViewPython,
                         DrawingGui::ViewProviderDrawingView)
/// @endcond

// explicit template instantiation
template class DrawingGuiExport ViewProviderPythonFeatureT<DrawingGui::ViewProviderDrawingView>;
}  // namespace Gui


// ----------------------------------------------------------------------------

PROPERTY_SOURCE(DrawingGui::ViewProviderDrawingClip, Gui::ViewProviderDocumentObjectGroup)

ViewProviderDrawingClip::ViewProviderDrawingClip()
{
    sPixmap = "Page";

    // Do not show in property editor
    DisplayMode.setStatus(App::Property::Hidden, true);
}

ViewProviderDrawingClip::~ViewProviderDrawingClip()
{}

void ViewProviderDrawingClip::attach(App::DocumentObject* pcFeat)
{
    // call parent attach method
    ViewProviderDocumentObject::attach(pcFeat);
}

void ViewProviderDrawingClip::setDisplayMode(const char* ModeName)
{
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderDrawingClip::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList;
    return StrList;
}

void ViewProviderDrawingClip::show(void)
{
    ViewProviderDocumentObjectGroup::show();

    App::DocumentObject* obj = getObject();
    if (!obj || obj->isRestoring()) {
        return;
    }
    if (obj->getTypeId().isDerivedFrom(Drawing::FeatureClip::getClassTypeId())) {
        // The 'Visible' property is marked as 'Output'. To update the drawing on recompute
        // the parent page object is touched.
        static_cast<Drawing::FeatureClip*>(obj)->Visible.setValue(true);
        std::vector<App::DocumentObject*> inp = obj->getInList();
        for (std::vector<App::DocumentObject*>::iterator it = inp.begin(); it != inp.end(); ++it) {
            (*it)->touch();
        }
    }
}

void ViewProviderDrawingClip::hide(void)
{
    ViewProviderDocumentObjectGroup::hide();

    App::DocumentObject* obj = getObject();
    if (!obj || obj->isRestoring()) {
        return;
    }
    if (obj->getTypeId().isDerivedFrom(Drawing::FeatureClip::getClassTypeId())) {
        // The 'Visible' property is marked as 'Output'. To update the drawing on recompute
        // the parent page object is touched.
        static_cast<Drawing::FeatureClip*>(obj)->Visible.setValue(false);
        std::vector<App::DocumentObject*> inp = obj->getInList();
        for (std::vector<App::DocumentObject*>::iterator it = inp.begin(); it != inp.end(); ++it) {
            (*it)->touch();
        }
    }
}

bool ViewProviderDrawingClip::isShow(void) const
{
    return Visibility.getValue();
}

void ViewProviderDrawingClip::startRestoring()
{
    // do nothing
}

void ViewProviderDrawingClip::finishRestoring()
{
    // do nothing
}

void ViewProviderDrawingClip::updateData(const App::Property*)
{}
