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

#include <vector>
#include <Inventor/SbBox2f.h>
#include <Inventor/nodes/SoSeparator.h>

#include "SoFCColorBar.h"


class SoCoordinate3;
class SoIndexedFaceSet;
class SoMaterial;
class SoMaterialBinding;
class SoMFString;
class SoTransparencyType;
class SbVec2s;

namespace Gui
{

class GuiExport SoFCColorGradient: public SoFCColorBarBase
{
    using inherited = SoFCColorBarBase;

    SO_NODE_HEADER(Gui::SoFCColorGradient);

public:
    static void initClass();
    static void finish();
    SoFCColorGradient();

    /**
     * Sets the range of the colorbar from the maximum \a fMax to the minimum \a fMin.
     * \a prec indicates the post decimal positions, \a prec should be in between 0 and 6.
     */
    void setRange(float fMin, float fMax, int prec = 3) override;
    /**
     * Returns the associated color to the value \a fVal.
     */
    Base::Color getColor(float fVal) const override
    {
        return _cColGrad.getColor(fVal);
    }
    void setOutsideGrayed(bool bVal) override
    {
        _cColGrad.setOutsideGrayed(bVal);
    }
    /**
     * Returns always true if the gradient is in mode to show colors to arbitrary values of \a fVal,
     * otherwise true is returned if \a fVal is within the specified parameter range, if not false
     * is returned.
     */
    bool isVisible(float fVal) const override;
    /** Returns the current minimum of the parameter range. */
    float getMinValue() const override
    {
        return _cColGrad.getMinValue();
    }
    /** Returns the current maximum of the parameter range. */
    float getMaxValue() const override
    {
        return _cColGrad.getMaxValue();
    }
    /**
     * Opens a dialog to customize the current settings of the color gradient bar.
     */
    void customize(SoFCColorBarBase*) override;
    /** Returns the name of the color bar. */
    const char* getColorBarName() const override;

protected:
    void applyFormat(const SoLabelTextFormat& fmt) override;
    /**
     * Sets the current viewer size this color gradient is embedded into, to recalculate its new
     * position.
     */
    void setViewportSize(const SbVec2s& size) override;

    ~SoFCColorGradient() override;
    /**
     * Sets the color model of the underlying color gradient to \a index.
     */
    void setColorModel(std::size_t index);
    /**
     * Sets the color style of the underlying color gradient to \a tStyle. \a tStyle either can
     * be \c FLOW or \c ZERO_BASED
     */
    void setColorStyle(App::ColorBarStyle tStyle);
    /** Rebuild the gradient bar. */
    void rebuildGradient();
    /** Returns a list of \a count labels within the range [\a fMin, \a fMax].  */
    std::vector<float> getMarkerValues(float fMin, float fMax, int count) const;

private:
    bool isZeroBased(float fMin, float fMax) const;
    /** Sets the new labels. */
    int getNumColors() const;
    void setMarkerLabel(const SoMFString& label);
    void modifyPoints(const SbBox2f&);
    void setCoordSize(int numPoints);
    SoIndexedFaceSet* createFaceSet(int numFaces) const;
    SoTransparencyType* createTransparencyType() const;
    SoMaterial* createMaterial() const;
    SoMaterialBinding* createMaterialBinding() const;

private:
    SoCoordinate3* coords;
    SoSeparator* labels;
    SbBox2f _bbox;
    int _precision {3};
    App::ColorGradient _cColGrad;
};

}  // namespace Gui
