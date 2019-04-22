/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2010     *
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


#ifndef APP_PropertyConstraintList_H
#define APP_PropertyConstraintList_H

// Std. configurations


#include <vector>
#include <string>
#include <App/Property.h>
#include <Mod/Part/App/Geometry.h>
#include "Constraint.h"
#include <boost/signals2.hpp>
#include <boost/unordered/unordered_map.hpp>

namespace Base {
class Writer;
}

namespace Sketcher
{
class Constraint;

class SketcherExport PropertyConstraintList : public App::PropertyLists
{
    TYPESYSTEM_HEADER();

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
    virtual ~PropertyConstraintList();

    virtual void setSize(int newSize);
    virtual int getSize(void) const;
    
    const char* getEditorName(void) const {
        return "SketcherGui::PropertyConstraintListItem";
    }

    /*!
      Sets a single constraint to the property at a certain
      position. The value is cloned inernally so it's in the
      responsibility of the caller to free the memory.
    */
    void set1Value(const int idx, const Constraint*);
    /*!
      Sets a single constraint to the property.
      The value is cloned inernally so it's in the
      responsibility of the caller to free the memory.
    */
    void setValue(const Constraint*);
    /*!
      Sets a vector of constraint to the property.
      The values of the array are cloned inernally so it's
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
    const Constraint *operator[] (const int idx) const {
        return invalidGeometry ? 0 : _lValueList[idx];
    }

    const std::vector<Constraint*> &getValues(void) const {
        return invalidGeometry ? _emptyValueList : _lValueList;
    }
    const std::vector<Constraint*> &getValuesForce(void) const {//to suppress check for invalid geometry, to be used for sketch repairing.
        return  _lValueList;
    }

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save(Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const App::Property &from);

    virtual unsigned int getMemSize(void) const;

    void acceptGeometry(const std::vector<Part::Geometry *> &GeoList);
    void checkGeometry(const std::vector<Part::Geometry *> &GeoList);
    bool scanGeometry(const std::vector<Part::Geometry *> &GeoList) const;

    /// Return status of geometry for better error reporting
    bool hasInvalidGeometry() const { return invalidGeometry; }


    const Constraint *getConstraint(const App::ObjectIdentifier &path) const;
    virtual void setPathValue(const App::ObjectIdentifier & path, const App::any & value);
    virtual App::any getPathValue(const App::ObjectIdentifier & path) const;
    virtual App::ObjectIdentifier canonicalPath(const App::ObjectIdentifier & p) const;
    virtual void getPaths(std::vector<App::ObjectIdentifier> & paths) const;

    virtual bool getPyPathValue(const App::ObjectIdentifier &path, Py::Object &res) const override;

    typedef std::pair<int, const Constraint*> ConstraintInfo ;

    boost::signals2::signal<void (const std::map<App::ObjectIdentifier, App::ObjectIdentifier> &)> signalConstraintsRenamed;
    boost::signals2::signal<void (const std::set<App::ObjectIdentifier> &)> signalConstraintsRemoved;

    static std::string getConstraintName(const std::string &name, int i);

    static std::string getConstraintName(int i);

    static int getIndexFromConstraintName(const std::string & name);

    static bool validConstraintName(const std::string &name);

    App::ObjectIdentifier createPath(int ConstrNbr) const;

private:
    App::ObjectIdentifier makeArrayPath(int idx);
    App::ObjectIdentifier makeSimplePath(const Constraint *c);
    App::ObjectIdentifier makePath(int idx, const Constraint *c);

    std::vector<Constraint *> _lValueList;
    boost::unordered_map<boost::uuids::uuid, std::size_t> valueMap;

    std::vector<unsigned int> validGeometryKeys;
    bool invalidGeometry;

    void applyValues(std::vector<Constraint*>&&);
    void applyValidGeometryKeys(const std::vector<unsigned int> &keys);

    static std::vector<Constraint *> _emptyValueList;
};

} // namespace Sketcher


#endif // APP_PropertyConstraintList_H
