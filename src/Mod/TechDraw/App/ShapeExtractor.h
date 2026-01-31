/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#pragma once

#include <TopoDS_Shape.hxx>

#include <App/DocumentObject.h>
#include <App/Link.h>
#include <Base/Type.h>
#include <Base/Vector3D.h>

#include <Mod/TechDraw/TechDrawGlobal.h>


namespace TechDraw
{

class TechDrawExport ShapeExtractor
{
public:
    static TopoDS_Shape getShapes(const std::vector<App::DocumentObject*> links, bool include2d = true);
    static std::vector<TopoDS_Shape> getShapes2d(const std::vector<App::DocumentObject*> links);
    static std::vector<TopoDS_Shape> getXShapes(const App::Link* xLink);
    static TopoDS_Shape getShapesFused(const std::vector<App::DocumentObject*> links);
    static TopoDS_Shape getShapeFromXLink(const App::Link* xLink);
    static std::vector<TopoDS_Shape> getShapesFromXRoot(const App::DocumentObject *xLinkRoot);
    static std::vector<TopoDS_Shape> getShapesFromObject(const App::DocumentObject* docObj);

    static bool is2dObject(const App::DocumentObject* obj);
    static bool isEdgeType(const App::DocumentObject* obj);
    static bool isPointType(const App::DocumentObject* obj);
    static bool isDraftPoint(const App::DocumentObject* obj);
    static bool isDatumPoint(const App::DocumentObject* obj);
    static bool isSketchObject(const App::DocumentObject* obj);
    static bool isExplodedAssembly(const App::DocumentObject* obj);

    static Base::Vector3d getLocation3dFromFeat(const App::DocumentObject *obj);
    static TopoDS_Shape getLocatedShape(const App::DocumentObject* docObj);

    static bool checkShape(const App::DocumentObject* shapeObj, TopoDS_Shape shape);

    static App::DocumentObject* getExplodedAssembly(std::vector<TopoDS_Shape>& sourceShapes,
                                                    App::DocumentObject* link);
    static void restoreExplodedAssembly(App::DocumentObject* link);

    static App::DocumentObject* getLinkedObject(const App::DocumentObject* root);

protected:

private:

};

} //namespace TechDraw