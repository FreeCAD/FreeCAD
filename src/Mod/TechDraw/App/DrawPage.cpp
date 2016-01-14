/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <sstream>
# include <iostream>
# include <iterator>
#endif

#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <App/Application.h>
#include <App/Document.h>
#include <boost/regex.hpp>

#include "DrawPage.h"
#include "DrawView.h"
#include "DrawProjGroup.h"
#include "DrawViewClip.h"
#include "DrawTemplate.h"
#include "DrawViewCollection.h"
#include "DrawViewPart.h"
#include "DrawViewDimension.h"

#include "DrawPagePy.h"  // generated from DrawPagePy.xml

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawPage
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawPage, App::DocumentObject)

const char* DrawPage::ProjectionTypeEnums[]= {"First Angle",
                                                 "Third Angle",
                                                 NULL};

DrawPage::DrawPage(void)
{
    static const char *group = "Page";

    ADD_PROPERTY_TYPE(Template, (0), group, (App::PropertyType)(App::Prop_None), "Attached Template");
    ADD_PROPERTY_TYPE(Views, (0), group, (App::PropertyType)(App::Prop_None),"Attached Views");

    // Projection Properties
    ProjectionType.setEnums(ProjectionTypeEnums);
    ADD_PROPERTY(ProjectionType, ((long)0));
    ADD_PROPERTY_TYPE(Scale ,(1.0), group, App::Prop_None, "Scale factor for this Page");
}

DrawPage::~DrawPage()
{
}

void DrawPage::onBeforeChange(const App::Property* prop)
{
    App::DocumentObject::onBeforeChange(prop);
}

void DrawPage::onChanged(const App::Property* prop)
{
    if (prop == &Template) {
        if (!isRestoring()) {
        //TODO: reload if Template prop changes (ie different Template)
        Base::Console().Message("TODO: Unimplemented function DrawPage::onChanged(Template)\n");
        }
    } else if (prop == &Views) {
        if (!isRestoring()) {
            //TODO: reload if Views prop changes (ie adds/deletes)
            //touch();
        }
    } else if(prop == &Scale) {
        // touch all views in the Page as they may be dependent on this scale
      const std::vector<App::DocumentObject*> &vals = Views.getValues();
      for(std::vector<App::DocumentObject *>::const_iterator it = vals.begin(); it < vals.end(); ++it) {
          TechDraw::DrawView *view = dynamic_cast<TechDraw::DrawView *>(*it);
          if (view != NULL && view->ScaleType.isValue("Document")) {
              view->Scale.touch();
          }
      }
    } else if (prop == &ProjectionType) {
      // touch all ortho views in the Page as they may be dependent on Projection Type
      const std::vector<App::DocumentObject*> &vals = Views.getValues();
      for(std::vector<App::DocumentObject *>::const_iterator it = vals.begin(); it < vals.end(); ++it) {
          TechDraw::DrawProjGroup *view = dynamic_cast<TechDraw::DrawProjGroup *>(*it);
          if (view != NULL && view->ProjectionType.isValue("Document")) {
              view->ProjectionType.touch();
          }
      }

      // TODO: Also update Template graphic.

    }
    App::DocumentObject::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawPage::execute(void)
{
    Template.touch();
    Views.touch();
    return App::DocumentObject::StdReturn;
}

short DrawPage::mustExecute() const
{
    if(Scale.isTouched())
        return 1;

    // Check the value of template if this has been modified
    App::DocumentObject* tmpl = Template.getValue();
    if(tmpl && tmpl->isTouched())
        return 1;

    // Check if within this Page, any Views have been touched
    bool ViewsTouched = false;
    const std::vector<App::DocumentObject*> &vals = Views.getValues();
    for(std::vector<App::DocumentObject *>::const_iterator it = vals.begin(); it < vals.end(); ++it) {
       if((*it)->isTouched()) {
            return 1;
        }
    }

    return (ViewsTouched) ? 1 : App::DocumentObject::mustExecute();
}

PyObject *DrawPage::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawPagePy(this),true);
    }

    return Py::new_reference_to(PythonObject); 
}

bool DrawPage::hasValidTemplate() const
{
    App::DocumentObject *obj = 0;
    obj = Template.getValue();

    if(obj && obj->isDerivedFrom(TechDraw::DrawTemplate::getClassTypeId())) {
        TechDraw::DrawTemplate *templ = static_cast<TechDraw::DrawTemplate *>(obj);
        if (templ->getWidth() > 0. &&
            templ->getHeight() > 0.) {
            return true;
        }
    }

    return false;
}

double DrawPage::getPageWidth() const
{
    App::DocumentObject *obj = 0;
    obj = Template.getValue();

    if( obj && obj->isDerivedFrom(TechDraw::DrawTemplate::getClassTypeId()) ) {
        TechDraw::DrawTemplate *templ = static_cast<TechDraw::DrawTemplate *>(obj);
        return templ->getWidth();
    }

    throw Base::Exception("Template not set for Page");
}

double DrawPage::getPageHeight() const
{
    App::DocumentObject *obj = 0;
    obj = Template.getValue();

    if(obj) {
        if(obj->isDerivedFrom(TechDraw::DrawTemplate::getClassTypeId())) {
            TechDraw::DrawTemplate *templ = static_cast<TechDraw::DrawTemplate *>(obj);
            return templ->getHeight();
        }
    }

    throw Base::Exception("Template not set for Page");
}

const char * DrawPage::getPageOrientation() const
{
    App::DocumentObject *obj;
    obj = Template.getValue();

    if(obj) {
        if(obj->isDerivedFrom(TechDraw::DrawTemplate::getClassTypeId())) {
          TechDraw::DrawTemplate *templ = static_cast<TechDraw::DrawTemplate *>(obj);

          return templ->Orientation.getValueAsString();
        }
    }
    throw Base::Exception("Template not set for Page");
}

int DrawPage::addView(App::DocumentObject *docObj)
{
    if(!docObj->isDerivedFrom(TechDraw::DrawView::getClassTypeId()))
        return -1;

    const std::vector<App::DocumentObject *> currViews = Views.getValues();
    std::vector<App::DocumentObject *> newViews(currViews);
    newViews.push_back(docObj);
    Views.setValues(newViews);
    Views.touch();
    return Views.getSize();
}

int DrawPage::removeView(App::DocumentObject *docObj)
{
    if(!docObj->isDerivedFrom(TechDraw::DrawView::getClassTypeId()))
        return -1;

    const std::vector<App::DocumentObject*> currViews = Views.getValues();
    std::vector<App::DocumentObject*> newViews;
    std::vector<App::DocumentObject*>::const_iterator it = currViews.begin();
    for (; it != currViews.end(); it++) {
        std::string viewName = docObj->getNameInDocument();
        if (viewName.compare((*it)->getNameInDocument()) != 0) {
            newViews.push_back((*it));
        }
    }
    Views.setValues(newViews);
    Views.touch();

    return Views.getSize();
}

void DrawPage::onDocumentRestored()
{
    std::vector<App::DocumentObject*> featViews = Views.getValues();
    std::vector<App::DocumentObject*>::const_iterator it = featViews.begin();
    //first, make sure all the Parts have been executed so GeometryObjects exist
    for(; it != featViews.end(); ++it) {
        TechDraw::DrawViewPart *part = dynamic_cast<TechDraw::DrawViewPart *>(*it);
        if (part != NULL) {
            part->execute();
        }
    }
    //second, make sure all the Dimensions have been executed so Measurements have References
    for(it = featViews.begin(); it != featViews.end(); ++it) {
        TechDraw::DrawViewDimension *dim = dynamic_cast<TechDraw::DrawViewDimension *>(*it);
        if (dim != NULL) {
            dim->execute();
        }
    }
    recompute();
    App::DocumentObject::onDocumentRestored();
}

