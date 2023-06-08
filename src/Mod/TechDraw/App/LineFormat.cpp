/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#include "LineGroup.h"
#include "LineFormat.h"
#include "Preferences.h"
#include <Base/Persistence.h>

using namespace TechDraw;

// TYPESYSTEM_SOURCE(TechDraw::LineFormat, Base::Persistence)

LineFormat::LineFormat()
{
    m_style = getDefEdgeStyle();
    m_weight = getDefEdgeWidth();
    m_color = getDefEdgeColor();
    m_visible = true;
}

LineFormat::LineFormat(int style,
               double weight,
               App::Color color,
               bool visible) :
    m_style(style),
    m_weight(weight),
    m_color(color),
    m_visible(visible)
{
}

void LineFormat::dump(const char* title)
{
    Base::Console().Message("LF::dump - %s \n", title);
    Base::Console().Message("LF::dump - %s \n", toString().c_str());
}

std::string LineFormat::toString() const
{
    std::stringstream ss;
    ss << m_style << ", " <<
          m_weight << ", " <<
          m_color.asHexString() << ", " <<
          m_visible;
    return ss.str();
}

//static preference getters.
double LineFormat::getDefEdgeWidth()
{
    return TechDraw::LineGroup::getDefaultWidth("Graphic");
}

App::Color LineFormat::getDefEdgeColor()
{
    return Preferences::normalColor();
}

int LineFormat::getDefEdgeStyle()
{
    return Preferences::getPreferenceGroup("Decorations")->GetInt("CosmoCLStyle", 2);   //dashed
}
