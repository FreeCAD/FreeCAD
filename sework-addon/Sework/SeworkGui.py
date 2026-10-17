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
# O FreeCAD procura o pacote em <addon>/<nome do pacote>/<nome>.cfg
PACK_CFG = os.path.join(ADDON_DIR, "Sework Claro", "Sework Claro.cfg")


def _params():
    return FreeCAD.ParamGet(PARAM_PATH)


def _version_text():
    v = FreeCAD.Version()
    txt = "{}.{}".format(v[0], v[1])
    if len(v) > 2 and v[2]:
        txt += "." + str(v[2])
    return txt


def _addon_version():
    try:
        import re
        txt = open(os.path.join(ADDON_DIR, "package.xml"), encoding="utf-8").read()
        return re.search(r"<version>([^<]+)</version>", txt).group(1)
    except Exception:
        return "?"


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


# Versões anteriores da splash Sework (podem ser substituídas sem perguntar)
OLD_SPLASH_MD5 = {"abaffad5496ffefc6199d1d0254842e1"}


def _is_old_sework_splash(path):
    import hashlib
    try:
        return hashlib.md5(open(path, "rb").read()).hexdigest() in OLD_SPLASH_MD5
    except OSError:
        return False


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
        if os.path.isfile(dest) and not _params().GetBool("SplashIsOurs", False) and not _is_old_sework_splash(dest):
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


# ------------------------------------------------------------- tema de ícones
ICON_ZIP = os.path.join(RES_DIR, "tema-icones-sework.zip")
BITMAPS = "User parameter:BaseApp/Preferences/Bitmaps"
ICON_PATH_KEY = "CustomPathSework"  # lido por BitmapFactory::restoreCustomPaths (prefixo "CustomPath")


def _icon_dir():
    return os.path.join(FreeCAD.getUserAppDataDir(), "SeworkIcones")


def install_icon_theme():
    """Descompacta o tema "Sework Sóbrio" e o registra antes dos ícones embutidos.

    O FreeCAD lê os caminhos de ícones ao iniciar, então a troca aparece na
    próxima abertura do programa.
    """
    bitmaps = FreeCAD.ParamGet(BITMAPS)
    if not _params().GetBool("IconTheme", True) or not os.path.isfile(ICON_ZIP):
        bitmaps.RemString(ICON_PATH_KEY)
        return
    import zipfile

    dest = _icon_dir()
    stamp = "{}-{}".format(os.path.getsize(ICON_ZIP), int(os.path.getmtime(ICON_ZIP)))
    try:
        if _params().GetString("IconThemeStamp", "") != stamp or not os.path.isdir(dest):
            if os.path.isdir(dest):
                shutil.rmtree(dest, ignore_errors=True)
            os.makedirs(dest, exist_ok=True)
            with zipfile.ZipFile(ICON_ZIP) as z:
                z.extractall(dest)
            _params().SetString("IconThemeStamp", stamp)
            FreeCAD.Console.PrintMessage("Sework: tema de ícones instalado (vale na próxima abertura).\n")
        if bitmaps.GetString(ICON_PATH_KEY, "") != dest:
            bitmaps.SetString(ICON_PATH_KEY, dest)
    except (OSError, zipfile.BadZipFile) as exc:
        FreeCAD.Console.PrintWarning("Sework: tema de ícones não instalado: {}\n".format(exc))


class CmdTemaIcones:
    def GetResources(self):
        return {
            "Pixmap": ICON_PATH,
            "MenuText": "Alternar tema de ícones Sework",
            "ToolTip": "Liga ou desliga o tema de ícones Sework Sóbrio (vale ao reabrir o FreeCAD)",
        }

    def Activated(self):
        on = not _params().GetBool("IconTheme", True)
        _params().SetBool("IconTheme", on)
        install_icon_theme()
        QtWidgets.QMessageBox.information(
            FreeCADGui.getMainWindow(), APP_NAME,
            "Tema de ícones Sework {}.\nFeche e abra o FreeCAD para ver a mudança.".format(
                "ligado" if on else "desligado"))

    def IsActive(self):
        return True


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


QUALITY = {
    # Anti-aliasing da vista 3D: 0 nenhum, 1 suavização de linha, 2 MSAA 2x,
    # 3 MSAA 4x, 5 MSAA 6x, 4 MSAA 8x (enum Gui::AntiAliasing). Vale para vistas novas.
    "AntiAliasing": 4,
    # Tesselação das peças (Mod/Part): desvio em % do tamanho e ângulo em graus.
    # Padrão do FreeCAD: 0,2 % e 28,65°. Menor = curvas mais lisas, arquivo mais pesado na tela.
    "MeshDeviation": 0.1,
    "MeshAngularDeflection": 12.0,
}


def apply_quality():
    """Linhas suaves (MSAA 8x) e curvas mais lisas. Vale para vistas abertas depois."""
    FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").SetInt(
        "AntiAliasing", QUALITY["AntiAliasing"])
    part = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Part")
    part.SetFloat("MeshDeviation", QUALITY["MeshDeviation"])
    part.SetFloat("MeshAngularDeflection", QUALITY["MeshAngularDeflection"])


def apply_theme():
    root = ET.parse(PACK_CFG).getroot()
    base = root.find("FCParamGroup[@Name='Root']/FCParamGroup[@Name='BaseApp']")
    _apply_group(base, "User parameter:BaseApp")
    _set_light_stylesheet()
    apply_quality()


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
        import SeworkSobre
        SeworkSobre.mostrar("{} · FreeCAD {}".format(_addon_version(), _version_text()),
                            "FreeCAD {} (freecad.org)".format(_version_text()),
                            FreeCADGui.getMainWindow())

    def IsActive(self):
        return True


# ------------------------------------------------------------- modo distribuição
DEV_DOCKS = ("Python console", "Report view")


def apply_modo_distribuicao(on=None):
    """Esconde (ou mostra) console Python e Relatório, que o usuário final não usa."""
    if on is None:
        on = _params().GetBool("ModoDistribuicao", False)
    mw = FreeCADGui.getMainWindow()
    if mw is None:
        return
    for dock in mw.findChildren(QtWidgets.QDockWidget):
        if dock.objectName() in DEV_DOCKS:
            dock.setVisible(not on)


class CmdModoDistribuicao:
    def GetResources(self):
        return {
            "Pixmap": ICON_PATH,
            "MenuText": "Modo distribuição",
            "ToolTip": "Liga/desliga o modo para usuário final: esconde o console Python e o Relatório",
            "Checkable": _params().GetBool("ModoDistribuicao", False),
        }

    def Activated(self, checked=None):
        on = bool(checked) if checked is not None else not _params().GetBool("ModoDistribuicao", False)
        _params().SetBool("ModoDistribuicao", on)
        apply_modo_distribuicao(on)

    def IsActive(self):
        return True


class CmdRemoverSplash:
    def GetResources(self):
        return {
            "Pixmap": ICON_PATH,
            "MenuText": "Voltar à splash original",
            "ToolTip": "Remove a splash Sework e restaura a do FreeCAD",
        }

    def Activated(self):
        remove_splash()
        FreeCAD.Console.PrintMessage("Sework: splash original restaurada.\n")

    def IsActive(self):
        return True


class CmdBarrasClassicas:
    def GetResources(self):
        return {
            "Pixmap": ICON_PATH,
            "MenuText": "Alternar faixa Sework / barras clássicas",
            "ToolTip": "Liga ou desliga a faixa de ferramentas Sework",
        }

    def Activated(self):
        import SeworkRibbon

        if SeworkRibbon._state["ribbon"] is not None:
            SeworkRibbon.uninstall(restore_toolbars=True)
            _params().SetBool("RibbonEnabled", False)
        else:
            SeworkRibbon.install()
            _params().SetBool("RibbonEnabled", True)

    def IsActive(self):
        return True


COMMANDS = ["Sework_AplicarTema", "Sework_Sobre"]
MENU_EXTRA = ["Sework_TemaIcones", "Sework_BarrasClassicas", "Sework_RemoverSplash"]


def register_commands():
    FreeCADGui.addCommand("Sework_AplicarTema", CmdAplicarTema())
    FreeCADGui.addCommand("Sework_Sobre", CmdSobre())
    FreeCADGui.addCommand("Sework_RemoverSplash", CmdRemoverSplash())
    FreeCADGui.addCommand("Sework_BarrasClassicas", CmdBarrasClassicas())
    FreeCADGui.addCommand("Sework_TemaIcones", CmdTemaIcones())
    FreeCADGui.addCommand("Sework_ModoDistribuicao", CmdModoDistribuicao())
    try:
        import SeworkInicio
        FreeCADGui.addCommand("Sework_Inicio", SeworkInicio.CmdInicio())
    except Exception as exc:
        FreeCAD.Console.PrintWarning("Sework: tela inicial indisponível: {}\n".format(exc))


def install_translations():
    """Tradução pt-BR Sework com prioridade sobre a oficial (ver SeworkI18n)."""
    try:
        import SeworkI18n

        SeworkI18n.install()
        mw = FreeCADGui.getMainWindow()
        if mw is not None:
            mw.workbenchActivated.connect(SeworkI18n.on_workbench)
    except Exception as exc:
        FreeCAD.Console.PrintWarning("Sework: tradução não aplicada: {}\n".format(exc))


def _start_ribbon():
    if not _params().GetBool("RibbonEnabled", True):
        return
    try:
        import SeworkRibbon

        SeworkRibbon.install()
    except Exception as exc:
        FreeCAD.Console.PrintError("Sework: falha ao montar a faixa: {}\n".format(exc))


def _start_guia():
    try:
        import SeworkGuia

        SeworkGuia.install()
    except Exception as exc:
        FreeCAD.Console.PrintError("Sework: falha no guia ilustrado: {}\n".format(exc))


def _start_inicio():
    try:
        import SeworkInicio
        SeworkInicio.startup()
    except Exception as exc:
        FreeCAD.Console.PrintWarning("Sework: tela inicial não aberta: {}\n".format(exc))


def startup():
    try:
        import SeworkInicio
        SeworkInicio.prepare()
    except Exception as exc:
        FreeCAD.Console.PrintWarning("Sework: tela inicial: {}\n".format(exc))
    register_commands()
    install_splash()
    install_translations()
    install_icon_theme()
    try:
        import SeworkEstilo
        SeworkEstilo.install()
    except Exception as exc:
        FreeCAD.Console.PrintWarning("Sework: estilo não instalado: {}\n".format(exc))
    QtCore.QTimer.singleShot(600, _start_ribbon)
    QtCore.QTimer.singleShot(900, _start_guia)
    QtCore.QTimer.singleShot(1300, _start_inicio)
    QtCore.QTimer.singleShot(1500, apply_modo_distribuicao)
    # Executa depois que a janela principal termina de montar.
    QtCore.QTimer.singleShot(0, apply_window_branding)
    if not _params().GetBool("QualityApplied", False):
        _params().SetBool("QualityApplied", True)
        apply_quality()
    if _params().GetBool("FirstRun", True):
        _params().SetBool("FirstRun", False)
        if _params().GetBool("ApplyThemeOnFirstRun", True):
            apply_theme()
            FreeCAD.Console.PrintMessage(
                "Sework: tema aplicado na primeira execução — reabra o FreeCAD.\n")
