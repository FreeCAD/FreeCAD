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
    ADD_PROPERTY_TYPE(OrientBasis ,(1.0,0.0,0.0)    ,"Base",App::Prop_None,"Controls rotary orientation of item in view. ");

    //projection group controls these
    Direction.setStatus(App::Property::ReadOnly,true);
    //OrientBasis.setStatus(App::Property::ReadOnly,true);
    Scale.setStatus(App::Property::ReadOnly,true);
    ScaleType.setStatus(App::Property::ReadOnly,true);
}

short DrawProjGroupItem::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result  =  (Direction.isTouched()  ||
                    OrientBasis.isTouched() ||
                    Source.isTouched()  ||
                    Scale.isTouched() ||
                    ScaleType.isTouched());
    }

    if (result) {
        return result;
    }
    return TechDraw::DrawViewPart::mustExecute();
}

void DrawProjGroupItem::onChanged(const App::Property *prop)
{
//    //TODO: too many executes!
//    //TODO: Should we allow changes to the Type here?  Seems that should be handled through DrawProjGroup
//    if (!isRestoring()) {
////        //Base::Console().Message("TRACE - DPGI::onChanged(%s) - %s/%s\n",prop->getName(),getNameInDocument(),Label.getValue());
//        if (prop == &Type && Type.isTouched()) {
////            //Base::Console().Message("TRACE - DPGI::onChanged(%s) - Type: %s\n",prop->getName(),Type.getValueAsString());
////            execute();
//        } else if (prop == &Direction) {
//            if (getGroup() != nullptr) {
//            OrientBasis.setValue(getGroup()->getXAxisDir(Type.getValueAsString()));
//            Base::Console().Message("TRACE - DPGI::onChanged(%s) - Direction: %s  Orient: %s\n",
//                                    prop->getName(),DrawUtil::formatVector(Direction.getValue()).c_str(),
//                                    DrawUtil::formatVector(OrientBasis.getValue()).c_str());
//            }
//        }
////            execute();
////        }  //else if (prop == &OrientBasis) {  //don't want to do twice though!
//    }

    TechDraw::DrawViewPart::onChanged(prop);

}

DrawProjGroupItem::~DrawProjGroupItem()
{
}

void DrawProjGroupItem::onDocumentRestored()
{
//    setAutoPos(false);                        //if restoring from file, use X,Y from file, not auto!
    App::DocumentObjectExecReturn* rc = DrawProjGroupItem::execute();
    if (rc) {
        delete rc;
    }
}

DrawProjGroup* DrawProjGroupItem::getGroup() const
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
//     Base::Console().Message("TRACE - DPGI::getViewAxis - %s/%s - Type: %s\n",getNameInDocument(),Label.getValue(),Type.getValueAsString());
     gp_Ax2 viewAxis;
     Base::Vector3d x = OrientBasis.getValue();
     Base::Vector3d nx = x;
     x.Normalize();
     Base::Vector3d na = axis;
     na.Normalize();
//     Base::Console().Message("TRACE - DPGI::getViewAxis - axis: %s orient: %s\n",
//                             DrawUtil::formatVector(axis).c_str(),DrawUtil::formatVector(x).c_str());
     
     if (DrawUtil::checkParallel(nx,na)) {                    //parallel/antiparallel
//         Base::Console().Message("TRACE - DPGI::getViewAxis - parallel  flip: %d\n",flip);
         viewAxis = TechDrawGeometry::getViewAxis(pt,axis,flip);        //use default orientation
     } else {
         //Base::Console().Message("TRACE - DPGI::getViewAxis - skew  flip: %d\n",flip);
//         if (Type.isValue("Right") || Type.isValue("Left")) {               //no difference with incorrect initial axis
//             viewAxis = TechDrawGeometry::getViewAxis(pt,axis,x,!flip);     //no difference with correct initial axis!!
//         } else {
             viewAxis = TechDrawGeometry::getViewAxis(pt,axis,x,flip);
//         }
     }
     
     //Base::Console().Message("TRACE - DPGI::getViewAxis exits\n");
     return viewAxis;
}

//! rotate OrientBasis by angle radians around view Direction
Base::Vector3d DrawProjGroupItem::rotated(const double angle)
{
    Base::Console().Message("TRACE - DPGI::rotated - %s/%s angle: %.3f\n",Label.getValue(),Type.getValueAsString(),angle);
    Base::Vector3d line = Direction.getValue();
    Base::Vector3d oldBasis = OrientBasis.getValue();
    Base::Vector3d newBasis;
    Base::Vector3d org(0.0,0.0,0.0);
    Base::Matrix4D xForm;
    xForm.rotLine(line,angle);
    newBasis = xForm * (oldBasis);
    Base::Console().Message("TRACE - DPGI::rotated - line: %s old: %s new: %s\n",
                            DrawUtil::formatVector(line).c_str(),
                            DrawUtil::formatVector(oldBasis).c_str(),
                            DrawUtil::formatVector(newBasis).c_str());
//    if (getGroup() != nullptr) {
//        if (getGroup()->getException(Type.getValueAsString())) {
//            newBasis = newBasis * -1.0;
//            Base::Console().Message("TRACE - DPGI::rotated - EXCEPTION\n");
//        }
//    }
    return newBasis;
}

PyObject *DrawProjGroupItem::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawProjGroupItemPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
