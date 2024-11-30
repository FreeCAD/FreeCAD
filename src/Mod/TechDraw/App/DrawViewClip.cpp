/***************************************************************************
 *   Copyright (c) 2012 Yorik van Havre <yorik@uncreated.net>              *
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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

#include <App/Link.h>

#include "DrawViewClip.h"
#include "DrawPage.h"
#include <Mod/TechDraw/App/DrawViewClipPy.h>  // generated from DrawViewClipPy.xml


using namespace TechDraw;

//===========================================================================
// DrawViewClip
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewClip, TechDraw::DrawView)

DrawViewClip::DrawViewClip()
{
    static const char *group = "Clip Group";
    //App::PropertyType hidden = (App::PropertyType)(App::Prop_Hidden);

    ADD_PROPERTY_TYPE(Height     ,(100), group, App::Prop_None, "The height of the view area of this clip");
    ADD_PROPERTY_TYPE(Width      ,(100), group, App::Prop_None, "The width of the view area of this clip");
    ADD_PROPERTY_TYPE(ShowFrame  ,(0) ,group, App::Prop_None, "Specifies if the clip frame appears on the page or not");
    ADD_PROPERTY_TYPE(Views      ,(nullptr) ,group, App::Prop_None, "The Views in this Clip group");
    Views.setScope(App::LinkScope::Global);

    // hide N/A properties
    ScaleType.setStatus(App::Property::ReadOnly, true);
    ScaleType.setStatus(App::Property::Hidden, true);
    Scale.setStatus(App::Property::ReadOnly, true);
    Scale.setStatus(App::Property::Hidden, true);
}

void DrawViewClip::onChanged(const App::Property* prop)
{
    if ((prop == &Height) ||
        (prop == &Width) ||
        (prop == &ShowFrame) ||
        (prop == &Views)) {
        requestPaint();
    }
    DrawView::onChanged(prop);
}

void DrawViewClip::addView(App::DocumentObject* docObj)
{
    if (!docObj->isDerivedFrom<DrawView>() && !docObj->isDerivedFrom<App::Link>()) {
        return;
    }

    auto* view = dynamic_cast<DrawView*>(docObj);

    if (!view) {
        auto* link = dynamic_cast<App::Link*>(docObj);
        if (!link) {
            return;
        }

        if (link) {
            view = dynamic_cast<DrawView*>(link->getLinkedObject());
            if (!view) {
                return;
            }
        }
    }

    std::vector<App::DocumentObject*> newViews(Views.getValues());
    newViews.push_back(docObj);
    Views.setValues(newViews);
    QRectF viewRect = view->getRectAligned();
    QPointF clipPoint(X.getValue(), Y.getValue());
    if (viewRect.contains(clipPoint)) {
        //position so the part of view that is overlapped by clip frame
        //stays in the clip frame
        double deltaX = view->X.getValue() - X.getValue();
        double deltaY = view->Y.getValue() - Y.getValue();
        view->X.setValue(deltaX);
        view->Y.setValue(deltaY);
    } else {
        //position in centre of clip group frame
        view->X.setValue(0.0);
        view->Y.setValue(0.0);
    }

    //reparent view to clip in tree
    auto page = findParentPage();
    page->Views.touch();
}

void DrawViewClip::removeView(App::DocumentObject* docObj)
{
    std::vector<App::DocumentObject *> newViews;
    std::string viewName = docObj->getNameInDocument();
    for (auto* view : Views.getValues()) {
        if (viewName.compare(view->getNameInDocument()) != 0) {
            newViews.push_back(view);
        }
    }
    Views.setValues(newViews);
}

std::vector<App::DocumentObject*> DrawViewClip::getViews() const
{
    std::vector<App::DocumentObject*> views = Views.getValues();
    std::vector<App::DocumentObject*> allViews;
    for (auto& v : views) {
        if (v->isDerivedFrom(App::Link::getClassTypeId())) {
            v = static_cast<App::Link*>(v)->getLinkedObject();
        }

        if (!v->isDerivedFrom(DrawView::getClassTypeId())) {
            continue;
        }

        allViews.push_back(v);
    }
    return allViews;
}

App::DocumentObjectExecReturn *DrawViewClip::execute()
{
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    std::vector<App::DocumentObject*> children = getViews();
    for (auto* obj : getViews()) {
        if (obj->isDerivedFrom<DrawView>()) {
            auto* view = static_cast<TechDraw::DrawView*>(obj);
            view->requestPaint();
        }
    }

    requestPaint();
    overrideKeepUpdated(false);
    return DrawView::execute();
}

//NOTE: DocumentObject::mustExecute returns 1/0 and not true/false
short DrawViewClip::mustExecute() const
{
    if (!isRestoring()) {
        if (Height.isTouched() ||
            Width.isTouched() ||
            Views.isTouched()) {
            return 1;
        }
    }
    return TechDraw::DrawView::mustExecute();
}

std::vector<std::string> DrawViewClip::getChildViewNames()
{
    std::vector<std::string> childNames;
    for (auto* obj : getViews()) {
        if (obj->isDerivedFrom<DrawView>()) {
            std::string name = obj->getNameInDocument();
            childNames.push_back(name);
        }
    }
    return childNames;
}

bool DrawViewClip::isViewInClip(App::DocumentObject* view)
{
    for (auto* obj : getViews()) {
        if (obj == view) {
            return true;
        }
    }
    return false;
}

PyObject *DrawViewClip::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewClipPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}


// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewClipPython, TechDraw::DrawViewClip)
template<> const char* TechDraw::DrawViewClipPython::getViewProviderName() const {
    return "TechDrawGui::ViewProviderViewClip";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewClip>;
}
