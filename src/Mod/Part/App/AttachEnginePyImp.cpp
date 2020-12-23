#include "PreCompiled.h"
#ifndef _PreComp_
# include <Standard_Failure.hxx>
#endif

#include "Mod/Part/App/Attacher.h"
#include <Base/PlacementPy.h>
#include <App/DocumentObjectPy.h>
#include "AttachExtensionPy.h"
#include "TopoShapePy.h"

#include "OCCError.h"

// inclusion of the generated files (generated out of AttachableObjectPy.xml)
#include "AttachEnginePy.h"
#include "AttachEnginePy.cpp"

using namespace Attacher;

// returns a string which represents the object e.g. when printed in python
std::string AttachEnginePy::representation(void) const
{
    return std::string("<Attacher::AttachEngine>");
}

PyObject* AttachEnginePy::PyMake(struct _typeobject *, PyObject *, PyObject *)
{
    // create a new instance of AttachEngine3D
    return new AttachEnginePy(new AttachEngine3D);
}

// constructor method
int AttachEnginePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* o;
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(AttachEnginePy::Type), &o)) {
        AttachEngine* attacher = static_cast<AttachEnginePy*>(o)->getAttachEnginePtr();
        AttachEngine* oldAttacher = this->getAttachEnginePtr();
        this->_pcTwinPointer = attacher->copy();
        delete oldAttacher;
        return 0;
    }

    PyErr_Clear();
    char* typeName;
    if (PyArg_ParseTuple(args, "s", &typeName)) {
        Base::Type t = Base::Type::fromName(typeName);
        AttachEngine* pNewAttacher = nullptr;
        if (t.isDerivedFrom(AttachEngine::getClassTypeId())){
            pNewAttacher = static_cast<Attacher::AttachEngine*>(Base::Type::createInstanceByName(typeName));
        }
        if (!pNewAttacher) {
            std::stringstream errMsg;
            errMsg << "Object if this type is not derived from AttachEngine: " << typeName;
            PyErr_SetString(Base::BaseExceptionFreeCADError, errMsg.str().c_str());
            return -1;
        }
        AttachEngine* oldAttacher = this->getAttachEnginePtr();
        this->_pcTwinPointer = pNewAttacher;
        delete oldAttacher;
        return 0;
    }

    PyErr_SetString(Base::BaseExceptionFreeCADError, "Wrong set of constructor arguments. Can be: (), ('Attacher::AttachEngine3D'), ('Attacher::AttachEnginePlane'), ('Attacher::AttachEngineLine'), ('Attacher::AttachEnginePoint'), (other_attacher_instance).");
    return -1;

}


Py::String AttachEnginePy::getAttacherType(void) const
{
    return  Py::String(std::string(this->getAttachEnginePtr()->getTypeId().getName()));
}

/**
  * @brief macro ATTACHERPY_STDCATCH_ATTR: catch for exceptions in attribute
  * code (re-throws the exception converted to Py::Exception). It is a helper
  * to avoid repeating the same error handling code over and over again.
  */
#define ATTACHERPY_STDCATCH_ATTR \
    catch (Standard_Failure& e) {\
        throw Py::Exception(Part::PartExceptionOCCError, e.GetMessageString());\
    } catch (Base::Exception &e) {\
        throw Py::Exception(Base::BaseExceptionFreeCADError, e.what());\
    }

Py::String AttachEnginePy::getMode(void) const
{
    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        return Py::String(attacher.getModeName(attacher.mapMode));
    } ATTACHERPY_STDCATCH_ATTR;
}

void AttachEnginePy::setMode(Py::String arg)
{
    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        std::string modeName = (std::string)arg;
        attacher.mapMode = attacher.getModeByName(modeName);
    } ATTACHERPY_STDCATCH_ATTR;
}

Py::Object AttachEnginePy::getReferences(void) const
{
    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        AttachEngine::verifyReferencesAreSafe(attacher.references);
        return Py::Object(attacher.references.getPyObject(),true);
    } ATTACHERPY_STDCATCH_ATTR;
}

void AttachEnginePy::setReferences(Py::Object arg)
{
    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        attacher.references.setPyObject(arg.ptr());
    } ATTACHERPY_STDCATCH_ATTR;
}

Py::Object AttachEnginePy::getAttachmentOffset(void) const
{
    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        return Py::Object(new Base::PlacementPy(new Base::Placement(attacher.attachmentOffset)),true);
    } ATTACHERPY_STDCATCH_ATTR;
}

void AttachEnginePy::setAttachmentOffset(Py::Object arg)
{
    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        if (PyObject_TypeCheck(arg.ptr(), &(Base::PlacementPy::Type))) {
            const Base::PlacementPy* plmPy = static_cast<const Base::PlacementPy*>(arg.ptr());
            attacher.attachmentOffset = *(plmPy->getPlacementPtr());
        } else {
            std::string error = std::string("type must be 'Placement', not ");
            error += arg.type().as_string();
            throw Py::TypeError(error);
        }
    } ATTACHERPY_STDCATCH_ATTR;
}

Py::Boolean AttachEnginePy::getReverse(void) const
{
    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        return Py::Boolean(attacher.mapReverse);
    } ATTACHERPY_STDCATCH_ATTR;
}

void AttachEnginePy::setReverse(Py::Boolean arg)
{
    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        attacher.mapReverse = arg.isTrue();
    } ATTACHERPY_STDCATCH_ATTR;
}

Py::Float AttachEnginePy::getParameter(void) const
{
    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        return Py::Float(attacher.attachParameter);
    } ATTACHERPY_STDCATCH_ATTR;
}

void AttachEnginePy::setParameter(Py::Float arg)
{
    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        attacher.attachParameter = (double)arg;
    } ATTACHERPY_STDCATCH_ATTR;
}


Py::List AttachEnginePy::getCompleteModeList(void) const
{
    try {
        Py::List ret;
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        for(int imode = 0   ;   imode < mmDummy_NumberOfModes   ;   imode++){
            ret.append(Py::String(attacher.getModeName(eMapMode(imode))));
        }
        return ret;
    } ATTACHERPY_STDCATCH_ATTR;
}

Py::List AttachEnginePy::getCompleteRefTypeList(void) const
{
    try {
        Py::List ret;
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        for(int irt = 0   ;   irt < rtDummy_numberOfShapeTypes   ;   irt++){
            ret.append(Py::String(attacher.getRefTypeName(eRefType(irt))));
        }
        return ret;
    } ATTACHERPY_STDCATCH_ATTR;
}

Py::List AttachEnginePy::getImplementedModes(void) const
{
    try {
        Py::List ret;
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        for(int imode = 0   ;   imode < mmDummy_NumberOfModes   ;   imode++){
            if(attacher.modeRefTypes[imode].size() > 0){
                ret.append(Py::String(attacher.getModeName(eMapMode(imode))));
            }
        }
        return ret;
    } ATTACHERPY_STDCATCH_ATTR;
}

/**
  * @brief macro ATTACHERPY_STDCATCH_METH: catch for exceptions in method code
  * (returns NULL if an exception is caught). It is a helper to avoid repeating
  * the same error handling code over and over again.
  */
#define ATTACHERPY_STDCATCH_METH \
    catch (Standard_Failure& e) {\
        PyErr_SetString(Part::PartExceptionOCCError, e.GetMessageString());\
        return NULL;\
    } catch (Base::Exception &e) {\
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());\
        return NULL;\
    } catch (const Py::Exception &){\
        return NULL;\
    }


PyObject* AttachEnginePy::getModeInfo(PyObject* args)
{
    char* modeName;
    if (!PyArg_ParseTuple(args, "s", &modeName))
        return 0;

    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        eMapMode mmode = attacher.getModeByName(modeName);
        Py::List pyListOfCombinations;
        Py::List pyCombination;
        refTypeStringList &listOfCombinations = attacher.modeRefTypes.at(mmode);
        for(const refTypeString &combination: listOfCombinations){
            pyCombination = Py::List(combination.size());
            for(std::size_t iref = 0; iref < combination.size(); iref++) {
                pyCombination[iref] = Py::String(AttachEngine::getRefTypeName(combination[iref]));
            }
            pyListOfCombinations.append(pyCombination);
        }
        Py::Dict ret;
        ret["ReferenceCombinations"] = pyListOfCombinations;
#if PY_MAJOR_VERSION >= 3
        ret["ModeIndex"] = Py::Long(mmode);
#else
        ret["ModeIndex"] = Py::Int(mmode);
#endif
        try {
            Py::Module module(PyImport_ImportModule("PartGui"),true);
            if (module.isNull() || !module.hasAttr("AttachEngineResources")) {
                // in v0.14+, the GUI module can be loaded in console mode (but doesn't have all its document methods)
                throw Py::RuntimeError("Gui is not up");//DeepSOIC: wanted to throw ImportError here, but it's not defined, so I don't know...
            }
            Py::Object submod(module.getAttr("AttachEngineResources"));
            Py::Callable method(submod.getAttr("getModeStrings"));
            Py::Tuple arg(2);
            arg.setItem(0, Py::String(this->getAttachEnginePtr()->getTypeId().getName()));
#if PY_MAJOR_VERSION >= 3
            arg.setItem(1, Py::Long(mmode));
#else
            arg.setItem(1, Py::Int(mmode));
#endif
            Py::List strs = method.apply(arg);
            assert(strs.size() == 2);
            ret["UserFriendlyName"] = strs[0];
            ret["BriefDocu"] = strs[1];
        } catch (Py::Exception& e) {
            if (PyErr_ExceptionMatches(PyExc_ImportError)) {
                // the GUI is not up.
                Base::Console().Warning("AttachEngine: Gui not up, so no gui-related entries in getModeInfo.\n");
                e.clear();
            } else {
                Base::Console().Warning("AttachEngine.getModeInfo: error obtaining GUI strings\n");
                e.clear();
            }
        } catch (Base::Exception &e){
            Base::Console().Warning("AttachEngine.getModeInfo: error obtaining GUI strings:");
            Base::Console().Warning(e.what());
            Base::Console().Warning("\n");
        }

        return Py::new_reference_to(ret);
    } ATTACHERPY_STDCATCH_METH;
}

PyObject* AttachEnginePy::getRefTypeOfShape(PyObject* args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapePy::Type), &pcObj))
        return NULL;

    try{
        TopoDS_Shape shape = static_cast<Part::TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
        eRefType rt = AttachEngine::getShapeType(shape);
        return Py::new_reference_to(Py::String(AttachEngine::getRefTypeName(rt)));
    } ATTACHERPY_STDCATCH_METH;
}

PyObject* AttachEnginePy::isFittingRefType(PyObject* args)
{
    char* type_shape_str;
    char* type_need_str;
    if (!PyArg_ParseTuple(args, "ss", &type_shape_str, &type_need_str))
        return 0;
    try {
        eRefType type_shape = AttachEngine::getRefTypeByName(std::string(type_shape_str));
        eRefType type_need = AttachEngine::getRefTypeByName(std::string(type_need_str));
        bool result = AttachEngine::isShapeOfType(type_shape, type_need) > -1;
        return Py::new_reference_to(Py::Boolean(result));
    } ATTACHERPY_STDCATCH_METH;
}

PyObject* AttachEnginePy::downgradeRefType(PyObject* args)
{
    char* type_shape_str;
    if (!PyArg_ParseTuple(args, "s", &type_shape_str))
        return 0;
    try {
        eRefType type_shape = AttachEngine::getRefTypeByName(std::string(type_shape_str));
        eRefType result = AttachEngine::downgradeType(type_shape);
        return Py::new_reference_to(Py::String(AttachEngine::getRefTypeName(result)));
    } ATTACHERPY_STDCATCH_METH;
}

PyObject* AttachEnginePy::getRefTypeInfo(PyObject* args)
{
    char* typeName;
    if (!PyArg_ParseTuple(args, "s", &typeName))
        return 0;

    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        eRefType rt = attacher.getRefTypeByName(typeName);
        Py::Dict ret;
#if PY_MAJOR_VERSION >= 3
        ret["TypeIndex"] = Py::Long(rt);
        ret["Rank"] = Py::Long(AttachEngine::getTypeRank(rt));
#else
        ret["TypeIndex"] = Py::Int(rt);
        ret["Rank"] = Py::Int(AttachEngine::getTypeRank(rt));
#endif

        try {
            Py::Module module(PyImport_ImportModule("PartGui"),true);
            if (module.isNull() || !module.hasAttr("AttachEngineResources")) {
                // in v0.14+, the GUI module can be loaded in console mode (but doesn't have all its document methods)
                throw Py::RuntimeError("Gui is not up");//DeepSOIC: wanted to throw ImportError here, but it's not defined, so I don't know...
            }
            Py::Object submod(module.getAttr("AttachEngineResources"));
            Py::Callable method(submod.getAttr("getRefTypeUserFriendlyName"));
            Py::Tuple arg(1);
#if PY_MAJOR_VERSION >= 3
            arg.setItem(0, Py::Long(rt));
#else
            arg.setItem(0, Py::Int(rt));
#endif
            Py::String st = method.apply(arg);
            ret["UserFriendlyName"] = st;
        } catch (Py::Exception& e) {
            if (PyErr_ExceptionMatches(PyExc_ImportError)) {
                // the GUI is not up.
                Base::Console().Warning("AttachEngine: Gui not up, so no gui-related entries in getModeInfo.\n");
                e.clear();
            } else {
                Base::Console().Warning("AttachEngine.getRefTypeInfo: error obtaining GUI strings\n");
                e.clear();
            }
        } catch (Base::Exception &e){
            Base::Console().Warning("AttachEngine.getRefTypeInfo: error obtaining GUI strings:");
            Base::Console().Warning(e.what());
            Base::Console().Warning("\n");
        }

        return Py::new_reference_to(ret);
    } ATTACHERPY_STDCATCH_METH;
}

PyObject* AttachEnginePy::copy(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    return new AttachEnginePy(this->getAttachEnginePtr()->copy());
}

PyObject* AttachEnginePy::calculateAttachedPlacement(PyObject* args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(Base::PlacementPy::Type), &pcObj))
        return NULL;
    try{
        const Base::Placement& plm = *(static_cast<const Base::PlacementPy*>(pcObj)->getPlacementPtr());
        Base::Placement result;
        try{
            result = this->getAttachEnginePtr()->calculateAttachedPlacement(plm);
        } catch (ExceptionCancel&) {
            Py_IncRef(Py_None);
            return Py_None;
        }
        return new Base::PlacementPy(new Base::Placement(result));
    } ATTACHERPY_STDCATCH_METH;

    return NULL;
}

PyObject* AttachEnginePy::suggestModes(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        SuggestResult sugr;
        attacher.suggestMapModes(sugr);
        Py::Dict result;
        { //sugr.allApplicableModes
            Py::List pyList;
            for(eMapMode mmode: sugr.allApplicableModes){
                pyList.append(Py::String(AttachEngine::getModeName(mmode)));
            }
            result["allApplicableModes"] = pyList;
        }
        { //sugr.bestFitMode
            result["bestFitMode"] = Py::String(AttachEngine::getModeName(sugr.bestFitMode));
        }
        {//sugr.error
            bool isError = sugr.message == SuggestResult::srUnexpectedError
                    || sugr.message == SuggestResult::srLinkBroken;
            result["error"] = Py::String(isError ? sugr.error.what() : "");
        }
        {//sugr.message
            std::string msg;
            switch(sugr.message){
            case SuggestResult::srIncompatibleGeometry:
                msg = "IncompatibleGeometry";
            break;
            case SuggestResult::srLinkBroken:
                msg = "LinkBroken";
            break;
            case SuggestResult::srNoModesFit:
                msg = "NoModesFit";
            break;
            case SuggestResult::srOK:
                msg = "OK";
            break;
            case SuggestResult::srUnexpectedError:
                msg = "UnexpectedError";
            break;
            default:
                msg = "<message index out of range>";
            }
            result["message"] = Py::String(msg);
        }
        {//sugr.nextRefTypeHint
            Py::List pyList;
            for(eRefType rt : sugr.nextRefTypeHint){
                pyList.append(Py::String(AttachEngine::getRefTypeName(rt)));
            }
            result["nextRefTypeHint"] = pyList;
        }
        {//sugr.reachableModes
            Py::Dict pyRM;
            for(std::pair<const eMapMode, refTypeStringList> &rm: sugr.reachableModes){
                Py::List pyListOfCombinations;
                for(refTypeString &rts : rm.second){
                    Py::List pyCombination;
                    for(eRefType rt : rts){
                        pyCombination.append(Py::String(AttachEngine::getRefTypeName(rt)));
                    }
                    pyListOfCombinations.append(pyCombination);
                }
                pyRM[AttachEngine::getModeName(rm.first)] = pyListOfCombinations;
            }
            result["reachableModes"] = pyRM;
        }
        {//sugr.references_Types
            Py::List pyList;
            for(eRefType rt : sugr.references_Types){
                pyList.append(Py::String(AttachEngine::getRefTypeName(rt)));
            }
            result["references_Types"] = pyList;
        }

        return Py::new_reference_to(result);
    } ATTACHERPY_STDCATCH_METH;
}

PyObject* AttachEnginePy::readParametersFromFeature(PyObject* args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O!",&(App::DocumentObjectPy::Type),&obj))
        return NULL;    // NULL triggers exception

    try{
        App::DocumentObjectPy* dobjpy = static_cast<App::DocumentObjectPy*>(obj);
        App::DocumentObject* dobj = dobjpy->getDocumentObjectPtr();
        if (! dobj->hasExtension(Part::AttachExtension::getExtensionClassTypeId())){
            throw Py::TypeError("Supplied object has no Part::AttachExtension");
        }
        Part::AttachExtension* feat = dobj->getExtensionByType<Part::AttachExtension>();
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        attacher.setUp(feat->Support,
                       eMapMode(feat->MapMode.getValue()),
                       feat->MapReversed.getValue(),
                       feat->MapPathParameter.getValue(),
                       0.0,0.0,
                       feat->AttachmentOffset.getValue());
        return Py::new_reference_to(Py::None());
    } ATTACHERPY_STDCATCH_METH;
}

PyObject* AttachEnginePy::writeParametersToFeature(PyObject* args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O!",&(App::DocumentObjectPy::Type),&obj))
        return NULL;    // NULL triggers exception

    try{
        App::DocumentObjectPy* dobjpy = static_cast<App::DocumentObjectPy*>(obj);
        App::DocumentObject* dobj = dobjpy->getDocumentObjectPtr();
        if (! dobj->hasExtension(Part::AttachExtension::getExtensionClassTypeId())){
            throw Py::TypeError("Supplied object has no Part::AttachExtension");
        }
        Part::AttachExtension* feat = dobj->getExtensionByType<Part::AttachExtension>();
        const AttachEngine &attacher = *(this->getAttachEnginePtr());
        AttachEngine::verifyReferencesAreSafe(attacher.references);
        feat->Support.Paste(attacher.references);
        feat->MapMode.setValue(attacher.mapMode);
        feat->MapReversed.setValue(attacher.mapReverse);
        feat->MapPathParameter.setValue(attacher.attachParameter);
        feat->AttachmentOffset.setValue(attacher.attachmentOffset);
        return Py::new_reference_to(Py::None());
    } ATTACHERPY_STDCATCH_METH;
}

PyObject* AttachEnginePy::getCustomAttributes(const char*) const
{
    return 0;
}

int AttachEnginePy::setCustomAttributes(const char*,PyObject*)
{
    return 0;
}

