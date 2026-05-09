# SPDX-License-Identifier: LGPL-2.1-or-later

import os

import Arch
from bimtests.TestArchBaseGui import TestArchBaseGui


class TestArchReferenceGui(TestArchBaseGui):

    def test_lightweight_reference_restores_view_provider_when_guidocument_is_missing(self):
        reference = Arch.makeReference(name="HeadlessRestoreReference")
        reference.ReferenceMode = "Lightweight"
        self.document.recompute()

        archive = None
        try:
            archive, _, restored = self.reopen_without_gui_document(reference)

            self.assertIsNotNone(restored)
            self.assertIsNotNone(restored.ViewObject)
            self.assertIsNotNone(restored.ViewObject.Proxy)
            self.assertEqual(type(restored.ViewObject.Proxy).__name__, "ViewProviderArchReference")
            self.assertTrue(restored.ViewObject.Visibility)
            self.assertIn("TimeStamp", restored.ViewObject.PropertiesList)
            self.assertIn("UpdateColors", restored.ViewObject.PropertiesList)
            self.assertEqual(restored.ReferenceMode, "Lightweight")
            self.assertFalse(restored.Proxy.reload)
        finally:
            if archive is not None:
                os.unlink(archive)
