// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2005 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <FCConfig.h>

#ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
#else
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# include <GL/gl.h>
#endif

#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/nodes/SoGroup.h>

#include "SoFCSelectionContext.h"


class SoFullPath;
class SoPickedPoint;


namespace Gui
{


/** Selection node
 *  This node does the complete highlighting and selection together with the viewer
 *  \author Jürgen Riegel
 */
class GuiExport SoFCSelection: public SoGroup
{
    using inherited = SoGroup;

    SO_NODE_HEADER(Gui::SoFCSelection);

public:
    static void initClass();
    static void finish();
    SoFCSelection();

    /// Load highlight settings from the configuration
    void applySettings();

    enum PreselectionModes
    {
        AUTO,
        ON,
        OFF
    };

    enum SelectionModes
    {
        SEL_ON,
        SEL_OFF
    };

    enum Selected
    {
        NOTSELECTED,
        SELECTED
    };

    enum Styles
    {
        EMISSIVE,
        EMISSIVE_DIFFUSE,
        BOX
    };

    SbBool isHighlighted() const
    {
        return highlighted;
    }

    SoSFColor colorHighlight;
    SoSFColor colorSelection;
    SoSFEnum style;
    SoSFEnum selected;
    SoSFEnum preselectionMode;
    SoSFEnum selectionMode;

    SoSFString documentName;
    SoSFString objectName;
    SoSFString subElementName;
    SoSFBool useNewSelection;

    void doAction(SoAction* action) override;
    void GLRender(SoGLRenderAction* action) override;

    void handleEvent(SoHandleEventAction* action) override;
    void GLRenderBelowPath(SoGLRenderAction* action) override;
    void GLRenderInPath(SoGLRenderAction* action) override;
    static void turnOffCurrentHighlight(SoGLRenderAction* action);

protected:
    ~SoFCSelection() override;

    using SelContext = SoFCSelectionContext;
    using SelContextPtr = std::shared_ptr<SelContext>;
    SelContextPtr selContext;
    SelContextPtr selContext2;

    virtual void redrawHighlighted(SoAction* act, SbBool flag);

    SbBool readInstance(SoInput* in, unsigned short flags) override;

private:
    static int getPriority(const SoPickedPoint*);
    static void turnoffcurrent(SoAction* action);
    bool setOverride(SoGLRenderAction* action, SelContextPtr);
    SbBool isHighlighted(SoAction* action);
    SbBool preRender(SoGLRenderAction* act, GLint& oldDepthFunc);
    const SoPickedPoint* getPickedPoint(SoHandleEventAction*) const;

    static SoFullPath* currenthighlight;

    SbBool highlighted;
    SoColorPacker colorpacker;

    SbBool bShift;
    SbBool bCtrl;
};


}  // namespace Gui
