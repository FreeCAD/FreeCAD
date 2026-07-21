/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/elements/SoReplacedElement.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>

#include <FCGlobal.h>

namespace Gui
{
/**
 * @author Werner Mayer
 */
class GuiExport SoFCInteractiveElement: public SoReplacedElement
{
    using inherited = SoReplacedElement;

    SO_ELEMENT_HEADER(SoFCInteractiveElement);

public:
    static void initClass();

    void init(SoState* state) override;
    static void set(SoState* const state, SoNode* const node, SbBool mode);
    static SbBool get(SoState* const state);
    static const SoFCInteractiveElement* getInstance(SoState* state);

protected:
    ~SoFCInteractiveElement() override;
    virtual void setElt(SbBool mode);

private:
    SbBool interactiveMode;
};

class GuiExport SoGLVBOActivatedElement: public SoElement
{
    using inherited = SoElement;

    SO_ELEMENT_HEADER(SoGLVBOActivatedElement);

public:
    static void initClass();

    void init(SoState* state) override;
    void push(SoState* state) override;
    void pop(SoState* state, const SoElement* prevTopElement) override;

    SbBool matches(const SoElement* element) const override;
    SoElement* copyMatchInfo() const override;

    static void set(SoState* state, SbBool);
    static void get(SoState* state, SbBool& active);

protected:
    ~SoGLVBOActivatedElement() override;

protected:
    SbBool active;
};

}  // namespace Gui
