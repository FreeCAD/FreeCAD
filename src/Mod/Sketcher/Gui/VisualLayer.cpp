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


#include <Base/Reader.h>
#include <Base/Writer.h>

#include "VisualLayer.h"

using namespace SketcherGui;

//**************** VisualClassConfig **************************************//

VisualLayer::VisualLayer(unsigned int linePattern, float lineWidth, bool visible)
    : linePattern(linePattern)
    , lineWidth(lineWidth)
    , visible(visible)
{}

unsigned int VisualLayer::getLinePattern() const
{
    return linePattern;
}

float VisualLayer::getLineWidth() const
{
    return lineWidth;
}

void VisualLayer::setLinePattern(unsigned int linepattern)
{
    linePattern = linepattern;
}

void VisualLayer::setLineWidth(float linewidth)
{
    lineWidth = linewidth;
}

bool VisualLayer::isVisible() const
{
    return visible;
}

void VisualLayer::setVisible(bool show)
{
    visible = show;
}

void VisualLayer::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<VisualLayer "
                    << "visible=\"" << (visible ? std::string("true") : std::string("false")).c_str()
                    << "\" linePattern=\"" << linePattern << "\" lineWidth=\"" << lineWidth
                    << "\"/>" << std::endl;
}

void VisualLayer::Restore(Base::XMLReader& reader)
{
    reader.readElement("VisualLayer");

    std::string str = reader.getAttribute<const char*>("visible");
    visible = (str == "true");

    linePattern = reader.getAttribute<unsigned long>("linePattern");
    lineWidth = reader.getAttribute<double>("lineWidth");
}
