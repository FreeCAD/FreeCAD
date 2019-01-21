/***************************************************************************
 *   Copyright (c) Yorik van Havre <yorik@uncreated.net> 2012              *
 *   Copyright (c) WandererFan <wandererfan@gmail.com> 2015                *
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

#endif

#include <Base/Console.h>

#include "DrawViewClip.h"
#include "DrawPage.h"

#include <Mod/TechDraw/App/DrawViewClipPy.h>  // generated from DrawViewClipPy.xml

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawViewClip
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewClip, TechDraw::DrawView)

DrawViewClip::DrawViewClip(void)
{
    static const char *group = "Clip Group";
    //App::PropertyType hidden = (App::PropertyType)(App::Prop_Hidden);

    ADD_PROPERTY_TYPE(Height     ,(100),group,App::Prop_None,"The height of the view area of this clip");
    ADD_PROPERTY_TYPE(Width      ,(100),group,App::Prop_None,"The width of the view area of this clip");
    ADD_PROPERTY_TYPE(ShowFrame  ,(0) ,group,App::Prop_None,"Specifies if the clip frame appears on the page or not");
    ADD_PROPERTY_TYPE(ShowLabels ,(0) ,group,App::Prop_None,"Specifies if View labels appear within the clip area");
    ADD_PROPERTY_TYPE(Views      ,(0) ,group,App::Prop_None,"The Views in this Clip group");
    Views.setScope(App::LinkScope::Global);

    // hide N/A properties
    ScaleType.setStatus(App::Property::ReadOnly,true);
    ScaleType.setStatus(App::Property::Hidden,true);
    Scale.setStatus(App::Property::ReadOnly,true);
    Scale.setStatus(App::Property::Hidden,true);
}

DrawViewClip::~DrawViewClip()
{
}

void DrawViewClip::onChanged(const App::Property* prop)
{
    DrawView::onChanged(prop);
}

void DrawViewClip::addView(DrawView *view)
{
    const std::vector<App::DocumentObject*> currViews = Views.getValues();
    std::vector<App::DocumentObject *> newViews(currViews);
    newViews.push_back(view);
    Views.setValues(newViews);
    view->X.setValue(0.0);                   //position in centre of clip group frame
    view->Y.setValue(0.0);
    auto page = findParentPage();             //get Page to release child relationship in tree
    page->Views.touch();
}

void DrawViewClip::removeView(DrawView *view)
{
    std::vector<App::DocumentObject *> currViews = Views.getValues();
    std::vector<App::DocumentObject *> newViews;
    std::vector<App::DocumentObject*>::iterator it = currViews.begin();
    for (; it != currViews.end(); it++) {
        std::string viewName = view->getNameInDocument();
        if (viewName.compare((*it)->getNameInDocument()) != 0) {
            newViews.push_back((*it));
        }
    }
    Views.setValues(newViews);
}

App::DocumentObjectExecReturn *DrawViewClip::execute(void)
{
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    std::vector<App::DocumentObject*> children = Views.getValues();
    for (std::vector<App::DocumentObject*>::iterator it = children.begin(); it != children.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawView::getClassTypeId())) {
            TechDraw::DrawView *view = static_cast<TechDraw::DrawView *>(*it);
            view->requestPaint();
        }
    }

    requestPaint();
    return DrawView::execute();
}

short DrawViewClip::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result = ( Height.isTouched() ||
                   Width.isTouched()  ||
                   Views.isTouched());
    }
    if (result) {
        return result;
    }
    return TechDraw::DrawView::mustExecute();
}

std::vector<std::string> DrawViewClip::getChildViewNames()
{
    std::vector<std::string> childNames;
    std::vector<App::DocumentObject*> children = Views.getValues();
    for (std::vector<App::DocumentObject*>::iterator it = children.begin(); it != children.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawView::getClassTypeId())) {
            std::string name = (*it)->getNameInDocument();
            childNames.push_back(name);
        }
    }
    return childNames;
}

bool DrawViewClip::isViewInClip(App::DocumentObject* view)
{
    bool result = false;
    std::vector<App::DocumentObject*> children = Views.getValues();
    for (std::vector<App::DocumentObject*>::iterator it = children.begin(); it != children.end(); ++it) {
        if ((*it) == view) {
            result = true;
        }
    }
    return result;
}

PyObject *DrawViewClip::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewClipPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}


// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewClipPython, TechDraw::DrawViewClip)
template<> const char* TechDraw::DrawViewClipPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderViewClip";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewClip>;
}
