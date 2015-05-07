/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2014     *
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


#ifndef APP_GeoFeatureGroup_H
#define APP_GeoFeatureGroup_H

#include "GeoFeature.h"
#include "PropertyLinks.h"
#include <App/FeaturePython.h>


namespace App
{


/** Base class of all geometric document objects.
 */
class AppExport GeoFeatureGroup : public App::GeoFeature
{
    PROPERTY_HEADER(App::GeoFeatureGroup);

public:
    PropertyLinkList Items;

    /// Constructor
    GeoFeatureGroup(void);
    virtual ~GeoFeatureGroup();

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "Gui::ViewProviderGeoFeatureGroup";
    }
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
     * Checks whether the object \a obj is GeoFeatureGroup of this group.
     */
    bool hasObject(const App::DocumentObject* obj, bool recursive = false) const;
    /**
     * Checks whether this group object is a child (or sub-child)
     * of the given group object.
     */
    bool isChildOf(const GeoFeatureGroup*) const;
    /** Returns a list of all objects this group does have.
     */
    std::vector<DocumentObject*> getObjects() const;
    /** Returns a list of all objects of \a typeId this group does have.
     */
    std::vector<DocumentObject*> getObjectsOfType(const Base::Type& typeId) const;
    /** Returns the number of objects of \a typeId this group does have.
     */
    int countObjectsOfType(const Base::Type& typeId) const;
    //@}

    virtual PyObject *getPyObject(void);

private:
    void removeObjectFromDocument(DocumentObject*);

};

typedef App::FeaturePythonT<GeoFeatureGroup> GeoFeatureGroupPython;

} //namespace App


#endif // APP_GeoFeatureGroup_H
