#include "PreCompiled.h"

#include "ParaTransform.h"
#include "G2D/ParaTransformPy.h"
#include "ParaPlacement.h"
#include "ParaShape.h"
#include "PyUtils.h"

#include <unordered_set>
#include <limits>

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ParaTransform, FCS::ParaObject);


void FCS::G2D::ParaTransform::initAttrs()
{
    ParaObject::initAttrs();
}

ParaTransform::ParaTransform()
{
    initAttrs();
}

ParaTransform::ParaTransform(std::vector<HParaPlacement>& fwchain, std::vector<HParaPlacement> revchain)
    : ParaTransform()
{
    _fwchain = fwchain;
    _revchain = revchain;
}

PyObject* ParaTransform::getPyObject()
{
    if (!_twin){
        new ParaTransformPy(this);
        assert(_twin);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }

}

FCS::G2D::Placement FCS::G2D::ParaTransform::value(const ValueSet& vals)
{
    Placement ret;
    for(auto it = _revchain.rbegin(); it != _revchain.rend(); ++it){
        ret *= (**it)(vals).inverse();
    }
    for(const HParaPlacement& plm : _fwchain){
        ret *= (*plm)(vals);
    }
    return ret;
}

void ParaTransform::update()
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

    auto onChild = [&](const HParaObject& child){
        assert(!child.isNone());
        if (child->isTouched())
            child->update();
        for(const ParameterRef& r : child->parameters()){
            add(r);
        };
    };

    for (const HParaPlacement& t : _fwchain){
        onChild(t);
    }
    for (const HParaPlacement& t : _revchain){
        onChild(t);
    }

    _touched = false;
}

HParaObject ParaTransform::copy() const
{
    HParaTransform cpy = ParaObject::copy().downcast<ParaTransform>();
    cpy->_fwchain = _fwchain;
    cpy->_revchain = _revchain;
    return cpy;
}

std::vector<ParameterRef> ParaTransform::makeParameters(HParameterStore into)
{
    throwIfLocked(); touch();
    std::vector<ParameterRef> ret;
    auto onChild = [&](const HParaObject& child){
        extend(ret, child->makeParameters(into));
    };

    for (const HParaPlacement& t : _fwchain){
        onChild(t);
    }
    for (const HParaPlacement& t : _revchain){
        onChild(t);
    }
    return ret;
}

void ParaTransform::throwIfIncomplete() const
{
    auto onChild = [&](const HParaObject& child){
        child->throwIfIncomplete();
    };

    for (const HParaPlacement& t : _fwchain){
        onChild(t);
    }
    for (const HParaPlacement& t : _revchain){
        onChild(t);
    }

}

void ParaTransform::initFromDict(Py::Dict dict)
{
    for(Py::Object it : dict.items()){
        Py::Tuple tup(it);
        std::string key = Py::String(tup[0]);
        Py::Object val = tup[1];
        FCS::setAttr(self().getHandledObject(), key, val);
    }
}

void ParaTransform::set(const std::vector<HParaPlacement>& fwchain, const std::vector<HParaPlacement>& revchain)
{
    throwIfLocked(); touch();
    _fwchain = fwchain;
    _revchain = revchain;
}

void ParaTransform::add(HParaPlacement plm)
{
    throwIfLocked(); touch();
    _fwchain.push_back(plm);
}

void ParaTransform::add(HParaTransform plm)
{
    plm->throwIfHasInverse();
    throwIfLocked(); touch();
    extend(_fwchain, plm->fwchain());
}

int ParaTransform::simplify()
{
    size_t i;
    for (i = 0; i < std::min(_fwchain.size(), _revchain.size()); ++i) {
        if (! _fwchain[i].is(_revchain[i]))
            break;
    }
    if (i == 0)
        return 0;
    throwIfLocked(); touch();
    _fwchain.erase(_fwchain.begin(), _fwchain.begin()+i);
    _revchain.erase(_revchain.begin(), _revchain.begin()+i);
    return i;
}

HParaTransform ParaTransform::transformFromInto(HParaTransform from, HParaTransform into)
{
    from->throwIfHasInverse();
    into->throwIfHasInverse();

    HParaTransform ret = new ParaTransform;
    ret->set(from->fwchain(), into->fwchain());
    ret->simplify();
    return ret;
}

int ParaTransform::simplifyTransforms(std::vector<HParaTransform> transforms, bool compute_not_change)
{
    if (transforms.size() == 0)
        return 0; //nothing to do; zero-length will cause later routines to crash
    int ret = 0;
    //simplify all the individual tranforms. Apart from contributing to simplifying, it makes the next step not hicc up on canceling transforms in the chain.
    for(HParaTransform& it : transforms){
        ret += it->simplify();
    }
    //gather length info
    //int minlenfw  = std::numeric_limits<int>::max();
    //int minlenrev = std::numeric_limits<int>::max();
    //int maxlenfw  = 0;
    //int maxlenrev = 0;
    int minlen = std::numeric_limits<int>::max();
    for(HParaTransform& it : transforms){
        //minlenfw  = std::min(minlenfw,int( ppl->fwchain().size() ));
        //minlenrev = std::min(minlenfw,int( ppl->fwchain().size() ));
        //maxlenfw  = std::max(minlenfw,int( ppl->fwchain().size() ));
        //maxlenrev = std::max(minlenfw,int( ppl->fwchain().size() ));
        minlen = std::min(minlen, it->size());
    }
    //find common start in full transform chain, and cut it off
    //the full chain begins from inverse placements, so we start with them.
    //FIXME: using a negative signed int causes unexpected behaviour when comparing with an unsigned int!!!
    int idif = -1; //first position where transforms are unequal
    for (int i = 0; i < minlen; ++i) {
        HParaPlacement plm = (*transforms[0])[idif];
        bool inverse = (*transforms[0]).isInverse(idif);
        for(HParaTransform& it : transforms){
            if ((*it)[idif].is(plm) && inverse == it->isInverse(idif))
                continue;
            idif = i;
            break;
        }
        if(idif != -1)
            break;
    }
    if (idif == 0)
        return idif; //can't be simplified
    if (! compute_not_change){
        for(HParaTransform& it : transforms){
            if (idif > it->revchain().size()){
                it->_fwchain.erase(it->_revchain.begin(), it->fwchain().begin() + (idif - int(it->revchain().size())));
                it->_revchain.clear(); //after fwchain, coz we are using the size of revchain to compute the slice
            } else {
                it->_revchain.erase(it->_revchain.end() - idif, it->_revchain.end());
            }
        }
    }
    return idif;
}

int ParaTransform::simplifyTransformsOfConstraint(ParaObject& constraint, bool compute_not_change)
{
    std::vector<HParaTransform> trs;
    constraint.throwIfIncomplete_Shapes();
    constraint.forEachShape([&](const ParaObject::ShapeRef& it){
        HParaObject paraobj = *it.value;
        if (!paraobj->isDerivedFrom(ParaShapeBase::getClassTypeId()))
            throw Py::RuntimeError("Not a 2D shape");
        ParaShapeBase& sh = static_cast<ParaShapeBase&>(*paraobj);
        trs.push_back(sh.placement->copy().downcast<ParaTransform>());
    });
    int ret = simplifyTransforms(trs, compute_not_change);
    if (ret > 0 && compute_not_change){
        int i = 0;
        constraint.forEachShape([&](const ParaObject::ShapeRef& it){
            ParaShapeBase& sh = static_cast<ParaShapeBase &>(*(*it.value));
            HParaShapeBase shcpy = sh.copy().downcast<ParaShapeBase>();
            shcpy->placement = trs[i];
            ++i;
        });
    }
    return ret;
}

void ParaTransform::throwIfHasInverse() const
{
    if (_revchain.size() > 0)
        throw Py::ValueError("Transform has inverse placements");
}

HParaPlacement ParaTransform::operator[](int index)
{
    if (index >= int(_revchain.size()))
        return _fwchain[index - int(_revchain.size())];
    else
        return _revchain[int(_revchain.size()) - 1 - index];

}
