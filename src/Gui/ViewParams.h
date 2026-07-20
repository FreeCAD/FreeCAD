/****************************************************************************
 *   Copyright (c) 2018 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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

#pragma once


#include <Base/ParameterObserver.h>

namespace Gui
{

class GuiExport ViewParams: public Base::ParameterObserver
{
public:
    ViewParams();
    static ViewParams* instance();
    using Base::ParameterObserver::getHandle;

    bool getUseNewSelection() const;
    void setUseNewSelection(bool v);

    bool getUseSelectionRoot() const;
    void setUseSelectionRoot(bool v);

    bool getEnableSelection() const;
    void setEnableSelection(bool v);

    long getRenderCache() const;
    void setRenderCache(long);

    bool getRandomColor() const;
    void setRandomColor(bool);

    unsigned long getBoundingBoxColor() const;
    void setBoundingBoxColor(unsigned long);

    unsigned long getAnnotationTextColor() const;
    void setAnnotationTextColor(unsigned long);

    long getMarkerSize() const;
    void setMarkerSize(long);

    unsigned long getDefaultLinkColor() const;
    void setDefaultLinkColor(unsigned long);

    unsigned long getDefaultShapeLineColor() const;
    void setDefaultShapeLineColor(unsigned long);

    unsigned long getDefaultShapeVertexColor() const;
    void setDefaultShapeVertexColor(unsigned long);

    unsigned long getDefaultShapeColor() const;
    void setDefaultShapeColor(unsigned long);

    long getDefaultShapeTransparency() const;
    void setDefaultShapeTransparency(long);

    long getDefaultShapeLineWidth() const;
    void setDefaultShapeLineWidth(long);

    long getDefaultShapePointSize() const;
    void setDefaultShapePointSize(long);

    bool getCoinCycleCheck() const;
    void setCoinCycleCheck(bool);

    bool getEnablePropertyViewForInactiveDocument() const;
    void setEnablePropertyViewForInactiveDocument(bool);

    bool getShowSelectionBoundingBox() const;
    void setShowSelectionBoundingBox(bool);

    unsigned long getPropertyViewTimer() const;
    void setPropertyViewTimer(unsigned long);

    unsigned long getAxisXColor() const;
    void setAxisXColor(unsigned long);

    unsigned long getAxisYColor() const;
    void setAxisYColor(unsigned long);

    unsigned long getAxisZColor() const;
    void setAxisZColor(unsigned long);

    unsigned long getOriginColor() const;
    void setOriginColor(unsigned long);

    unsigned long getNeutralColor() const;
    void setNeutralColor(unsigned long);

    double getPlacementIndicatorScale() const;
    void setPlacementIndicatorScale(double);

    double getDraggerScale() const;
    void setDraggerScale(double);

    double getDatumScale() const;
    void setDatumScale(double);

    double getDatumPlaneSize() const;
    void setDatumPlaneSize(double);

    double getDatumLineSize() const;
    void setDatumLineSize(double);

    double getDatumTemporaryScaleFactor() const;
    void setDatumTemporaryScaleFactor(double);

    bool getViewSelectionExtend() const;
    void setViewSelectionExtend(bool);

    double getViewSelectionExtendFactor() const;
    void setViewSelectionExtendFactor(double);

    double getSelectionLineThicken() const;
    void setSelectionLineThicken(double);

    double getSelectionLineMaxWidth() const;
    void setSelectionLineMaxWidth(double);

    double getSelectionBBoxLineWidth() const;
    void setSelectionBBoxLineWidth(double);

    long getMaxViewSelections() const;
    void setMaxViewSelections(long);

    unsigned long getSelectionColor() const;
    void setSelectionColor(unsigned long);

    bool getUseTightBoundingBox() const;
    void setUseTightBoundingBox(bool);

    bool getRenderProjectedBBox() const;
    void setRenderProjectedBBox(bool);

private:
    void setup();
};

}  // namespace Gui
