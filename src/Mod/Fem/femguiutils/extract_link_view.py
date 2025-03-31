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

__title__ = "FreeCAD FEM postprocessing view for summarizing extractor links"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package data_extraction
#  \ingroup FEM
#  \brief A widget that shows summaries of all available links to extractors

from PySide import QtGui, QtCore

import femobjects.base_fempostextractors as extr
import femobjects.base_fempostvisualizations as vis

import FreeCAD
import FreeCADGui

from . import post_visualization as pv

# a model showing available visualizations and possible extractions
# #################################################################

def build_new_visualization_tree_model():
    # model that shows all options to create new visualizations

    model = QtGui.QStandardItemModel()

    visualizations = pv.get_registered_visualizations()
    for vis_name in visualizations:
        vis_icon = FreeCADGui.getIcon(visualizations[vis_name].icon)
        vis_item = QtGui.QStandardItem(vis_icon, f"New {vis_name}")
        vis_item.setFlags(QtGui.Qt.ItemIsEnabled)
        vis_item.setData(visualizations[vis_name])

        for ext in visualizations[vis_name].extractions:
            icon = FreeCADGui.getIcon(ext.icon)
            name = ext.name.removeprefix(vis_name)
            ext_item = QtGui.QStandardItem(icon, f"with {name}")
            ext_item.setData(ext)
            vis_item.appendRow(ext_item)
        model.appendRow(vis_item)

    return model

def build_add_to_visualization_tree_model():
    # model that shows all possible visualization objects to add data to

    visualizations = pv.get_registered_visualizations()
    model = QtGui.QStandardItemModel()

    for obj in FreeCAD.ActiveDocument.Objects:
        if obj.isDerivedFrom("Fem::FemAnalysis"):
            ana_item = QtGui.QStandardItem(obj.ViewObject.Icon, obj.Label)
            ana_item.setFlags(QtGui.Qt.ItemIsEnabled)

            # check all children it it is a visualization
            for child in obj.Group:
                if vis.is_visualization_object(child):

                    vis_item = QtGui.QStandardItem(child.ViewObject.Icon, child.Label)
                    vis_type = vis.get_visualization_type(child)
                    vis_item.setFlags(QtGui.Qt.ItemIsEnabled)
                    vis_item.setData(child)
                    ana_item.appendRow(vis_item)

                    # add extractor items
                    for ext in visualizations[vis_type].extractions:
                        icon = FreeCADGui.getIcon(ext.icon)
                        name = ext.name.removeprefix(vis_type)
                        ext_item = QtGui.QStandardItem(icon, f"Add {name}")
                        ext_item.setData(ext)
                        vis_item.appendRow(ext_item)

            if ana_item.rowCount():
                model.appendRow(ana_item)

    return model

def build_post_object_item(post_object, extractions, vis_type):

    # definitely build a item and add the extractions
    post_item = QtGui.QStandardItem(post_object.ViewObject.Icon, f"From {post_object.Label}")
    post_item.setFlags(QtGui.Qt.ItemIsEnabled)
    post_item.setData(post_object)

    # add extractor items
    for ext in extractions:
        icon = FreeCADGui.getIcon(ext.icon)
        name = ext.name.removeprefix(vis_type)
        ext_item = QtGui.QStandardItem(icon, f"add {name}")
        ext_item.setData(ext)
        post_item.appendRow(ext_item)

    # if we are a post group, we need to add the children
    if post_object.hasExtension("Fem::FemPostGroupExtension"):

        for child in post_object.Group:
            if child.isDerivedFrom("Fem::FemPostObject"):
                item = build_post_object_item(child, extractions, vis_type)
                post_item.appendRow(item)

    return post_item


def build_add_from_data_tree_model(vis_type):
    # model that shows all Post data objects from which data can be extracted
    extractions = pv.get_registered_visualizations()[vis_type].extractions

    model = QtGui.QStandardItemModel()
    for obj in FreeCAD.ActiveDocument.Objects:
        if obj.isDerivedFrom("Fem::FemAnalysis"):
            ana_item = QtGui.QStandardItem(obj.ViewObject.Icon, obj.Label)
            ana_item.setFlags(QtGui.Qt.ItemIsEnabled)

            # check all children if it is a post object
            for child in obj.Group:
                if child.isDerivedFrom("Fem::FemPostObject"):
                    item = build_post_object_item(child, extractions, vis_type)
                    ana_item.appendRow(item)

            if ana_item.rowCount():
                model.appendRow(ana_item)

    return model

class TreeChoiceButton(QtGui.QToolButton):

    selection = QtCore.Signal(object,object)

    def __init__(self, model):
        super().__init__()

        self.model = model
        self.setEnabled(bool(model.rowCount()))

        self.__skip_next_hide = False

        self.tree_view = QtGui.QTreeView(self)
        self.tree_view.setModel(model)

        self.tree_view.setFrameShape(QtGui.QFrame.NoFrame)
        self.tree_view.setHeaderHidden(True)
        self.tree_view.setEditTriggers(QtGui.QTreeView.EditTriggers.NoEditTriggers)
        self.tree_view.setSelectionBehavior(QtGui.QTreeView.SelectionBehavior.SelectRows)
        self.tree_view.expandAll()
        self.tree_view.activated.connect(self.selectIndex)

        # set a complex menu
        self.popup = QtGui.QWidgetAction(self)
        self.popup.setDefaultWidget(self.tree_view)
        self.setPopupMode(QtGui.QToolButton.InstantPopup)
        self.addAction(self.popup);

    QtCore.Slot(QtCore.QModelIndex)
    def selectIndex(self, index):
        item = self.model.itemFromIndex(index)

        if item and not item.hasChildren():
            extraction = item.data()
            parent = item.parent().data()
            self.selection.emit(parent, extraction)
            self.popup.trigger()

    def setModel(self, model):
        self.model = model
        self.tree_view.setModel(model)
        self.tree_view.expandAll()

        # check if we should be disabled
        self.setEnabled(bool(model.rowCount()))


# implementationof GUI and its functionality
# ##########################################

class _ShowVisualization:
    def __init__(self, st_object):
        self._st_object = st_object

    def __call__(self):
        if vis.is_visualization_object(self._st_object):
            # show the visualization
            self._st_object.ViewObject.Proxy.show_visualization()
        else:
            # for now just select the thing
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.Selection.addSelection(self._st_object)

class _ShowEditDialog:
    def __init__(self, extractor, post_dialog, widget):
        self._extractor = extractor
        self._post_dialog = post_dialog
        self._widget = widget

        widgets = self._extractor.ViewObject.Proxy.get_edit_widgets(self._post_dialog)
        vbox = QtGui.QVBoxLayout()

        buttonBox = QtGui.QDialogButtonBox()
        buttonBox.setCenterButtons(True)
        buttonBox.setStandardButtons(self._post_dialog.getStandardButtons())
        vbox.addWidget(buttonBox)

        started = False
        for widget in widgets:

            if started:
                # add a seperator line
                frame = QtGui.QFrame()
                frame.setFrameShape(QtGui.QFrame.HLine);
                vbox.addWidget(frame);
            else:
                started = True

            vbox.addWidget(widget)

        vbox.addStretch()

        self.dialog = QtGui.QDialog(self._widget)
        buttonBox.accepted.connect(self.accept)
        buttonBox.rejected.connect(self.dialog.close)
        buttonBox.button(QtGui.QDialogButtonBox.Apply).clicked.connect(self.apply)
        self.dialog.setLayout(vbox)


    def accept(self):
        # recompute and close
        self._extractor.Document.recompute()
        self.dialog.close()

    def apply(self):
        self._extractor.Document.recompute()

    def __call__(self):
        # create the widgets, add it to dialog
        self.dialog.show()

class _DeleteExtractor:
    def __init__(self, extractor, widget):
        self._extractor = extractor
        self._widget = widget

    def __call__(self):
        # remove the document object
        doc = self._extractor.Document
        doc.removeObject(self._extractor.Name)
        doc.recompute()

        # remove the widget
        self._widget.deleteLater()

class ExtractLinkView(QtGui.QWidget):

    def __init__(self, obj, is_source, post_dialog):
        # initializes the view.
        # obj: The object for which the links should be shown / summarized
        # is_source: Bool, if the object is the data source (e.g. postobject), or the target (e.g. plots)

        super().__init__()

        self._object = obj
        self._is_source = is_source
        self._post_dialog = post_dialog
        self._widgets = []

        # build the layout:
        self._scroll_view = QtGui.QScrollArea(self)
        self._scroll_view.setHorizontalScrollBarPolicy(QtGui.Qt.ScrollBarAlwaysOff)
        self._scroll_view.setWidgetResizable(True)

        hbox = QtGui.QHBoxLayout()
        label = QtGui.QLabel("Data used in:")
        if not self._is_source:
            label.setText("Data used from:")

        label.setAlignment(QtGui.Qt.AlignBottom)
        hbox.addWidget(label)
        hbox.addStretch()

        if self._is_source:

            self._add = TreeChoiceButton(build_add_to_visualization_tree_model())
            self._add.setText("Add data to")
            self._add.selection.connect(self.addExtractionToVisualization)
            hbox.addWidget(self._add)

            self._create = TreeChoiceButton(build_new_visualization_tree_model())
            self._create.setText("New")
            self._create.selection.connect(self.newVisualization)
            hbox.addWidget(self._create)

        else:
            vis_type = vis.get_visualization_type(self._object)
            self._add = TreeChoiceButton(build_add_from_data_tree_model(vis_type))
            self._add.setText("Add data from")
            self._add.selection.connect(self.addExtractionToPostObject)
            hbox.addWidget(self._add)

        vbox = QtGui.QVBoxLayout()
        vbox.setContentsMargins(0,0,0,0)
        vbox.addItem(hbox)
        vbox.addWidget(self._scroll_view)

        self.setLayout(vbox)



        # add the content
        self.repopulate()

    def _build_summary_widget(self, extractor):

        widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/PostExtractionSummaryWidget.ui"
        )

        # add the separation line
        frame = QtGui.QFrame()
        frame.setFrameShape(QtGui.QFrame.HLine);
        widget.layout().addWidget(frame);

        if self._is_source:
            st_object = extractor.getParentGroup()
        else:
            st_object = extractor.Source

        widget.RemoveButton.setIcon(QtGui.QIcon.fromTheme("delete"))

        widget.STButton.setIcon(st_object.ViewObject.Icon)
        widget.STButton.setText(st_object.Label)

        widget.ExtractButton.setIcon(extractor.ViewObject.Icon)

        extr_label = extr.get_extraction_dimension(extractor)
        extr_label += " " + extr.get_extraction_type(extractor)
        widget.ExtractButton.setText(extr_label)

        # connect actions. We add functions to widget, as well as the data we need,
        # and use those as callback. This way every widget knows which objects to use
        widget.STButton.clicked.connect(_ShowVisualization(st_object))
        widget.ExtractButton.clicked.connect(_ShowEditDialog(extractor, self._post_dialog, widget))
        widget.RemoveButton.clicked.connect(_DeleteExtractor(extractor, widget))

        return widget

    def repopulate(self):
        # collect all links that are available and shows them

        # clear the view
        for widget in self._widgets:
            widget.hide()
            widget.deleteLater()

        self._widgets = []

        # rebuild the widgets

        if self._is_source:
            candidates = self._object.InList
        else:
            candidates = self._object.OutList

        # get all widgets from the candidates
        extractors = []
        for candidate in candidates:
            if extr.is_extractor_object(candidate):
                summary = self._build_summary_widget(candidate)
                self._widgets.append(summary)

        # fill the scroll area
        vbox = QtGui.QVBoxLayout()
        for widget in self._widgets:
            vbox.addWidget(widget)

        vbox.addStretch()
        widget = QtGui.QWidget()
        widget.setLayout(vbox)

        self._scroll_view.setWidget(widget)

        # also reset the add button model
        if self._is_source:
            self._add.setModel(build_add_to_visualization_tree_model())

    def _find_parent_analysis(self, obj):
        # iterate upwards, till we find a analysis
        for parent in obj.InList:
            if parent.isDerivedFrom("Fem::FemAnalysis"):
                return parent

            analysis = self._find_parent_analysis(parent)
            if analysis:
                return analysis

        return None

    QtCore.Slot(object, object) # visualization data, extraction data
    def newVisualization(self, vis_data, ext_data):

        doc = self._object.Document

        FreeCADGui.addModule(vis_data.module)
        FreeCADGui.addModule(ext_data.module)
        FreeCADGui.addModule("FemGui")

        # create visualization
        FreeCADGui.doCommand(
            f"visualization = {vis_data.module}.{vis_data.factory}(FreeCAD.ActiveDocument)"
        )
        analysis = self._find_parent_analysis(self._object)
        if analysis:
            FreeCADGui.doCommand(
                f"FreeCAD.ActiveDocument.{analysis.Name}.addObject(visualization)"
            )

        # create extraction and add it
        FreeCADGui.doCommand(
            f"extraction = {ext_data.module}.{ext_data.factory}(FreeCAD.ActiveDocument)"
        )
        FreeCADGui.doCommand(
            f"extraction.Source = FreeCAD.ActiveDocument.{self._object.Name}"
        )
        FreeCADGui.doCommand(
            f"visualization.addObject(extraction)"
        )

        self._post_dialog._recompute()
        self.repopulate()

    QtCore.Slot(object, object) # visualization object, extraction data
    def addExtractionToVisualization(self, vis_obj, ext_data):

        FreeCADGui.addModule(ext_data.module)
        FreeCADGui.addModule("FemGui")

        # create extraction and add it
        FreeCADGui.doCommand(
            f"extraction = {ext_data.module}.{ext_data.factory}(FreeCAD.ActiveDocument)"
        )
        FreeCADGui.doCommand(
            f"extraction.Source = FreeCAD.ActiveDocument.{self._object.Name}"
        )
        FreeCADGui.doCommand(
            f"App.ActiveDocument.{vis_obj.Name}.addObject(extraction)"
        )

        self._post_dialog._recompute()
        self.repopulate()

    QtCore.Slot(object, object) # post object, extraction data
    def addExtractionToPostObject(self, post_obj, ext_data):

        FreeCADGui.addModule(ext_data.module)
        FreeCADGui.addModule("FemGui")

        # create extraction and add it
        FreeCADGui.doCommand(
            f"extraction = {ext_data.module}.{ext_data.factory}(FreeCAD.ActiveDocument)"
        )
        FreeCADGui.doCommand(
            f"extraction.Source = FreeCAD.ActiveDocument.{post_obj.Name}"
        )
        FreeCADGui.doCommand(
            f"App.ActiveDocument.{self._object.Name}.addObject(extraction)"
        )

        self._post_dialog._recompute()
        self.repopulate()

