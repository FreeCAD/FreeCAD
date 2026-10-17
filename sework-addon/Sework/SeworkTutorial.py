# SPDX-License-Identifier: LGPL-2.1-or-later
"""Tutorial interativo "Primeira peça" — 10 passos com detecção automática.

O painel acompanha o documento: cada passo fica concluído sozinho quando o usuário faz a
ação (criar corpo, esboço, retângulo, cotas, extrusão, furo, arredondamento, salvar).
"Mostrar onde" leva a faixa até a aba certa e faz o botão piscar em amarelo.
"""
import os

import FreeCAD
import FreeCADGui
from PySide import QtCore, QtGui, QtWidgets

import SeworkEstilo

HERE = os.path.dirname(os.path.abspath(__file__))
ICON = os.path.join(HERE, "Resources", "icons", "tutorial.svg")


# ------------------------------------------------------------------ verificações do documento
def _doc():
    return FreeCAD.ActiveDocument


def _objs(tipo):
    d = _doc()
    return [o for o in d.Objects if o.isDerivedFrom(tipo)] if d else []


def _valido(o):
    try:
        return o.Shape.isValid() and o.Shape.Volume > 1e-6
    except Exception:
        return False


def _esbocos_no_corpo():
    return [s for s in _objs("Sketcher::SketchObject")
            if any(p.isDerivedFrom("PartDesign::Body") for p in s.InList)]


def _tem_retangulo():
    for s in _esbocos_no_corpo():
        try:
            linhas = [g for g in s.Geometry if g.TypeId == "Part::GeomLineSegment"]
            if len(linhas) >= 4 and any(w.isClosed() for w in s.Shape.Wires):
                return True
        except Exception:
            pass
    return False


def _tem_cotas():
    for s in _esbocos_no_corpo():
        n = sum(1 for c in s.Constraints if c.Type in ("DistanceX", "DistanceY", "Distance") and c.Driving)
        if n >= 2:
            return True
    return False


def _fora_da_edicao():
    try:
        gd = FreeCADGui.ActiveDocument
        return gd is not None and gd.getInEdit() is None
    except Exception:
        return True


def _salvo():
    d = _doc()
    return bool(d and d.FileName)


# (título, texto, aba da faixa, comandos para destacar, verificação)
PASSOS = [
    ("Criar um documento",
     "Clique em Novo, na barra de acesso rápido no canto superior direito, ou em Peça na tela inicial.",
     None, ["Std_New"], lambda: _doc() is not None),
    ("Criar o corpo da peça",
     "Na aba Peça, clique em Novo corpo. O corpo guarda todas as operações da peça, em ordem.",
     "peca", ["PartDesign_Body"], lambda: bool(_objs("PartDesign::Body"))),
    ("Criar um esboço no plano XY",
     "Clique em Esboço e escolha XY_Plane (plano de cima). Esboços nos planos da origem não se perdem "
     "quando o modelo muda.",
     "peca", ["PartDesign_CompSketches", "PartDesign_NewSketch"], lambda: bool(_esbocos_no_corpo())),
    ("Desenhar um retângulo",
     "Na aba Esboço, clique em Retângulo e depois em dois cantos opostos na vista.",
     "esboco", ["Sketcher_CompCreateRectangles", "Sketcher_CreateRectangle"], _tem_retangulo),
    ("Dar as medidas",
     "Clique em Cota, selecione uma linha horizontal e digite 60 mm; depois uma vertical e digite 40 mm.",
     "esboco", ["Sketcher_CompDimensionTools", "Sketcher_Dimension"], _tem_cotas),
    ("Concluir o esboço",
     "Clique em Concluir esboço para voltar ao modelo.",
     "esboco", ["Sketcher_LeaveSketch"], lambda: _tem_cotas() and _fora_da_edicao()),
    ("Extrudar 10 mm",
     "Com o esboço selecionado, clique em Extrusão, informe 10 mm e clique em OK.",
     "peca", ["PartDesign_Pad"], lambda: any(_valido(o) for o in _objs("PartDesign::Pad"))),
    ("Fazer um furo",
     "Crie outro esboço no plano XY, desenhe um círculo no meio do retângulo e conclua. Clique em "
     "Cavidade, escolha o tipo Através de tudo e marque Simétrico ao plano.",
     "peca", ["PartDesign_Pocket"],
     lambda: any(_valido(o) for o in _objs("PartDesign::Pocket") + _objs("PartDesign::Hole"))),
    ("Arredondar as arestas",
     "Selecione as 4 arestas verticais do bloco (Ctrl + clique) e clique em Arredondar. Raio de 5 mm. "
     "Arredondamentos ficam melhor no fim do histórico.",
     "peca", ["PartDesign_Fillet"],
     lambda: any(_valido(o) for o in _objs("PartDesign::Fillet") + _objs("PartDesign::Chamfer"))),
    ("Salvar o projeto",
     "Clique em Salvar e escolha a pasta e o nome do arquivo.",
     None, ["Std_Save"], _salvo),
]


def estado():
    """Lista de bool: passo concluído?"""
    out = []
    for _t, _x, _a, _c, chk in PASSOS:
        try:
            out.append(bool(chk()))
        except Exception:
            out.append(False)
    return out


def passo_atual(feitos):
    for i, f in enumerate(feitos):
        if not f:
            return i
    return len(feitos)


# ------------------------------------------------------------------ destaque na faixa
def destacar(aba, comandos):
    try:
        import SeworkRibbon
        rb = SeworkRibbon._state.get("ribbon")
    except Exception:
        rb = None
    if rb is None:
        return False
    if aba:
        rb.select_tab(aba)
    QtWidgets.QApplication.processEvents()
    alvo = []
    for cmd in comandos:
        acts = list(SeworkRibbon._cmd_actions(cmd))
        proprio = getattr(SeworkRibbon, "_OWN_ACTIONS", {}).get(cmd)
        if proprio is not None:
            acts.append(proprio)       # botões da barra rápida usam ação própria
        for b in rb.findChildren(QtWidgets.QToolButton):
            if any(a in acts for a in getattr(b, "_actions", [])) and b.isVisible():
                alvo.append(b)
        if alvo:
            break
    if not alvo:
        return False
    estilo = "QToolButton { background: #FFF1BF; border: 2px solid #F7BE00; border-radius: 5px; }"
    estado_pisca = {"n": 0}

    def piscar():
        on = estado_pisca["n"] % 2 == 0
        for b in alvo:
            try:
                b.setStyleSheet(estilo if on else "")
            except RuntimeError:
                pass
        estado_pisca["n"] += 1
        if estado_pisca["n"] < 8:
            QtCore.QTimer.singleShot(350, piscar)
        else:
            for b in alvo:
                try:
                    b.setStyleSheet("")
                except RuntimeError:
                    pass
    piscar()
    return True


# ------------------------------------------------------------------ painel
QSS = """
#SwTut { background: #F3F5F8; }
#SwTut QLabel { background: transparent; color: #1F2A36; }
#SwTutTopo { background: #FFFFFF; border-bottom: 2px solid #F7BE00; }
#SwTutTitulo { color: #003A70; font-size: 12pt; font-weight: 700; }
#SwTutSub { color: #5B6876; font-size: 8.5pt; }
#SwTutAtual { background: #FFFFFF; border: 1px solid #7FA3C9; border-radius: 8px; }
#SwTutPassoTit { color: #003A70; font-size: 11pt; font-weight: 700; }
#SwTutPassoTxt { color: #1F2A36; font-size: 9.5pt; }
#SwTutItem { color: #4A5866; font-size: 9pt; padding: 2px 0; }
#SwTutItem[feito="true"] { color: #2E8B6A; }
#SwTutItem[atual="true"] { color: #003A70; font-weight: 700; }
#SwTutFim { background: #E5F3ED; color: #1E5C46; border-left: 3px solid #2E8B6A; border-radius: 6px; padding: 8px; }
QProgressBar { min-height: 8px; max-height: 8px; }
QPushButton#SwPrimario { background: #003A70; color: #FFFFFF; border: 1px solid #003A70; font-weight: 600; padding: 6px 16px; }
QPushButton#SwPrimario:hover { background: #0B4F8F; }
"""


class Tutorial(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("SwTut")
        self.setStyleSheet(SeworkEstilo.tema(QSS))
        raiz = QtWidgets.QVBoxLayout(self)
        raiz.setContentsMargins(0, 0, 0, 0)
        raiz.setSpacing(0)
        topo = QtWidgets.QFrame()
        topo.setObjectName("SwTutTopo")
        tl = QtWidgets.QHBoxLayout(topo)
        tl.setContentsMargins(12, 10, 12, 10)
        ic = QtWidgets.QLabel()
        ic.setPixmap(QtGui.QIcon(ICON).pixmap(34, 34))
        tl.addWidget(ic)
        tv = QtWidgets.QVBoxLayout()
        tv.setSpacing(0)
        t = QtWidgets.QLabel("Tutorial: primeira peça")
        t.setObjectName("SwTutTitulo")
        self.sub = QtWidgets.QLabel("Uma placa com furo, em 10 passos. Cada passo conclui sozinho quando você o faz.")
        self.sub.setObjectName("SwTutSub")
        self.sub.setWordWrap(True)
        tv.addWidget(t)
        tv.addWidget(self.sub)
        tl.addLayout(tv, 1)
        raiz.addWidget(topo)

        corpo = QtWidgets.QWidget()
        cv = QtWidgets.QVBoxLayout(corpo)
        cv.setContentsMargins(12, 12, 12, 12)
        cv.setSpacing(10)
        self.barra = QtWidgets.QProgressBar()
        self.barra.setRange(0, len(PASSOS))
        self.barra.setTextVisible(False)
        self.lb_prog = QtWidgets.QLabel("")
        self.lb_prog.setObjectName("SwTutSub")
        cv.addWidget(self.lb_prog)
        cv.addWidget(self.barra)

        self.cartao = QtWidgets.QFrame()
        self.cartao.setObjectName("SwTutAtual")
        kv = QtWidgets.QVBoxLayout(self.cartao)
        kv.setContentsMargins(12, 10, 12, 10)
        kv.setSpacing(6)
        self.lb_tit = QtWidgets.QLabel("")
        self.lb_tit.setObjectName("SwTutPassoTit")
        self.lb_txt = QtWidgets.QLabel("")
        self.lb_txt.setObjectName("SwTutPassoTxt")
        self.lb_txt.setWordWrap(True)
        self.bt_onde = QtWidgets.QPushButton("Mostrar onde")
        self.bt_onde.setObjectName("SwPrimario")
        self.bt_onde.clicked.connect(self._mostrar_onde)
        kv.addWidget(self.lb_tit)
        kv.addWidget(self.lb_txt)
        hl = QtWidgets.QHBoxLayout()
        hl.addWidget(self.bt_onde)
        hl.addStretch(1)
        kv.addLayout(hl)
        cv.addWidget(self.cartao)

        self.fim = QtWidgets.QLabel("Pronto! Você modelou a primeira peça: esboço, cotas, extrusão, furo e "
                                    "arredondamento. Próximo passo sugerido: gerar o desenho técnico na aba "
                                    "Desenho, ou simular a peça com o Assistente de simulação.")
        self.fim.setObjectName("SwTutFim")
        self.fim.setWordWrap(True)
        cv.addWidget(self.fim)

        self.itens = []
        lista = QtWidgets.QVBoxLayout()
        lista.setSpacing(0)
        for i, (tit, _x, _a, _c, _k) in enumerate(PASSOS):
            lb = QtWidgets.QLabel()
            lb.setObjectName("SwTutItem")
            self.itens.append(lb)
            lista.addWidget(lb)
        cv.addLayout(lista)
        cv.addStretch(1)
        raiz.addWidget(corpo, 1)

        self._ultimo = None
        self.timer = QtCore.QTimer(self)
        self.timer.setInterval(700)
        self.timer.timeout.connect(self.atualizar)
        self.timer.start()
        self.atualizar()

    def _mostrar_onde(self):
        i = passo_atual(estado())
        if i < len(PASSOS):
            _t, _x, aba, cmds, _k = PASSOS[i]
            if not destacar(aba, cmds):
                QtWidgets.QToolTip.showText(QtGui.QCursor.pos(), "Esse botão aparece depois do passo anterior.")

    def atualizar(self):
        feitos = estado()
        if feitos == self._ultimo:
            return
        self._ultimo = feitos
        i = passo_atual(feitos)
        n = sum(feitos)
        self.barra.setValue(n)
        self.lb_prog.setText("{} de {} passos concluídos".format(n, len(PASSOS)))
        terminou = i >= len(PASSOS)
        self.cartao.setVisible(not terminou)
        self.fim.setVisible(terminou)
        if not terminou:
            tit, txt, _a, _c, _k = PASSOS[i]
            self.lb_tit.setText("Passo {} · {}".format(i + 1, tit))
            self.lb_txt.setText(txt)
        for j, lb in enumerate(self.itens):
            lb.setText("{}  {}".format("✓" if feitos[j] else "{:>2}.".format(j + 1), PASSOS[j][0]))
            lb.setProperty("feito", "true" if feitos[j] else "false")
            lb.setProperty("atual", "true" if j == i else "false")
            lb.style().unpolish(lb)
            lb.style().polish(lb)


_estado = {"dock": None}


def mostrar():
    mw = FreeCADGui.getMainWindow()
    dock = _estado["dock"]
    try:
        vivo = dock is not None and dock.objectName() is not None
    except RuntimeError:
        vivo = False
    if not vivo:
        for velho in mw.findChildren(QtWidgets.QDockWidget, "SeworkTutorial"):
            velho.setParent(None)
            velho.deleteLater()
        dock = QtWidgets.QDockWidget("Tutorial: primeira peça", mw)
        dock.setObjectName("SeworkTutorial")
        dock.setWidget(Tutorial(dock))
        dock.setMinimumWidth(330)
        mw.addDockWidget(QtCore.Qt.RightDockWidgetArea, dock)
        _estado["dock"] = dock
    dock.show()
    dock.raise_()
    return dock


class CmdTutorial:
    def GetResources(self):
        return {"Pixmap": ICON, "MenuText": "Tutorial: primeira peça",
                "ToolTip": "Passo a passo interativo: esboço, cotas, extrusão, furo, arredondamento e salvar"}

    def IsActive(self):
        return True

    def Activated(self):
        mostrar()
