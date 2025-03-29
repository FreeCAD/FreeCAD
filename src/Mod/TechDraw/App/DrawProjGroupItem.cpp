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
# include <gp_Ax2.hxx>
#endif

#include <App/DocumentObject.h>
#include <Base/Console.h>

#include "DrawProjGroupItem.h"
#include "DrawProjGroupItemPy.h" // generated from DrawProjGroupItemPy.xml
#include "DrawPage.h"
#include "DrawProjGroup.h"
#include "DrawUtil.h"
#include "GeometryObject.h"


using namespace TechDraw;

const char *DrawProjGroupItem::TypeEnums[] = {
    QT_TRANSLATE_NOOP("DrawProjGroupItem", "Front"),
    QT_TRANSLATE_NOOP("DrawProjGroupItem", "Left"),
    QT_TRANSLATE_NOOP("DrawProjGroupItem", "Right"),
    QT_TRANSLATE_NOOP("DrawProjGroupItem", "Rear"),
    QT_TRANSLATE_NOOP("DrawProjGroupItem", "Top"),
    QT_TRANSLATE_NOOP("DrawProjGroupItem", "Bottom"),
    QT_TRANSLATE_NOOP("DrawProjGroupItem", "FrontTopLeft"),
    QT_TRANSLATE_NOOP("DrawProjGroupItem", "FrontTopRight"),
    QT_TRANSLATE_NOOP("DrawProjGroupItem", "FrontBottomLeft"),
    QT_TRANSLATE_NOOP("DrawProjGroupItem", "FrontBottomRight"),
    nullptr};

PROPERTY_SOURCE(TechDraw::DrawProjGroupItem, TechDraw::DrawViewPart)

DrawProjGroupItem::DrawProjGroupItem()
{
    Type.setEnums(TypeEnums);
    ADD_PROPERTY(Type, ((long)0));
    ADD_PROPERTY_TYPE(RotationVector ,(1.0, 0.0, 0.0)    ,"Base",
            App::Prop_None, "Deprecated. Use XDirection.");

    //projection group controls these
//    Direction.setStatus(App::Property::ReadOnly, true);
    RotationVector.setStatus(App::Property::ReadOnly, true);   //Use XDirection
    if (getPGroup()) {
        ScaleType.setValue("Custom");
        Scale.setStatus(App::Property::Hidden, true);
        ScaleType.setStatus(App::Property::Hidden, true);
    }
}

void DrawProjGroupItem::onChanged(const App::Property *prop)
{
    if ((prop == &X) || (prop == &Y)) {
        DrawProjGroup* pGroup = getPGroup();
        if (pGroup) {
            pGroup->touch(false);
        }
    }
    TechDraw::DrawViewPart::onChanged(prop);
}

bool DrawProjGroupItem::isLocked() const
{
    if (isAnchor()) {                             //Anchor view is always locked to DPG
        return true;
    }
    return DrawView::isLocked();
}

bool DrawProjGroupItem::showLock() const
{
    DrawProjGroup* parent = getPGroup();
    bool parentLock = false;
    if (parent) {
        parentLock = parent->LockPosition.getValue();
    }
    //don't show lock for Front if DPG is not locked
    if (isAnchor() && !parentLock) {
        return false;
    }

    return DrawView::showLock();
}

App::DocumentObjectExecReturn *DrawProjGroupItem::execute()
{
//    Base::Console().message("DPGI::execute() - %s / %s\n", getNameInDocument(), Label.getValue());
    if (!keepUpdated()) {
        return DrawView::execute();
    }

    if (waitingForHlr()) {
        return DrawView::execute();
    }

    bool haveX = checkXDirection();
    if (!haveX) {
        //block touch/onChanged stuff
        Base::Vector3d newX = getXDirection();
        XDirection.setValue(newX);
        XDirection.purgeTouched();  //don't trigger updates!
        //unblock
    }

    if (DrawUtil::checkParallel(Direction.getValue(), getXDirection())) {
        return new App::DocumentObjectExecReturn("DPGI: Direction and XDirection are parallel");
    }

    return DrawViewPart::execute();
}

void DrawProjGroupItem::postHlrTasks()
{
//    Base::Console().message("DPGI::postHlrTasks() - %s\n", getNameInDocument());
    DrawViewPart::postHlrTasks();

    DrawProjGroup* pGroup = getPGroup();
    if (pGroup) {
        //DPGI has no geometry until HLR has finished, and the DPG can not properly
        //AutoDistibute until all its items have geometry.
        autoPosition();

        pGroup->reportReady();     //tell the parent DPG we are ready
    }
}

void DrawProjGroupItem::autoPosition()
{
    DrawProjGroup* pGroup = getPGroup();
    if (!pGroup) {
        return;
    }
//    Base::Console().message("DPGI::autoPosition(%s)\n", Label.getValue());
    if (LockPosition.getValue()) {
        return;
    }
    Base::Vector3d newPos;
    if (pGroup && pGroup->AutoDistribute.getValue()) {
        newPos = pGroup->getXYPosition(Type.getValueAsString());
        X.setValue(newPos.x);
        Y.setValue(newPos.y);
        requestPaint();
        purgeTouched();               //prevents "still touched after recompute" message
        pGroup->purgeTouched();  //changing dpgi x, y marks parent dpg as touched
    }
}

void DrawProjGroupItem::onDocumentRestored()
{
//    Base::Console().message("DPGI::onDocumentRestored() - %s\n", getNameInDocument());
    DrawView::onDocumentRestored();
    App::DocumentObjectExecReturn* rc = DrawProjGroupItem::execute();
    if (rc) {
        delete rc;
    }
}

DrawProjGroup* DrawProjGroupItem::getPGroup() const
{
    return dynamic_cast<DrawProjGroup *>(getCollection());
}

bool DrawProjGroupItem::isAnchor() const
{
    return getPGroup() && (getPGroup()->getAnchor() == this);
}

Base::Vector3d DrawProjGroupItem::getXDirection() const
{
//    Base::Console().message("DPGI::getXDirection() - %s\n", Label.getValue());
    Base::Vector3d result(1.0, 0.0, 0.0);               //default X
    App::Property* prop = getPropertyByName("XDirection");
    if (prop) {
        Base::Vector3d propVal = XDirection.getValue();
        if (DrawUtil::fpCompare(propVal.Length(), 0.0))  {  //have XDirection property, but not set
            prop = getPropertyByName("RotationVector");
            if (prop) {
                result = RotationVector.getValue();         //use RotationVector if we have it
            }
            else {
                result = DrawViewPart::getXDirection();     //over complex.
            }
        }
        else {
            result = DrawViewPart::getXDirection();
        }
    }
    else {                                     //not sure this branch can actually happen
        Base::Console().message("DPGI::getXDirection - unexpected branch taken!\n");
        prop = getPropertyByName("RotationVector");
        if (prop) {
            result = RotationVector.getValue();

        }
        else {
            Base::Console().message("DPGI::getXDirection - missing RotationVector and XDirection\n");
        }
    }
    return result;
}

Base::Vector3d DrawProjGroupItem::getLegacyX(const Base::Vector3d& pt,
                                        const Base::Vector3d& axis,
                                        const bool flip)  const
{
//    Base::Console().message("DPGI::getLegacyX() - %s\n", Label.getValue());
    App::Property* prop = getPropertyByName("RotationVector");
    if (prop) {
        Base::Vector3d result = RotationVector.getValue();
        if (DrawUtil::fpCompare(result.Length(), 0.0))  {  //have RotationVector property, but not set
            gp_Ax2 va = getViewAxis(pt,
                                    axis,
                                    flip);
            gp_Dir gXDir = va.XDirection();
            return Base::Vector3d(gXDir.X(),
                                  gXDir.Y(),
                                  gXDir.Z());
        }
        return result;
    }

    gp_Ax2 va = getViewAxis(pt,
                            axis,
                            flip);
    gp_Dir gXDir = va.XDirection();
    return Base::Vector3d(gXDir.X(),
                            gXDir.Y(),
                            gXDir.Z());
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
    Base::Vector3d org(0.0, 0.0, 0.0);

    viewAxis = getProjectionCS(org);
    gp_Dir gxDir = viewAxis.XDirection();
    Base::Vector3d origX(gxDir.X(), gxDir.Y(), gxDir.Z());
    origX.Normalize();
    double angle = origX.GetAngle(nx);

    Base::Vector3d rotAxis = origX.Cross(nx);
    if (rotAxis == Direction.getValue()) {
        angle *= -1.0;
    }
    return angle;
}

double DrawProjGroupItem::getScale() const
{
    auto pgroup = getPGroup();
    if (pgroup) {
        double result = pgroup->getScale();
        if (!(result > 0.0)) {
            return 1.0;                                   //kludgy protective fix. autoscale sometimes serves up 0.0!
        }
        return result;
    }
    return Scale.getValue();
}

int DrawProjGroupItem::getScaleType() const
{
    auto pgroup = getPGroup();
    if (pgroup) {
        return pgroup->getScaleType();
    }

    return ScaleType.getValue();
}

void DrawProjGroupItem::unsetupObject()
{
    if (!getPGroup()) {
        DrawViewPart::unsetupObject();
        return;
    }

    if (!getPGroup()->hasProjection(Type.getValueAsString())) {
        DrawViewPart::unsetupObject();
        return;
    }

    if (getPGroup()->getAnchor() == this && !getPGroup()->isUnsetting()) {
           Base::Console().warning("Warning - DPG (%s/%s) may be corrupt - Anchor deleted\n",
                                   getPGroup()->getNameInDocument(), getPGroup()->Label.getValue());
           getPGroup()->Anchor.setValue(nullptr);    //this catches situation where DPGI is deleted w/o DPG::removeProjection
    }

    DrawViewPart::unsetupObject();
}

//DPGIs have DPG as parent, not Page, so we need to ask the DPG how many Pages own it.
int DrawProjGroupItem::countParentPages() const
{
    DrawProjGroup* dpg = getPGroup();
    if (dpg) {
        return dpg->countParentPages();
    }
    return DrawView::countParentPages();
}

DrawPage* DrawProjGroupItem::findParentPage() const
{
    DrawProjGroup* dpg = getPGroup();
    if (dpg) {
        return dpg->findParentPage();
    }
    return DrawView::findParentPage();
}

std::vector<DrawPage*> DrawProjGroupItem::findAllParentPages() const
{
    DrawProjGroup* dpg = getPGroup();
    if (dpg) {
        return dpg->findAllParentPages();
    }
    return DrawView::findAllParentPages();
}

PyObject *DrawProjGroupItem::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawProjGroupItemPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
