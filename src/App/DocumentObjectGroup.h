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


#ifndef APP_DOCUMENTOBJECTGROUP_H
#define APP_DOCUMENTOBJECTGROUP_H

#include "FeaturePython.h"
#include "DocumentObject.h"
#include "PropertyLinks.h"
#include <vector>


namespace App
{
class DocumentObjectGroupPy;

class AppExport DocumentObjectGroup : public DocumentObject
{
    PROPERTY_HEADER(App::DocumentObjectGroup);

public:
    /// Constructor
    DocumentObjectGroup(void);
    virtual ~DocumentObjectGroup();

    /** @name Object handling  */
    //@{
    /** Adds an object of \a sType with \a pObjectName to the document this group belongs to and 
     * append it to this group as well.
     */
    DocumentObject *addObject(const char* sType, const char* pObjectName);
    /* Adds the object \a obj to this group. 
     */
    void addObject(DocumentObject* obj);
    /** Removes an object from this group.
     */
    void removeObject(DocumentObject* obj);
    /** Removes all children objects from this group and the document.
     */
    void removeObjectsFromDocument();
    /** Returns the object of this group with \a Name. If the group doesn't have such an object 0 is returned.
     * @note This method might return 0 even if the document this group belongs to contains an object with this name.
     */
    DocumentObject *getObject(const char* Name) const;
    /**
     * Checks whether the object \a obj is part of this group.
     */
    bool hasObject(const DocumentObject* obj) const;
    /**
     * Checks whether this group object is a child (or sub-child)
     * of the given group object.
     */
    bool isChildOf(const DocumentObjectGroup*) const;
    /** Returns a list of all objects this group does have.
     */
    std::vector<DocumentObject*> getObjects() const;
    /** Returns a list of all objects of \a typeId this group does have.
     */
    std::vector<DocumentObject*> getObjectsOfType(const Base::Type& typeId) const;
    /** Returns the number of objects of \a typeId this group does have.
     */
    int countObjectsOfType(const Base::Type& typeId) const;
    /** Returns the object group of the document which the given object \a obj is part of.
     * In case this object is not part of a group 0 is returned.
     */
    static DocumentObjectGroup* getGroupOfObject(const DocumentObject* obj);
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "Gui::ViewProviderDocumentObjectGroup";
    }
    virtual PyObject *getPyObject(void);

    /// Properties
    PropertyLinkList Group;

private:
    void removeObjectFromDocument(DocumentObject*);
};

typedef App::FeaturePythonT<DocumentObjectGroup> DocumentObjectGroupPython;


} //namespace App


#endif // APP_DOCUMENTOBJECTGROUP_H
