/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <Standard_Failure.hxx>
#include <Precision.hxx>
#include <cmath>
#endif


#include <App/Application.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Console.h>
#include <Base/UnitsApi.h>

#include "DrawPage.h"
#include "DrawViewCollection.h"
#include "DrawViewClip.h"
#include "DrawProjGroup.h"
#include "DrawProjGroupItem.h"
#include "DrawLeaderLine.h"
#include "Preferences.h"
#include "DrawUtil.h"
#include "Geometry.h"
#include "Cosmetic.h"

#include <Mod/TechDraw/App/DrawViewPy.h>  // generated from DrawViewPy.xml

#include "DrawView.h"

using namespace TechDraw;

//===========================================================================
// DrawView
//===========================================================================

const char* DrawView::ScaleTypeEnums[]= {"Page",
                                         "Automatic",
                                         "Custom",
                                         NULL};
App::PropertyFloatConstraint::Constraints DrawView::scaleRange = {Precision::Confusion(),
                                                                  std::numeric_limits<double>::max(),
                                                                  (0.1)}; // increment by 0.1


PROPERTY_SOURCE(TechDraw::DrawView, App::DocumentObject)

DrawView::DrawView(void):
    autoPos(true),
    mouseMove(false)
{
    static const char *group = "Base";
    ADD_PROPERTY_TYPE(X, (0.0), group, (App::PropertyType)(App::Prop_Output | App::Prop_NoRecompute), "X position");
    ADD_PROPERTY_TYPE(Y, (0.0), group, (App::PropertyType)(App::Prop_Output | App::Prop_NoRecompute), "Y position");
    ADD_PROPERTY_TYPE(LockPosition, (false), group, App::Prop_Output, "Lock View position to parent Page or Group");
    ADD_PROPERTY_TYPE(Rotation, (0.0), group, App::Prop_Output, "Rotation in degrees counterclockwise");

    ScaleType.setEnums(ScaleTypeEnums);
    ADD_PROPERTY_TYPE(ScaleType, (prefScaleType()), group, App::Prop_Output, "Scale Type");
    ADD_PROPERTY_TYPE(Scale, (prefScale()), group, App::Prop_Output, "Scale factor of the view. Scale factors like 1:100 can be written as =1/100");
    Scale.setConstraints(&scaleRange);

    ADD_PROPERTY_TYPE(Caption, (""), group, App::Prop_Output, "Short text about the view");
}

DrawView::~DrawView()
{
}

App::DocumentObjectExecReturn *DrawView::execute(void)
{
//    Base::Console().Message("DV::execute() - %s touched: %d\n", getNameInDocument(), isTouched());
    if (findParentPage() == nullptr) {
        return App::DocumentObject::execute();
    }
    handleXYLock();
    requestPaint();
    //documentobject::execute doesn't do anything useful for us.
    //documentObject::recompute causes an infinite loop.
    //should not be necessary to purgeTouched here, but it prevents a superfluous feature recompute
    purgeTouched();                           //this should not be necessary!
    return App::DocumentObject::StdReturn;
}

void DrawView::checkScale(void)
{
    TechDraw::DrawPage *page = findParentPage();
    if(page &&
       keepUpdated()) {
        if (ScaleType.isValue("Page")) {
            if(std::abs(page->Scale.getValue() - getScale()) > FLT_EPSILON) {
                Scale.setValue(page->Scale.getValue());
                Scale.purgeTouched();
            }
        }
    }
}

void DrawView::onChanged(const App::Property* prop)
{
//Coding note: calling execute, recompute or recomputeFeature inside an onChanged
//method can create infinite loops.  In general don't do this!  There may be 
//situations where it is OK, but careful analysis is a must. 
    if (!isRestoring()) {
        if (prop == &ScaleType) {
            auto page = findParentPage();
            if (ScaleType.isValue("Page")) {
                Scale.setStatus(App::Property::ReadOnly,true);
                if (page != nullptr) {
                    if(std::abs(page->Scale.getValue() - getScale()) > FLT_EPSILON) {
                       Scale.setValue(page->Scale.getValue());
                       Scale.purgeTouched();
                    }
                }
            } else if ( ScaleType.isValue("Custom") ) {
                //don't change Scale
                Scale.setStatus(App::Property::ReadOnly,false);
            } else if ( ScaleType.isValue("Automatic") ) {
                Scale.setStatus(App::Property::ReadOnly,true);
                if (!checkFit(page)) {
                    double newScale = autoScale(page->getPageWidth(),page->getPageHeight());
                    if(std::abs(newScale - getScale()) > FLT_EPSILON) {           //stops onChanged/execute loop
                        Scale.setValue(newScale);
                        Scale.purgeTouched();
                    }
                }
            }
        } else if (prop == &LockPosition) {
            handleXYLock();
            requestPaint();         //change lock icon
            LockPosition.purgeTouched(); 
        } else if ((prop == &Caption) ||
            (prop == &Label)) {
            requestPaint();
        } else if ((prop == &X) ||
            (prop == &Y)) {
            X.purgeTouched();
            Y.purgeTouched();
        }
    }
    App::DocumentObject::onChanged(prop);
}

bool DrawView::isLocked(void) const
{
    return LockPosition.getValue();
}

bool DrawView::showLock(void) const
{
    return true;
}

//override this for View inside a group (ex DPGI in DPG)
void DrawView::handleXYLock(void) 
{
    if (isLocked()) {
        if (!X.testStatus(App::Property::ReadOnly)) {
            X.setStatus(App::Property::ReadOnly,true);
            X.purgeTouched();
        }
        if (!Y.testStatus(App::Property::ReadOnly)) {
            Y.setStatus(App::Property::ReadOnly,true);
            Y.purgeTouched();
        }
    } else {
        if (X.testStatus(App::Property::ReadOnly)) {
            X.setStatus(App::Property::ReadOnly,false);
            X.purgeTouched();
        }
        if (Y.testStatus(App::Property::ReadOnly)) {
            Y.setStatus(App::Property::ReadOnly,false);
            Y.purgeTouched();
        }
    }
}

short DrawView::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result  =  (Scale.isTouched()  ||
                    ScaleType.isTouched());
    }
    if ((bool) result) {
        return result;
    }
    return App::DocumentObject::mustExecute();
}

////you must override this in derived class
QRectF DrawView::getRect() const
{
    QRectF result(0,0,1,1);
    return result;
}

void DrawView::onDocumentRestored()
{
    handleXYLock();
    DrawView::execute();
}

DrawPage* DrawView::findParentPage() const
{
    // Get Feature Page
    DrawPage *page = 0;
    DrawViewCollection *collection = 0;
    std::vector<App::DocumentObject*> parent = getInList();
    for (std::vector<App::DocumentObject*>::iterator it = parent.begin(); it != parent.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawPage::getClassTypeId())) {
            page = static_cast<TechDraw::DrawPage *>(*it);
        }

        if ((*it)->getTypeId().isDerivedFrom(DrawViewCollection::getClassTypeId())) {
            collection = static_cast<TechDraw::DrawViewCollection *>(*it);
            page = collection->findParentPage();
        }

        if(page)
          break; // Found page so leave
    }

    return page;
}

bool DrawView::isInClip()
{
    std::vector<App::DocumentObject*> parent = getInList();
    for (std::vector<App::DocumentObject*>::iterator it = parent.begin(); it != parent.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawViewClip::getClassTypeId())) {
            return true;
        }
    }
    return false;
}

DrawViewClip* DrawView::getClipGroup(void)
{
    std::vector<App::DocumentObject*> parent = getInList();
    App::DocumentObject* obj = nullptr;
    DrawViewClip* result = nullptr;
    for (std::vector<App::DocumentObject*>::iterator it = parent.begin(); it != parent.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawViewClip::getClassTypeId())) {
            obj = (*it);
            result = dynamic_cast<DrawViewClip*>(obj);
            break;

        }
    }
    return result;
}

double DrawView::autoScale(void) const
{
    auto page = findParentPage();
    double w = page->getPageWidth();
    double h = page->getPageHeight();
    return autoScale(w,h);
}

//compare 1:1 rect of view to pagesize(pw,h) 
double DrawView::autoScale(double pw, double ph) const
{
//    Base::Console().Message("DV::autoScale(Page: %.3f, %.3f) - %s\n", pw, ph, getNameInDocument());
    double fudgeFactor = 1.0;  //make it a bit smaller just in case.
    QRectF viewBox = getRect();           //getRect is scaled (ie current actual size)
    if (!viewBox.isValid()) {
        return 1.0;
    }
    //have to unscale rect to determine new scale
    double vbw = viewBox.width()/getScale();
    double vbh = viewBox.height()/getScale();
    double xScale = pw/vbw;           // > 1 page bigger than figure
    double yScale = ph/vbh;           // < 1 page is smaller than figure
    double newScale = std::min(xScale,yScale) * fudgeFactor; 
    double sensibleScale = DrawUtil::sensibleScale(newScale);
    return sensibleScale;
}

bool DrawView::checkFit(void) const
{
    auto page = findParentPage();
    return checkFit(page);
}

//!check if View is too big for page
//should check if unscaled rect is too big for page
bool DrawView::checkFit(TechDraw::DrawPage* p) const
{
    bool result = true;
    double fudge = 1.1;

    double width = 0.0;
    double height = 0.0;
    QRectF viewBox = getRect();    //rect is scaled
    if (!viewBox.isValid()) {
        result = true;
    } else {
        width = viewBox.width() / getScale();        //unscaled rect w x h
        height = viewBox.height() / getScale(); 
        width *= fudge;
        height *= fudge;
        if ( (width > p->getPageWidth()) ||
             (height > p->getPageHeight()) ) {
            result = false;
        }
    }
    return result;
}

void DrawView::setPosition(double x, double y, bool force)
{
//    Base::Console().Message("DV::setPosition(%.3f,%.3f) - \n",x,y,getNameInDocument());
    if ( (!isLocked()) ||
         (force) ) {
        X.setValue(x);
        Y.setValue(y);
    }
}

//TODO: getScale is no longer needed and could revert to Scale.getValue
double DrawView::getScale(void) const
{
    auto result = Scale.getValue();
    if (!(result > 0.0)) {
        result = 1.0;
        Base::Console().Log("DrawView - %s - bad scale found (%.3f) using 1.0\n",getNameInDocument(),Scale.getValue());
    }
    return result;
}

//return list of Leaders which reference this DV
std::vector<TechDraw::DrawLeaderLine*> DrawView::getLeaders() const
{
    std::vector<TechDraw::DrawLeaderLine*> result;
    std::vector<App::DocumentObject*> children = getInList();
    for (std::vector<App::DocumentObject*>::iterator it = children.begin(); it != children.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawLeaderLine::getClassTypeId())) {
            TechDraw::DrawLeaderLine* lead = dynamic_cast<TechDraw::DrawLeaderLine*>(*it);
            result.push_back(lead);
        }
    }
    return result;
}

void DrawView::handleChangedPropertyType(
        Base::XMLReader &reader, const char * TypeName, App::Property * prop) 
{
    if (prop == &Scale) {
        App::PropertyFloat tmp;
        if (strcmp(tmp.getTypeId().getName(),TypeName)==0) {                   //property in file is Float
            tmp.setContainer(this);
            tmp.Restore(reader);
            double tmpValue = tmp.getValue();
            if (tmpValue > 0.0) {
                Scale.setValue(tmpValue);
            } else {
                Scale.setValue(1.0);
            }
        } else {
            // has Scale prop that isn't Float! 
            Base::Console().Log("DrawPage::Restore - old Document Scale is Not Float!\n");
            // no idea
        }
    }
    else if (prop->isDerivedFrom(App::PropertyLinkList::getClassTypeId())
        && strcmp(prop->getName(), "Source") == 0) {
        App::PropertyLinkGlobal glink;
        App::PropertyLink link;
        if (strcmp(glink.getTypeId().getName(), TypeName) == 0) {            //property in file is plg
            glink.setContainer(this);
            glink.Restore(reader);
            if (glink.getValue() != nullptr) {
                static_cast<App::PropertyLinkList*>(prop)->setScope(App::LinkScope::Global);
                static_cast<App::PropertyLinkList*>(prop)->setValue(glink.getValue());
            }
        }
        else if (strcmp(link.getTypeId().getName(), TypeName) == 0) {            //property in file is pl
            link.setContainer(this);
            link.Restore(reader);
            if (link.getValue() != nullptr) {
                static_cast<App::PropertyLinkList*>(prop)->setScope(App::LinkScope::Global);
                static_cast<App::PropertyLinkList*>(prop)->setValue(link.getValue());
            }
        }
    }

    // property X had App::PropertyFloat and was changed to App::PropertyLength
    // and later to PropertyDistance because some X,Y are relative to existing points on page
    else if (prop == &X && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat XProperty;
        XProperty.setContainer(this);
        // restore the PropertyFloat to be able to set its value
        XProperty.Restore(reader);
        X.setValue(XProperty.getValue());
    }
    else if (prop == &X && strcmp(TypeName, "App::PropertyLength") == 0) {
        App::PropertyLength X2Property;
        X2Property.Restore(reader);
        X.setValue(X2Property.getValue());
    }
    else if (prop == &Y && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat YProperty;
        YProperty.setContainer(this);
        YProperty.Restore(reader);
        Y.setValue(YProperty.getValue());
    }
    else if (prop == &Y && strcmp(TypeName, "App::PropertyLength") == 0) {
        App::PropertyLength Y2Property;
        Y2Property.Restore(reader);
        Y.setValue(Y2Property.getValue());
    }
       
// property Rotation had App::PropertyFloat and was changed to App::PropertyAngle
    else if (prop == &Rotation && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat RotationProperty;
        RotationProperty.setContainer(this);
        RotationProperty.Restore(reader);
        Rotation.setValue(RotationProperty.getValue());
    }
}

bool DrawView::keepUpdated(void)
{
//    Base::Console().Message("DV::keepUpdated() - %s\n", getNameInDocument());
    bool result = false;

    bool pageUpdate = false;
    bool force = false;
    TechDraw::DrawPage *page = findParentPage();
    if(page) {
        pageUpdate = page->KeepUpdated.getValue();
        force = page->forceRedraw();
    }

    if (DrawPage::GlobalUpdateDrawings() &&
        pageUpdate)  {
        result = true;
    } else if (!DrawPage::GlobalUpdateDrawings() &&
                DrawPage::AllowPageOverride()    &&
                pageUpdate) {
        result = true;
    }
    if (force) {         //when do we turn this off??
        result = true;
    }
    return result;
}

int DrawView::prefScaleType(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    int result = hGrp->GetInt("DefaultScaleType", 0); 
    return result;
}

double DrawView::prefScale(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    double result = hGrp->GetFloat("DefaultViewScale", 1.0); 
    return result;
}

void DrawView::requestPaint(void)
{
//    Base::Console().Message("DV::requestPaint() - %s\n", getNameInDocument());
    signalGuiPaint(this);
}

PyObject *DrawView::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewPython, TechDraw::DrawView)
template<> const char* TechDraw::DrawViewPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderDrawingView";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawView>;
}
