# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""The BIM Classification command"""

import os
import re

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")
BSDD_PROVIDER_MODE_KEY = "BimClassificationProvider"
BSDD_DICTIONARY_META_KEY = "BIM_BsddActiveDictionaries"
BSDD_DICTIONARY_META_PRESENT_KEY = "BIM_BsddActiveDictionariesPresent"
BSDD_CONTRACT_ROLE = 33


def _get_qt_modules():
    try:
        from PySide import QtCore, QtGui

        try:
            from PySide import QtWidgets
        except ImportError:
            QtWidgets = QtGui
    except ImportError:
        try:
            from PySide6 import QtCore, QtGui, QtWidgets
        except ImportError:
            from PySide2 import QtCore, QtGui, QtWidgets
    return QtCore, QtGui, QtWidgets


def _load_bsdd_module():
    try:
        import BimBsddClient as BsddModule
    except ImportError:
        import BimBsdd as BsddModule
    return BsddModule


def _load_bsdd_contract_module():
    import BimBsddContract

    return BimBsddContract


def _print_ui_error(context, err):
    FreeCAD.Console.PrintError("BIM bSDD UI error in {}: {}\n".format(context, err))


class BIM_Classification:

    def GetResources(self):
        return {
            "Pixmap": "BIM_Classification",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Classification", "Manage Classification"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Classification",
                "Manages classification systems and apply classification to objects",
            ),
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):

        # only raise the dialog if it is already open
        if getattr(self, "form", None):
            self.form.raise_()
            return

        import Draft
        from bimcommands import BimMaterial

        QtCore, QtGui, QtWidgets = _get_qt_modules()
        BsddModule = _load_bsdd_module()
        BsddContractModule = _load_bsdd_contract_module()

        # init checks
        if not hasattr(self, "Classes"):
            self.Classes = {}
        self.isEditing = None
        current = None

        # load the form and set the tree model up
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/dialogClassification.ui")
        self.form.setWindowIcon(QtGui.QIcon(":/icons/BIM_Classification.svg"))
        self.form.groupMode.setItemIcon(
            0, QtGui.QIcon(":/icons/Arch_SectionPlane_Tree.svg")
        )  # Alphabetical
        self.form.groupMode.setItemIcon(1, QtGui.QIcon(":/icons/IFC.svg"))  # Type
        self.form.groupMode.setItemIcon(2, QtGui.QIcon(":/icons/Arch_Material.svg"))  # Material
        self.form.groupMode.setItemIcon(3, QtGui.QIcon(":/icons/Document.svg"))  # Model structure

        # restore saved values
        self.form.onlyVisible.setChecked(PARAMS.GetInt("BimClassificationVisibleState", 0))
        self.form.checkPrefix.setChecked(PARAMS.GetInt("BimClassificationSystemNamePrefix", 1))
        w = PARAMS.GetInt("BimClassificationDialogWidth", 629)
        h = PARAMS.GetInt("BimClassificationDialogHeight", 516)
        self.form.resize(w, h)

        # add modified search box from bimmaterial
        searchBox = BimMaterial.MatLineEdit(self.form)
        searchBox.setPlaceholderText(translate("BIM", "Search…"))
        searchBox.setToolTip(translate("BIM", "Searches classes"))
        self.form.search = searchBox
        self.form.horizontalLayout_2.addWidget(searchBox)
        self._bsdd_module = BsddModule
        self._bsdd_contract_module = BsddContractModule
        self._bsdd_client = BsddModule.get_bsdd_network_client()
        self._bsdd_context_ifc_class = ""
        self._bsdd_selected_concept = None
        self._restored_bsdd_dictionary_uris = set()
        self._has_saved_bsdd_dictionary_state = False
        self._bsdd_dictionary_uris = []
        self._bsdd_search_batch_id = 0
        self._bsdd_dictionary_labels = {}
        self._bsdd_dictionary_nodes = {}
        self._bsdd_pending_requests = {}
        self._bsdd_dictionary_candidates = {}
        self._bsdd_dictionary_has_results = set()
        self._bsdd_contract = None
        self._bsdd_contract_detail_payload = None
        self._bsdd_object_contracts = {}
        self._search_timer = QtCore.QTimer(self.form)
        self._search_timer.setSingleShot(True)
        self._search_timer.setInterval(300)
        self._selection_observer_installed = False
        self._updating_bsdd_dictionaries = False
        self._bsdd_signals_connected = False
        self._setup_bsdd_ui(QtCore, QtGui, QtWidgets)
        self._connect_bsdd_signals()
        self._install_selection_observer()

        # set help line
        self.form.labelDownload.setText(
            self.form.labelDownload.text().replace(
                "%s", os.path.join(FreeCAD.getUserAppDataDir(), "BIM", "Classification")
            )
        )

        # hide materials list if we are editing a particular object
        if len(FreeCADGui.Selection.getSelection()) == 1:
            self.isEditing = FreeCADGui.Selection.getSelection()[0]
            pl = self.isEditing.PropertiesList
            if ("StandardCode" in pl) or ("IfcClass" in pl):
                self.form.groupMaterials.hide()
                self.form.buttonApply.hide()
                self.form.buttonRename.hide()
                self.form.setWindowTitle(translate("BIM", "Editing") + " " + self.isEditing.Label)
                if "IfcClass" in pl:
                    # load existing class if needed
                    from nativeifc import ifc_classification

                    ifc_classification.show_classification(self.isEditing)
                if "StandardCode" in pl:
                    current = self.isEditing.StandardCode
                elif "Classification" in self.isEditing.PropertiesList:
                    current = self.isEditing.Classification

        # fill materials list
        self.objectslist = {}
        self.matlist = {}
        self.labellist = {}
        for obj in FreeCAD.ActiveDocument.Objects:
            if "StandardCode" in obj.PropertiesList:
                if Draft.getType(obj) in ["Material", "MultiMaterial"]:
                    self.matlist[obj.Name] = obj.StandardCode
                else:
                    self.objectslist[obj.Name] = obj.StandardCode
                self.labellist[obj.Name] = obj.Label
            elif "Classification" in obj.PropertiesList:
                self.objectslist[obj.Name] = obj.Classification
                self.labellist[obj.Name] = obj.Label

        # fill objects list
        if not self.isEditing:
            self.updateObjects()

        # fill available classifications
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetString(
            "DefaultClassificationSystem", ""
        )
        presetdir = os.path.join(FreeCAD.getUserAppDataDir(), "BIM", "Classification")
        if os.path.isdir(presetdir):
            presets = []
            for f in os.listdir(presetdir):
                if f.lower().endswith(".xml") or f.lower().endswith(".ifc"):
                    n = os.path.splitext(f)[0]
                    if not n in presets:
                        presets.append(n)
                        self.form.comboSystem.addItem(n)
                        if n == p:
                            self.form.comboSystem.setCurrentIndex(self.form.comboSystem.count() - 1)

        # connect signals
        self.form.comboSystem.currentIndexChanged.connect(self.updateClasses)
        self.form.buttonApply.clicked.connect(self.apply)
        self.form.buttonRename.clicked.connect(self.rename)
        self.form.search.textEdited.connect(self.updateClasses)
        self.form.buttonBox.accepted.connect(self.accept)
        self.form.rejected.connect(self.reject)  # also triggered by self.form.buttonBox.rejected
        self.form.groupMode.currentIndexChanged.connect(self.updateObjects)
        self.form.treeClass.itemDoubleClicked.connect(self.apply)
        self.form.search.up.connect(self.onUpArrow)
        self.form.search.down.connect(self.onDownArrow)
        if hasattr(self.form.onlyVisible, "checkStateChanged"):  # Qt version >= 6.7.0
            self.form.onlyVisible.checkStateChanged.connect(self.onVisible)
            self.form.checkPrefix.checkStateChanged.connect(self.onPrefix)
        else:  # Qt version < 6.7.0
            self.form.onlyVisible.stateChanged.connect(self.onVisible)
            self.form.checkPrefix.stateChanged.connect(self.onPrefix)
        # center the dialog over FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.form.move(
            mw.frameGeometry().topLeft() + mw.rect().center() - self.form.rect().center()
        )

        self.updateClasses()
        self._refresh_bsdd_context()
        self._request_bsdd_dictionaries()

        # select current classification
        if current:
            system, classification = current.split(" ", 1)
            print("searching for", classification)
            if system in self.Classes:
                self.form.comboSystem.setCurrentText(system)
                res = self.form.treeClass.findItems(
                    classification, QtCore.Qt.MatchExactly | QtCore.Qt.MatchRecursive, 0
                )
                if res:
                    self.form.treeClass.setCurrentItem(res[0])

        self.form.show()

        self.form.search.setFocus()

    def _setup_bsdd_ui(self, QtCore, QtGui, QtWidgets):
        try:
            providerLayout = QtWidgets.QHBoxLayout()
            providerLabel = QtWidgets.QLabel(translate("BIM", "Provider"))
            self.form.comboProvider = QtWidgets.QComboBox(self.form.groupClasses)
            self.form.comboProvider.addItem("Legacy")
            self.form.comboProvider.addItem("bSDD")
            providerLayout.addWidget(providerLabel)
            providerLayout.addWidget(self.form.comboProvider, 1)
            self.form.verticalLayout_3.insertLayout(0, providerLayout)

            self.form.bsddPanel = QtWidgets.QWidget(self.form.groupClasses)
            self.form.bsddPanelLayout = QtWidgets.QVBoxLayout(self.form.bsddPanel)
            self.form.bsddPanelLayout.setContentsMargins(0, 0, 0, 0)

            self.form.bsddContextLabel = QtWidgets.QLabel(self.form.bsddPanel)
            self.form.bsddContextLabel.setWordWrap(True)
            self.form.bsddPanelLayout.addWidget(self.form.bsddContextLabel)

            self.form.bsddSearch = QtWidgets.QLineEdit(self.form.bsddPanel)
            self.form.bsddSearch.setPlaceholderText(translate("BIM", "Search bSDD concepts..."))
            self.form.bsddPanelLayout.addWidget(self.form.bsddSearch)

            self.form.bsddDictionaryToggle = QtWidgets.QToolButton(self.form.bsddPanel)
            self.form.bsddDictionaryToggle.setText(translate("BIM", "Active dictionaries"))
            self.form.bsddDictionaryToggle.setCheckable(True)
            self.form.bsddDictionaryToggle.setChecked(False)
            self.form.bsddDictionaryToggle.setToolButtonStyle(QtCore.Qt.ToolButtonTextBesideIcon)
            self.form.bsddDictionaryToggle.setArrowType(QtCore.Qt.RightArrow)
            self.form.bsddPanelLayout.addWidget(self.form.bsddDictionaryToggle)

            self.form.bsddDictionaryContainer = QtWidgets.QWidget(self.form.bsddPanel)
            self.form.bsddDictionaryContainerLayout = QtWidgets.QVBoxLayout(
                self.form.bsddDictionaryContainer
            )
            self.form.bsddDictionaryContainerLayout.setContentsMargins(0, 0, 0, 0)

            self.form.bsddDictionaryList = QtWidgets.QListWidget(self.form.bsddDictionaryContainer)
            self.form.bsddDictionaryList.setSelectionMode(QtWidgets.QAbstractItemView.NoSelection)
            self.form.bsddDictionaryList.setMinimumHeight(110)
            self.form.bsddDictionaryContainerLayout.addWidget(self.form.bsddDictionaryList)
            self.form.bsddPanelLayout.addWidget(self.form.bsddDictionaryContainer)
            self.form.bsddDictionaryContainer.setVisible(False)

            self.form.bsddSplitter = QtWidgets.QSplitter(QtCore.Qt.Vertical, self.form.bsddPanel)
            self.form.bsddResultsTree = QtWidgets.QTreeView(self.form.bsddSplitter)
            self.form.bsddResultsTree.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
            self.form.bsddResultsTree.setAlternatingRowColors(True)
            self.form.bsddResultsTree.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
            self.form.bsddResultsTree.setUniformRowHeights(True)

            self.form.bsddPropertyTable = QtWidgets.QTableWidget(self.form.bsddSplitter)
            self.form.bsddPropertyTable.setColumnCount(2)
            self.form.bsddPropertyTable.setHorizontalHeaderLabels(
                [translate("BIM", "Property"), translate("BIM", "Value")]
            )
            self.form.bsddPropertyTable.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
            self.form.bsddPropertyTable.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
            self.form.bsddPropertyTable.setWordWrap(True)
            self.form.bsddPropertyTable.verticalHeader().setVisible(False)
            self.form.bsddPropertyTable.horizontalHeader().setStretchLastSection(True)
            self.form.bsddPanelLayout.addWidget(self.form.bsddSplitter, 1)

            self.form.bsddResultsModel = QtGui.QStandardItemModel(self.form.bsddResultsTree)
            self.form.bsddResultsModel.setHorizontalHeaderLabels(
                [translate("BIM", "bSDD Concepts")]
            )
            self.form.bsddResultsTree.setModel(self.form.bsddResultsModel)
            self.form.bsddResultsTree.header().setStretchLastSection(True)

            insertIndex = self.form.verticalLayout_3.indexOf(self.form.treeClass)
            if insertIndex < 0:
                insertIndex = 3
            self.form.verticalLayout_3.insertWidget(insertIndex, self.form.bsddPanel)
            self.form.bsddPanel.hide()

            provider = PARAMS.GetString(BSDD_PROVIDER_MODE_KEY, "Legacy")
            self.form.comboProvider.setCurrentText(
                provider if provider in ["Legacy", "bSDD"] else "Legacy"
            )
            (
                self._has_saved_bsdd_dictionary_state,
                self._restored_bsdd_dictionary_uris,
            ) = self._load_bsdd_dictionary_state()
            self._apply_provider_visibility()
        except Exception as err:
            _print_ui_error("_setup_bsdd_ui", err)

    def _connect_bsdd_signals(self):
        try:
            if getattr(self, "_bsdd_signals_connected", False):
                return
            self.form.comboProvider.currentIndexChanged.connect(self._on_provider_changed)
            self.form.bsddSearch.textChanged.connect(self._on_bsdd_search_text_changed)
            self._search_timer.timeout.connect(self._perform_bsdd_search)
            self.form.bsddDictionaryToggle.toggled.connect(self._on_bsdd_dictionary_toggle)
            self.form.bsddDictionaryList.itemChanged.connect(self._on_bsdd_dictionary_item_changed)
            self.form.bsddResultsTree.selectionModel().currentChanged.connect(
                self._on_bsdd_result_changed
            )
            self._bsdd_client.dictionariesReady.connect(self._on_bsdd_dictionaries_ready)
            self._bsdd_client.searchReady.connect(self._on_bsdd_search_ready)
            self._bsdd_client.conceptReady.connect(self._on_bsdd_concept_ready)
            self._bsdd_client.requestFailed.connect(self._on_bsdd_request_failed)
            self._bsdd_signals_connected = True
        except Exception as err:
            _print_ui_error("_connect_bsdd_signals", err)

    def _disconnect_bsdd_signals(self):
        try:
            if not getattr(self, "_bsdd_signals_connected", False):
                return
            self._bsdd_client.dictionariesReady.disconnect(self._on_bsdd_dictionaries_ready)
            self._bsdd_client.searchReady.disconnect(self._on_bsdd_search_ready)
            self._bsdd_client.conceptReady.disconnect(self._on_bsdd_concept_ready)
            self._bsdd_client.requestFailed.disconnect(self._on_bsdd_request_failed)
        except (RuntimeError, TypeError):
            pass
        except Exception as err:
            _print_ui_error("_disconnect_bsdd_signals", err)
        finally:
            self._bsdd_signals_connected = False

    def _install_selection_observer(self):
        try:
            FreeCADGui.Selection.addObserver(self)
            self._selection_observer_installed = True
        except Exception as err:
            _print_ui_error("_install_selection_observer", err)

    def _remove_selection_observer(self):
        try:
            if getattr(self, "_selection_observer_installed", False):
                FreeCADGui.Selection.removeObserver(self)
                self._selection_observer_installed = False
        except Exception as err:
            _print_ui_error("_remove_selection_observer", err)

    def _request_bsdd_dictionaries(self):
        try:
            self._bsdd_client.fetch_dictionaries()
        except Exception as err:
            _print_ui_error("_request_bsdd_dictionaries", err)

    def _on_provider_changed(self, *_args):
        try:
            PARAMS.SetString(BSDD_PROVIDER_MODE_KEY, self.form.comboProvider.currentText())
            self._apply_provider_visibility()
            self._refresh_bsdd_context()
            if self._is_bsdd_provider_active():
                self._request_bsdd_dictionaries()
                self._schedule_bsdd_search()
        except Exception as err:
            _print_ui_error("_on_provider_changed", err)

    def _apply_provider_visibility(self):
        try:
            legacy_visible = not self._is_bsdd_provider_active()
            self.form.comboSystem.setVisible(legacy_visible)
            self.form.search.setVisible(legacy_visible)
            self.form.treeClass.setVisible(legacy_visible)
            self.form.buttonApply.setVisible(True)
            self.form.buttonRename.setVisible(True)
            self.form.checkPrefix.setVisible(True)
            self.form.bsddPanel.setVisible(not legacy_visible)
            self.form.labelDownload.setVisible(legacy_visible)
            title = (
                translate("BIM", "Available classification systems")
                if legacy_visible
                else translate("BIM", "buildingSMART Data Dictionary")
            )
            self.form.groupClasses.setTitle(title)
        except Exception as err:
            _print_ui_error("_apply_provider_visibility", err)

    def _is_bsdd_provider_active(self):
        try:
            return (
                hasattr(self.form, "comboProvider")
                and self.form.comboProvider.currentText() == "bSDD"
            )
        except Exception as err:
            _print_ui_error("_is_bsdd_provider_active", err)
            return False

    def _on_bsdd_search_text_changed(self, *_args):
        try:
            self._schedule_bsdd_search()
        except Exception as err:
            _print_ui_error("_on_bsdd_search_text_changed", err)

    def _schedule_bsdd_search(self):
        try:
            if not self._is_bsdd_provider_active():
                return
            if self._search_timer.isActive():
                self._search_timer.stop()
            self._search_timer.start()
        except Exception as err:
            _print_ui_error("_schedule_bsdd_search", err)

    def _normalize_bsdd_context_query(self, related_ifc_entity):
        try:
            if not related_ifc_entity:
                return ""
            normalized = str(related_ifc_entity)
            if normalized.startswith("Ifc") and len(normalized) > 3:
                normalized = normalized[3:]
            normalized = re.sub(r"(?<!^)(?=[A-Z])", " ", normalized).strip()
            return normalized or str(related_ifc_entity)
        except Exception as err:
            _print_ui_error("_normalize_bsdd_context_query", err)
            return related_ifc_entity or ""

    def _set_bsdd_results_placeholder(self, message):
        try:
            QtCore, QtGui, _QtWidgets = _get_qt_modules()
            self.form.bsddResultsModel.removeRows(0, self.form.bsddResultsModel.rowCount())
            if message:
                item = QtGui.QStandardItem(message)
                item.setEditable(False)
                item.setSelectable(False)
                item.setData(None, QtCore.Qt.UserRole)
                self.form.bsddResultsModel.appendRow(item)
            self._populate_bsdd_properties(None)
        except Exception as err:
            _print_ui_error("_set_bsdd_results_placeholder", err)

    def _get_bsdd_search_inputs(self):
        try:
            query_text = self.form.bsddSearch.text().strip()
            active_dictionaries = self._get_active_bsdd_dictionaries()
            related_ifc_entity = self._bsdd_context_ifc_class or ""
            return query_text, active_dictionaries, related_ifc_entity
        except Exception as err:
            _print_ui_error("_get_bsdd_search_inputs", err)
            return "", [], ""

    def _build_bsdd_search_candidates(self, query_text, active_dictionaries, related_ifc_entity):
        try:
            candidates = []
            normalized_context_query = self._normalize_bsdd_context_query(related_ifc_entity)

            if query_text:
                candidates.append((query_text, ""))
                return candidates

            if related_ifc_entity:
                fallback_pairs = [
                    (normalized_context_query, related_ifc_entity),
                    (normalized_context_query, ""),
                    (related_ifc_entity, ""),
                ]
                for candidate_query, candidate_context in fallback_pairs:
                    candidate_query = (candidate_query or "").strip()
                    candidate = (candidate_query, candidate_context)
                    if candidate_query and candidate not in candidates:
                        candidates.append(candidate)
            return candidates
        except Exception as err:
            _print_ui_error("_build_bsdd_search_candidates", err)
            return []

    def _dispatch_bsdd_search_candidate(self, dictionary_uri):
        try:
            candidates = self._bsdd_dictionary_candidates.get(dictionary_uri, [])
            if not candidates:
                self._finalize_bsdd_dictionary_branch(dictionary_uri)
                return
            query_text, related_ifc_entity = candidates.pop(0)
            search_key = self._bsdd_client._search_cache_key(
                query_text, (dictionary_uri,), related_ifc_entity
            )
            self._bsdd_pending_requests[search_key] = (self._bsdd_search_batch_id, dictionary_uri)
            returned_search_key = self._bsdd_client.search_concepts(
                query_text,
                active_dictionaries=[dictionary_uri],
                related_ifc_entity=related_ifc_entity,
            )
            if returned_search_key != search_key:
                self._bsdd_pending_requests.pop(search_key, None)
                self._bsdd_pending_requests[returned_search_key] = (
                    self._bsdd_search_batch_id,
                    dictionary_uri,
                )
        except Exception as err:
            _print_ui_error("_dispatch_bsdd_search_candidate", err)

    def _initialize_bsdd_result_tree(self, active_dictionaries):
        try:
            QtCore, QtGui, _QtWidgets = _get_qt_modules()
            self.form.bsddResultsModel.removeRows(0, self.form.bsddResultsModel.rowCount())
            self._populate_bsdd_properties(None)
            self._bsdd_dictionary_nodes = {}
            for dictionary_uri in active_dictionaries:
                label = self._bsdd_dictionary_labels.get(dictionary_uri, dictionary_uri)
                dict_item = QtGui.QStandardItem(label)
                dict_item.setEditable(False)
                dict_item.setSelectable(False)
                dict_item.setData(dictionary_uri, QtCore.Qt.UserRole)
                loading_item = QtGui.QStandardItem(translate("BIM", "Searching..."))
                loading_item.setEditable(False)
                loading_item.setSelectable(False)
                dict_item.appendRow(loading_item)
                self.form.bsddResultsModel.appendRow(dict_item)
                self._bsdd_dictionary_nodes[dictionary_uri] = dict_item
            self.form.bsddResultsTree.expandAll()
        except Exception as err:
            _print_ui_error("_initialize_bsdd_result_tree", err)

    def _clear_bsdd_dictionary_branch(self, dictionary_uri):
        try:
            node = self._bsdd_dictionary_nodes.get(dictionary_uri)
            if node is not None:
                node.removeRows(0, node.rowCount())
        except Exception as err:
            _print_ui_error("_clear_bsdd_dictionary_branch", err)

    def _append_bsdd_dictionary_results(self, dictionary_uri, concepts):
        try:
            QtCore, QtGui, _QtWidgets = _get_qt_modules()
            node = self._bsdd_dictionary_nodes.get(dictionary_uri)
            if node is None:
                return
            self._clear_bsdd_dictionary_branch(dictionary_uri)
            for concept in concepts:
                name = concept.get("name") or concept.get("referenceCode") or "<unnamed>"
                concept_item = QtGui.QStandardItem(name)
                concept_item.setEditable(False)
                concept_item.setData(concept, QtCore.Qt.UserRole)
                node.appendRow(concept_item)
            self._bsdd_dictionary_has_results.add(dictionary_uri)
            index = node.index()
            if index.isValid():
                self.form.bsddResultsTree.setExpanded(index, True)
                if node.rowCount() and not self.form.bsddResultsTree.currentIndex().isValid():
                    self.form.bsddResultsTree.setCurrentIndex(node.child(0).index())
        except Exception as err:
            _print_ui_error("_append_bsdd_dictionary_results", err)

    def _finalize_bsdd_dictionary_branch(self, dictionary_uri, message=None):
        try:
            QtCore, QtGui, _QtWidgets = _get_qt_modules()
            del QtCore
            node = self._bsdd_dictionary_nodes.get(dictionary_uri)
            if node is None:
                return
            if dictionary_uri in self._bsdd_dictionary_has_results:
                return
            self._clear_bsdd_dictionary_branch(dictionary_uri)
            placeholder = QtGui.QStandardItem(
                message or translate("BIM", "No matches in this dictionary.")
            )
            placeholder.setEditable(False)
            placeholder.setSelectable(False)
            node.appendRow(placeholder)
        except Exception as err:
            _print_ui_error("_finalize_bsdd_dictionary_branch", err)

    def _finalize_bsdd_search_batch_if_empty(self):
        try:
            if self._bsdd_dictionary_has_results:
                return
            if self._bsdd_pending_requests:
                return
            for dictionary_uri in self._bsdd_dictionary_nodes.keys():
                self._finalize_bsdd_dictionary_branch(dictionary_uri)
        except Exception as err:
            _print_ui_error("_finalize_bsdd_search_batch_if_empty", err)

    def _perform_bsdd_search(self):
        try:
            if not self._is_bsdd_provider_active():
                return
            self._refresh_bsdd_context()
            query_text, active_dictionaries, related_ifc_entity = self._get_bsdd_search_inputs()
            text_search_active = bool(query_text)
            self._bsdd_search_batch_id += 1
            self._bsdd_pending_requests = {}
            self._bsdd_dictionary_candidates = {}
            self._bsdd_dictionary_has_results = set()
            if not active_dictionaries:
                self._set_bsdd_results_placeholder(
                    translate("BIM", "Select at least one bSDD dictionary to search.")
                )
                if hasattr(self.form, "bsddDictionaryToggle") and (
                    not self.form.bsddDictionaryToggle.isChecked()
                ):
                    self.form.bsddDictionaryToggle.setChecked(True)
                return
            candidates = self._build_bsdd_search_candidates(
                query_text, active_dictionaries, related_ifc_entity
            )
            if not candidates:
                self._set_bsdd_results_placeholder(
                    translate("BIM", "Select an IFC object or enter search text.")
                )
                return
            self._initialize_bsdd_result_tree(active_dictionaries)
            for dictionary_uri in active_dictionaries:
                self._bsdd_dictionary_candidates[dictionary_uri] = list(candidates)
                self._dispatch_bsdd_search_candidate(dictionary_uri)
            if text_search_active:
                FreeCAD.Console.PrintMessage(
                    "bSDD text search across {} selected dictionaries for '{}'\n".format(
                        len(active_dictionaries), query_text
                    )
                )
        except Exception as err:
            _print_ui_error("_perform_bsdd_search", err)

    def _on_bsdd_dictionary_toggle(self, checked):
        try:
            QtCore, _QtGui, _QtWidgets = _get_qt_modules()
            self.form.bsddDictionaryContainer.setVisible(bool(checked))
            self.form.bsddDictionaryToggle.setArrowType(
                QtCore.Qt.DownArrow if checked else QtCore.Qt.RightArrow
            )
        except Exception as err:
            _print_ui_error("_on_bsdd_dictionary_toggle", err)

    def _get_active_bsdd_dictionaries(self):
        active = []
        try:
            QtCore, _QtGui, _QtWidgets = _get_qt_modules()
            for row in range(self.form.bsddDictionaryList.count()):
                item = self.form.bsddDictionaryList.item(row)
                if item.checkState() == QtCore.Qt.Checked:
                    uri = item.data(QtCore.Qt.UserRole)
                    if uri:
                        active.append(uri)
        except Exception as err:
            _print_ui_error("_get_active_bsdd_dictionaries", err)
        return active

    def _on_bsdd_dictionary_item_changed(self, *_args):
        try:
            if getattr(self, "_updating_bsdd_dictionaries", False):
                return
            self._save_bsdd_dictionary_state()
            self._schedule_bsdd_search()
        except Exception as err:
            _print_ui_error("_on_bsdd_dictionary_item_changed", err)

    def _on_bsdd_dictionaries_ready(self, payload):
        try:
            if isinstance(payload, dict):
                dictionaries = payload.get("dictionaries") or payload.get("results") or []
            else:
                dictionaries = payload or []
            QtCore, QtGui, QtWidgets = _get_qt_modules()
            del QtGui
            self._updating_bsdd_dictionaries = True
            self.form.bsddDictionaryList.blockSignals(True)
            self.form.bsddDictionaryList.clear()
            restored = set(self._restored_bsdd_dictionary_uris or [])
            self._bsdd_dictionary_uris = []
            for dictionary in dictionaries:
                name = dictionary.get("name", "<unnamed>")
                uri = dictionary.get("uri", "")
                status = dictionary.get("status", "")
                text = name if not status else "{} ({})".format(name, status)
                self._bsdd_dictionary_labels[uri] = name
                item = QtWidgets.QListWidgetItem(text)
                item.setFlags(item.flags() | QtCore.Qt.ItemIsUserCheckable)
                item.setData(QtCore.Qt.UserRole, uri)
                item.setToolTip(uri)
                is_checked = uri in restored if self._has_saved_bsdd_dictionary_state else False
                item.setCheckState(QtCore.Qt.Checked if is_checked else QtCore.Qt.Unchecked)
                self.form.bsddDictionaryList.addItem(item)
                self._bsdd_dictionary_uris.append(uri)
            self.form.bsddDictionaryList.blockSignals(False)
            self._updating_bsdd_dictionaries = False
            if self._has_saved_bsdd_dictionary_state:
                self._save_bsdd_dictionary_state()
            if self._is_bsdd_provider_active():
                self._schedule_bsdd_search()
        except Exception as err:
            self._updating_bsdd_dictionaries = False
            try:
                self.form.bsddDictionaryList.blockSignals(False)
            except Exception:
                pass
            _print_ui_error("_on_bsdd_dictionaries_ready", err)

    def _on_bsdd_search_ready(self, search_key, payload):
        try:
            if not self._is_bsdd_provider_active():
                return
            request_info = self._bsdd_pending_requests.pop(tuple(search_key), None)
            if not request_info:
                return
            batch_id, dictionary_uri = request_info
            if batch_id != self._bsdd_search_batch_id:
                return
            if isinstance(payload, dict):
                classes = payload.get("classes") or payload.get("results") or []
            else:
                classes = payload or []
            if (not classes) and self._bsdd_dictionary_candidates.get(dictionary_uri):
                self._dispatch_bsdd_search_candidate(dictionary_uri)
                return
            if not classes:
                self._finalize_bsdd_dictionary_branch(dictionary_uri)
                self._finalize_bsdd_search_batch_if_empty()
                return
            self._append_bsdd_dictionary_results(dictionary_uri, classes)
        except Exception as err:
            _print_ui_error("_on_bsdd_search_ready", err)

    def _on_bsdd_result_changed(self, current, _previous):
        try:
            QtCore, _QtGui, _QtWidgets = _get_qt_modules()
            self._bsdd_selected_concept = None
            self._bsdd_contract_detail_payload = None
            self._bsdd_contract = None
            concept = current.data(QtCore.Qt.UserRole)
            if not concept:
                self._populate_bsdd_properties(None)
                return
            self._bsdd_selected_concept = concept
            self._update_bsdd_contract()
            self._populate_bsdd_properties(concept)
            concept_uri = concept.get("uri", "")
            if concept_uri:
                self._bsdd_client.fetch_concept(concept_uri)
        except Exception as err:
            _print_ui_error("_on_bsdd_result_changed", err)

    def _on_bsdd_concept_ready(self, concept_uri, payload):
        try:
            if not self._is_bsdd_provider_active():
                return
            current = self._get_selected_bsdd_concept()
            if current and current.get("uri") == concept_uri:
                self._bsdd_contract_detail_payload = payload
                self._update_bsdd_contract()
                self._populate_bsdd_properties(payload)
        except Exception as err:
            _print_ui_error("_on_bsdd_concept_ready", err)

    def _on_bsdd_request_failed(self, request_kind, message, cache_key):
        try:
            cache_key = tuple(cache_key) if isinstance(cache_key, (list, tuple)) else cache_key
            request_info = self._bsdd_pending_requests.pop(cache_key, None)
            if request_info:
                batch_id, dictionary_uri = request_info
                if batch_id == self._bsdd_search_batch_id:
                    if self._bsdd_dictionary_candidates.get(dictionary_uri):
                        self._dispatch_bsdd_search_candidate(dictionary_uri)
                    else:
                        self._finalize_bsdd_dictionary_branch(
                            dictionary_uri,
                            translate("BIM", "Request failed for this dictionary."),
                        )
                        self._finalize_bsdd_search_batch_if_empty()
            FreeCAD.Console.PrintError(
                "bSDD request failed [{}] {} ({})\n".format(request_kind, message, cache_key)
            )
        except Exception as err:
            _print_ui_error("_on_bsdd_request_failed", err)

    def _populate_bsdd_properties(self, payload):
        try:
            _QtCore, _QtGui, QtWidgets = _get_qt_modules()
            self.form.bsddPropertyTable.setRowCount(0)
            if not payload:
                return
            rows = []
            for key in [
                "name",
                "referenceCode",
                "dictionaryName",
                "classType",
                "uri",
                "description",
            ]:
                value = payload.get(key)
                if value:
                    rows.append((key, value))
            for relation_key in ["relatedIfcEntity", "relatedIfcEntities"]:
                value = payload.get(relation_key)
                if value:
                    rows.append((relation_key, value))
            for property_item in payload.get("classProperties", []):
                label = property_item.get("name", "property")
                pset = property_item.get("propertySet", "")
                value = pset if pset else property_item.get("dataType", "")
                rows.append((label, value))
            self.form.bsddPropertyTable.setRowCount(len(rows))
            for row, (key, value) in enumerate(rows):
                self.form.bsddPropertyTable.setItem(row, 0, QtWidgets.QTableWidgetItem(str(key)))
                self.form.bsddPropertyTable.setItem(row, 1, QtWidgets.QTableWidgetItem(str(value)))
            self.form.bsddPropertyTable.resizeColumnsToContents()
        except Exception as err:
            _print_ui_error("_populate_bsdd_properties", err)

    def _refresh_bsdd_context(self):
        try:
            self._bsdd_context_ifc_class = self._get_active_ifc_context()
            context_text = self._bsdd_context_ifc_class or translate("BIM", "No IFC context")
            if hasattr(self.form, "bsddContextLabel"):
                self.form.bsddContextLabel.setText(
                    translate("BIM", "Current IFC context: {}").format(context_text)
                )
        except Exception as err:
            _print_ui_error("_refresh_bsdd_context", err)

    def _get_active_ifc_context(self):
        try:
            selection = FreeCADGui.Selection.getSelection()
            if not selection:
                return ""
            obj = selection[0]
            for attr_name in ["IfcClass", "IfcType", "IfcRole"]:
                value = getattr(obj, attr_name, "")
                if value and str(value).startswith("Ifc"):
                    return str(value)
            classification = getattr(obj, "Classification", "")
            if classification:
                for token in str(classification).split():
                    if token.startswith("Ifc"):
                        return token
        except Exception as err:
            _print_ui_error("_get_active_ifc_context", err)
        return ""

    def _serialize_bsdd_dictionary_state(self):
        try:
            return "|".join(self._get_active_bsdd_dictionaries())
        except Exception as err:
            _print_ui_error("_serialize_bsdd_dictionary_state", err)
            return ""

    def _save_bsdd_dictionary_state(self):
        try:
            if not FreeCAD.ActiveDocument:
                return
            meta = FreeCAD.ActiveDocument.Meta
            meta[BSDD_DICTIONARY_META_KEY] = self._serialize_bsdd_dictionary_state()
            meta[BSDD_DICTIONARY_META_PRESENT_KEY] = "1"
            FreeCAD.ActiveDocument.Meta = meta
            self._has_saved_bsdd_dictionary_state = True
        except Exception as err:
            _print_ui_error("_save_bsdd_dictionary_state", err)

    def _load_bsdd_dictionary_state(self):
        try:
            if not FreeCAD.ActiveDocument:
                return False, set()
            meta = FreeCAD.ActiveDocument.Meta
            has_saved = BSDD_DICTIONARY_META_PRESENT_KEY in meta or BSDD_DICTIONARY_META_KEY in meta
            value = meta.get(BSDD_DICTIONARY_META_KEY, "")
            if not value:
                return has_saved, set()
            return has_saved, {entry for entry in value.split("|") if entry}
        except Exception as err:
            _print_ui_error("_load_bsdd_dictionary_state", err)
            return False, set()

    def _get_selected_bsdd_concept(self):
        try:
            QtCore, _QtGui, _QtWidgets = _get_qt_modules()
            index = self.form.bsddResultsTree.currentIndex()
            return index.data(QtCore.Qt.UserRole)
        except Exception as err:
            _print_ui_error("_get_selected_bsdd_concept", err)
            return None

    def _update_bsdd_contract(self):
        try:
            concept = self._bsdd_selected_concept
            if not concept:
                self._bsdd_contract = None
                return
            active_object = None
            selection = FreeCADGui.Selection.getSelection()
            if selection:
                active_object = selection[0]
            contract = self._bsdd_contract_module.build_canonical_contract(
                concept, self._bsdd_contract_detail_payload, active_object
            )
            contract["legacy_string"] = (
                self._bsdd_contract_module.build_legacy_classification_string(contract)
            )
            self._bsdd_contract = contract
        except Exception as err:
            self._bsdd_contract = None
            _print_ui_error("_update_bsdd_contract", err)

    def _get_current_bsdd_contract(self):
        try:
            if not self._bsdd_contract:
                self._update_bsdd_contract()
            return self._bsdd_contract
        except Exception as err:
            _print_ui_error("_get_current_bsdd_contract", err)
            return None

    def _store_bsdd_contract_for_item(self, tree_item, contract):
        try:
            QtCore, _QtGui, _QtWidgets = _get_qt_modules()
            tree_item.setData(0, QtCore.Qt.UserRole + BSDD_CONTRACT_ROLE, contract)
            object_name = tree_item.toolTip(0)
            if object_name and contract:
                self._bsdd_object_contracts[object_name] = contract
            elif object_name:
                self._bsdd_object_contracts.pop(object_name, None)
        except Exception as err:
            _print_ui_error("_store_bsdd_contract_for_item", err)

    def _get_stored_bsdd_contract_for_item(self, tree_item):
        try:
            QtCore, _QtGui, _QtWidgets = _get_qt_modules()
            contract = tree_item.data(0, QtCore.Qt.UserRole + BSDD_CONTRACT_ROLE)
            if contract:
                return contract
            object_name = tree_item.toolTip(0)
            if object_name:
                return self._bsdd_object_contracts.get(object_name)
            return None
        except Exception as err:
            _print_ui_error("_get_stored_bsdd_contract_for_item", err)
            return None

    def _restore_bsdd_contract_for_item(self, tree_item, object_name):
        try:
            if not object_name:
                return
            contract = self._bsdd_object_contracts.get(object_name)
            if contract:
                self._store_bsdd_contract_for_item(tree_item, contract)
        except Exception as err:
            _print_ui_error("_restore_bsdd_contract_for_item", err)

    def _apply_bsdd_contract_to_object(self, obj, contract):
        try:
            if not contract:
                return False
            from nativeifc import ifc_classification, ifc_tools

            if self._bsdd_contract_module:
                contract = self._bsdd_contract_module.refresh_contract_validation(contract, obj)
            if not ifc_tools.get_ifcfile(obj) or not ifc_tools.get_ifc_element(obj):
                return True
            ifc_classification.apply_canonical_contract(obj, contract)
            return True
        except Exception as err:
            FreeCAD.Console.PrintError(
                "BIM bSDD semantic mapping error for {}: {}\n".format(obj.Label, err)
            )
            return False

    def _get_selected_class_values(self):
        try:
            if self._is_bsdd_provider_active():
                contract = self._get_current_bsdd_contract()
                if not contract:
                    return None, None, None
                class_metadata = contract.get("class_metadata", {})
                dictionary_metadata = contract.get("dictionary_metadata", {})
                code = class_metadata.get("reference_code") or class_metadata.get("name")
                label = class_metadata.get("name") or code
                prefix = dictionary_metadata.get("name") or "bSDD"
                return code, label, prefix
            if len(self.form.treeClass.selectedItems()) != 1:
                return None, None, None
            item = self.form.treeClass.selectedItems()[0]
            code = item.text(0)
            label = item.toolTip(0) or item.text(0)
            prefix = self.form.comboSystem.currentText()
            return code, label, prefix
        except Exception as err:
            _print_ui_error("_get_selected_class_values", err)
            return None, None, None

    def addSelection(self, document, object, element, position):
        try:
            del document, object, element, position
            self._refresh_bsdd_context()
            if self._is_bsdd_provider_active():
                self._schedule_bsdd_search()
        except Exception as err:
            _print_ui_error("addSelection", err)

    def removeSelection(self, document, object, element, position=None):
        try:
            del document, object, element, position
            self._refresh_bsdd_context()
            if self._is_bsdd_provider_active():
                self._schedule_bsdd_search()
        except Exception as err:
            _print_ui_error("removeSelection", err)

    def setSelection(self, document):
        try:
            del document
            self._refresh_bsdd_context()
            if self._is_bsdd_provider_active():
                self._schedule_bsdd_search()
        except Exception as err:
            _print_ui_error("setSelection", err)

    def clearSelection(self, document):
        try:
            del document
            self._refresh_bsdd_context()
            if self._is_bsdd_provider_active():
                self._schedule_bsdd_search()
        except Exception as err:
            _print_ui_error("clearSelection", err)

    def updateObjects(self, idx=None):
        # store current state of tree into self.objectslist before redrawing

        for row in range(self.form.treeObjects.topLevelItemCount()):
            child = self.form.treeObjects.topLevelItem(row)
            if child.toolTip(0):
                if child.toolTip(0) in self.objectslist:
                    self.objectslist[child.toolTip(0)] = child.text(1)
                elif child.toolTip(0) in self.matlist:
                    self.matlist[child.toolTip(0)] = child.text(1)
                self.labellist[child.toolTip(0)] = child.text(0)
                contract = self._get_stored_bsdd_contract_for_item(child)
                if contract:
                    self._bsdd_object_contracts[child.toolTip(0)] = contract
            for childrow in range(child.childCount()):
                grandchild = child.child(childrow)
                if grandchild.toolTip(0):
                    if grandchild.toolTip(0) in self.objectslist:
                        self.objectslist[grandchild.toolTip(0)] = grandchild.text(1)
                    elif grandchild.toolTip(0) in self.matlist:
                        self.matlist[grandchild.toolTip(0)] = grandchild.text(1)
                    self.labellist[grandchild.toolTip(0)] = grandchild.text(0)
                    contract = self._get_stored_bsdd_contract_for_item(grandchild)
                    if contract:
                        self._bsdd_object_contracts[grandchild.toolTip(0)] = contract

        self.form.treeObjects.clear()

        if self.form.groupMode.currentIndex() == 1:
            # group by type
            self.updateByType()
        elif self.form.groupMode.currentIndex() == 2:
            # group by material
            self.updateByMaterial()
        elif self.form.groupMode.currentIndex() == 3:
            # group by model structure
            self.updateByTree()
        else:
            # group alphabetically
            self.updateDefault()

        # resize columns - no resizeSection in pyside2
        # self.form.treeObjects.header().resizeSection(0,int(self.form.treeObjects.width()/2))
        # self.form.treeObjects.header().resizeSection(1,int(self.form.treeObjects.width()/2))

    def updateByType(self):
        from PySide import QtCore, QtGui
        import Draft

        groups = {}
        for name in self.objectslist.keys():
            obj = FreeCAD.ActiveDocument.getObject(name)
            if obj and hasattr(obj, "IfcType"):
                groups.setdefault(obj.IfcType, []).append(name)
            elif obj and hasattr(obj, "IfcRole"):
                groups.setdefault(obj.IfcRole, []).append(name)
            else:
                groups.setdefault("Undefined", []).append(name)
        groups["Materials"] = self.matlist.keys()
        d = self.objectslist.copy()
        d.update(self.matlist)

        for group in groups.keys():
            mit = QtGui.QTreeWidgetItem([group, ""])
            self.form.treeObjects.addTopLevelItem(mit)
            for name in groups[group]:
                obj = FreeCAD.ActiveDocument.getObject(name)
                if obj:
                    if (
                        (not self.form.onlyVisible.isChecked())
                        or obj.ViewObject.isVisible()
                        or (Draft.getType(obj) in ["Material", "MultiMaterial"])
                    ):
                        it = QtGui.QTreeWidgetItem([self.labellist[name], d[name]])
                        it.setIcon(0, self.getIcon(obj))
                        it.setToolTip(0, name)
                        self._restore_bsdd_contract_for_item(it, name)
                        mit.addChild(it)
            mit.sortChildren(0, QtCore.Qt.AscendingOrder)
        self.form.treeObjects.expandAll()
        # self.spanTopLevels()

    def updateByMaterial(self):
        from PySide import QtCore, QtGui

        groups = {}
        claimed = []
        for name in self.matlist.keys():
            mat = FreeCAD.ActiveDocument.getObject(name)
            if mat:
                children = [par.Name for par in mat.InList if par.Name in self.objectslist.keys()]
                groups[name] = children
                claimed.extend(children)
        groups["Undefined"] = [o for o in self.objectslist.keys() if not o in claimed]

        for group in groups.keys():
            matobj = FreeCAD.ActiveDocument.getObject(group)
            if matobj:
                mit = QtGui.QTreeWidgetItem([self.labellist[group], self.matlist[group]])
                mit.setIcon(0, self.getIcon(matobj))
                mit.setToolTip(0, group)
            else:
                mit = QtGui.QTreeWidgetItem(["Undefined", ""])
            self.form.treeObjects.addTopLevelItem(mit)
            for name in groups[group]:
                obj = FreeCAD.ActiveDocument.getObject(name)
                if obj:
                    if (not self.form.onlyVisible.isChecked()) or obj.ViewObject.isVisible():
                        it = QtGui.QTreeWidgetItem([self.labellist[name], self.objectslist[name]])
                        it.setIcon(0, self.getIcon(obj))
                        it.setToolTip(0, name)
                        self._restore_bsdd_contract_for_item(it, name)
                        mit.addChild(it)
            mit.sortChildren(0, QtCore.Qt.AscendingOrder)
        self.form.treeObjects.expandAll()
        # self.spanTopLevels()

    def updateByTree(self):
        from PySide import QtGui

        # order by hierarchy
        def istop(obj):
            for parent in obj.InList:
                if parent.Name in self.objectslist.keys():
                    return False
            return True

        rel = []
        deps = []
        for name in self.objectslist.keys():
            obj = FreeCAD.ActiveDocument.getObject(name)
            if obj:
                if istop(obj):
                    rel.append(obj)
                else:
                    deps.append(obj)
        pa = 1
        while deps:
            for obj in rel:
                for child in obj.OutList:
                    if child in deps:
                        rel.append(child)
                        deps.remove(child)
            pa += 1
            if pa == 10:  # max 10 hierarchy levels, okay? Let's keep civilised
                rel.extend(deps)
                break

        done = {}
        # materials first
        mit = QtGui.QTreeWidgetItem(["Materials", ""])
        self.form.treeObjects.addTopLevelItem(mit)
        for name, code in self.matlist.items():
            obj = FreeCAD.ActiveDocument.getObject(name)
            if obj:
                it = QtGui.QTreeWidgetItem([self.labellist[name], code])
                it.setIcon(0, self.getIcon(obj))
                it.setToolTip(0, name)
                self._restore_bsdd_contract_for_item(it, name)
                mit.addChild(it)
        # objects next
        for obj in rel:
            code = self.objectslist[obj.Name]
            if (not self.form.onlyVisible.isChecked()) or obj.ViewObject.isVisible():
                it = QtGui.QTreeWidgetItem([self.labellist[obj.Name], code])
                it.setIcon(0, self.getIcon(obj))
                it.setToolTip(0, name)
                self._restore_bsdd_contract_for_item(it, obj.Name)
                ok = False
                for par in obj.InListRecursive:
                    if par.Name in done:
                        if (not hasattr(par, "Hosts")) or (obj not in par.Hosts):
                            done[par.Name].addChild(it)
                            done[obj.Name] = it
                            ok = True
                            break
                if not ok:
                    self.form.treeObjects.addTopLevelItem(it)
                    done[obj.Name] = it
        self.form.treeObjects.expandAll()

    def updateDefault(self):
        from PySide import QtGui
        import Draft

        d = self.objectslist.copy()
        d.update(self.matlist)
        for name, code in d.items():
            obj = FreeCAD.ActiveDocument.getObject(name)
            if obj:
                if (
                    (not self.form.onlyVisible.isChecked())
                    or obj.ViewObject.isVisible()
                    or (Draft.getType(obj) in ["Material", "MultiMaterial"])
                ):
                    it = QtGui.QTreeWidgetItem([self.labellist[name], code])
                    it.setIcon(0, self.getIcon(obj))
                    it.setToolTip(0, name)
                    self._restore_bsdd_contract_for_item(it, name)
                    self.form.treeObjects.addTopLevelItem(it)
                    if obj in FreeCADGui.Selection.getSelection():
                        self.form.treeObjects.setCurrentItem(it)

    def updateClasses(self, search=""):
        from PySide import QtGui

        self.form.treeClass.clear()

        # save as default
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").SetString(
            "DefaultClassificationSystem", self.form.comboSystem.currentText()
        )

        if isinstance(search, int):
            search = ""
        if self.form.search.text():
            search = self.form.search.text()
        if search:
            search = search.lower()

        system = self.form.comboSystem.currentText()
        if not system:
            return

        if not system in self.Classes:
            self.Classes[system] = self.build(system)
        if not self.Classes[system]:
            return

        for c in self.Classes[system]:
            it = None
            first = True
            if not c[1]:
                c[1] = ""
            if search:
                if (search in c[0].lower()) or (search in c[1].lower()):
                    it = QtGui.QTreeWidgetItem([c[0] + " " + c[1]])
                    it.setToolTip(0, c[1])
                    self.form.treeClass.addTopLevelItem(it)
            else:
                it = QtGui.QTreeWidgetItem([c[0] + " " + c[1]])
                it.setToolTip(0, c[1])
                self.form.treeClass.addTopLevelItem(it)
            if c[2]:
                self.addChildren(c[2], it, search)
            if it and first:
                # select first entry
                self.form.treeClass.setCurrentItem(it)
                first = False

    def addChildren(self, children, parent, search=""):
        from PySide import QtGui

        if children:
            for c in children:
                it = None
                if not c[1]:
                    c[1] = ""
                if search:
                    if (search in c[0].lower()) or (search in c[1].lower()):
                        it = QtGui.QTreeWidgetItem([c[0] + " " + c[1]])
                        it.setToolTip(0, c[1])
                        self.form.treeClass.addTopLevelItem(it)
                else:
                    it = QtGui.QTreeWidgetItem([c[0] + " " + c[1]])
                    it.setToolTip(0, c[1])
                    if parent:
                        parent.addChild(it)
                if c[2]:
                    self.addChildren(c[2], it, search)

    def build(self, system):
        # try to load the IFC first
        preset = os.path.join(FreeCAD.getUserAppDataDir(), "BIM", "Classification", system + ".ifc")
        if os.path.exists(preset):
            return self.build_ifc(system)
        else:
            preset = os.path.join(
                FreeCAD.getUserAppDataDir(), "BIM", "Classification", system + ".xml"
            )
            if os.path.exists(preset):
                return self.build_xml(system)
            else:
                FreeCAD.Console.PrintError("Unable to find classification file:" + system + "\n")
                return []

    def build_ifc(self, system):
        # builds from ifc instead of xml

        class Item:
            def __init__(self, parent=None):
                self.parent = parent
                self.ID = None
                self.Name = None
                self.children = []

        preset = os.path.join(FreeCAD.getUserAppDataDir(), "BIM", "Classification", system + ".ifc")
        if not os.path.exists(preset):
            return None
        import ifcopenshell

        f = ifcopenshell.open(preset)
        classes = f.by_type("IfcClassificationReference")
        rootclass = f.by_type("IfcClassification")
        if rootclass:
            rootclass = rootclass[0]
        else:
            return None
        root = Item()
        classdict = {rootclass.id(): root}
        for cl in classes:
            currentItem = Item()
            currentItem.Name = cl.Name
            currentItem.Description = cl.Description
            currentItem.ID = cl.Identification
            if cl.ReferencedSource:
                if cl.ReferencedSource.id() in classdict:
                    currentItem.parent = classdict[cl.ReferencedSource.id()]
                    classdict[cl.ReferencedSource.id()].children.append(currentItem)
            classdict[cl.id()] = currentItem
        return [self.listize(c) for c in root.children]

    def build_xml(self, system):
        class Item:
            def __init__(self, parent=None):
                self.parent = parent
                self.ID = None
                self.Name = None
                self.children = []

        preset = os.path.join(FreeCAD.getUserAppDataDir(), "BIM", "Classification", system + ".xml")
        if not os.path.exists(preset):
            return None
        import codecs
        import re

        d = Item()
        with codecs.open(preset, "r", "utf-8") as f:
            currentItem = d
            for l in f:
                if "<Item>" in l:
                    currentItem = Item(currentItem)
                    currentItem.parent.children.append(currentItem)
                if "</Item>" in l:
                    currentItem = currentItem.parent
                elif currentItem and re.findall(r"<ID>(.*?)</ID>", l):
                    currentItem.ID = re.findall(r"<ID>(.*?)</ID>", l)[0]
                elif currentItem and re.findall(r"<Name>(.*?)</Name>", l):
                    currentItem.Name = re.findall(r"<Name>(.*?)</Name>", l)[0]
                elif (
                    currentItem
                    and re.findall(r"<Description>(.*?)</Description>", l)
                    and not currentItem.Name
                ):
                    currentItem.Name = re.findall("<Description>(.*?)</Description>", l)[0]
        return [self.listize(c) for c in d.children]

    def listize(self, item):
        return [item.ID, item.Name, [self.listize(it) for it in item.children]]

    def apply(self, item=None, col=None):
        try:
            del item, col
            if self.form.treeObjects.selectedItems():
                code, _label, prefix = self._get_selected_class_values()
                if not code:
                    return
                if self.form.checkPrefix.isChecked() and prefix:
                    code = prefix + " " + code
                contract = (
                    self._get_current_bsdd_contract() if self._is_bsdd_provider_active() else None
                )
                for m in self.form.treeObjects.selectedItems():
                    if m.toolTip(0):
                        m.setText(1, code)
                        self._store_bsdd_contract_for_item(m, contract)
        except Exception as err:
            _print_ui_error("apply", err)

    def rename(self):
        try:
            if self.form.treeObjects.selectedItems():
                _code, label, _prefix = self._get_selected_class_values()
                if not label:
                    return
                for m in self.form.treeObjects.selectedItems():
                    if m.toolTip(0):
                        m.setText(0, label)
        except Exception as err:
            _print_ui_error("rename", err)

    def accept(self):
        if not self.isEditing:
            changed = False
            for row in range(self.form.treeObjects.topLevelItemCount()):
                child = self.form.treeObjects.topLevelItem(row)
                items = [child]
                items.extend([child.child(childrow) for childrow in range(child.childCount())])
                for item in items:
                    code = item.text(1)
                    label = item.text(0)
                    if item.toolTip(0):
                        obj = FreeCAD.ActiveDocument.getObject(item.toolTip(0))
                        if obj:
                            contract = self._get_stored_bsdd_contract_for_item(item)
                            if contract:
                                if not changed:
                                    FreeCAD.ActiveDocument.openTransaction("Change standard codes")
                                    changed = True
                                if not self._apply_bsdd_contract_to_object(obj, contract):
                                    continue
                            if hasattr(obj, "StandardCode"):
                                if code != obj.StandardCode:
                                    if not changed:
                                        FreeCAD.ActiveDocument.openTransaction(
                                            "Change standard codes"
                                        )
                                        changed = True
                                    obj.StandardCode = code
                            elif hasattr(obj, "IfcClass"):
                                if not "Classification" in obj.PropertiesList:
                                    obj.addProperty(
                                        "App::PropertyString", "Classification", "IFC", locked=True
                                    )
                                if code != obj.Classification:
                                    if not changed:
                                        FreeCAD.ActiveDocument.openTransaction(
                                            "Change standard codes"
                                        )
                                        changed = True
                                    obj.Classification = code
                            if label != obj.Label:
                                if not changed:
                                    FreeCAD.ActiveDocument.openTransaction("Change standard codes")
                                    changed = True
                                obj.Label = label
            if changed:
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()
        else:
            # Close the form if user has pressed Enter and did not
            # select anything
            code, _label, prefix = self._get_selected_class_values()
            if not code:
                return self.reject()
            pl = self.isEditing.PropertiesList
            if ("StandardCode" in pl) or ("IfcClass" in pl):
                FreeCAD.ActiveDocument.openTransaction("Change standard codes")
                if self._is_bsdd_provider_active():
                    if not self._apply_bsdd_contract_to_object(
                        self.isEditing, self._get_current_bsdd_contract()
                    ):
                        try:
                            FreeCAD.ActiveDocument.abortTransaction()
                        except Exception:
                            pass
                        return self.reject()
                if self.form.checkPrefix.isChecked() and prefix:
                    code = prefix + " " + code
                if "StandardCode" in pl:
                    self.isEditing.StandardCode = code
                else:
                    if not "Classification" in self.isEditing.PropertiesList:
                        self.isEditing.addProperty(
                            "App::PropertyString", "Classification", "IFC", locked=True
                        )
                    self.isEditing.Classification = code
                if hasattr(self.isEditing.ViewObject, "Proxy") and hasattr(
                    self.isEditing.ViewObject.Proxy, "setTaskValue"
                ):
                    self.isEditing.ViewObject.Proxy.setTaskValue("FieldCode", code)
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")
        p.SetInt("BimClassificationDialogWidth", self.form.width())
        p.SetInt("BimClassificationDialogHeight", self.form.height())
        return self.reject()

    def reject(self):
        try:
            if hasattr(self, "_search_timer") and self._search_timer.isActive():
                self._search_timer.stop()
            self._disconnect_bsdd_signals()
            self._remove_selection_observer()
        except Exception as err:
            _print_ui_error("reject", err)
        self.form.hide()
        del self.form
        return True

    def onUpArrow(self):
        try:
            if self.form and (not self._is_bsdd_provider_active()):
                i = self.form.treeClass.currentItem()
                if self.form.treeClass.itemAbove(i):
                    self.form.treeClass.setCurrentItem(self.form.treeClass.itemAbove(i))
        except Exception as err:
            _print_ui_error("onUpArrow", err)

    def onDownArrow(self):
        try:
            if self.form and (not self._is_bsdd_provider_active()):
                i = self.form.treeClass.currentItem()
                if self.form.treeClass.itemBelow(i):
                    self.form.treeClass.setCurrentItem(self.form.treeClass.itemBelow(i))
        except Exception as err:
            _print_ui_error("onDownArrow", err)

    def onVisible(self, index):
        PARAMS.SetInt("BimClassificationVisibleState", getattr(index, "value", index))
        self.updateObjects()

    def onPrefix(self, index):
        PARAMS.SetInt("BimClassificationSystemNamePrefix", getattr(index, "value", index))

    def getIcon(self, obj):
        """returns a QIcon for an object"""

        from PySide import QtGui
        import Arch_rc

        if hasattr(obj.ViewObject, "Icon"):
            return obj.ViewObject.Icon
        elif hasattr(obj.ViewObject, "Proxy") and hasattr(obj.ViewObject.Proxy, "getIcon"):
            icon = obj.ViewObject.Proxy.getIcon()
            if icon.startswith("/*"):
                return QtGui.QIcon(QtGui.QPixmap(icon))
            else:
                return QtGui.QIcon(icon)
        else:
            return QtGui.QIcon(":/icons/Arch_Component.svg")


FreeCADGui.addCommand("BIM_Classification", BIM_Classification())
