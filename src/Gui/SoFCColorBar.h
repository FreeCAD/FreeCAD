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

#include <QElapsedTimer>
#include <vector>
#include <Inventor/SbVec2s.h>
#include <Inventor/nodes/SoSeparator.h>

#include <App/ColorModel.h>
#include <Base/Observer.h>


class SoSwitch;
class SoEventCallback;
class SbVec2s;
class SoHandleEventAction;
class SoGLRenderAction;

namespace Gui
{
class SoFCColorGradient;

struct SoLabelTextFormat
{
    // NOLINTBEGIN
    int textSize = 13;
    uint32_t textColor = 0xffffffff;
    // NOLINTEND
};

/**
 * The abstract color bar base class to get most important information on how to convert a scalar to
 * an RGB color.
 * @author Werner Mayer
 */
class GuiExport SoFCColorBarBase: public SoSeparator, public App::ValueFloatToRGB
{
    using inherited = SoSeparator;

    SO_NODE_ABSTRACT_HEADER(Gui::SoFCColorBarBase);

public:
    static void initClass();
    static void finish();

    void GLRenderBelowPath(SoGLRenderAction* action) override;

    /**
     * Sets the range of the colorbar from the maximum \a fMax to the minimum \a fMin.
     * \a prec indicates the post decimal positions, \a prec should be in between 0 and 6.
     *
     * This method must be implemented in subclasses.
     */
    virtual void setRange(float fMin, float fMax, int prec = 3) = 0;
    /**
     * Returns the associated color to the value \a fVal.
     *
     * This method must be implemented in subclasses.
     */
    Base::Color getColor(float fVal) const override = 0;
    /**
     * Returns always true if the color bar is in mode to show colors to arbitrary values of \a
     * fVal, otherwise true is returned if \a fVal is within the specified parameter range, if not
     * false is returned.
     *
     * This method must be implemented in subclasses.
     */
    virtual bool isVisible(float fVal) const = 0;
    /**
     * Sets whether values outside the range should be in gray,
     *
     * This method must be implemented in subclasses.
     */
    virtual void setOutsideGrayed(bool bVal) = 0;
    /** Returns the current minimum of the parameter range.
     *
     * This method must be implemented in subclasses.
     */
    virtual float getMinValue() const = 0;
    /** Returns the current maximum of the parameter range.
     *
     * This method must be implemented in subclasses.
     */
    virtual float getMaxValue() const = 0;
    /**
     * Opens a dialog to customize the current settings of the color bar.
     *
     * This method must be implemented in subclasses.
     */
    virtual void customize(SoFCColorBarBase*) = 0;
    /**
     * Forward a triggered change
     */
    virtual void triggerChange(SoFCColorBarBase* base)
    {
        base->triggerChange(this);
    }
    /** Returns the name of the color bar.
     *
     * This method must be implemented in subclasses.
     */
    virtual const char* getColorBarName() const = 0;
    /** Sets the format for the label text.
     */
    virtual void setFormat(const SoLabelTextFormat& fmt);
    /** Returns the format for the label text.
     */
    SoLabelTextFormat getFormat() const;

protected:
    /** Applies the format to the label text */
    virtual void applyFormat(const SoLabelTextFormat& fmt);
    /** Computes the dimensions of the color bar and labels in coordinates with
     * respect to the defined height of the camera.
     * Returns the width of the bounding box
     */
    float getBounds(const SbVec2s& size, float& fMinX, float& fMinY, float& fMaxX, float& fMaxY);
    /** Returns the width of the color bar and labels
     *
     * Computes the occupied width of the color bar and its labels.
     * It therefore determines the bounding box.
     */
    float getBoundingWidth(const SbVec2s& size);
    /**
     * Sets the current viewer size to recalculate the new position.
     *
     * This method must be implemented in subclasses.
     */
    virtual void setViewportSize(const SbVec2s& size) = 0;
    /**
     * Mark the object as modified.
     */
    void setModified();

    SoFCColorBarBase();
    ~SoFCColorBarBase() override;

private:
    float _boxWidth {-1.0F};
    SbVec2s _windowSize;
    SoLabelTextFormat format;
};

// --------------------------------------------------------------------------

/**
 * The color bar class that redirects all calls to its handled color bars.
 * @author Werner Mayer
 */
class GuiExport SoFCColorBar: public SoFCColorBarBase, public Base::Subject<int>
{
    using inherited = SoFCColorBarBase;

    SO_NODE_HEADER(Gui::SoFCColorBar);

public:
    static void initClass();
    static void finish();
    SoFCColorBar();

    /**
     * Returns the currently active color bar object.
     */
    SoFCColorBarBase* getActiveBar() const;
    /**
     * Handles the mouse button events and checks if the user has clicked on the area of the
     * currently active color bar.
     */
    void handleEvent(SoHandleEventAction* action) override;
    /**
     * Sets the range of all color bars from the maximum \a fMax to the minimum \a fMin.
     * \a prec indicates the post decimal positions, \a prec should be in between 0 and 6.
     */
    void setRange(float fMin, float fMax, int prec = 3) override;
    /**
     * Returns the associated color to the value \a fVal of the currently active color bar.
     */
    Base::Color getColor(float fVal) const override;
    /**
     * Sets whether values outside the range should be in gray,
     */
    void setOutsideGrayed(bool bVal) override;
    /**
     * Returns the return value of the currently active color bar.
     */
    bool isVisible(float fVal) const override;
    /**
     * Returns the current minimum of the parameter range of the currently active color bar.
     */
    float getMinValue() const override;
    /**
     * Returns the current maximum of the parameter range of the currently active color bar.
     */
    float getMaxValue() const override;
    /**
     * Customizes the currently active color bar.
     */
    void customize(SoFCColorBarBase*) override;
    /**
     * Notify observers
     */
    void triggerChange(SoFCColorBarBase*) override;
    /** Returns the name of the color bar.
     */
    const char* getColorBarName() const override
    {
        return "Color Bar";
    }
    void setFormat(const SoLabelTextFormat& fmt) override;

protected:
    /**
     * Sets the current viewer size to all color bars to recalculate their new position.
     */
    void setViewportSize(const SbVec2s& size) override;

    ~SoFCColorBar() override;

private:
    static void eventCallback(void* userdata, SoEventCallback* node);

private:
    QElapsedTimer _timer;

    SoSwitch* pColorMode;
    std::vector<SoFCColorBarBase*> _colorBars;
};

}  // namespace Gui
