/***************************************************************************
 *   Copyright (c) 2023 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com      *
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

#include "PreCompiled.h"

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
    writer.Stream() << writer.ind() << "<VisualLayer " << "visible=\""
                    << (visible ? std::string("true") : std::string("false")).c_str()
                    << "\" linePattern=\"" << linePattern << "\" lineWidth=\"" << lineWidth
                    << "\"/>" << std::endl;
}

void VisualLayer::Restore(Base::XMLReader& reader)
{
    reader.readElement("VisualLayer");

    std::string str = reader.getAttribute("visible");
    visible = (str == "true");

    linePattern = reader.getAttributeAsUnsigned("linePattern");
    lineWidth = reader.getAttributeAsFloat("lineWidth");
}
