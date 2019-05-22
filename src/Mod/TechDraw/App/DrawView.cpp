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

#include "DrawView.h"
#include "DrawPage.h"
#include "DrawViewCollection.h"
#include "DrawViewClip.h"
#include "DrawProjGroup.h"
#include "DrawProjGroupItem.h"
#include "DrawLeaderLine.h"
#include "DrawUtil.h"

#include <Mod/TechDraw/App/DrawViewPy.h>  // generated from DrawViewPy.xml

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
                                                                  pow(10,- Base::UnitsApi::getDecimals())};


PROPERTY_SOURCE(TechDraw::DrawView, App::DocumentObject)

DrawView::DrawView(void):
    autoPos(true),
    mouseMove(false)
{
    static const char *group = "Base";
    ADD_PROPERTY_TYPE(X ,(0),group,App::Prop_None,"X position of the view on the page in internal units (mm)");
    ADD_PROPERTY_TYPE(Y ,(0),group,App::Prop_None,"Y position of the view on the page in internal units (mm)");
    ADD_PROPERTY_TYPE(LockPosition ,(false),group,App::Prop_None,"Lock View position to parent Page or Group");
    ADD_PROPERTY_TYPE(Rotation ,(0),group,App::Prop_None,"Rotation of the view on the page in degrees counterclockwise");

    ScaleType.setEnums(ScaleTypeEnums);
    ADD_PROPERTY_TYPE(ScaleType,((long)0),group, App::Prop_None, "Scale Type");
    ADD_PROPERTY_TYPE(Scale ,(1.0),group,App::Prop_None,"Scale factor of the view");
    Scale.setConstraints(&scaleRange);

    ADD_PROPERTY_TYPE(Caption ,(""),group,App::Prop_None,"Short text about the view");
}

DrawView::~DrawView()
{
}

App::DocumentObjectExecReturn *DrawView::execute(void)
{
//    Base::Console().Message("DV::execute() - %s\n", getNameInDocument());
    handleXYLock();
    requestPaint();
    return App::DocumentObject::execute();
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
            LockPosition.purgeTouched(); 
        }
    }
    App::DocumentObject::onChanged(prop);
}

bool DrawView::isLocked(void) const
{
    return LockPosition.getValue();
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
                    ScaleType.isTouched() ||
                    X.isTouched() ||
                    Y.isTouched() );
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
            page = dynamic_cast<TechDraw::DrawPage *>(*it);
        }

        if ((*it)->getTypeId().isDerivedFrom(DrawViewCollection::getClassTypeId())) {
            collection = dynamic_cast<TechDraw::DrawViewCollection *>(*it);
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


double DrawView::autoScale(double w, double h) const
{
    double fudgeFactor = 0.90;
    QRectF viewBox = getRect();
    //have to unscale rect to determine new scale
    double vbw = viewBox.width()/getScale();
    double vbh = viewBox.height()/getScale();
    double xScale = w/vbw;
    double yScale = h/vbh;
    //TODO: find a standard scale that's close? 1:2, 1:10, 1:100...?  Logic in TaskProjGroup
    double newScale = fudgeFactor * std::min(xScale,yScale);
    newScale = DrawUtil::sensibleScale(newScale);
    return newScale;
}

//!check if View fits on Page
bool DrawView::checkFit(TechDraw::DrawPage* p) const
{
    bool result = true;
    QRectF viewBox = getRect();
    if ( (viewBox.width() > p->getPageWidth()) ||
         (viewBox.height() > p->getPageHeight()) ) {
        result = false;
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

void DrawView::addRandomVertex(Base::Vector3d pos)
{
    (void) pos;
    Base::Console().Message("DV::addRandomVertex()\n");
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
    } else if (prop->isDerivedFrom(App::PropertyLinkList::getClassTypeId()) 
            && strcmp(prop->getName(),"Source")==0) 
    {
        App::PropertyLinkGlobal glink;
        App::PropertyLink link;
        if (strcmp(glink.getTypeId().getName(),TypeName) == 0) {            //property in file is plg
            glink.setContainer(this);
            glink.Restore(reader);
            if (glink.getValue() != nullptr) {
                static_cast<App::PropertyLinkList*>(prop)->setScope(App::LinkScope::Global);
                static_cast<App::PropertyLinkList*>(prop)->setValue(glink.getValue());
            }
        } else if (strcmp(link.getTypeId().getName(),TypeName) == 0) {            //property in file is pl
            link.setContainer(this);
            link.Restore(reader);
            if (link.getValue() != nullptr) {
                static_cast<App::PropertyLinkList*>(prop)->setScope(App::LinkScope::Global);
                static_cast<App::PropertyLinkList*>(prop)->setValue(link.getValue());
            }
        }
    }
}

bool DrawView::keepUpdated(void)
{
    bool result = false;
    TechDraw::DrawPage *page = findParentPage();
    if(page) {
        result = page->KeepUpdated.getValue();
    }
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
