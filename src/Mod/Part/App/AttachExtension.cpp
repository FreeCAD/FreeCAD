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

#include "AttachExtension.h"

#include <Base/Console.h>
#include <App/Application.h>

#include <App/FeaturePythonPyImp.h>
#include "AttachExtensionPy.h"


using namespace Part;
using namespace Attacher;

EXTENSION_PROPERTY_SOURCE(Part::AttachExtension, App::DocumentObjectExtension)

AttachExtension::AttachExtension()
   :  _attacher(0)
{
    EXTENSION_ADD_PROPERTY_TYPE(AttacherType, ("Attacher::AttachEngine3D"), "Attachment",(App::PropertyType)(App::Prop_None),"Class name of attach engine object driving the attachment.");
    this->AttacherType.setStatus(App::Property::Status::Hidden, true);

    EXTENSION_ADD_PROPERTY_TYPE(Support, (0,0), "Attachment",(App::PropertyType)(App::Prop_None),"Support of the 2D geometry");

    EXTENSION_ADD_PROPERTY_TYPE(MapMode, (mmDeactivated), "Attachment", App::Prop_None, "Mode of attachment to other object");
    MapMode.setEditorName("PartGui::PropertyEnumAttacherItem");
    MapMode.setEnums(AttachEngine::eMapModeStrings);
    //a rough test if mode string list in Attacher.cpp is in sync with eMapMode enum.
    assert(MapMode.getEnumVector().size() == mmDummy_NumberOfModes);

    EXTENSION_ADD_PROPERTY_TYPE(MapReversed, (false), "Attachment", App::Prop_None, "Reverse Z direction (flip sketch upside down)");

    EXTENSION_ADD_PROPERTY_TYPE(MapPathParameter, (0.0), "Attachment", App::Prop_None, "Sets point of curve to map the sketch to. 0..1 = start..end");

    EXTENSION_ADD_PROPERTY_TYPE(AttachmentOffset, (Base::Placement()), "Attachment", App::Prop_None, "Extra placement to apply in addition to attachment (in local coordinates)");

    // Only show these properties when applicable. Controlled by extensionOnChanged
    this->MapPathParameter.setStatus(App::Property::Status::Hidden, true);
    this->MapReversed.setStatus(App::Property::Status::Hidden, true);
    this->AttachmentOffset.setStatus(App::Property::Status::Hidden, true);

    setAttacher(new AttachEngine3D);//default attacher
    initExtensionType(AttachExtension::getExtensionClassTypeId());
}

AttachExtension::~AttachExtension()
{
    if(_attacher)
        delete _attacher;
}

void AttachExtension::setAttacher(AttachEngine* attacher)
{
    if (_attacher)
        delete _attacher;
    _attacher = attacher;
    if (_attacher){
        const char* typeName = attacher->getTypeId().getName();
        if(strcmp(this->AttacherType.getValue(),typeName)!=0) //make sure we need to change, to break recursive onChange->changeAttacherType->onChange...
            this->AttacherType.setValue(typeName);
        updateAttacherVals();
    } else {
        if (strlen(AttacherType.getValue()) != 0){ //make sure we need to change, to break recursive onChange->changeAttacherType->onChange...
            this->AttacherType.setValue("");
        }
    }
}

bool AttachExtension::changeAttacherType(const char* typeName)
{
    //check if we need to actually change anything
    if (_attacher){
        if (strcmp(_attacher->getTypeId().getName(),typeName)==0){
            return false;
        }
    } else if (strlen(typeName) == 0){
        return false;
    }
    if (strlen(typeName) == 0){
        setAttacher(nullptr);
        return true;
    }
    Base::Type t = Base::Type::fromName(typeName);
    if (t.isDerivedFrom(AttachEngine::getClassTypeId())){
        AttachEngine* pNewAttacher = static_cast<Attacher::AttachEngine*>(Base::Type::createInstanceByName(typeName));
        this->setAttacher(pNewAttacher);
        return true;
    }

    std::stringstream errMsg;
    errMsg << "Object if this type is not derived from AttachEngine: " << typeName;
    throw AttachEngineException(errMsg.str());
}

bool AttachExtension::positionBySupport()
{
    if (!_attacher)
        throw Base::RuntimeError("AttachExtension: can't positionBySupport, because no AttachEngine is set.");
    updateAttacherVals();
    try {
        if (_attacher->mapMode == mmDeactivated)
            return false;
        bool subChanged = false;
        getPlacement().setValue(_attacher->calculateAttachedPlacement(
                    getPlacement().getValue(), &subChanged));
        if(subChanged) 
            Support.setValues(Support.getValues(),_attacher->getSubValues());

        return true;
    } catch (ExceptionCancel&) {
        //disabled, don't do anything
        return false;
    };
}

short int AttachExtension::extensionMustExecute(void) {
    return DocumentObjectExtension::extensionMustExecute();
}


App::DocumentObjectExecReturn *AttachExtension::extensionExecute()
{
    if(this->isTouched_Mapping()) {
        try{
            positionBySupport();
        // we let all Base::Exceptions thru, so that App:DocumentObject can take appropriate action
        /*} catch (Base::Exception &e) {
            return new App::DocumentObjectExecReturn(e.what());*/
        // Convert OCC exceptions to Base::Exception
        } catch (Standard_Failure &e){
            throw Base::RuntimeError(e.GetMessageString());
//            return new App::DocumentObjectExecReturn(e.GetMessageString());
        }
    }
    return App::DocumentObjectExtension::extensionExecute();
}

void AttachExtension::extensionOnChanged(const App::Property* prop)
{
    if(! getExtendedObject()->isRestoring()){
        if ((prop == &Support
             || prop == &MapMode
             || prop == &MapPathParameter
             || prop == &MapReversed
             || prop == &AttachmentOffset)){

            bool bAttached = false;
            try{
                bAttached = positionBySupport();
            } catch (Base::Exception &e) {
                getExtendedObject()->setStatus(App::Error, true);
                Base::Console().Error("PositionBySupport: %s\n",e.what());
                //set error message - how?
            } catch (Standard_Failure &e){
                getExtendedObject()->setStatus(App::Error, true);
                Base::Console().Error("PositionBySupport: %s\n",e.GetMessageString());
            }

            // Hide properties when not applicable to reduce user confusion

            eMapMode mmode = eMapMode(this->MapMode.getValue());

            bool modeIsPointOnCurve = mmode == mmNormalToPath ||
                mmode == mmFrenetNB || mmode == mmFrenetTN || mmode == mmFrenetTB ||
                mmode == mmRevolutionSection || mmode == mmConcentric;

            // MapPathParameter is only used if there is a reference to one edge and not edge + vertex
            bool hasOneRef = false;
            if (_attacher && _attacher->subnames.size() == 1) {
                hasOneRef = true;
            }

            this->MapPathParameter.setStatus(App::Property::Status::Hidden, !bAttached || !(modeIsPointOnCurve && hasOneRef));
            this->MapReversed.setStatus(App::Property::Status::Hidden, !bAttached);
            this->AttachmentOffset.setStatus(App::Property::Status::Hidden, !bAttached);
            getPlacement().setReadOnly(bAttached && mmode != mmTranslate); //for mmTranslate, orientation should remain editable even when attached.
        }

    }

    if(prop == &(this->AttacherType)){
        this->changeAttacherType(this->AttacherType.getValue());
    }

    App::DocumentObjectExtension::extensionOnChanged(prop);
}

void AttachExtension::extHandleChangedPropertyName(Base::XMLReader &reader, const char* TypeName, const char* PropName)
{
    // Was superPlacement
    Base::Type type = Base::Type::fromName(TypeName);
    if (AttachmentOffset.getClassTypeId() == type && strcmp(PropName, "superPlacement") == 0) {
        AttachmentOffset.Restore(reader);
    }
}

void AttachExtension::onExtendedDocumentRestored()
{
    try {
        bool bAttached = positionBySupport();

        // Hide properties when not applicable to reduce user confusion
        eMapMode mmode = eMapMode(this->MapMode.getValue());
        bool modeIsPointOnCurve =
                (mmode == mmNormalToPath ||
                 mmode == mmFrenetNB ||
                 mmode == mmFrenetTN ||
                 mmode == mmFrenetTB ||
                 mmode == mmRevolutionSection ||
                 mmode == mmConcentric);

        // MapPathParameter is only used if there is a reference to one edge and not edge + vertex
        bool hasOneRef = false;
        if (_attacher && _attacher->subnames.size() == 1) {
            hasOneRef = true;
        }

        this->MapPathParameter.setStatus(App::Property::Status::Hidden, !bAttached || !(modeIsPointOnCurve && hasOneRef));
        this->MapReversed.setStatus(App::Property::Status::Hidden, !bAttached);
        this->AttachmentOffset.setStatus(App::Property::Status::Hidden, !bAttached);
        getPlacement().setReadOnly(bAttached && mmode != mmTranslate); //for mmTranslate, orientation should remain editable even when attached.
    }
    catch (Base::Exception&) {
    }
    catch (Standard_Failure &) {
    }
}

void AttachExtension::updateAttacherVals()
{
    if (!_attacher)
        return;
    _attacher->setUp(this->Support,
                     eMapMode(this->MapMode.getValue()),
                     this->MapReversed.getValue(),
                     this->MapPathParameter.getValue(),
                     0.0,0.0,
                     this->AttachmentOffset.getValue());
}

App::PropertyPlacement& AttachExtension::getPlacement() {

    if(!getExtendedObject()->isDerivedFrom(App::GeoFeature::getClassTypeId()))
        throw Base::RuntimeError("AttachExtension not added to GeooFeature!");
    
    return static_cast<App::GeoFeature*>(getExtendedObject())->Placement;
}

PyObject* AttachExtension::getExtensionPyObject(void) {
    
    if (ExtensionPythonObject.is(Py::_None())){
        // ref counter is set to 1
        ExtensionPythonObject = Py::Object(new AttachExtensionPy(this),true);
    }
    return Py::new_reference_to(ExtensionPythonObject);
}

// ------------------------------------------------

AttachEngineException::AttachEngineException()
  : Base::Exception()
{
}

AttachEngineException::AttachEngineException(const char * sMessage)
  : Base::Exception(sMessage)
{
}

AttachEngineException::AttachEngineException(const std::string& sMessage)
  : Base::Exception(sMessage)
{
}

AttachEngineException::AttachEngineException(const AttachEngineException &inst)
  : Base::Exception(inst)
{
}


namespace App {
/// @cond DOXERR
  EXTENSION_PROPERTY_SOURCE_TEMPLATE(Part::AttachExtensionPython, Part::AttachExtension)
/// @endcond

// explicit template instantiation
  template class PartExport ExtensionPythonT<Part::AttachExtension>;
}

