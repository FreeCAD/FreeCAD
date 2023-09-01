/***************************************************************************
 *   Copyright (c) 2018 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PARTGUI_BOXSELECTION_H
#define PARTGUI_BOXSELECTION_H

#include <TopAbs_ShapeEnum.hxx>

class SoEventCallback;
class TopoDS_Shape;

namespace Base {
class Polygon2d;
}

namespace Gui {
class View3DInventorViewer;
class ViewVolumeProjection;
}

namespace PartGui {

class BoxSelection
{
public:
    BoxSelection();
    ~BoxSelection();

    void setAutoDelete(bool);
    bool isAutoDelete() const;
    void start(TopAbs_ShapeEnum shape);

private:
    class FaceSelectionGate;
    void addShapeToSelection(const char* doc, const char* obj,
                             const Gui::ViewVolumeProjection& proj,
                             const Base::Polygon2d& polygon,
                             const TopoDS_Shape& shape,
                             TopAbs_ShapeEnum subtype);
    const char* nameFromShapeType(TopAbs_ShapeEnum) const;
    static void selectionCallback(void * ud, SoEventCallback * cb);

private:
    bool autodelete{false};
    TopAbs_ShapeEnum shapeEnum{TopAbs_SHAPE};
};

} //namespace PartGui

#endif // PARTGUI_BOXSELECTION_H
