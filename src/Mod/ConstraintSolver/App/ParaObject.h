/***************************************************************************
 *   Copyright (c) 2019 Viktor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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
#pragma once //to make qt creator happy, see QTCREATORBUG-20883

#ifndef FREECAD_CONSTRAINTSOLVER_PARAOBJECT_H
#define FREECAD_CONSTRAINTSOLVER_PARAOBJECT_H


#include "ParameterRef.h"
#include "ValueSet.h"
#include "Utils.h"

#include <Base/BaseClass.h>

#include <vector>

namespace FCS {

class Constraint;
typedef UnsafePyHandle<Constraint> HConstraint;

class ParaObject;
typedef UnsafePyHandle<ParaObject> HParaObject;

/**
 * @brief Base class for all objects (geometries, constraints) built on solver parameters
 */
class FCSExport ParaObject : public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public://helper structs
    struct ParameterAttribute
    {
        ParameterRef* value = nullptr;
        std::string name;
        bool make = true; //if true, makeParameters makes it. False <-> doesn't make.
        bool required = true;
        double defvalue = 0.0;
    };

    struct ChildAttribute
    {
        HParaObject* value = nullptr;
        std::string name;
        PyTypeObject* type;
        bool make = false; //true if it's an actual child, like an endpoint of an arc. False if it is a reference, like a constraint referring a point. If make, the object is auto-constructed upon call to makeParameters
        bool required = true; //if true, isComplete will check the attribute is filled. Otherwise it won't.
        bool writeOnce = false; //if true, the child can only be assigned once (i.e., overwrite is forbidden).
    };
    struct ShapeRef
    {
        HParaObject* value = nullptr;
        std::string name;
        Base::Type type; //type of tshape
    };

protected://data members
    std::vector<ParameterRef> _parameters;
    PyObject* _twin = nullptr;
    std::vector<ParameterAttribute> _attrs; //vectors are likely to be faster for name lookups than maps on small lists
    std::vector<ChildAttribute> _children;
    std::vector<ShapeRef> _shapes;
    bool _touched = true;
    bool _locked = false;
public://data members
    int tag = 0;
    Py::Object userData;
    std::string label;

public: //methods
    virtual PyObject* getPyObject() override;
    HParaObject self();
    virtual std::string repr() const;

    /**
     * @brief update: updates list of parameters. Must call after redirecting prarmeters.
     * It ignores touched state of this object, but calls update on child objects only if the child is touched.
     * Touched flag is cleared after a successful update.
     */
    virtual void update();
    void touch() {_touched = true;}
    bool isTouched() const {return _touched;}
    
    ///locking means parameter references and references to objects can't be
    ///changed. This is used in particular in ParaTransform objects bound to
    ///shapes.
    void lock() {_locked = true;}
    bool isLocked() const {return _locked;}
    void throwIfLocked() const;

    virtual HParaObject copy() const;

    const std::vector<ParameterRef>& parameters() const {return _parameters;}
    virtual std::vector<ParameterRef> makeParameters(HParameterStore into);
    ///returns true if all vital parameter and object refs are set up
    bool isComplete() const;
    virtual void throwIfIncomplete() const;
    virtual void throwIfIncomplete_Shapes() const;


    virtual Py::Object getAttr(const char* attrname);
    Py::Object getAttr(std::string attrname);
    virtual void setAttr(std::string attrname, Py::Object val);
    virtual std::vector<std::string> listAttrs() const;

    ///iterate over every shape referenced by this ParaObject and call a lambda for every one of them.
    /// Note: ShapeRef may have been generated on the fly, and may not have a name. But it should be writable.
    virtual void forEachShape(std::function<void(const ShapeRef&)> callback) const;

    virtual std::vector<HConstraint> makeRuleConstraints(){return std::vector<HConstraint>();}

    virtual void initFromDict(Py::Dict dict);

protected: //methods
    virtual void initAttrs() = 0;
    void tieAttr_Parameter(ParameterRef& ref, std::string name, bool make = true, bool required = true, double defvalue = 0.0);
    void tieAttr_Child(HParaObject& ref, std::string name, PyTypeObject* type, bool make = false, bool required = true, bool writeOnce = false);
    void tieAttr_Shape(HParaObject& ref, std::string name, Base::Type type);
    ///we need this to support type-checked shape attributes
    virtual Base::Type shapeType() const {return Base::Type::badType();}
    virtual ~ParaObject() = default; //protect destructor to enforce handle-only
public: //friends
    friend class ParaObjectPy;
};

} //namespace

#endif
