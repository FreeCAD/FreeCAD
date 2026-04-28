// SPDX-License-Identifier: LGPL-2.1-or-later

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
#pragma once

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/elements/SoElement.h>
#include <Inventor/elements/SoSubElement.h>
#include <FCGlobal.h>
#include <vector>

namespace Gui
{

class GuiExport SoDelayedAnnotationsElement: public SoElement
{
    using inherited = SoElement;

    SO_ELEMENT_HEADER(SoDelayedAnnotationsElement);

protected:
    ~SoDelayedAnnotationsElement() override = default;

    SoDelayedAnnotationsElement& operator=(const SoDelayedAnnotationsElement& other) = default;
    SoDelayedAnnotationsElement& operator=(SoDelayedAnnotationsElement&& other) noexcept = default;

    // internal structure to hold path with it's rendering
    // priority (lower renders first)
    struct PriorityPath
    {
        SoPath* path;
        int priority;

        PriorityPath(SoPath* p, int pr = 0)
            : path(p)
            , priority(pr)
        {}
    };

public:
    SoDelayedAnnotationsElement(const SoDelayedAnnotationsElement& other) = delete;
    SoDelayedAnnotationsElement(SoDelayedAnnotationsElement&& other) noexcept = delete;

    void init(SoState* state) override;

    static void initClass();

    static void addDelayedPath(SoState* state, SoPath* path, int priority = 0);

    static SoPathList getDelayedPaths(SoState* state);

    static void processDelayedPathsWithPriority(SoState* state, SoGLRenderAction* action);

    static bool isProcessingDelayedPaths;

    SbBool matches([[maybe_unused]] const SoElement* element) const override
    {
        return FALSE;
    }

    SoElement* copyMatchInfo() const override
    {
        return nullptr;
    }

private:
    std::vector<PriorityPath> paths;
};

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
    using inherited = SoSeparator;

    SO_NODE_HEADER(So3DAnnotation);

public:
    static bool render;

    So3DAnnotation();

    So3DAnnotation(const So3DAnnotation& other) = delete;
    So3DAnnotation(So3DAnnotation&& other) noexcept = delete;
    So3DAnnotation& operator=(const So3DAnnotation& other) = delete;
    So3DAnnotation& operator=(So3DAnnotation&& other) noexcept = delete;

    static void initClass();

    void GLRender(SoGLRenderAction* action) override;
    void GLRenderBelowPath(SoGLRenderAction* action) override;
    void GLRenderInPath(SoGLRenderAction* action) override;
    void GLRenderOffPath(SoGLRenderAction* action) override;

protected:
    ~So3DAnnotation() override = default;
};

}  // namespace Gui
