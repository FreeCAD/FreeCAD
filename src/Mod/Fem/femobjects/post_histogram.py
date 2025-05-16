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

__title__ = "FreeCAD post histogram"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package post_histogram
#  \ingroup FEM
#  \brief Post processing plot displaying histograms

# check vtk version to potentially find missmatchs
from femguiutils.vtk_module_handling import vtk_module_handling

vtk_module_handling()

from . import base_fempostextractors
from . import base_fempostvisualizations
from . import post_extract1D

from femguiutils import post_visualization

# register visualization and extractors
post_visualization.register_visualization(
    "Histogram", ":/icons/FEM_PostHistogram.svg", "ObjectsFem", "makePostHistogram"
)

post_visualization.register_extractor(
    "Histogram",
    "HistogramFieldData",
    ":/icons/FEM_PostField.svg",
    "1D",
    "Field",
    "ObjectsFem",
    "makePostHistogramFieldData",
)


post_visualization.register_extractor(
    "Histogram",
    "HistogramIndexOverFrames",
    ":/icons/FEM_PostIndex.svg",
    "1D",
    "Index",
    "ObjectsFem",
    "makePostHistogramIndexOverFrames",
)

# Implementation
# ##############


def is_histogram_extractor(obj):

    if not base_fempostextractors.is_extractor_object(obj):
        return False

    if not hasattr(obj.Proxy, "VisualizationType"):
        return False

    return obj.Proxy.VisualizationType == "Histogram"


class PostHistogramFieldData(post_extract1D.PostFieldData1D):
    """
    A 1D Field extraction for histograms.
    """

    VisualizationType = "Histogram"


class PostHistogramIndexOverFrames(post_extract1D.PostIndexOverFrames1D):
    """
    A 1D index extraction for histogram.
    """

    VisualizationType = "Histogram"


class PostHistogram(base_fempostvisualizations.PostVisualization):
    """
    A post processing plot for showing extracted data as histograms
    """

    VisualizationType = "Histogram"
