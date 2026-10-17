# SPDX-License-Identifier: LGPL-2.1-or-later
# Sework Engenharia - guia ilustrado no painel de tarefas.
"""Mostra, acima dos parâmetros do recurso em edição, um desenho que explica o que
a função faz no modo escolhido, com uma instrução curta. O desenho acompanha as
mudanças de modo em tempo real (lê as propriedades do objeto em edição).
"""

import FreeCAD
import FreeCADGui

try:
    from PySide import QtCore, QtGui, QtWidgets
except ImportError:  # pragma: no cover
    from PySide2 import QtCore, QtGui, QtWidgets
try:
    from PySide6 import QtSvg
except ImportError:  # pragma: no cover
    from PySide2 import QtSvg
try:
    import shiboken6 as shiboken
except ImportError:  # pragma: no cover
    import shiboken2 as shiboken

import SeworkIlustra as I

PARAM_PATH = "User parameter:BaseApp/Preferences/Mod/Sework"

QSS = """
#SeworkGuia { background: #FFFFFF; border: 1px solid #DCE4EC; border-left: 4px solid #F7BE00; border-radius: 8px; }
#SeworkGuia QLabel { background: transparent; color: #1F2A36; }
#SeworkGuiaTitulo { color: #003A70; font-weight: 700; font-size: 10pt; }
#SeworkGuiaTexto { color: #33404D; font-size: 9pt; }
#SeworkGuiaLegenda { color: #4A5866; font-size: 8pt; }
#SeworkGuia QToolButton { border: none; color: #66727F; padding: 0 2px; }
#SeworkGuia QToolButton:hover { color: #003A70; }
"""


def _get(obj, name, default=None):
    try:
        return getattr(obj, name)
    except Exception:
        return default


def _side_mode(obj, default="Length"):
    """Extrusão/Cavidade 1.1: SideType (One side / Two sides / Symmetric) + Type."""
    side = str(_get(obj, "SideType", "") or "")
    mode = str(_get(obj, "Type", default) or default)
    mid = bool(_get(obj, "Midplane", False)) or side == "Symmetric"
    if side == "Two sides":
        mode = "TwoLengths"
    return mode, mid


PAD_TXT = {
    "Length": "Selecione um esboço fechado e informe o comprimento. O sólido cresce na direção normal ao esboço.",
    "TwoLengths": "Informe um comprimento para cada lado do esboço.",
    "UpToFace": "Clique na face onde a extrusão deve parar.",
    "UpToFirst": "A extrusão para na primeira face que encontrar.",
    "UpToLast": "A extrusão segue até a última face do corpo na direção escolhida.",
    "UpToShape": "Selecione a forma (sólido ou faces) onde a extrusão deve parar.",
}
POCKET_TXT = {
    "Length": "Selecione um esboço fechado sobre a peça e informe a profundidade do corte.",
    "ThroughAll": "O corte atravessa a peça inteira na direção escolhida.",
    "TwoLengths": "Informe uma profundidade para cada lado do esboço.",
    "UpToFace": "Clique na face onde o corte deve parar.",
    "UpToFirst": "O corte para na primeira face que encontrar.",
    "UpToShape": "Selecione a forma onde o corte deve parar.",
}


def describe(obj):
    """Retorna (chave, título, desenho Iso, texto) para o objeto em edição, ou None."""
    t = obj.TypeId
    if t == "PartDesign::Pad":
        mode, mid = _side_mode(obj)
        g = I.pad(mode, mid, bool(_get(obj, "Reversed", False)))
        txt = "O comprimento é dividido igualmente para os dois lados do esboço." if mid and mode == "Length" \
            else PAD_TXT.get(mode, PAD_TXT["Length"])
        return ("pad", mode, mid), "Extrusão", g, txt
    if t == "PartDesign::Pocket":
        mode, mid = _side_mode(obj)
        g = I.pocket(mode, mid)
        txt = "A profundidade é dividida igualmente para os dois lados do esboço." if mid and mode == "Length" \
            else POCKET_TXT.get(mode, POCKET_TXT["Length"])
        return ("pocket", mode, mid), "Cavidade", g, txt
    if t in ("PartDesign::Revolution", "PartDesign::Groove"):
        groove = t.endswith("Groove")
        mode = str(_get(obj, "Type", "Angle"))
        g = I.revolution(mode, groove)
        txt = ("Gira o perfil em torno do eixo removendo material. " if groove else
               "Gira o perfil em torno do eixo criando o sólido. ") + \
              "Escolha o eixo (do esboço, da origem ou uma aresta) e o ângulo."
        return ("rev", groove, mode), "Ranhura" if groove else "Revolução", g, txt
    if t == "PartDesign::Hole":
        cut = str(_get(obj, "HoleCutType", "None"))
        cut = "Counterbore" if "Counterbore" in cut or "Cheesehead" in cut or "Socket" in cut \
            else "Countersink" if "Countersink" in cut or "Countersunk" in cut or "Counterdrill" in cut else "None"
        thr = bool(_get(obj, "Threaded", False))
        drill = str(_get(obj, "DrillPoint", "Angled"))
        g = I.hole(cut, thr, drill)
        txt = "Selecione círculos ou pontos de um esboço. Defina diâmetro, profundidade, cabeça (rebaixo ou escareado) e rosca."
        return ("hole", cut, thr, drill), "Furo", g, txt
    if t == "PartDesign::Fillet":
        return ("fillet",), "Arredondar", I.fillet_chamfer("Fillet"), \
            "Selecione as arestas (ou faces) e informe o raio do arredondamento."
    if t == "PartDesign::Chamfer":
        ct = str(_get(obj, "ChamferType", "Equal distance"))
        txt = {"Two distances": "Selecione as arestas e informe as duas distâncias do chanfro.",
               "Distance and Angle": "Selecione as arestas e informe a distância e o ângulo do chanfro."}.get(
            ct, "Selecione as arestas e informe a distância do chanfro (igual nas duas faces).")
        return ("chamfer", ct), "Chanfro", I.fillet_chamfer("Chamfer", ct), txt
    if t == "PartDesign::LinearPattern":
        return ("lin",), "Padrão linear", I.pattern("Linear"), \
            "Escolha a direção, o comprimento total (ou o espaçamento) e o número de ocorrências."
    if t == "PartDesign::PolarPattern":
        return ("pol",), "Padrão circular", I.pattern("Polar"), \
            "Escolha o eixo, o ângulo total e o número de ocorrências."
    if t == "PartDesign::Mirrored":
        return ("mir",), "Espelhar", I.pattern("Mirror"), \
            "Escolha o plano de espelhamento (do esboço, da origem ou uma face plana)."
    if t == "PartDesign::Thickness":
        return ("thick",), "Casca", I.thickness(), \
            "Selecione as faces que ficarão abertas e informe a espessura da parede."
    if t == "PartDesign::Draft":
        return ("draft",), "Ângulo de saída", I.draft(), \
            "Selecione as faces a inclinar, o plano neutro e o ângulo de saída."
    return None


class GuiaFrame(QtWidgets.QFrame):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("SeworkGuia")
        self.setAttribute(QtCore.Qt.WA_StyledBackground, True)
        self.setStyleSheet(QSS)
        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(10, 8, 10, 8)
        lay.setSpacing(4)
        head = QtWidgets.QHBoxLayout()
        self.title = QtWidgets.QLabel()
        self.title.setObjectName("SeworkGuiaTitulo")
        head.addWidget(self.title, 1)
        self.toggle = QtWidgets.QToolButton()
        self.toggle.setToolTip("Mostrar / ocultar o desenho")
        self.toggle.clicked.connect(self._toggle)
        head.addWidget(self.toggle)
        lay.addLayout(head)
        self.image = QtWidgets.QLabel()
        self.image.setAlignment(QtCore.Qt.AlignCenter)
        self.image.setMinimumSize(1, 60)
        self.image.setSizePolicy(QtWidgets.QSizePolicy.Ignored, QtWidgets.QSizePolicy.Fixed)
        self.setSizePolicy(QtWidgets.QSizePolicy.Ignored, QtWidgets.QSizePolicy.Preferred)
        lay.addWidget(self.image)
        self.legend = QtWidgets.QLabel()
        self.legend.setObjectName("SeworkGuiaLegenda")
        self.legend.setTextFormat(QtCore.Qt.RichText)
        self.legend.setWordWrap(True)
        lay.addWidget(self.legend)
        self.text = QtWidgets.QLabel()
        self.text.setObjectName("SeworkGuiaTexto")
        self.text.setWordWrap(True)
        lay.addWidget(self.text)
        self._svg = None
        self._apply_collapsed()

    def _collapsed(self):
        return FreeCAD.ParamGet(PARAM_PATH).GetBool("GuiaRecolhido", False)

    def _apply_collapsed(self):
        c = self._collapsed()
        self.image.setVisible(not c)
        self.legend.setVisible(not c)
        self.toggle.setText("mostrar ▾" if c else "ocultar ▴")

    def _toggle(self):
        FreeCAD.ParamGet(PARAM_PATH).SetBool("GuiaRecolhido", not self._collapsed())
        self._apply_collapsed()
        self._render()

    def set_content(self, title, g, text):
        self.title.setText("Como funciona · " + title)
        self.text.setText(text)
        self._svg = g.svg()
        items = getattr(g, "legend", [])
        self.legend.setText("&nbsp;&nbsp;".join(
            "<span style='color:%s'>■</span>&nbsp;%s" % (c if c.upper() != "#FFFFFF" else "#C9D2DC", t) for c, t in items))
        self._render()

    def resizeEvent(self, ev):
        super().resizeEvent(ev)
        self._render()

    def _render(self):
        if not self._svg or self._collapsed():
            return
        par = self.parentWidget()
        avail = (par.width() if par is not None else self.width()) - 40
        w = max(160, min(avail, 420))
        h = int(w * I.H / I.W)
        dpr = self.devicePixelRatioF()
        img = QtGui.QImage(int(w * dpr), int(h * dpr), QtGui.QImage.Format_ARGB32_Premultiplied)
        img.fill(0)
        r = QtSvg.QSvgRenderer(QtCore.QByteArray(self._svg.encode("utf-8")))
        p = QtGui.QPainter(img)
        p.setRenderHint(QtGui.QPainter.Antialiasing)
        r.render(p)
        p.end()
        pm = QtGui.QPixmap.fromImage(img)
        pm.setDevicePixelRatio(dpr)
        self.image.setPixmap(pm)
        self.image.setFixedHeight(h)


class Guia:
    def __init__(self):
        self.frame = None
        self.key = None
        self.timer = QtCore.QTimer(FreeCADGui.getMainWindow())
        self.timer.setInterval(350)
        self.timer.timeout.connect(self.tick)

    def start(self):
        self.timer.start()

    def stop(self):
        self.timer.stop()
        self._drop()

    def _valid(self):
        return self.frame is not None and shiboken.isValid(self.frame)

    def _drop(self):
        if self._valid():
            self.frame.hide()
            self.frame.deleteLater()
        self.frame, self.key = None, None

    def _task_panel(self):
        mw = FreeCADGui.getMainWindow()
        for w in mw.findChildren(QtWidgets.QWidget):
            if w.metaObject().className() == "Gui::TaskView::TaskPanel" and w.isVisible():
                return w
        return None

    def tick(self):
        try:
            self._tick()
        except Exception as exc:  # nunca derrubar a interface
            FreeCAD.Console.PrintLog("Sework guia: {}\n".format(exc))

    def _tick(self):
        if not FreeCAD.ParamGet(PARAM_PATH).GetBool("GuiaIlustrado", True):
            self._drop()
            return
        if not FreeCADGui.Control.activeDialog():
            if self.frame is not None:
                self._drop()
            return
        gdoc = FreeCADGui.ActiveDocument
        vp = gdoc.getInEdit() if gdoc else None
        info = describe(vp.Object) if vp is not None else None
        if info is None:
            self._drop()
            return
        key, title, g, text = info
        panel = self._task_panel()
        if panel is None:
            return
        if not self._valid() or self.frame.parent() is not panel:
            self._drop()
            lay = panel.layout()
            idx = 0
            for i in range(lay.count()):
                w = lay.itemAt(i).widget()
                if w is not None and w.isVisible():
                    idx = i
                    break
            self.frame = GuiaFrame(panel)
            lay.insertWidget(idx, self.frame)
            self.key = None
        if key != self.key:
            self.key = key
            self.frame.set_content(title, g, text)


_guia = {"obj": None}


def install():
    if _guia["obj"] is None:
        _guia["obj"] = Guia()
    _guia["obj"].start()
    return _guia["obj"]


def uninstall():
    if _guia["obj"] is not None:
        _guia["obj"].stop()


def reload():
    import importlib
    import SeworkGuia as mod
    import SeworkIlustra
    mod.uninstall()
    importlib.reload(SeworkIlustra)
    mod = importlib.reload(mod)
    return mod.install()
