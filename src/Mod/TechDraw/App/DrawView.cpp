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
# include <cmath>
# include <sstream>
# include <Standard_Failure.hxx>
# include <Precision.hxx>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Mod/TechDraw/App/DrawViewPy.h>  // generated from DrawViewPy.xml

#include "DrawView.h"
#include "DrawLeaderLine.h"
#include "DrawPage.h"
#include "DrawUtil.h"
#include "DrawViewClip.h"
#include "DrawViewCollection.h"
#include "Preferences.h"


using namespace TechDraw;
using DU = DrawUtil;

//===========================================================================
// DrawView
//===========================================================================

#if 0   // needed for Qt's lupdate utility
    QT_TRANSLATE_NOOP("DrawPage", "Page");
    QT_TRANSLATE_NOOP("DrawSVGTemplate", "Template");
    QT_TRANSLATE_NOOP("DrawView", "View");
    QT_TRANSLATE_NOOP("DrawViewPart", "View");
    QT_TRANSLATE_NOOP("DrawViewSection", "Section");
    QT_TRANSLATE_NOOP("DrawComplexSection", "Section");
    QT_TRANSLATE_NOOP("DrawViewDetail", "Detail");
    QT_TRANSLATE_NOOP("DrawActiveView", "ActiveView");
    QT_TRANSLATE_NOOP("DrawViewAnnotation", "Annotation");
    QT_TRANSLATE_NOOP("DrawViewImage", "Image");
    QT_TRANSLATE_NOOP("DrawViewSymbol", "Symbol");
    QT_TRANSLATE_NOOP("DrawViewArch", "Arch");
    QT_TRANSLATE_NOOP("DrawViewDraft", "Draft");
    QT_TRANSLATE_NOOP("DrawLeaderLine", "LeaderLine");
    QT_TRANSLATE_NOOP("DrawViewBalloon", "Balloon");
    QT_TRANSLATE_NOOP("DrawViewDimension", "Dimension");
    QT_TRANSLATE_NOOP("DrawViewDimExtent", "Extent");
    QT_TRANSLATE_NOOP("DrawHatch", "Hatch");
    QT_TRANSLATE_NOOP("DrawGeomHatch", "GeomHatch");
#endif

const char* DrawView::ScaleTypeEnums[]= {"Page",
                                         "Automatic",
                                         "Custom",
                                         nullptr};
const double SCALEINCREMENT(0.1);
App::PropertyFloatConstraint::Constraints DrawView::scaleRange = {Precision::Confusion(),
                                                                  std::numeric_limits<double>::max(),
                                                                  (SCALEINCREMENT)}; // increment by 0.1


PROPERTY_SOURCE(TechDraw::DrawView, App::DocumentObject)

DrawView::DrawView():
    autoPos(true),
    mouseMove(false),
    m_overrideKeepUpdated(false)
{
    static const char *group = "Base";
    ADD_PROPERTY_TYPE(X, (0.0), group, (App::PropertyType)(App::Prop_None), "X position");
    ADD_PROPERTY_TYPE(Y, (0.0), group, (App::PropertyType)(App::Prop_None), "Y position");
    ADD_PROPERTY_TYPE(LockPosition, (false), group, App::Prop_Output, "Lock View position to parent Page or Group");
    ADD_PROPERTY_TYPE(Rotation, (0.0), group, App::Prop_Output, "Rotation in degrees counterclockwise");

    ScaleType.setEnums(ScaleTypeEnums);
    ADD_PROPERTY_TYPE(ScaleType, (prefScaleType()), group, App::Prop_Output, "Scale Type");
    ADD_PROPERTY_TYPE(Scale, (prefScale()), group, App::Prop_None, "Scale factor of the view. Scale factors like 1:100 can be written as =1/100");
    Scale.setConstraints(&scaleRange);

    ADD_PROPERTY_TYPE(Caption, (""), group, App::Prop_Output, "Short text about the view");

    setScaleAttribute();
}

DrawView::~DrawView()
{
}

App::DocumentObjectExecReturn *DrawView::execute()
{
//    Base::Console().Message("DV::execute() - %s touched: %d\n", getNameInDocument(), isTouched());
    if (!findParentPage()) {
        return App::DocumentObject::execute();
    }
    handleXYLock();
    //should not be necessary to purgeTouched here, but it prevents a superfluous feature recompute
    purgeTouched();                           //this should not be necessary!
    requestPaint();
    return App::DocumentObject::execute();
}

void DrawView::checkScale()
{
    TechDraw::DrawPage *page = findParentPage();
    if(page) {
        if (ScaleType.isValue("Page")) {
            if(std::abs(page->Scale.getValue() - Scale.getValue()) > FLT_EPSILON) {
                Scale.setValue(page->Scale.getValue());
                Scale.purgeTouched();
            }
        }
    }
}

void DrawView::touchTreeOwner(App::DocumentObject *owner) const
{
    auto ownerView = dynamic_cast<DrawView *>(owner);
    if (ownerView) {
        ownerView->touch();
    }
    else { // If no owner is specified, touch all parent pages
        for (auto page : findAllParentPages()) {
            page->touch();
        }
    }
}

void DrawView::onBeforeChange(const App::Property *prop)
{
    // To avoid keeping the previous parent in some extra variable, we will mark
    // the previous owner for update before the property is actually changed.
    App::PropertyLink *ownerProp = getOwnerProperty();
    if (ownerProp && prop == ownerProp  && !isRestoring()) {
        touchTreeOwner(ownerProp->getValue());
    }

    App::DocumentObject::onBeforeChange(prop);
}

void DrawView::onChanged(const App::Property* prop)
{
//Coding note: calling execute, recompute or recomputeFeature inside an onChanged
//method can create infinite loops if the called method changes a property.  In general
//don't do this!  There are situations where it is OK, but careful analysis is a must.

    if (prop == &Scale &&
        Scale.getValue() < Precision::Confusion()) {
        //this is not supposed to happen since Scale has constraints, but it may
        //happen during changes made in PropertyEditor?
        Scale.setValue(1.0);
        return;
    }

    if (isRestoring()) {
        App::DocumentObject::onChanged(prop);
        return;
    }

    if (prop == &ScaleType) {
        auto page = findParentPage();
        if (!page) {
            //we don't belong to a page yet, so don't bother
            return;
        }
        if (ScaleType.isValue("Page")) {
            Scale.setStatus(App::Property::ReadOnly, true);
            if(std::abs(page->Scale.getValue() - getScale()) > FLT_EPSILON) {
               Scale.setValue(page->Scale.getValue());
            }
        } else if ( ScaleType.isValue("Custom") ) {
            //don't change Scale
            Scale.setStatus(App::Property::ReadOnly, false);
        } else if ( ScaleType.isValue("Automatic") ) {
            Scale.setStatus(App::Property::ReadOnly, true);
            if (!checkFit(page)) {
                double newScale = autoScale(page->getPageWidth(), page->getPageHeight());
                if(std::abs(newScale - getScale()) > FLT_EPSILON) {           //stops onChanged/execute loop
                    Scale.setValue(newScale);
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
    } else if ( prop == &X ||
                prop == &Y ) {
        //X,Y changes are only interesting to DPGI and Gui side
        X.purgeTouched();
        Y.purgeTouched();
    }

    App::PropertyLink *ownerProp = getOwnerProperty();
    if (ownerProp && prop == ownerProp) {
        touchTreeOwner(ownerProp->getValue());
    }

    App::DocumentObject::onChanged(prop);
}

bool DrawView::isLocked() const
{
    return LockPosition.getValue();
}

bool DrawView::showLock() const
{
    return true;
}

//override this for View inside a group (ex DPGI in DPG)
void DrawView::handleXYLock()
{
    if (isLocked()) {
        if (!X.testStatus(App::Property::ReadOnly)) {
            X.setStatus(App::Property::ReadOnly, true);
            X.purgeTouched();
        }
        if (!Y.testStatus(App::Property::ReadOnly)) {
            Y.setStatus(App::Property::ReadOnly, true);
            Y.purgeTouched();
        }
    } else {
        if (X.testStatus(App::Property::ReadOnly)) {
            X.setStatus(App::Property::ReadOnly, false);
            X.purgeTouched();
        }
        if (Y.testStatus(App::Property::ReadOnly)) {
            Y.setStatus(App::Property::ReadOnly, false);
            Y.purgeTouched();
        }
    }
}

short DrawView::mustExecute() const
{
    if (!isRestoring()) {
        if (Scale.isTouched() ||
            ScaleType.isTouched()) {
            return true;
        }
    }
    return App::DocumentObject::mustExecute();
}

////you must override this in derived class
QRectF DrawView::getRect() const
{
    return QRectF(0, 0,1, 1);
}

//get the rectangle centered on the position
QRectF DrawView::getRectAligned() const
{
    double top = Y.getValue() + 0.5 * getRect().height();
    double left = X.getValue() - 0.5 * getRect().width();
    return {left, top, getRect().width(), - getRect().height()};
}

void DrawView::onDocumentRestored()
{
    handleXYLock();
    setScaleAttribute();
    validateScale();
}

//in versions before 0.20 Scale and ScaleType were mishandled.
//In order to not introduce unintended drawing changes in later
//versions, ScaleType Page must be modified if view Scale does
//not match Page Scale
void DrawView::validateScale()
{
    if (ScaleType.isValue("Custom")) {
        //nothing to do here
        return;
    }
    DrawPage* page = findParentPage();
    if (page) {
        if (ScaleType.isValue("Page")) {
            double pageScale = page->Scale.getValue();
            double myScale = Scale.getValue();
            if (!DrawUtil::fpCompare(pageScale, myScale)) {
                ScaleType.setValue("Custom");
            }
        }
    }
}

/**
 * @brief DrawView::countParentPages
 * Fixes a crash in TechDraw when user creates duplicate page without dependencies
 * In fixOrphans() we check how many parent pages an object has before deleting
 * in case it is also a child of another duplicate page
 * @return
 */
//note this won't find parent pages for DrawProjItem since their parent is DrawProjGroup!
int DrawView::countParentPages() const
{
    int count = 0;
    std::vector<App::DocumentObject*> parentAll = getInList();

    //it can happen that a page is repeated in the InList, so we need to
    //prune the duplicates
    std::sort(parentAll.begin(), parentAll.end());
    auto last = std::unique(parentAll.begin(), parentAll.end());
    parentAll.erase(last, parentAll.end());

    for (auto& parent : parentAll) {
        if (parent->isDerivedFrom<DrawPage>()) {
            count++;
        }
    }
    return count;
}

//finds the first DrawPage in this Document that claims to own this DrawView
//note that it is possible to manipulate the Views property of DrawPage so that
//more than 1 DrawPage claims a DrawView.
DrawPage* DrawView::findParentPage() const
{
    // Get Feature Page
    DrawPage *page = nullptr;
    DrawViewCollection *collection = nullptr;
    std::vector<App::DocumentObject*> parentsAll = getInList();
    for (auto& parent : parentsAll) {
        if (parent->isDerivedFrom<DrawPage>()) {
            page = static_cast<TechDraw::DrawPage *>(parent);
        } else if (parent->isDerivedFrom<DrawViewCollection>()) {
            collection = static_cast<TechDraw::DrawViewCollection *>(parent);
            page = collection->findParentPage();
        }

        if(page)
          break; // Found a page so leave
    }

    return page;
}


std::vector<DrawPage*> DrawView::findAllParentPages() const
{
    // Get Feature Page
    std::vector<DrawPage*> result;
    DrawPage *page = nullptr;
    DrawViewCollection *collection = nullptr;
    std::vector<App::DocumentObject*> parentsAll = getInList();

   for (auto& parent : parentsAll) {
        if (parent->isDerivedFrom<DrawPage>()) {
            page = static_cast<TechDraw::DrawPage*>(parent);
        } else if (parent->isDerivedFrom<DrawViewCollection>()) {
            collection = static_cast<TechDraw::DrawViewCollection *>(parent);
            page = collection->findParentPage();
        }

        if(page) {
            result.emplace_back(page);
        }
    }

    //prune the duplicates
    std::sort(result.begin(), result.end());
    auto last = std::unique(result.begin(), result.end());
    result.erase(last, result.end());

    return result;
}

bool DrawView::isInClip()
{
    std::vector<App::DocumentObject*> parent = getInList();
    for (std::vector<App::DocumentObject*>::iterator it = parent.begin(); it != parent.end(); ++it) {
        if ((*it)->isDerivedFrom<DrawViewClip>()) {
            return true;
        }
    }
    return false;
}

DrawView *DrawView::claimParent() const
{
    App::PropertyLink *ownerProp = const_cast<DrawView *>(this)->getOwnerProperty();
    if (ownerProp) {
        auto ownerView = dynamic_cast<DrawView *>(ownerProp->getValue());
        if (ownerView) {
            return ownerView;
        }
    }

    // If there is no parent view we are aware of, return the view collection we may belong to
    return getCollection();
}

DrawViewClip* DrawView::getClipGroup()
{
    std::vector<App::DocumentObject*> parent = getInList();
    App::DocumentObject* obj = nullptr;
    for (std::vector<App::DocumentObject*>::iterator it = parent.begin(); it != parent.end(); ++it) {
        if ((*it)->isDerivedFrom<DrawViewClip>()) {
            obj = (*it);
            DrawViewClip* result = dynamic_cast<DrawViewClip*>(obj);
            return result;

        }
    }
    return nullptr;
}

DrawViewCollection *DrawView::getCollection() const
{
    std::vector<App::DocumentObject *> parents = getInList();
    for (auto it = parents.begin(); it != parents.end(); ++it) {
        auto parentCollection = dynamic_cast<DrawViewCollection *>(*it);
        if (parentCollection) {
            return parentCollection;
        }
    }

    return nullptr;
}

double DrawView::autoScale() const
{
    auto page = findParentPage();
    double w = page->getPageWidth();
    double h = page->getPageHeight();
    return autoScale(w, h);
}

//compare 1:1 rect of view to pagesize(pw, h)
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
    double newScale = std::min(xScale, yScale) * fudgeFactor;
    double sensibleScale = DrawUtil::sensibleScale(newScale);
    return sensibleScale;
}

bool DrawView::checkFit() const
{
//    Base::Console().Message("DV::checkFit() - %s\n", getNameInDocument());
    auto page = findParentPage();
    return checkFit(page);
}

//!check if View is too big for page
bool DrawView::checkFit(TechDraw::DrawPage* p) const
{
//    Base::Console().Message("DV::checkFit(page) - %s\n", getNameInDocument());
    bool result = true;
    double fudge = 1.1;

    double width = 0.0;
    double height = 0.0;
    QRectF viewBox = getRect();         //rect is scaled
    if (!viewBox.isValid()) {
        result = true;
    } else {
        width = viewBox.width();        //scaled rect w x h
        height = viewBox.height();
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
//    Base::Console().Message("DV::setPosition(%.3f, %.3f) - \n", x,y, getNameInDocument());
    if ( (!isLocked()) ||
         (force) ) {
        double currX = X.getValue();
        double currY = Y.getValue();
        if (!DrawUtil::fpCompare(currX, x, 0.001)) {    // 0.001mm tolerance
            X.setValue(x);
        }
        if (!DrawUtil::fpCompare(currY, y, 0.001)) {
            Y.setValue(y);
        }
    }
}

double DrawView::getScale() const
{
    auto result = Scale.getValue();
    if (ScaleType.isValue("Page")) {
        auto page = findParentPage();
        if (page) {
            result = page->Scale.getValue();
        }
    }
    if (!(result > 0.0)) {
        result = 1.0;
    }
    return result;
}

//return list of Leaders which reference this DV
std::vector<TechDraw::DrawLeaderLine*> DrawView::getLeaders() const
{
    std::vector<TechDraw::DrawLeaderLine*> result;
    std::vector<App::DocumentObject*> children = getInList();
    for (std::vector<App::DocumentObject*>::iterator it = children.begin(); it != children.end(); ++it) {
        if ((*it)->isDerivedFrom<DrawLeaderLine>()) {
            TechDraw::DrawLeaderLine* lead = dynamic_cast<TechDraw::DrawLeaderLine*>(*it);
            result.push_back(lead);
        }
    }
    return result;
}

void DrawView::handleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, App::Property * prop)
{
    if (prop == &Scale) {
        App::PropertyFloat tmp;
        if (strcmp(tmp.getTypeId().getName(), TypeName)==0) {                   //property in file is Float
            tmp.setContainer(this);
            tmp.Restore(reader);
            double tmpValue = tmp.getValue();
            if (tmpValue > 0.0) {
                Scale.setValue(tmpValue);
            } else {
                Scale.setValue(1.0);
            }
        }
    }
    else if (prop->isDerivedFrom(App::PropertyLinkList::getClassTypeId())
        && strcmp(prop->getName(), "Source") == 0) {
        App::PropertyLinkGlobal glink;
        App::PropertyLink link;
        if (strcmp(glink.getTypeId().getName(), TypeName) == 0) {            //property in file is plg
            glink.setContainer(this);
            glink.Restore(reader);
            if (glink.getValue()) {
                static_cast<App::PropertyLinkList*>(prop)->setScope(App::LinkScope::Global);
                static_cast<App::PropertyLinkList*>(prop)->setValue(glink.getValue());
            }
        }
        else if (strcmp(link.getTypeId().getName(), TypeName) == 0) {            //property in file is pl
            link.setContainer(this);
            link.Restore(reader);
            if (link.getValue()) {
                static_cast<App::PropertyLinkList*>(prop)->setScope(App::LinkScope::Global);
                static_cast<App::PropertyLinkList*>(prop)->setValue(link.getValue());
            }
        }
    }

    // property X had App::PropertyFloat and was changed to App::PropertyLength
    // and later to PropertyDistance because some X, Y are relative to existing points on page
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

bool DrawView::keepUpdated()
{
//    Base::Console().Message("DV::keepUpdated() - %s\n", getNameInDocument());
    if (overrideKeepUpdated()) {
        return true;
    }
    TechDraw::DrawPage *page = findParentPage();
    if(page) {
        return (page->canUpdate() || page->forceRedraw());
    }
    return false;
}

void DrawView::setScaleAttribute()
{
    if (ScaleType.isValue("Page") ||
        ScaleType.isValue("Automatic")) {
        Scale.setStatus(App::Property::ReadOnly, true);
    } else {
        Scale.setStatus(App::Property::ReadOnly, false);
    }
}

int DrawView::prefScaleType()
{
    return Preferences::getPreferenceGroup("General")->GetInt("DefaultScaleType", 0);
}

double DrawView::prefScale()
{
    if (ScaleType.isValue("Page")) {
        auto page = findParentPage();
        if (page) {
            return page->Scale.getValue();
        }
    }
    return Preferences::getPreferenceGroup("General")->GetFloat("DefaultViewScale", 1.0);
}

void DrawView::requestPaint()
{
//    Base::Console().Message("DV::requestPaint() - %s\n", getNameInDocument());
    signalGuiPaint(this);
}

void DrawView::showProgressMessage(std::string featureName, std::string text)
{
    if (Preferences::reportProgress()) {
        signalProgressMessage(this, featureName, text);
    }
}

//! get a translated label string from the context (ex TaskActiveView), the base name (ex ActiveView) and
//! the unique name within the document (ex ActiveView001), and use it to update the Label property.
void DrawView::translateLabel(std::string context, std::string baseName, std::string uniqueName)
{
//    Base::Console().Message("DV::translateLabel - context: %s baseName: %s uniqueName: %s\n",
//                            context.c_str(), baseName.c_str(), uniqueName.c_str());

    Label.setValue(DU::translateArbitrary(context, baseName, uniqueName));
//    Base::Console().Message("DV::translateLabel - new label: %s\n", Label.getValue());
}

PyObject *DrawView::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewPython, TechDraw::DrawView)
template<> const char* TechDraw::DrawViewPython::getViewProviderName() const {
    return "TechDrawGui::ViewProviderDrawingView";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawView>;
}
