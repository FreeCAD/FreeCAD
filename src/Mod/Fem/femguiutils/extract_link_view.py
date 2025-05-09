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

translate = FreeCAD.Qt.translate

# a model showing available visualizations and possible extractions
# #################################################################

def build_new_visualization_tree_model():
    # model that shows all options to create new visualizations

    model = QtGui.QStandardItemModel()

    visualizations = pv.get_registered_visualizations()
    for vis_name in visualizations:
        vis_icon = FreeCADGui.getIcon(visualizations[vis_name].icon)
        vis_item = QtGui.QStandardItem(vis_icon, translate("FEM", "New {}").format(vis_name))
        vis_item.setFlags(QtGui.Qt.ItemIsEnabled)
        vis_item.setData(visualizations[vis_name])

        for ext in visualizations[vis_name].extractions:
            icon = FreeCADGui.getIcon(ext.icon)
            name = ext.name.removeprefix(vis_name)
            ext_item = QtGui.QStandardItem(icon, translate("FEM", "with {}").format(name))
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
                        ext_item = QtGui.QStandardItem(icon, translate("FEM", "Add {}").format(name))
                        ext_item.setData(ext)
                        vis_item.appendRow(ext_item)

            if ana_item.rowCount():
                model.appendRow(ana_item)

    return model

def build_post_object_item(post_object, extractions, vis_type):

    # definitely build a item and add the extractions
    post_item = QtGui.QStandardItem(post_object.ViewObject.Icon, translate("FEM", "From {}").format(post_object.Label))
    post_item.setFlags(QtGui.Qt.ItemIsEnabled)
    post_item.setData(post_object)

    # add extractor items
    for ext in extractions:
        icon = FreeCADGui.getIcon(ext.icon)
        name = ext.name.removeprefix(vis_type)
        ext_item = QtGui.QStandardItem(icon, translate("FEM", "add {}").format(name))
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

# implementation of GUI and its functionality
# ###########################################

class _TreeChoiceButton(QtGui.QToolButton):

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

class _SettingsPopup(QtGui.QDialog):

    close = QtCore.Signal()

    def __init__(self, setting, parent):
        super().__init__(parent)

        self.setWindowFlags(QtGui.Qt.Popup)
        self.setFocusPolicy(QtGui.Qt.ClickFocus)

        vbox = QtGui.QVBoxLayout()
        vbox.addWidget(setting)

        buttonBox = QtGui.QDialogButtonBox()
        buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Ok)
        buttonBox.accepted.connect(self.hide)
        vbox.addWidget(buttonBox)

        self.setLayout(vbox)

    def showEvent(self, event):
        # required to get keyboard events
        self.setFocus()

    def hideEvent(self, event):
        # emit on hide: this happens for OK button as well as
        # "click away" closing of the popup
        self.close.emit()

    def keyPressEvent(self, event):
        # close on hitting enter
        if event.key() == QtGui.Qt.Key_Enter or event.key() == QtGui.Qt.Key_Return:
            self.hide()


class _SummaryWidget(QtGui.QWidget):

    delete = QtCore.Signal(object, object) # to delete: document object, summary widget

    def __init__(self, st_object, extractor, post_dialog):
        super().__init__()

        self._st_object = st_object
        self._extractor = extractor
        self._post_dialog = post_dialog

        extr_label = extractor.Proxy.get_representive_fieldname(extractor)
        extr_repr = extractor.ViewObject.Proxy.get_preview()

        # build the UI

        self.extrButton = self._button(extr_label)
        self.extrButton.setIcon(extractor.ViewObject.Icon)

        self.viewButton = self._button(extr_repr[1])
        if not extr_repr[0].isNull():
            size = self.viewButton.iconSize()
            size.setWidth(size.width()*2)
            self.viewButton.setIconSize(size)
            self.viewButton.setIcon(extr_repr[0])
        else:
            self.viewButton.setIconSize(QtCore.QSize(0,0))

        if st_object:
            self.stButton = self._button(st_object.Label)
            self.stButton.setIcon(st_object.ViewObject.Icon)

        else:
            # that happens if the source of the extractor was deleted and now
            # that property is set to None
            self.extrButton.hide()
            self.viewButton.hide()

            self.warning = QtGui.QLabel(self)
            self.warning.full_text = translate("FEM", "{}: Data source not available").format(extractor.Label)

        self.rmButton = QtGui.QToolButton(self)
        self.rmButton.setIcon(QtGui.QIcon.fromTheme("delete"))
        self.rmButton.setAutoRaise(True)

        # add the separation line
        self.frame = QtGui.QFrame(self)
        self.frame.setFrameShape(QtGui.QFrame.HLine);

        policy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Fixed)
        self.setSizePolicy(policy)
        self.setMinimumSize(self.extrButton.sizeHint()+self.frame.sizeHint()*3)

        # connect actions. We add functions to widget, as well as the data we need,
        # and use those as callback. This way every widget knows which objects to use
        if st_object:
            self.stButton.clicked.connect(self.showVisualization)
            self.extrButton.clicked.connect(self.editApp)
            self.viewButton.clicked.connect(self.editView)

        self.rmButton.clicked.connect(self.deleteTriggered)

        # make sure initial drawing happened
        self._redraw()


    def _button(self, text):
        btn = QtGui.QPushButton(self)
        btn.full_text = text

        btn.setMinimumWidth(0)
        btn.setFlat(True)
        btn.setText(text)
        btn.setStyleSheet("text-align:left;padding:6px");
        btn.setToolTip(text)

        return btn

    def _redraw(self):

        btn_spacing = 3
        btn_height = self.extrButton.sizeHint().height()

        # total size notes:
        #   - 5 spacing = 2x between buttons + 3 spacings to remove button
        #   - remove btn_height as this is used as remove button square size
        btn_total_size = self.size().width() - btn_height - 5*btn_spacing
        btn_margin = (self.rmButton.size() - self.rmButton.iconSize()).width()
        fm = self.fontMetrics()

        if self._st_object:

            min_text_width = fm.size(QtGui.Qt.TextSingleLine, "...").width()*2

            pos = 0
            btns = [self.stButton, self.extrButton, self.viewButton]
            btn_rel_size = [0.4, 0.4, 0.2]
            btn_elide_mode = [QtGui.Qt.ElideMiddle, QtGui.Qt.ElideMiddle, QtGui.Qt.ElideRight]
            for i, btn in enumerate(btns):

                btn_size = btn_total_size*btn_rel_size[i]
                txt_size = btn_size - btn.iconSize().width() - btn_margin

                # we elide only if there is enough space for a meaningful text
                if txt_size >= min_text_width:

                    text = fm.elidedText(btn.full_text, btn_elide_mode[i], txt_size)
                    btn.setText(text)
                    btn.setStyleSheet("text-align:left;padding:6px");
                else:
                    btn.setText("")
                    btn.setStyleSheet("text-align:center;");

                rect = QtCore.QRect(pos, 0, btn_size, btn_height)
                btn.setGeometry(rect)
                pos += btn_size + btn_spacing

        else:
            warning_txt = fm.elidedText(self.warning.full_text, QtGui.Qt.ElideRight, btn_total_size)
            self.warning.setText(warning_txt)
            rect = QtCore.QRect(0,0, btn_total_size, btn_height)
            self.warning.setGeometry(rect)


        rmsize = btn_height
        pos = self.size().width() - rmsize
        self.rmButton.setGeometry(pos, 0, rmsize, rmsize)

        frame_hint = self.frame.sizeHint()
        rect = QtCore.QRect(0, btn_height+frame_hint.height(), self.size().width(), frame_hint.height())
        self.frame.setGeometry(rect)

    def resizeEvent(self, event):

        # calculate the allowed text length
        self._redraw()
        super().resizeEvent(event)

    @QtCore.Slot()
    def showVisualization(self):
        if vis.is_visualization_object(self._st_object):
            # show the visualization
            self._st_object.ViewObject.Proxy.show_visualization()
        else:
            # for now just select the thing
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.Selection.addSelection(self._st_object)

    def _position_dialog(self, dialog):

        # the scroll area does mess the mapping to global up, somehow
        # the transformation from the widget ot the scroll area gives
        # very weird values. Hence we build the coords of the widget
        # ourself

        summary = dialog.parent() # == self
        base_widget = summary.parent()
        viewport = summary.parent()
        scroll = viewport.parent()

        top_left = summary.geometry().topLeft() + base_widget.geometry().topLeft() + viewport.geometry().topLeft()
        delta = (summary.width() - dialog.sizeHint().width())/2
        local_point = QtCore.QPoint(top_left.x()+delta, top_left.y()+summary.height())
        global_point = scroll.mapToGlobal(local_point)

        dialog.setGeometry(QtCore.QRect(global_point, dialog.sizeHint()))

    @QtCore.Slot()
    def editApp(self):
        if not hasattr(self, "appDialog"):
            widget = self._extractor.ViewObject.Proxy.get_app_edit_widget(self._post_dialog)
            self.appDialog = _SettingsPopup(widget, self)
            self.appDialog.close.connect(self.appAccept)

        if not self.appDialog.isVisible():
            # position correctly and show
            self._position_dialog(self.appDialog)
            self.appDialog.show()
            #self.appDialog.raise_()

    @QtCore.Slot()
    def editView(self):

        if not hasattr(self, "viewDialog"):
            widget = self._extractor.ViewObject.Proxy.get_view_edit_widget(self._post_dialog)
            self.viewDialog = _SettingsPopup(widget, self)
            self.viewDialog.close.connect(self.viewAccept)

        if not self.viewDialog.isVisible():
            # position correctly and show
            self._position_dialog(self.viewDialog)
            self.viewDialog.show()
            #self.viewDialog.raise_()

    @QtCore.Slot()
    def deleteTriggered(self):
        self.delete.emit(self._extractor, self)

    @QtCore.Slot()
    def viewAccept(self):

        # update the preview
        extr_repr = self._extractor.ViewObject.Proxy.get_preview()
        self.viewButton.setIcon(extr_repr[0])
        self.viewButton.full_text = extr_repr[1]
        self.viewButton.setToolTip(extr_repr[1])
        self._redraw()

    @QtCore.Slot()
    def appAccept(self):

        # update the preview
        extr_label = self._extractor.Proxy.get_representive_fieldname(self._extractor)
        self.extrButton.full_text = extr_label
        self.extrButton.setToolTip(extr_label)
        self._redraw()



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
        label = QtGui.QLabel(translate("FEM", "Data used in:"))
        if not self._is_source:
            label.setText(translate("FEM", "Data used from:"))

        label.setAlignment(QtGui.Qt.AlignBottom)
        hbox.addWidget(label)
        hbox.addStretch()

        if self._is_source:

            self._add = _TreeChoiceButton(build_add_to_visualization_tree_model())
            self._add.setText(translate("FEM", "Add data to"))
            self._add.selection.connect(self.addExtractionToVisualization)
            hbox.addWidget(self._add)

            self._create = _TreeChoiceButton(build_new_visualization_tree_model())
            self._create.setText(translate("FEM", "New"))
            self._create.selection.connect(self.newVisualization)
            hbox.addWidget(self._create)

        else:
            vis_type = vis.get_visualization_type(self._object)
            self._add = _TreeChoiceButton(build_add_from_data_tree_model(vis_type))
            self._add.setText(translate("FEM", "Add data from"))
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

        if self._is_source:
            st_object = extractor.getParentGroup()
        else:
            st_object = extractor.Source

        widget = _SummaryWidget(st_object, extractor, self._post_dialog)
        widget.delete.connect(self._delete_extraction)

        return widget

    def _delete_extraction(self, extractor, widget):
        # remove the document object
        doc = extractor.Document
        doc.removeObject(extractor.Name)
        doc.recompute()

        # remove the widget
        self._widgets.remove(widget)
        widget.deleteLater()

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
        # default values: color
        color_prop = FreeCADGui.ActiveDocument.ActiveObject.Proxy.get_default_color_property()
        if color_prop:
            FreeCADGui.doCommand(
                f"extraction.ViewObject.{color_prop} = visualization.ViewObject.Proxy.get_next_default_color()"
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

        # default values: color
        color_prop = FreeCADGui.ActiveDocument.ActiveObject.Proxy.get_default_color_property()
        if color_prop:
            FreeCADGui.doCommand(
                f"extraction.ViewObject.{color_prop} = (Gui.ActiveDocument.{vis_obj.Name}.Proxy.get_next_default_color())"
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

        # default values for color
        color_prop = FreeCADGui.ActiveDocument.ActiveObject.Proxy.get_default_color_property()
        if color_prop:
            FreeCADGui.doCommand(
                f"extraction.ViewObject.{color_prop} = Gui.ActiveDocument.{self._object.Name}.Proxy.get_next_default_color()"
            )

        FreeCADGui.doCommand(
            f"App.ActiveDocument.{self._object.Name}.addObject(extraction)"
        )

        self._post_dialog._recompute()
        self.repopulate()

