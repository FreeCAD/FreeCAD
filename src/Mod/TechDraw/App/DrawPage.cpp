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
#include <Precision.hxx>
#include <cmath>
#endif

#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>
#include <Base/UnitsApi.h>

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
#include "DrawViewBalloon.h"

#include <Mod/TechDraw/App/DrawPagePy.h>  // generated from DrawPagePy.xml

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawPage
//===========================================================================

App::PropertyFloatConstraint::Constraints DrawPage::scaleRange = {Precision::Confusion(),
                                                                  std::numeric_limits<double>::max(),
                                                                  pow(10,- Base::UnitsApi::getDecimals())};

PROPERTY_SOURCE(TechDraw::DrawPage, App::DocumentObject)

const char* DrawPage::ProjectionTypeEnums[] = { "First Angle",
                                                "Third Angle",
                                                NULL };

DrawPage::DrawPage(void)
{
    static const char *group = "Page";
    nowUnsetting = false;
    
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    bool autoUpdate = hGrp->GetBool("KeepPagesUpToDate", 1l);

    ADD_PROPERTY_TYPE(KeepUpdated, (autoUpdate), group, (App::PropertyType)(App::Prop_None), "Keep page in sync with model");
    ADD_PROPERTY_TYPE(Template, (0), group, (App::PropertyType)(App::Prop_None), "Attached Template");
    Template.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(Views, (0), group, (App::PropertyType)(App::Prop_None), "Attached Views");
    Views.setScope(App::LinkScope::Global);

    // Projection Properties
    ProjectionType.setEnums(ProjectionTypeEnums);

    hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");

    // In preferences, 0 -> First Angle 1 -> Third Angle
    int projType = hGrp->GetInt("ProjectionAngle", -1);

    if (projType == -1) {
        ADD_PROPERTY(ProjectionType, ((long)0)); // Default to first angle
    } else {
        ADD_PROPERTY(ProjectionType, ((long)projType));
    }

    ADD_PROPERTY_TYPE(Scale, (1.0), group, App::Prop_None, "Scale factor for this Page");
    Scale.setConstraints(&scaleRange);
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
    if ((prop == &KeepUpdated)  &&
         KeepUpdated.getValue()) {
        if (!isRestoring() &&
            !isUnsetting()) {
            //would be nice if this message was displayed immediately instead of after the recomputeFeature
            Base::Console().Message("Rebuilding Views for: %s/%s\n",getNameInDocument(),Label.getValue());
            auto views(Views.getValues());
            for (auto& v: views) {
                //check for children of current view 
                if (v->isDerivedFrom(TechDraw::DrawViewCollection::getClassTypeId()))  {
                    auto dvc = static_cast<TechDraw::DrawViewCollection*>(v);
                    for (auto& vv: dvc->Views.getValues()) {
                        vv->touch();
                    }
                }
                v->recomputeFeature();                   //get all views up to date
            }
        }
    } else if (prop == &Template) {
        if (!isRestoring() &&
            !isUnsetting()) {
            //nothing to page to do??
        }
    } else if(prop == &Scale) {
        // touch all views in the Page as they may be dependent on this scale
        // WF: not sure this loop is required.  Views figure out their scale as required. but maybe
        //     this is needed just to mark the Views to recompute??
        if (!isRestoring()) {
            const std::vector<App::DocumentObject*> &vals = Views.getValues();
            for(std::vector<App::DocumentObject *>::const_iterator it = vals.begin(); it < vals.end(); ++it) {
                TechDraw::DrawView *view = dynamic_cast<TechDraw::DrawView *>(*it);
                if (view != NULL && view->ScaleType.isValue("Page")) {
                    if(std::abs(view->Scale.getValue() - Scale.getValue()) > FLT_EPSILON) {
                       view->Scale.setValue(Scale.getValue());
                    }
                }
            }
        }
    } else if (prop == &ProjectionType) {
      // touch all ortho views in the Page as they may be dependent on Projection Type  //(is this true?)
      const std::vector<App::DocumentObject*> &vals = Views.getValues();
      for(std::vector<App::DocumentObject *>::const_iterator it = vals.begin(); it < vals.end(); ++it) {
          TechDraw::DrawProjGroup *view = dynamic_cast<TechDraw::DrawProjGroup *>(*it);
          if (view != NULL && view->ProjectionType.isValue("Default")) {
              view->ProjectionType.touch();
          }
      }

      // TODO: Also update Template graphic.

    }
    App::DocumentObject::onChanged(prop); 
}

//Page is just a container. It doesn't "do" anything.
App::DocumentObjectExecReturn *DrawPage::execute(void)
{
    return App::DocumentObject::StdReturn;
}

// this is now irrelevant, b/c DP::execute doesn't do anything. 
short DrawPage::mustExecute() const
{
    return App::DocumentObject::mustExecute();
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

    throw Base::RuntimeError("Template not set for Page");
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

    throw Base::RuntimeError("Template not set for Page");
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
    throw Base::RuntimeError("Template not set for Page");
}

int DrawPage::addView(App::DocumentObject *docObj)
{
    if(!docObj->isDerivedFrom(TechDraw::DrawView::getClassTypeId()))
        return -1;
    DrawView* view = static_cast<DrawView*>(docObj);

      //position all new views in center of Page (exceptDVDimension)
    if (!docObj->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId()) &&
        !docObj->isDerivedFrom(TechDraw::DrawViewBalloon::getClassTypeId())) {
        view->X.setValue(getPageWidth()/2.0);
        view->Y.setValue(getPageHeight()/2.0);
    }

    //add view to list
    const std::vector<App::DocumentObject *> currViews = Views.getValues();
    std::vector<App::DocumentObject *> newViews(currViews);
    newViews.push_back(docObj);
    Views.setValues(newViews);

    //check if View fits on Page
    if ( !view->checkFit(this) ) {
        Base::Console().Warning("%s is larger than page. Will be scaled.\n",view->getNameInDocument());
        view->ScaleType.setValue("Automatic");
    }

    view->checkScale();

    return Views.getSize();
}

//Note Views might be removed from document elsewhere so need to check if a View is still in Document here
int DrawPage::removeView(App::DocumentObject *docObj)
{
    if(!docObj->isDerivedFrom(TechDraw::DrawView::getClassTypeId()))
        return -1;

    App::Document* doc = docObj->getDocument();
    if (doc == nullptr) {
        return -1;
    }

    const char* name = docObj->getNameInDocument();
    if (!name) {
         return -1;
    }
    const std::vector<App::DocumentObject*> currViews = Views.getValues();
    std::vector<App::DocumentObject*> newViews;
    std::vector<App::DocumentObject*>::const_iterator it = currViews.begin();
    for (; it != currViews.end(); it++) {
        App::Document* viewDoc = (*it)->getDocument();
        if (viewDoc == nullptr) {
            continue;
        }

        std::string viewName = name;
        if (viewName.compare((*it)->getNameInDocument()) != 0) {
            newViews.push_back((*it));
        }
    }
    Views.setValues(newViews);
    return Views.getSize();
}

void DrawPage::requestPaint(void)
{
    signalGuiPaint(this);
}

void DrawPage::onDocumentRestored()
{
    //control drawing updates on restore based on Preference
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    bool autoUpdate = hGrp->GetBool("KeepPagesUpToDate", 1l);
    KeepUpdated.setValue(autoUpdate);

    std::vector<App::DocumentObject*> featViews = getAllViews();
    std::vector<App::DocumentObject*>::const_iterator it = featViews.begin();
    //first, make sure all the Parts have been executed so GeometryObjects exist
    for(; it != featViews.end(); ++it) {
        TechDraw::DrawViewPart *part = dynamic_cast<TechDraw::DrawViewPart *>(*it);
        if (part != nullptr &&
            !part->hasGeometry()) {
            part->recomputeFeature();
        }
    }
    //second, make sure all the Dimensions have been executed so Measurements have References
    for(it = featViews.begin(); it != featViews.end(); ++it) {
        TechDraw::DrawViewDimension *dim = dynamic_cast<TechDraw::DrawViewDimension *>(*it);
        if (dim != nullptr) {
            dim->recomputeFeature();
        }
    }
    App::DocumentObject::onDocumentRestored();
}

std::vector<App::DocumentObject*> DrawPage::getAllViews(void) 
{
    auto views = Views.getValues();   //list of docObjects
    std::vector<App::DocumentObject*> allViews;
    for (auto& v: views) {
        allViews.push_back(v);
        if (v->isDerivedFrom(TechDraw::DrawProjGroup::getClassTypeId())) {
            TechDraw::DrawProjGroup* dpg = static_cast<TechDraw::DrawProjGroup*>(v);
            if (dpg != nullptr) {                                              //can't really happen!
              std::vector<App::DocumentObject*> pgViews = dpg->Views.getValues();
              allViews.insert(allViews.end(),pgViews.begin(),pgViews.end());
            }
        }
    }
    return allViews;
}

void DrawPage::unsetupObject()
{
    nowUnsetting = true;

    // Remove the Page's views & template from document
    App::Document* doc = getDocument();
    std::string docName = doc->getName();

    while (Views.getValues().size() > 0 ) {
        const std::vector<App::DocumentObject*> currViews = Views.getValues();
        App::DocumentObject* child = currViews.front();
        std::string viewName = child->getNameInDocument();
        Base::Interpreter().runStringArg("App.getDocument(\"%s\").removeObject(\"%s\")",
                                          docName.c_str(), viewName.c_str());
    }
    std::vector<App::DocumentObject*> emptyViews;      //probably superfluous
    Views.setValues(emptyViews);

    App::DocumentObject* tmp = Template.getValue();
    if (tmp != nullptr) {
        std::string templateName = Template.getValue()->getNameInDocument();
        Base::Interpreter().runStringArg("App.getDocument(\"%s\").removeObject(\"%s\")",
                                              docName.c_str(), templateName.c_str());
    }
    Template.setValue(nullptr);
}

void DrawPage::Restore(Base::XMLReader &reader)
{
    reader.readElement("Properties");
    int Cnt = reader.getAttributeAsInteger("Count");

    for (int i=0 ;i<Cnt ;i++) {
        reader.readElement("Property");
        const char* PropName = reader.getAttribute("name");
        const char* TypeName = reader.getAttribute("type");
        App::Property* schemaProp = getPropertyByName(PropName);
        try {
            if(schemaProp){
                if (strcmp(schemaProp->getTypeId().getName(), TypeName) == 0){        //if the property type in obj == type in schema
                    schemaProp->Restore(reader);                                      //nothing special to do
                } else  {
                    if (strcmp(PropName, "Scale") == 0) {
                        if (schemaProp->isDerivedFrom(App::PropertyFloatConstraint::getClassTypeId())){  //right property type
                            schemaProp->Restore(reader);                                                  //nothing special to do
                        } else {                                                                //Scale, but not PropertyFloatConstraint
                            App::PropertyFloat tmp;
                            if (strcmp(tmp.getTypeId().getName(),TypeName)) {                   //property in file is Float
                                tmp.setContainer(this);
                                tmp.Restore(reader);
                                double tmpValue = tmp.getValue();
                                if (tmpValue > 0.0) {
                                    static_cast<App::PropertyFloatConstraint*>(schemaProp)->setValue(tmpValue);
                                } else {
                                    static_cast<App::PropertyFloatConstraint*>(schemaProp)->setValue(1.0);
                                }
                            } else {
                                // has Scale prop that isn't Float! 
                                Base::Console().Log("DrawPage::Restore - old Document Scale is Not Float!\n");
                                // no idea
                            }
                        }
                    } else {
                        Base::Console().Log("DrawPage::Restore - old Document has unknown Property\n");
                    }
                }
            }
        }
        catch (const Base::XMLParseException&) {
            throw; // re-throw
        }
        catch (const Base::Exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        catch (const std::exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        catch (const char* e) {
            Base::Console().Error("%s\n", e);
        }
#ifndef FC_DEBUG
        catch (...) {
            Base::Console().Error("PropertyContainer::Restore: Unknown C++ exception thrown\n");
        }
#endif

        reader.readEndElement("Property");
    }
    reader.readEndElement("Properties");
}


