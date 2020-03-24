#include "PreCompiled.h"

#include "ParaObject.h"
#include "ParaObjectPy.h"

#include "ParameterSubset.h"
#include "ParameterRefPy.h"
#include "ParameterStorePy.h"
#include "ParaGeometryPy.h"
#include "PyUtils.h"

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

std::string ParaObject::repr() const
{
    std::stringstream ss;
    ss << "<" ;
    if (label.size() == 0)
        ss << "unlabeled ";
    ss << getTypeId().getName() << " object";
    if (label.size() != 0)
        ss << " '" << label << "'";
    ss << ">";
    return ss.str();
}

void ParaObject::update()
{
    throwIfIncomplete();
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
        if (!v.required && v.value->isNull())
            continue;
        add(*(v.value));
    };
    for(auto& v : this->_children){
        if (v.value->isNone())
            continue;
        ParaObject& child = *HParaObject(*(v.value));
        if (child._touched)
            child.update();
        for(const ParameterRef& r : child.parameters()){
            add(r);
        };
    };
    forEachShape([&](const ShapeRef& v){
        if (v.value->isNone())
            return;
        ParaObject& sh = *HParaObject(*(v.value));
        if (sh._touched)
            sh.update();
        for(const ParameterRef& r : sh.parameters()){
            add(r);
        };
    });

    _touched = false;
}

void ParaObject::throwIfLocked() const
{
    if (_locked)
        throw Py::RuntimeError(repr() + " is locked");
}

HParaObject ParaObject::copy() const
{
    HParaObject cpy =
        static_cast<ParaObject*>(
            getTypeId().createInstance()
        );

    //copy parameter references
    for(size_t i = 0; i < _attrs.size(); ++i){
        assert(cpy->_attrs[i].name == _attrs[i].name);
        *(cpy->_attrs[i].value) = *(_attrs[i].value);
    };
    //copy references to children
    for(size_t i = 0; i < _children.size(); ++i){
        assert(cpy->_children[i].name == _children[i].name);
        *(cpy->_children[i].value) = *(_children[i].value);
    };
    //copy references to shapes
    for(size_t i = 0; i < _shapes.size(); ++i){
        assert(cpy->_shapes[i].name == _shapes[i].name);
        *(cpy->_shapes[i].value) = *(_shapes[i].value);
    };

    cpy->_parameters = _parameters;
    cpy->_touched = _touched;
    cpy->_locked = false;

    return cpy;
}

std::vector<ParameterRef> ParaObject::makeParameters(HParameterStore into)
{
    throwIfLocked(); touch();
    std::vector<ParameterRef> ret;
    for(auto& v : this->_attrs){
        if (! v.make)
            continue;
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
        //create object (by calling the type object)
        HParaObject child = HParaObject(
            Py::Callable(reinterpret_cast<PyObject*>(v.type)).apply(Py::Tuple()));
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
    } catch (Py::Exception &e) {
        if (PyErr_ExceptionMatches(PyExc_LookupError)){
            e.clear();
            return false;
        } else {
            throw;
        }
    }
    return true;
}

void ParaObject::throwIfIncomplete() const
{
    for(auto& v : this->_attrs){
        if (v.required && v.value->isNull()){
            throw Py::Exception(PyExc_LookupError,"Parameter '" + v.name + "' of " + repr() + " is null");
        }
    };
    for(auto& v : this->_children){
        if (v.required && v.value->isNone()){
            throw Py::Exception(PyExc_LookupError,"Child reference '" + v.name + "' of " + repr() + " is None");
        }
        if (!v.value->isNone())
            HParaObject(*v.value)->throwIfIncomplete(); //#FIXME: can it be a performance problem?
    };
    throwIfIncomplete_Shapes();
}

void ParaObject::throwIfIncomplete_Shapes() const
{
    forEachShape([&](const ShapeRef& it){
        if (it.value->isNone()){
            throw Py::Exception(PyExc_LookupError,"Shape reference '" + it.name + "' of " + repr() + " is None");
        }
        HParaObject(*it.value)->throwIfIncomplete(); //#FIXME: can it be a performance problem?
    });

}

Py::Object ParaObject::getAttr(const char* attrname)
{
    for(auto& v : this->_attrs){
        if (v.name == attrname)
            return v.value->getPyHandle().getHandledObject();
    };
    for(auto& v : this->_children){
        if (v.name == attrname)
            return (*v.value).getHandledObject();
    };
    for(auto& v : this->_shapes){
        if (v.name == attrname)
            return (*v.value).getHandledObject();
    };
    std::stringstream ss;
    ss << repr() << " has no attribute "
       << attrname;
    throw Py::AttributeError(ss.str());
}

void ParaObject::setAttr(std::string attrname, Py::Object val)
{
    for(auto& v : this->_attrs){
        if (v.name == attrname){
            throwIfLocked();
            if (!PyObject_TypeCheck(val.ptr(), &ParameterRefPy::Type)){
                std::stringstream ss;
                ss << "Must be ParameterRef object, not " << val.type().as_string();
                throw Py::TypeError(ss.str());
            }
            touch();
            *(v.value) = *HParameterRef(val);
            return;
        }
    };
    for(auto& v : this->_children){
        if (v.name == attrname){
            if (v.writeOnce && !v.value->isNone()){
                throw Py::RuntimeError("Attribute " + v.name + " of " + repr() + " is write-once, can't overwrite");
            }
            throwIfLocked();
            if (!PyObject_TypeCheck(val.ptr(), v.type)){
                std::stringstream ss;
                ss << "Must be "<< v.type->tp_name <<" object, not " << val.type().as_string();
                throw Py::TypeError(ss.str());
            }
            touch();
            *(v.value) = HParaObject(val);;
            return;
        }
    };
    for(auto& v : this->_shapes){
        if (v.name == attrname){
            throwIfLocked();
            if (PyObject_TypeCheck(val.ptr(), &ParaGeometryPy::Type)){
                touch();
                HParaGeometry g = HParaGeometry(val);
                if (!g->isDerivedFrom(v.type)){
                    std::stringstream ss;
                    ss << "Geometry must be derived from " << v.type.getName()
                       << " (got unsuitsable " << g->getTypeId().getName() << ")";
                    throw Py::TypeError(ss.str());
                }
                *(v.value) = g->toShape();
                return;
            }
            else if (PyObject_TypeCheck(val.ptr(), &ParaObjectPy::Type)){
                touch();
                HParaObject sh = HParaObject(val);
                if (sh->shapeType() == Base::Type::badType())
                    throw Py::TypeError(std::string("Object ") + sh->repr() + " is not a shape or geometry");
                if (! sh->shapeType().isDerivedFrom(v.type)){
                    std::stringstream ss;
                    ss << "Shape must be derived from " << v.type.getName()
                       << " (got unsuitsable " << sh->shapeType().getName() << ")";
                    throw Py::TypeError(ss.str());
                }

                *(v.value) = HParaObject(val);
                return;
            }

            std::stringstream ss;
            ss << "Must be ParaObject object, not " << val.type().as_string();
            throw Py::TypeError(ss.str());
        }
    };
    std::stringstream ss;
    ss << getHandle().getHandledObject().repr().as_std_string() << " has no attribute "
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
    for(auto& v : this->_shapes){
        ret.push_back(v.name);
    };
    return ret;
}

void ParaObject::forEachShape(std::function<void (const ShapeRef&)> callback) const
{
    for(auto v : _shapes){
        callback(v);
    }
}

void ParaObject::initFromDict(Py::Dict dict)
{
    for(Py::Object it : dict.items()){
        Py::Tuple tup(it);
        std::string key = Py::String(tup[0]);
        Py::Object val = tup[1];
        if (key == "store")
            continue;//process it last
        FCS::setAttr(getHandle().getHandledObject(), key, val);
    }
    if (dict.hasKey("store")){
        HParameterStore store (dict["store"]);
        if (!PyObject_TypeCheck(store.ptr(), &ParameterStorePy::Type))
            throw Py::TypeError("'store' must be ParameterStore, not "+store.getHandledObject().type().as_string());
        makeParameters(store);
    }
}

void ParaObject::initAttrs() {}

void ParaObject::tieAttr_Parameter(ParameterRef& ref, std::string name, bool make, bool required, double defvalue)
{
    ParameterAttribute tmp;
    tmp.value = &ref;
    tmp.name = name;
    tmp.make = make;
    tmp.required = required;
    tmp.defvalue = defvalue;
    _attrs.push_back(tmp);
}

void ParaObject::tieAttr_Child(HParaObject& ref, std::string name, PyTypeObject* type, bool make, bool required, bool writeOnce)
{
    ChildAttribute tmp;
    tmp.value = &ref;
    tmp.name = name;
    tmp.type = type;
    tmp.make = make;
    tmp.required = required;
    tmp.writeOnce = writeOnce;
    _children.push_back(tmp);
}

void ParaObject::tieAttr_Shape(HParaObject& ref, std::string name, Base::Type type)
{
    ShapeRef tmp;
    tmp.value = &ref;
    tmp.name = name;
    tmp.type = type;
    _shapes.push_back(tmp);
}
