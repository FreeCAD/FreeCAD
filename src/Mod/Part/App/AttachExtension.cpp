/***************************************************************************
 *   Copyright (c) 2015 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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
#include <Base/Tools.h>

#include "AttachExtension.h"
#include "AttachExtensionPy.h"


using namespace Part;
using namespace Attacher;

EXTENSION_PROPERTY_SOURCE(Part::AttachExtension, App::DocumentObjectExtension)

#ifdef FC_USE_TNP_FIX

AttachExtension::AttachExtension()
{
    EXTENSION_ADD_PROPERTY_TYPE(AttacherType,
                                ("Attacher::AttachEngine3D"),
                                "Attachment",
                                (App::PropertyType)(App::Prop_None),
                                "Class name of attach engine object driving the attachment.");
    this->AttacherType.setStatus(App::Property::Status::Hidden, true);

    EXTENSION_ADD_PROPERTY_TYPE(
        Support,
        (nullptr, nullptr),
        "Attachment",
        (App::PropertyType)(App::Prop_Hidden),
        "Support of the 2D geometry (Deprecated! Use AttachmentSupport instead");
    Support.setScope(App::LinkScope::Global);

    EXTENSION_ADD_PROPERTY_TYPE(AttachmentSupport,
                                (nullptr, nullptr),
                                "Attachment",
                                (App::PropertyType)(App::Prop_None),
                                "Support of the 2D geometry");
    AttachmentSupport.setScope(App::LinkScope::Global);

    EXTENSION_ADD_PROPERTY_TYPE(MapMode,
                                (mmDeactivated),
                                "Attachment",
                                App::Prop_None,
                                "Mode of attachment to other object");
    MapMode.setEditorName("PartGui::PropertyEnumAttacherItem");
    MapMode.setEnums(AttachEngine::eMapModeStrings);
    // a rough test if mode string list in Attacher.cpp is in sync with eMapMode enum.
    assert(MapMode.getEnumVector().size() == mmDummy_NumberOfModes);

    EXTENSION_ADD_PROPERTY_TYPE(MapReversed,
                                (false),
                                "Attachment",
                                App::Prop_None,
                                "Reverse Z direction (flip sketch upside down)");

    EXTENSION_ADD_PROPERTY_TYPE(MapPathParameter,
                                (0.0),
                                "Attachment",
                                App::Prop_None,
                                "Sets point of curve to map the sketch to. 0..1 = start..end");

    EXTENSION_ADD_PROPERTY_TYPE(
        AttachmentOffset,
        (Base::Placement()),
        "Attachment",
        App::Prop_None,
        "Extra placement to apply in addition to attachment (in local coordinates)");

    _props.attacherType = &AttacherType;
    _props.attachment = &AttachmentSupport;
    _props.mapMode = &MapMode;
    _props.mapReversed = &MapReversed;
    _props.mapPathParameter = &MapPathParameter;

    setAttacher(new AttachEngine3D);  // default attacher
    _baseProps.attacher.reset(new AttachEngine3D);

    updatePropertyStatus(false);

    initExtensionType(AttachExtension::getExtensionClassTypeId());
}

AttachExtension::~AttachExtension()
{}

template<class T>
static inline bool getProp(bool force,
                           T*& prop,
                           Base::Type type,
                           App::PropertyContainer* owner,
                           const char* name,
                           const char* doc)
{
    prop = Base::freecad_dynamic_cast<T>(owner->getDynamicPropertyByName(name));
    if (prop || !force) {
        return false;
    }
    prop = static_cast<T*>(owner->addDynamicProperty(type.getName(), name, "Attachment", doc));
    if (!prop) {
        FC_THROWM(Base::RuntimeError, "Failed to add property " << owner->getFullName() << name);
    }
    prop->setStatus(App::Property::Status::LockDynamic, true);
    prop->setStatus(App::Property::Status::Hidden, true);
    return true;
}

template<class T>
static inline bool
getProp(bool force, T*& prop, App::PropertyContainer* owner, const char* name, const char* doc)
{
    return getProp(force, prop, T::getClassTypeId(), owner, name, doc);
}

void AttachExtension::initBase(bool force)
{
    if (_baseProps.attacherType) {
        return;
    }
    auto obj = getExtendedObject();

    // Temporary holding the properties so that we only handle onChanged() event
    // when all relevant properties are ready.
    Properties props;

    if (getProp<App::PropertyString>(
            force,
            props.attacherType,
            obj,
            "BaseAttacherType",
            "Class name of attach engine object driving the attachment for base geometry.")) {
        props.attacherType->setValue(_baseProps.attacher->getTypeId().getName());
    }
    else if (!props.attacherType) {
        return;
    }

    getProp<App::PropertyLinkSubList>(force,
                                      props.attachment,
                                      App::PropertyLinkSubListHidden::getClassTypeId(),
                                      obj,
                                      "BaseAttachment",
                                      "Link to base geometry.");

    if (getProp<App::PropertyEnumeration>(force,
                                          props.mapMode,
                                          obj,
                                          "BaseMapMode",
                                          "Mode of attachment for the base geometry")) {
        props.mapMode->setStatus(App::Property::Status::Hidden, false);
    }
    if (props.mapMode) {
        props.mapMode->setEditorName("PartGui::PropertyEnumAttacherItem");
        props.mapMode->setEnums(AttachEngine::eMapModeStrings);
    }

    getProp<App::PropertyBool>(force,
                               props.mapReversed,
                               obj,
                               "BaseMapReversed",
                               "Reverse Z direction of the base geometry attachment");

    getProp<App::PropertyFloat>(force,
                                props.mapPathParameter,
                                obj,
                                "BaseMapPathParameter",
                                "Sets point of base curve to map 0..1 = start..end");

    static_cast<Properties&>(_baseProps) = props;
}

void AttachExtension::setAttacher(AttachEngine* pAttacher, bool base)
{
    auto& props = base ? _baseProps : _props;
    props.attacher.reset(pAttacher);
    if (props.attacher) {
        if (base) {
            initBase(true);
        }
        const char* typeName = props.attacher->getTypeId().getName();
        if (strcmp(props.attacherType->getValue(), typeName)
            != 0) {  // make sure we need to change, to break recursive
                     // onChange->changeAttacherType->onChange...
            props.attacherType->setValue(typeName);
        }
        updateAttacherVals(base);
    }
    else {
        if (props.attacherType
            && strlen(props.attacherType->getValue())
                != 0) {  // make sure we need to change, to break recursive
                         // onChange->changeAttacherType->onChange...
            props.attacherType->setValue("");
        }
    }
}

bool AttachExtension::changeAttacherType(const char* typeName, bool base)
{
    auto& prop = base ? _baseProps : _props;

    // check if we need to actually change anything
    if (prop.attacher) {
        if (strcmp(prop.attacher->getTypeId().getName(), typeName) == 0) {
            return false;
        }
    }
    else if (strlen(typeName) == 0) {
        return false;
    }
    if (strlen(typeName) == 0) {
        setAttacher(nullptr, base);
        return true;
    }
    Base::Type t = Base::Type::fromName(typeName);
    if (t.isDerivedFrom(AttachEngine::getClassTypeId())) {
        AttachEngine* pNewAttacher =
            static_cast<Attacher::AttachEngine*>(Base::Type::createInstanceByName(typeName));
        this->setAttacher(pNewAttacher, base);
        return true;
    }

    std::stringstream errMsg;
    errMsg << "Object if this type is not derived from AttachEngine: " << typeName;
    throw AttachEngineException(errMsg.str());
}

bool AttachExtension::positionBySupport()
{
    _active = 0;
    if (!_props.attacher) {
        throw Base::RuntimeError(
            "AttachExtension: can't positionBySupport, because no AttachEngine is set.");
    }
    Base::Placement plaOriginal = getPlacement().getValue();
    try {
        if (_props.attacher->mapMode == mmDeactivated) {
            return false;
        }
        bool subChanged = false;

        getPlacement().setValue(Base::Placement());

        Base::Placement basePlacement;
        if (_baseProps.attacher && _baseProps.attacher->mapMode != mmDeactivated) {
            basePlacement =
                _baseProps.attacher->calculateAttachedPlacement(Base::Placement(), &subChanged);
            if (subChanged) {
                _baseProps.attachment->setValues(_baseProps.attachment->getValues(),
                                                 _baseProps.attacher->getSubValues());
            }
        }

        subChanged = false;
        _props.attacher->setOffset(AttachmentOffset.getValue() * basePlacement.inverse());
        auto placement = _props.attacher->calculateAttachedPlacement(plaOriginal, &subChanged);
        if (subChanged) {
            Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(
                App::Property::User3,
                &AttachmentSupport);
            AttachmentSupport.setValues(AttachmentSupport.getValues(),
                                        _props.attacher->getSubValues());
        }
        getPlacement().setValue(placement);
        _active = 1;
        return true;
    }
    catch (ExceptionCancel&) {
        // disabled, don't do anything
        getPlacement().setValue(plaOriginal);
        return false;
    }
    catch (Base::Exception&) {
        getPlacement().setValue(plaOriginal);
        throw;
    }
    catch (Standard_Failure&) {
        getPlacement().setValue(plaOriginal);
        throw;
    }
}

bool AttachExtension::isAttacherActive() const
{
    if (_active < 0) {
        _active = 0;
        try {
            updateAttacherVals(/*base*/ false);
            updateAttacherVals(/*base*/ true);
            _props.attacher->calculateAttachedPlacement(getPlacement().getValue());
            _active = 1;
        }
        catch (Base::Exception&) {
        }
    }
    return _active != 0;
}

short int AttachExtension::extensionMustExecute()
{
    return DocumentObjectExtension::extensionMustExecute();
}


App::DocumentObjectExecReturn* AttachExtension::extensionExecute()
{
    if (this->isTouched_Mapping()) {
        try {
            positionBySupport();
            // we let all Base::Exceptions thru, so that App:DocumentObject can take appropriate
            // action
            /*} catch (Base::Exception &e) {
                return new App::DocumentObjectExecReturn(e.what());*/
            // Convert OCC exceptions to Base::Exception
        }
        catch (Standard_Failure& e) {
            throw Base::RuntimeError(e.GetMessageString());
            //            return new App::DocumentObjectExecReturn(e.GetMessageString());
        }
    }
    return App::DocumentObjectExtension::extensionExecute();
}

void AttachExtension::extensionOnChanged(const App::Property* prop)
{
    if (!getExtendedObject()->isRestoring()) {
        if (prop == &Support) {
            if (!prop->testStatus(App::Property::User3)) {
                Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(
                    App::Property::User3,
                    &Support);
                AttachmentSupport.Paste(Support);
            }
        }
        else if (_props.matchProperty(prop)) {
            if (prop == &AttachmentSupport) {
                Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(
                    App::Property::User3,
                    &Support);
                Support.Paste(AttachmentSupport);
            }
            _active = -1;
            updateAttacherVals(/*base*/ false);
            updatePropertyStatus(isAttacherActive());
        }
        else if (_baseProps.matchProperty(prop)) {
            _active = -1;
            updateAttacherVals(/*base*/ true);
            updatePropertyStatus(isAttacherActive(), /*base*/ true);
        }
    }
    if (prop == &(this->AttacherType)) {
        this->changeAttacherType(this->AttacherType.getValue());
    }
    else if (prop == _baseProps.attacherType) {
        this->changeAttacherType(_baseProps.attacherType->getValue());
    }

    App::DocumentObjectExtension::extensionOnChanged(prop);
}

void AttachExtension::extHandleChangedPropertyName(Base::XMLReader& reader,
                                                   const char* TypeName,
                                                   const char* PropName)
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
        if (Support.getValue()) {
            AttachmentSupport.Paste(Support);
        }
        initBase(false);
        if (_baseProps.attachment) {
            _baseProps.attachment->setScope(App::LinkScope::Hidden);
        }
        if (_baseProps.attacherType) {
            changeAttacherType(_baseProps.attacherType->getValue(), true);
        }
        _active = -1;
        updatePropertyStatus(isAttacherActive());
    }
    catch (Base::Exception&) {
    }
    catch (Standard_Failure&) {
    }
}

void AttachExtension::updatePropertyStatus(bool bAttached, bool base)
{
    auto& props = base ? this->_baseProps : this->_props;
    if (!props.mapMode) {
        return;
    }

    // Hide properties when not applicable to reduce user confusion
    eMapMode mmode = eMapMode(props.mapMode->getValue());
    bool modeIsPointOnCurve =
        (mmode == mmNormalToPath || mmode == mmFrenetNB || mmode == mmFrenetTN
         || mmode == mmFrenetTB || mmode == mmRevolutionSection || mmode == mmConcentric);

    // MapPathParameter is only used if there is a reference to one edge and not edge + vertex
    bool hasOneRef = false;
    if (props.attacher && props.attacher->subnames.size() == 1) {
        hasOneRef = true;
    }
    props.mapPathParameter->setStatus(App::Property::Status::Hidden,
                                      !bAttached || !(modeIsPointOnCurve && hasOneRef));
    props.mapReversed->setStatus(App::Property::Status::Hidden, !bAttached);

    if (base) {
        props.attachment->setStatus(App::Property::Status::Hidden, !bAttached);
    }
    else {
        this->AttachmentOffset.setStatus(App::Property::Status::Hidden, !bAttached);
        if (getExtendedContainer()) {
            getPlacement().setReadOnly(
                bAttached && mmode != mmTranslate);  // for mmTranslate, orientation should remain
                                                     // editable even when attached.
        }
        updatePropertyStatus(bAttached, true);
    }
}

void AttachExtension::updateAttacherVals(bool base) const
{
    auto& props = base ? this->_baseProps : this->_props;
    if (!props.attachment) {
        return;
    }
    attacher(base).setUp(*props.attachment,
                         eMapMode(props.mapMode->getValue()),
                         props.mapReversed->getValue(),
                         props.mapPathParameter->getValue(),
                         0.0,
                         0.0);
}

AttachExtension::Properties AttachExtension::getProperties(bool base) const
{
    return base ? _baseProps : _props;
}

AttachExtension::Properties AttachExtension::getInitedProperties(bool base)
{
    if (base) {
        initBase(true);
        return _baseProps;
    }
    return _props;
}

App::PropertyPlacement& AttachExtension::getPlacement() const
{
    auto pla = Base::freecad_dynamic_cast<App::PropertyPlacement>(
        getExtendedObject()->getPropertyByName("Placement"));
    if (!pla) {
        throw Base::RuntimeError("AttachExtension cannot find placement property");
    }
    return *pla;
}

PyObject* AttachExtension::getExtensionPyObject()
{

    if (ExtensionPythonObject.is(Py::_None())) {
        // ref counter is set to 1
        ExtensionPythonObject = Py::Object(new AttachExtensionPy(this), true);
    }
    return Py::new_reference_to(ExtensionPythonObject);
}


Attacher::AttachEngine& AttachExtension::attacher(bool base) const
{
    auto& props = base ? _baseProps : _props;
    if (!props.attacher) {
        throw AttachEngineException("AttachableObject: no attacher is set.");
    }
    return *props.attacher;
}

// ------------------------------------------------

AttachEngineException::AttachEngineException()
    : Base::Exception()
{}

AttachEngineException::AttachEngineException(const char* sMessage)
    : Base::Exception(sMessage)
{}

AttachEngineException::AttachEngineException(const std::string& sMessage)
    : Base::Exception(sMessage)
{}

#else
AttachExtension::AttachExtension()
{
    EXTENSION_ADD_PROPERTY_TYPE(AttacherType, ("Attacher::AttachEngine3D"), "Attachment",(App::PropertyType)(App::Prop_None),"Class name of attach engine object driving the attachment.");
    this->AttacherType.setStatus(App::Property::Status::Hidden, true);

    EXTENSION_ADD_PROPERTY_TYPE(AttachmentSupport, (nullptr,nullptr), "Attachment",(App::PropertyType)(App::Prop_None),"Support of the 2D geometry");

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
    _active = 0;
    if (!_attacher)
        throw Base::RuntimeError("AttachExtension: can't positionBySupport, because no AttachEngine is set.");
    updateAttacherVals();
    try {
        if (_attacher->mapMode == mmDeactivated)
            return false;
        bool subChanged = false;
        getPlacement().setValue(_attacher->calculateAttachedPlacement(getPlacement().getValue(), &subChanged));
        if(subChanged)
            AttachmentSupport.setValues(AttachmentSupport.getValues(),_attacher->getSubValues());
        _active = 1;
        return true;
    } catch (ExceptionCancel&) {
        //disabled, don't do anything
        return false;
    };
}

bool AttachExtension::isAttacherActive() const {
    if(_active < 0) {
        _active = 0;
        try {
            _attacher->calculateAttachedPlacement(getPlacement().getValue());
            _active = 1;
        } catch (ExceptionCancel&) {
        }
    }
    return _active!=0;
}

short int AttachExtension::extensionMustExecute() {
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
        if ((prop == &AttachmentSupport
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

bool AttachExtension::extensionHandleChangedPropertyName(Base::XMLReader &reader, const char* TypeName, const char *PropName)
{
    // superPlacement -> AttachmentOffset
    Base::Type type = Base::Type::fromName(TypeName);
    if (strcmp(PropName, "superPlacement") == 0 && AttachmentOffset.getClassTypeId() == type) {
        AttachmentOffset.Restore(reader);
        return true;
    }
    // Support -> AttachmentSupport
    else if (strcmp(PropName, "Support") == 0) {
        // At one point, the type of Support changed from PropertyLinkSub to its present type of PropertyLinkSubList.
        // Later, the property name changed to AttachmentSupport
        App::PropertyLinkSub tmp;
        if (0 == strcmp(tmp.getTypeId().getName(),TypeName)) {
            tmp.setContainer(this->getExtendedContainer());
            tmp.Restore(reader);
            AttachmentSupport.setValue(tmp.getValue(), tmp.getSubValues());
            this->MapMode.setValue(Attacher::mmFlatFace);
            return true;
        }
        else if (AttachmentSupport.getClassTypeId() == type) {
            AttachmentSupport.Restore(reader);
            return true;
        }
    }
    return App::DocumentObjectExtension::extensionHandleChangedPropertyName(reader, TypeName, PropName);
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
    _attacher->setUp(this->AttachmentSupport,
                     eMapMode(this->MapMode.getValue()),
                     this->MapReversed.getValue(),
                     this->MapPathParameter.getValue(),
                     0.0,0.0,
                     this->AttachmentOffset.getValue());
}

App::PropertyPlacement& AttachExtension::getPlacement() const {
    auto pla = Base::freecad_dynamic_cast<App::PropertyPlacement>(
            getExtendedObject()->getPropertyByName("Placement"));
    if(!pla)
        throw Base::RuntimeError("AttachExtension cannot find placement property");
    return *pla;
}

PyObject* AttachExtension::getExtensionPyObject() {

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
#endif

namespace App {
/// @cond DOXERR
  EXTENSION_PROPERTY_SOURCE_TEMPLATE(Part::AttachExtensionPython, Part::AttachExtension)
/// @endcond

// explicit template instantiation
  template class PartExport ExtensionPythonT<Part::AttachExtension>;
}

