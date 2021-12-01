/***************************************************************************
 *   Copyright (c) 2012 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef ItemAssembly_ItemAssembly_H
#define ItemAssembly_ItemAssembly_H

#include <App/PropertyStandard.h>

#include "Item.h"
#include "Solver/Solver.h"

namespace Assembly
{


class AssemblyExport Product : public Assembly::Item
{
    PROPERTY_HEADER(Assembly::Product); 

public:
    Product();

	/// Items of the Product
    App::PropertyLinkList   Items;

   /** @name base properties of all Assembly Items 
     * This properties correspond mostly to the meta information
     * in the App::Document class
     */
    //@{
    /// Id e.g. Part number
    App::PropertyString  Id;
    /// unique identifier of the Item 
    App::PropertyUUID    Uid;
    /// material descriptions
    App::PropertyMap     Material;

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
        the color or the next hierarchy is used
        */
    App::PropertyColor Color;
    /// Visibility
    App::PropertyBool  Visibility;
    //@}
    
 
    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "AssemblyGui::ViewProviderProduct";
    }
    //PyObject *getPyObject(void);
    //@}

};

} //namespace Assembly


#endif // Assembly_ItemAssembly_H
