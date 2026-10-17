# SPDX-License-Identifier: LGPL-2.1-or-later
"""Tela inicial Sework: cartões de "Novo projeto" e arquivos recentes com miniatura."""
import os
import zipfile

import FreeCAD
import SeworkEstilo
import FreeCADGui
from PySide import QtCore, QtGui, QtWidgets

HERE = os.path.dirname(os.path.abspath(__file__))
LOGO = os.path.join(HERE, "Resources", "icons", "sework_64.png")
PARAM_PATH = "User parameter:BaseApp/Preferences/Mod/Sework"
TITLE = "Início"

QSS = """
#SeworkInicio { background: #F4F7FB; }
#SeworkInicio QLabel { background: transparent; }
#SwTitulo { color: #003A70; font-size: 18pt; font-weight: 700; }
#SwSub { color: #5B6876; font-size: 10pt; }
#SwSecao { color: #003A70; font-size: 12pt; font-weight: 700; padding-top: 6px; }
#SwSecaoSub { color: #6B7785; font-size: 9pt; }
#SwCard { background: #FFFFFF; border: 1px solid #DCE3EC; border-radius: 8px; }
#SwCard:hover { border-color: #7FA3C9; background: #FBFCFE; }
#SeworkInicio #SwIconBox { background: #EEF2F8; border-radius: 6px; }
#SwCardTitulo { color: #003A70; font-size: 10.5pt; font-weight: 700; }
#SwCardTexto { color: #5B6876; font-size: 9pt; }
#SwVazio { color: #8A96A3; font-size: 9pt; }
#SwRodape { color: #8A96A3; font-size: 8.5pt; }
#SeworkInicio QCheckBox { color: #5B6876; font-size: 9pt; }
#SeworkInicio QScrollArea { border: none; background: transparent; }
"""

NOVOS = [
    ("PartDesign_Body", "Peça", "Modelagem de peça sólida por esboços e operações.", "peca"),
    ("Geoassembly", "Montagem", "Una peças com juntas e monte o conjunto.", "montagem"),
    ("TechDraw_PageDefault", "Desenho técnico", "Folha com vistas, cotas e legenda.", "desenho"),
    ("Sketcher_NewSketch", "Esboço", "Perfil 2D com restrições geométricas.", "esboco"),
    ("Spreadsheet", "Planilha", "Parâmetros e tabelas ligados ao modelo.", "planilha"),
    ("document-open", "Abrir arquivo", "Abra um projeto .FCStd, STEP, IGES, STL…", "abrir"),
]


def _params():
    return FreeCAD.ParamGet(PARAM_PATH)


def _icon_path(name):
    d = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Bitmaps").GetString("CustomPathSework", "")
    p = os.path.join(d, name + ".svg") if d else ""
    return p if os.path.isfile(p) else None


def _icon(name, size):
    p = _icon_path(name)
    icon = QtGui.QIcon(p) if p else FreeCADGui.getIcon(name) if hasattr(FreeCADGui, "getIcon") else QtGui.QIcon()
    dpr = QtWidgets.QApplication.instance().devicePixelRatio()
    dpr = max(dpr, 1)
    pm = icon.pixmap(int(size * dpr), int(size * dpr))
    pm.setDevicePixelRatio(dpr)
    return pm


def _thumb(path, size):
    try:
        with zipfile.ZipFile(path) as z:
            data = z.read("thumbnails/Thumbnail.png")
        img = QtGui.QImage.fromData(data)
        if img.isNull():
            return None
        dpr = max(QtWidgets.QApplication.instance().devicePixelRatio(), 1)
        pm = QtGui.QPixmap.fromImage(img).scaled(int(size * dpr), int(size * dpr), QtCore.Qt.KeepAspectRatio,
                                                 QtCore.Qt.SmoothTransformation)
        pm.setDevicePixelRatio(dpr)
        return pm
    except Exception:
        return None


# ---------------------------------------------------------------- ações
def _new(kind):
    if kind == "abrir":
        FreeCADGui.runCommand("Std_Open")
        return
    wb, cmd = {
        "peca": ("PartDesignWorkbench", "PartDesign_Body"),
        "montagem": ("AssemblyWorkbench", "Assembly_CreateAssembly"),
        "desenho": ("TechDrawWorkbench", "TechDraw_PageDefault"),
        "esboco": ("SketcherWorkbench", "Sketcher_NewSketch"),
        "planilha": ("SpreadsheetWorkbench", "Spreadsheet_CreateSheet"),
    }[kind]
    FreeCAD.newDocument()
    try:
        FreeCADGui.activateWorkbench(wb)
    except Exception:
        pass
    QtCore.QTimer.singleShot(150, lambda: _run(cmd))
    try:  # a bancada pode já estar ativa (sem sinal de troca): leva a faixa à aba certa
        import SeworkRibbon
        rib = SeworkRibbon._state.get("ribbon")
        if rib is not None:
            rib.sync_workbench(wb)
    except Exception:
        pass


def _run(cmd):
    try:
        FreeCADGui.runCommand(cmd)
    except Exception as exc:
        FreeCAD.Console.PrintWarning("Sework: {} indisponível: {}\n".format(cmd, exc))


def _open(path):
    if not os.path.isfile(path):
        QtWidgets.QMessageBox.information(FreeCADGui.getMainWindow(), "Arquivo não encontrado",
                                          "O arquivo não existe mais:\n" + path)
        return
    ext = os.path.splitext(path)[1].lower()
    if ext == ".fcstd":
        FreeCAD.openDocument(path)
    else:
        FreeCADGui.open(path)


def _recent():
    g = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/RecentFiles")
    out = []
    for i in range(g.GetInt("RecentFiles", 8) or 8):
        p = g.GetString("MRU%d" % i, "")
        if p and p not in out:
            out.append(p)
    return out


# ---------------------------------------------------------------- widgets
class Card(QtWidgets.QFrame):
    def __init__(self, pix, title, text, action, box=48, parent=None):
        super().__init__(parent)
        self.setObjectName("SwCard")
        self.setCursor(QtCore.Qt.PointingHandCursor)
        self._action = action
        lay = QtWidgets.QHBoxLayout(self)
        lay.setContentsMargins(18, 16, 18, 16)
        lay.setSpacing(16)
        ib = QtWidgets.QLabel()
        ib.setObjectName("SwIconBox")
        ib.setFixedSize(box, box)
        ib.setAlignment(QtCore.Qt.AlignCenter)
        if pix is not None:
            ib.setPixmap(pix)
        lay.addWidget(ib, 0, QtCore.Qt.AlignTop)
        col = QtWidgets.QVBoxLayout()
        col.setSpacing(2)
        t = QtWidgets.QLabel(title)
        t.setObjectName("SwCardTitulo")
        d = QtWidgets.QLabel(text)
        d.setObjectName("SwCardTexto")
        d.setWordWrap(True)
        col.addWidget(t)
        col.addWidget(d)
        col.addStretch(1)
        lay.addLayout(col, 1)
        self.setToolTip(text)

    def mouseReleaseEvent(self, ev):
        if ev.button() == QtCore.Qt.LeftButton and self.rect().contains(ev.pos()):
            QtCore.QTimer.singleShot(0, self._action)
        super().mouseReleaseEvent(ev)


class InicioSework(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("SeworkInicio")
        self.setAttribute(QtCore.Qt.WA_StyledBackground, True)
        self.setStyleSheet(SeworkEstilo.tema(QSS))
        self.setWindowTitle(TITLE)
        outer = QtWidgets.QVBoxLayout(self)
        outer.setContentsMargins(0, 0, 0, 0)
        scroll = QtWidgets.QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QtWidgets.QFrame.NoFrame)
        outer.addWidget(scroll)
        host = QtWidgets.QWidget()
        host.setObjectName("SeworkInicio")
        host.setAttribute(QtCore.Qt.WA_StyledBackground, True)
        scroll.setWidget(host)
        hl = QtWidgets.QHBoxLayout(host)
        hl.setContentsMargins(32, 28, 32, 28)
        hl.addStretch(1)
        self.body = QtWidgets.QWidget()
        self.body.setMaximumWidth(1120)
        self.body.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Preferred)
        hl.addWidget(self.body, 8)
        hl.addStretch(1)
        self.v = QtWidgets.QVBoxLayout(self.body)
        self.v.setContentsMargins(0, 0, 0, 0)
        self.v.setSpacing(14)
        self.build()

    def _section(self, title, sub=None):
        t = QtWidgets.QLabel(title)
        t.setObjectName("SwSecao")
        self.v.addSpacing(8)
        self.v.addWidget(t)
        if sub:
            s = QtWidgets.QLabel(sub)
            s.setObjectName("SwSecaoSub")
            self.v.addWidget(s)

    def build(self):
        # cabeçalho
        head = QtWidgets.QHBoxLayout()
        head.setSpacing(14)
        if os.path.isfile(LOGO):
            lg = QtWidgets.QLabel()
            pm = QtGui.QPixmap(LOGO)
            lg.setPixmap(pm.scaled(48, 48, QtCore.Qt.KeepAspectRatio, QtCore.Qt.SmoothTransformation))
            head.addWidget(lg, 0, QtCore.Qt.AlignTop)
        tc = QtWidgets.QVBoxLayout()
        tc.setSpacing(0)
        t = QtWidgets.QLabel("FreeCAD Sework")
        t.setObjectName("SwTitulo")
        s = QtWidgets.QLabel("Projetos mecânicos — escolha por onde começar.")
        s.setObjectName("SwSub")
        tc.addWidget(t)
        tc.addWidget(s)
        head.addLayout(tc, 1)
        self.v.addLayout(head)

        # novo projeto
        self._section("Novo projeto")
        grid = QtWidgets.QGridLayout()
        grid.setHorizontalSpacing(14)
        grid.setVerticalSpacing(14)
        for i, (ic, title, text, kind) in enumerate(NOVOS):
            card = Card(_icon(ic, 32), title, text, (lambda k=kind: _new(k)))
            grid.addWidget(card, i // 3, i % 3)
        for c in range(3):
            grid.setColumnStretch(c, 1)
        self.v.addLayout(grid)

        # recentes
        rec = _recent()
        self._section("Arquivos recentes", None if rec else None)
        if not rec:
            e = QtWidgets.QLabel("Nenhum arquivo aberto recentemente.")
            e.setObjectName("SwVazio")
            self.v.addWidget(e)
        else:
            g2 = QtWidgets.QGridLayout()
            g2.setHorizontalSpacing(14)
            g2.setVerticalSpacing(14)
            fallback = _icon("PartDesign_Body", 30)
            for i, path in enumerate(rec[:9]):
                name = os.path.basename(path)
                folder = os.path.dirname(path)
                pix = _thumb(path, 46) if path.lower().endswith(".fcstd") else None
                card = Card(pix or fallback, name, folder, (lambda p=path: _open(p)))
                g2.addWidget(card, i // 3, i % 3)
            for c in range(3):
                g2.setColumnStretch(c, 1)
            self.v.addLayout(g2)

        self.v.addStretch(1)
        foot = QtWidgets.QHBoxLayout()
        cb = QtWidgets.QCheckBox("Mostrar esta tela ao abrir o FreeCAD")
        cb.setChecked(_params().GetBool("TelaInicial", True))
        cb.toggled.connect(_toggle_tela)
        foot.addWidget(cb)
        foot.addStretch(1)
        r = QtWidgets.QLabel("Sework Engenharia · Patos de Minas/MG")
        r.setObjectName("SwRodape")
        foot.addWidget(r)
        self.v.addSpacing(10)
        self.v.addLayout(foot)

    def refresh(self):
        while self.v.count():
            it = self.v.takeAt(0)
            _clear(it)
        self.build()


def _toggle_tela(on):
    _params().SetBool("TelaInicial", on)
    prepare()


def _clear(item):
    w = item.widget()
    if w is not None:
        w.deleteLater()
        return
    lay = item.layout()
    if lay is not None:
        while lay.count():
            _clear(lay.takeAt(0))


# ---------------------------------------------------------------- integração com a área MDI
_state = {"sub": None}


def _mdi():
    mw = FreeCADGui.getMainWindow()
    return mw.findChild(QtWidgets.QMdiArea) if mw else None


def _close_stock_start():
    area = _mdi()
    if area is None:
        return
    for sub in area.subWindowList():
        w = sub.widget()
        cn = w.metaObject().className() if w is not None else ""
        if cn.startswith("StartGui::") or cn == "Gui::StartView":
            sub.close()


def show():
    area = _mdi()
    if area is None:
        return
    sub = _state["sub"]
    try:
        alive = sub is not None and sub.widget() is not None and sub in area.subWindowList()
    except RuntimeError:
        alive = False
    if alive:
        sub.widget().refresh()
        area.setActiveSubWindow(sub)
        return
    w = InicioSework()
    sub = area.addSubWindow(w)
    sub.setWindowTitle(TITLE)
    sub.setAttribute(QtCore.Qt.WA_DeleteOnClose, True)
    p = os.path.join(HERE, "Resources", "icons", "sework.png")
    if os.path.isfile(p):
        sub.setWindowIcon(QtGui.QIcon(p))
    sub.showMaximized()
    _state["sub"] = sub
    _connect_side()
    _safe(_on_subwindow, sub)


def prepare():
    """Antes da janela terminar de montar: impede a tela inicial padrão de abrir.

    Fechar a tela padrão depois de aberta pode derrubar o FreeCAD (Access violation),
    então desligamos o parâmetro do próprio FreeCAD e guardamos que fomos nós.
    """
    start = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start")
    if _params().GetBool("TelaInicial", True):
        if start.GetBool("ShowOnStartup", True):
            start.SetBool("ShowOnStartup", False)
            _params().SetBool("StartPadraoDesligadoPelaSework", True)
    elif _params().GetBool("StartPadraoDesligadoPelaSework", False):
        start.SetBool("ShowOnStartup", True)
        _params().SetBool("StartPadraoDesligadoPelaSework", False)


# ---------------------------------------------------------------- barra lateral só no trabalho
SIDE_DOCKS = ("Model", "Tasks", "Combo View")
_side = {"connected": False}


def _is_start(sub):
    try:
        return sub is not None and isinstance(sub.widget(), InicioSework)
    except RuntimeError:
        return False


def _hidden_names():
    txt = _params().GetString("LateralOcultaPelaSework", "")
    return [n for n in txt.split("|") if n]


def _set_hidden_names(names):
    _params().SetString("LateralOcultaPelaSework", "|".join(sorted(set(names))))


def _on_subwindow(sub):
    """Esconde Modelo/Tarefas enquanto a tela inicial está na frente; devolve ao sair.

    O que a Sework escondeu fica gravado nas preferências: o FreeCAD salva o layout
    ao fechar, e a barra voltaria escondida na próxima abertura.
    """
    if not _params().GetBool("OcultarLateralNoInicio", True):
        return
    mw = FreeCADGui.getMainWindow()
    if mw is None:
        return
    docks = [d for d in mw.findChildren(QtWidgets.QDockWidget) if d.objectName() in SIDE_DOCKS]
    hidden = _hidden_names()
    if _is_start(sub):
        newly = [d.objectName() for d in docks if d.isVisible()]
        for d in docks:
            if d.isVisible():
                d.hide()
        if newly:
            _set_hidden_names(hidden + newly)
    elif hidden:
        for d in docks:
            if d.objectName() in hidden:
                d.show()
        _set_hidden_names([])


def _poll_side():
    area = _mdi()
    if area is None:
        return
    _on_subwindow(area.currentSubWindow())


def _connect_side():
    """Acompanha a janela ativa (o sinal subWindowActivated nem sempre chega)."""
    if _side["connected"]:
        return
    t = QtCore.QTimer()
    t.setInterval(400)
    t.timeout.connect(lambda: _safe(_poll_side))
    t.start()
    _side["timer"] = t
    _side["connected"] = True


def _safe(fn, *a):
    try:
        fn(*a)
    except Exception as exc:
        FreeCAD.Console.PrintWarning("Sework: tela inicial: {}\n".format(exc))


def startup():
    """Na abertura: mostra a tela inicial Sework quando não há documento aberto."""
    _connect_side()
    if not _params().GetBool("TelaInicial", True):
        return
    if not FreeCAD.listDocuments():
        show()


class CmdInicio:
    def GetResources(self):
        return {"Pixmap": os.path.join(HERE, "Resources", "icons", "sework.png"),
                "MenuText": "Tela inicial", "ToolTip": "Abre a tela inicial Sework"}

    def Activated(self):
        show()

    def IsActive(self):
        return True
