# -*- coding: utf8 -*-
# ***************************************************************************
# *   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2018 George Shuklin (amarao)                            *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides functions to return the SVG representation of text elements."""
## @package svgtext
# \ingroup draftfunctions
# \brief Provides functions to return the SVG representation of text elements.

import math

import FreeCAD as App
import draftutils.utils as utils

## \addtogroup draftfunctions
# @{


def _get_text_techdraw(text, tcolor, fontsize, anchor,
                       align, fontname, angle, base,
                       linespacing):
    """Return the SVG representation of text for TechDraw display.

    `text` is a list of textual elements; they are iterated, styled,
    and added around a `<text>` tag.
    ::
        <text ...> text[0] </text>
        <text ...> text[1] </text>
    """
    svg = ""
    for i in range(len(text)):
        _t = text[i].replace("&", "&amp;")
        _t = _t.replace("<", "&lt;")
        t = _t.replace(">", "&gt;")

        svg += '<text '
        svg += 'stroke-width="0" stroke="{}" '.format(tcolor)
        svg += 'fill="{}" font-size="{}" '.format(tcolor, fontsize)
        svg += 'style="text-anchor:{};text-align:{};'.format(anchor,
                                                             align.lower())
        svg += 'font-family:{}" '.format(fontname)
        svg += 'transform="'
        svg += 'rotate({},{},{}) '.format(math.degrees(angle),
                                          base.x,
                                          base.y - i * linespacing)
        svg += 'translate({},{}) '.format(base.x,
                                          base.y - i * linespacing)
        svg += 'scale(1,-1)"'
        # svg += 'freecad:skip="1"'
        svg += '>\n'
        svg += t
        svg += '</text>\n'
    return svg


def _get_text_header(tcolor, fontsize, anchor, align,
                     fontname, angle, base, flip):
    """Return the initial <text> tag with style options.

    The text must be added after this tag, and then must be closed.
    ::
        <text ...>
        ...
        </text>
    """
    svg = '<text '
    svg += 'stroke-width="0" stroke="{}" '.format(tcolor)
    svg += 'fill="{}" font-size="{}" '.format(tcolor, fontsize)
    svg += 'style="text-anchor:{};text-align:{};'.format(anchor,
                                                         align.lower())
    svg += 'font-family:{}" '.format(fontname)
    svg += 'transform="'
    svg += 'rotate({},{},{}) '.format(math.degrees(angle),
                                      base.x,
                                      base.y)
    if flip:
        svg += 'translate({},{}) '.format(base.x, base.y)
    else:
        svg += 'translate({},{}) '.format(base.x, -base.y)
    # svg += 'scale({},-{}) '.format(tmod/2000, tmod/2000)

    if flip:
        svg += 'scale(1,-1) '
    else:
        svg += 'scale(1,1) '

    svg += '" '
    svg += 'freecad:skip="1"'
    svg += '>\n'
    return svg


def get_text(plane, techdraw,
             tcolor, fontsize, fontname,
             angle, base, text,
             linespacing=0.5, align="center", flip=True):
    """Get the SVG representation of a textual element."""
    if isinstance(angle, App.Rotation):
        if not plane:
            angle = angle.Angle
        else:
            if plane.axis.getAngle(angle.Axis) < 0.001:
                angle = angle.Angle
            elif abs(plane.axis.getAngle(angle.Axis) - math.pi) < 0.001:
                if abs(angle.Angle) > 0.1:
                    angle = -angle.Angle
                else:
                    angle = angle.Angle
            elif abs(plane.axis.getAngle(angle.Axis) - math.pi/2) < 0.001:
                # text is perpendicular to view, so it shouldn't appear
                return ""
            else:
                # TODO maybe there is something better to do here?
                angle = 0

    # text should be a list of strings separated by a newline
    if not isinstance(text, list):
        text = text.split("\n")

    if align.lower() == "center":
        anchor = "middle"
    elif align.lower() == "left":
        anchor = "start"
    else:
        anchor = "end"

    if techdraw:
        # For TechDraw display each item in the text list is placed
        # in an individual tag.
        # <text ...> text[0] </text>
        # <text ...> text[1] </text>
        svg = _get_text_techdraw(text, tcolor, fontsize, anchor,
                                 align, fontname, angle, base,
                                 linespacing)
    else:
        # If the SVG is not for TechDraw, and there is a single item
        # in the text list, place it in a single tag.
        # <text ...> text </text>
        #
        # For multiple elements, place each element inside a <tspan> tag.
        # <text ...>
        #   <tspan>text[0]</tspan>
        #   <tspan>text[1]</tspan>
        # </text>
        svg = _get_text_header(tcolor, fontsize, anchor, align,
                               fontname, angle, base, flip)

        if len(text) == 1:
            _t = text[0].replace("&", "&amp;").replace("<", "&lt;")
            svg += _t.replace(">", "&gt;")
        else:
            for i in range(len(text)):
                if i == 0:
                    svg += '<tspan>'
                else:
                    svg += '<tspan x="0" dy="{}">'.format(linespacing)
                _t = text[i].replace("&", "&amp;").replace("<", "&lt;")
                svg += _t.replace(">", "&gt;")
                svg += '</tspan>\n'
        svg += '</text>\n'
    return svg


def getText(plane, techdraw,
            tcolor, fontsize, fontname,
            angle, base, text,
            linespacing=0.5, align="center", flip=True):
    """Get the SVG representation of a textual element. DEPRECATED."""
    utils.use_instead("get_text")
    return get_text(plane, techdraw,
                    tcolor, fontsize, fontname,
                    angle, base, text,
                    linespacing, align, flip)

## @}
