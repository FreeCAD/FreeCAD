/***************************************************************************
 *   Copyright (c) Victor Titov (DeepSOIC)   <vv.titov@gmail.com> 2017     *
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

#ifndef APP_CONTAINER_H
#define APP_CONTAINER_H

#include "ContainerBase.h"

namespace App {

/**
 * @brief Container: unified interface for container objects.
 * Container objects are:
 * * Document
 * * anything with GroupExtension (and derived, incl. OriginGroupExtension)
 * * Origin
 */
class AppExport Container: public ContainerBase
{
    TYPESYSTEM_HEADER();
public:
    Container() {}
    Container(PropertyContainer* pcObject);
    virtual ~Container();

    virtual std::vector<DocumentObject*> dynamicChildren() const override;
    virtual std::vector<DocumentObject*> staticChildren() const override;

    virtual std::vector<PropertyContainer*> parents() const override;

    /**
     * @brief canAccept(object): returns if a given object can be added. Usually, type of the object is tested, and if no container dependency loops will result.
     * @param obj:
     * @param b_throw: if true, an exception is thrown instead of returning false. This can be useful to obtain information about reasons of rejection.
     * @return
     */
    virtual bool canAccept(DocumentObject* obj, bool b_throw = false) const;
    /**
     * @brief canAccept(type, pytype): tests if a new object of given type can be created in this container.
     * @param type: name of type of object, like "Part::Feature"
     * @param pytype: name of python subtype (optional, free-form). It is fed
     * to GroupExtension::allowObject, which can be overridden to provide
     * allow/disallow rules for python-powered objects.
     */
    virtual bool canAccept(const char* type, const char* pytype = "") const;
    /**
     * @brief newObject: creates a new object in this container.
     * @param sType: class name of the object to create, e.g. "Part::Box"
     * @param pObjectName: preferred name for the object. If not unique, it is made unique by appending numbers.
     * @param pytype: name of python subtype (optional, free-form). The only purpose is making canAccept happy.
     * @param isNew: (not supported yet) if false, object's setupObject method is not called (e.g. this will prevent App::Part from creating its Origin.
     * @return the created object. If object is not created, exception is thrown.
     */
    virtual DocumentObject* newObject(const char* sType, const char* pObjectName=0, const char* pytype = "", bool isNew=true);
    /**
     * @brief adoptObject: adds object to this container if it isn't in any
     * other container (i.e. is directly in Document). Otherwise does nothing.
     * This is useful for adding lists of objects with internal container
     * relationships.
     * @return returns true if object was adopted, false otherwise. (if re-adopting an own child, returns true too).
     */
    virtual bool adoptObject(DocumentObject* obj);
    /**
     * @brief takeObject: adds an object to this container, withdrawing it from its current container if necessary.
     */
    virtual void addObject(DocumentObject* obj);
    /**
     * @brief withdrawObject: removes object from this container (and adds it to Document, implicitly).
     * @param obj
     */
    virtual void withdrawObject(DocumentObject* obj);
    /**
     * @brief deleteObject: deletes an object.
     */
    virtual void deleteObject(DocumentObject* obj);

    PyObject* getPyObject() override;

public:
    static bool isAContainer(PropertyContainer* object);
    static std::vector<DocumentObject*> findAllContainers(App::Document& doc);
    static std::vector<PropertyContainer*> getContainersOf(App::DocumentObject* obj);
    static PropertyContainer* getContainerOf(App::DocumentObject* obj);

public:
    static PyMethodDef    PyMethods[]; //static methods
private:
    static PyObject* sIsAConainer(PyObject *self,PyObject *args,PyObject *kwd);
    static PyObject* sFindAllContainers(PyObject *self,PyObject *args,PyObject *kwd);
    static PyObject* sGetContainersOf(PyObject *self,PyObject *args,PyObject *kwd);
    static PyObject* sGetContainerOf(PyObject *self,PyObject *args,PyObject *kwd);

};

}//namespace App
#endif // APP_CONTAINER_H
