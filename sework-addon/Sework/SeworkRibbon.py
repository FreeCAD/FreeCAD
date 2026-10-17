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
            ("Part_Box", L, "Caixa"), ("Part_Cylinder", L, "Cilindro"),
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
    {"id": "simulacao", "label": "Simulação", "wb": "FemWorkbench", "panels": [
        # Na ordem em que a análise precisa ser montada
        ("Assistente", [("Sework_AssistenteFEM", L, "Assistente\nde simulação")]),
        ("1 · Estudo", [("FEM_Analysis", L, "Novo\nestudo"), ("FEM_SolverCalculiXCcxTools", L, "Solver\nCalculiX")]),
        ("2 · Material", [("FEM_MaterialSolid", L, "Material\nsólido"), ("FEM_MaterialEditor", S, "Editor de materiais")]),
        ("3 · Fixações", [("FEM_ConstraintFixed", L, "Fixação"), ("FEM_ConstraintDisplacement", S, "Deslocamento imposto"),
                          ("FEM_ConstraintSpring", S, "Mola (apoio elástico)")]),
        ("4 · Cargas", [("FEM_ConstraintForce", L, "Força"), ("FEM_ConstraintPressure", S, "Pressão"),
                        ("FEM_ConstraintSelfWeight", S, "Peso próprio"), ("FEM_ConstraintCentrif", S, "Centrífuga")]),
        ("5 · Malha", [("FEM_MeshGmshFromShape", L, "Gerar\nmalha"), ("FEM_MeshRegion", S, "Refino local"),
                       ("FEM_MeshDisplayInfo", S, "Dados da malha"), ("FEM_MeshClear", S, "Limpar malha")]),
        ("6 · Calcular", [("FEM_SolverRun", L, "Calcular"), ("FEM_SolverControl", S, "Controle do solver")]),
        ("7 · Resultados", [("FEM_ResultShow", L, "Ver\nresultados"), ("FEM_PostFilterWarp", S, "Deformada"),
                            ("FEM_PostFilterClipScalar", S, "Recorte por valor"), ("FEM_PostFilterDataAlongLine", S, "Gráfico ao longo de linha"),
                            ("FEM_ResultsPurge", S, "Limpar resultados")]),
    ]},
    {"id": "sework", "label": "Sework", "wb": None, "panels": [
        ("Sework", [
            ("Sework_Inicio", L, "Tela\ninicial"), ("Sework_AplicarTema", L, "Aplicar\ntema"),
            ("Sework_TemaEscuro", L, "Tema\nescuro"), ("Sework_Sobre", L, "Sobre"),
        ]),
        ("Interface", [
            ("Sework_TemaIcones", S, "Tema de ícones"), ("Sework_BarrasClassicas", S, "Barras clássicas"),
            ("Sework_RemoverSplash", S, "Splash original"), ("Sework_ModoDistribuicao", S, "Modo distribuição"),
        ]),
        ("Qualidade", [("Sework_Testes", L, "Testes\nautomáticos")]),
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
#SeworkBrand { color: #003A70; font-weight: 700; font-size: 10.5pt; padding: 0 14px 0 8px; }
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
#SeworkTabRow QToolButton#SeworkWbBtn {
    background: #FFFFFF; color: #003A70; font-weight: 600; font-size: 9pt;
    border: 1px solid #C9D5E2; border-radius: 6px; padding: 3px 22px 3px 10px; margin: 0 2px;
}
#SeworkTabRow QToolButton#SeworkWbBtn:hover { background: #EAF1F8; border-color: #7FA3C9; }
#SeworkTabRow QToolButton#SeworkWbBtn::menu-indicator {
    subcontrol-origin: padding; subcontrol-position: center right; right: 7px; width: 9px; height: 9px;
}
#SeworkWbSep { color: #D5DEE8; background: transparent; }
"""

# Estilo global complementar (aplicado sobre o tema do FreeCAD)
GLOBAL_QSS = """
/*SEWORK-BEGIN*/
QMenuBar { background: #FFFFFF; border: none; border-bottom: 1px solid #E3EAF2; padding: 2px 6px; }
QMenuBar::item { padding: 4px 10px; border-radius: 4px; color: #1F2A36; background: transparent; }
QMenuBar::item:disabled { color: #9AA7B4; }
QMenuBar::item:selected { background: #EAF1F8; color: #003A70; }
QMenu { background: #FFFFFF; color: #1F2A36; border: 1px solid #D5DEE8; padding: 4px; }
QMenu::item { padding: 5px 26px 5px 26px; border-radius: 4px; color: #1F2A36; background: transparent; }
QMenu::item:disabled { color: #9AA7B4; }
QMenu::item:selected { background: #EAF1F8; color: #003A70; }
QMenu::separator { height: 1px; background: #E3EAF2; margin: 4px 8px; }
QStatusBar { background: #F5F8FC; border-top: 1px solid #E3EAF2; color: #4A5866; }
QDockWidget { font-weight: 600; color: #003A70; }
QDockWidget::title { background: #F5F8FC; padding: 6px 8px; border-bottom: 1px solid #E3EAF2; }
QTabBar::tab:selected { color: #003A70; }
QToolTip { background: #FFFFFF; color: #1F2A36; border: 1px solid #D5DEE8; padding: 8px 10px; }
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
        acts = [a for a in (cmd.getAction() or []) if a is not None]
    except Exception:
        acts = []
    if not acts and (name.startswith("Sework_") or name.startswith("FEM_")):
        acts = [_own_action(name, cmd)]
    return [a for a in acts if a is not None]


_OWN_ACTIONS = {}


def _own_action(name, cmd):
    """O FreeCAD só cria a QAction de um comando Python quando alguma barra/menu o usa.

    Os comandos Sework só entram em barras da bancada Sework, então antes de ela ser
    ativada eles não têm ação. Criamos uma ação própria que executa o comando.
    """
    if name in _OWN_ACTIONS:
        return _OWN_ACTIONS[name]
    try:
        info = cmd.getInfo()
    except Exception:
        info = {}
    import os as _os
    QAction = getattr(QtGui, "QAction", None) or QtWidgets.QAction
    act = QAction(FreeCADGui.getMainWindow())
    act.setText(info.get("menuText") or name)
    tip = info.get("toolTip") or ""
    act.setToolTip(tip)
    act.setStatusTip(tip)
    pix = info.get("pixmap") or ""
    if pix and _os.path.isfile(pix):
        act.setIcon(QtGui.QIcon(pix))
    elif pix:
        try:
            act.setIcon(FreeCADGui.getIcon(pix))
        except Exception:
            pass
    act.triggered.connect(lambda checked=False, n=name: FreeCADGui.runCommand(n, 0))
    _OWN_ACTIONS[name] = act
    return act


# -------------------------------------------------------------------------- ícones e dicas
import hashlib
import os

_HERE = os.path.dirname(os.path.abspath(__file__))
_DIS_CACHE = {}
_TIP_CACHE = {}


def _t(texto):
    """Cores do tema ativo (claro ou escuro)."""
    try:
        import SeworkEstilo
        return SeworkEstilo.tema(texto)
    except Exception:
        return texto


def _escuro():
    try:
        import SeworkEstilo
        return SeworkEstilo.escuro()
    except Exception:
        return False


def _disabled_pixmap(pm):
    """Versão desabilitada legível: véu cinza-azulado sobre o ícone, 78 % de opacidade."""
    tinted = QtGui.QPixmap(pm.size())
    tinted.fill(QtCore.Qt.transparent)
    p = QtGui.QPainter(tinted)
    p.drawPixmap(0, 0, pm)
    p.setCompositionMode(QtGui.QPainter.CompositionMode_SourceAtop)
    if not _escuro():
        p.fillRect(tinted.rect(), QtGui.QColor(150, 166, 186, 120))
    p.end()
    out = QtGui.QPixmap(pm.size())
    out.fill(QtCore.Qt.transparent)
    p = QtGui.QPainter(out)
    p.setOpacity(0.5 if _escuro() else 0.78)
    p.drawPixmap(0, 0, tinted)
    p.end()
    out.setDevicePixelRatio(pm.devicePixelRatio())
    return out


def _with_disabled(icon, size):
    """Mesmo ícone, com o modo Desabilitado desenhado pela Sework."""
    if icon.isNull():
        return icon
    key = (icon.cacheKey(), size)
    if key in _DIS_CACHE:
        return _DIS_CACHE[key]
    dpr = 2.0
    pm = icon.pixmap(QtCore.QSize(size, size) * dpr)
    pm.setDevicePixelRatio(dpr)
    out = QtGui.QIcon()
    out.addPixmap(pm, QtGui.QIcon.Normal)
    out.addPixmap(pm, QtGui.QIcon.Active)
    out.addPixmap(_disabled_pixmap(pm), QtGui.QIcon.Disabled)
    _DIS_CACHE[key] = out
    return out


def _tip_image(icon):
    """Grava o ícone em 44 px para usar na dica rica (o QToolTip só aceita arquivo)."""
    d = os.path.join(FreeCAD.getUserAppDataDir(), "SeworkUI", "dicas")
    key = str(icon.cacheKey())
    if key in _TIP_CACHE:
        return _TIP_CACHE[key]
    try:
        os.makedirs(d, exist_ok=True)
        path = os.path.join(d, hashlib.md5(key.encode()).hexdigest()[:12] + ".png")
        icon.pixmap(88, 88).save(path)
        _TIP_CACHE[key] = path.replace("\\", "/")
    except Exception:
        _TIP_CACHE[key] = None
    return _TIP_CACHE[key]


def _tip_desc(raw, titles):
    """Texto da dica do FreeCAD sem o título repetido e sem o nome interno do comando."""
    if not raw:
        return ""
    doc = QtGui.QTextDocument()
    doc.setHtml(raw) if "<" in raw else doc.setPlainText(raw)
    out = []
    low = {_clean(t).lower() for t in titles if t}
    for line in doc.toPlainText().splitlines():
        t = line.strip()
        if not t or _clean(t).lower() in low:
            continue
        if " " not in t and "_" in t:      # PartDesign_Pad, Std_Open...
            continue
        out.append(t)
    return " ".join(out)


def _rich_tip(icon, title, desc, shortcut):
    img = _tip_image(icon) if not icon.isNull() else None
    body = "<div style='font-weight:600; color:#003A70; font-size:10pt'>{}</div>".format(title)
    if desc:
        body += "<div style='color:#4A5866; margin-top:3px'>{}</div>".format(desc)
    if shortcut:
        body += ("<div style='margin-top:6px'><span style='background:#EEF2F8; color:#003A70; "
                 "font-weight:600'>&nbsp;{}&nbsp;</span></div>").format(shortcut)
    body = _t(body)
    if not img:
        return "<div style='max-width:300px'>{}</div>".format(body)
    return ("<table cellspacing='0' cellpadding='0'><tr>"
            "<td valign='top' style='padding-right:10px'><img src='{}' width='44' height='44'></td>"
            "<td valign='top' width='240'>{}</td></tr></table>").format(img, body)


# --------------------------------------------------------------------------
L_ALTURA = 90   # botão grande: ícone + rótulo em até duas linhas (+ seta de menu)
S_ALTURA = 28   # botão pequeno (três por coluna)


def _quebra(texto):
    """Rótulos longos dos botões grandes em duas linhas, quebrando no espaço mais central."""
    if "\n" in texto or len(texto) <= 11 or " " not in texto:
        return texto
    meio = len(texto) / 2.0
    pos = min((i for i, c in enumerate(texto) if c == " "), key=lambda i: abs(i - meio))
    return texto[:pos] + "\n" + texto[pos + 1:]


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
            self.setFixedHeight(L_ALTURA)
            self.setSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.Fixed)
        else:
            self.setToolButtonStyle(QtCore.Qt.ToolButtonTextBesideIcon)
            self.setIconSize(QtCore.QSize(ICON_S, ICON_S))
            self.setSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.Fixed)
            self.setFixedHeight(S_ALTURA)
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
        self.setIcon(_with_disabled(icon, ICON_L if self._size == L else ICON_S))
        txt = self._label if self._label else _clean(a.text())
        self.setText(_quebra(txt) if self._size == L else txt)
        action_title = _clean(a.text())
        title = self._label.replace("\n", " ") if (self._label and len(self._actions) == 1) else action_title
        desc = _tip_desc(a.toolTip() or a.statusTip(), (title, action_title))
        sc = a.shortcut().toString(QtGui.QKeySequence.NativeText)
        key = (title, desc, sc, icon.cacheKey())
        if getattr(self, "_tipkey", None) != key:
            self._tipkey = key
            self.setToolTip(_rich_tip(icon, title, desc, sc))
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
        cap.setFixedHeight(20)
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
        self.setStyleSheet(_t(RIBBON_QSS))
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
        brand = QtWidgets.QLabel()
        brand.setObjectName("SeworkBrand")
        _ic = os.path.join(_HERE, "Resources", "icons", "sework.svg")
        if os.path.isfile(_ic):
            _pm = QtGui.QIcon(_ic).pixmap(40, 40)
            _pm.setDevicePixelRatio(2.0)
            brand.setText("<table cellspacing='0' cellpadding='0'><tr><td valign='middle'><img src='{}' width='20' height='20'></td>"
                          "<td valign='middle' style='padding-left:6px'>Sework</td></tr></table>".format(_ic.replace("\\", "/")))
        else:
            brand.setText("Sework")
        brand.setSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Preferred)
        rl.addWidget(brand)
        rl.addWidget(self._build_wb_button())
        sep = QtWidgets.QFrame()
        sep.setObjectName("SeworkWbSep")
        sep.setFrameShape(QtWidgets.QFrame.VLine)
        sep.setFixedHeight(20)
        rl.addSpacing(4)
        rl.addWidget(sep)
        rl.addSpacing(2)
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
        pal.setColor(QtGui.QPalette.Window, QtGui.QColor(_t("background: #FFFFFF").split(": ")[1]))
        self.content.viewport().setPalette(pal)
        self.content.setFixedHeight(max(_params().GetInt("RibbonHeight", 126), 126))
        lay.addWidget(self.content)

    # ---- seletor de bancadas (logo após a marca)
    def _build_wb_button(self):
        b = QtWidgets.QToolButton()
        b.setObjectName("SeworkWbBtn")
        b.setText("Bancadas")
        b.setToolTip("Trocar de bancada de trabalho")
        b.setPopupMode(QtWidgets.QToolButton.InstantPopup)
        b.setToolButtonStyle(QtCore.Qt.ToolButtonTextOnly)
        b.setCursor(QtCore.Qt.PointingHandCursor)
        try:
            import SeworkEstilo
            b.setStyleSheet("QToolButton::menu-indicator { image: url(%s); }" % SeworkEstilo._url("chev_down.svg"))
        except Exception:
            pass
        menu = QtWidgets.QMenu(b)
        menu.aboutToShow.connect(lambda m=menu: self._fill_wb_menu(m))
        b.setMenu(menu)
        self._wb_btn = b
        return b

    def _rotulo_bancada(self, name):
        b = getattr(self, "_wb_btn", None)
        if b is None:
            return
        try:
            wb = FreeCADGui.getWorkbench(name)
            nome = WB_NAMES.get(name) or getattr(wb, "MenuText", name)
        except Exception:
            nome = WB_NAMES.get(name, name)
        b.setText(nome if name not in ("NoneWorkbench", "StartWorkbench") else "Bancadas")
        b.setToolTip("Bancada ativa: {}. Clique para trocar.".format(nome))

    # ---- barra de acesso rápido (direita)
    def _build_quick(self):
        box = QtWidgets.QWidget()
        box.setObjectName("SeworkQuick")
        hl = QtWidgets.QHBoxLayout(box)
        hl.setContentsMargins(0, 0, 0, 0)
        hl.setSpacing(1)
        for name, tip in (("Std_New", "Novo"), ("Std_Open", "Abrir"), ("Std_Save", "Salvar"),
                          ("Std_Undo", "Desfazer"), ("Std_Redo", "Refazer"), ("|", ""),
                          ("Std_Measure", "Medir")):
            if name == "|":
                sep = QtWidgets.QFrame(box)
                sep.setFrameShape(QtWidgets.QFrame.VLine)
                sep.setStyleSheet("color: #3A4450;" if _escuro() else "color: #D5DEE8;")
                sep.setFixedHeight(18)
                hl.addSpacing(4)
                hl.addWidget(sep)
                hl.addSpacing(4)
                continue
            acts = _cmd_actions(name)
            if not acts:
                # Comandos padrão também podem ainda não ter QAction quando a faixa
                # é montada; usamos uma ação própria que executa o comando.
                try:
                    cmd = FreeCADGui.Command.get(name)
                except Exception:
                    cmd = None
                if cmd is not None:
                    a = _own_action(name, cmd)
                    if a is not None:
                        a.setToolTip(tip)
                        acts = [a]
            if acts:
                b = RibbonButton(acts[:1], S, "", box)
                b.setToolButtonStyle(QtCore.Qt.ToolButtonIconOnly)
                b.setIconSize(QtCore.QSize(18, 18))
                b.setFixedSize(28, 28)
                hl.addWidget(b)
        hl.addSpacing(8)
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
        self._rotulo_bancada(name)
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
    """Fixa o esquema de cores do Qt no tema Sework (claro ou escuro), independente do Windows."""
    try:
        hints = QtGui.QGuiApplication.styleHints()
        alvo = QtCore.Qt.ColorScheme.Dark if _escuro() else QtCore.Qt.ColorScheme.Light
        if hasattr(hints, "setColorScheme") and hints.colorScheme() != alvo:
            hints.setColorScheme(alvo)
    except Exception:
        pass
    if _escuro() and not _state.get("paleta"):
        _state["paleta"] = True
        try:
            app = QtWidgets.QApplication.instance()
            pal = QtGui.QPalette(app.palette())
            for role, c in ((QtGui.QPalette.Window, "#161A20"), (QtGui.QPalette.Base, "#1F252D"),
                            (QtGui.QPalette.AlternateBase, "#1B2129"), (QtGui.QPalette.Button, "#1F252D"),
                            (QtGui.QPalette.WindowText, "#E6ECF2"), (QtGui.QPalette.Text, "#E6ECF2"),
                            (QtGui.QPalette.ButtonText, "#E6ECF2"), (QtGui.QPalette.ToolTipBase, "#1F252D"),
                            (QtGui.QPalette.ToolTipText, "#E6ECF2"), (QtGui.QPalette.Highlight, "#2F6FB5"),
                            (QtGui.QPalette.HighlightedText, "#F1F4F8"), (QtGui.QPalette.Link, "#8CBCEB"),
                            (QtGui.QPalette.PlaceholderText, "#6B7785")):
                pal.setColor(role, QtGui.QColor(c))
            pal.setColor(QtGui.QPalette.Disabled, QtGui.QPalette.Text, QtGui.QColor("#7D8996"))
            pal.setColor(QtGui.QPalette.Disabled, QtGui.QPalette.WindowText, QtGui.QColor("#7D8996"))
            pal.setColor(QtGui.QPalette.Disabled, QtGui.QPalette.ButtonText, QtGui.QColor("#7D8996"))
            app.setPalette(pal)
        except Exception:
            pass


def _apply_global_qss():
    _force_light_scheme()
    app = QtWidgets.QApplication.instance()
    css = app.styleSheet() or ""
    if "/*SEWORK-BEGIN*/" in css:
        return
    extra = ""
    try:
        import SeworkEstilo
        if SeworkEstilo.enabled():
            extra = SeworkEstilo.qss()
    except Exception as exc:
        FreeCAD.Console.PrintWarning("Sework: estilo das janelas não aplicado: {}\n".format(exc))
    app.setStyleSheet(css + "\n" + _t(GLOBAL_QSS).replace("/*SEWORK-END*/", extra + "\n/*SEWORK-END*/"))


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
    bar.setStyleSheet(_t("QToolBar#SeworkRibbonBar { border: none; padding: 0; margin: 0; spacing: 0; background: #FFFFFF; }"))
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
