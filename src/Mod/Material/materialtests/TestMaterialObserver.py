# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Joao Matos
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""
Test module for FreeCAD material observers
"""

import gc
import shutil
import tempfile
import unittest
import weakref

import FreeCAD
import Materials


class _RecordingObserver:
    def __init__(self):
        self.created = []
        self.changed = []
        self.deleted = []

    def slotCreatedMaterial(self, material):
        self.created.append(material)

    def slotChangedMaterial(self, material):
        self.changed.append(material)

    def slotDeletedMaterial(self, material):
        self.deleted.append(material)


class MaterialObserverTestCases(unittest.TestCase):
    """
    Test class for FreeCAD material observer registration and callbacks
    """

    def setUp(self):
        self.MaterialManager = Materials.MaterialManager()
        self.observer = _RecordingObserver()
        self._observer_registrations = 0
        self._resource_param = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/Material/Resources"
        )
        self._external_param = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface"
        )
        self._saved_resource_prefs = {
            "CustomMaterialsDir": self._resource_param.GetString("CustomMaterialsDir", ""),
            "UseBuiltInMaterials": self._resource_param.GetBool("UseBuiltInMaterials", True),
            "UseMaterialsFromWorkbenches": self._resource_param.GetBool(
                "UseMaterialsFromWorkbenches", True
            ),
            "UseMaterialsFromConfigDir": self._resource_param.GetBool(
                "UseMaterialsFromConfigDir", True
            ),
            "UseMaterialsFromCustomDir": self._resource_param.GetBool(
                "UseMaterialsFromCustomDir", False
            ),
        }
        self._saved_external_prefs = {
            "UseExternal": self._external_param.GetBool("UseExternal", False),
        }
        self._custom_dir = tempfile.mkdtemp(prefix="freecad-material-observer-")
        self._resource_param.SetString("CustomMaterialsDir", self._custom_dir)
        self._resource_param.SetBool("UseBuiltInMaterials", False)
        self._resource_param.SetBool("UseMaterialsFromWorkbenches", False)
        self._resource_param.SetBool("UseMaterialsFromConfigDir", False)
        self._resource_param.SetBool("UseMaterialsFromCustomDir", True)
        self._external_param.SetBool("UseExternal", False)
        self.MaterialManager.refresh()
        self._library_name = "Custom"

    def tearDown(self):
        while self._observer_registrations > 0:
            Materials.removeMaterialObserver(self.observer)
            self._observer_registrations -= 1

        shutil.rmtree(self._custom_dir, ignore_errors=True)
        self._restore_preferences()

    def _register_observer(self):
        Materials.addMaterialObserver(observer=self.observer)
        self._observer_registrations += 1

    def _remove_observer(self):
        if self._observer_registrations > 0:
            Materials.removeMaterialObserver(observer=self.observer)
            self._observer_registrations -= 1

    def _restore_preferences(self):
        for key, value in self._saved_resource_prefs.items():
            if key == "CustomMaterialsDir":
                self._resource_param.SetString(key, value)
            else:
                self._resource_param.SetBool(key, value)
        for key, value in self._saved_external_prefs.items():
            self._external_param.SetBool(key, value)
        self.MaterialManager.refresh()

    def testObserverReceivesCreateAndChange(self):
        material = Materials.Material()
        material.Description = "created"
        created_uuid = material.UUID

        self._register_observer()

        self.MaterialManager.save(
            self._library_name,
            material,
            "Example/ObserverCard.FCMat",
            overwrite=True,
        )
        self.assertEqual(len(self.observer.created), 1)
        self.assertEqual(self.observer.created[0].UUID, created_uuid)
        self.assertEqual(self.observer.created[0].Name, "ObserverCard")
        self.assertEqual(len(self.observer.changed), 0)
        self.assertEqual(len(self.observer.deleted), 0)

        material.Description = "changed"
        self.MaterialManager.save(
            self._library_name,
            material,
            "Example/ObserverCard.FCMat",
            overwrite=True,
        )
        self.assertEqual(len(self.observer.changed), 1)
        self.assertEqual(self.observer.changed[0].UUID, created_uuid)
        self.assertEqual(self.observer.changed[0].Name, "ObserverCard")

        self._remove_observer()

        material.Description = "changed again"
        self.MaterialManager.save(
            self._library_name,
            material,
            "Example/ObserverCard.FCMat",
            overwrite=True,
        )
        self.assertEqual(len(self.observer.changed), 1)

    def testDuplicateRegistrationAndUnknownRemoval(self):
        material = Materials.Material()
        material.Description = "created"
        created_uuid = material.UUID

        stranger = _RecordingObserver()
        Materials.removeMaterialObserver(stranger)

        self._register_observer()
        self._register_observer()

        self.MaterialManager.save(
            self._library_name,
            material,
            "Example/DuplicateObserverCard.FCMat",
            overwrite=True,
        )
        self.assertEqual(len(self.observer.created), 2)
        self.assertTrue(all(event.UUID == created_uuid for event in self.observer.created))

        self._remove_observer()
        material.Description = "changed"
        self.MaterialManager.save(
            self._library_name,
            material,
            "Example/DuplicateObserverCard.FCMat",
            overwrite=True,
        )
        self.assertEqual(len(self.observer.changed), 1)
        self.assertEqual(self.observer.changed[0].UUID, created_uuid)

        self._remove_observer()

        material.Description = "changed again"
        self.MaterialManager.save(
            self._library_name,
            material,
            "Example/DuplicateObserverCard.FCMat",
            overwrite=True,
        )
        self.assertEqual(len(self.observer.changed), 1)

    def testObserverKeepsPythonObjectAliveWhileRegistered(self):
        observer = _RecordingObserver()
        observer_ref = weakref.ref(observer)

        Materials.addMaterialObserver(observer=observer)
        try:
            del observer
            gc.collect()

            registered_observer = observer_ref()
            self.assertIsNotNone(registered_observer)

            material = Materials.Material()
            material.Description = "created"
            self.MaterialManager.save(
                self._library_name,
                material,
                "Example/ObserverCard.FCMat",
                overwrite=True,
            )
            self.assertEqual(len(registered_observer.created), 1)
        finally:
            registered_observer = observer_ref()
            if registered_observer is not None:
                Materials.removeMaterialObserver(observer=registered_observer)
                del registered_observer

        gc.collect()
        self.assertIsNone(observer_ref())
