// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/DocumentObject.h>
#include <App/DocumentObjectExtension.h>
#include <App/ExtensionPython.h>
#include <vector>


namespace App
{
class DocumentObjectGroup;
class GroupExtensionPy;

class AppExport GroupExtension: public DocumentObjectExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(App::GroupExtension);
    using inherited = DocumentObjectExtension;

public:
    /// Constructor
    GroupExtension();
    ~GroupExtension() override;

    /** @name Object handling  */
    //@{
    /** Adds an object of \a sType with \a pObjectName to the document this group belongs to and
     * append it to this group as well.
     */
    virtual DocumentObject* addObject(const char* sType, const char* pObjectName);
    /** Adds an object of \a T with \a pObjectName to the document this group belongs to and
     * append it to this group as well.
     */
    template<typename T>
    T* addObject(const char* pObjectName);
    /* Adds the object \a obj to this group. Returns all objects that have been added.
     */
    virtual std::vector<DocumentObject*> addObject(DocumentObject* obj);
    /* Adds the objects \a objs to this group. Returns all objects that have been added.
     */
    virtual std::vector<DocumentObject*> addObjects(std::vector<DocumentObject*> obj);

    /* Sets the objects in this group. Everything contained already will be removed first
     */
    virtual std::vector<DocumentObject*> setObjects(std::vector<DocumentObject*> obj);

    /*override this function if you want only special objects
     */
    virtual bool allowObject(DocumentObject*)
    {
        return true;
    }

    /** Removes an object from this group. Returns all objects that have been removed.
     */
    virtual std::vector<DocumentObject*> removeObject(DocumentObject* obj);
    /** Removes objects from this group. Returns all objects that have been removed.
     */
    virtual std::vector<DocumentObject*> removeObjects(std::vector<DocumentObject*> obj);
    /** Removes all children objects from this group and the document.
     */
    virtual void removeObjectsFromDocument();
    /** Returns the object of this group with \a Name. If the group doesn't have such an object 0 is
     * returned.
     * @note This method might return 0 even if the document this group belongs to contains an
     * object with this name.
     */
    DocumentObject* getObject(const char* Name) const;
    /**
     * Checks whether the object \a obj is part of this group.
     * @param obj        the object to check for.
     * @param recursive  if true check also if the obj is child of some sub group (default is
     * false).
     */
    virtual bool hasObject(const DocumentObject* obj, bool recursive = false) const;
    /**
     * Checks whether this group object is a child (or sub-child if enabled)
     * of the given group object.
     */
    bool isChildOf(const GroupExtension* group, bool recursive = true) const;
    /** Returns a list of all objects this group does have.
     */
    const std::vector<DocumentObject*>& getObjects() const;
    /** Returns a list of all objects of \a typeId this group does have.
     */
    std::vector<DocumentObject*> getObjectsOfType(const Base::Type& typeId) const;
    /** Returns the number of objects of \a typeId this group does have.
     */
    int countObjectsOfType(const Base::Type& typeId) const;
    /** Returns the object group of the document which the given object \a obj is part of.
     * In case this object is not part of a group 0 is returned.
     * @note This only returns objects that are normal groups, not any special derived type
     * like GeoFeatureGroups or OriginGroups. To retrieve those please use their appropriate
     * functions
     */
    static DocumentObject* getGroupOfObject(const DocumentObject* obj);
    //@}

    PyObject* getExtensionPyObject() override;

    void extensionOnChanged(const Property* p) override;

    bool extensionGetSubObject(DocumentObject*& ret,
                               const char* subname,
                               PyObject** pyObj,
                               Base::Matrix4D* mat,
                               bool transform,
                               int depth) const override;

    bool extensionGetSubObjects(std::vector<std::string>& ret, int reason) const override;

    App::DocumentObjectExecReturn* extensionExecute() override;

    std::vector<DocumentObject*> getAllChildren() const;
    void getAllChildren(std::vector<DocumentObject*>&, std::set<DocumentObject*>&) const;

    /// Properties
    PropertyLinkList Group;
    PropertyBool _GroupTouched;

private:
    void removeObjectFromDocument(DocumentObject*);
    // This function stores the already searched objects to prevent infinite recursion in case of a
    // cyclic group graph It throws an exception of type Base::RuntimeError if a cyclic dependency
    // is detected.
    bool recursiveHasObject(const DocumentObject* obj,
                            const GroupExtension* group,
                            std::vector<const GroupExtension*> history) const;

    // for tracking children visibility
    void slotChildChanged(const App::DocumentObject&, const App::Property&);
    std::unordered_map<const App::DocumentObject*, fastsignals::scoped_connection> _Conns;
};


template<typename T>
T* GroupExtension::addObject(const char* pObjectName)
{
    static_assert(std::is_base_of<DocumentObject, T>::value, "T must be derived from DocumentObject");
    return static_cast<T*>(addObject(T::getClassName(), pObjectName));
}

template<typename ExtensionT>
class GroupExtensionPythonT: public ExtensionT
{

public:
    GroupExtensionPythonT() = default;
    ~GroupExtensionPythonT() override = default;

    // override the documentobjectextension functions to make them available in python
    bool allowObject(DocumentObject* obj) override
    {
        Base::PyGILStateLocker locker;
        Py::Object pyobj = Py::asObject(obj->getPyObject());
        EXTENSION_PROXY_ONEARG(allowObject, pyobj);

        if (result.isNone()) {
            return ExtensionT::allowObject(obj);
        }

        if (result.isBoolean()) {
            return result.isTrue();
        }

        return false;
    };
};

using GroupExtensionPython = ExtensionPythonT<GroupExtensionPythonT<GroupExtension>>;

}  // namespace App
