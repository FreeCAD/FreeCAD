/***************************************************************************
 *   Copyright (c) 2024 Kacper Donat <kacper@kadet.net>                    *
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
#ifndef GUI_SO3DANNOTATION_H
#define GUI_SO3DANNOTATION_H

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoSeparator.h>
#include <FCGlobal.h>

namespace Gui
{

/*! @brief 3D Annotation Node - Annotation with depth buffer
 *
 * This class is just like SoAnnotation with the difference that it does not disable
 * the depth buffer instead it clears it and renders on top of everything with proper
 * depth control.
 *
 * It should be used with caution as it does clear the depth buffer for each annotation!
 */
class GuiExport So3DAnnotation: public SoSeparator
{
    typedef SoSeparator inherited;

    SO_NODE_HEADER(So3DAnnotation);

public:
    static void initClass();
    So3DAnnotation();

    virtual void GLRender(SoGLRenderAction* action);
    virtual void GLRenderBelowPath(SoGLRenderAction* action);
    virtual void GLRenderInPath(SoGLRenderAction* action);
    virtual void GLRenderOffPath(SoGLRenderAction* action);

protected:
    virtual ~So3DAnnotation() = default;
};

}  // namespace Gui

#endif  // GUI_SO3DANNOTATION_H