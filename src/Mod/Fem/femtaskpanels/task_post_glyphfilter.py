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

__title__ = "FreeCAD FEM glyph filter task panel for the document object"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package task_post_glyphfilter
#  \ingroup FEM
#  \brief task panel for post glyph filter object

from PySide import QtCore, QtGui

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets
from . import base_fempostpanel


class _TaskPanel(base_fempostpanel._BasePostTaskPanel):
    """
    The TaskPanel for editing properties of glyph filter
    """

    def __init__(self, vobj):
        super().__init__(vobj.Object)

        # glyph parameter widget
        self.widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/TaskPostGlyph.ui"
        )
        self.widget.setWindowIcon(FreeCADGui.getIcon(":/icons/FEM_PostFilterGlyph.svg"))
        self.__init_widget()

        # form made from param and selection widget
        self.form = [self.widget, vobj.createDisplayTaskWidget()]

    # Setup functions
    # ###############

    def __init_widget(self):

        # set current values to ui
        self._enumPropertyToCombobox(self.obj, "Glyph", self.widget.FormComboBox)
        self._enumPropertyToCombobox(self.obj, "OrientationData", self.widget.OrientationComboBox)
        self._enumPropertyToCombobox(self.obj, "ScaleData", self.widget.ScaleComboBox)
        self._enumPropertyToCombobox(self.obj, "VectorScaleMode", self.widget.VectorModeComboBox)
        self._enumPropertyToCombobox(self.obj, "MaskMode", self.widget.MaskModeComboBox)

        self.widget.ScaleFactorBox.setValue(self.obj.ScaleFactor)
        self.__slide_min = self.obj.ScaleFactor * 0.5
        self.__slide_max = self.obj.ScaleFactor * 1.5
        self.widget.ScaleSlider.setValue(50)
        self.widget.StrideBox.setValue(self.obj.Stride)
        self.widget.MaxBox.setValue(self.obj.MaxNumber)
        self.__update_scaling_ui()
        self.__update_masking_ui()

        # connect all signals
        self.widget.FormComboBox.currentTextChanged.connect(self._form_changed)
        self.widget.OrientationComboBox.currentTextChanged.connect(self._orientation_changed)
        self.widget.ScaleComboBox.currentTextChanged.connect(self._scale_data_changed)
        self.widget.VectorModeComboBox.currentTextChanged.connect(self._scale_vector_mode_changed)
        self.widget.ScaleFactorBox.valueChanged.connect(self._scale_factor_changed)
        self.widget.ScaleSlider.valueChanged.connect(self._scale_slider_changed)
        self.widget.MaskModeComboBox.currentTextChanged.connect(self._mask_mode_changed)
        self.widget.StrideBox.valueChanged.connect(self._stride_changed)
        self.widget.MaxBox.valueChanged.connect(self._max_number_changed)

    def __update_scaling_ui(self):
        enabled = self.widget.ScaleComboBox.currentIndex() != 0
        self.widget.VectorModeComboBox.setEnabled(enabled)
        self.widget.ScaleFactorBox.setEnabled(enabled)
        self.widget.ScaleSlider.setEnabled(enabled)

    def __update_masking_ui(self):
        enabled = self.widget.MaskModeComboBox.currentIndex() != 0
        self.widget.StrideBox.setEnabled(enabled)
        self.widget.MaxBox.setEnabled(enabled)

    # callbacks and logic
    # ###################

    def _form_changed(self, value):
        self.obj.Glyph = value
        self._recompute()

    def _orientation_changed(self, value):
        self.obj.OrientationData = value
        self._recompute()

    def _scale_data_changed(self, value):
        self.obj.ScaleData = value
        self._enumPropertyToCombobox(self.obj, "VectorScaleMode", self.widget.VectorModeComboBox)
        self.__update_scaling_ui()
        self._recompute()

    def _scale_vector_mode_changed(self, value):
        self.obj.VectorScaleMode = value
        self._recompute()

    def _scale_factor_changed(self, value):

        # set slider
        self.__slide_min = value * 0.5
        self.__slide_max = value * 1.5
        slider_value = (value - self.__slide_min) / (self.__slide_max - self.__slide_min) * 100.0
        self.widget.ScaleSlider.blockSignals(True)
        self.widget.ScaleSlider.setValue(slider_value)
        self.widget.ScaleSlider.blockSignals(False)

        self.obj.ScaleFactor = value
        self._recompute()

    def _scale_slider_changed(self, value):

        # calculate value
        #                                 ( max - min )
        # factor = min + ( slider_value x ------------- )
        #                                     100
        #
        f = self.__slide_min + (value * (self.__slide_max - self.__slide_min) / 100)

        # sync factor spin box
        self.widget.ScaleFactorBox.blockSignals(True)
        self.widget.ScaleFactorBox.setValue(f)
        self.widget.ScaleFactorBox.blockSignals(False)

        # set value
        self.obj.ScaleFactor = f
        self._recompute()

    def _mask_mode_changed(self, value):
        self.obj.MaskMode = value
        self.__update_masking_ui()
        self._recompute()

    def _stride_changed(self, value):
        self.obj.Stride = value
        self._recompute()

    def _max_number_changed(self, value):
        self.obj.MaxNumber = value
        self._recompute()
