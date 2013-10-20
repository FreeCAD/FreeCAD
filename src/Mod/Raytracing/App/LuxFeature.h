/***************************************************************************
 *   Copyright (c) Yorik van Havre          (yorik@uncreated.net) 2013     *
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


#ifndef _LuxFeature_h_
#define _LuxFeature_h_

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/PropertyFile.h>
#include <App/PropertyStandard.h>

#include "RaySegment.h"


namespace Raytracing
{

class Property;

/** Base class of all Feature classes in FreeCAD
 */
//class LuxFeature: public Part::PartFeature
class AppRaytracingExport LuxFeature: public Raytracing::RaySegment
{
    PROPERTY_HEADER(Raytracing::LuxFeature);
public:
    /// Constructor
    LuxFeature(void);

    App::PropertyLink       Source;
    App::PropertyColor      Color;

    /** @name methods overide Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);

    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const { 
        return "Gui::ViewProviderDocumentObject"; 
    }
    //@}
};

} //namespace Raytracing

#endif //_LuxFeature_h_
