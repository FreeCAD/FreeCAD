# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2023 FreeCAD Project Association                        *
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

""" Contains a parameter observer class and parameter related functions."""

import PySide.QtCore as QtCore
import xml.etree.ElementTree as ET

import FreeCAD as App
import Draft_rc
import Arch_rc

from draftutils import init_draft_statusbar
from draftutils.translate import translate
from draftviewproviders import view_base

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtGui

class ParamObserverDraft:

    def slotParamChanged(self, param_grp, typ, entry, value):
        if entry == "textheight":
            _param_observer_callback_tray()
            return
        if entry in ("gridBorder", "gridShowHuman", "coloredGridAxes", "gridEvery",
                    "gridSpacing", "gridSize", "gridTransparency", "gridColor"):
            _param_observer_callback_grid()
            return
        if entry == "DefaultAnnoScaleMultiplier":
            _param_observer_callback_scalemultiplier(value)
            return
        if entry == "SnapBarShowOnlyDuringCommands":
            _param_observer_callback_snapbar(value)
            return
        if entry == "DisplayStatusbarSnapWidget":
            _param_observer_callback_snapwidget()
            return
        if entry == "DisplayStatusbarScaleWidget":
            _param_observer_callback_scalewidget()
            return
        if entry == "snapStyle":
            _param_observer_callback_snapstyle()
            return
        if entry == "snapcolor":
            _param_observer_callback_snapcolor()
            return
        if entry == "patternFile":
            _param_observer_callback_svg_pattern()
            return


class ParamObserverView:

    def slotParamChanged(self, param_grp, typ, entry, value):
        if entry in ("DefaultShapeColor", "DefaultShapeLineColor", "DefaultShapeLineWidth"):
            _param_observer_callback_tray()
            return


def _param_observer_callback_tray():
    if not hasattr(Gui, "draftToolBar"):
        return
    tray = Gui.draftToolBar.tray
    if tray is None:
        return
    Gui.draftToolBar.setStyleButton()


def _param_observer_callback_scalemultiplier(value):
    value = float(value)  # value is a string
    if value <= 0:
        return
    mw = Gui.getMainWindow()
    sb = mw.statusBar()
    scale_widget = sb.findChild(QtGui.QToolBar,"draft_scale_widget")
    if scale_widget is not None:
        scale_label = init_draft_statusbar.scale_to_label(1 / value)
        scale_widget.scaleLabel.setText(scale_label)


def _param_observer_callback_grid():
    if hasattr(App, "draft_working_planes") and hasattr(Gui, "Snapper"):
        try:
            trackers = Gui.Snapper.trackers
            for wp in App.draft_working_planes[1]:
                view = wp._view
                if view in trackers[0]:
                    i = trackers[0].index(view)
                    grid = trackers[1][i]
                    grid.pts = []
                    grid.reset()
                    grid.displayHumanFigure(wp)
                    grid.setAxesColor(wp)
        except Exception:
            pass


def _param_observer_callback_snapbar(value):
    if Gui.activeWorkbench().name() not in ("DraftWorkbench", "ArchWorkbench", "BIMWorkbench"):
        return
    if hasattr(Gui, "Snapper"):
        toolbar = Gui.Snapper.get_snap_toolbar()
        if toolbar is not None:
            toolbar.setVisible(value == "0")  # value is a string: "0" or "1"


def _param_observer_callback_snapwidget():
    if Gui.activeWorkbench().name() == "DraftWorkbench":
        init_draft_statusbar.hide_draft_statusbar()
        init_draft_statusbar.show_draft_statusbar()


def _param_observer_callback_scalewidget():
    if Gui.activeWorkbench().name() == "DraftWorkbench":
        init_draft_statusbar.hide_draft_statusbar()
        init_draft_statusbar.show_draft_statusbar()


def _param_observer_callback_snapstyle():
    if hasattr(Gui, "Snapper"):
        Gui.Snapper.set_snap_style()


def _param_observer_callback_snapcolor():
    if hasattr(Gui, "Snapper"):
        for snap_track in Gui.Snapper.trackers[2]:
            snap_track.setColor()


def _param_observer_callback_svg_pattern():
    from draftutils import utils  # has to happen here to avoid circular import
    utils.load_svg_patterns()
    if App.ActiveDocument is None:
        return

    pats = list(utils.svg_patterns())
    pats.sort()
    pats = ["None"] + pats

    data = []
    for doc in App.listDocuments().values():
        vobjs = []
        for obj in doc.Objects:
            if hasattr(obj, "ViewObject"):
                vobj = obj.ViewObject
                if hasattr(vobj, "Pattern") \
                        and hasattr(vobj, "Proxy") \
                        and isinstance(vobj.Proxy, view_base.ViewProviderDraft) \
                        and vobj.getEnumerationsOfProperty("Pattern") != pats:
                    vobjs.append(vobj)
        if vobjs:
            data.append([doc, vobjs])
    if not data:
        return

    msg = translate("draft",
"""Do you want to update the SVG pattern options
of existing objects in all opened documents?""")
    res = QtGui.QMessageBox.question(None, "Update SVG patterns", msg,
                                     QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                                     QtGui.QMessageBox.No)
    if res == QtGui.QMessageBox.No:
        return

    for doc, vobjs in data:
        doc.openTransaction("SVG pattern update")
        for vobj in vobjs:
            old = vobj.Pattern
            if old in pats:
                vobj.Pattern = pats
            else:
                tmp_pats = [old] + pats[1:]
                tmp_pats.sort()
                vobj.Pattern = ["None"] + tmp_pats
            vobj.Pattern = old
        doc.commitTransaction()


def _param_observer_start():
    if App.GuiUp:
        _param_observer_start_draft()
        _param_observer_start_view()


def _param_observer_start_draft(param_grp = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")):
    param_grp.AttachManager(ParamObserverDraft())


def _param_observer_start_view(param_grp = App.ParamGet("User parameter:BaseApp/Preferences/View")):
    param_grp.AttachManager(ParamObserverView())


def _param_from_PrefCheckBox(widget):
    value = False
    for elem in list(widget):
        if "name" in elem.keys():
            att_name = elem.attrib["name"]
            if att_name == "checked":  # Can be missing.
                value = elem.find("bool").text == "true"
            elif att_name == "prefEntry":
                entry = elem.find("cstring").text
            elif att_name == "prefPath":
                path = elem.find("cstring").text
    return path, entry, value


def _param_from_PrefRadioButton(widget):
    return _param_from_PrefCheckBox(widget)


def _param_from_PrefComboBox(widget):
    value = 0
    for elem in list(widget):
        if "name" in elem.keys():
            att_name = elem.attrib["name"]
            if att_name == "currentIndex":  # Can be missing.
                value = int(elem.find("number").text)
            elif att_name == "prefEntry":
                entry = elem.find("cstring").text
            elif att_name == "prefPath":
                path = elem.find("cstring").text
    return path, entry, value


def _param_from_PrefSpinBox(widget):
    value = 0
    for elem in list(widget):
        if "name" in elem.keys():
            att_name = elem.attrib["name"]
            if att_name == "value":  # Can be missing.
                value = int(elem.find("number").text)
            elif att_name == "prefEntry":
                entry = elem.find("cstring").text
            elif att_name == "prefPath":
                path = elem.find("cstring").text
    return path, entry, value


def _param_from_PrefDoubleSpinBox(widget):
    value = 0.0
    for elem in list(widget):
        if "name" in elem.keys():
            att_name = elem.attrib["name"]
            if att_name == "value": # Can be missing.
                value = float(elem.find("double").text)
            elif att_name == "prefEntry":
                entry = elem.find("cstring").text
            elif att_name == "prefPath":
                path = elem.find("cstring").text
    return path, entry, value


def _param_from_PrefUnitSpinBox(widget):
    return _param_from_PrefDoubleSpinBox(widget)


def _param_from_PrefQuantitySpinBox(widget):
    value = "0"
    for elem in list(widget):
        if "name" in elem.keys():
            att_name = elem.attrib["name"]
            if att_name == "rawValue":  # Not sure if this can be missing.
                value = elem.find("double").text.rstrip(".0")
            elif att_name == "unit":
                unit = elem.find("string").text
            elif att_name == "prefEntry":
                entry = elem.find("cstring").text
            elif att_name == "prefPath":
                path = elem.find("cstring").text
    value = value + " " + unit
    return path, entry, value


def _param_from_PrefColorButton(widget):
    for elem in list(widget):
        if "name" in elem.keys():
            att_name = elem.attrib["name"]
            if att_name == "color":
                sub = list(elem)[0]
                r = int(sub.find("red").text)
                g = int(sub.find("green").text)
                b = int(sub.find("blue").text)
            elif att_name == "prefEntry":
                entry = elem.find("cstring").text
            elif att_name == "prefPath":
                path = elem.find("cstring").text
    value = (r << 24) + (g << 16) + (b << 8) + 255
    return path, entry, value


def _param_from_PrefLineEdit(widget):
    value = None
    for elem in list(widget):
        if "name" in elem.keys():
            att_name = elem.attrib["name"]
            if att_name == "text":                # Can be missing.
                value = elem.find("string").text  # If text is missing value will be None here.
            elif att_name == "prefEntry":
                entry = elem.find("cstring").text
            elif att_name == "prefPath":
                path = elem.find("cstring").text
    if value is None:
        value = ""
    return path, entry, value


def _param_from_PrefFileChooser(widget):
    for elem in list(widget):
        if "name" in elem.keys():
            att_name = elem.attrib["name"]
            if att_name == "prefEntry":
                entry = elem.find("cstring").text
            elif att_name == "prefPath":
                path = elem.find("cstring").text
    return path, entry, ""


def _get_param_dictionary():

    # print("Creating preferences dictionary...")

    param_dict = {}

    # Draft parameters that are not in the preferences:
    param_dict["Mod/Draft"] = {
        "AnnotationStyleEditorHeight": ("int",       450),
        "AnnotationStyleEditorWidth":  ("int",       450),
        "CenterPlaneOnView":           ("bool",      False),
        "DefaultAnnoDisplayMode":      ("int",       0),
        "DefaultDisplayMode":          ("int",       0),
        "DefaultDrawStyle":            ("int",       0),
        "DefaultPrintColor":           ("unsigned",  255),
        "Draft_array_fuse":            ("bool",      False),
        "Draft_array_Link":            ("bool",      True),
        "fillmode":                    ("bool",      True),
        "HatchPatternResolution":      ("int",       128),
        "HatchPatternRotation":        ("float",     0.0),
        "HatchPatternScale":           ("float",     100.0),
        "labeltype":                   ("string",    "Custom"),
        "LayersManagerHeight":         ("int",       320),
        "LayersManagerWidth":          ("int",       640),
        "maxSnapEdges":                ("int",       0),
        "Offset_OCC":                  ("bool",      False),
        "ScaleClone":                  ("bool",      False),
        "ScaleCopy":                   ("bool",      False),
        "ScaleRelative":               ("bool",      False),
        "ScaleUniform":                ("bool",      False),
        "snapModes":                   ("string",    "100000000000000"),
        "snapRange":                   ("int",       8),
        "SvgLinesBlack":               ("bool",      True),
        "useSupport":                  ("bool",      False),


    }

    # Arch parameters that are not in the preferences:
    param_dict["Mod/Arch"] = {
        "ColorHelpers":                ("unsigned",  674321151),


    }

    # For the View parameters we do not check the preferences:
    param_dict["View"] = {
        "DefaultShapeColor":           ("unsigned",  3435973887),
        "DefaultShapeLineColor":       ("unsigned",  421075455),
        "DefaultShapeLineWidth":       ("int",       2),
        "DefaultShapePointSize":       ("int",       2),
        "DefaultShapeTransparency":    ("int",       0),
        "DefaultShapeVertexColor":     ("unsigned",  421075455),
        "EnableSelection":             ("bool",      True),
        "MarkerSize":                  ("int",       9),
    }

    # For the Units parameters we do not check the preferences:
    param_dict["Units"] = {
        "Decimals":                    ("int",       2),
        "UserSchema":                  ("int",       0),
    }

    # For the Mod/TechDraw/PAT parameters we do not check the preferences:
    param_dict["Mod/TechDraw/PAT"] = {
        "FilePattern":                 ("string",    ""),
        "NamePattern":                 ("string",    "Diamant"),
    }


    # Preferences ui files are stored in resource files.
    # For the Draft Workbench: /Mod/Draft/Draft_rc.py
    # For the Arch Workbench: /Mod/Arch/Arch_rc.py
    for fnm in (":/ui/preferences-draft.ui",
                ":/ui/preferences-draftinterface.ui",
                ":/ui/preferences-draftsnap.ui",
                ":/ui/preferences-drafttexts.ui",
                ":/ui/preferences-draftvisual.ui",
                ":/ui/preferences-dwg.ui",
                ":/ui/preferences-dxf.ui",
                ":/ui/preferences-oca.ui",
                ":/ui/preferences-svg.ui",
                ":/ui/preferences-arch.ui",
                ":/ui/preferences-archdefaults.ui",
                ":/ui/preferences-dae.ui",
                ":/ui/preferences-ifc.ui",
                ":/ui/preferences-ifc-export.ui"):

        # https://stackoverflow.com/questions/14750997/load-txt-file-from-resources-in-python
        fd = QtCore.QFile(fnm)
        if fd.open(QtCore.QIODevice.ReadOnly | QtCore.QFile.Text):
            text = QtCore.QTextStream(fd).readAll()
            fd.close()
        else:
            print("Preferences file " + fnm + " not found")
            continue

        # https://docs.python.org/3/library/xml.etree.elementtree.html
        root = ET.fromstring(text)

        # Get all preference widgets:
        # pref_widgets = [wid for wid in root.iter("widget") if "Gui::Pref" in wid.attrib["class"]]

        for widget in root.iter("widget"):
            if "class" in widget.keys():
                path = None
                att_class = widget.attrib["class"]

                if att_class == "Gui::PrefCheckBox":
                    path, entry, value = _param_from_PrefCheckBox(widget)
                    typ = "bool"
                elif att_class == "Gui::PrefRadioButton":
                    path, entry, value = _param_from_PrefRadioButton(widget)
                    typ = "bool"
                elif att_class == "Gui::PrefComboBox":
                    path, entry, value = _param_from_PrefComboBox(widget)
                    typ = "int"
                elif att_class == "Gui::PrefSpinBox":
                    path, entry, value = _param_from_PrefSpinBox(widget)
                    typ = "int"
                elif att_class == "Gui::PrefDoubleSpinBox":
                    path, entry, value = _param_from_PrefDoubleSpinBox(widget)
                    typ = "float"
                elif att_class == "Gui::PrefUnitSpinBox":
                    path, entry, value = _param_from_PrefUnitSpinBox(widget)
                    typ = "float"
                elif att_class == "Gui::PrefQuantitySpinBox":
                    path, entry, value = _param_from_PrefQuantitySpinBox(widget)
                    typ = "string"
                elif att_class == "Gui::PrefColorButton":
                    path, entry, value = _param_from_PrefColorButton(widget)
                    typ = "unsigned"
                elif att_class == "Gui::PrefLineEdit":
                    path, entry, value = _param_from_PrefLineEdit(widget)
                    typ = "string"
                elif att_class == "Gui::PrefFileChooser":
                    path, entry, value = _param_from_PrefFileChooser(widget)
                    typ = "string"

                if path is not None:
                    if path in param_dict:
                        param_dict[path][entry] = (typ, value)
                    else:
                        param_dict[path] = {entry: (typ, value)}

    return param_dict


PARAM_DICT = _get_param_dictionary()


# get_param("gridSpacing")
def get_param(entry, path="Mod/Draft"):
    """Return a stored parameter value or its default.

    Parameters
    ----------
    entry: str
        Name of the parameter.
    path: str, optional
        Defaults to "Mod/Draft".
        The path where the parameter can be found.
        This string is appended to "User parameter:BaseApp/Preferences/".

    Returns
    -------
    bool, float, int or str (if successful) or `None`.
    """
    if path not in PARAM_DICT:
        return None
    if entry not in PARAM_DICT[path]:
        return None
    param_grp = App.ParamGet("User parameter:BaseApp/Preferences/" + path)
    typ, default = PARAM_DICT[path][entry]
    if typ == "bool":
        return param_grp.GetBool(entry, default)
    if typ == "float":
        return param_grp.GetFloat(entry, default)
    if typ == "int":
        return param_grp.GetInt(entry, default)
    if typ == "string":
        return param_grp.GetString(entry, default)
    if typ == "unsigned":
        return param_grp.GetUnsigned(entry, default)
    return None


def get_param_arch(entry):
    return get_param(entry, path="Mod/Arch")


def get_param_view(entry):
    return get_param(entry, path="View")


def set_param(entry, value, path="Mod/Draft"):
    """Store a parameter value.

    Parameters
    ----------
    entry: str
        Name of the parameter.
    value: bool, float, int or str
        New value of the correct type.
    path: str, optional
        Defaults to "Mod/Draft".
        The path where the parameter can be found.
        This string is appended to "User parameter:BaseApp/Preferences/".

    Returns
    -------
    `True` (if successful) or `False`.
    """
    if path not in PARAM_DICT:
        return False
    if entry not in PARAM_DICT[path]:
        return False
    param_grp = App.ParamGet("User parameter:BaseApp/Preferences/" + path)
    typ = PARAM_DICT[path][entry][0]
    ret = True
    if typ == "bool":
        param_grp.SetBool(entry, value)
    elif typ == "float":
        param_grp.SetFloat(entry, value)
    elif typ == "int":
        param_grp.SetInt(entry, value)
    elif typ == "string":
        param_grp.SetString(entry, value)
    elif typ == "unsigned":
        param_grp.SetUnsigned(entry, value)
    else:
        ret = False
    return ret


def set_param_arch(entry, value):
    return set_param(entry, value, path="Mod/Arch")


def set_param_view(entry, value):
    return set_param(entry, value, path="View")
