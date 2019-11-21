#include "PreCompiled.h"

#include "ParaObject.h"
#include "ParaObjectPy.h"

#include "ParameterSubset.h"
#include "ParameterRefPy.h"

#include "unordered_set"

using namespace FCS;

TYPESYSTEM_SOURCE_ABSTRACT(FCS::ParaObject, Base::BaseClass);


PyObject* ParaObject::getPyObject()
{
    if (_twin == nullptr){
        new ParaObjectPy(this);
        assert(_twin);
        return _twin;
    } else {
        return Py::new_reference_to(_twin);
    }
}

HParaObject ParaObject::self()
{
    return HParaObject(getPyObject(), true);
}

void ParaObject::update()
{
    _parameters.clear();
    std::unordered_set<int> added;

    auto add = [&](const ParameterRef& v){
        v.throwNull();
        if (added.find(v.masterIndex()) != added.end())
            return;
        _parameters.push_back(v);
        added.insert(v.masterIndex());
    };

    for(auto& v : this->_attrs){
        add(*(v.value));
    };
    for(auto& v : this->_children){
        ParaObject& child = *HParaObject(*(v.value));
        if (child._touched)
            child.update();
        for(const ParameterRef& r : child.parameters()){
            add(r);
        };
    };

    _touched = false;
}

HParaObject ParaObject::copy() const
{
    HParaObject cpy(
        static_cast<ParaObject*>(
            getTypeId().createInstance()
        )->getPyObject()
        , true
    );

    //copy parameter references
    for(int i = 0; i < _attrs.size(); ++i){
        assert(cpy->_attrs[i].name == _attrs[i].name);
        *(cpy->_attrs[i].value) = *(_attrs[i].value);
    };
    //copy references to children
    for(int i = 0; i < _children.size(); ++i){
        assert(cpy->_attrs[i].name == _attrs[i].name);
        *(cpy->_children[i].value) = *(_children[i].value);
    };

    cpy->_parameters = _parameters;
    cpy->_touched = _touched;

    return cpy;
}

std::vector<ParameterRef> ParaObject::makeParameters(HParameterStore into)
{
    touch();
    std::vector<ParameterRef> ret;
    for(auto& v : this->_attrs){
        if (! v.value->isNull())
            continue;
        ParameterRef newp = into->add(Parameter(label + "." + v.name, v.defvalue));
        ret.push_back(newp);
        *v.value = newp;
    };
    for(auto& v : this->_children){
        if (!v.value->isNone())
            continue;
        if (!v.make)
            continue;
        HParaObject child (_PyObject_New(v.type), true);
        *v.value = child;
        child->label = this->label + "." + v.name;
        extend(ret, child->makeParameters(into));
    }
    return ret;
}

bool ParaObject::isComplete() const
{
    try {
        throwIfIncomplete();
    } catch (Base::ReferencesError &e) {
        return false;
    }
    return true;
}

void ParaObject::throwIfIncomplete() const
{
    for(auto& v : this->_attrs){
        if (v.value->isNull()){
            throw Base::ReferencesError("Parameter " + v.name + " is null");
        }
    };
    for(auto& v : this->_children){
        if (v.value->isNone()){
            throw Base::ReferencesError("Child reference " + v.name + " is null");
        }
        HParaObject(*v.value)->throwIfIncomplete();
    };
}

Py::Object ParaObject::getAttr(const char* attrname)
{
    for(auto& v : this->_attrs){
        if (v.name == attrname)
            return v.value->getPyHandle();
    };
    for(auto& v : this->_children){
        if (v.name == attrname)
            return HParaObject(* v.value)->self();
    };
    std::stringstream ss;
    ss << self().repr().as_std_string() << " has no attribute "
       << attrname;
    throw Py::AttributeError(ss.str());
}

void ParaObject::setAttr(std::string attrname, Py::Object val)
{
    for(auto& v : this->_attrs){
        if (v.name == attrname){
            if (!PyObject_TypeCheck(val.ptr(), &ParameterRefPy::Type)){
                std::stringstream ss;
                ss << "Must be ParameterRef object, not " << val.type().as_string();
                throw Py::TypeError(ss.str());
            }
            *(v.value) = *HParameterRef(val);
            touch();
            return;
        }
    };
    for(auto& v : this->_children){
        if (v.name == attrname){
            if (!PyObject_TypeCheck(val.ptr(), v.type)){
                std::stringstream ss;
                ss << "Must be "<< v.type->tp_name <<" object, not " << val.type().as_string();
                throw Py::TypeError(ss.str());
            }
            *(v.value) = val;
            touch();
            return;
        }
    };
    std::stringstream ss;
    ss << self().repr().as_std_string() << " has no attribute "
       << attrname;
    throw Py::AttributeError(ss.str());
}

std::vector<std::string> ParaObject::listAttrs() const
{
    std::vector<std::string> ret;

    for(auto& v : this->_attrs){
        ret.push_back(v.name);
    };
    for(auto& v : this->_children){
        ret.push_back(v.name);
    };
    return ret;
}
