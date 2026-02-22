# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

fc_copy_to_mod_path("TemplatePyMod"
    InitGui.py
    Init.py
    Commands.py
    PythonQt.py
    Tests.py
    DocumentObject.py
)

INSTALL(
    FILES
        Init.py
        InitGui.py
        Commands.py
        FeaturePython.py
        PythonQt.py
        Tests.py
    DESTINATION
        Mod/TemplatePyMod
)
