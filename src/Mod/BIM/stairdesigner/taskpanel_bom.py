# SPDX-License-Identifier: LGPL-2.1-or-later

"""Bill-of-materials task-panel page: the timber stock to order.

Two sub-tabs: the dimensions of each member, the smallest rectangle of
stock it is cut from; and the stock layout, the members laid out on the
planks or panels of their materials, counting the stock to order,
previewing each plank or panel and exporting it to DXF.
"""

import FreeCAD
from PySide import QtCore, QtGui

from .stair_bom import (
    MEMBER_KINDS,
    Material,
    default_materials,
    flat_pieces,
    kind_label,
    set_stair_materials,
    stair_layout,
    stair_materials,
    stair_stock,
    stock_csv,
    stock_totals,
)
from .stair_dxf import stock_dxf

translate = FreeCAD.Qt.translate

_PIECE_ROLE = QtCore.Qt.UserRole
# Identity of a materials table row: kinds follow their row when rows
# above it are deleted or it is renamed.
_ROW_ROLE = QtCore.Qt.UserRole + 1


def _kinds_label(kind):
    return {
        "Stringer": translate("BIM", "Stringers"),
        "Tread": translate("BIM", "Treads"),
        "Riser": translate("BIM", "Risers"),
        "Wedge": translate("BIM", "Wedges"),
    }[kind]


def _number(value):
    return f"{value:.0f}"


class _MaterialTableWidget(QtGui.QTableWidget):
    """Materials table that deletes the current row with the Delete key."""

    def __init__(self, delete_callback, parent=None):
        super().__init__(0, 4, parent)
        self._delete_callback = delete_callback

    def keyPressEvent(self, event):
        if event.key() == QtCore.Qt.Key_Delete and self.state() != self.EditingState:
            self._delete_callback()
            event.accept()
            return
        super().keyPressEvent(event)


def _stock_size(found):
    """Return the stock size of a material layout: thickness, length, width."""

    sizes = [found.material.length, found.material.width]
    if found.thickness > 0.0:
        sizes.insert(0, found.thickness)
    return " × ".join(_number(size) for size in sizes)


def _stock_name(found):
    """Return the name of a material layout's stock, with its thickness."""

    if found.thickness > 0.0:
        return f"{found.material.name} {_number(found.thickness)}"
    return found.material.name


def _fit_columns(tree):
    """Stretch a tree's first column; the others fit their contents."""

    header = tree.header()
    header.setStretchLastSection(False)
    header.setSectionResizeMode(0, QtGui.QHeaderView.Stretch)
    for column in range(1, tree.columnCount()):
        header.setSectionResizeMode(column, QtGui.QHeaderView.ResizeToContents)


class _StockView(QtGui.QGraphicsView):
    """Preview of one plank or panel with the parts laid on it."""

    def __init__(self):
        super().__init__()
        self.setScene(QtGui.QGraphicsScene(self))
        self.setRenderHint(QtGui.QPainter.Antialiasing)
        self.setMinimumHeight(140)
        # Always fitted: nothing to scroll to.
        self.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        # Y up, as in the drawing.
        self.scale(1.0, -1.0)

    def show_stock(self, stock):
        scene = self.scene()
        scene.clear()
        self._stock_rect = None
        if stock is None:
            scene.setSceneRect(QtCore.QRectF())
            return
        self._stock_rect = QtCore.QRectF(0.0, 0.0, stock.length, stock.width)
        # The scene rect would otherwise only grow, from stock to stock.
        scene.setSceneRect(self._stock_rect.adjusted(-20, -20, 20, 20))
        palette = self.palette()
        text_color = palette.color(QtGui.QPalette.Text)
        scene.addRect(
            QtCore.QRectF(0.0, 0.0, stock.length, stock.width),
            QtGui.QPen(text_color, 0),
            QtGui.QBrush(palette.color(QtGui.QPalette.AlternateBase)),
        )
        part_brush = QtGui.QBrush(QtGui.QColor(196, 150, 100))
        pocket_brush = QtGui.QBrush(QtGui.QColor(140, 95, 55))
        for placed in stock.placed:
            polygon = QtGui.QPolygonF([QtCore.QPointF(x, y) for x, y in placed.outline])
            scene.addPolygon(polygon, QtGui.QPen(text_color, 0), part_brush)
            for _depth, outline in placed.part.pockets:
                pocket = QtGui.QPolygonF([QtCore.QPointF(x, y) for x, y in placed.map(outline)])
                scene.addPolygon(pocket, QtGui.QPen(QtCore.Qt.NoPen), pocket_brush)
            # Cut from the underside: outlined only.
            back_pen = QtGui.QPen(QtGui.QColor(140, 95, 55), 0, QtCore.Qt.DashLine)
            for _depth, outline in placed.part.back_pockets:
                pocket = QtGui.QPolygonF([QtCore.QPointF(x, y) for x, y in placed.map(outline)])
                scene.addPolygon(pocket, back_pen)
            label = scene.addSimpleText(placed.part.label)
            label.setFlag(QtGui.QGraphicsItem.ItemIgnoresTransformations)
            text = label.boundingRect()
            label.setTransform(
                QtGui.QTransform.fromTranslate(-text.width() / 2.0, -text.height() / 2.0)
            )
            label.setPos(polygon.boundingRect().center())
        self.fit()

    def fit(self):
        rect = getattr(self, "_stock_rect", None)
        if rect is not None and not rect.isEmpty():
            self.fitInView(rect.adjusted(-20, -20, 20, 20), QtCore.Qt.KeepAspectRatio)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self.fit()


class BomPanelMixin:
    """Task-panel methods grouped by responsibility."""

    def _make_bom_panel(self):
        widget = QtGui.QWidget()
        layout = QtGui.QVBoxLayout(widget)
        layout.setContentsMargins(0, 0, 0, 0)
        self.bom_tabs = QtGui.QTabWidget()
        layout.addWidget(self.bom_tabs)

        # Component dimensions
        components = QtGui.QWidget()
        components_layout = QtGui.QVBoxLayout(components)
        self.bom_tabs.addTab(components, translate("BIM", "Component dimensions"))

        # Stock layout
        self.bom_layout_page = QtGui.QWidget()
        box = QtGui.QVBoxLayout(self.bom_layout_page)
        self.bom_tabs.addTab(self.bom_layout_page, translate("BIM", "Stock layout"))
        self.bom_tabs.setTabToolTip(
            1,
            translate(
                "BIM",
                "Lays the members out on planks or panels of their materials and "
                "counts how many to order",
            ),
        )
        self.bom_materials = _MaterialTableWidget(self._remove_bom_material)
        self.bom_materials.setHorizontalHeaderLabels(
            [
                translate("BIM", "Material"),
                translate("BIM", "Length"),
                translate("BIM", "Width"),
                translate("BIM", "Spacing"),
            ]
        )
        self.bom_materials.horizontalHeaderItem(3).setToolTip(
            translate(
                "BIM",
                "Spacing left between parts on the stock: saw kerf or cutter diameter",
            )
        )
        self.bom_materials.setToolTip(
            translate(
                "BIM",
                "Planks or panels the members are cut from, in mm, as thick as "
                "the parts: parts of another thickness get stock of their own. "
                "Treads wider than their planks are glued up from strips laid "
                "from the nosing. Right-click to add or delete a material",
            )
        )
        self.bom_materials.verticalHeader().setVisible(False)
        header = self.bom_materials.horizontalHeader()
        header.setSectionResizeMode(0, QtGui.QHeaderView.Stretch)
        for column in (1, 2, 3):
            header.setSectionResizeMode(column, QtGui.QHeaderView.ResizeToContents)
        self.bom_materials.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.bom_materials.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)
        self.bom_materials.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.bom_materials.customContextMenuRequested.connect(self._show_bom_material_menu)
        # Tall enough for the three default materials.
        self.bom_materials.setFixedHeight(
            header.sizeHint().height()
            + 3 * self.bom_materials.verticalHeader().defaultSectionSize()
            + 2 * self.bom_materials.frameWidth()
        )
        box.addWidget(self.bom_materials)

        # Two kinds per row, to keep the panel short.
        form = QtGui.QGridLayout()
        self.bom_assignment = {}
        for index, kind in enumerate(MEMBER_KINDS):
            combo = QtGui.QComboBox()
            self.bom_assignment[kind] = combo
            row, column = divmod(index, 2)
            form.addWidget(QtGui.QLabel(_kinds_label(kind)), row, 2 * column)
            form.addWidget(combo, row, 2 * column + 1)
            combo.currentIndexChanged.connect(self._bom_materials_changed)
        form.setColumnStretch(1, 1)
        form.setColumnStretch(3, 1)
        box.addLayout(form)

        self.bom_totals = QtGui.QTreeWidget()
        self.bom_totals.setHeaderLabels(
            [translate("BIM", "Stock"), translate("BIM", "Quantity"), translate("BIM", "Used")]
        )
        self.bom_totals.setToolTip(
            translate(
                "BIM",
                "Planks or panels to order per material. Select one to preview "
                "how the parts are laid on it",
            )
        )
        _fit_columns(self.bom_totals)
        box.addWidget(self.bom_totals)
        self.bom_warnings = QtGui.QLabel()
        self.bom_warnings.setWordWrap(True)
        self.bom_warnings.setVisible(False)
        box.addWidget(self.bom_warnings)
        self.bom_preview = _StockView()
        box.addWidget(self.bom_preview)
        layout_buttons = QtGui.QHBoxLayout()
        self.bom_export_dxf = QtGui.QPushButton(translate("BIM", "Export DXF…"))
        self.bom_export_dxf.setToolTip(
            translate(
                "BIM",
                "Saves the selected plank or panel, or all of the selected "
                "material's, with the parts' outlines and housing pockets",
            )
        )
        layout_buttons.addStretch(1)
        layout_buttons.addWidget(self.bom_export_dxf)
        box.addLayout(layout_buttons)

        self.bom_tree = QtGui.QTreeWidget()
        self.bom_tree.setRootIsDecorated(False)
        self.bom_tree.setAlternatingRowColors(True)
        self.bom_tree.setHeaderLabels(
            [
                translate("BIM", "Part"),
                translate("BIM", "Qty"),
                translate("BIM", "Thickness"),
                translate("BIM", "Length"),
                translate("BIM", "Width"),
                translate("BIM", "Items"),
            ]
        )
        self.bom_tree.setToolTip(
            translate(
                "BIM",
                "Smallest rectangle of stock each part is cut from, in mm. Treads "
                "are measured along their nosing, the grain direction",
            )
        )
        components_layout.addWidget(self.bom_tree)
        self.bom_sections = QtGui.QTreeWidget()
        self.bom_sections.setRootIsDecorated(False)
        self.bom_sections.setHeaderLabels(
            [translate("BIM", "Plank section"), translate("BIM", "Total length")]
        )
        self.bom_sections.setToolTip(
            translate(
                "BIM",
                "Plank length to order per section, end to end, without saw kerf "
                "or offcut allowance",
            )
        )
        _fit_columns(self.bom_sections)
        components_layout.addWidget(self.bom_sections)
        component_buttons = QtGui.QHBoxLayout()
        export = QtGui.QPushButton(translate("BIM", "Export CSV…"))
        component_buttons.addStretch(1)
        component_buttons.addWidget(export)
        components_layout.addLayout(component_buttons)

        buttons = QtGui.QHBoxLayout()
        refresh = QtGui.QPushButton(translate("BIM", "Refresh"))
        refresh.setToolTip(translate("BIM", "Measures the members again"))
        buttons.addStretch(1)
        buttons.addWidget(refresh)
        layout.addLayout(buttons)

        self.bom_rows = []
        self.bom_pieces = None
        self.bom_signature = None
        self.bom_layouts = []
        self._bom_filling = False
        self._fill_bom_materials()
        refresh.clicked.connect(self._refresh_bom_pieces)
        export.clicked.connect(self._export_bom)
        self.bom_export_dxf.clicked.connect(self._export_bom_dxf)
        self.bom_materials.itemChanged.connect(self._bom_materials_changed)
        self.bom_tabs.currentChanged.connect(self._bom_tab_shown)
        self.bom_totals.currentItemChanged.connect(self._bom_stock_selected)
        self.section_tabs.currentChanged.connect(self._bom_tab_shown)
        return widget

    # Materials

    def _fill_bom_materials(self):
        self._bom_filling = True
        try:
            materials, assignment = stair_materials(self.stair)
            self.bom_materials.setRowCount(0)
            for material in materials:
                self._append_bom_material_row(material)
            self._fill_bom_assignment(assignment)
        finally:
            self._bom_filling = False

    def _append_bom_material_row(self, material):
        row = self.bom_materials.rowCount()
        self.bom_materials.insertRow(row)
        self._bom_row_ids = getattr(self, "_bom_row_ids", 0) + 1
        name = QtGui.QTableWidgetItem(material.name)
        name.setData(_ROW_ROLE, self._bom_row_ids)
        self.bom_materials.setItem(row, 0, name)
        for column, value in enumerate(
            (material.length, material.width, material.spacing), start=1
        ):
            # Spacing may be fractional: a 3.2 mm saw kerf.
            item = QtGui.QTableWidgetItem(f"{value:g}" if column == 3 else _number(value))
            item.setTextAlignment(QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)
            self.bom_materials.setItem(row, column, item)

    def _table_materials(self):
        materials = []
        for row in range(self.bom_materials.rowCount()):
            name_item = self.bom_materials.item(row, 0)
            name = name_item.text().strip() if name_item else ""
            values = []
            for column in range(1, 4):
                item = self.bom_materials.item(row, column)
                try:
                    values.append(max(0.0, float(item.text().replace(",", "."))))
                except (AttributeError, ValueError):
                    values.append(0.0)
            materials.append(Material(name or f"{row + 1}", *values))
        return materials

    def _bom_row_id(self, row):
        item = self.bom_materials.item(row, 0)
        return item.data(_ROW_ROLE) if item is not None else None

    def _fill_bom_assignment(self, assignment):
        names = [material.name for material in self._table_materials()]
        ids = [self._bom_row_id(row) for row in range(len(names))]
        for kind, combo in self.bom_assignment.items():
            combo.blockSignals(True)
            combo.clear()
            combo.addItem(translate("BIM", "None"), None)
            for name, row_id in zip(names, ids):
                combo.addItem(name, row_id)
            wanted = assignment.get(kind, "")
            combo.setCurrentIndex(names.index(wanted) + 1 if wanted in names else 0)
            combo.blockSignals(False)

    def _show_bom_material_menu(self, position):
        clicked = self.bom_materials.itemAt(position)
        if clicked is not None:
            self.bom_materials.setCurrentItem(clicked)
        menu = QtGui.QMenu(self.bom_materials)
        add_action = menu.addAction(
            QtGui.QIcon(":/icons/Arch_Add.svg"),
            translate("BIM", "Add Material"),
        )
        add_action.triggered.connect(self._add_bom_material)
        if clicked is not None:
            menu.addSeparator()
            delete_action = menu.addAction(
                QtGui.QIcon(":/icons/Arch_Remove.svg"),
                translate("BIM", "Delete Material"),
            )
            delete_action.triggered.connect(self._remove_bom_material)
        menu.exec_(self.bom_materials.viewport().mapToGlobal(position))

    def _add_bom_material(self, *args):
        materials = self._table_materials()
        name = translate("BIM", "Material {}").format(len(materials) + 1)
        self._bom_filling = True
        try:
            self._append_bom_material_row(Material(name, 2500.0, 1250.0))
        finally:
            self._bom_filling = False
        self._bom_materials_changed()

    def _remove_bom_material(self, *args):
        row = self.bom_materials.currentRow()
        if row < 0:
            return
        self._bom_filling = True
        try:
            self.bom_materials.removeRow(row)
        finally:
            self._bom_filling = False
        self._bom_materials_changed()

    def _bom_materials_changed(self, *args):
        if self._loading or self._bom_filling:
            return
        materials = self._table_materials()
        # Keep each kind on the row it chose, renamed or moved up by a
        # deletion; a deleted row leaves its kinds without material.
        names = {self._bom_row_id(row): material.name for row, material in enumerate(materials)}
        assignment = {
            kind: names.get(combo.currentData(), "") for kind, combo in self.bom_assignment.items()
        }
        self._fill_bom_assignment(assignment)
        set_stair_materials(self.stair, materials, assignment)
        self._refresh_bom()

    def _default_bom_materials(self):
        """Give a stair without materials the default ones, sized from its parts."""

        if self._loading or self._bom_filling or self.bom_materials.rowCount():
            return
        if self.bom_pieces is not None:
            materials, assignment = default_materials(self.stair, self.bom_pieces)
            self._bom_filling = True
            try:
                for material in materials:
                    self._append_bom_material_row(material)
                self._fill_bom_assignment(assignment)
            finally:
                self._bom_filling = False
            set_stair_materials(self.stair, materials, assignment)

    def _stock_layout_shown(self):
        return (
            self.section_tabs.currentWidget() is self.section_widgets.get("bom")
            and self.bom_tabs.currentWidget() is self.bom_layout_page
        )

    # Pieces and layout

    def _bom_signature(self):
        """Return what the list depends on, cheap to compare."""

        return tuple(
            (part.Name, round(part.Shape.Volume, 3), tuple(part.Placement.Base))
            for group in (self.stair.StringersGroup, self.stair.StepsGroup)
            if group
            for part in group.Group
            if not part.Shape.isNull()
        )

    def _bom_tab_shown(self, *args):
        # Measure the members again only when they changed.
        if self.section_tabs.currentWidget() is self.section_widgets.get("bom"):
            if self.bom_pieces is None or self.bom_signature != self._bom_signature():
                self._refresh_bom_pieces()
            elif self._stock_layout_shown() and not self.bom_materials.rowCount():
                self._refresh_bom()

    def _refresh_bom_pieces(self, *args):
        QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
        try:
            self.bom_pieces = flat_pieces(self.stair)
        finally:
            QtGui.QApplication.restoreOverrideCursor()
        self.bom_signature = self._bom_signature()
        self._refresh_bom()

    def _refresh_bom(self, *args):
        if self.bom_pieces is None:
            return
        if self._stock_layout_shown():
            self._default_bom_materials()
        self.bom_rows = stair_stock(self.stair, 0.0, self.bom_pieces)
        self._fill_bom_rows()
        self.bom_sections.clear()
        for thickness, width, length in stock_totals(self.bom_rows):
            item = QtGui.QTreeWidgetItem(
                [f"{thickness:.0f} × {width:.0f} mm", f"{length / 1000.0:.2f} m"]
            )
            item.setTextAlignment(1, QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)
            self.bom_sections.addTopLevelItem(item)

        materials, assignment = stair_materials(self.stair)
        self.bom_totals.clear()
        self.bom_layouts, unassigned = stair_layout(materials, assignment, self.bom_pieces)
        self._fill_bom_layouts()
        warnings = []
        loose_wedges = [piece for piece in unassigned if piece.kind == "Wedge"]
        unassigned = [piece for piece in unassigned if piece.kind != "Wedge"]
        if unassigned:
            warnings.append(
                translate("BIM", "No material: {}").format(
                    ", ".join(sorted({piece.label for piece in unassigned}))
                )
            )
        if loose_wedges:
            warnings.append(
                translate("BIM", "No material for {} wedges: cut them from offcuts").format(
                    len(loose_wedges)
                )
            )
        for found in self.bom_layouts:
            too_large = [piece for piece in found.unplaced if piece.kind != "Wedge"]
            offcuts = [piece for piece in found.unplaced if piece.kind == "Wedge"]
            if too_large:
                warnings.append(
                    translate("BIM", "Too large for {}: {}").format(
                        found.material.name,
                        ", ".join(piece.label for piece in too_large),
                    )
                )
            if offcuts:
                warnings.append(
                    translate(
                        "BIM", "No room left on {} for {} wedges: cut them from offcuts"
                    ).format(found.material.name, len(offcuts))
                )
        self.bom_warnings.setText("\n".join(warnings))
        self.bom_warnings.setVisible(bool(warnings))
        self.bom_export_dxf.setEnabled(bool(self.bom_layouts))
        self.bom_preview.show_stock(None)
        if self.bom_totals.topLevelItemCount():
            first = self.bom_totals.topLevelItem(0)
            self.bom_totals.setCurrentItem(first.child(0) if first.childCount() else first)

    def _fill_bom_rows(self):
        self.bom_tree.clear()
        for row in self.bom_rows:
            item = QtGui.QTreeWidgetItem(
                [
                    kind_label(row["kind"]),
                    str(row["quantity"]),
                    _number(row["thickness"]),
                    _number(row["length"]),
                    _number(row["width"]),
                    ", ".join(row["parts"]),
                ]
            )
            for column in range(1, 5):
                item.setTextAlignment(column, QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)
            self.bom_tree.addTopLevelItem(item)
        for column in range(5):
            self.bom_tree.resizeColumnToContents(column)

    def _fill_bom_layouts(self):
        for found in self.bom_layouts:
            material = found.material
            used = (
                sum(stock.used for stock in found.stocks) / len(found.stocks)
                if found.stocks
                else 0.0
            )
            top = QtGui.QTreeWidgetItem(
                [
                    f"{material.name}: {_stock_size(found)} mm",
                    str(len(found.stocks)),
                    f"{used * 100.0:.0f} %",
                ]
            )
            top.setData(0, _PIECE_ROLE, ("material", found))
            for index, stock in enumerate(found.stocks, start=1):
                child = QtGui.QTreeWidgetItem(
                    [
                        translate("BIM", "{} {}").format(_stock_name(found), index),
                        str(len(stock.placed)),
                        f"{stock.used * 100.0:.0f} %",
                    ]
                )
                child.setData(0, _PIECE_ROLE, ("stock", found, stock))
                top.addChild(child)
            self.bom_totals.addTopLevelItem(top)
            top.setExpanded(True)

    def _bom_stock_selected(self, current, *args):
        data = current.data(0, _PIECE_ROLE) if current is not None else None
        stock = None
        if data and data[0] == "stock":
            stock = data[2]
        elif data and data[0] == "material" and data[1].stocks:
            stock = data[1].stocks[0]
        self.bom_preview.show_stock(stock)

    # Export

    def _export_bom(self, *args):
        if self.bom_pieces is None or self.bom_signature != self._bom_signature():
            self._refresh_bom_pieces()
        path, _filter = QtGui.QFileDialog.getSaveFileName(
            self.form,
            translate("BIM", "Export bill of materials"),
            f"{self.stair.Label}.csv",
            translate("BIM", "CSV files (*.csv)"),
        )
        if not path:
            return
        self._write_bom_file(path, stock_csv(self.bom_rows))

    def _export_bom_dxf(self, *args):
        current = self.bom_totals.currentItem()
        data = current.data(0, _PIECE_ROLE) if current is not None else None
        if not data:
            QtGui.QMessageBox.information(
                self.form,
                translate("BIM", "Export DXF"),
                translate("BIM", "Select a material, or one of its planks or panels"),
            )
            return
        found = data[1]
        stocks = [data[2]] if data[0] == "stock" else found.stocks
        name = (
            f"{self.stair.Label} {current.text(0)}"
            if data[0] == "stock"
            else f"{self.stair.Label} {_stock_name(found)}"
        )
        path, _filter = QtGui.QFileDialog.getSaveFileName(
            self.form,
            translate("BIM", "Export DXF"),
            f"{name}.dxf",
            translate("BIM", "DXF files (*.dxf)"),
        )
        if not path:
            return
        # DXF R12 text is in the drawing's code page, not UTF-8.
        self._write_bom_file(path, stock_dxf(stocks), "cp1252")

    def _write_bom_file(self, path, text, encoding="utf-8"):
        try:
            with open(path, "w", encoding=encoding, errors="replace", newline="") as stream:
                stream.write(text)
        except OSError as error:
            QtGui.QMessageBox.warning(self.form, translate("BIM", "Export"), str(error))
