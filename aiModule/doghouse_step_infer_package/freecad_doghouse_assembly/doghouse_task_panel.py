"""Task panel UI for doghouse clip recommendation and preview."""
from __future__ import annotations

import os
from pathlib import Path

try:
    from clip_library_model import merge_recommendations, scan_clip_library
    from freecad_io import color_analysis_faces, clear_preview_group, default_cache_dir, export_active_model_to_step, import_clip_previews
    from qt_compat import load_qt, message_box
    from runner import DoghouseRunner, default_doghouse_python
except ImportError:
    from .clip_library_model import merge_recommendations, scan_clip_library
    from .freecad_io import color_analysis_faces, clear_preview_group, default_cache_dir, export_active_model_to_step, import_clip_previews
    from .qt_compat import load_qt, message_box
    from .runner import DoghouseRunner, default_doghouse_python


def _default_project_root() -> Path:
    env_value = os.environ.get("DOGHOUSE_PROJECT_ROOT")
    if env_value:
        return Path(env_value)
    package_root = Path(__file__).resolve().parent.parent
    search_roots = [package_root, Path.cwd()]
    try:
        import FreeCAD

        search_roots.append(Path(FreeCAD.getHomePath()).resolve())
    except Exception:
        pass
    checked = set()
    for root in search_roots:
        for parent in (root, *root.parents):
            for candidate in (
                parent,
                parent / "aiModule" / "doghouse_step_infer_package",
            ):
                resolved = candidate.resolve()
                if resolved in checked:
                    continue
                checked.add(resolved)
                if (resolved / "doghouse_ai" / "recommend_and_assemble.py").is_file():
                    return resolved
    return package_root


def _name_variants(name: str) -> list[str]:
    stem = Path(str(name)).stem.strip()
    variants = [stem]
    for old, new in (("_", "-"), ("-", "_")):
        converted = stem.replace(old, new)
        if converted not in variants:
            variants.append(converted)
    return [item for item in variants if item]


def resolve_prediction_json_from_names(root: Path, names: list[str], explicit: str = "") -> Path | None:
    if explicit:
        path = Path(explicit)
        return path if path.exists() else None
    root = Path(root)
    search_dirs = [root / "step - 副本2", root]
    suffixes = ["_annotation.json", " annotation.json", ".json"]
    for name in names:
        for variant in _name_variants(name):
            for folder in search_dirs:
                for suffix in suffixes:
                    candidate = folder / f"{variant}{suffix}"
                    if candidate.exists():
                        return candidate
    return None


def resolve_source_step_from_prediction_json(prediction_json: Path | None) -> Path | None:
    if prediction_json is None:
        return None
    prediction_json = Path(prediction_json)
    stem = prediction_json.stem
    for suffix in ("_annotation", " annotation", "_doghouse_pred_faces"):
        if stem.endswith(suffix):
            stem = stem[: -len(suffix)]
            break
    for ext in (".step", ".stp", ".STEP", ".STP"):
        candidate = prediction_json.with_name(f"{stem}{ext}")
        if candidate.exists():
            return candidate
    return None


class DoghouseTaskPanel:
    def __init__(self):
        QtCore, QtGui, QtWidgets = load_qt()
        self.QtCore = QtCore
        self.QtGui = QtGui
        self.QtWidgets = QtWidgets
        self.project_root = _default_project_root()
        self.recommendation_payload = None
        self.source_step = None
        self.active_clip_name = ""

        self.form = QtWidgets.QWidget()
        self.form.setWindowTitle("Doghouse Clip Assembly")
        layout = QtWidgets.QVBoxLayout(self.form)

        layout.addWidget(QtWidgets.QLabel("OK: analyze first, then preview the selected clip."))
        self.status_label = QtWidgets.QLabel("Ready")
        layout.addWidget(self.status_label)

        buttons = QtWidgets.QHBoxLayout()
        self.analyze_button = QtWidgets.QPushButton("Analyze && Recommend")
        self.clear_generated_button = QtWidgets.QPushButton("清除已生成的卡扣")
        buttons.addWidget(self.analyze_button)
        buttons.addWidget(self.clear_generated_button)
        layout.addLayout(buttons)

        self.project_root_edit = QtWidgets.QLineEdit(str(self.project_root))
        self.python_edit = QtWidgets.QLineEdit(
            str(default_doghouse_python(self.project_root))
        )
        self.clip_dir_edit = QtWidgets.QLineEdit(str(self.project_root / "卡扣库"))
        self.prediction_json_edit = QtWidgets.QLineEdit("")
        self.filter_edit = QtWidgets.QLineEdit("")
        self.valid_only_check = QtWidgets.QCheckBox("Only valid diameter gap")
        self.reverse_direction_check = QtWidgets.QCheckBox("Reverse clip direction")

        layout.addWidget(QtWidgets.QLabel("Search clip name"))
        layout.addWidget(self.filter_edit)
        layout.addWidget(self.valid_only_check)
        layout.addWidget(self.reverse_direction_check)

        content_splitter = QtWidgets.QSplitter(QtCore.Qt.Horizontal)
        layout.addWidget(content_splitter)

        left_pane = QtWidgets.QWidget()
        left_layout = QtWidgets.QVBoxLayout(left_pane)
        right_pane = QtWidgets.QWidget()
        right_layout = QtWidgets.QVBoxLayout(right_pane)
        content_splitter.addWidget(left_pane)
        content_splitter.addWidget(right_pane)
        content_splitter.setStretchFactor(0, 2)
        content_splitter.setStretchFactor(1, 3)

        self.recommend_table = QtWidgets.QTableWidget(0, 8)
        self.recommend_table.setHorizontalHeaderLabels(
            ["Rank", "Clip", "Diameter", "BOLT Height", "Diameter Gap", "Height Gap", "Valid", "Score"]
        )
        left_layout.addWidget(QtWidgets.QLabel("Recommendations"))
        left_layout.addWidget(self.recommend_table)

        self.library_table = QtWidgets.QTableWidget(0, 6)
        self.library_table.setHorizontalHeaderLabels(
            ["Clip", "Diameter", "BOLT Height", "Rank", "Diameter Gap", "Height Gap"]
        )
        right_layout.addWidget(QtWidgets.QLabel("Clip Library Preview"))
        self.clip_grid = QtWidgets.QListWidget()
        self.clip_grid.setViewMode(QtWidgets.QListView.IconMode)
        self.clip_grid.setIconSize(QtCore.QSize(150, 110))
        self.clip_grid.setGridSize(QtCore.QSize(170, 150))
        self.clip_grid.setResizeMode(QtWidgets.QListView.Adjust)
        self.clip_grid.setMovement(QtWidgets.QListView.Static)
        self.clip_grid.setSpacing(8)
        right_layout.addWidget(self.clip_grid)

        self.analyze_button.clicked.connect(self.on_analyze)
        self.clear_generated_button.clicked.connect(self.on_clear)
        self.filter_edit.textChanged.connect(self.refresh_library_table)
        self.valid_only_check.stateChanged.connect(self.refresh_library_table)
        self.recommend_table.itemSelectionChanged.connect(self._select_recommendation)
        self.clip_grid.itemSelectionChanged.connect(self._select_clip_card)
        self.recommend_table.itemDoubleClicked.connect(lambda _item: self.on_preview())
        self.clip_grid.itemDoubleClicked.connect(lambda _item: self.on_preview())

        self.refresh_library_table()

    def accept(self):
        if self.recommendation_payload and self.active_clip_name:
            self.on_preview()
        else:
            self.on_analyze()
        return False

    def reject(self):
        return True

    def _runner(self) -> DoghouseRunner:
        return DoghouseRunner(
            project_root=Path(self.project_root_edit.text()),
            python_exe=Path(self.python_edit.text()),
            clip_library=Path(self.clip_dir_edit.text()),
        )

    def _cache_dir(self) -> Path:
        try:
            import FreeCAD

            doc_name = FreeCAD.ActiveDocument.Name if FreeCAD.ActiveDocument else "NoDocument"
        except Exception:
            doc_name = "NoDocument"
        return default_cache_dir(Path(self.project_root_edit.text()), doc_name)

    def _active_model_names(self) -> list[str]:
        names = []
        try:
            import FreeCAD
            import FreeCADGui

            selected = FreeCADGui.Selection.getSelection()
            objects = selected or (FreeCAD.ActiveDocument.Objects if FreeCAD.ActiveDocument else [])
            if FreeCAD.ActiveDocument and getattr(FreeCAD.ActiveDocument, "FileName", ""):
                names.append(Path(FreeCAD.ActiveDocument.FileName).stem)
            for obj in objects:
                for value in (getattr(obj, "Label", ""), getattr(obj, "Name", "")):
                    if value and value not in names:
                        names.append(value)
        except Exception:
            pass
        return names

    def _resolve_prediction_json(self) -> Path | None:
        resolved = resolve_prediction_json_from_names(
            Path(self.project_root_edit.text()),
            self._active_model_names(),
            explicit="",
        )
        return resolved

    def on_analyze(self):
        try:
            cache_dir = self._cache_dir()
            self.source_step = export_active_model_to_step(cache_dir)
            output_json = cache_dir / "recommendation.json"
            self.recommendation_payload = self._runner().recommend(
                self.source_step,
                output_json,
                prediction_json=None,
            )
            self.active_clip_name = self.recommendation_payload.get("selected_clip") or ""
            self.refresh_recommend_table()
            self.refresh_library_table()
            try:
                color_analysis_faces(self.recommendation_payload)
                color_status = "colored"
            except Exception as color_exc:
                color_status = f"color skipped: {color_exc}"
            self.status_label.setText(f"Recommended: {self.active_clip_name} ({color_status})")
        except Exception as exc:
            message_box(self.form, "Doghouse Analysis Failed", str(exc), critical=True)

    def on_preview(self):
        if not self.source_step or not self.active_clip_name:
            message_box(self.form, "Doghouse Preview", "Please analyze and select a clip first.", critical=True)
            return
        try:
            output_json = self._cache_dir() / "placement.json"
            payload = self._runner().placement(
                self.source_step,
                output_json,
                clip_name=self.active_clip_name,
                prediction_json=None,
                invert_indices=self._invert_indices_for_preview(),
            )
            count = len(import_clip_previews(payload))
            self.status_label.setText(f"Previewed {count} clip object(s): {self.active_clip_name}")
        except Exception as exc:
            message_box(self.form, "Doghouse Preview Failed", str(exc), critical=True)

    def _invert_indices_for_preview(self) -> list[int] | None:
        if not self.reverse_direction_check.isChecked():
            return None
        hole_count = int((self.recommendation_payload or {}).get("hole_count") or 0)
        return list(range(1, hole_count + 1)) if hole_count > 0 else None

    def on_color_faces(self):
        if not self.recommendation_payload:
            message_box(self.form, "Doghouse Coloring", "Please analyze first.", critical=True)
            return
        try:
            color_analysis_faces(self.recommendation_payload)
            self.status_label.setText("Colored mount faces and hole walls")
        except Exception as exc:
            message_box(self.form, "Doghouse Coloring Failed", str(exc), critical=True)

    def on_clear(self):
        clip_names = [
            clip.get("name")
            for clip in (self.recommendation_payload or {}).get("clips", [])
            if clip.get("name")
        ]
        if self.active_clip_name and self.active_clip_name not in clip_names:
            clip_names.append(self.active_clip_name)
        removed = clear_preview_group(clip_names=clip_names)
        self.status_label.setText(f"Cleared {removed} generated clip object(s)")

    def refresh_recommend_table(self):
        clips = (self.recommendation_payload or {}).get("clips", [])
        self.recommend_table.setRowCount(len(clips))
        for row, clip in enumerate(clips):
            values = [
                clip.get("rank", ""),
                clip.get("name", ""),
                clip.get("clip_diameter_mm", ""),
                clip.get("clip_depth_mm", ""),
                clip.get("diameter_gap_mm", ""),
                clip.get("depth_gap_mm", ""),
                "yes" if clip.get("diameter_valid") else "no",
                clip.get("score", ""),
            ]
            self._set_row(self.recommend_table, row, values)
        self.recommend_table.resizeColumnsToContents()

    def refresh_library_table(self):
        rows = scan_clip_library(Path(self.clip_dir_edit.text()))
        rows = merge_recommendations(rows, self.recommendation_payload)
        text = self.filter_edit.text().strip().lower()
        if text:
            rows = [row for row in rows if text in row["name"].lower()]
        if self.valid_only_check.isChecked():
            rows = [row for row in rows if row.get("diameter_valid")]
        rows.sort(key=lambda row: (row.get("rank") is None, row.get("rank") or 999999, row["name"]))
        self.library_rows = rows
        self.library_table.setRowCount(len(rows))
        self.clip_grid.clear()
        for row_idx, row in enumerate(rows):
            values = [
                row.get("name", ""),
                row.get("bolt_cyl_diameter_mm", ""),
                row.get("bolt_cyl_height_mm", ""),
                row.get("rank", ""),
                row.get("diameter_gap_mm", ""),
                row.get("depth_gap_mm", ""),
            ]
            self._set_row(self.library_table, row_idx, values)
            self._add_clip_card(row)
        self.library_table.resizeColumnsToContents()
        if self.active_clip_name:
            self._highlight_clip_card(self.active_clip_name)

    def _set_row(self, table, row: int, values):
        for col, value in enumerate(values):
            table.setItem(row, col, self.QtWidgets.QTableWidgetItem(str(value)))

    def _select_recommendation(self):
        items = self.recommend_table.selectedItems()
        if items:
            self.active_clip_name = self.recommend_table.item(items[0].row(), 1).text()
            self._highlight_clip_card(self.active_clip_name)
            self.status_label.setText(f"Selected: {self.active_clip_name}")

    def _select_clip_card(self):
        items = self.clip_grid.selectedItems()
        if items:
            self.active_clip_name = items[0].data(self.QtCore.Qt.UserRole)
            self.status_label.setText(f"Selected: {self.active_clip_name}")

    def _highlight_clip_card(self, clip_name: str):
        previous = self.clip_grid.blockSignals(True)
        try:
            self.clip_grid.clearSelection()
            for idx in range(self.clip_grid.count()):
                item = self.clip_grid.item(idx)
                if item.data(self.QtCore.Qt.UserRole) == clip_name:
                    item.setSelected(True)
                    self.clip_grid.scrollToItem(item)
                    break
        finally:
            self.clip_grid.blockSignals(previous)

    def _add_clip_card(self, row: dict):
        item = self.QtWidgets.QListWidgetItem()
        item.setText(row.get("name", ""))
        item.setIcon(self.QtGui.QIcon(self._clip_preview_pixmap(row)))
        item.setData(self.QtCore.Qt.UserRole, row.get("name", ""))
        self.clip_grid.addItem(item)

    def _clip_preview_pixmap(self, row: dict):
        pixmap = self.QtGui.QPixmap(150, 110)
        pixmap.fill(self.QtGui.QColor(245, 248, 250))
        painter = self.QtGui.QPainter(pixmap)
        painter.setRenderHint(self.QtGui.QPainter.Antialiasing, True)
        rank = row.get("rank")
        diameter = float(row.get("bolt_cyl_diameter_mm") or 0.0)
        height = float(row.get("bolt_cyl_height_mm") or 0.0)
        thumb_path = row.get("thumbnail_path") or ""
        source = self.QtGui.QPixmap(thumb_path) if thumb_path else self.QtGui.QPixmap()
        if not source.isNull():
            scaled = source.scaled(
                142,
                82,
                self.QtCore.Qt.KeepAspectRatio,
                self.QtCore.Qt.SmoothTransformation,
            )
            x = int((150 - scaled.width()) / 2)
            y = int((82 - scaled.height()) / 2) + 4
            painter.drawPixmap(x, y, scaled)
        else:
            valid = row.get("diameter_valid")
            body_color = self.QtGui.QColor(82, 202, 205) if valid or rank else self.QtGui.QColor(185, 205, 215)
            edge_color = self.QtGui.QColor(70, 80, 90)
            painter.setPen(self.QtGui.QPen(edge_color, 2))
            painter.setBrush(self.QtGui.QBrush(body_color))
            painter.drawRoundedRect(32, 36, 86, 34, 12, 12)
            painter.setBrush(self.QtGui.QBrush(self.QtGui.QColor(230, 235, 235)))
            painter.drawEllipse(24, 34, 28, 38)
            painter.drawEllipse(98, 34, 28, 38)
            painter.setBrush(self.QtGui.QBrush(self.QtGui.QColor(245, 248, 250)))
            painter.drawEllipse(107, 47, 10, 10)

        if rank:
            painter.setPen(self.QtGui.QPen(self.QtGui.QColor(255, 255, 255), 1))
            painter.setBrush(self.QtGui.QBrush(self.QtGui.QColor(230, 70, 45)))
            painter.drawEllipse(6, 6, 28, 28)
            painter.drawText(6, 6, 28, 28, int(self.QtCore.Qt.AlignCenter), str(rank))

        painter.setPen(self.QtGui.QPen(self.QtGui.QColor(30, 30, 30), 1))
        painter.drawText(8, 84, 134, 14, int(self.QtCore.Qt.AlignCenter), row.get("name", ""))
        spec = f"D{diameter:.1f} H{height:.1f}" if diameter or height else "No geom"
        painter.drawText(8, 98, 134, 12, int(self.QtCore.Qt.AlignCenter), spec)
        painter.end()
        return pixmap
