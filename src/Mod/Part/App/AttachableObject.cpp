/***************************************************************************
 *   Copyright (c) Victor Titov (DeepSOIC)                                 *
 *                                           (vv.titov@gmail.com) 2015     *
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

#include "AttachableObject.h"

#include <Base/Console.h>
#include <App/Application.h>

#include <App/FeaturePythonPyImp.h>
#include "AttachableObjectPy.h"


using namespace Part;
using namespace Attacher;

PROPERTY_SOURCE(Part::AttachableObject, Part::Feature);

AttachableObject::AttachableObject()
   :  _attacher(0)
{
    ADD_PROPERTY_TYPE(Support, (0,0), "Attachment",(App::PropertyType)(App::Prop_None),"Support of the 2D geometry");

    ADD_PROPERTY_TYPE(MapMode, (mmDeactivated), "Attachment", App::Prop_None, "Mode of attachment to other object");
    MapMode.setEnums(AttachEngine::eMapModeStrings);
    //a rough test if mode string list in Attacher.cpp is in sync with eMapMode enum.
    assert(MapMode.getEnumVector().size() == mmDummy_NumberOfModes);

    ADD_PROPERTY_TYPE(MapReversed, (false), "Attachment", App::Prop_None, "Reverse Z direction (flip sketch upside down)");

    ADD_PROPERTY_TYPE(MapPathParameter, (0.0), "Attachment", App::Prop_None, "Sets point of curve to map the sketch to. 0..1 = start..end");

    ADD_PROPERTY_TYPE(superPlacement, (Base::Placement()), "Attachment", App::Prop_None, "Extra placement to apply in addition to attachment (in local coordinates)");

    //setAttacher(new AttachEngine3D);//default attacher
}

AttachableObject::~AttachableObject()
{
    if(_attacher)
        delete _attacher;
}

void AttachableObject::setAttacher(AttachEngine* attacher)
{
    if (_attacher)
        delete _attacher;
    _attacher = attacher;
    updateAttacherVals();
}

bool AttachableObject::positionBySupport()
{
    if (!_attacher)
        throw Base::Exception("AttachableObject: can't positionBySupport, because no AttachEngine is set.");
    updateAttacherVals();
    try{
        this->Placement.setValue(_attacher->calculateAttachedPlacement(this->Placement.getValue()));
        return true;
    } catch (ExceptionCancel) {
        //disabled, don't do anything
        return false;
    };
}

App::DocumentObjectExecReturn *AttachableObject::execute()
{
    if(this->isTouched_Mapping()) {
        try{
            positionBySupport();
        } catch (Base::Exception &e) {
            return new App::DocumentObjectExecReturn(e.what());
        } catch (Standard_Failure &e){
            return new App::DocumentObjectExecReturn(e.GetMessageString());
        }
    }
    return Part::Feature::execute();
}

namespace Attacher {
    void setReadonlyness(App::Property &prop, bool on)
    {
        unsigned long status = prop.getStatus();
        prop.setStatus(App::Property::ReadOnly, on);
        if (status != prop.getStatus())
            App::GetApplication().signalChangePropertyEditor(prop);
    }
}

void AttachableObject::onChanged(const App::Property* prop)
{
    if(! this->isRestoring()){
        if ((prop == &Support
             || prop == &MapMode
             || prop == &MapPathParameter
             || prop == &MapReversed
             || prop == &superPlacement)){

            bool bAttached = false;
            try{
                bAttached = positionBySupport();
            } catch (Base::Exception &e) {
                this->setError();
                Base::Console().Error("PositionBySupport: %s",e.what());
                //set error message - how?
            } catch (Standard_Failure &e){
                this->setError();
                Base::Console().Error("PositionBySupport: %s",e.GetMessageString());
            }

            eMapMode mmode = eMapMode(this->MapMode.getValue());
            setReadonlyness(this->superPlacement, !bAttached);
            setReadonlyness(this->Placement, bAttached && mmode != mmTranslate); //for mmTranslate, orientation should remain editable even when attached.
        }

    }

    Part::Feature::onChanged(prop);
}

void AttachableObject::updateAttacherVals()
{
    if (!_attacher)
        return;
    _attacher->setUp(this->Support,
                     eMapMode(this->MapMode.getValue()),
                     this->MapReversed.getValue(),
                     this->MapPathParameter.getValue(),
                     0.0,0.0,
                     this->superPlacement.getValue());
}

namespace App {
/// @cond DOXERR
  PROPERTY_SOURCE_TEMPLATE(Part::AttachableObjectPython, Part::AttachableObject)
  template<> const char* Part::AttachableObjectPython::getViewProviderName(void) const {
    return "PartGui::ViewProviderPython";
  }
  template<> PyObject* Part::AttachableObjectPython::getPyObject(void) {
        if (PythonObject.is(Py::_None())) {
            // ref counter is set to 1
            PythonObject = Py::Object(new FeaturePythonPyT<Part::AttachableObjectPy>(this),true);
        }
        return Py::new_reference_to(PythonObject);
  }
/// @endcond

// explicit template instantiation
  template class PartExport FeaturePythonT<Part::AttachableObject>;
}

