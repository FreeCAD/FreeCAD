/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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




#ifndef _AppPlacement_h_
#define _AppPlacement_h_

#include <Base/Placement.h>

#include "FeaturePython.h"
#include "GeoFeature.h"
#include "PropertyGeo.h"


namespace Base
{
//  class Vector3D;
  //class Matrix4D;
}

//using Base::Vector3D;
//using Base::Matrix4D;

namespace App
{


/** Placement Object
 *  Handles the repositioning of data. Also can do grouping
 */
class AppExport Placement: public App::GeoFeature
{
    PROPERTY_HEADER(App::Placement);

public:


  /// Constructor
  Placement(void);
  virtual ~Placement();
  
  /// returns the type name of the ViewProvider
  virtual const char* getViewProviderName(void) const {
      return "Gui::ViewProviderPlacement";
  }


};
typedef App::FeaturePythonT<App::Placement> PlacementPython;




} //namespace App



#endif
