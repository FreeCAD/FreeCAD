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

#ifndef PARTGUI_SOBREPEDGESET_H
#define PARTGUI_SOBREPEDGESET_H

#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoMFInt32.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/fields/SoSubField.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoReplacedElement.h>
#include <vector>

class SoGLCoordinateElement;
class SoTextureCoordinateBundle;

namespace PartGui {

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

} // namespace PartGui


#endif // PARTGUI_SOBREPEDGESET_H

