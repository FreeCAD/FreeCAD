/***************************************************************************
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
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
#endif

#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <gp_Trsf.hxx>

#include <App/Application.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>

#include "GeometryObject.h"
#include "DrawUtil.h"
#include "DrawPage.h"
#include "DrawProjGroup.h"
#include "DrawProjGroupItem.h"

#include <Mod/TechDraw/App/DrawProjGroupItemPy.h>  // generated from DrawProjGroupItemPy.xml


using namespace TechDraw;

const char* DrawProjGroupItem::TypeEnums[] = {"Front",
                                             "Left",
                                             "Right",
                                             "Rear",
                                             "Top",
                                             "Bottom",
                                             "FrontTopLeft",
                                             "FrontTopRight",
                                             "FrontBottomLeft",
                                             "FrontBottomRight",
                                             nullptr};


PROPERTY_SOURCE(TechDraw::DrawProjGroupItem, TechDraw::DrawViewPart)

DrawProjGroupItem::DrawProjGroupItem(void)
{
    Type.setEnums(TypeEnums);
    ADD_PROPERTY(Type, ((long)0));
    ADD_PROPERTY_TYPE(RotationVector ,(1.0,0.0,0.0)    ,"Base",
            App::Prop_None,"Deprecated. Use XDirection.");

    //projection group controls these
//    Direction.setStatus(App::Property::ReadOnly,true);
    RotationVector.setStatus(App::Property::ReadOnly,true);   //Use XDirection
    ScaleType.setValue("Custom");
    Scale.setStatus(App::Property::Hidden,true);
    ScaleType.setStatus(App::Property::Hidden,true);
}

DrawProjGroupItem::~DrawProjGroupItem()
{
}

short DrawProjGroupItem::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result  =  (Direction.isTouched()  ||
                    XDirection.isTouched() ||
                    Source.isTouched()  ||
                    XSource.isTouched()  ||
                    Scale.isTouched());
    }

    if (result) {
        return result;
    }
    return TechDraw::DrawViewPart::mustExecute();
}

void DrawProjGroupItem::onChanged(const App::Property *prop)
{
    if ((prop == &X) ||
        (prop == &Y)) {
        DrawProjGroup* parent = getPGroup();
        if (parent != nullptr) {
            parent->touch(false);
        }
    }
    TechDraw::DrawViewPart::onChanged(prop);
}

bool DrawProjGroupItem::isLocked(void) const
{
    if (isAnchor()) {                             //Anchor view is always locked to DPG
        return true;
    }
    return DrawView::isLocked();
}

bool DrawProjGroupItem::showLock(void) const
{
    bool result = DrawView::showLock();
    DrawProjGroup* parent = getPGroup();
    bool parentLock = false;
    if (parent != nullptr) {
        parentLock = parent->LockPosition.getValue();
    }

    if (isAnchor() &&                         //don't show lock for Front if DPG is not locked
        !parentLock) { 
        result = false;
    }   

    return result;
}

App::DocumentObjectExecReturn *DrawProjGroupItem::execute(void)
{
//    Base::Console().Message("DPGI::execute(%s)\n",Label.getValue());
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    bool haveX = checkXDirection();
    if (!haveX) {
        //block touch/onChanged stuff
        Base::Vector3d newX = getXDirection();
        XDirection.setValue(newX);
        XDirection.purgeTouched();  //don't trigger updates!
        //unblock
    }

    if (DrawUtil::checkParallel(Direction.getValue(),
                                getXDirection())) {
        return new App::DocumentObjectExecReturn("DPGI: Direction and XDirection are parallel");
    }

    App::DocumentObjectExecReturn* ret = DrawViewPart::execute();
    autoPosition();
    return ret;
}

void DrawProjGroupItem::autoPosition()
{
//    Base::Console().Message("DPGI::autoPosition(%s)\n",Label.getValue());
    if (LockPosition.getValue()) {
        return;
    }
    auto pgroup = getPGroup();
    Base::Vector3d newPos;
    if (pgroup != nullptr) {
        if (pgroup->AutoDistribute.getValue()) {
            newPos = pgroup->getXYPosition(Type.getValueAsString());
            X.setValue(newPos.x);
            Y.setValue(newPos.y);
            requestPaint();
            purgeTouched();               //prevents "still touched after recompute" message
        }
    }
}

void DrawProjGroupItem::onDocumentRestored()
{
//    Base::Console().Message("DPGI::onDocumentRestored() - %s\n", getNameInDocument());
    DrawView::onDocumentRestored();
    App::DocumentObjectExecReturn* rc = DrawProjGroupItem::execute();
    if (rc) {
        delete rc;
    }
}

DrawProjGroup* DrawProjGroupItem::getPGroup() const
{
    DrawProjGroup* result = nullptr;
    std::vector<App::DocumentObject*> parent = getInList();
    for (std::vector<App::DocumentObject*>::iterator it = parent.begin(); it != parent.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawProjGroup::getClassTypeId())) {
            result = dynamic_cast<TechDraw::DrawProjGroup *>(*it);
            break;
        }
    }
    return result;
}

bool DrawProjGroupItem::isAnchor(void) const
{
    bool result = false;
    auto group = getPGroup();
    if (group != nullptr) {
        DrawProjGroupItem* anchor = group->getAnchor();
        if (anchor == this) {
            result = true;
        }
    }
    return result;
}

/// get a coord system aligned with Direction and Rotation Vector
gp_Ax2 DrawProjGroupItem::getViewAxis(const Base::Vector3d& pt,
                                 const Base::Vector3d& axis, 
                                 const bool flip) const
{
    Base::Console().Message("DPGI::getViewAxis - deprecated. use getProjectionCS\n");
    (void) flip;
    gp_Ax2 viewAxis;
    Base::Vector3d projDir = Direction.getValue();
    Base::Vector3d rotVec  = getXDirection();

// mirror projDir through XZ plane
    Base::Vector3d yNorm(0.0,1.0,0.0);
    projDir = projDir - (yNorm * 2.0) * (projDir.Dot(yNorm));
    rotVec = rotVec - (yNorm * 2.0) * (rotVec.Dot(yNorm));

    if (DrawUtil::checkParallel(projDir,rotVec)) {
         Base::Console().Warning("DPGI::getVA - %s - Direction and XDirection parallel. using defaults\n",
                                 getNameInDocument());
    }
    try {
        viewAxis = gp_Ax2(gp_Pnt(pt.x,pt.y,pt.z),
                          gp_Dir(projDir.x, projDir.y, projDir.z),
                          gp_Dir(rotVec.x, rotVec.y, rotVec.z));
    }
    catch (Standard_Failure& e4) {
        Base::Console().Message("PROBLEM - DPGI (%s) failed to create viewAxis: %s **\n",
                                getNameInDocument(),e4.GetMessageString());
        return TechDraw::getViewAxis(pt,axis,false);
    }

    return viewAxis;
}

Base::Vector3d DrawProjGroupItem::getXDirection(void) const
{
//    Base::Console().Message("DPGI::getXDirection() - %s\n", Label.getValue());
    Base::Vector3d result(1.0, 0.0, 0.0);               //default X
    App::Property* prop = getPropertyByName("XDirection");
    if (prop != nullptr) {
        Base::Vector3d propVal = XDirection.getValue();
        if (DrawUtil::fpCompare(propVal.Length(), 0.0))  {  //have XDirection property, but not set
            prop = getPropertyByName("RotationVector");
            if (prop != nullptr) {
                result = RotationVector.getValue();         //use RotationVector if we have it
            } else {
                result = DrawViewPart::getXDirection();     //over complex.
            }
        } else {
            result = DrawViewPart::getXDirection();
        }
    } else {                                     //not sure this branch can actually happen
        Base::Console().Message("DPGI::getXDirection - unexpected branch taken!\n");
        prop = getPropertyByName("RotationVector");
        if (prop != nullptr) {
            result = RotationVector.getValue();
            
        } else {
            Base::Console().Message("DPGI::getXDirection - missing RotationVector and XDirection\n");
        }
    }
    return result;
}

Base::Vector3d DrawProjGroupItem::getLegacyX(const Base::Vector3d& pt,
                                        const Base::Vector3d& axis,
                                        const bool flip)  const
{
//    Base::Console().Message("DPGI::getLegacyX() - %s\n", Label.getValue());
    Base::Vector3d result(1.0, 0.0, 0.0);
    App::Property* prop = getPropertyByName("RotationVector");
    if (prop != nullptr) {
        result = RotationVector.getValue();
        if (DrawUtil::fpCompare(result.Length(), 0.0))  {  //have RotationVector property, but not set
            gp_Ax2 va = getViewAxis(pt,
                                    axis,
                                    flip);
            gp_Dir gXDir = va.XDirection();
            result = Base::Vector3d(gXDir.X(),
                                    gXDir.Y(),
                                    gXDir.Z());
        }
    } else {
        gp_Ax2 va = getViewAxis(pt,
                                axis,
                                flip);
        gp_Dir gXDir = va.XDirection();
        result = Base::Vector3d(gXDir.X(),
                                gXDir.Y(),
                                gXDir.Z());
    }
    return result;
}

//get the angle between the current RotationVector vector and the original X dir angle
double DrawProjGroupItem::getRotateAngle()
{
    gp_Ax2 viewAxis;
    Base::Vector3d x = getXDirection();   //current rotation
    Base::Vector3d nx = x;
    nx.Normalize();
    Base::Vector3d na = Direction.getValue();
    na.Normalize();
    Base::Vector3d org(0.0,0.0,0.0);

    viewAxis = getProjectionCS(org);
    gp_Dir gxDir = viewAxis.XDirection();
    Base::Vector3d origX(gxDir.X(),gxDir.Y(),gxDir.Z());
    origX.Normalize();
    double angle = origX.GetAngle(nx);

    Base::Vector3d rotAxis = origX.Cross(nx);
    if (rotAxis == Direction.getValue()) {
        angle *= -1.0;
    }
    return angle;
}

double DrawProjGroupItem::getScale(void) const
{
    double result = 1.0;
    auto pgroup = getPGroup();
    if (pgroup != nullptr) {
        result = pgroup->getScale();
        if (!(result > 0.0)) {
            Base::Console().Log("DPGI - %s - bad scale found (%.3f) using 1.0\n",getNameInDocument(),Scale.getValue());
            result = 1.0;                                   //kludgy protective fix. autoscale sometimes serves up 0.0!
        }
    }
    return result;
}

void DrawProjGroupItem::unsetupObject()
{
    if (getPGroup() != nullptr) {
        if (getPGroup()->hasProjection(Type.getValueAsString()) ) {
            if ((getPGroup()->getAnchor() == this) &&
                 !getPGroup()->isUnsetting() )         {
                   Base::Console().Warning("Warning - DPG (%s/%s) may be corrupt - Anchor deleted\n",
                                           getPGroup()->getNameInDocument(),getPGroup()->Label.getValue());
                   getPGroup()->Anchor.setValue(nullptr);    //this catches situation where DPGI is deleted w/o DPG::removeProjection
             }
        }
    }
    DrawViewPart::unsetupObject();
}

//DPGIs have DPG as parent, not Page, so we need to ask the DPG how many Pages own it.
int DrawProjGroupItem::countParentPages() const
{
    DrawProjGroup* dpg = getPGroup();
    if (dpg != nullptr) {
        int count = dpg->countParentPages();
        return count;
    }
    return 0;
}

DrawPage* DrawProjGroupItem::findParentPage() const
{
    DrawProjGroup* dpg = getPGroup();
    if (dpg != nullptr) {
        DrawPage* dp = dpg->findParentPage();
        return dp;
    }
    return nullptr;
}

std::vector<DrawPage*> DrawProjGroupItem::findAllParentPages() const
{
    DrawProjGroup* dpg = getPGroup();
    if (dpg != nullptr) {
        std::vector<DrawPage*> dps = dpg->findAllParentPages();
        return dps;
    }
    std::vector<DrawPage*> empty;
    return empty;
}

PyObject *DrawProjGroupItem::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawProjGroupItemPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
