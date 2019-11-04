#include "PreCompiled.h"
#include "ParameterStore.h"
#include <ParameterStorePy.h>
#include <Base/Console.h>

using namespace GCS;

TYPESYSTEM_SOURCE(GCS::ParameterStore, Base::BaseClass);

ParameterStore::ParameterStore(int prealloc)
{
    _params.reserve(size_t(prealloc));
}

void ParameterStore::on_added(int old_sz, int new_sz)
{
    for(int i = old_sz; i < new_sz; ++i){
        Parameter& p =_params[i];
        p._ownIndex = i;
        p._masterIndex = i;
    }
}

HParameterStore ParameterStore::make(int prealloc) {
    ParameterStore* obj = new ParameterStore(prealloc);
    PyObject* pyobj = new ParameterStorePy(obj);
    obj->_twin = pyobj;
    return HParameterStore(pyobj, /*new_reference=*/true);
}

ParameterStore::~ParameterStore()
{
    Base::Console().Warning("destruct ParameterStore\n");
}

ParameterRef ParameterStore::add()
{
    int old_sz = _params.size();
    int new_sz = old_sz + 1;
    _params.resize(new_sz);
    on_added(old_sz, new_sz);
    return (*this)[new_sz - 1];
}

ParameterRef ParameterStore::add(const Parameter& p)
{
    int old_sz = _params.size();
    int new_sz = old_sz + 1;
    _params.resize(new_sz);
    _params[new_sz - 1] = p;
    on_added(old_sz, new_sz);
    return (*this)[new_sz - 1];
}

std::vector<ParameterRef> ParameterStore::add(int count)
{
    int old_sz = _params.size();
    int new_sz = old_sz + count;
    _params.resize(new_sz);
    on_added(old_sz, new_sz);
    std::vector<ParameterRef> ret;
    ret.reserve(new_sz - old_sz);
    for(int i = old_sz; i < new_sz; ++i){
        ret.push_back((*this)[i]);
    }
    return ret;
}

std::vector<ParameterRef> ParameterStore::add(const std::vector<Parameter>& pp)
{
    int old_sz = _params.size();
    int new_sz = old_sz + pp.size();
    _params.resize(new_sz);
    on_added(old_sz, new_sz);
    std::vector<ParameterRef> ret;
    ret.reserve(new_sz - old_sz);
    for(int i = old_sz; i < new_sz; ++i){
        _params[i].pasteFrom(pp[i - old_sz]);
        ret.push_back((*this)[i]);
    }
    return ret;

}

int ParameterStore::size() const {return _params.size();}

void ParameterStore::resize(int newSize)
{
    if(newSize < size()){
        throw Base::ValueError("ParameterStore can't shrink (as ParameterRefs will become invalid).");
    }
}

ParameterRef ParameterStore::operator[](int index) const
{
    return ParameterRef(getPyHandle(), index);
}

double& ParameterStore::value(int index){
    return _params[size_t(index)].value;
}

double ParameterStore::value(int index) const {
    return _params[size_t(index)].value;
}

HParameterStore ParameterStore::getPyHandle() const {
    return HParameterStore(_twin, /*new_reference = */false);
}

