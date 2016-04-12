/***************************************************************************
 *   Copyright (c) J�rgen Riegel          (juergen.riegel@web.de) 2012     *
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




#ifndef _AppLine_h_
#define _AppLine_h_


#include "GeoFeature.h"
#include "PropertyGeo.h"



namespace App
{


/** Line Object
 *  Used to define planar support for all kind of operations in the document space
 */
class AppExport Line: public App::GeoFeature
{
    PROPERTY_HEADER(App::Line);

public:

  /// Constructor
  Line(void);
  virtual ~Line();
  /// additional information about the plane usage (e.g. "BaseLine-xy" in a Part)
  PropertyString LineType;


  /// returns the type name of the ViewProvider
  virtual const char* getViewProviderName(void) const {
      return "Gui::ViewProviderLine";
  }

  /// Return the bounding box of the plane (this is always a fixed size)
  static Base::BoundBox3d getBoundBox();
};


} //namespace App



#endif
