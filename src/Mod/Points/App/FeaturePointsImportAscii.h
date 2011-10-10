/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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


#ifndef FEATURE_Points_IMPORT_Ascii_H
#define FEATURE_Points_IMPORT_Ascii_H

#include "PointsFeature.h"

#include <App/PropertyStandard.h>


namespace Points
{

/**
 * The FeaturePointsImportAscii class reads the STL Points format
 * into the FreeCAD workspace.
 * @author Werner Mayer
 */
class ImportAscii : public Points::Feature
{
  PROPERTY_HEADER(Points::ImportAscii);

public:
  ImportAscii();

  App::PropertyString FileName;

  /** @name methods overide Feature */
  //@{
  /// recalculate the Feature
  App::DocumentObjectExecReturn *execute(void);
  short mustExecute() const;
  //@}
};

}

#endif // FEATURE_Points_IMPORT_STL_H 
