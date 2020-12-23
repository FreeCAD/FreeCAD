/***************************************************************************
 *   Copyright (c) 2019 WandererFan    <wandererfan@gmail.com>             *
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

#ifndef _ShapeExtractor_h_
#define _ShapeExtractor_h_

#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <App/GroupExtension.h>
#include <App/Part.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>

namespace TechDraw
{

class TechDrawExport ShapeExtractor
{
public:
    static TopoDS_Shape getShapes(const std::vector<App::DocumentObject*> links); 
    static std::vector<TopoDS_Shape> getShapesFromObject(const App::DocumentObject* docObj);
    static TopoDS_Shape getShapesFused(const std::vector<App::DocumentObject*> links);
    static std::vector<TopoDS_Shape> extractDrawableShapes(const TopoDS_Shape shapeIn);

protected:

private:

};

} //namespace TechDraw

#endif  // #ifndef _ShapeExtractor_h_
