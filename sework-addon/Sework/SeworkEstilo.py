# SPDX-License-Identifier: LGPL-2.1-or-later
"""Padrão visual único Sework para todas as janelas do FreeCAD.

- Fonte Poppins (embutida em Resources/fonts, licença OFL).
- Folha de estilo global: cartões com cantos de 8 px, controles com 6 px,
  azul #003A70 como ação, amarelo #F7BE00 como destaque, fundo cinza-claro.
- Painel de tarefas (QSint) redesenhado: cabeçalho branco com filete amarelo.
- Fundo da vista 3D em cor única cinza-claro.
- Ícones de botões do painel que vêm de recursos internos (:/icons/...) trocados
  pelos ícones Sework.
"""
import os

import FreeCAD
import FreeCADGui
from PySide import QtCore, QtGui, QtWidgets

HERE = os.path.dirname(os.path.abspath(__file__))
FONT_DIR = os.path.join(HERE, "Resources", "fonts")
PARAM_PATH = "User parameter:BaseApp/Preferences/Mod/Sework"

# ---------------------------------------------------------------- tokens
NAVY = "#003A70"
NAVY_HOVER = "#0B4F8F"
YEL = "#F7BE00"
YEL_SOFT = "#FFF1BF"
INK = "#1F2A36"
INK_2 = "#4A5866"
MUTED = "#9AA7B4"
APP_BG = "#F3F5F8"      # fundo das janelas
SURFACE = "#FFFFFF"     # cartões, listas, campos
BORDER = "#D9E0E8"
BORDER_INPUT = "#C9D5E2"
HOVER = "#EAF1F8"
PRESSED = "#D3E1EF"
FUNDO_3D = "#EEF0F3"    # vista 3D, cor única
PECA = "#9EBDCA"        # cor padrão das peças (azul acinzentado)
ARESTA = "#1E2A38"      # arestas das peças

R_CARD = 8
R_CTRL = 6


def _params():
    return FreeCAD.ParamGet(PARAM_PATH)


def _ui_dir():
    return os.path.join(FreeCAD.getUserAppDataDir(), "SeworkUI")


def _url(name):
    return os.path.join(_ui_dir(), name).replace("\\", "/")


# ---------------------------------------------------------------- imagens de controles
def _chev(up, color, right=False):
    d = "M 6 4 L 10 8 L 6 12" if right else ("M 4 10 L 8 6 L 12 10" if up else "M 4 6 L 8 10 L 12 6")
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16">'
            '<path d="%s" fill="none" stroke="%s" stroke-width="2" stroke-linecap="round" '
            'stroke-linejoin="round"/></svg>' % (d, color))


def _cb(state, disabled=False, hover=False):
    border = BORDER_INPUT if disabled else (NAVY if hover else "#8EA3B8")
    if state == "off":
        body = '<rect x="1.5" y="1.5" width="13" height="13" rx="3.5" fill="%s" stroke="%s" stroke-width="1.4"/>' % (
            APP_BG if disabled else SURFACE, border)
    else:
        fill = "#B7C3CF" if disabled else NAVY
        mark = ('<path d="M 4.3 8.3 L 7 11 L 11.8 5.3" fill="none" stroke="#FFFFFF" stroke-width="2" '
                'stroke-linecap="round" stroke-linejoin="round"/>' if state == "on" else
                '<path d="M 4.5 8 H 11.5" stroke="#FFFFFF" stroke-width="2" stroke-linecap="round"/>')
        body = '<rect x="1" y="1" width="14" height="14" rx="3.5" fill="%s"/>%s' % (fill, mark)
    return '<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16">%s</svg>' % body


def _rb(on, disabled=False):
    ring = BORDER_INPUT if disabled else ("#8EA3B8" if not on else NAVY)
    dot = '<circle cx="8" cy="8" r="3.6" fill="%s"/>' % ("#B7C3CF" if disabled else NAVY) if on else ""
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16">'
            '<circle cx="8" cy="8" r="6.6" fill="%s" stroke="%s" stroke-width="1.4"/>%s</svg>'
            % (APP_BG if disabled else SURFACE, ring, dot))


UI_FILES = {
    "chev_up.svg": lambda: _chev(True, NAVY),
    "chev_up_h.svg": lambda: _chev(True, "#C99A00"),
    "chev_down.svg": lambda: _chev(False, NAVY),
    "chev_down_h.svg": lambda: _chev(False, "#C99A00"),
    "chev_right.svg": lambda: _chev(False, NAVY, right=True),
    "cb_off.svg": lambda: _cb("off"),
    "cb_off_h.svg": lambda: _cb("off", hover=True),
    "cb_off_d.svg": lambda: _cb("off", True),
    "cb_on.svg": lambda: _cb("on"),
    "cb_on_d.svg": lambda: _cb("on", True),
    "cb_mid.svg": lambda: _cb("mid"),
    "rb_off.svg": lambda: _rb(False),
    "rb_on.svg": lambda: _rb(True),
    "rb_off_d.svg": lambda: _rb(False, True),
    "rb_on_d.svg": lambda: _rb(True, True),
}


def write_ui_files():
    d = _ui_dir()
    os.makedirs(d, exist_ok=True)
    for name, fn in UI_FILES.items():
        data = fn()
        p = os.path.join(d, name)
        try:
            if os.path.isfile(p) and open(p, encoding="utf-8").read() == data:
                continue
            with open(p, "w", encoding="utf-8") as f:
                f.write(data)
        except OSError:
            pass


# ---------------------------------------------------------------- fonte
_font_state = {"family": None}


def install_font():
    if not _params().GetBool("FontePoppins", True):
        return None
    fams = []
    for f in sorted(os.listdir(FONT_DIR)) if os.path.isdir(FONT_DIR) else []:
        if f.lower().endswith(".ttf"):
            fid = QtGui.QFontDatabase.addApplicationFont(os.path.join(FONT_DIR, f))
            if fid >= 0:
                fams += QtGui.QFontDatabase.applicationFontFamilies(fid)
    if "Poppins" not in fams:
        FreeCAD.Console.PrintWarning("Sework: fonte Poppins não carregada.\n")
        return None
    app = QtWidgets.QApplication.instance()
    font = QtGui.QFont(app.font())
    size = font.pointSizeF() if font.pointSizeF() > 0 else 9.0
    font.setFamily("Poppins")
    font.setPointSizeF(_params().GetFloat("FonteTamanho", size))
    font.setHintingPreference(QtGui.QFont.PreferNoHinting)
    app.setFont(font)
    # O Windows define fontes por classe (menus, listas, abas, dicas…); sobrescreve todas.
    for cls in ("QMenu", "QMenuBar", "QMessageBox", "QTipLabel", "QToolTip", "QStatusBar", "QAbstractItemView",
                "QHeaderView", "QDockWidget", "QMdiSubWindow", "QTabBar", "QToolButton", "QPushButton",
                "QLabel", "QComboBox", "QLineEdit", "QAbstractSpinBox", "QCheckBox", "QRadioButton", "QGroupBox"):
        app.setFont(font, cls)
    _font_state["family"] = "Poppins"
    return font


# ---------------------------------------------------------------- vista 3D
def _rgba_uint(hexcolor):
    c = QtGui.QColor(hexcolor)
    return (c.red() << 24) | (c.green() << 16) | (c.blue() << 8) | 0xFF


def apply_background(force=False):
    """Fundo da vista 3D em cor única. Aplica uma vez (o usuário pode mudar depois)."""
    p = _params()
    if p.GetBool("CoresVista2", False) and not force:
        return
    v = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
    v.SetBool("Simple", True)
    v.SetBool("Gradient", False)
    v.SetBool("RadialGradient", False)
    v.SetUnsigned("BackgroundColor", _rgba_uint(FUNDO_3D))
    v.SetBool("RandomColor", False)
    v.SetUnsigned("DefaultShapeColor", _rgba_uint(PECA))
    v.SetUnsigned("DefaultShapeLineColor", _rgba_uint(ARESTA))
    p.SetBool("CoresVista2", True)
    # vistas já abertas
    try:
        for doc in FreeCAD.listDocuments().values():
            gd = FreeCADGui.getDocument(doc.Name)
            for view in gd.mdiViewsOfType("Gui::View3DInventor") if gd else []:
                try:
                    view.setBackgroundColor(QtGui.QColor(FUNDO_3D)) if hasattr(view, "setBackgroundColor") else None
                except Exception:
                    pass
    except Exception:
        pass


# ---------------------------------------------------------------- cores da vista 3D e do esboço
VIEW_COLORS = {
    # seleção e pré-seleção
    "HighlightColor": "#F7BE00",
    "SelectionColor": "#0B5CAD",
    # eixos
    "AxisXColor": "#C2412D",
    "AxisYColor": "#2E8B6A",
    "AxisZColor": "#2F6FB5",
    # esboço fora da edição
    "SketchEdgeColor": "#003A70",
    "SketchVertexColor": "#003A70",
    # esboço em edição
    "EditedEdgeColor": "#003A70",
    "EditedVertexColor": "#1E2A38",
    "ConstructionColor": "#4D7FAD",
    "InternalAlignedGeoColor": "#8FAFD0",
    "FullyConstraintElementColor": "#2E8B6A",
    "FullyConstraintConstructionElementColor": "#6FB59A",
    "FullyConstraintInternalAlignmentColor": "#A6D4C3",
    "FullyConstrainedColor": "#2E8B6A",
    "InvalidSketchColor": "#B5523F",
    "ConstrainedDimColor": "#B5523F",
    "ConstrainedIcoColor": "#B5523F",
    "NonDrivingConstrDimColor": "#4D7FAD",
    "ExprBasedConstrDimColor": "#D97A00",
    "DeactivatedConstrDimColor": "#9AA7B4",
    "ExternalColor": "#8E5BB5",
    "ExternalDefiningColor": "#6A3D9A",
    "CursorTextColor": "#003A70",
    "InformationColor": "#4A5866",
}
SKETCH_GENERAL = {"GridLineColor": "#C9D2DC"}
TREE_COLORS = {"TreeActiveColor": "#FFE9A6", "TreeEditColor": "#D3E1EF"}
TECHDRAW_COLORS = {"Background": "#E4E8ED"}
NAVICUBE = {
    "colors": {"BaseColor": ("#D3E1EF", 0xE6), "EmphaseColor": ("#003A70", 0xFF), "HiliteColor": ("#F7BE00", 0xFF)},
    "text": {"TextFront": "FRENTE", "TextRear": "TRÁS", "TextTop": "TOPO", "TextBottom": "BAIXO",
             "TextLeft": "ESQUERDA", "TextRight": "DIREITA", "FontString": "Poppins"},
    "int": {"InactiveOpacity": 75, "FontWeight": 63},
    "float": {"BorderWidth": 1.4, "FontZoom": 0.3},
}
VIEW_COLORS_VERSION = 3


def _rgba(hexcolor, alpha=0xFF):
    c = QtGui.QColor(hexcolor)
    return (c.red() << 24) | (c.green() << 16) | (c.blue() << 8) | alpha


def apply_view_colors(force=False):
    """Cubo de navegação, seleção, eixos e esboço nas cores Sework (uma vez por versão)."""
    p = _params()
    if p.GetInt("CoresVistaVersao", 0) >= VIEW_COLORS_VERSION and not force:
        return
    v = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
    for k, c in VIEW_COLORS.items():
        v.SetUnsigned(k, _rgba(c))
    g = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Sketcher/General")
    for k, c in SKETCH_GENERAL.items():
        g.SetUnsigned(k, _rgba(c))
    t = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/TreeView")
    for k, c in TREE_COLORS.items():
        t.SetUnsigned(k, _rgba(c))
    td = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/TechDraw/Colors")
    for k, c in TECHDRAW_COLORS.items():
        td.SetUnsigned(k, _rgba(c))
    n = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/NaviCube")
    for k, (c, a) in NAVICUBE["colors"].items():
        n.SetUnsigned(k, _rgba(c, a))
    for k, t in NAVICUBE["text"].items():
        n.SetString(k, t)
    for k, i in NAVICUBE["int"].items():
        n.SetInt(k, i)
    for k, f in NAVICUBE["float"].items():
        n.SetFloat(k, f)
    p.SetInt("CoresVistaVersao", VIEW_COLORS_VERSION)


# ---------------------------------------------------------------- folha de estilo
def qss():
    u = _url
    return """
/* ===== Sework: padrão único de janelas ===== */
QMainWindow, QDialog, QMessageBox, QWizard {{ background: {APP_BG}; }}
QMainWindow::separator {{ background: {APP_BG}; width: 5px; height: 5px; }}
QSplitter::handle {{ background: {APP_BG}; }}
QLabel {{ color: {INK}; }}

QDockWidget {{ color: {NAVY} }}
QDockWidget::title {{ background: {APP_BG}; padding: 7px 10px; border: none; }}
QDockWidget > QWidget {{ background: {APP_BG}; }}

/* abas */
QTabWidget::pane {{ background: {SURFACE}; border: 1px solid {BORDER}; border-radius: {R_CARD}px; top: -1px; }}
QTabBar::tab {{ background: transparent; color: {INK_2}; padding: 6px 14px; margin-right: 2px; border: none;
               border-bottom: 2px solid transparent; }}
QTabBar::tab:selected {{ color: {NAVY}; border-bottom: 2px solid {YEL}; }}
QTabBar::tab:hover:!selected {{ color: {NAVY}; background: {HOVER}; border-top-left-radius: {R_CTRL}px;
                               border-top-right-radius: {R_CTRL}px; }}

/* listas, árvores e tabelas */
QTreeView, QListView, QTableView, QTreeWidget, QListWidget, QTableWidget {{
    background: {SURFACE}; alternate-background-color: #F7F9FC; color: {INK};
    border: 1px solid {BORDER}; border-radius: {R_CARD}px; padding: 2px;
    selection-background-color: {YEL_SOFT}; selection-color: {INK}; outline: 0; }}
QTreeView::item, QListView::item {{ padding: 2px 2px; }}
QTreeView::item:hover, QListView::item:hover, QTableView::item:hover {{ background: #EEF3F9; color: {INK}; }}
QTreeView::item:selected, QListView::item:selected, QTableView::item:selected {{ background: {YEL_SOFT}; color: {INK}; }}
QHeaderView {{ background: transparent; border: none; }}
QHeaderView::section {{ background: #F5F8FC; color: {NAVY}; font-weight: 600; padding: 5px 8px; border: none;
                        border-bottom: 1px solid {BORDER}; border-right: 1px solid #EDF1F5; }}
QTableCornerButton::section {{ background: #F5F8FC; border: none; }}
Gui--PropertyEditor--PropertyEditor {{ qproperty-groupBackground: #EAF1F8; qproperty-groupTextColor: {NAVY}; }}

/* árvore do modelo */
Gui--TreeWidget {{ padding: 4px 2px; }}
Gui--TreeWidget::item {{ padding: 3px 2px; border-radius: 4px; }}
QTreeView::branch:selected {{ background: {YEL_SOFT}; }}
QTreeView::branch:has-children:!has-siblings:closed, QTreeView::branch:closed:has-children:has-siblings {{
    border-image: none; image: url({right}); }}
QTreeView::branch:open:has-children:!has-siblings, QTreeView::branch:open:has-children:has-siblings {{
    border-image: none; image: url({down}); }}
Gui--PropertyEditor--PropertyEditor::item {{ padding: 2px 2px; }}

/* campos */
QLineEdit, QAbstractSpinBox, QComboBox, QPlainTextEdit, QTextEdit, QTextBrowser {{
    background: {SURFACE}; color: {INK}; border: 1px solid {BORDER_INPUT}; border-radius: {R_CTRL}px;
    selection-background-color: {NAVY}; selection-color: #FFFFFF; }}
QLineEdit, QAbstractSpinBox, QComboBox {{ padding: 3px 6px; min-height: 18px; }}
QLineEdit:hover, QAbstractSpinBox:hover, QComboBox:hover {{ border-color: #8EA3B8; }}
QLineEdit:focus, QAbstractSpinBox:focus, QComboBox:focus, QPlainTextEdit:focus, QTextEdit:focus {{ border-color: {NAVY}; }}
QLineEdit:disabled, QAbstractSpinBox:disabled, QComboBox:disabled {{ background: {APP_BG}; color: {MUTED}; border-color: #E1E7EE; }}
QComboBox {{ padding-right: 22px; }}
QComboBox::drop-down {{ subcontrol-origin: padding; subcontrol-position: center right; width: 22px; border: none;
                       background: transparent; }}
QComboBox::down-arrow {{ image: url({down}); width: 12px; height: 12px; }}
QComboBox::down-arrow:on {{ image: url({up}); }}
QAbstractSpinBox {{ padding-right: 20px; }}
QAbstractSpinBox::up-button {{ subcontrol-origin: border; subcontrol-position: top right; width: 18px; border: none;
                              background: transparent; margin: 2px 2px 0 0; }}
QAbstractSpinBox::down-button {{ subcontrol-origin: border; subcontrol-position: bottom right; width: 18px; border: none;
                                background: transparent; margin: 0 2px 2px 0; }}
QAbstractSpinBox::up-button:hover, QAbstractSpinBox::down-button:hover {{ background: {HOVER}; border-radius: 3px; }}
QAbstractSpinBox::up-arrow {{ image: url({up}); width: 10px; height: 10px; }}
QAbstractSpinBox::down-arrow {{ image: url({down}); width: 10px; height: 10px; }}
QAbstractSpinBox::up-arrow:disabled, QAbstractSpinBox::down-arrow:disabled {{ image: none; }}
QComboBox QAbstractItemView {{ background: {SURFACE}; border: 1px solid {BORDER}; border-radius: 0; padding: 2px;
                               selection-background-color: {HOVER}; selection-color: {NAVY}; }}
/* itens da lista suspensa (o padrão do FreeCAD deixa o item em destaque branco sobre branco) */
QComboBox::item {{ background: {SURFACE}; color: {INK} }}
QComboBox::item:alternate {{ background: {SURFACE}; }}
QComboBox::item:selected {{ background: {HOVER}; color: {NAVY}; }}
QComboBox::item:checked {{ background: {YEL_SOFT}; color: {INK} }}
QComboBox::item:checked:selected {{ background: {HOVER}; color: {NAVY}; }}
QComboBox QAbstractItemView::item {{ background: {SURFACE}; color: {INK} }}
QComboBox QAbstractItemView::item:hover, QComboBox QAbstractItemView::item:selected {{ background: {HOVER}; color: {NAVY}; }}
QComboBox QAbstractItemView:selected, QComboBox QAbstractItemView:alternate {{ background: {SURFACE}; }}
/* editores dentro de listas (painel de propriedades): sem moldura */
QAbstractItemView QLineEdit, QAbstractItemView QAbstractSpinBox, QAbstractItemView QComboBox {{
    border: none; border-radius: 0; padding: 0 2px; min-height: 0; background: {SURFACE}; }}
QAbstractItemView QAbstractSpinBox, QAbstractItemView QComboBox {{ padding-right: 18px; }}

/* botões */
QPushButton {{ background: {SURFACE}; color: {NAVY}; border: 1px solid #B9CDE2; border-radius: {R_CTRL}px;
              padding: 5px 14px }}
QPushButton:hover {{ background: {HOVER}; border-color: #7FA3C9; }}
QPushButton:pressed, QPushButton:checked {{ background: {PRESSED}; }}
QPushButton:default {{ background: {NAVY}; color: #FFFFFF; border-color: {NAVY}; }}
QPushButton:default:hover {{ background: {NAVY_HOVER}; border-color: {NAVY_HOVER}; }}
QPushButton:disabled {{ background: {APP_BG}; color: {MUTED}; border-color: #E1E7EE; }}
QAbstractItemView QPushButton {{ padding: 0 4px; border-radius: 3px; font-weight: 400; }}
QDialogButtonBox QPushButton {{ min-width: 72px; }}

/* caixas de grupo */
QGroupBox {{ background: {SURFACE}; border: 1px solid {BORDER}; border-radius: {R_CARD}px;
            margin-top: 14px; padding: 10px 8px 8px 8px; }}
QGroupBox::title {{ subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 4px;
                   color: {NAVY}; font-weight: 600; background: transparent; }}

/* caixas de seleção e opções */
QCheckBox, QRadioButton {{ color: {INK}; spacing: 7px; }}
QCheckBox::indicator, QAbstractItemView::indicator, QGroupBox::indicator {{ width: 16px; height: 16px; }}
QCheckBox::indicator:unchecked, QAbstractItemView::indicator:unchecked, QGroupBox::indicator:unchecked {{ image: url({cb_off}); }}
QCheckBox::indicator:unchecked:hover, QGroupBox::indicator:unchecked:hover {{ image: url({cb_off_h}); }}
QCheckBox::indicator:checked, QAbstractItemView::indicator:checked, QGroupBox::indicator:checked {{ image: url({cb_on}); }}
QCheckBox::indicator:indeterminate, QAbstractItemView::indicator:indeterminate {{ image: url({cb_mid}); }}
QCheckBox::indicator:unchecked:disabled {{ image: url({cb_off_d}); }}
QCheckBox::indicator:checked:disabled {{ image: url({cb_on_d}); }}
QRadioButton::indicator {{ width: 16px; height: 16px; }}
QRadioButton::indicator:unchecked {{ image: url({rb_off}); }}
QRadioButton::indicator:checked {{ image: url({rb_on}); }}
QRadioButton::indicator:unchecked:disabled {{ image: url({rb_off_d}); }}
QRadioButton::indicator:checked:disabled {{ image: url({rb_on_d}); }}

/* barras de rolagem finas */
QScrollBar:vertical {{ background: transparent; width: 11px; margin: 2px; border: none; }}
QScrollBar:horizontal {{ background: transparent; height: 11px; margin: 2px; border: none; }}
QScrollBar::handle:vertical {{ background: #C3D0DD; border-radius: 3px; min-height: 28px; margin: 0 1px; }}
QScrollBar::handle:horizontal {{ background: #C3D0DD; border-radius: 3px; min-width: 28px; margin: 1px 0; }}
QScrollBar::handle:hover {{ background: #7FA3C9; }}
QScrollBar::add-line, QScrollBar::sub-line {{ width: 0; height: 0; border: none; background: transparent; }}
QScrollBar::add-page, QScrollBar::sub-page {{ background: transparent; }}

QProgressBar {{ background: #E6EBF1; border: none; border-radius: {R_CTRL}px; text-align: center; color: {INK}; min-height: 12px; }}
QProgressBar::chunk {{ background: {NAVY}; border-radius: {R_CTRL}px; }}

/* ===== painel de tarefas ===== */
Gui--TaskView--TaskView, Gui--TaskView--TaskView QScrollArea, Gui--TaskView--TaskView QScrollArea > QWidget > QWidget {{
    background: {APP_BG}; border: none; }}
Gui--TaskView--TaskPanel QSint--ActionPanel, QFrame[class="panel"] {{ background: transparent; }}
QSint--ActionGroup {{ background: transparent; border: none; }}
QSint--ActionGroup QFrame[class="header"] {{
    background: {SURFACE}; border: 1px solid {BORDER}; border-bottom: 2px solid {YEL};
    border-top-left-radius: {R_CARD}px; border-top-right-radius: {R_CARD}px;
    border-bottom-left-radius: 0; border-bottom-right-radius: 0; }}
QSint--ActionGroup QToolButton[class="header"] {{ color: {NAVY}; font-weight: 600; background: transparent;
                                                 border: none; text-align: left; padding: 2px; }}
QSint--ActionGroup QFrame[class="header"] QLabel[fold="true"] {{ padding: 1px; image: url({up}); }}
QSint--ActionGroup QFrame[class="header"] QLabel[fold="true"]:hover {{ padding: 1px; image: url({up_h}); }}
QSint--ActionGroup QFrame[class="header"] QLabel[fold="false"] {{ padding: 1px; image: url({down}); }}
QSint--ActionGroup QFrame[class="header"] QLabel[fold="false"]:hover {{ padding: 1px; image: url({down_h}); }}
QSint--ActionGroup QFrame[class="content"] {{
    background: {SURFACE}; border: 1px solid {BORDER}; border-top: none;
    border-top-left-radius: 0; border-top-right-radius: 0;
    border-bottom-left-radius: {R_CARD}px; border-bottom-right-radius: {R_CARD}px; }}
QSint--ActionGroup QFrame[class="content"][header="false"] {{ border-top: 1px solid {BORDER}; border-radius: {R_CARD}px; }}
QSint--ActionGroup QFrame[class="content"] QTreeView, QSint--ActionGroup QFrame[class="content"] QListView,
QSint--ActionGroup QFrame[class="content"] QTableView {{ background: {SURFACE}; border-radius: {R_CTRL}px; }}
QSint--ActionGroup QFrame[class="content"] QToolButton {{
    background: {SURFACE}; color: {NAVY}; border: 1px solid #B9CDE2; border-radius: {R_CTRL}px;
    padding: 3px 4px; min-height: 18px; }}
QSint--ActionGroup QFrame[class="content"] QToolButton:hover {{ background: {HOVER}; border-color: #7FA3C9; }}
QSint--ActionGroup QFrame[class="content"] QToolButton:pressed {{ background: {PRESSED}; }}
QSint--ActionGroup QFrame[class="content"] QToolButton:checked {{ background: {YEL_SOFT}; border-color: #E0A100; }}
QSint--ActionGroup QFrame[class="content"] QToolButton:disabled {{ background: {APP_BG}; color: {MUTED}; border-color: #E1E7EE; }}
QSint--ActionGroup QToolButton::menu-button {{ border: none; border-left: 1px solid #D5DEE8; width: 18px;
    border-top-right-radius: {R_CTRL}px; border-bottom-right-radius: {R_CTRL}px; background: transparent; }}
QSint--ActionGroup QFrame[class="content"] > QWidget > QPushButton {{ padding: 4px 10px; border-radius: {R_CTRL}px; }}
Gui--TaskView--TaskView QPushButton {{ padding: 5px 16px; }}
QSint--ActionGroup QGroupBox {{ background: #FAFBFD; }}
""".format(
        APP_BG=APP_BG, SURFACE=SURFACE, BORDER=BORDER, BORDER_INPUT=BORDER_INPUT, NAVY=NAVY, NAVY_HOVER=NAVY_HOVER,
        YEL=YEL, YEL_SOFT=YEL_SOFT, INK=INK, INK_2=INK_2, MUTED=MUTED, HOVER=HOVER, PRESSED=PRESSED,
        R_CARD=R_CARD, R_CTRL=R_CTRL,
        cb_off=u("cb_off.svg"), cb_off_h=u("cb_off_h.svg"), cb_off_d=u("cb_off_d.svg"), cb_on=u("cb_on.svg"),
        cb_on_d=u("cb_on_d.svg"), cb_mid=u("cb_mid.svg"), rb_off=u("rb_off.svg"), rb_on=u("rb_on.svg"),
        rb_off_d=u("rb_off_d.svg"), rb_on_d=u("rb_on_d.svg"),
        up=u("chev_up.svg"), up_h=u("chev_up_h.svg"), down=u("chev_down.svg"), down_h=u("chev_down_h.svg"),
        right=u("chev_right.svg"))


# ---------------------------------------------------------------- ícones de botões do painel
# Botões cujos ícones vêm de recursos internos (:/icons/...) e não passam pelo tema.
BUTTON_ICONS = {
    "showHideButton": "Std_ToggleVisibility",
    "settingsButton": "Sketcher_Settings",
    "manualUpdate": "view-refresh",
}


def _icon_file(name):
    d = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Bitmaps").GetString("CustomPathSework", "")
    p = os.path.join(d, name + ".svg") if d else ""
    return p if p and os.path.isfile(p) else None


_icon_cache = {}


def _in_task_panel(w):
    p = w.parentWidget()
    while p is not None:
        cn = p.metaObject().className()
        if cn.startswith("Gui::TaskView") or cn.startswith("QSint::"):
            return True
        p = p.parentWidget()
    return False


def fix_task_icons():
    mw = FreeCADGui.getMainWindow()
    if mw is None:
        return
    for btn in mw.findChildren(QtWidgets.QToolButton):
        name = btn.objectName()
        if name not in BUTTON_ICONS or btn.property("seworkIcon") or not btn.isVisible():
            continue
        if not _in_task_panel(btn):
            continue
        f = _icon_file(BUTTON_ICONS[name])
        if not f:
            continue
        if f not in _icon_cache:
            _icon_cache[f] = QtGui.QIcon(f)
        btn.setIcon(_icon_cache[f])
        btn.setProperty("seworkIcon", True)


_timer = {"t": None}


def start_icon_fixer():
    if _timer["t"] is not None:
        return
    t = QtCore.QTimer()
    t.setInterval(400)
    t.timeout.connect(lambda: _safe(fix_task_icons))
    t.start()
    _timer["t"] = t


def _safe(fn):
    try:
        fn()
    except Exception:
        pass


def install():
    """Chamado na inicialização do addon."""
    if not _params().GetBool("EstiloJanelas", True):
        return
    write_ui_files()
    install_font()
    apply_background()
    apply_view_colors()
    start_icon_fixer()


def enabled():
    return _params().GetBool("EstiloJanelas", True)
