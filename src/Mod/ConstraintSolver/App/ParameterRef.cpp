#include "PreCompiled.h"

#include "ParameterRef.h"
#include "ParameterRefPy.h"
#include <Base/Exception.h>
#include <Base/Console.h>

using namespace GCS;

ParameterRef::ParameterRef(HParameterStore st, int index)
    : _store(st), _ownIndex(index)
{
    if(index < 0 || index > st->size())
        throw Base::ValueError("Parameter index is out of bounds");
}

ParameterRef::~ParameterRef()
{
    Base::Console().Warning("destruct ParameterRef\n");
}

int ParameterRef::masterIndex() const {
    return _store->_params[_ownIndex].masterIndex();
}

double& ParameterRef::ownValue() const {
    return _store->_params[_ownIndex].value;
}

double& ParameterRef::value() const {
    return _store->_params[masterIndex()].value;
}

double& ParameterRef::ownScale() const {
    return _store->_params[_ownIndex].scale;
}

double& ParameterRef::masterScale() const {
    return _store->_params[masterIndex()].scale;
}

Parameter& ParameterRef::param() const {
    return _store->_params[_ownIndex];
}

ParameterRef ParameterRef::masterParam() const {
    return ParameterRef(_store, masterIndex());
}

bool ParameterRef::isSameRef(const ParameterRef& other) const {
    return &(param()) == &(other.param());
}

bool ParameterRef::isSameValue(const ParameterRef& other) const {
    return (_store == other._store)
            && (masterIndex() == other.masterIndex());
}

UnsafePyHandle<ParameterRef> ParameterRef::getPyObject() const
{
    UnsafePyHandle<ParameterRef> ret(new ParameterRefPy(new ParameterRef(*this)), /*new_reference=*/true);
    Base::Console().Warning("ref cnt: %i", int( ret.reference_count()));
    return ret;
}
