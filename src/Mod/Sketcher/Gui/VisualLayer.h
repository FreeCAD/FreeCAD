// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2023 Abdullah Tahiri
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <map>
#include <memory>

namespace Base
{
class Writer;
class XMLReader;
}  // namespace Base

namespace SketcherGui
{

/** Provides the visual layer configuration for a class of geometry.
 *  A class of geometry can be any grouping of geometry, for
 * which the user wants to provide a per coin layer specific
 * configuration.
 */
class VisualLayer
{
public:
    explicit VisualLayer(unsigned int linePattern = 0xFFFF, float lineWidth = 3.0, bool visible = true);

    unsigned int getLinePattern() const;
    float getLineWidth() const;

    void setLinePattern(unsigned int linepattern);
    void setLineWidth(float linewidth);

    bool isVisible() const;
    void setVisible(bool show);

    void Save(Base::Writer& /*writer*/) const;
    void Restore(Base::XMLReader& /*reader*/);

private:
    unsigned int linePattern;
    float lineWidth;
    bool visible;

    friend inline bool operator==(VisualLayer const& lhs, VisualLayer const& rhs);
};

bool operator==(VisualLayer const& lhs, VisualLayer const& rhs)
{
    return (lhs.linePattern == rhs.linePattern) && (lhs.lineWidth == rhs.lineWidth)
        && (lhs.visible == rhs.visible);
}

}  // namespace SketcherGui
