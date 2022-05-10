/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>

#include "DrawPage.h"
#include "DrawView.h"
#include "DrawUtil.h"

#include <Mod/TechDraw/App/DrawLeaderLinePy.h>  // generated from DrawLeaderLinePy.xml
#include "DrawLeaderLine.h"
#include "ArrowPropEnum.h"

using namespace TechDraw;

//===========================================================================
// DrawLeaderLine - Base class for drawing leader based features
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawLeaderLine, TechDraw::DrawView)

//TODO: share this between DrawViewBalloon, DrawLeaderLine, QGIArrow, Prefs, etc
//const char* DrawLeaderLine::ArrowTypeEnums[]= {
//                               "FILLED_ARROW",
//                               "OPEN_ARROW",
//                               "TICK",
//                               "DOT",
//                               "OPEN_CIRCLE",
//                               "FORK",
//                               "FILLED_TRIANGLE",
//                               "NONE"
//                               NULL};
//const char* DrawLeaderLine::ArrowTypeEnums2[]= {
//                               "FILLED_ARROW",
//                               "OPEN_ARROW",
//                               "TICK",
//                               "DOT",
//                               "OPEN_CIRCLE",
//                               "FORK",
//                               "FILLED_TRIANGLE",
//                               "NONE"
//                               NULL};

DrawLeaderLine::DrawLeaderLine(void)
{
    static const char *group = "Leader";

    ADD_PROPERTY_TYPE(LeaderParent,(nullptr),group,(App::PropertyType)(App::Prop_None),
                      "View to which this leader is attached");
    LeaderParent.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(WayPoints,(Base::Vector3d()) ,group, App::Prop_None,
                      "Intermediate points for Leader line");

//    EndType.setEnums(ArrowTypeEnums);
//    ADD_PROPERTY(EndType,(prefEnd()));

    StartSymbol.setEnums(ArrowPropEnum::ArrowTypeEnums);
    ADD_PROPERTY(StartSymbol,(0l));              //filled arrow

//    ADD_PROPERTY_TYPE(StartSymbol, (0), group, App::Prop_None, "Symbol (arrowhead) for start of line");
    EndSymbol.setEnums(ArrowPropEnum::ArrowTypeEnums);
    ADD_PROPERTY(EndSymbol,(7l));                //no symbol
//    ADD_PROPERTY_TYPE(EndSymbol, (0), group, App::Prop_None, "Symbol (arrowhead) for end of line");


    ADD_PROPERTY_TYPE(Scalable ,(false),group,App::Prop_None,"Scale line with LeaderParent");
    ADD_PROPERTY_TYPE(AutoHorizontal ,(getDefAuto()),group,App::Prop_None,"Forces last line segment to be horizontal");

    //hide the DrawView properties that don't apply to Leader
    ScaleType.setStatus(App::Property::ReadOnly,true);
    ScaleType.setStatus(App::Property::Hidden,true);
    Scale.setStatus(App::Property::ReadOnly,true);
    Scale.setStatus(App::Property::Hidden,true);
    Rotation.setStatus(App::Property::ReadOnly,true);
    Rotation.setStatus(App::Property::Hidden,true);
    Caption.setStatus(App::Property::Hidden,true);

    LockPosition.setValue(true);
    LockPosition.setStatus(App::Property::Hidden,true);
}

DrawLeaderLine::~DrawLeaderLine()
{
}

void DrawLeaderLine::onChanged(const App::Property* prop)
{
    DrawView::onChanged(prop);
}

short DrawLeaderLine::mustExecute() const
{
    bool result = 0;
    if (!isRestoring()) {
        result =  (LeaderParent.isTouched());          //Property changed
    }
    if (result) {
        return result;
    }

    const App::DocumentObject* docObj = getBaseObject();
    if (docObj != nullptr) {
        result = docObj->isTouched();                 //object property points to is touched
    }
    if (result) {
        return result;
    }

    return DrawView::mustExecute();
}

App::DocumentObjectExecReturn *DrawLeaderLine::execute(void)
{
//    Base::Console().Message("DLL::execute()\n");
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }
    adjustLastSegment();
    return DrawView::execute();
}

DrawView* DrawLeaderLine::getBaseView(void) const
{
    DrawView* result = nullptr;
    App::DocumentObject* baseObj = LeaderParent.getValue();
    if (baseObj != nullptr) {
        DrawView* cast = dynamic_cast<DrawView*>(baseObj);
        if (cast != nullptr) {
            result = cast;
        }
    }
    return result;
}

App::DocumentObject* DrawLeaderLine::getBaseObject(void) const
{
    App::DocumentObject* result = nullptr;
    DrawView* view = getBaseView();
    if (view != nullptr) {
        result = view;
    }
    return result;
}

bool DrawLeaderLine::keepUpdated(void)
{
    bool result = false;
    DrawView* view = getBaseView();
    if (view != nullptr) {
        result = view->keepUpdated();
    }
    return result;
}

//need separate getParentScale()???

double DrawLeaderLine::getBaseScale(void) const
{
//    Base::Console().Message("DLL::getBaseScale()\n");
    double result = 1.0;
    DrawView* parent = getBaseView();
    if (parent != nullptr) {
        result = parent->getScale();
    } else {
        //TARFU
        Base::Console().Log("DrawLeaderLine - %s - scale not found.  Using 1.0. \n", getNameInDocument());
    }
    return result;
}

double DrawLeaderLine::getScale(void) const
{
//    Base::Console().Message("DLL::getScale()\n");
    double result = 1.0;
    if (Scalable.getValue()) {
        DrawView* parent = getBaseView();
        if (parent != nullptr) {
            result = parent->getScale();
        } else {
            //TARFU
            Base::Console().Log("DrawLeaderLine - %s - scale not found.  Using 1.0. \n", getNameInDocument());
        }
    }
    return result;
}

Base::Vector3d DrawLeaderLine::getAttachPoint(void)
{
    Base::Vector3d result(X.getValue(),
                          Y.getValue(),
                          0.0);
    return result;
}

void DrawLeaderLine::adjustLastSegment(void)
{
//    Base::Console().Message("DLL::adjustLastSegment()\n");
    bool adjust = AutoHorizontal.getValue();
    std::vector<Base::Vector3d> wp = WayPoints.getValues();
    if (adjust) {
        if (wp.size() > 1) {
            int iLast = wp.size() - 1;
            int iPen  = wp.size() - 2;
            Base::Vector3d last = wp.at(iLast);
            Base::Vector3d penUlt = wp.at(iPen);
            last.y = penUlt.y;
            wp.at(iLast) = last;
        }
    }
    WayPoints.setValues(wp);
}

//middle of last line segment
Base::Vector3d DrawLeaderLine::getTileOrigin(void) const
{
    Base::Vector3d result;
    std::vector<Base::Vector3d> wp = WayPoints.getValues();
    if (wp.size() > 1) {
        Base::Vector3d last = wp.rbegin()[0];
        Base::Vector3d second = wp.rbegin()[1];
        result = (last + second) / 2.0;
    } else {
        Base::Console().Warning("DLL::getTileOrigin - no waypoints\n");
    }
    return result;
}

//start of last line segment
Base::Vector3d DrawLeaderLine::getKinkPoint(void) const
{
    Base::Vector3d result;
    std::vector<Base::Vector3d> wp = WayPoints.getValues();
    if (wp.size() > 1) {
        Base::Vector3d second = wp.rbegin()[1];
        result = second;
    } else {
        Base::Console().Warning("DLL::getKinkPoint - no waypoints\n");
    }

    return result;
}

//end of last line segment
Base::Vector3d DrawLeaderLine::getTailPoint(void) const
{
    Base::Vector3d result;
    std::vector<Base::Vector3d> wp = WayPoints.getValues();
    if (!wp.empty()) {
        Base::Vector3d last = wp.rbegin()[0];
        result = last;
    } else {
        Base::Console().Warning("DLL::getTailPoint - no waypoints\n");
    }

    return result;
}


bool DrawLeaderLine::getDefAuto(void) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/LeaderLines");
    bool result = hGrp->GetBool("AutoHorizontal",true);
    return result;
}


PyObject *DrawLeaderLine::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawLeaderLinePy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawLeaderLinePython, TechDraw::DrawLeaderLine)
template<> const char* TechDraw::DrawLeaderLinePython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderLeader";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawLeaderLine>;
}

