# SPDX-License-Identifier: LGPL-2.1-or-later
# Sework Engenharia - faixa de ferramentas (ribbon) em português para o FreeCAD.
"""Substitui as barras de ferramentas clássicas por uma faixa com abas e grupos.

Os botões reaproveitam as ações (QAction) dos próprios comandos do FreeCAD,
então habilitar/desabilitar, marcar e executar continuam sendo do FreeCAD.
Os rótulos são da Sework, em português, independentes da tradução instalada.
"""

import FreeCAD
import FreeCADGui

try:
    from PySide import QtCore, QtGui, QtWidgets
except ImportError:  # pragma: no cover
    from PySide2 import QtCore, QtGui, QtWidgets

PARAM_PATH = "User parameter:BaseApp/Preferences/Mod/Sework"
L, S = "L", "S"  # botão grande / pequeno
ICON_L, ICON_S = 32, 18

# --------------------------------------------------------------------------
# Nomes das bancadas em português
WB_NAMES = {
    "PartDesignWorkbench": "Peça",
    "SketcherWorkbench": "Esboço",
    "PartWorkbench": "Sólidos",
    "AssemblyWorkbench": "Montagem",
    "TechDrawWorkbench": "Desenho técnico",
    "SpreadsheetWorkbench": "Planilha",
    "SeworkWorkbench": "Sework",
    "DraftWorkbench": "Desenho 2D",
    "BIMWorkbench": "BIM / Arquitetura",
    "CAMWorkbench": "Usinagem (CAM)",
    "FemWorkbench": "Simulação (FEM)",
    "InspectionWorkbench": "Inspeção",
    "MaterialWorkbench": "Materiais",
    "MeshWorkbench": "Malhas",
    "OpenSCADWorkbench": "OpenSCAD",
    "PointsWorkbench": "Nuvem de pontos",
    "ReverseEngineeringWorkbench": "Engenharia reversa",
    "RobotWorkbench": "Robótica",
    "SurfaceWorkbench": "Superfícies",
    "TestWorkbench": "Testes",
    "NoneWorkbench": "Nenhuma",
}

# --------------------------------------------------------------------------
# Abas. Cada item: (comando, tamanho, rótulo). "\n" quebra o rótulo grande.
TABS = [
    {"id": "inicio", "label": "Início", "wb": None, "panels": [
        ("Arquivo", [
            ("Std_New", L, "Novo"), ("Std_Open", L, "Abrir"), ("Std_Save", L, "Salvar"),
            ("Std_SaveAs", S, "Salvar como"), ("Std_Import", S, "Importar"), ("Std_Export", S, "Exportar"),
            ("Std_PrintPdf", S, "Exportar PDF"), ("Std_Print", S, "Imprimir"), ("Std_ProjectInfo", S, "Info. do projeto"),
        ]),
        ("Editar", [
            ("Std_Undo", S, "Desfazer"), ("Std_Redo", S, "Refazer"), ("Std_Refresh", S, "Recalcular"),
            ("Std_Cut", S, "Recortar"), ("Std_Copy", S, "Copiar"), ("Std_Paste", S, "Colar"),
            ("Std_Delete", S, "Excluir"), ("Std_Placement", S, "Posicionamento"), ("Std_Edit", S, "Editar objeto"),
        ]),
        ("Vista", [
            ("Std_ViewFitAll", L, "Enquadrar\ntudo"), ("Std_ViewIsometric", L, "Isométrica"),
            ("Std_ViewFront", S, "Frontal"), ("Std_ViewTop", S, "Superior"), ("Std_ViewRight", S, "Direita"),
            ("Std_ViewRear", S, "Traseira"), ("Std_ViewBottom", S, "Inferior"), ("Std_ViewLeft", S, "Esquerda"),
            ("Std_ViewFitSelection", S, "Enquadrar seleção"), ("Std_DrawStyle", S, "Estilo de exibição"),
            ("Std_AlignToSelection", S, "Alinhar à seleção"),
        ]),
        ("Ferramentas", [
            ("Std_Measure", L, "Medir"), ("Part_SelectFilter", S, "Filtro de seleção"),
            ("Std_ToggleClipPlane", S, "Plano de corte"), ("Std_TreeViewActions", S, "Árvore"),
            ("Std_DlgMacroExecute", S, "Macros"), ("Std_DlgPreferences", L, "Preferências"),
        ]),
        ("Estrutura", [
            ("Std_Part", S, "Nova parte"), ("Std_Group", S, "Novo grupo"), ("Part_Datums", S, "Referências"),
            ("Std_LinkActions", S, "Vínculos"), ("Std_VarSet", S, "Variáveis"),
        ]),
    ]},
    {"id": "esboco", "label": "Esboço", "wb": "SketcherWorkbench", "panels": [
        ("Esboço", [
            ("Sketcher_NewSketch", L, "Novo\nesboço"), ("Sketcher_EditSketch", L, "Editar\nesboço"),
            ("Sketcher_LeaveSketch", L, "Concluir\nesboço"),
            ("Sketcher_MapSketch", S, "Anexar"), ("Sketcher_ReorientSketch", S, "Reorientar"),
            ("Sketcher_ViewSketch", S, "Vista normal"), ("Sketcher_ViewSection", S, "Vista em corte"),
            ("Sketcher_ValidateSketch", S, "Validar"), ("Sketcher_MergeSketches", S, "Mesclar"),
            ("Sketcher_MirrorSketch", S, "Espelhar esboço"),
        ]),
        ("Desenhar", [
            ("Sketcher_CreateLine", L, "Linha"), ("Sketcher_CompCreateRectangles", L, "Retângulo"),
            ("Sketcher_CompCreateConic", L, "Círculo"), ("Sketcher_CompCreateArc", L, "Arco"),
            ("Sketcher_CreatePolyline", S, "Polilinha"), ("Sketcher_CreatePoint", S, "Ponto"),
            ("Sketcher_CompCreateRegularPolygon", S, "Polígono"), ("Sketcher_CompSlot", S, "Rasgo"),
            ("Sketcher_CompCreateBSpline", S, "Curva B-spline"), ("Sketcher_ToggleConstruction", S, "Construção"),
        ]),
        ("Restrições", [
            ("Sketcher_CompDimensionTools", L, "Cota"),
            ("Sketcher_ConstrainCoincidentUnified", S, "Coincidente"), ("Sketcher_CompHorVer", S, "Horiz. / vert."),
            ("Sketcher_ConstrainParallel", S, "Paralela"), ("Sketcher_ConstrainPerpendicular", S, "Perpendicular"),
            ("Sketcher_ConstrainTangent", S, "Tangente"), ("Sketcher_ConstrainEqual", S, "Igual"),
            ("Sketcher_ConstrainSymmetric", S, "Simétrica"), ("Sketcher_ConstrainBlock", S, "Bloquear"),
            ("Sketcher_CompToggleConstraints", S, "Ativar restrição"),
        ]),
        ("Modificar", [
            ("Sketcher_CompCurveEdition", L, "Aparar"), ("Sketcher_CompCreateFillets", L, "Arredondar"),
            ("Sketcher_CompExternal", L, "Externa"),
            ("Sketcher_Offset", S, "Offset"), ("Sketcher_Symmetry", S, "Espelhar"), ("Sketcher_Translate", S, "Mover / matriz"),
            ("Sketcher_Rotate", S, "Rotacionar"), ("Sketcher_Scale", S, "Escala"), ("Sketcher_CarbonCopy", S, "Cópia carbono"),
            ("Sketcher_RemoveAxesAlignment", S, "Remover alinhamento"),
        ]),
        ("Auxiliares", [
            ("Sketcher_SelectConstraints", S, "Selecionar restrições"),
            ("Sketcher_SelectElementsAssociatedWithConstraints", S, "Selecionar geometria"),
            ("Sketcher_RestoreInternalAlignmentGeometry", S, "Geometria interna"),
            ("Sketcher_SwitchVirtualSpace", S, "Espaço virtual"), ("Sketcher_ArcOverlay", S, "Auxiliar de arcos"),
            ("Sketcher_JoinCurves", S, "Unir curvas"),
        ]),
    ]},
    {"id": "peca", "label": "Peça", "wb": "PartDesignWorkbench", "panels": [
        ("Corpo", [
            ("PartDesign_Body", L, "Novo\ncorpo"), ("PartDesign_CompSketches", L, "Esboço"),
            ("PartDesign_CompDatums", S, "Referências"), ("PartDesign_ShapeBinder", S, "Vínculo de forma"),
            ("PartDesign_SubShapeBinder", S, "Vínculo de subforma"), ("PartDesign_Clone", S, "Clonar"),
        ]),
        ("Adicionar material", [
            ("PartDesign_Pad", L, "Extrusão"), ("PartDesign_Revolution", L, "Revolução"),
            ("PartDesign_AdditiveLoft", S, "Loft"), ("PartDesign_AdditivePipe", S, "Varredura"),
            ("PartDesign_AdditiveHelix", S, "Hélice"), ("PartDesign_CompPrimitiveAdditive", S, "Primitivas"),
        ]),
        ("Remover material", [
            ("PartDesign_Pocket", L, "Cavidade"), ("PartDesign_Hole", L, "Furo"),
            ("PartDesign_Groove", S, "Ranhura"), ("PartDesign_SubtractiveLoft", S, "Loft"),
            ("PartDesign_SubtractivePipe", S, "Varredura"), ("PartDesign_SubtractiveHelix", S, "Hélice"),
            ("PartDesign_CompPrimitiveSubtractive", S, "Primitivas"),
        ]),
        ("Acabamento", [
            ("PartDesign_Fillet", L, "Arredondar"), ("PartDesign_Chamfer", L, "Chanfro"),
            ("PartDesign_Thickness", S, "Casca"), ("PartDesign_Draft", S, "Ângulo de saída"),
        ]),
        ("Padrões", [
            ("PartDesign_Mirrored", L, "Espelhar"), ("PartDesign_LinearPattern", S, "Padrão linear"),
            ("PartDesign_PolarPattern", S, "Padrão circular"), ("PartDesign_MultiTransform", S, "Multitransformação"),
        ]),
        ("Ferramentas", [
            ("PartDesign_Boolean", S, "Booleana"), ("Part_CheckGeometry", S, "Verificar geometria"),
            ("Sketcher_ValidateSketch", S, "Validar esboço"), ("Std_Measure", L, "Medir"),
        ]),
    ]},
    {"id": "solidos", "label": "Sólidos", "wb": "PartWorkbench", "panels": [
        ("Primitivas", [
            ("Part_Box", L, "Cubo"), ("Part_Cylinder", L, "Cilindro"),
            ("Part_Sphere", S, "Esfera"), ("Part_Cone", S, "Cone"), ("Part_Torus", S, "Toro"),
            ("Part_Tube", S, "Tubo"), ("Part_Primitives", S, "Mais primitivas"), ("Part_Builder", S, "Construtor"),
        ]),
        ("Criar", [
            ("Part_Extrude", L, "Extrusão"), ("Part_Revolve", L, "Revolução"),
            ("Part_Loft", S, "Loft"), ("Part_Sweep", S, "Varredura"), ("Part_MakeFace", S, "Face de arames"),
            ("Part_RuledSurface", S, "Superfície regrada"),
        ]),
        ("Modificar", [
            ("Part_Fillet", L, "Arredondar"), ("Part_Chamfer", L, "Chanfro"),
            ("Part_Mirror", S, "Espelhar"), ("Part_Scale", S, "Escala"), ("Part_CompOffset", S, "Offset"),
            ("Part_Thickness", S, "Casca"), ("Part_Section", S, "Seção"), ("Part_CrossSections", S, "Seções transversais"),
            ("Part_ProjectionOnSurface", S, "Projetar na superfície"),
        ]),
        ("Booleanas", [
            ("Part_Cut", L, "Subtrair"), ("Part_Fuse", L, "Unir"), ("Part_Common", L, "Interseção"),
            ("Part_Boolean", S, "Booleana"), ("Part_CompJoinFeatures", S, "Juntar formas"),
            ("Part_CompSplitFeatures", S, "Dividir formas"), ("Part_CompCompoundTools", S, "Compostos"),
            ("Part_CheckGeometry", S, "Verificar geometria"), ("Part_Defeaturing", S, "Remover detalhes"),
        ]),
    ]},
    {"id": "montagem", "label": "Montagem", "wb": "AssemblyWorkbench", "panels": [
        ("Montagem", [
            ("Assembly_CreateAssembly", L, "Nova\nmontagem"), ("Assembly_Insert", L, "Inserir\ncomponente"),
            ("Assembly_SolveAssembly", S, "Resolver"), ("Assembly_CreateView", S, "Vista explodida"),
            ("Assembly_CreateSimulation", S, "Simulação"), ("Assembly_CreateBom", S, "Lista de materiais"),
        ]),
        ("Juntas", [
            ("Assembly_ToggleGrounded", L, "Fixar"), ("Assembly_CreateJointFixed", L, "Junta\nfixa"),
            ("Assembly_CreateJointRevolute", S, "Revoluta"), ("Assembly_CreateJointCylindrical", S, "Cilíndrica"),
            ("Assembly_CreateJointSlider", S, "Deslizante"), ("Assembly_CreateJointBall", S, "Esférica"),
            ("Assembly_CreateJointDistance", S, "Distância"), ("Assembly_CreateJointParallel", S, "Paralela"),
            ("Assembly_CreateJointPerpendicular", S, "Perpendicular"), ("Assembly_CreateJointAngle", S, "Ângulo"),
            ("Assembly_CreateJointRackPinion", S, "Pinhão e cremalheira"), ("Assembly_CreateJointScrew", S, "Parafuso"),
            ("Assembly_CreateJointGearBelt", S, "Engrenagem / correia"),
        ]),
    ]},
    {"id": "desenho", "label": "Desenho", "wb": "TechDrawWorkbench", "panels": [
        ("Folha", [
            ("TechDraw_PageDefault", L, "Nova\nfolha"), ("TechDraw_PageTemplate", S, "Folha de modelo"),
            ("TechDraw_FillTemplateFields", S, "Preencher carimbo"), ("TechDraw_RedrawPage", S, "Redesenhar"),
            ("TechDraw_PrintAll", S, "Imprimir tudo"),
        ]),
        ("Vistas", [
            ("TechDraw_View", L, "Vista"), ("TechDraw_SectionGroup", L, "Corte"), ("TechDraw_DetailView", L, "Detalhe"),
            ("TechDraw_BrokenView", S, "Vista interrompida"), ("TechDraw_ActiveView", S, "Vista da tela"),
            ("TechDraw_DraftView", S, "Vista 2D"), ("TechDraw_ClipGroup", S, "Recorte"), ("TechDraw_StackGroup", S, "Ordem das vistas"),
        ]),
        ("Cotas", [
            ("TechDraw_CompDimensionTools", L, "Cota"), ("TechDraw_Balloon", L, "Balão"),
            ("TechDraw_AxoLengthDimension", S, "Cota axonométrica"), ("TechDraw_DimensionRepair", S, "Reparar cotas"),
            ("TechDraw_ExtensionInsertPrefixGroup", S, "Prefixo ⌀"), ("TechDraw_ExtensionIncreaseDecreaseGroup", S, "Casas decimais"),
        ]),
        ("Anotação", [
            ("TechDraw_Annotation", L, "Texto"), ("TechDraw_WeldSymbol", L, "Solda"),
            ("TechDraw_LeaderLine", S, "Linha de chamada"), ("TechDraw_RichTextAnnotation", S, "Texto formatado"),
            ("TechDraw_SurfaceFinishSymbols", S, "Rugosidade"), ("TechDraw_HoleShaftFit", S, "Ajuste furo / eixo"),
            ("TechDraw_CenterLineGroup", S, "Linha de centro"), ("TechDraw_ExtensionCircleCenterLinesGroup", S, "Centro de círculo"),
            ("TechDraw_ExtensionThreadsGroup", S, "Rosca cosmética"), ("TechDraw_CosmeticVertexGroup", S, "Vértice cosmético"),
            ("TechDraw_2PointCosmeticLine", S, "Linha cosmética"),
        ]),
        ("Aparência", [
            ("TechDraw_Hatch", S, "Hachura (imagem)"), ("TechDraw_GeometricHatch", S, "Hachura geométrica"),
            ("TechDraw_DecorateLine", S, "Estilo de linha"), ("TechDraw_ShowAll", S, "Arestas ocultas"),
            ("TechDraw_ToggleFrame", S, "Molduras"), ("TechDraw_ExtensionLockUnlockView", S, "Travar vista"),
        ]),
        ("Exportar", [
            ("Std_PrintPdf", L, "PDF"), ("TechDraw_ExportPageDXF", S, "DXF"), ("TechDraw_ExportPageSVG", S, "SVG"),
        ]),
    ]},
    {"id": "planilha", "label": "Planilha", "wb": "SpreadsheetWorkbench", "panels": [
        ("Planilha", [
            ("Spreadsheet_CreateSheet", L, "Nova\nplanilha"), ("Spreadsheet_Import", S, "Importar"),
            ("Spreadsheet_Export", S, "Exportar"), ("Spreadsheet_SetAlias", S, "Apelido"),
        ]),
        ("Células", [
            ("Spreadsheet_MergeCells", S, "Mesclar"), ("Spreadsheet_SplitCell", S, "Dividir"),
            ("Spreadsheet_StyleBold", S, "Negrito"), ("Spreadsheet_StyleItalic", S, "Itálico"),
            ("Spreadsheet_StyleUnderline", S, "Sublinhado"),
        ]),
        ("Alinhamento", [
            ("Spreadsheet_AlignLeft", S, "Esquerda"), ("Spreadsheet_AlignCenter", S, "Centro"),
            ("Spreadsheet_AlignRight", S, "Direita"), ("Spreadsheet_AlignTop", S, "Topo"),
            ("Spreadsheet_AlignVCenter", S, "Meio"), ("Spreadsheet_AlignBottom", S, "Base"),
        ]),
    ]},
    {"id": "sework", "label": "Sework", "wb": None, "panels": [
        ("Sework", [
            ("Sework_AplicarTema", L, "Aplicar\ntema"), ("Sework_Sobre", L, "Sobre"),
        ]),
        ("Interface", [
            ("Sework_TemaIcones", S, "Tema de ícones"), ("Sework_BarrasClassicas", S, "Barras clássicas"),
            ("Sework_RemoverSplash", S, "Splash original"),
        ]),
    ]},
]

STD_TOOLBARS = {"File", "Edit", "Clipboard", "Workbench", "Macro", "View", "Individual Views", "Structure", "Help"}

RIBBON_QSS = """
#SeworkRibbon { background: #FFFFFF; }
#SeworkTabRow { background: #F5F8FC; border-bottom: 1px solid #DCE4EC; }
#SeworkTabRow QTabBar { background: transparent; }
#SeworkTabRow QTabBar::tab {
    background: transparent; color: #2E3B4A; border: none;
    padding: 7px 16px 6px 16px; margin: 0 1px; font-size: 10pt;
    border-top: 3px solid transparent;
}
#SeworkTabRow QTabBar::tab:hover { color: #003A70; background: #EAF1F8; }
#SeworkTabRow QTabBar::tab:selected {
    color: #003A70; background: #FFFFFF; font-weight: 600;
    border-top: 3px solid #F7BE00; border-left: 1px solid #DCE4EC; border-right: 1px solid #DCE4EC;
}
#SeworkBrand { color: #003A70; font-weight: 700; font-size: 10pt; padding: 0 12px 0 10px; }
#SeworkContent { background: #FFFFFF; border: none; border-bottom: 1px solid #DCE4EC; }
#SeworkContent > QWidget, #SeworkPage, #SeworkCol { background: #FFFFFF; }
#SeworkPage QWidget, #SeworkPage QLabel { background: #FFFFFF; color: #1F2A36; }
#SeworkTabRow QLabel { background: transparent; }
#SeworkPage #SeworkPanelTitle { color: #66727F; font-size: 8pt; padding: 1px 0 3px 0; background: transparent; }
#SeworkPage #SeworkSep { background: #DCE4EC; margin: 6px 0 6px 0; }
#SeworkPanel { background: transparent; border: none; }


#SeworkRibbon QToolButton {
    background: transparent; border: 1px solid transparent; border-radius: 5px;
    color: #1F2A36; padding: 2px 4px; font-size: 9pt;
}
#SeworkRibbon QToolButton:hover { background: #EAF1F8; border-color: #C9DAEA; }
#SeworkRibbon QToolButton:pressed, #SeworkRibbon QToolButton:checked { background: #D8E6F3; border-color: #9FBEDD; }
#SeworkRibbon QToolButton:disabled { color: #A3ADB8; }
#SeworkRibbon QToolButton[sw_size="L"] { padding: 3px 4px 2px 4px; }
#SeworkRibbon QToolButton[sw_size="L"][popupMode="1"] { padding-bottom: 12px; }
#SeworkRibbon QToolButton[sw_size="L"]::menu-button {
    subcontrol-origin: border; subcontrol-position: bottom center;
    width: 100%; height: 11px; border: none; border-top: 1px solid transparent;
    border-bottom-left-radius: 5px; border-bottom-right-radius: 5px;
}
#SeworkRibbon QToolButton[sw_size="L"]::menu-button:hover { background: #D8E6F3; }
#SeworkRibbon QToolButton[sw_size="S"][popupMode="1"] { padding-right: 14px; }
#SeworkRibbon QToolButton::menu-arrow { width: 7px; height: 7px; }
#SeworkQuick QToolButton { padding: 3px; border-radius: 4px; }
"""

# Estilo global complementar (aplicado sobre o tema do FreeCAD)
GLOBAL_QSS = """
/*SEWORK-BEGIN*/
QMenuBar { background: #FFFFFF; border: none; border-bottom: 1px solid #E3EAF2; padding: 2px 6px; }
QMenuBar::item { padding: 4px 10px; border-radius: 4px; color: #1F2A36; background: transparent; }
QMenuBar::item:selected { background: #EAF1F8; color: #003A70; }
QMenu { background: #FFFFFF; border: 1px solid #D5DEE8; padding: 4px; }
QMenu::item { padding: 5px 26px 5px 26px; border-radius: 4px; }
QMenu::item:selected { background: #EAF1F8; color: #003A70; }
QMenu::separator { height: 1px; background: #E3EAF2; margin: 4px 8px; }
QStatusBar { background: #F5F8FC; border-top: 1px solid #E3EAF2; color: #4A5866; }
QDockWidget { font-weight: 600; color: #003A70; }
QDockWidget::title { background: #F5F8FC; padding: 6px 8px; border-bottom: 1px solid #E3EAF2; }
QTabBar::tab:selected { color: #003A70; }
QToolTip { background: #003A70; color: #FFFFFF; border: none; padding: 6px 8px; border-radius: 4px; }
/*SEWORK-END*/
"""


def _params():
    return FreeCAD.ParamGet(PARAM_PATH)


def _clean(text):
    return text.replace("&&", "\x00").replace("&", "").replace("\x00", "&").strip()


def _cmd_actions(name):
    try:
        cmd = FreeCADGui.Command.get(name)
    except Exception:
        return []
    if cmd is None:
        return []
    try:
        acts = cmd.getAction() or []
    except Exception:
        return []
    return [a for a in acts if a is not None]


# --------------------------------------------------------------------------
class RibbonButton(QtWidgets.QToolButton):
    """Botão ligado a uma ou várias QActions de um comando do FreeCAD."""

    def __init__(self, actions, size, label, parent=None):
        super().__init__(parent)
        self._actions = actions
        self._size = size
        self._label = label
        self._current = actions[0]
        self.setProperty("sw_size", size)
        self.setAutoRaise(True)
        self.setFocusPolicy(QtCore.Qt.NoFocus)
        if size == L:
            self.setToolButtonStyle(QtCore.Qt.ToolButtonTextUnderIcon)
            self.setIconSize(QtCore.QSize(ICON_L, ICON_L))
            self.setMinimumWidth(54)
            self.setFixedHeight(76)
            self.setSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.Fixed)
        else:
            self.setToolButtonStyle(QtCore.Qt.ToolButtonTextBesideIcon)
            self.setIconSize(QtCore.QSize(ICON_S, ICON_S))
            self.setSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.Fixed)
            self.setFixedHeight(25)
        if len(actions) > 1:
            menu = QtWidgets.QMenu(self)
            for a in actions:
                menu.addAction(a)
            menu.triggered.connect(self._on_menu)
            self.setMenu(menu)
            self.setPopupMode(QtWidgets.QToolButton.MenuButtonPopup)
        self.clicked.connect(self._on_click)
        for a in actions:
            a.changed.connect(self._sync)
        self._sync()

    def _on_menu(self, action):
        self._current = action
        self._sync()

    def _on_click(self):
        a = self._current
        if a is not None and a.isEnabled():
            a.trigger()
        self._sync()

    def _sync(self):
        try:
            self._do_sync()
        except RuntimeError:  # botão ou ação já destruídos
            pass

    def _do_sync(self):
        a = self._current
        icon = a.icon()
        if icon.isNull():
            for x in self._actions:
                if not x.icon().isNull():
                    icon = x.icon()
                    break
        self.setIcon(icon)
        self.setText(self._label if self._label else _clean(a.text()))
        tip = _clean(a.text())
        extra = a.toolTip() or a.statusTip()
        if extra and _clean(extra) != tip:
            tip = "<b>{}</b><br>{}".format(tip, extra)
        sc = a.shortcut().toString(QtGui.QKeySequence.NativeText)
        if sc:
            tip += "<br><span style='color:#F7BE00'>{}</span>".format(sc)
        self.setToolTip(tip)
        enabled = any(x.isEnabled() for x in self._actions) if len(self._actions) > 1 else a.isEnabled()
        self.setEnabled(enabled)
        # Em grupos (menu), o botão não espelha o estado "marcado" da subopção
        checkable = a.isCheckable() and len(self._actions) == 1
        self.setCheckable(checkable)
        if checkable:
            self.setChecked(a.isChecked())


class RibbonPanel(QtWidgets.QFrame):
    def __init__(self, title, parent=None):
        super().__init__(parent)
        self.setObjectName("SeworkPanel")
        outer = QtWidgets.QVBoxLayout(self)
        outer.setContentsMargins(4, 3, 4, 0)
        outer.setSpacing(0)
        self.row = QtWidgets.QHBoxLayout()
        self.row.setSpacing(2)
        self.row.setContentsMargins(0, 0, 0, 0)
        self.row.setAlignment(QtCore.Qt.AlignLeft | QtCore.Qt.AlignTop)
        outer.addLayout(self.row, 1)
        cap = QtWidgets.QLabel(title)
        cap.setObjectName("SeworkPanelTitle")
        cap.setAlignment(QtCore.Qt.AlignCenter)
        cap.setFixedHeight(18)
        outer.addWidget(cap)
        self._col = None
        self.count = 0

    def add(self, btn, size):
        self.count += 1
        if size == L:
            self._col = None
            self.row.addWidget(btn)
            return
        if self._col is None or self._col.count() >= 3:
            w = QtWidgets.QWidget()
            w.setObjectName("SeworkCol")
            w.setAttribute(QtCore.Qt.WA_StyledBackground, True)
            self._col = QtWidgets.QVBoxLayout(w)
            self._col.setContentsMargins(0, 0, 0, 0)
            self._col.setSpacing(1)
            self._col.setAlignment(QtCore.Qt.AlignTop)
            self.row.addWidget(w)
        self._col.addWidget(btn)


class SeworkRibbon(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("SeworkRibbon")
        self.setAttribute(QtCore.Qt.WA_StyledBackground, True)
        self.setStyleSheet(RIBBON_QSS)
        self._tabs = list(TABS)
        self._dynamic = None  # aba gerada para bancadas sem aba própria
        self._edit_prev = None
        self._building = False

        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 0)
        lay.setSpacing(0)

        # Linha das abas
        row = QtWidgets.QWidget()
        row.setObjectName("SeworkTabRow")
        row.setAttribute(QtCore.Qt.WA_StyledBackground, True)
        rl = QtWidgets.QHBoxLayout(row)
        rl.setContentsMargins(6, 0, 6, 0)
        rl.setSpacing(4)
        brand = QtWidgets.QLabel("SEWORK")
        brand.setObjectName("SeworkBrand")
        brand.setSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Preferred)
        rl.addWidget(brand)
        self.tabbar = QtWidgets.QTabBar()
        self.tabbar.setDrawBase(False)
        self.tabbar.setExpanding(False)
        self.tabbar.setUsesScrollButtons(False)
        self.tabbar.setElideMode(QtCore.Qt.ElideNone)
        self.tabbar.setSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Fixed)
        self.tabbar.setFocusPolicy(QtCore.Qt.NoFocus)
        for t in self._tabs:
            self.tabbar.addTab(t["label"])
        self.tabbar.currentChanged.connect(self._on_tab_changed)
        self.tabbar.tabBarDoubleClicked.connect(self._toggle_collapse)
        rl.addWidget(self.tabbar)
        rl.addStretch(1)
        rl.addWidget(self._build_quick())
        lay.addWidget(row)

        # Área de conteúdo
        self.content = QtWidgets.QScrollArea()
        self.content.setObjectName("SeworkContent")
        self.content.setFrameShape(QtWidgets.QFrame.NoFrame)
        self.content.setWidgetResizable(True)
        self.content.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.content.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAsNeeded)
        self.content.viewport().setAutoFillBackground(True)
        pal = self.content.viewport().palette()
        pal.setColor(QtGui.QPalette.Window, QtGui.QColor("#FFFFFF"))
        self.content.viewport().setPalette(pal)
        self.content.setFixedHeight(_params().GetInt("RibbonHeight", 104))
        lay.addWidget(self.content)

    # ---- barra de acesso rápido (direita)
    def _build_quick(self):
        box = QtWidgets.QWidget()
        box.setObjectName("SeworkQuick")
        hl = QtWidgets.QHBoxLayout(box)
        hl.setContentsMargins(0, 0, 0, 0)
        hl.setSpacing(1)
        for name, tip in (("Std_New", "Novo"), ("Std_Open", "Abrir"), ("Std_Save", "Salvar"),
                          ("Std_Undo", "Desfazer"), ("Std_Redo", "Refazer")):
            acts = _cmd_actions(name)
            if acts:
                b = RibbonButton(acts[:1], S, "", box)
                b.setToolButtonStyle(QtCore.Qt.ToolButtonIconOnly)
                b.setIconSize(QtCore.QSize(18, 18))
                b.setFixedSize(28, 28)
                hl.addWidget(b)
        wbbtn = QtWidgets.QToolButton(box)
        wbbtn.setText("Bancadas")
        wbbtn.setToolTip("Trocar de bancada de trabalho")
        wbbtn.setPopupMode(QtWidgets.QToolButton.InstantPopup)
        wbbtn.setToolButtonStyle(QtCore.Qt.ToolButtonTextOnly)
        menu = QtWidgets.QMenu(wbbtn)
        menu.aboutToShow.connect(lambda m=menu: self._fill_wb_menu(m))
        wbbtn.setMenu(menu)
        hl.addSpacing(8)
        hl.addWidget(wbbtn)
        col = QtWidgets.QToolButton(box)
        col.setArrowType(QtCore.Qt.UpArrow)
        col.setToolTip("Recolher / expandir a faixa (ou duplo clique na aba)")
        col.clicked.connect(self._toggle_collapse)
        self._collapse_btn = col
        hl.addWidget(col)
        return box

    def _fill_wb_menu(self, menu):
        menu.clear()
        wbs = FreeCADGui.listWorkbenches()
        items = []
        for key, wb in wbs.items():
            if key in ("NoneWorkbench",):
                continue
            label = WB_NAMES.get(key) or getattr(wb, "MenuText", key)
            items.append((label, key, wb))
        for label, key, wb in sorted(items, key=lambda x: x[0].lower()):
            icon = QtGui.QIcon()
            try:
                ic = wb.Icon if hasattr(wb, "Icon") else ""
                if isinstance(ic, str) and ic and not ic.lstrip().startswith("/*"):
                    icon = QtGui.QIcon(ic)
            except Exception:
                pass
            act = menu.addAction(icon, label)
            act.triggered.connect(lambda checked=False, k=key: FreeCADGui.activateWorkbench(k))

    def _toggle_collapse(self, *args):
        vis = not self.content.isVisible()
        self.content.setVisible(vis)
        self._collapse_btn.setArrowType(QtCore.Qt.UpArrow if vis else QtCore.Qt.DownArrow)

    # ---- abas
    def tab_index(self, tab_id):
        for i, t in enumerate(self._tabs):
            if t["id"] == tab_id:
                return i
        return -1

    def _on_tab_changed(self, idx):
        if self._building or idx < 0:
            return
        tab = self._tabs[idx]
        wb = tab.get("wb")
        if wb and self._user_click:
            try:
                if FreeCADGui.activeWorkbench().name() != wb:
                    FreeCADGui.activateWorkbench(wb)  # dispara sync_workbench
                    return
            except Exception as exc:
                FreeCAD.Console.PrintWarning("Sework: {}\n".format(exc))
        self.build(tab)

    _user_click = True

    def select_tab(self, tab_id, user=False):
        idx = self.tab_index(tab_id)
        if idx < 0:
            return
        self._user_click = user
        try:
            if self.tabbar.currentIndex() == idx:
                self.build(self._tabs[idx])
            else:
                self.tabbar.setCurrentIndex(idx)
        finally:
            self._user_click = True

    def sync_workbench(self, name=None):
        """Chamado quando a bancada ativa muda."""
        try:
            name = name or FreeCADGui.activeWorkbench().name()
        except Exception:
            return
        for t in self._tabs:
            if t.get("wb") == name:
                self.select_tab(t["id"])
                return
        if name == "SeworkWorkbench":
            self.select_tab("sework")
            return
        if name == "StartWorkbench" or name == "NoneWorkbench":
            self.select_tab("inicio")
            return
        self._make_dynamic(name)

    def _make_dynamic(self, name):
        try:
            wb = FreeCADGui.activeWorkbench()
            items = wb.getToolbarItems()
        except Exception:
            items = {}
        mw = FreeCADGui.getMainWindow()
        panels = []
        for tbname, cmds in items.items():
            if tbname in STD_TOOLBARS:
                continue
            tb = mw.findChild(QtWidgets.QToolBar, tbname)
            title = tb.windowTitle() if tb is not None else tbname
            entries = []
            for c in cmds:
                if c == "Separator":
                    continue
                entries.append((c, L if not entries else S, None))
            if entries:
                panels.append((title, entries))
        label = WB_NAMES.get(name) or getattr(wb, "MenuText", name)
        tab = {"id": "dyn", "label": label, "wb": name, "panels": panels}
        self._building = True
        if self._dynamic is None:
            self._tabs.append(tab)
            self.tabbar.addTab(label)
        else:
            self._tabs[-1] = tab
            self.tabbar.setTabText(len(self._tabs) - 1, label)
        self._dynamic = tab
        self._building = False
        self.select_tab("dyn")

    # ---- construção do conteúdo
    def build(self, tab):
        page = QtWidgets.QWidget()
        page.setObjectName("SeworkPage")
        page.setAttribute(QtCore.Qt.WA_StyledBackground, True)
        hl = QtWidgets.QHBoxLayout(page)
        hl.setContentsMargins(6, 2, 6, 0)
        hl.setSpacing(0)
        first = True
        for title, entries in tab["panels"]:
            panel = RibbonPanel(title)
            for cmd, size, label in entries:
                acts = _cmd_actions(cmd)
                if not acts:
                    continue
                panel.add(RibbonButton(acts, size, label, panel), size)
            if panel.count == 0:
                panel.deleteLater()
                continue
            if not first:
                sep = QtWidgets.QFrame()
                sep.setObjectName("SeworkSep")
                sep.setFixedWidth(1)
                sep.setAttribute(QtCore.Qt.WA_StyledBackground, True)
                hl.addWidget(sep)
            hl.addWidget(panel)
            first = False
        hl.addStretch(1)
        old = self.content.takeWidget()
        self.content.setWidget(page)
        if old is not None:
            old.deleteLater()

    # ---- edição de esboço: mostra a aba Esboço sem trocar de bancada
    def check_edit(self):
        try:
            gdoc = FreeCADGui.ActiveDocument
            vp = gdoc.getInEdit() if gdoc else None
            editing = bool(vp) and vp.Object.TypeId.startswith("Sketcher::")
        except Exception:
            editing = False
        cur = self._tabs[self.tabbar.currentIndex()]["id"] if self.tabbar.currentIndex() >= 0 else None
        if editing and cur != "esboco" and self._edit_prev is None:
            self._edit_prev = cur
            self.select_tab("esboco")
        elif not editing and self._edit_prev is not None:
            prev, self._edit_prev = self._edit_prev, None
            if cur == "esboco":
                self.select_tab(prev)


# --------------------------------------------------------------------------
_state = {"bar": None, "ribbon": None, "timer": None}


def _hide_native_toolbars():
    mw = FreeCADGui.getMainWindow()
    bar = _state["bar"]
    for tb in mw.findChildren(QtWidgets.QToolBar):
        if tb is bar or tb.parent() is not mw:
            continue
        if tb.isVisible():
            tb.hide()


def _force_light_scheme():
    """Windows em modo escuro faz o Qt 6 pintar de escuro o que o tema não cobre."""
    try:
        hints = QtGui.QGuiApplication.styleHints()
        if hasattr(hints, "setColorScheme") and hints.colorScheme() != QtCore.Qt.ColorScheme.Light:
            hints.setColorScheme(QtCore.Qt.ColorScheme.Light)
    except Exception:
        pass


def _apply_global_qss():
    _force_light_scheme()
    app = QtWidgets.QApplication.instance()
    css = app.styleSheet() or ""
    if "/*SEWORK-BEGIN*/" in css:
        return
    app.setStyleSheet(css + "\n" + GLOBAL_QSS)


def _tick():
    if _state["ribbon"] is None:
        return
    _hide_native_toolbars()
    _apply_global_qss()
    _state["ribbon"].check_edit()


def _on_workbench(name):
    rb = _state["ribbon"]
    if rb is None:
        return
    QtCore.QTimer.singleShot(0, _hide_native_toolbars)
    QtCore.QTimer.singleShot(50, lambda: rb.sync_workbench(name))


def install():
    if _state["ribbon"] is not None:
        return _state["ribbon"]
    mw = FreeCADGui.getMainWindow()
    bar = QtWidgets.QToolBar("Faixa Sework", mw)
    bar.setObjectName("SeworkRibbonBar")
    bar.setMovable(False)
    bar.setFloatable(False)
    bar.setContextMenuPolicy(QtCore.Qt.PreventContextMenu)
    bar.toggleViewAction().setVisible(False)
    bar.setStyleSheet("QToolBar#SeworkRibbonBar { border: none; padding: 0; margin: 0; spacing: 0; background: #FFFFFF; }")
    ribbon = SeworkRibbon(bar)
    ribbon.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Fixed)
    bar.addWidget(ribbon)
    mw.addToolBar(QtCore.Qt.TopToolBarArea, bar)
    _state.update(bar=bar, ribbon=ribbon)
    try:
        mw.workbenchActivated.connect(_on_workbench)
    except Exception as exc:
        FreeCAD.Console.PrintWarning("Sework: sinal workbenchActivated indisponível: {}\n".format(exc))
    timer = QtCore.QTimer(mw)
    timer.setInterval(400)
    timer.timeout.connect(_tick)
    timer.start()
    _state["timer"] = timer
    _hide_native_toolbars()
    _apply_global_qss()
    ribbon.sync_workbench()
    return ribbon


def uninstall(restore_toolbars=True):
    mw = FreeCADGui.getMainWindow()
    if _state["timer"] is not None:
        _state["timer"].stop()
    try:
        mw.workbenchActivated.disconnect(_on_workbench)
    except Exception:
        pass
    if _state["bar"] is not None:
        mw.removeToolBar(_state["bar"])
        _state["bar"].deleteLater()
    _state.update(bar=None, ribbon=None, timer=None)
    app = QtWidgets.QApplication.instance()
    css = app.styleSheet() or ""
    if "/*SEWORK-BEGIN*/" in css:
        a = css.index("/*SEWORK-BEGIN*/")
        b = css.index("/*SEWORK-END*/") + len("/*SEWORK-END*/")
        app.setStyleSheet(css[:a] + css[b:])
    if restore_toolbars:
        for tb in mw.findChildren(QtWidgets.QToolBar):
            if tb.parent() is mw and tb.toggleViewAction().isVisible():
                tb.show()


def reload():
    """Para desenvolvimento: recarrega o módulo e reinstala a faixa."""
    import importlib
    import SeworkRibbon as mod
    mod.uninstall(restore_toolbars=False)
    mod = importlib.reload(mod)
    return mod.install()
