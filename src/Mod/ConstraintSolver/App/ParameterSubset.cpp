#include "PreCompiled.h"

#include "ParameterSubset.h"
#include "ParameterSubsetPy.h"
#include "ParameterRef.h"
#include "ParameterStore.h"

#include <Base/Console.h>

using namespace FCS;

FCS::ParameterSubset::ParameterSubset(int prealloc)
    : _host(Py::None())
{
    this->_params.reserve(size_t(prealloc));
}

void FCS::ParameterSubset::attach(FCS::HParameterStore store)
{
    _host = store;
    _host->onNewSubset(self());
    onStoreExpand();
}

void ParameterSubset::detach()
{
    if (host().isNone())
        return;
    _host->onDeletedSubset(this);
    _host.makeNone();
}

void ParameterSubset::onStoreExpand()
{
    _lut.resize(host()->size(), -1);
}

bool ParameterSubset::checkParameter(const ParameterRef& param) const
{
    if (host().isNone())
        return false;
    return host()->has(param);
}

HParameterSubset ParameterSubset::make(std::vector<ParameterRef> params)
{
    if (params.size() == 0)
        throw Base::ValueError("Can't construct on empty parameter list, use another constructor to construct an empty set.");
    HParameterSubset s = make(params[0].host(), params.size());
    s->add(params);
    return s;
}


HParameterSubset ParameterSubset::make(HParameterStore store, int prealloc)
{
    ParameterSubset* obj = new ParameterSubset(prealloc);
    PyObject* pyobj = new ParameterSubsetPy(obj);
    obj->_twin = pyobj;
    obj->attach(store);
    return HParameterSubset(pyobj, /*new_reference=*/true);
}

HParameterSubset ParameterSubset::copy() const
{
    HParameterSubset cpy = make(this->host());
    if (size() > 0){
        cpy->_lut = _lut;
        cpy->_params = _params;
    }
    return cpy;
}

ParameterSubset::~ParameterSubset()
{
    detach();
}

FCS::HParameterStore FCS::ParameterSubset::host() const
{
    return _host;
}

int ParameterSubset::size() const
{
    return _params.size();
}

bool ParameterSubset::add(ParameterRef param)
{
    if (host().isNone()){
        attach(param.host());
    }
    if (!checkParameter(param))
        throw Base::ValueError("Can't add a parameter from a different store");
    if (indexOf(param) != -1)
        return false; //already added
    _params.push_back(param);
    std::vector<ParameterRef> grp = host()->getEqualityGroup(param);
    _lut[param.masterIndex()] = int(_params.size()) - 1;
    return true;
}

int ParameterSubset::add(const std::vector<ParameterRef>& params)
{
    int cnt = 0;
    for(const ParameterRef& r : params){
        cnt += add(r);
    }
    return cnt;
}

bool ParameterSubset::remove(ParameterRef param)
{
    if (! has(param))
        return false;

    if(size() == 1){
        clear();
        return true;
    }

    int i_param = indexOf(param);

    _params.erase(_params.begin() + i_param);

    //update lookup table
    _lut[param.masterIndex()] = -1;
    for (int& i : _lut){
        if (i > i_param)
            --i;
    }
    return true;
}

void ParameterSubset::clear()
{
    _lut.assign(_lut.size(), -1);
    _params.clear();
    detach();
}

bool ParameterSubset::has(ParameterRef param) const
{
    if (! checkParameter(param))
        return false;
    return indexOf(param) != -1;
}

bool ParameterSubset::has(const HParameterSubset other) const
{
    for(const ParameterRef& p : other->list()){
        if (! has(p))
            return false;
    }
    return true;
}

bool ParameterSubset::in(const HParameterSubset other) const
{
    return other->has(self());
}

int ParameterSubset::indexOf(const ParameterRef& param) const
{
    assert(checkParameter(param));
    if(size() == 0)
        return -1; //a guard against when host is still None
    return _lut[param.masterIndex()];
}

ParameterRef ParameterSubset::operator[](int index) const
{
    return _params[index];
}

HParameterSubset ParameterSubset::self() const
{
    return HParameterSubset(_twin, false);
}

PyObject* ParameterSubset::getPyObject()
{
    return Py::new_reference_to(self().getHandledObject());
}
