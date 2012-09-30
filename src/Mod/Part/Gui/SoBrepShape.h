/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef PARTGUI_SOBREPSHAPE_H
#define PARTGUI_SOBREPSHAPE_H

#include <Inventor/fields/SoMFInt32.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/fields/SoSubField.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoReplacedElement.h>
#include <vector>

class SoGLCoordinateElement;
class SoTextureCoordinateBundle;

namespace PartGui {

class PartGuiExport SoBrepFaceSet : public SoIndexedFaceSet {
    typedef SoIndexedFaceSet inherited;

    SO_NODE_HEADER(SoBrepFaceSet);

public:
    static void initClass();
    SoBrepFaceSet();

    SoMFInt32 partIndex;
    SoSFInt32 highlightIndex;
    SoMFInt32 selectionIndex;

protected:
    virtual ~SoBrepFaceSet() {};
    virtual void GLRender(SoGLRenderAction *action);
    virtual void GLRenderBelowPath(SoGLRenderAction * action);
    virtual void doAction(SoAction* action); 
    virtual SoDetail * createTriangleDetail(
        SoRayPickAction * action,
        const SoPrimitiveVertex * v1,
        const SoPrimitiveVertex * v2,
        const SoPrimitiveVertex * v3,
        SoPickedPoint * pp);
    virtual void generatePrimitives(SoAction * action);

private:
    enum Binding {
        OVERALL = 0,
        PER_PART,
        PER_PART_INDEXED,
        PER_FACE,
        PER_FACE_INDEXED,
        PER_VERTEX,
        PER_VERTEX_INDEXED,
        NONE = OVERALL
    };
    Binding findMaterialBinding(SoState * const state) const;
    Binding findNormalBinding(SoState * const state) const;
    void renderShape(const SoGLCoordinateElement * const vertexlist,
                     const int32_t *vertexindices,
                     int num_vertexindices,
                     const int32_t *partindices,
                     int num_partindices,
                     const SbVec3f *normals,
                     const int32_t *normindices,
                     SoMaterialBundle *const materials,
                     const int32_t *matindices,
                     SoTextureCoordinateBundle * const texcoords,
                     const int32_t *texindices,
                     const int nbind,
                     const int mbind,
                     const int texture);
    void renderHighlight(SoGLRenderAction *action);
    void renderSelection(SoGLRenderAction *action);

private:
    SbColor selectionColor;
    SbColor highlightColor;
    SoColorPacker colorpacker;
};

// ---------------------------------------------------------------------

class PartGuiExport SoBrepEdgeSet : public SoIndexedLineSet {
    typedef SoIndexedLineSet inherited;

    SO_NODE_HEADER(SoBrepEdgeSet);

public:
    static void initClass();
    SoBrepEdgeSet();

    SoSFInt32 highlightIndex;
    SoMFInt32 selectionIndex;

protected:
    virtual ~SoBrepEdgeSet() {};
    virtual void GLRender(SoGLRenderAction *action);
    virtual void GLRenderBelowPath(SoGLRenderAction * action);
    virtual void doAction(SoAction* action); 
    virtual SoDetail * createLineSegmentDetail(
        SoRayPickAction *action,
        const SoPrimitiveVertex *v1,
        const SoPrimitiveVertex *v2,
        SoPickedPoint *pp);
private:
    void renderShape(const SoGLCoordinateElement * const vertexlist,
                     const int32_t *vertexindices,
                     int num_vertexindices);
    void renderHighlight(SoGLRenderAction *action);
    void renderSelection(SoGLRenderAction *action);

private:
    std::vector<int32_t> hl, sl;
    SbColor selectionColor;
    SbColor highlightColor;
    //#0000834: Minor preselection color bug
    //To solve this we need a seprate color packer for highlighting and selection
    SoColorPacker colorpacker1;
    SoColorPacker colorpacker2;
};

// ---------------------------------------------------------------------

class PartGuiExport SoBrepPointSet : public SoPointSet {
    typedef SoPointSet inherited;

    SO_NODE_HEADER(SoBrepPointSet);

public:
    static void initClass();
    SoBrepPointSet();

    SoSFInt32 highlightIndex;
    SoMFInt32 selectionIndex;

protected:
    virtual ~SoBrepPointSet() {};
    virtual void GLRender(SoGLRenderAction *action);
    virtual void GLRenderBelowPath(SoGLRenderAction * action);
    virtual void doAction(SoAction* action); 

private:
    void renderShape(const SoGLCoordinateElement * const vertexlist,
                     const int32_t *vertexindices,
                     int num_vertexindices);
    void renderHighlight(SoGLRenderAction *action);
    void renderSelection(SoGLRenderAction *action);

private:
    SbColor selectionColor;
    SbColor highlightColor;
    SoColorPacker colorpacker;
};

} // namespace PartGui


#endif // PARTGUI_SOBREPSHAPE_H

