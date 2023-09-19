/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <cassert>
#endif

#include <App/ExpressionParser.h>
#include <App/ObjectIdentifier.h>
#include <Base/QuantityPy.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Base/Writer.h>

#include "ConstraintPy.h"
#include "PropertyConstraintList.h"


using namespace App;
using namespace Base;
using namespace std;
using namespace Sketcher;

//**************************************************************************
// PropertyConstraintList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(Sketcher::PropertyConstraintList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction


PropertyConstraintList::PropertyConstraintList()
    : validGeometryKeys(0)
    , invalidGeometry(true)
    , restoreFromTransaction(false)
    , invalidIndices(false)
{}

PropertyConstraintList::~PropertyConstraintList()
{
    for (std::vector<Constraint*>::iterator it = _lValueList.begin(); it != _lValueList.end();
         ++it) {
        if (*it) {
            delete *it;
        }
    }
}

App::ObjectIdentifier PropertyConstraintList::makeArrayPath(int idx)
{
    return App::ObjectIdentifier(*this, idx);
}

App::ObjectIdentifier PropertyConstraintList::makeSimplePath(const Constraint* c)
{
    return App::ObjectIdentifier(*this) << App::ObjectIdentifier::SimpleComponent(
               App::ObjectIdentifier::String(c->Name,
                                             !ExpressionParser::isTokenAnIndentifier(c->Name)));
}

App::ObjectIdentifier PropertyConstraintList::makePath(int idx, const Constraint* c)
{
    return c->Name.empty() ? makeArrayPath(idx) : makeSimplePath(c);
}

void PropertyConstraintList::setSize(int newSize)
{
    std::set<App::ObjectIdentifier> removed;

    /* Collect information about erased elements */
    for (unsigned int i = newSize; i < _lValueList.size(); i++) {
        valueMap.erase(_lValueList[i]->tag);
        removed.insert(makePath(i, _lValueList[i]));
    }

    /* Signal removed elements */
    if (!removed.empty()) {
        signalConstraintsRemoved(removed);
    }

    /* Actually delete them */
    for (unsigned int i = newSize; i < _lValueList.size(); i++) {
        delete _lValueList[i];
    }

    /* Resize array to new size */
    _lValueList.resize(newSize);
}

int PropertyConstraintList::getSize() const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyConstraintList::set1Value(const int idx, const Constraint* lValue)
{
    if (lValue) {
        aboutToSetValue();
        Constraint* oldVal = _lValueList[idx];
        Constraint* newVal = lValue->clone();

        if (oldVal->Name != newVal->Name) {
            std::map<App::ObjectIdentifier, App::ObjectIdentifier> renamed;

            renamed[makePath(idx, _lValueList[idx])] = makePath(idx, lValue);
            if (!renamed.empty()) {
                signalConstraintsRenamed(renamed);
            }
        }

        _lValueList[idx] = newVal;
        valueMap.erase(oldVal->tag);
        valueMap[newVal->tag] = idx;
        delete oldVal;
        hasSetValue();
    }
}

void PropertyConstraintList::setValue(const Constraint* lValue)
{
    if (lValue) {
        aboutToSetValue();
        Constraint* newVal = lValue->clone();
        std::set<App::ObjectIdentifier> removed;
        std::map<App::ObjectIdentifier, App::ObjectIdentifier> renamed;
        int start = 0;

        /* Determine if it is a rename or not * */
        if (!_lValueList.empty() && lValue->tag == _lValueList[0]->tag) {
            renamed[makePath(0, _lValueList[0])] = makePath(0, lValue);
            start = 1;
        }

        /* Signal rename changes */
        if (!renamed.empty()) {
            signalConstraintsRenamed(renamed);
        }

        /* Collect info about removals */
        for (unsigned int i = start; i < _lValueList.size(); i++) {
            valueMap.erase(_lValueList[i]->tag);
            removed.insert(makePath(i, _lValueList[i]));
        }

        /* Signal removes */
        if (!removed.empty()) {
            signalConstraintsRemoved(removed);
        }

        // Cleanup
        for (unsigned int i = 0; i < _lValueList.size(); i++) {
            delete _lValueList[i];
        }

        /* Set new data */
        _lValueList.resize(1);
        _lValueList[0] = newVal;
        valueMap[_lValueList[0]->tag] = 0;
        hasSetValue();
    }
}

void PropertyConstraintList::setValues(const std::vector<Constraint*>& lValue)
{
    auto copy = lValue;
    for (auto& cstr : copy) {
        cstr = cstr->clone();
    }

    setValues(std::move(copy));
}

void PropertyConstraintList::setValues(std::vector<Constraint*>&& lValue)
{
    aboutToSetValue();
    applyValues(std::move(lValue));
    hasSetValue();
}

void PropertyConstraintList::applyValues(std::vector<Constraint*>&& lValue)
{
    std::set<Constraint*> oldVals(_lValueList.begin(), _lValueList.end());
    std::map<App::ObjectIdentifier, App::ObjectIdentifier> renamed;
    std::set<App::ObjectIdentifier> removed;
    boost::unordered_map<boost::uuids::uuid, std::size_t> newValueMap;

    /* Check for renames */
    for (unsigned int i = 0; i < lValue.size(); i++) {
        boost::unordered_map<boost::uuids::uuid, std::size_t>::const_iterator j =
            valueMap.find(lValue[i]->tag);

        if (j != valueMap.end()) {
            if (i != j->second || _lValueList[j->second]->Name != lValue[i]->Name) {
                App::ObjectIdentifier old_oid(makePath(j->second, _lValueList[j->second]));
                App::ObjectIdentifier new_oid(makePath(i, lValue[i]));
                renamed[old_oid] = new_oid;
            }
            valueMap.erase(j);
        }

        newValueMap[lValue[i]->tag] = i;

        // safety insurance in case new new values contain some pointers of the old values
        oldVals.erase(lValue[i]);
    }

    /* Collect info about removed elements */
    for (auto& v : valueMap) {
        removed.insert(makePath(v.second, _lValueList[v.second]));
    }

    /* Update value map with new tags from new array */
    valueMap = std::move(newValueMap);

    /* Signal removes first, in case renamed values below have the same names as some of the removed
     * ones. */
    if (!removed.empty() && !restoreFromTransaction) {
        signalConstraintsRemoved(removed);
    }

    /* Signal renames */
    if (!renamed.empty() && !restoreFromTransaction) {
        signalConstraintsRenamed(renamed);
    }

    _lValueList = std::move(lValue);

    /* Clean-up; remove old values */
    for (auto& v : oldVals) {
        delete v;
    }
}

PyObject* PropertyConstraintList::getPyObject()
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0; i < getSize(); i++) {
        PyList_SetItem(list, i, _lValueList[i]->getPyObject());
    }
    return list;
}

bool PropertyConstraintList::getPyPathValue(const App::ObjectIdentifier& path,
                                            Py::Object& res) const
{
    if (path.numSubComponents() != 2 || path.getPropertyComponent(0).getName() != getName()) {
        return false;
    }

    const ObjectIdentifier::Component& c1 = path.getPropertyComponent(1);

    const Constraint* cstr = nullptr;

    if (c1.isArray()) {
        cstr = _lValueList[c1.getIndex(_lValueList.size())];
    }
    else if (c1.isSimple()) {
        ObjectIdentifier::Component c1 = path.getPropertyComponent(1);
        for (auto c : _lValueList) {
            if (c->Name == c1.getName()) {
                cstr = c;
                break;
            }
        }
    }
    if (!cstr) {
        return false;
    }
    Quantity q = cstr->getPresentationValue();
    res = new Base::QuantityPy(new Base::Quantity(q));
    return true;
}

void PropertyConstraintList::setPyObject(PyObject* value)
{
    if (PyList_Check(value)) {
        Py_ssize_t nSize = PyList_Size(value);
        std::vector<Constraint*> values;
        values.resize(nSize);

        for (Py_ssize_t i = 0; i < nSize; ++i) {
            PyObject* item = PyList_GetItem(value, i);
            if (!PyObject_TypeCheck(item, &(ConstraintPy::Type))) {
                std::string error = std::string("types in list must be 'Constraint', not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            values[i] = static_cast<ConstraintPy*>(item)->getConstraintPtr();
        }

        setValues(values);
    }
    else if (PyObject_TypeCheck(value, &(ConstraintPy::Type))) {
        ConstraintPy* pcObject = static_cast<ConstraintPy*>(value);
        setValue(pcObject->getConstraintPtr());
    }
    else {
        std::string error = std::string("type must be 'Constraint' or list of 'Constraint', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyConstraintList::Save(Writer& writer) const
{
    writer.Stream() << writer.ind() << "<ConstraintList count=\"" << getSize() << "\">" << endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++) {
        _lValueList[i]->Save(writer);
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</ConstraintList>" << endl;
}

void PropertyConstraintList::Restore(Base::XMLReader& reader)
{
    // read my element
    reader.readElement("ConstraintList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");

    std::vector<Constraint*> values;
    values.reserve(count);
    for (int i = 0; i < count; i++) {
        Constraint* newC = new Constraint();
        newC->Restore(reader);
        // To keep upward compatibility ignore unknown constraint types
        if (newC->Type < Sketcher::NumConstraintTypes) {
            values.push_back(newC);
        }
        else {
            // reading a new constraint type which this version cannot handle
            delete newC;
        }
    }

    reader.readEndElement("ConstraintList");

    // assignment
    setValues(std::move(values));
}

Property* PropertyConstraintList::Copy() const
{
    PropertyConstraintList* p = new PropertyConstraintList();
    p->applyValidGeometryKeys(validGeometryKeys);
    p->setValues(_lValueList);
    return p;
}

void PropertyConstraintList::Paste(const Property& from)
{
    Base::StateLocker lock(restoreFromTransaction, true);
    const PropertyConstraintList& FromList = dynamic_cast<const PropertyConstraintList&>(from);
    setValues(FromList._lValueList);
}

unsigned int PropertyConstraintList::getMemSize() const
{
    int size = sizeof(PropertyConstraintList);
    for (int i = 0; i < getSize(); i++) {
        size += _lValueList[i]->getMemSize();
    }
    return size;
}

void PropertyConstraintList::acceptGeometry(const std::vector<Part::Geometry*>& GeoList)
{
    aboutToSetValue();
    validGeometryKeys.clear();
    validGeometryKeys.reserve(GeoList.size());
    for (const auto& it : GeoList) {
        validGeometryKeys.push_back((it)->getTypeId().getKey());
    }
    invalidGeometry = false;
    hasSetValue();
}

void PropertyConstraintList::applyValidGeometryKeys(const std::vector<unsigned int>& keys)
{
    validGeometryKeys = keys;
}

bool PropertyConstraintList::checkGeometry(const std::vector<Part::Geometry*>& GeoList)
{
    if (!scanGeometry(GeoList)) {
        invalidGeometry = true;
        return invalidGeometry;
    }

    // if we made it here, geometry is OK
    if (invalidGeometry) {
        // geometry was bad, but now it became OK.
        invalidGeometry = false;
        touch();
    }

    return invalidGeometry;
}

bool PropertyConstraintList::checkConstraintIndices(int geomax, int geomin)
{
    int mininternalgeoid = std::numeric_limits<int>::max();
    int maxinternalgeoid = GeoEnum::GeoUndef;

    auto cmin = [](int previousmin, int cindex) {
        if (cindex == GeoEnum::GeoUndef) {
            return previousmin;
        }

        return (cindex < previousmin) ? cindex : previousmin;
    };

    auto cmax = [](int previousmax, int cindex) {
        return (cindex > previousmax) ? cindex : previousmax;
    };

    for (const auto& v : _lValueList) {

        mininternalgeoid = cmin(mininternalgeoid, v->First);
        mininternalgeoid = cmin(mininternalgeoid, v->Second);
        mininternalgeoid = cmin(mininternalgeoid, v->Third);

        maxinternalgeoid = cmax(maxinternalgeoid, v->First);
        maxinternalgeoid = cmax(maxinternalgeoid, v->Second);
        maxinternalgeoid = cmax(maxinternalgeoid, v->Third);
    }

    if (maxinternalgeoid > geomax || mininternalgeoid < geomin) {
        invalidIndices = true;
    }
    else {
        invalidIndices = false;
    }

    return invalidIndices;
}

/*!
 * \brief PropertyConstraintList::scanGeometry tests if the supplied geometry
 *  is the same (all elements are of the same type as they used to be).
 * \param GeoList - new geometry list to be checked
 * \return false, if the types have changed.
 */
bool PropertyConstraintList::scanGeometry(const std::vector<Part::Geometry*>& GeoList) const
{
    if (validGeometryKeys.size() != GeoList.size()) {
        return false;
    }

    unsigned int i = 0;
    for (std::vector<Part::Geometry*>::const_iterator it = GeoList.begin(); it != GeoList.end();
         ++it, i++) {
        if (validGeometryKeys[i] != (*it)->getTypeId().getKey()) {
            return false;
        }
    }

    return true;
}

string PropertyConstraintList::getConstraintName(const std::string& name, int i)
{
    if (!name.empty()) {
        return name;
    }
    else {
        return getConstraintName(i);
    }
}

string PropertyConstraintList::getConstraintName(int i)
{
    std::stringstream str;

    str << "Constraint" << (i + 1);
    return str.str();
}

bool PropertyConstraintList::validConstraintName(const std::string& name)
{
    return !name.empty();
}

ObjectIdentifier PropertyConstraintList::createPath(int ConstrNbr) const
{
    return App::ObjectIdentifier(*this, ConstrNbr);
}

int PropertyConstraintList::getIndexFromConstraintName(const string& name)
{
    return std::atoi(name.substr(10, 4000).c_str()) - 1;
}

void PropertyConstraintList::setPathValue(const ObjectIdentifier& path, const boost::any& value)
{
    if (path.numSubComponents() != 2 || path.getPropertyComponent(0).getName() != getName()) {
        FC_THROWM(Base::ValueError, "invalid constraint path " << path.toString());
    }

    const ObjectIdentifier::Component& c1 = path.getPropertyComponent(1);
    double dvalue;

    if (value.type() == typeid(double)) {
        dvalue = boost::any_cast<double>(value);
    }
    else if (value.type() == typeid(float)) {
        dvalue = App::any_cast<float>(value);
    }
    else if (value.type() == typeid(long)) {
        dvalue = App::any_cast<long>(value);
    }
    else if (value.type() == typeid(int)) {
        dvalue = App::any_cast<int>(value);
    }
    else if (value.type() == typeid(Quantity)) {
        dvalue = (App::any_cast<const Quantity&>(value)).getValue();
    }
    else {
        throw std::bad_cast();
    }

    if (c1.isArray()) {
        size_t index = c1.getIndex(_lValueList.size());
        switch (_lValueList[index]->Type) {
            case Angle:
                dvalue = Base::toRadians<double>(dvalue);
                break;
            default:
                break;
        }
        aboutToSetValue();
        _lValueList[index]->setValue(dvalue);
        hasSetValue();
        return;
    }
    else if (c1.isSimple()) {
        for (std::vector<Constraint*>::const_iterator it = _lValueList.begin();
             it != _lValueList.end();
             ++it) {
            int index = it - _lValueList.begin();

            if ((*it)->Name == c1.getName()) {
                switch (_lValueList[index]->Type) {
                    case Angle:
                        dvalue = Base::toRadians<double>(dvalue);
                        break;
                    default:
                        break;
                }
                aboutToSetValue();
                _lValueList[index]->setValue(dvalue);
                hasSetValue();
                return;
            }
        }
    }
    FC_THROWM(Base::ValueError, "invalid constraint path " << path.toString());
}

const Constraint* PropertyConstraintList::getConstraint(const ObjectIdentifier& path) const
{
    if (path.numSubComponents() != 2 || path.getPropertyComponent(0).getName() != getName()) {
        FC_THROWM(Base::ValueError, "Invalid constraint path " << path.toString());
    }

    const ObjectIdentifier::Component& c1 = path.getPropertyComponent(1);

    if (c1.isArray()) {
        return _lValueList[c1.getIndex(_lValueList.size())];
    }
    else if (c1.isSimple()) {
        ObjectIdentifier::Component c1 = path.getPropertyComponent(1);

        for (std::vector<Constraint*>::const_iterator it = _lValueList.begin();
             it != _lValueList.end();
             ++it) {
            if ((*it)->Name == c1.getName()) {
                return *it;
            }
        }
    }
    FC_THROWM(Base::ValueError, "Invalid constraint path " << path.toString());
}

const boost::any PropertyConstraintList::getPathValue(const ObjectIdentifier& path) const
{
    return boost::any(getConstraint(path)->getPresentationValue());
}

ObjectIdentifier PropertyConstraintList::canonicalPath(const ObjectIdentifier& p) const
{
    if (p.numSubComponents() != 2 || p.getPropertyComponent(0).getName() != getName()) {
        FC_THROWM(Base::ValueError, "Invalid constraint path " << p.toString());
    }

    const ObjectIdentifier::Component& c1 = p.getPropertyComponent(1);

    if (c1.isArray()) {
        size_t idx = c1.getIndex();
        if (idx < _lValueList.size() && !_lValueList[idx]->Name.empty()) {
            return ObjectIdentifier(*this)
                << ObjectIdentifier::SimpleComponent(_lValueList[idx]->Name);
        }
        return p;
    }
    else if (c1.isSimple()) {
        return p;
    }
    FC_THROWM(Base::ValueError, "Invalid constraint path " << p.toString());
}

void PropertyConstraintList::getPaths(std::vector<ObjectIdentifier>& paths) const
{
    for (std::vector<Constraint*>::const_iterator it = _lValueList.begin(); it != _lValueList.end();
         ++it) {
        if (!(*it)->Name.empty()) {
            paths.push_back(ObjectIdentifier(*this)
                            << ObjectIdentifier::SimpleComponent((*it)->Name));
        }
    }
}

std::vector<Constraint*> PropertyConstraintList::_emptyValueList(0);
