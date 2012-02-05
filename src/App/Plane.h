/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2012     *
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




#ifndef _AppPlane_h_
#define _AppPlane_h_


#include "GeoFeature.h"
#include "PropertyGeo.h"



namespace App
{


/** Plane Object
 *  Used to define planar support for all kind of operations in the document space
 */
class AppExport Plane: public App::GeoFeature
{
    PROPERTY_HEADER(App::Plane);

public:

  /// Constructor
  Plane(void);
  virtual ~Plane();

  /// returns the type name of the ViewProvider
  virtual const char* getViewProviderName(void) const {
      return "Gui::ViewProviderPlane";
  }
};


} //namespace App



#endif
