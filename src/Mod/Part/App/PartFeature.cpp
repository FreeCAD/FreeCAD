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
# include <gp_Trsf.hxx>
# include <gp_Ax1.hxx>
#endif


#include <strstream>
#include <Base/Console.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <Base/Placement.h>
#include <Base/Rotation.h>

#include "PartFeature.h"
#include "PartFeaturePy.h"
#include "FeaturePythonPy.h"

using namespace Part;


PROPERTY_SOURCE(Part::Feature, App::GeoFeature)


Feature::Feature(void) 
{
    ADD_PROPERTY(Shape, (TopoDS_Shape()));
}

Feature::~Feature()
{
}

short Feature::mustExecute(void) const
{
    return GeoFeature::mustExecute();
}

App::DocumentObjectExecReturn *Feature::execute(void)
{
    return App::DocumentObject::StdReturn;
}

PyObject *Feature::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new PartFeaturePy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

std::vector<PyObject *> Feature::getPySubObjects(const std::vector<std::string>& NameVec) const
{
    std::vector<PyObject *> temp;
    for(std::vector<std::string>::const_iterator it=NameVec.begin();it!=NameVec.end();++it){
        PyObject *obj = Shape.getShape().getPySubShape((*it).c_str());
        if(obj)
            temp.push_back(obj);
    }
    return temp;
}

void Feature::onChanged(const App::Property* prop)
{
    // if the placement has changed apply the change to the point data as well
    if (prop == &this->Placement) {
        TopoShape& shape = const_cast<TopoShape&>(this->Shape.getShape());
        shape.setTransform(this->Placement.getValue().toMatrix());
    }
    // if the point data has changed check and adjust the transformation as well
    else if (prop == &this->Shape) {
        if (this->isRecomputing()) {
            TopoShape& shape = const_cast<TopoShape&>(this->Shape.getShape());
            shape.setTransform(this->Placement.getValue().toMatrix());
        }
        else {
            Base::Placement p;
            p.fromMatrix(this->Shape.getShape().getTransform());
            if (p != this->Placement.getValue())
                this->Placement.setValue(p);
        }
    }
    
    GeoFeature::onChanged(prop);
}

TopLoc_Location Feature::getLocation() const
{
    Base::Placement pl = this->Placement.getValue();
    Base::Rotation rot(pl.getRotation());
    Base::Vector3d axis;
    double angle;
    rot.getValue(axis, angle);
    gp_Trsf trf;
    trf.SetRotation(gp_Ax1(gp_Pnt(), gp_Dir(axis.x, axis.y, axis.z)), angle);
    trf.SetTranslationPart(gp_Vec(pl.getPosition().x,pl.getPosition().y,pl.getPosition().z));
    return TopLoc_Location(trf);
}

    /// returns the type name of the ViewProvider
const char* Feature::getViewProviderName(void) const {
    return "PartGui::ViewProviderPart";
}

// ---------------------------------------------------------

PROPERTY_SOURCE(Part::FilletBase, Part::Feature)

FilletBase::FilletBase()
{
    ADD_PROPERTY(Base,(0));
    ADD_PROPERTY(Edges,(0,0,0));
    Edges.setSize(0);
}

short FilletBase::mustExecute() const
{
    if (Base.isTouched() || Edges.isTouched())
        return 1;
    return 0;
}

// ---------------------------------------------------------

PROPERTY_SOURCE(Part::FeatureExt, Part::Feature)



namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Part::FeaturePython, Part::Feature)
template<> const char* Part::FeaturePython::getViewProviderName(void) const {
    return "PartGui::ViewProviderPython";
}

template<> PyObject* Part::FeaturePython::getPyObject(void) {
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new Part::FeaturePythonPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}
/// @endcond

// explicit template instantiation
template class PartExport FeaturePythonT<Part::Feature>;
}

