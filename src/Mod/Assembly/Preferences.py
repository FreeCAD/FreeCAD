# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

import FreeCAD
import FreeCADGui

translate = FreeCAD.Qt.translate


def preferences():
    return FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Assembly")


class PreferencesPage:
    def __init__(self, parent=None):
        self.form = FreeCADGui.PySideUic.loadUi(":preferences/Assembly.ui")

    def saveSettings(self):
        pref = preferences()
        pref.SetBool("LeaveEditWithEscape", self.form.checkBoxEnableEscape.isChecked())
        pref.SetBool("LogSolverDebug", self.form.checkBoxSolverDebug.isChecked())
        pref.SetInt("GroundFirstPart", self.form.groundFirstPart.currentIndex())

        # Drag target sphere radius (always present)
        pref.SetFloat(
            "DragTargetSphereRadius",
            self.form.spinDragSphereRadius.value(),
        )

        oldSolver = pref.GetString("SolverBackend", "Ondsel")
        newSolver = self.form.solverBackend.currentText()
        pref.SetString("SolverBackend", newSolver)

        # ChronoSolver tuning parameters
        chronoPref = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/Assembly/ChronoSolver"
        )
        chronoPref.SetFloat(
            "MouseConstraintCompliance",
            self.form.spinMouseCompliance.value(),
        )
        chronoPref.SetFloat(
            "MaxRotationPerStep",
            self.form.spinMaxRotation.value(),
        )
        chronoPref.SetFloat(
            "MaxTranslationPerStep",
            self.form.spinMaxTranslation.value(),
        )
        chronoPref.SetFloat(
            "JointViolationTolerance",
            self.form.spinJointViolation.value(),
        )
        chronoPref.SetFloat(
            "RotationLimitTolerance",
            self.form.spinRotationLimit.value(),
        )
        chronoPref.SetFloat(
            "TranslationLimitTolerance",
            self.form.spinTranslationLimit.value(),
        )

        # If the solver changed, reset all open assemblies to use the new solver
        if newSolver != oldSolver:
            for doc in FreeCAD.listDocuments().values():
                for obj in doc.Objects:
                    if obj.isDerivedFrom("Assembly::AssemblyObject"):
                        obj.resetSolver()
                        obj.solve()

    def loadSettings(self):
        pref = preferences()
        self.form.checkBoxEnableEscape.setChecked(pref.GetBool("LeaveEditWithEscape", True))
        self.form.checkBoxSolverDebug.setChecked(pref.GetBool("LogSolverDebug", False))
        self.form.groundFirstPart.clear()
        self.form.groundFirstPart.addItem(translate("Assembly", "Ask"))
        self.form.groundFirstPart.addItem(translate("Assembly", "Always"))
        self.form.groundFirstPart.addItem(translate("Assembly", "Never"))
        self.form.groundFirstPart.setCurrentIndex(pref.GetInt("GroundFirstPart", 0))

        # Drag target sphere radius
        self.form.spinDragSphereRadius.setValue(pref.GetFloat("DragTargetSphereRadius", 5.0))

        # Populate solver backend combo from the registry
        current = pref.GetString("SolverBackend", "Ondsel")
        try:
            import AssemblyApp

            solvers = AssemblyApp.getAvailableSolvers()
        except ImportError:
            # Module not yet loaded (preferences opened before workbench activation)
            solvers = [current] if current else ["Ondsel"]
        self.form.solverBackend.clear()
        for name in solvers:
            self.form.solverBackend.addItem(name)
        idx = solvers.index(current) if current in solvers else 0
        self.form.solverBackend.setCurrentIndex(idx)

        # ChronoSolver tuning
        chronoPref = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/Assembly/ChronoSolver"
        )
        self.form.spinMouseCompliance.setValue(
            chronoPref.GetFloat("MouseConstraintCompliance", 1e-4)
        )
        self.form.spinMaxRotation.setValue(chronoPref.GetFloat("MaxRotationPerStep", 0.05))
        self.form.spinMaxTranslation.setValue(chronoPref.GetFloat("MaxTranslationPerStep", 20.0))
        self.form.spinJointViolation.setValue(chronoPref.GetFloat("JointViolationTolerance", 1.0))
        self.form.spinRotationLimit.setValue(chronoPref.GetFloat("RotationLimitTolerance", 0.5))
        self.form.spinTranslationLimit.setValue(
            chronoPref.GetFloat("TranslationLimitTolerance", 50.0)
        )

        # Only show ChronoSolver group when Chrono is selected
        self.form.solverBackend.currentTextChanged.connect(self._updateChronoVisibility)
        self._updateChronoVisibility(self.form.solverBackend.currentText())

    def _updateChronoVisibility(self, solverName):
        self.form.groupChronoSolver.setVisible(solverName == "Chrono")
