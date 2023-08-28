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

#include <Base/Console.h>
#include <Base/Parameter.h>

#include "DrawLeaderLine.h"
#include "DrawLeaderLinePy.h"  // generated from DrawLeaderLinePy.xml
#include "ArrowPropEnum.h"
#include "DrawView.h"
#include "Preferences.h"


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

DrawLeaderLine::DrawLeaderLine()
{
    static const char *group = "Leader";

    ADD_PROPERTY_TYPE(LeaderParent, (nullptr), group, (App::PropertyType)(App::Prop_None),
                      "View to which this leader is attached");
    LeaderParent.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(WayPoints, (Base::Vector3d()) ,group, App::Prop_None,
                      "Intermediate points for Leader line");

//    EndType.setEnums(ArrowTypeEnums);
//    ADD_PROPERTY(EndType, (prefEnd()));

    StartSymbol.setEnums(ArrowPropEnum::ArrowTypeEnums);
    ADD_PROPERTY(StartSymbol, (0l));              //filled arrow

//    ADD_PROPERTY_TYPE(StartSymbol, (0), group, App::Prop_None, "Symbol (arrowhead) for start of line");
    EndSymbol.setEnums(ArrowPropEnum::ArrowTypeEnums);
    ADD_PROPERTY(EndSymbol, (7l));                //no symbol
//    ADD_PROPERTY_TYPE(EndSymbol, (0), group, App::Prop_None, "Symbol (arrowhead) for end of line");


    ADD_PROPERTY_TYPE(Scalable ,(false), group, App::Prop_None, "Scale line with LeaderParent");
    ADD_PROPERTY_TYPE(AutoHorizontal ,(getDefAuto()), group, App::Prop_None, "Forces last line segment to be horizontal");

    //hide the DrawView properties that don't apply to Leader
    ScaleType.setStatus(App::Property::ReadOnly, true);
    ScaleType.setStatus(App::Property::Hidden, true);
    Scale.setStatus(App::Property::ReadOnly, true);
    Scale.setStatus(App::Property::Hidden, true);
    Rotation.setStatus(App::Property::ReadOnly, true);
    Rotation.setStatus(App::Property::Hidden, true);
    Caption.setStatus(App::Property::Hidden, true);

    LockPosition.setValue(true);
    LockPosition.setStatus(App::Property::Hidden, true);
}

void DrawLeaderLine::onChanged(const App::Property* prop)
{
    DrawView::onChanged(prop);
}

short DrawLeaderLine::mustExecute() const
{
    if (!isRestoring() && LeaderParent.isTouched()) {
        return true;  // Property changed
    }

    const App::DocumentObject* docObj = getBaseObject();
    if (docObj && docObj->isTouched()) {
        return true;  // Object property points to is touched
    }

    return DrawView::mustExecute();
}

App::DocumentObjectExecReturn *DrawLeaderLine::execute()
{
//    Base::Console().Message("DLL::execute()\n");
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }
    adjustLastSegment();
    overrideKeepUpdated(false);
    return DrawView::execute();
}

DrawView* DrawLeaderLine::getBaseView() const
{
    App::DocumentObject* baseObj = LeaderParent.getValue();
    if (!baseObj) {
        return nullptr;
    }

    DrawView* cast = dynamic_cast<DrawView*>(baseObj);
    return cast;
}

App::DocumentObject* DrawLeaderLine::getBaseObject() const
{
    return getBaseView();
}

bool DrawLeaderLine::keepUpdated()
{
    DrawView* view = getBaseView();
    if (!view) {
        return false;
    }
    return view->keepUpdated();
}

//need separate getParentScale()???

double DrawLeaderLine::getBaseScale() const
{
//    Base::Console().Message("DLL::getBaseScale()\n");
    DrawView* parent = getBaseView();
    if (!parent) {
        return 1.0;
    }
    return parent->getScale();
}

double DrawLeaderLine::getScale() const
{
//    Base::Console().Message("DLL::getScale()\n");
    if (!Scalable.getValue()) {
        return 1.0;
    }

    DrawView* parent = getBaseView();
    if (!parent) {
        return 1.0;
    }
    return parent->getScale();
}

Base::Vector3d DrawLeaderLine::getAttachPoint()
{
    return Base::Vector3d(
        X.getValue(),
        Y.getValue(),
        0.0
    );
}

void DrawLeaderLine::adjustLastSegment()
{
//    Base::Console().Message("DLL::adjustLastSegment()\n");
    bool adjust = AutoHorizontal.getValue();
    std::vector<Base::Vector3d> wp = WayPoints.getValues();
    if (adjust && wp.size() > 1) {
        int iLast = wp.size() - 1;
        int iPen  = wp.size() - 2;
        Base::Vector3d last = wp.at(iLast);
        Base::Vector3d penUlt = wp.at(iPen);
        last.y = penUlt.y;
        wp.at(iLast) = last;
    }
    WayPoints.setValues(wp);
}

//middle of last line segment
Base::Vector3d DrawLeaderLine::getTileOrigin() const
{
    std::vector<Base::Vector3d> wp = WayPoints.getValues();
    if (wp.size() > 1) {
        Base::Vector3d last = wp.rbegin()[0];
        Base::Vector3d second = wp.rbegin()[1];
        return (last + second) / 2.0;
    }
    
    Base::Console().Warning("DLL::getTileOrigin - no waypoints\n");
    return Base::Vector3d();
}

//start of last line segment
Base::Vector3d DrawLeaderLine::getKinkPoint() const
{
    std::vector<Base::Vector3d> wp = WayPoints.getValues();
    if (wp.size() > 1) {
        return wp.rbegin()[1];  // Second
    }

    Base::Console().Warning("DLL::getKinkPoint - no waypoints\n");
    return Base::Vector3d();
}

//end of last line segment
Base::Vector3d DrawLeaderLine::getTailPoint() const
{
    std::vector<Base::Vector3d> wp = WayPoints.getValues();
    if (!wp.empty()) {
        return wp.rbegin()[0];  // Last
    }
    
    Base::Console().Warning("DLL::getTailPoint - no waypoints\n");
    return Base::Vector3d();
}


bool DrawLeaderLine::getDefAuto() const
{
    return Preferences::getPreferenceGroup("LeaderLine")->GetBool("AutoHorizontal", true);
}


PyObject *DrawLeaderLine::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawLeaderLinePy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawLeaderLinePython, TechDraw::DrawLeaderLine)
template<> const char* TechDraw::DrawLeaderLinePython::getViewProviderName() const {
    return "TechDrawGui::ViewProviderLeader";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawLeaderLine>;
}

