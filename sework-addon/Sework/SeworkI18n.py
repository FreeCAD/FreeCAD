# SPDX-License-Identifier: LGPL-2.1-or-later
# Sework Engenharia - tradução pt-BR com prioridade sobre a tradução oficial.
"""Instala as traduções Sework por cima das oficiais.

1. ``translations/sework_ptbr.qm``: compilada da tradução pt-BR mais recente
   do repositório do FreeCAD (mais completa que a da versão instalada).
2. ``EXTRA``: textos que nem o repositório traduz, por texto de origem.

O Qt consulta primeiro o tradutor instalado por último. Os módulos do FreeCAD
instalam os tradutores deles ao carregar, então reaplicamos a prioridade na
primeira ativação de cada bancada.
"""

import os

import FreeCAD

try:
    from PySide import QtCore
except ImportError:  # pragma: no cover
    from PySide2 import QtCore

ADDON_DIR = os.path.dirname(os.path.abspath(__file__))
PARAM_PATH = "User parameter:BaseApp/Preferences/Mod/Sework"

# Textos de origem (inglês) -> português. Aceleradores (&) são opcionais.
EXTRA = {
    # Textos encontrados na auditoria visual (sem tradução na versão 1.1.4)
    "Search preferences…": "Buscar nas preferências…", "Search preferences...": "Buscar nas preferências...",
    "&Content": "&Conteúdo", "&Alias": "&Apelido", "Pad": "Extrusão",
    # Botões padrão dos diálogos (vêm do Qt, sem tradução no FreeCAD)
    "Close": "Fechar", "&Close": "&Fechar", "Cancel": "Cancelar", "&Cancel": "&Cancelar",
    "Apply": "Aplicar", "&Apply": "&Aplicar", "Help": "Ajuda", "&Help": "&Ajuda",
    "Reset": "Redefinir", "Restore Defaults": "Restaurar padrões", "&Yes": "&Sim", "&No": "&Não",
    "Yes to &All": "Sim para &todos", "N&o to All": "Nã&o para todos", "Save": "Salvar", "&Save": "&Salvar",
    "Save All": "Salvar todos", "Discard": "Descartar", "Don't Save": "Não salvar", "Abort": "Abortar",
    "Retry": "Tentar de novo", "Ignore": "Ignorar", "&OK": "&OK", "Open": "Abrir", "&Open": "&Abrir",
    # Vistas
    "&1 Front": "&1 Frontal", "&2 Top": "&2 Superior", "&3 Right": "&3 Direita",
    "&4 Rear": "&4 Traseira", "&5 Bottom": "&5 Inferior", "&6 Left": "&6 Esquerda",
    "Stereo Interleaved &Columns": "Estéreo intercalado por &colunas",
    "Stereo Interleaved &Rows": "Estéreo intercalado por &linhas",
    "Stereo &Off": "Estéreo &desligado", "Stereo &Quad Buffer": "Estéreo &quad buffer",
    "Stereo Re&d/Cyan": "Estéreo &vermelho/ciano",
    # Árvore
    "&1 Sync View": "&1 Sincronizar vista", "&2 Sync Selection": "&2 Sincronizar seleção",
    "&3 Sync Placement": "&3 Sincronizar posicionamento", "&4 Preselection": "&4 Pré-seleção",
    "&5 Record Selection": "&5 Registrar seleção",
    # Arquivo / ferramentas / macro
    "Save Cop&y": "Salvar &cópia", "Clear Recent Files": "Limpar arquivos recentes",
    "&Addon Manager": "Gerenciador de &complementos", "Cu&stomize…": "&Personalizar…",
    "Cu&stomize...": "&Personalizar…", "Ma&cros": "Ma&cros…", "&Material": "&Material",
    "Export Dependency &Graph": "Exportar &grafo de dependências",
    "&Debug Macro": "&Depurar macro", "Step &Into": "Entrar na &função", "Step &Over": "&Pular função",
    "&Stop Debugging": "&Parar depuração", "Toggle &Breakpoint": "Alternar ponto de &interrupção",
    "&Part Design": "&Peça", "Python Console": "Console Python", "Python console": "Console Python",
    "Report View": "Relatório", "Report view": "Relatório",
    # Part / PartDesign
    "Sub-Shape Binder": "Vínculo de subforma", "Cone": "Cone", "Loft": "Loft",
    # Planilha
    "Align &Bottom": "Alinhar na &base", "Align Horizontal &Center": "Centralizar &horizontalmente",
    "Align &Left": "Alinhar à &esquerda", "Align &Right": "Alinhar à &direita", "Align &Top": "Alinhar no &topo",
    "Align &Vertical Center": "Centralizar &verticalmente", "&New Spreadsheet": "&Nova planilha",
    "&Export Spreadsheet": "&Exportar planilha", "&Import Spreadsheet": "&Importar planilha",
    "&Merge Cells": "&Mesclar células", "Set Alias": "Definir apelido", "Sp&lit Cell": "&Dividir célula",
    "&Bold Text": "&Negrito", "&Italic Text": "&Itálico", "&Underline Text": "&Sublinhado",
    # Desenho técnico
    "Centerline Between 2 Lines": "Linha de centro entre 2 linhas",
    "Centerline Between 2 Points": "Linha de centro entre 2 pontos",
    "Centerline Between 2 Faces": "Linha de centro entre 2 faces",
    "Cosmetic Line Through 2 Points": "Linha cosmética por 2 pontos",
    "Angle Dimension From 3 Points": "Cota angular por 3 pontos",
    "Active View": "Vista da tela", "Align Vertices/Edge Horizontally": "Alinhar vértices/aresta na horizontal",
    "Align Vertices/Edge Vertically": "Alinhar vértices/aresta na vertical",
    "Angle Dimension": "Cota angular", "Text Annotation": "Anotação de texto", "BIM View": "Vista BIM",
    "Axonometric Length Dimension": "Cota axonométrica", "Balloon Annotation": "Balão",
    "Offset Vertex": "Vértice deslocado", "Complex Section View": "Corte complexo",
    "Cosmetic 1 Point Circle": "Círculo cosmético por 1 ponto",
    "Cosmetic 2 Point Circle": "Círculo cosmético por 2 pontos",
    "Cosmetic 3 Point Circle": "Círculo cosmético por 3 pontos", "Cosmetic Arc": "Arco cosmético",
    "Edit Line Appearance": "Editar aparência da linha", "Diameter Dimension": "Cota de diâmetro",
    "Repair Dimension References": "Reparar referências das cotas", "Draft View": "Vista 2D (Draft)",
    "Arc Length Annotation": "Anotação de comprimento de arco", "Area Annotation": "Anotação de área",
    "Horizontal Chain Dimension": "Cota em cadeia horizontal",
    "Horizontal Chamfer Dimension": "Cota de chanfro horizontal",
    "Horizontal Coordinate Dimension": "Cota por coordenadas horizontal",
    "Arc Length Dimension": "Cota de comprimento de arco",
    "Oblique Chain Dimension": "Cota em cadeia oblíqua",
    "Oblique Coordinate Dimension": "Cota por coordenadas oblíqua",
    "Vertical Chain Dimension": "Cota em cadeia vertical",
    "Vertical Chamfer Dimension": "Cota de chanfro vertical",
    "Vertical Coordinate Dimension": "Cota por coordenadas vertical",
    "Bolt Circle Centerlines": "Linhas de centro de furação circular",
    "Insert 'n×' Prefix": "Inserir prefixo 'n×'", "Cosmetic Parallel Line": "Linha cosmética paralela",
    "Cosmetic Perpendicular Line": "Linha cosmética perpendicular", "Toggle View Lock": "Travar/destravar vista",
    "Align Chain Dimensions Horizontally": "Alinhar cotas em cadeia na horizontal",
    "Align Oblique Chain Dimensions": "Alinhar cotas em cadeia oblíquas",
    "Align Chain Dimensions Vertically": "Alinhar cotas em cadeia na vertical",
    "Position Section View": "Posicionar vista em corte",
    "Cosmetic Thread Bolt Bottom View": "Rosca cosmética de parafuso (vista inferior)",
    "Cosmetic Thread Bolt Side View": "Rosca cosmética de parafuso (vista lateral)",
    "Cosmetic Thread Hole Bottom View": "Rosca cosmética de furo (vista inferior)",
    "Cosmetic Thread Hole Side View": "Rosca cosmética de furo (vista lateral)",
    "Cosmetic Intersection Vertices": "Vértices cosméticos de interseção",
    "Update Template Fields": "Preencher campos do carimbo", "Geometric Hatch": "Hachura geométrica",
    "Image Hatch": "Hachura por imagem", "Hole/Shaft Fit": "Ajuste furo/eixo",
    "Horizontal Length Dimension": "Cota horizontal", "Horizontal Extent Dimension": "Cota de extensão horizontal",
    "Bitmap Image": "Imagem", "Length Dimension": "Cota linear", "Midpoint Vertices": "Vértices de ponto médio",
    "New Page": "Nova folha", "New Page From Template": "Nova folha a partir de modelo",
    "Project Shape": "Projetar forma", "Quadrant Vertices": "Vértices de quadrante",
    "Radius Dimension": "Cota de raio", "Rich Text Annotation": "Texto formatado", "Share View": "Compartilhar vista",
    "Toggle Edge Visibility": "Mostrar/ocultar arestas", "Spreadsheet View": "Vista de planilha",
    "Surface Finish Symbol": "Símbolo de rugosidade", "Insert SVG": "Inserir SVG",
    "Vertical Length Dimension": "Cota vertical", "Vertical Extent Dimension": "Cota de extensão vertical",
    "New View": "Nova vista", "Weld Symbol": "Símbolo de solda",
    # Títulos dos painéis de tarefa
    "Pad parameters": "Parâmetros da extrusão", "Pad Parameters": "Parâmetros da extrusão",
    "Pocket parameters": "Parâmetros da cavidade", "Pocket Parameters": "Parâmetros da cavidade",
    "Revolution parameters": "Parâmetros da revolução", "Revolution Parameters": "Parâmetros da revolução",
    "Groove parameters": "Parâmetros da ranhura", "Groove Parameters": "Parâmetros da ranhura",
    "Hole parameters": "Parâmetros do furo", "Hole Parameters": "Parâmetros do furo",
    "Fillet parameters": "Parâmetros do arredondamento", "Fillet Parameters": "Parâmetros do arredondamento",
    "Chamfer parameters": "Parâmetros do chanfro", "Chamfer Parameters": "Parâmetros do chanfro",
    "Thickness parameters": "Parâmetros da casca", "Draft parameters": "Parâmetros do ângulo de saída",
    "Sketch normal": "Normal do esboço", "Taper angle": "Ângulo de inclinação",
    # Barras de ferramentas (títulos)
    "Individual Views": "Vistas individuais", "Part Design Helper Features": "Auxiliares de peça",
    "Part Design Modeling Features": "Modelagem de peça", "Part Design Dress-Up Features": "Acabamento de peça",
    "Part Design Transformation Features": "Padrões de peça", "Sketcher Tools": "Ferramentas de esboço",
}


class SeworkTranslator(QtCore.QTranslator):
    """Um único tradutor: dicionário EXTRA primeiro, depois o .qm completo.

    Retorna None (QString nula) quando não sabe traduzir, para o Qt seguir
    para o próximo tradutor da pilha.
    """

    def __init__(self, table, qm_path, parent=None):
        super().__init__(parent)
        self._t = dict(table)
        for k, v in list(table.items()):
            bare = k.replace("&", "")
            if bare not in self._t:
                self._t[bare] = v.replace("&", "")
        try:
            from SeworkPropNames import PROPS
        except Exception:  # pragma: no cover
            PROPS = {}
        self._props = PROPS
        self._qm = QtCore.QTranslator()
        self._qm_ok = bool(qm_path) and self._qm.load(qm_path)

    def translate(self, context, sourceText, disambiguation=None, n=-1):
        if context == "App::Property":
            hit = self._props.get(sourceText)
            if hit:
                return hit
        hit = self._t.get(sourceText)
        if hit:
            return hit
        if self._qm_ok:
            res = self._qm.translate(context, sourceText, disambiguation, n)
            if res:
                return res
        return None

    def isEmpty(self):
        return False


_state = {"tr": None, "seen": set()}


def _app():
    return QtCore.QCoreApplication.instance()


def _language_is_ptbr():
    lang = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/General").GetString("Language", "")
    return "Brazil" in lang or lang in ("pt-BR",)


def raise_priority():
    """(Re)instala o tradutor Sework no topo da pilha do Qt."""
    tr = _state["tr"]
    if tr is None:
        return
    app = _app()
    app.removeTranslator(tr)
    app.installTranslator(tr)


def install():
    if not FreeCAD.ParamGet(PARAM_PATH).GetBool("SeworkTranslations", True):
        return
    if not _language_is_ptbr():
        return
    if _state["tr"] is None:
        qm = os.path.join(ADDON_DIR, "translations", "sework_ptbr.qm")
        _state["tr"] = SeworkTranslator(EXTRA, qm if os.path.isfile(qm) else "")
    raise_priority()


def uninstall():
    if _state["tr"] is not None:
        _app().removeTranslator(_state["tr"])


def on_workbench(name):
    """Na primeira ativação de cada bancada o módulo instala seus tradutores."""
    if name in _state["seen"]:
        return
    _state["seen"].add(name)
    QtCore.QTimer.singleShot(0, raise_priority)
