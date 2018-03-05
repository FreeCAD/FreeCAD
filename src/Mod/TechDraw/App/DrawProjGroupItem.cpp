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
#include <Base/Console.h>
#include <Base/Writer.h>

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
                                             NULL};


PROPERTY_SOURCE(TechDraw::DrawProjGroupItem, TechDraw::DrawViewPart)

DrawProjGroupItem::DrawProjGroupItem(void)
{
    Type.setEnums(TypeEnums);
    ADD_PROPERTY(Type, ((long)0));
    ADD_PROPERTY_TYPE(RotationVector ,(1.0,0.0,0.0)    ,"Base",App::Prop_None,"Controls rotation of item in view. ");

    //projection group controls these
    Direction.setStatus(App::Property::ReadOnly,true);
    RotationVector.setStatus(App::Property::ReadOnly,true);
    Scale.setStatus(App::Property::Hidden,true);
    ScaleType.setStatus(App::Property::Hidden,true);
}

short DrawProjGroupItem::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result  =  (Direction.isTouched()  ||
                    RotationVector.isTouched() ||
                    Source.isTouched()  );
    }

    if (result) {
        return result;
    }
    return TechDraw::DrawViewPart::mustExecute();
}

void DrawProjGroupItem::onChanged(const App::Property *prop)
{
    TechDraw::DrawViewPart::onChanged(prop);

}

DrawProjGroupItem::~DrawProjGroupItem()
{
}

App::DocumentObjectExecReturn *DrawProjGroupItem::execute(void)
{
    App::DocumentObjectExecReturn * ret = DrawViewPart::execute();
    delete ret;

    autoPosition();
    requestPaint();

    return App::DocumentObject::StdReturn;
}

void DrawProjGroupItem::autoPosition()
{
    auto pgroup = getPGroup();
    Base::Vector3d newPos;
    if ((pgroup != nullptr) && 
        (pgroup->AutoDistribute.getValue()) &&
        (!LockPosition.getValue())) {
        newPos = pgroup->getXYPosition(Type.getValueAsString());
        X.setValue(newPos.x);
        Y.setValue(newPos.y);
    }
}

void DrawProjGroupItem::onDocumentRestored()
{
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
gp_Ax2 DrawProjGroupItem::getViewAxis(const Base::Vector3d& pt,
                                 const Base::Vector3d& axis, 
                                 const bool flip) const
{
     gp_Ax2 viewAxis;
     Base::Vector3d x = RotationVector.getValue();
     Base::Vector3d nx = x;
     x.Normalize();
     Base::Vector3d na = axis;
     na.Normalize();
     viewAxis = TechDrawGeometry::getViewAxis(pt,axis,flip);        //default orientation

     if (!DrawUtil::checkParallel(nx,na)) {                         //!parallel/antiparallel
         viewAxis = TechDrawGeometry::getViewAxis(pt,axis,x,flip);
     }
     return viewAxis;
}

//obs??
//get the angle between the current RotationVector vector and the original X dir angle
double DrawProjGroupItem::getRotateAngle()
{
    gp_Ax2 viewAxis;
    Base::Vector3d x = RotationVector.getValue();   //current rotation
    Base::Vector3d nx = x;
    nx.Normalize();
    Base::Vector3d na = Direction.getValue();
    na.Normalize();
    Base::Vector3d org(0.0,0.0,0.0);

    viewAxis = TechDrawGeometry::getViewAxis(org,na,true);        //default orientation

    gp_Dir gxDir = viewAxis.XDirection();
    Base::Vector3d origX(gxDir.X(),gxDir.Y(),gxDir.Z());
    origX.Normalize();
    double dot = fabs(origX.Dot(nx));  
    double angle = acos(dot);

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
        result = pgroup->Scale.getValue();
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

PyObject *DrawProjGroupItem::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawProjGroupItemPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
