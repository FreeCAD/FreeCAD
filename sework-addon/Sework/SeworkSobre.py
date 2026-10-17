# SPDX-License-Identifier: LGPL-2.1-or-later
"""Janela "Sobre o FreeCAD Sework" no padrão visual Sework."""
import os

from PySide import QtCore, QtGui, QtWidgets

HERE = os.path.dirname(os.path.abspath(__file__))
ICON_SVG = os.path.join(HERE, "Resources", "icons", "sework.svg")
ICON_PNG = os.path.join(HERE, "Resources", "icons", "sework.png")

QSS = """
#SwSobre { background: #F4F7FB; }
#SwSobreTopo { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #0A4D8C, stop:1 #002A52);
               border-bottom: 4px solid #F7BE00; }
#SwSobreTopo QLabel { background: transparent; }
#SwSobreNome { color: #FFFFFF; font-size: 20pt; font-weight: 700; }
#SwSobreVersao { color: #C9DAEA; font-size: 10pt; }
#SwSobreCard { background: #FFFFFF; border: 1px solid #DCE3EC; border-radius: 8px; }
#SwSobreCard QLabel { background: transparent; }
#SwSobreRotulo { color: #6B7785; font-size: 8.5pt; }
#SwSobreValor { color: #1F2A36; font-size: 10pt; }
#SwSobreEmpresa { color: #003A70; font-size: 11pt; font-weight: 700; }
#SwSobreLegal { color: #8A96A3; font-size: 8.5pt; }
"""


def _pix(size):
    dpr = max(QtWidgets.QApplication.instance().devicePixelRatio(), 1)
    src = ICON_SVG if os.path.isfile(ICON_SVG) else ICON_PNG
    pm = QtGui.QIcon(src).pixmap(int(size * dpr), int(size * dpr))
    pm.setDevicePixelRatio(dpr)
    return pm


class SobreDialog(QtWidgets.QDialog):
    def __init__(self, versao, base, parent=None):
        super().__init__(parent)
        self.setObjectName("SwSobre")
        self.setWindowTitle("Sobre o FreeCAD Sework")
        self.setStyleSheet(QSS)
        self.setMinimumWidth(520)
        v = QtWidgets.QVBoxLayout(self)
        v.setContentsMargins(0, 0, 0, 16)
        v.setSpacing(14)

        topo = QtWidgets.QFrame()
        topo.setObjectName("SwSobreTopo")
        th = QtWidgets.QHBoxLayout(topo)
        th.setContentsMargins(24, 20, 24, 20)
        th.setSpacing(18)
        ic = QtWidgets.QLabel()
        ic.setPixmap(_pix(84))
        th.addWidget(ic)
        tc = QtWidgets.QVBoxLayout()
        tc.setSpacing(2)
        n = QtWidgets.QLabel("FreeCAD Sework")
        n.setObjectName("SwSobreNome")
        ver = QtWidgets.QLabel("Versão " + versao)
        ver.setObjectName("SwSobreVersao")
        tc.addStretch(1)
        tc.addWidget(n)
        tc.addWidget(ver)
        tc.addStretch(1)
        th.addLayout(tc, 1)
        v.addWidget(topo)

        card = QtWidgets.QFrame()
        card.setObjectName("SwSobreCard")
        g = QtWidgets.QGridLayout(card)
        g.setContentsMargins(18, 14, 18, 14)
        g.setHorizontalSpacing(18)
        g.setVerticalSpacing(8)
        emp = QtWidgets.QLabel("SEWORK ENGENHARIA")
        emp.setObjectName("SwSobreEmpresa")
        g.addWidget(emp, 0, 0, 1, 2)
        linhas = [
            ("Área", "Engenharia mecânica e segurança do trabalho"),
            ("Local", "Patos de Minas/MG"),
            ("Contato", "sework@sework.com.br"),
            ("Base", base),
        ]
        for i, (r, val) in enumerate(linhas, start=1):
            a = QtWidgets.QLabel(r)
            a.setObjectName("SwSobreRotulo")
            b = QtWidgets.QLabel(val)
            b.setObjectName("SwSobreValor")
            b.setTextInteractionFlags(QtCore.Qt.TextSelectableByMouse)
            g.addWidget(a, i, 0)
            g.addWidget(b, i, 1)
        g.setColumnStretch(1, 1)
        wrap = QtWidgets.QHBoxLayout()
        wrap.setContentsMargins(20, 0, 20, 0)
        wrap.addWidget(card)
        v.addLayout(wrap)

        legal = QtWidgets.QLabel(
            "Distribuição do FreeCAD (freecad.org), software livre sob licença LGPL-2.1-or-later. "
            "Interface, ícones e tema Sework.")
        legal.setObjectName("SwSobreLegal")
        legal.setWordWrap(True)
        lw = QtWidgets.QHBoxLayout()
        lw.setContentsMargins(22, 0, 22, 0)
        lw.addWidget(legal)
        v.addLayout(lw)

        bb = QtWidgets.QDialogButtonBox(QtWidgets.QDialogButtonBox.Close)
        bb.rejected.connect(self.reject)
        bb.button(QtWidgets.QDialogButtonBox.Close).setText("Fechar")
        bb.button(QtWidgets.QDialogButtonBox.Close).setDefault(True)
        bw = QtWidgets.QHBoxLayout()
        bw.setContentsMargins(20, 0, 20, 0)
        bw.addWidget(bb)
        v.addLayout(bw)


def mostrar(versao, base, parent=None):
    d = SobreDialog(versao, base, parent)
    return d.exec_() if hasattr(d, "exec_") else d.exec()
