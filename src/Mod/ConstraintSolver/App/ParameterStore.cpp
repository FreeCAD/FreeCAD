#include "PreCompiled.h"
#include "ParameterStore.h"
#include <ParameterStorePy.h>
#include "ParameterSubset.h"
#include "ValueSet.h"
#include <Base/Console.h>

using namespace FCS;

TYPESYSTEM_SOURCE(FCS::ParameterStore, Base::BaseClass);

ParameterStore::ParameterStore(int prealloc)
    : _trivialValueSet(Py::None())
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
    for(ParameterSubset* p : _subsets){
        p->onStoreExpand();
    }
}

void ParameterStore::onNewSubset(HParameterSubset ss)
{
    _subsets.insert(ss.cptr());
}

void ParameterStore::onDeletedSubset(ParameterSubset* ss)
{
    _subsets.erase(ss);
}

HParameterStore ParameterStore::make(int prealloc) {
    ParameterStore* obj = new ParameterStore(prealloc);
    PyObject* pyobj = new ParameterStorePy(obj);
    obj->_twin = pyobj;
    return HParameterStore(pyobj, /*new_reference=*/true);
}

ParameterStore::~ParameterStore()
{
}

HParameterStore ParameterStore::copy()
{
    HParameterStore cpy = ParameterStore::make(size());
    cpy->_params = this->_params;
    return cpy;
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

bool ParameterStore::has(ParameterRef param) const
{
    return param.host().is(self());
}

int ParameterStore::size() const {return _params.size();}

void ParameterStore::resize(int newSize)
{
    if(newSize < size()){
        throw Base::ValueError("ParameterStore can't shrink (as ParameterRefs will become invalid).");
    }
    add(newSize - size());
}

ParameterRef ParameterStore::operator[](int index) const
{
    //FIXME: range-check, maybe? watch out the usage in method "end".
    return ParameterRef(self(), index);
}

double& ParameterStore::value(int index){
    return _params[size_t(index)].savedValue;
}

double ParameterStore::value(int index) const {
    return _params[size_t(index)].savedValue;
}

void ParameterStore::redirect(ParameterRef who, ParameterRef where, bool mean_out)
{
    //first, make sure "who" is master parameter
    who = who.master();
    //there might be some other parameters redirected onto who, we'll call "hair". They need to be redirected too.
    std::vector<ParameterRef> grpWho = getEqualityGroup(who);
    if (mean_out){
        if (where.isFixed()) {
            who.savedValue() = where.savedValue();
        } else if (who.isFixed()) {
            where.savedValue() = who.savedValue();
        } else { //none fixed
            double weight_who = grpWho.size();
            double weight_where = getEqualityGroup(where.master()).size();
            double mean = (who.savedValue() * weight_who + where.savedValue() * weight_where) / (weight_who + weight_where);
            who.savedValue() = mean;
            where.savedValue() = mean;
        }
    }
    if (inSubsets(who) || inSubsets(where))
        Base::Console().Warning("Redirection when there are ParameterSets around are likely to break the sets.\n");
    for (ParameterRef &p : grpWho){
        p.param()._masterIndex = where.masterIndex();
    }

}

std::vector<ParameterRef> ParameterStore::getEqualityGroup(ParameterRef param)
{
    std::vector<ParameterRef> ret;
    for(Parameter &p : _params){
        if (p.masterIndex() == param.masterIndex()){
            ret.push_back( (*this)[p.ownIndex()]);
        }
    }
    return ret;
}

ParameterStore::eConstrainEqual_Result ParameterStore::constrainEqual(ParameterRef param1, ParameterRef param2, bool mean_out)
{
    if (param1.masterIndex() == param2.masterIndex())
        return eConstrainEqual_Result::Redundant;

    auto ret = eConstrainEqual_Result::Constrained;
    if (param1.isFixed() && param2.isFixed()){
        if (fabs(param1.savedValue() - param2.savedValue()) < 1e-12 * std::max(param1.masterScale(), param2.masterScale())){
            ret = eConstrainEqual_Result::Redundant;
        } else {
            throw Base::ValueError("Conflicting constraint");
        }
    }
    if(param1.isFixed())
        redirect(param2, param1, mean_out);
    else//param2 fixed or both variable
        redirect(param1, param2, mean_out);
    return ret;
}

void ParameterStore::deconstrain()
{
    if (! _subsets.empty())
        Base::Console().Warning("Redirection when there are ParameterSets around are likely to break the sets.\n");
    sync();
    for (Parameter& p : _params){
        p._masterIndex = p.ownIndex();
    }
}

void ParameterStore::deconstrain(ParameterRef param)
{
    if (this->inSubsets(param))
        Base::Console().Warning("Redirection when there are ParameterSets around are likely to break the sets.\n");
    sync(param);
    param.param()._masterIndex = param.param().ownIndex();
}

void ParameterStore::sync()
{
    for (Parameter& p : _params){
        p.savedValue = _params[p.masterIndex()].savedValue;
    }
}

void ParameterStore::sync(ParameterRef param)
{
    param.ownSavedValue() = param.savedValue();
}

void ParameterStore::fix(ParameterRef param)
{
    std::vector<ParameterRef> grp = getEqualityGroup(param);
    param.ownFixed() = true;
    if (! param.isFixed() && grp.size() > 1){
        //the whole group should become fixed. To avoid changing the flag of master,
        //we redirect the group to use this parameter as master.
        if (inSubsets(grp[0]))
            Base::Console().Warning("Redirection when there are ParameterSets around are likely to break the sets.\n");
        for (ParameterRef &p : grp){
            p.param()._masterIndex = param.ownIndex();
        }
    }
}

bool ParameterStore::inSubsets(ParameterRef param) const
{
    for(ParameterSubset* ss : _subsets){
        if(ss->has(param))
            return true;
    }
    return false;
}

std::vector<ParameterRef> ParameterStore::allFree() const
{
    std::vector<ParameterRef> ret;
    for(ParameterRef r : *this){
        if (r.isMaster() && ! r.isFixed())
            ret.push_back(r);
    }
    return ret;
}

std::vector<ParameterRef> ParameterStore::allFixed() const
{
    std::vector<ParameterRef> ret;
    for(ParameterRef r : *this){
        if (r.isFixed())
            ret.push_back(r);
    }
    return ret;
}

std::vector<ParameterRef> ParameterStore::allDriven() const
{
    std::vector<ParameterRef> ret;
    for(ParameterRef r : *this){
        if (!r.isMaster())
            ret.push_back(r);
    }
    return ret;
}

int ParameterStore::dofCount() const
{
    int ret = 0;
    for(ParameterRef r : *this){
        if (r.isMaster() && ! r.isFixed())
            ++ret;
    }
    return ret;
}

PyObject* ParameterStore::getPyObject()
{
    return Py::new_reference_to(self()->getPyObject());
}

HParameterStore ParameterStore::self() const {
    return HParameterStore(_twin, /*new_reference = */false);
}

const ValueSet& ParameterStore::asValueSet()
{
    if (_trivialValueSet.isNone())
        _trivialValueSet = ValueSet::makeTrivial(self());
    return *_trivialValueSet;
}

ParameterStore::const_iterator ParameterStore::begin() const{
    return const_iterator(*this, 0);
}

ParameterStore::const_iterator ParameterStore::end() const{
    return const_iterator(*this, size());
}


ParameterStore::const_iterator::const_iterator(const ParameterStore& host, int index)
    : host(host.self()), index(index)
{
}

ParameterRef ParameterStore::const_iterator::operator*()
{
    return (*(this->host))[index];
}
