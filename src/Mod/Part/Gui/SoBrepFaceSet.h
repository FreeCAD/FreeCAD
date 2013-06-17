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

#ifndef PARTGUI_SOBREPFACESET_H
#define PARTGUI_SOBREPFACESET_H
#if 0
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


#define RENDER_GLARRAYS

namespace PartGui {

class VertexArray;


class PartGuiExport SoBrepFaceSet : public SoIndexedFaceSet {
    typedef SoIndexedFaceSet inherited;

    SO_NODE_HEADER(SoBrepFaceSet);

public:
    static void initClass();
    SoBrepFaceSet();

    SoMFInt32 partIndex;
    SoSFInt32 highlightIndex;
    SoMFInt32 selectionIndex;

#ifdef RENDER_GLARRAYS
    std::vector<int32_t> index_array;
    std::vector<float> vertex_array;
#endif

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

protected:
    virtual ~SoBrepFaceSet();
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
    Binding findMaterialBinding(SoState * const state) const;
    Binding findNormalBinding(SoState * const state) const;
    void renderHighlight(SoGLRenderAction *action);
    void renderSelection(SoGLRenderAction *action);


	// low-level render functions

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

#ifdef RENDER_GLARRAYS
    void renderSimpleArray();
    void renderColoredArray(SoMaterialBundle *const materials);
#endif

private:
    SbColor selectionColor;
    SbColor highlightColor;
    SoColorPacker colorpacker;
};

} // namespace PartGui
#endif
#endif // PARTGUI_SOBREPFACESET_H

