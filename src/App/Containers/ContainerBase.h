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

//
//Containers are objects that contain other objects. An important
//characteristics of a container are:
// *hiding/showing it affects if child objects are shown
// *they can be activated to receive new objects
//
//Movable containers form Workspaces. Objects cannot freely use geometry across
//workspaces, because when doing so, one has to deal with transformations
//somehow.
//
//Containers should form a tree. A graph structure is possible, but highly
//discouraged. Document is the root of container tree, and is treated as being
//a container itself. Document is the only "true" container however; for all
//the rest, containership is emulated. Technically, a Document actually
//contains every object and every container in a plain structure. However, a
//Container interface around Document will emulate as if document only contains
//objects not "contained" by other containers.
//
//These modules provide interface classes, whose purpose is to provide a
//unified interface for all different container objects in FreeCAD, and reduce
//the amount of typecasting. They also provide container path management and
//coordinate system transforms.
//

#ifndef APP_CONTAINERBASE_H
#define APP_CONTAINERBASE_H

#include <Base/BaseClass.h>
#include <FCConfig.h>

#include <unordered_set> //for private function
#include <CXX/Objects.hxx>

namespace App {

class Document;
class DocumentObject;
class PropertyContainer;
class PropertyContainerPy;
class GroupExtension;
class GeoFeatureGroupExtension;
class Origin;


/**
 * @brief ContainerBase: abstract class to be a base for Container and
 * Workspace. The derived classes are supposed to be handled in a by-value
 * manner, similarly to TopoDS_Shape. It is recommended to contsruct instances
 * around actual container objects on-the-fly, as needed.
 *
 * Container remembers pointer to the object as a python object, so if the
 * object gets deleted while the Container is alive, the Container will not
 * crash.
 */
class AppExport ContainerBase: public Base::BaseClass
{
    TYPESYSTEM_HEADER();
public:
    ContainerBase() : pcObject(nullptr) {}
    ContainerBase(PropertyContainer* pcObject);
    virtual ~ContainerBase();

    PropertyContainer* object() const;

    std::string getName() const;

    bool isNull() const {return object() == nullptr;}

    /**
     * @brief allChildren: all cobjects directly belonging to the container
     * @return
     */
    virtual std::vector<DocumentObject*> allChildren() const;
    /**
     * @brief dynamicChildren: children that can be added and removed, such as objects in a group.
     */
    virtual std::vector<DocumentObject*> dynamicChildren() const = 0;
    /**
     * @brief staticChildren: children that are bound to this container. For example, Origin.
     * @return
     */
    virtual std::vector<DocumentObject*> staticChildren() const = 0;

    /**
     * @brief dynamicChildrenRecursive: returns children contained by this
     * container directly and indirectly, that can be removed. This will
     * include static children from dynamic-child subcontainers, since they
     * can be removed from this container by removing the child container.
     */
    virtual std::vector<DocumentObject*> dynamicChildrenRecursive() const;
    /**
     * @brief staticChildrenRecursive: returns children contained by this
     * container directly and indirectly, that canot be removed. E.g. for
     * OriginGroups, Origin and its contents are returned, but not
     * origin/content of subcontainers.
     */
    virtual std::vector<DocumentObject*> staticChildrenRecursive() const;
    virtual std::vector<DocumentObject*> allChildrenRecursive() const;

    virtual bool hasObject(const DocumentObject* obj) const;
    virtual bool hasDynamicObject(const DocumentObject* obj) const;
    virtual bool hasStaticObject(const DocumentObject* obj) const;

    virtual PropertyContainer* parent() const;
    virtual std::vector<PropertyContainer*> parents() const = 0;

    virtual DocumentObject* getObject(const char* objectName) const;

    virtual Document* getDocument() const;

    /**
     * @brief isA<Something>: tests if the container is represented by an object of this type. These are typechecking shortcuts.
     */
    //@{
    bool isADocument() const;
    bool isAGroup() const;
    bool isAGeoGroup() const;
    bool isAnOrigin() const;
    bool isADocumentObject() const {return isAGroup() || isAnOrigin();}
    //@}

    /**
     * @brief isAWorkspace: returns true if this container forms a workspace (workspace is a set of objects sharing a coordinate system)
     */
    virtual bool isAWorkspace() const;

    /**
     * @brief isRoot: true if the container is the root of container tree (now, same as isADocument()).
     * @return
     */
    virtual bool isRoot() const {check(); return isADocument();}

    /**
     * @brief as<Something>: same as object(), but with specific type, and insurance of non-null pointer.
     */
    //@{
    Document& asDocument() const;
    GroupExtension& asGroup() const;
    GeoFeatureGroupExtension& asGeoGroup() const;
    Origin& asOrigin() const;
    DocumentObject& asDocumentObject() const;
    //@}

protected:
    //we are remembering the object as a python object. The beauty about doing so is that if the object is deleted, we don't end up with an invalid pointer.
    Py::Object ref; //keeps reference, so that the object isn't deleted while this Container is alive
    PropertyContainerPy* pcObject; //usually, same as ref.ptr(), cast to the right type

    void check() const;

    /**
     * @brief recursiveChildren: unified algorithm for allChildrenRecursive(), dynamicChildrenRecursive(), staticChildrenRecursive()
     * @param b_dynamic: if true, include dynamic children to output
     * @param b_static: if true, include static children to output
     * @return
     */
    std::vector<DocumentObject*> recursiveChildren(bool b_dynamic, bool b_static) const;

private:
    /**
     * @brief recursiveChildrenRec: the actual traversal of container tree, with infinite recursion protection
     */
    static void recursiveChildrenRec(PropertyContainer* cnt,
                                   bool b_dynamic, bool b_static,
                                   std::unordered_set<DocumentObject*>& visited,
                                   std::vector<DocumentObject*> &result);
};


}//namespace App
#endif // APP_CONTAINERBASE_H
