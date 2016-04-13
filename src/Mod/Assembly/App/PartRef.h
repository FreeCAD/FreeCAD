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


#ifndef Assembly_ItemPart_H
#define Assembly_ItemPart_H

#include <App/PropertyLinks.h>
#include <App/GeoFeature.h>

#include "Solver/Solver.h"


namespace Assembly
{

class Product;

class AssemblyExport PartRef : public  App::GeoFeature
{
    PROPERTY_HEADER(Assembly::PartRef);
    
public:
    PartRef();

    App::PropertyLink       Item;
    //App::PropertyLinkList   Annotation;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    // returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "AssemblyGui::ViewProviderItemPart";
    }
    PyObject *getPyObject(void);
    //@}

    //virtual TopoDS_Shape getShape(void) const;
    
    bool holdsObject(App::DocumentObject* obj) const;
    Product* getParentAssembly();
    void ensureInitialisation();
    
    boost::shared_ptr<Part3D> m_part;
    virtual boost::shared_ptr<Geometry3D> getGeometry3D(const char* Type );
    void setCalculatedPlacement( boost::shared_ptr<Part3D> part );
};

} //namespace Assembly


#endif // Assembly_ItemPart_H
