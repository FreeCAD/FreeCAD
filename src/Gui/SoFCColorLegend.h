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

#include <Inventor/SbBox2f.h>
#include <Inventor/nodes/SoSeparator.h>

#include "SoFCColorBar.h"


class SoCoordinate3;
class SoMFString;
class SbVec2s;

namespace Gui
{

class GuiExport SoFCColorLegend: public SoFCColorBarBase
{
    using inherited = SoFCColorBarBase;

    SO_NODE_HEADER(Gui::SoFCColorLegend);

public:
    static void initClass();
    static void finish();
    SoFCColorLegend();

    void setLegendLabels(const App::ColorLegend& legend, int prec = 3);

    /**
     * Sets the range of the colorbar from the maximum \a fMax to the minimum \a fMin.
     * \a prec indicates the post decimal positions, \a prec should be in between 0 and 6.
     */
    void setRange(float fMin, float fMax, int prec = 3) override;
    /**
     * Updates the node with the given color legend.
     */
    void setColorLegend(const App::ColorLegend& legend);

    unsigned short getColorIndex(float fVal) const
    {
        return _currentLegend.getColorIndex(fVal);
    }
    Base::Color getColor(float fVal) const override
    {
        return _currentLegend.getColor(fVal);
    }
    void setOutsideGrayed(bool bVal) override
    {
        _currentLegend.setOutsideGrayed(bVal);
    }
    bool isVisible(float) const override
    {
        return false;
    }
    float getMinValue() const override
    {
        return _currentLegend.getMinValue();
    }
    float getMaxValue() const override
    {
        return _currentLegend.getMaxValue();
    }
    std::size_t countColors() const
    {
        return _currentLegend.hasNumberOfFields();
    }

    void customize(SoFCColorBarBase*) override
    {}
    const char* getColorBarName() const override;

    //  virtual void handleEvent(SoHandleEventAction * action);
    //  virtual void GLRenderBelowPath(SoGLRenderAction * action);
    //  virtual void GLRenderInPath(SoGLRenderAction * action);

protected:
    void setViewportSize(const SbVec2s& size) override;
    ~SoFCColorLegend() override;
    //  virtual void redrawHighlighted(SoAction * act, SbBool  flag);
private:
    void setMarkerLabel(const SoMFString& label);
    void setMarkerValue(const SoMFString& value);
    void modifyPoints(const SbBox2f&);
    void arrangeLabels(const SbBox2f&);
    void arrangeValues(const SbBox2f&);

private:
    SoCoordinate3* coords;
    SoSeparator* labelGroup;
    SoSeparator* valueGroup;
    SbBox2f _bbox;
    App::ColorLegend _currentLegend;
};

}  // namespace Gui
