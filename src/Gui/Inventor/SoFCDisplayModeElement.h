/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef FC_SOFCDISPLAYMODEELEMENT_H
#define FC_SOFCDISPLAYMODEELEMENT_H

#include <Inventor/elements/SoReplacedElement.h>

class GuiExport SoFCDisplayModeElement: public SoReplacedElement {
    typedef SoReplacedElement inherited;

    SO_ELEMENT_HEADER(SoFCDisplayModeElement);

public:
    static void initClass(void);
protected:
    virtual ~SoFCDisplayModeElement();

public:
    virtual void init(SoState *state);

    static void set(SoState * const state, SoNode * const node,
            const SbName &mode, SbBool hiddenLine);

    static void setColors(SoState * const state, SoNode * const node,
            const SbColor *faceColor, const SbColor *lineColor, float transp);

    static const SbName &get(SoState * const state);
    static SbBool showHiddenLines(SoState * const state);
    static const SbColor *getFaceColor(SoState * const state);
    static const SbColor *getLineColor(SoState * const state);
    static float getTransparency(SoState * const state);

    static SoFCDisplayModeElement * getInstance(SoState *state);

    const SbName &get() const;
    SbBool showHiddenLines() const;
    const SbColor *getFaceColor() const;
    const SbColor *getLineColor() const;
    float getTransparency() const;

    virtual SbBool matches(const SoElement * element) const;
    virtual SoElement *copyMatchInfo(void) const;

protected:
    SbName displayMode;
    SbBool hiddenLines;
    SbBool hasFaceColor;
    SbBool hasLineColor;
    SbColor faceColor;
    SbColor lineColor;
    float transp;
};

#endif // FC_SOFCDISPLAYMODEELEMENT_H
// vim: noai:ts=2:sw=2
