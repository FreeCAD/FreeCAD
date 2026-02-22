/****************************************************************************
 *   Copyright (c) 2019 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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

#include "ViewProvider.h"

namespace Gui
{

/// A class to create a Coin3D node representation of an coordinate system
class GuiExport AxisOrigin: public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    AxisOrigin();

    /// Set axis line width
    void setLineWidth(float size);

    /// Get axis line width
    float getLineWidth() const
    {
        return lineSize;
    }

    /// Set origin point size
    void setPointSize(float size);

    /// Get origin point size
    float getPointSize() const
    {
        return pointSize;
    }

    /// Set the axis line length
    void setAxisLength(float size);

    /// Get axis base line length
    float getAxisLength() const
    {
        return size;
    }

    /// Set the origin plane size and distance from the axis
    void setPlane(float size, float dist);

    /// Get the origin plane size and distance from the axis
    std::pair<float, float> getPlane() const
    {
        return std::make_pair(pSize, dist);
    }

    /// Set the auto scale factor, 0 to disable it
    void setScale(float scale);

    /// Get the auto scale factor
    float getScale() const
    {
        return scale;
    }

    /** Set customized names for axis components
     *
     * @param labels: the input names. Available keys are, O: origin,
     *                X: x axis, Y: y axis, Z: z axis, XY: XY plane,
     *                XZ: XY plane, YZ: YZ plane
     *
     * There are default labels for all components. You can also use
     * this function to choose which components are hidden, by not
     * include the key in the input labels.
     */
    void setLabels(const std::map<std::string, std::string>& labels);

    /// Obtain the axis component names
    const std::map<std::string, std::string>& getLabels() const
    {
        return labels;
    }

    /// Obtain the constructed Coin3D representation
    SoGroup* getNode();

    /** Return the name of picked element
     * @sa ViewProvider::getElementPicked()
     */
    bool getElementPicked(const SoPickedPoint* pp, std::string& subname) const;

    /** Return the coin path of a named element
     * @sa ViewProvider::getDetailPath()
     */
    bool getDetailPath(const char* subname, SoFullPath* pPath, SoDetail*& det) const;

private:
    float size = 6;
    float pSize = 4;
    float dist = 2;
    float scale = 1;
    float lineSize = 2;
    float pointSize = 4;
    std::map<std::string, std::string> labels;
    CoinPtr<SoGroup> node;
    std::map<std::string, CoinPtr<SoNode>> nodeMap;
};

}  // namespace Gui
