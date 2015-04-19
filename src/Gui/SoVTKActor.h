/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger  <stefantroeger@gmx.net>             *
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

#ifndef GUI_SOVTKACTOR_H
#define GUI_SOVTKACTOR_H

#ifdef FC_USE_VTK

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedMarkerSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoPackedColor.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoNormal.h>

class vtkPoints;
class vtkDataArray;
class vtkUnsignedCharArray;
class vtkMapper;
class SoGLCoordinateElement;
class SoTextureCoordinateBundle;

namespace Gui {

class GuiExport SoVTKActor : public SoSeparator {

    SO_NODE_HEADER(SoVTKActor);

public:   
    static void initClass();
    SoVTKActor();    
  
    //vtkActor functions
    static SoVTKActor *New();
    
    void setMapper(vtkMapper* m);
    vtkMapper* getMapper();
    
protected:
    virtual ~SoVTKActor();
    
    //SoNode functions
    /*
    virtual void GLRender(SoGLRenderAction *action);
    virtual void GLRenderBelowPath(SoGLRenderAction* action);
    virtual void GLRenderInPath(SoGLRenderAction* action);
    virtual void GLRenderOffPath(SoGLRenderAction* action);*/
    virtual void doAction(SoAction* action); 
    
    void WritePointData(vtkPoints *points, vtkDataArray *normals,
                        vtkDataArray *tcoords,
                        vtkUnsignedCharArray *colors);
    
    vtkMapper*                  m_mapper;
    SoCoordinate3*              m_coordinates;
    SoIndexedMarkerSet*         m_markers;
    SoIndexedLineSet*           m_lines;
    SoIndexedFaceSet*           m_faces;    
    SoIndexedTriangleStripSet*  m_triangleStrips;
    SoMaterialBinding*          m_materialBinding;
    SoShapeHints*               m_shapeHints;
    SoMaterial*                 m_material;
    SoNormalBinding*            m_normalBinding;
    SoNormal*                   m_normals;
    
};

} // namespace PartGui

#endif // FC_USE_VTK
#endif // GUI_SOVTKACTOR_H

