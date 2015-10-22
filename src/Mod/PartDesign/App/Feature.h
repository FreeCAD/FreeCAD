/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef PARTDESIGN_Feature_H
#define PARTDESIGN_Feature_H

#include <App/PropertyStandard.h>
#include <Mod/Part/App/PartFeature.h>

class gp_Pnt;


/// Base class of all additive features in PartDesign
namespace PartDesign
{

 /** PartDesign feature
 *   Base class of all PartDesign features.
 *   This kind of features only produce solids or fail.
 */
class PartDesignExport Feature : public Part::Feature
{
    PROPERTY_HEADER(PartDesign::Feature);

public:
    Feature();

protected:
    /**
     * Get a solid of the given shape. If no solid is found an exception is raised.
     */
    static TopoDS_Shape getSolid(const TopoDS_Shape&);

    /// Grab any point from the given face
    static const gp_Pnt getPointFromFace(const TopoDS_Face& f);

};

} //namespace PartDesign


#endif // PARTDESIGN_Feature_H
