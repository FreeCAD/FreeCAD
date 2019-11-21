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
        double defvalue = 0.0;
    };

    struct ChildAttribute
    {
        Base::PyHandleBase* value = nullptr;
        std::string name;
        PyTypeObject *type;
        bool make = false; //true if it's an actual child, like an endpoint of an arc. False if it is a reference, like a constraint referring a point. If make, the object is auto-constructed upon call to makeParameters
    };

protected://data members
    std::vector<ParameterRef> _parameters;
    PyObject* _twin = nullptr;
    std::vector<ParameterAttribute> _attrs; //vectors are likely to be faster than maps on small lists
    std::vector<ChildAttribute> _children;
    bool _touched = true;
public://data members
    int tag = 0;
    Py::Object userData;
    std::string label;

public: //methods
    virtual PyObject* getPyObject() override;
    HParaObject self();

    /**
     * @brief update: updates list of parameters. Must call after redirecting prarmeters.
     * It ignores touched state of this object, but calls update on child objects only if the child is touched.
     * Touched flag is cleared after a successful update.
     */
    virtual void update();
    void touch() {_touched = true;}
    bool isTouched() const {return _touched;}

    virtual HParaObject copy() const;

    const std::vector<ParameterRef>& parameters() const {return _parameters;}
    virtual std::vector<ParameterRef> makeParameters(HParameterStore into);
    ///returns true if all vital parameter and object refs are set up
    bool isComplete() const;
    virtual void throwIfIncomplete() const;


    virtual Py::Object getAttr(const char* attrname);
    Py::Object getAttr(std::string attrname);
    virtual void setAttr(std::string attrname, Py::Object val);
    virtual std::vector<std::string> listAttrs() const;

protected: //methods
    virtual void initAttrs() = 0;
    virtual ~ParaObject() = default; //protect destructor to enforce handle-only
public: //friends
    friend class ParaObjectPy;
};

} //namespace

#endif
