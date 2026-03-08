// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <string>
#include <vector>

#include <boost/unordered/unordered_map.hpp>
#include <fastsignals/signal.h>

#include <App/Property.h>
#include <Mod/Part/App/Geometry.h>

#include "Constraint.h"


namespace Base
{
class Writer;
}

namespace Sketcher
{
class Constraint;

class SketcherExport PropertyConstraintList: public App::PropertyLists
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyConstraintList();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyConstraintList() override;

    void setSize(int newSize) override;
    int getSize() const override;

    const char* getEditorName() const override
    {
        return "SketcherGui::PropertyConstraintListItem";
    }

    /*!
      Sets a single constraint to the property at a certain
      position. The value is cloned internally so it's in the
      responsibility of the caller to free the memory.
    */
    void set1Value(const int idx, const Constraint*);
    /*!
      Sets a single constraint to the property.
      The value is cloned internally so it's in the
      responsibility of the caller to free the memory.
    */
    void setValue(const Constraint*);
    /*!
      Sets a vector of constraint to the property.
      The values of the array are cloned internally so it's
      in the responsibility of the caller to free the memory.
    */
    void setValues(const std::vector<Constraint*>&);

    /*!
      Sets a vector of constraint to the property.
      The values of the array are moved, and the ownership of constraints
      inside are taken by this property
    */
    void setValues(std::vector<Constraint*>&&);

    /*!
     Index operator
     \note If the geometry is invalid then the index operator
           returns null. This must be checked by the caller.
    */
    const Constraint* operator[](const int idx) const
    {
        return (invalidGeometry || invalidIndices) ? nullptr : _lValueList[idx];
    }

    const std::vector<Constraint*>& getValues() const
    {
        return (invalidGeometry || invalidIndices) ? _emptyValueList : _lValueList;
    }

    // to suppress check for invalid geometry, to be used for sketch repairing.
    const std::vector<Constraint*>& getValuesForce() const
    {
        return _lValueList;
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const App::Property& from) override;

    unsigned int getMemSize() const override;

    void acceptGeometry(const std::vector<Part::Geometry*>& GeoList);
    bool checkGeometry(const std::vector<Part::Geometry*>& GeoList);
    bool scanGeometry(const std::vector<Part::Geometry*>& GeoList) const;

    bool checkConstraintIndices(int geomax, int geomin);

    /// Return status of geometry for better error reporting
    bool hasInvalidGeometry() const
    {
        return invalidGeometry;
    }


    const Constraint* getConstraint(const App::ObjectIdentifier& path) const;
    void setPathValue(const App::ObjectIdentifier& path, const boost::any& value) override;
    const boost::any getPathValue(const App::ObjectIdentifier& path) const override;
    App::ObjectIdentifier canonicalPath(const App::ObjectIdentifier& p) const override;
    void getPaths(std::vector<App::ObjectIdentifier>& paths) const override;

    bool getPyPathValue(const App::ObjectIdentifier& path, Py::Object& res) const override;

    using ConstraintInfo = std::pair<int, const Constraint*>;

    fastsignals::signal<void(const std::map<App::ObjectIdentifier, App::ObjectIdentifier>&)>
        signalConstraintsRenamed;
    fastsignals::signal<void(const std::set<App::ObjectIdentifier>&)> signalConstraintsRemoved;

    static std::string getConstraintName(const std::string& name, int i);

    static std::string getConstraintName(int i);

    static int getIndexFromConstraintName(const std::string& name);

    static bool validConstraintName(const std::string& name);

    App::ObjectIdentifier createPath(int ConstrNbr) const;

private:
    App::ObjectIdentifier makeArrayPath(int idx);
    App::ObjectIdentifier makeSimplePath(const Constraint* c);
    App::ObjectIdentifier makePath(int idx, const Constraint* c);

    std::vector<Constraint*> _lValueList;
    boost::unordered_map<boost::uuids::uuid, std::size_t> valueMap;

    std::vector<unsigned int> validGeometryKeys;
    bool invalidGeometry;
    bool restoreFromTransaction;
    bool invalidIndices;

    void applyValues(std::vector<Constraint*>&&);
    void applyValidGeometryKeys(const std::vector<unsigned int>& keys);

    static std::vector<Constraint*> _emptyValueList;
};

}  // namespace Sketcher
