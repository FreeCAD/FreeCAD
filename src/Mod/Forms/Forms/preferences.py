# SPDX-License-Identifier: LGPL-2.1-or-later
"""Forms interaction preferences; existing appearance is the default."""

from pathlib import Path

import FreeCAD as App


def preferences():
    return App.ParamGet("User parameter:BaseApp/Preferences/Mod/Forms")


class PreferencesPage:
    def __init__(self, parent=None):
        import FreeCADGui as Gui
        self.form = Gui.PySideUic.loadUi(str(Path(__file__).with_name("Preferences.ui")))

    def loadSettings(self):
        pref = preferences()
        self.form.greedySelection.setChecked(pref.GetBool("GreedySelection", False))
        self.form.meshPreview.setChecked(pref.GetBool("MeshPreview", False))

    def saveSettings(self):
        pref = preferences()
        pref.SetBool("GreedySelection", self.form.greedySelection.isChecked())
        pref.SetBool("MeshPreview", self.form.meshPreview.isChecked())
        from .edit import active_form_session
        session = active_form_session()
        if session is not None:
            session.apply_preferences()
