# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2026 FreeCAD Project Association                       *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it   *
# *   under the terms of the GNU Lesser General Public License as published *
# *   by the Free Software Foundation, either version 2 of the             *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# ***************************************************************************

from types import SimpleNamespace
import unittest

import FreeCAD as App
from draftguitools import gui_snapper
from draftguitools import gui_viewpolicy


class _FakeGrid:
    def __init__(self):
        self.runtime_config = {}
        self.default_show_always = False
        self.default_show_during_command = False
        self.show_always = False
        self.show_during_command = False
        self.Visible = False
        self.set_count = 0
        self.off_count = 0

    def setRuntimeConfig(self, config=None):
        self.runtime_config = dict(config or {})

    def clearRuntimeConfig(self):
        self.runtime_config = {}

    def restoreDefaultVisibility(self):
        self.show_always = self.default_show_always
        self.show_during_command = self.default_show_during_command

    def set(self):
        self.set_count += 1
        self.Visible = True

    def off(self):
        self.off_count += 1
        self.Visible = False


class DraftViewPolicyGui(unittest.TestCase):
    def test_all_tracked_grids_are_resolved_and_synchronized(self):
        registry = gui_viewpolicy.DraftViewPolicyRegistry()
        metric_document = SimpleNamespace(Context="BIM", UnitSystem=App.Units.Scheme.MKS)
        imperial_document = SimpleNamespace(
            Context="BIM", UnitSystem=App.Units.Scheme.ImperialBuilding
        )
        registry.register_context_policy(
            "BIM",
            "test",
            lambda document: gui_viewpolicy.DraftViewPolicy(
                spacing=(100 if document.UnitSystem == App.Units.Scheme.MKS else 25.4),
                visibility=gui_viewpolicy.GridVisibility.ALWAYS,
            ),
        )

        view_metric = object()
        view_imperial = object()
        grid_metric = _FakeGrid()
        grid_imperial = _FakeGrid()
        snapper = object.__new__(gui_snapper.Snapper)
        snapper.trackers = [[view_metric, view_imperial], [grid_metric, grid_imperial]]
        snapper._viewDocuments = {
            id(view_metric): metric_document,
            id(view_imperial): imperial_document,
        }

        old_registry = gui_snapper.view_policy.registry
        gui_snapper.view_policy.registry = registry
        try:
            observer = gui_snapper.GridDocumentObserver(snapper)
            snapper._applyGridProfiles()
            self.assertEqual(grid_metric.runtime_config["space"], 100)
            self.assertEqual(grid_imperial.runtime_config["space"], 25.4)
            self.assertEqual(grid_metric.set_count, 1)
            self.assertEqual(grid_imperial.set_count, 1)

            imperial_document.UnitSystem = App.Units.Scheme.MKS
            observer.slotChangedDocument(imperial_document, "UnitSystem")
            self.assertEqual(grid_imperial.runtime_config["space"], 100)
            self.assertEqual(grid_imperial.set_count, 2)

            metric_document.Context = ""
            observer.slotChangedDocument(metric_document, "Context")
            self.assertEqual(grid_metric.runtime_config, {})
            self.assertEqual(grid_metric.off_count, 1)
        finally:
            gui_snapper.view_policy.registry = old_registry
