# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD post line plot"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package post_lineplot
#  \ingroup FEM
#  \brief Post processing plot displaying lines

# check vtk version to potentially find missmatchs
from femguiutils.vtk_module_handling import vtk_module_handling

vtk_module_handling()

from . import base_fempostextractors
from . import base_fempostvisualizations
from . import post_extract2D

from femguiutils import post_visualization

# register visualization and extractors
post_visualization.register_visualization(
    "Lineplot", ":/icons/FEM_PostLineplot.svg", "ObjectsFem", "makePostLineplot"
)

post_visualization.register_extractor(
    "Lineplot",
    "LineplotFieldData",
    ":/icons/FEM_PostField.svg",
    "2D",
    "Field",
    "ObjectsFem",
    "makePostLineplotFieldData",
)

post_visualization.register_extractor(
    "Lineplot",
    "LineplotIndexOverFrames",
    ":/icons/FEM_PostIndex.svg",
    "2D",
    "Index",
    "ObjectsFem",
    "makePostLineplotIndexOverFrames",
)


# Implementation
# ##############


def is_lineplot_extractor(obj):

    if not base_fempostextractors.is_extractor_object(obj):
        return False

    if not hasattr(obj.Proxy, "VisualizationType"):
        return False

    return obj.Proxy.VisualizationType == "Lineplot"


class PostLineplotFieldData(post_extract2D.PostFieldData2D):
    """
    A 2D Field extraction for lineplot.
    """

    VisualizationType = "Lineplot"


class PostLineplotIndexOverFrames(post_extract2D.PostIndexOverFrames2D):
    """
    A 2D index extraction for lineplot.
    """

    VisualizationType = "Lineplot"


class PostLineplot(base_fempostvisualizations.PostVisualization):
    """
    A post processing plot for showing extracted data as line plots
    """

    VisualizationType = "Lineplot"
