// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 **************************************************************************/

#pragma once

#include <memory>

#include <QElapsedTimer>
#include <Inventor/SbVec2s.h>
#include <Inventor/nodes/SoSeparator.h>

#include <App/ColorModel.h>
#include <Base/Observer.h>

#include "ColorScaleOverlay.h"


class SoHandleEventAction;
class SoGLRenderAction;

namespace Gui
{

using SoLabelTextFormat = ColorScaleTextFormat;

class ColorScaleCoinPresentation;
class ColorScaleOptionsEvent;

/**
 * Foreground presentation for a scalar color scale.
 *
 * The node owns interaction and viewport integration. Its child graph is an implementation detail
 * built from ColorScaleOverlay snapshots.
 */
class GuiExport SoFCColorBar: public SoSeparator,
                              public App::ValueFloatToRGB,
                              public Base::Subject<int>
{
    using inherited = SoSeparator;

    SO_NODE_HEADER(Gui::SoFCColorBar);

public:
    static void initClass();
    static void finish();
    SoFCColorBar();

    /** Returns the color-scale state owned by this presentation node. */
    const ColorScaleOverlay& getColorScale() const;

    void GLRenderBelowPath(SoGLRenderAction* action) override;
    void handleEvent(SoHandleEventAction* action) override;

    /** Sets the range from the maximum \a fMax to the minimum \a fMin. */
    void setRange(float fMin, float fMax, int prec = 3);
    Base::Color getColor(float fVal) const override;
    void setOutsideGrayed(bool value);
    bool isVisible(float value) const;
    float getMinValue() const;
    float getMaxValue() const;
    void setFormat(const SoLabelTextFormat& format);

protected:
    ~SoFCColorBar() override;

private:
    friend class ColorScaleOptionsEvent;

    void prepareViewport(const SbVec2s& size, bool force = false);
    void updatePresentation();
    void setMode(ColorScaleMode mode);
    void customize();

    float getBounds(const SbVec2s& size, float& minimumX, float& minimumY, float& maximumX, float& maximumY);
    float getBoundingWidth(const SbVec2s& size);
    float snapToPixelGrid(float coordinate, const SbVec2s& size) const;
    float snapExtentToPixelGrid(float extent, const SbVec2s& size) const;

    QElapsedTimer timer;
    std::shared_ptr<ColorScaleOverlay> colorScale;
    std::unique_ptr<ColorScaleCoinPresentation> presentation;
    SbVec2s windowSize;
    float boxWidth {-1.0F};
};

}  // namespace Gui
