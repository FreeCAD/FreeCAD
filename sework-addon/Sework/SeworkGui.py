# SPDX-License-Identifier: LGPL-2.1-or-later
# Sework Engenharia - identidade visual para o FreeCAD.
"""Branding Sework: título e ícone da janela, splash e aplicação do tema."""

import filecmp
import os
import shutil
import xml.etree.ElementTree as ET

import FreeCAD
import FreeCADGui

try:
    from PySide import QtCore, QtGui, QtWidgets  # FreeCAD 1.x (shim PySide)
except ImportError:  # pragma: no cover
    from PySide2 import QtCore, QtGui, QtWidgets

APP_NAME = "FreeCAD Sework"
PARAM_PATH = "User parameter:BaseApp/Preferences/Mod/Sework"
ADDON_DIR = os.path.dirname(os.path.abspath(__file__))
RES_DIR = os.path.join(ADDON_DIR, "Resources")
ICON_PATH = os.path.join(RES_DIR, "icons", "sework.png")
SPLASH_SRC = os.path.join(RES_DIR, "splash_image.png")
PACK_CFG = os.path.join(ADDON_DIR, "PreferencePacks", "Sework Claro", "Sework Claro.cfg")


def _params():
    return FreeCAD.ParamGet(PARAM_PATH)


def _version_text():
    v = FreeCAD.Version()
    txt = "{}.{}".format(v[0], v[1])
    if len(v) > 2 and v[2]:
        txt += "." + str(v[2])
    return txt


# --------------------------------------------------------------------- branding
def apply_window_branding():
    """Troca nome da aplicação, título e ícone da janela principal."""
    if not _params().GetBool("RenameWindow", True):
        return
    app = QtWidgets.QApplication.instance()
    if app is None:
        return
    # O MainWindow do FreeCAD monta o título a partir do applicationName,
    # então os títulos gerados depois (troca de documento) já saem "FreeCAD Sework".
    app.setApplicationName(APP_NAME)
    if os.path.isfile(ICON_PATH):
        app.setWindowIcon(QtGui.QIcon(ICON_PATH))
    mw = FreeCADGui.getMainWindow()
    if mw is not None:
        doc = FreeCAD.ActiveDocument
        base = "{} {}".format(APP_NAME, _version_text())
        mw.setWindowTitle("[*] {} - {}".format(doc.Label, base) if doc else base)


def install_splash():
    """Copia a splash para <dados do usuário>/Gui/images/splash_image.png.

    O FreeCAD procura esse arquivo antes da splash padrão, então a troca
    aparece a partir da próxima abertura do programa.
    """
    if not _params().GetBool("InstallSplash", True) or not os.path.isfile(SPLASH_SRC):
        return
    dest_dir = os.path.join(FreeCAD.getUserAppDataDir(), "Gui", "images")
    dest = os.path.join(dest_dir, "splash_image.png")
    try:
        if os.path.isfile(dest) and filecmp.cmp(SPLASH_SRC, dest, shallow=False):
            return
        if os.path.isfile(dest) and not _params().GetBool("SplashIsOurs", False):
            # Já existe uma splash personalizada que não é nossa: não sobrescrever.
            FreeCAD.Console.PrintWarning(
                "Sework: já existe {} — splash Sework não instalada.\n".format(dest)
            )
            return
        os.makedirs(dest_dir, exist_ok=True)
        shutil.copyfile(SPLASH_SRC, dest)
        _params().SetBool("SplashIsOurs", True)
        FreeCAD.Console.PrintMessage("Sework: splash instalada (vale na próxima abertura).\n")
    except OSError as exc:
        FreeCAD.Console.PrintWarning("Sework: não foi possível instalar a splash: {}\n".format(exc))


def remove_splash():
    dest = os.path.join(FreeCAD.getUserAppDataDir(), "Gui", "images", "splash_image.png")
    if os.path.isfile(dest) and _params().GetBool("SplashIsOurs", False):
        os.remove(dest)
    _params().SetBool("SplashIsOurs", False)
    _params().SetBool("InstallSplash", False)


# ------------------------------------------------------------------------ tema
_SETTERS = {
    "FCUInt": lambda g, n, v: g.SetUnsigned(n, int(v)),
    "FCInt": lambda g, n, v: g.SetInt(n, int(v)),
    "FCFloat": lambda g, n, v: g.SetFloat(n, float(v)),
    "FCBool": lambda g, n, v: g.SetBool(n, v == "1"),
}


def _apply_group(node, path):
    grp = FreeCAD.ParamGet(path)
    for child in node:
        name = child.get("Name")
        if child.tag == "FCParamGroup":
            _apply_group(child, path + "/" + name)
        elif child.tag == "FCText":
            grp.SetString(name, child.text or "")
        elif child.tag in _SETTERS:
            _SETTERS[child.tag](grp, name, child.get("Value"))


def _set_light_stylesheet():
    """Escolhe o stylesheet claro que existe na versão instalada."""
    sheets = os.path.join(FreeCAD.getResourceDir(), "Gui", "Stylesheets")
    mw = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/MainWindow")
    for sheet in ("FreeCAD.qss", "FreeCAD Light.qss"):
        if os.path.isfile(os.path.join(sheets, sheet)):
            mw.SetString("StyleSheet", sheet)
            mw.SetString("Theme", "FreeCAD Light")
            mw.SetString("QtStyle", "FreeCAD")
            return True
    return False


def apply_theme():
    root = ET.parse(PACK_CFG).getroot()
    base = root.find("FCParamGroup[@Name='Root']/FCParamGroup[@Name='BaseApp']")
    _apply_group(base, "User parameter:BaseApp")
    _set_light_stylesheet()


# -------------------------------------------------------------------- comandos
class CmdAplicarTema:
    def GetResources(self):
        return {
            "Pixmap": ICON_PATH,
            "MenuText": "Aplicar tema Sework",
            "ToolTip": "Aplica as cores Sework na interface e na vista 3D",
        }

    def Activated(self):
        apply_theme()
        QtWidgets.QMessageBox.information(
            FreeCADGui.getMainWindow(),
            APP_NAME,
            "Tema Sework aplicado.\nFeche e abra o FreeCAD para ver todas as cores.",
        )

    def IsActive(self):
        return True


class CmdSobre:
    def GetResources(self):
        return {
            "Pixmap": ICON_PATH,
            "MenuText": "Sobre o FreeCAD Sework",
            "ToolTip": "Informações sobre esta distribuição",
        }

    def Activated(self):
        box = QtWidgets.QMessageBox(FreeCADGui.getMainWindow())
        box.setWindowTitle("Sobre o " + APP_NAME)
        box.setIconPixmap(QtGui.QPixmap(ICON_PATH).scaled(
            96, 96, QtCore.Qt.KeepAspectRatio, QtCore.Qt.SmoothTransformation))
        box.setTextFormat(QtCore.Qt.RichText)
        box.setText(
            "<h3 style='color:#003A70'>{} {}</h3>"
            "<p><b>SEWORK ENGENHARIA</b><br>"
            "Engenharia Mecânica e Segurança do Trabalho<br>Patos de Minas/MG</p>"
            "<p>sework@sework.com.br</p>"
            "<p style='color:#878787'>Baseado no FreeCAD (freecad.org), "
            "software livre sob licença LGPL-2.1-or-later.</p>".format(APP_NAME, _version_text())
        )
        box.exec_() if hasattr(box, "exec_") else box.exec()

    def IsActive(self):
        return True


class CmdRemoverSplash:
    def GetResources(self):
        return {
            "MenuText": "Voltar à splash original",
            "ToolTip": "Remove a splash Sework e restaura a do FreeCAD",
        }

    def Activated(self):
        remove_splash()
        FreeCAD.Console.PrintMessage("Sework: splash original restaurada.\n")

    def IsActive(self):
        return True


COMMANDS = ["Sework_AplicarTema", "Sework_Sobre"]
MENU_EXTRA = ["Sework_RemoverSplash"]


def register_commands():
    FreeCADGui.addCommand("Sework_AplicarTema", CmdAplicarTema())
    FreeCADGui.addCommand("Sework_Sobre", CmdSobre())
    FreeCADGui.addCommand("Sework_RemoverSplash", CmdRemoverSplash())


def startup():
    register_commands()
    install_splash()
    # Executa depois que a janela principal termina de montar.
    QtCore.QTimer.singleShot(0, apply_window_branding)
    if _params().GetBool("FirstRun", True):
        _params().SetBool("FirstRun", False)
        if _params().GetBool("ApplyThemeOnFirstRun", True):
            apply_theme()
            FreeCAD.Console.PrintMessage(
                "Sework: tema aplicado na primeira execução — reabra o FreeCAD.\n")
