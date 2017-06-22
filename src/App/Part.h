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


#ifndef APP_Part_H
#define APP_Part_H

#include "OriginGroupExtension.h"
#include "PropertyLinks.h"



namespace App
{


/** Base class of all geometric document objects.
 */
class AppExport Part : public App::GeoFeature, public App::OriginGroupExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(App::Part);

public:
    /// type of the part
    PropertyString Type;

    /** @name base properties of all Assembly Items
    * This properties corospond mostly to the meta information
    * in the App::Document class
    */
    //@{
    /// Id e.g. Part number
    App::PropertyString  Id;
    /// unique identifier of the Item
    App::PropertyUUID    Uid;
    /// material descriptions
    App::PropertyMap     Material;
    /// Meta descriptions
    App::PropertyMap     Meta;

    /** License string
    * Holds the short license string for the Item, e.g. CC-BY
    * for the Creative Commons license suit.
    */
    App::PropertyString  License;
    /// License description/contract URL
    App::PropertyString  LicenseURL;
    //@}

    /** @name Visual properties */
    //@{
    /** Base color of the Item
    If the transparency value is 1.0
    the color or the next hirachy is used
    */
    App::PropertyColor Color;
    //@}

    /// Constructor
    Part(void);
    virtual ~Part();

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "Gui::ViewProviderPart";
    }

    /**
     * Returns the part which contains this object.
     * In case this object is not belongs to any Part 0 is returned.
     * @param obj       the object to search for
     */
    static App::Part* getPartOfObject (const DocumentObject* obj);

    virtual PyObject *getPyObject(void);
};

//typedef App::FeaturePythonT<Part> PartPython;

} //namespace App


#endif // APP_Part_H
