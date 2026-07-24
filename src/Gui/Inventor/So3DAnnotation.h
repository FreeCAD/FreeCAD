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
#include <Inventor/SoPath.h>
#include <Inventor/misc/SoRefPtr.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/elements/SoElement.h>
#include <Inventor/elements/SoSubElement.h>
#include <FCGlobal.h>
#include <vector>

class SoAction;
class SoIRRenderAction;

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
        SoRefPtr<SoPath> path;
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

    static bool hasDelayedPaths(SoState* state);

    static bool isProcessingDelayedPaths(SoState* state);
    static void setProcessingDelayedPaths(SoState* state, bool processing);

    static SoPathList getDelayedPaths(SoState* state);

    static void processDelayedPathsWithPriority(SoState* state, SoGLRenderAction* action);
    static void processDelayedPathsWithPriority(SoState* state, SoIRRenderAction* action);

    SbBool matches([[maybe_unused]] const SoElement* element) const override
    {
        return FALSE;
    }

    SoElement* copyMatchInfo() const override
    {
        return nullptr;
    }

private:
    static SoDelayedAnnotationsElement* getElement(SoState* state);

    std::vector<PriorityPath> paths;
    bool processingDelayedPaths {false};
};

/*! @brief 3D Annotation Node - Annotation with depth buffer
 *
 * This class queues its subtree for the after-main stage, which clears the main-scene
 * depth buffer once before rendering all delayed 3D annotations.
 */
class GuiExport So3DAnnotation: public SoSeparator
{
    using inherited = SoSeparator;

    SO_NODE_HEADER(So3DAnnotation);

public:
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

private:
    static void IRRender(SoAction* action, SoNode* node);

protected:
    ~So3DAnnotation() override = default;
};

}  // namespace Gui
