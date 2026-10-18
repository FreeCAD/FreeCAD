# SPDX-License-Identifier: LGPL-2.1-or-later
"""Assistente de simulação Sework (FEM) — passo a passo com verificação de pendências.

Etapas, na ordem que a análise exige:
  1 Estudo e peça · 2 Material · 3 Fixações · 4 Cargas · 5 Malha · 6 Verificar e calcular
  7 Resultados · 8 Relatório

Cada etapa mostra um estado (ok / atenção / falta) e o botão Calcular só é liberado quando
não há nenhuma pendência que faria o CalculiX falhar. Usa só os objetos padrão da bancada
FEM (análise, CalculiX, material, restrições, malha Gmsh), então o estudo continua editável
pelas ferramentas normais do FreeCAD.
"""
import os
import time

import FreeCAD
import FreeCADGui
from PySide import QtCore, QtGui, QtWidgets

import SeworkEstilo

HERE = os.path.dirname(os.path.abspath(__file__))
ICON = os.path.join(HERE, "Resources", "icons", "assistente_fem.svg")
Q = FreeCAD.Units.Quantity

ETAPAS = [
    ("estudo", "Estudo e peça", "O que vai ser analisado"),
    ("material", "Material", "Do que a peça é feita"),
    ("fixacao", "Fixações", "Onde a peça está presa"),
    ("carga", "Cargas", "O que age sobre a peça"),
    ("malha", "Malha", "Divisão da peça em elementos"),
    ("calculo", "Verificar e calcular", "Pendências e cálculo"),
    ("resultado", "Resultados", "Tensões, deslocamentos e segurança"),
    ("relatorio", "Relatório", "PDF no padrão Sework"),
]
TIPOS = [("static", "Estática linear — tensões e deslocamentos"),
         ("frequency", "Frequências naturais — modos de vibrar")]
NIVEIS_MALHA = [("Muito grossa", 8.0), ("Grossa", 15.0), ("Média", 25.0), ("Fina", 40.0), ("Muito fina", 60.0)]

OK, AVISO, ERRO, VAZIO = "ok", "aviso", "erro", "vazio"

QSS = """
#SwFem { background: #F3F5F8; }
#SwFem QLabel { color: #1F2A36; background: transparent; }
#SwFemTopo { background: #FFFFFF; border-bottom: 2px solid #F7BE00; }
#SwFemTitulo { color: #003A70; font-size: 12pt; font-weight: 700; }
#SwFemSub { color: #5B6876; font-size: 8.5pt; }
#SwEtapa { background: #FFFFFF; border: 1px solid #DCE4EC; border-radius: 8px; }
#SwEtapa[aberta="true"] { border: 1px solid #7FA3C9; }
#SwEtapaCab { background: transparent; border: none; text-align: left; padding: 0; }
#SwEtapaNum { color: #FFFFFF; background: #9AA7B4; border-radius: 11px; font-weight: 700; font-size: 9pt; }
#SwEtapaNum[estado="ok"] { background: #2E8B6A; }
#SwEtapaNum[estado="aviso"] { background: #E0A100; }
#SwEtapaNum[estado="erro"] { background: #B5523F; }
#SwEtapaTit { color: #003A70; font-weight: 700; font-size: 10pt; }
#SwEtapaRes { color: #5B6876; font-size: 8.5pt; }
#SwEtapaCorpo { background: transparent; }
#SwDica { color: #4A5866; font-size: 8.5pt; background: #F5F8FC; border-radius: 6px; padding: 6px 8px; }
#SwMsg { font-size: 8.5pt; border-radius: 6px; padding: 5px 8px; }
#SwMsg[nivel="erro"] { background: #FBE9E5; color: #7A2E20; border-left: 3px solid #B5523F; }
#SwMsg[nivel="aviso"] { background: #FFF6D6; color: #4D3B00; border-left: 3px solid #E0A100; }
#SwMsg[nivel="ok"] { background: #E5F3ED; color: #1E5C46; border-left: 3px solid #2E8B6A; }
#SwValor { color: #003A70; font-size: 15pt; font-weight: 700; }
#SwValorRot { color: #5B6876; font-size: 8pt; }
#SwCartao { background: #F5F8FC; border: 1px solid #DCE4EC; border-radius: 8px; }
QPushButton#SwPrimario { background: #003A70; color: #FFFFFF; border: 1px solid #003A70; font-weight: 600; padding: 6px 16px; }
QPushButton#SwPrimario:hover { background: #0B4F8F; }
QPushButton#SwPrimario:disabled { background: #C9D5E2; border-color: #C9D5E2; color: #FFFFFF; }
"""


# =========================================================================== modelo
def _q(valor, unidade):
    try:
        return float(Q(valor).getValueAs(unidade))
    except Exception:
        return None


def _prop(obj, tipo, nome, grupo="Sework", doc=""):
    if obj is not None and nome not in obj.PropertiesList:
        obj.addProperty(tipo, nome, grupo, doc)
    return obj


def _faces_validas(obj, subs):
    try:
        n = len(obj.Shape.Faces)
    except Exception:
        return False
    for s in subs:
        if not s.startswith("Face"):
            return False
        try:
            if not 1 <= int(s[4:]) <= n:
                return False
        except ValueError:
            return False
    return True


class Estudo:
    """Lê (e cria quando preciso) os objetos FEM de um documento."""

    def __init__(self, doc=None):
        self.doc = doc or FreeCAD.ActiveDocument
        self.analise = None
        if self.doc is None:
            return
        anal = [o for o in self.doc.Objects if o.isDerivedFrom("Fem::FemAnalysis")]
        marcadas = [a for a in anal if "SeworkTipo" in a.PropertiesList]
        try:
            import FemGui
            ativa = FemGui.getActiveAnalysis()
        except Exception:
            ativa = None
        if ativa is not None and ativa in anal:
            self.analise = ativa
        elif marcadas:
            self.analise = marcadas[0]
        elif anal:
            self.analise = anal[0]

    # ---- membros
    def _tipo(self, *tipos):
        if self.analise is None:
            return []
        out = []
        for o in self.analise.Group:
            tid = getattr(getattr(o, "Proxy", None), "Type", "") or o.TypeId
            for t in tipos:
                if o.isDerivedFrom(t) or tid == t:
                    out.append(o)
                    break
        return out

    @property
    def solver(self):
        s = self._tipo("Fem::SolverCcxTools")
        if not s:
            s = [o for o in self._tipo("Fem::FemSolverObjectPython") if hasattr(o, "AnalysisType")]
        return s[0] if s else None

    @property
    def material(self):
        m = self._tipo("Fem::MaterialCommon", "Fem::MaterialSolid")
        m = [o for o in m if getattr(o, "Category", "Solid") == "Solid"] or m
        return m[0] if m else None

    @property
    def fixacoes(self):
        return self._tipo("Fem::ConstraintFixed", "Fem::ConstraintDisplacement")

    @property
    def cargas(self):
        return self._tipo("Fem::ConstraintForce", "Fem::ConstraintPressure", "Fem::ConstraintSelfWeight")

    @property
    def malha(self):
        m = [o for o in self._tipo("Fem::FemMeshObject") if "Shape" in o.PropertiesList
             and not o.Name.startswith("ResultMesh")]
        return m[0] if m else None

    @property
    def resultados(self):
        return self._tipo("Fem::FemResultObject")

    @property
    def pipeline(self):
        p = self._tipo("Fem::FemPostPipeline")
        return p[-1] if p else None

    @property
    def peca(self):
        a = self.analise
        if a is not None and "SeworkPeca" in a.PropertiesList and a.SeworkPeca is not None:
            return a.SeworkPeca
        m = self.malha
        if m is not None and getattr(m, "Shape", None) is not None:
            return m.Shape
        return None

    @property
    def tipo(self):
        s = self.solver
        if s is not None:
            return s.AnalysisType if s.AnalysisType in ("static", "frequency") else "static"
        a = self.analise
        return a.SeworkTipo if a is not None and "SeworkTipo" in a.PropertiesList and a.SeworkTipo else "static"

    def get(self, nome, padrao=""):
        a = self.analise
        if a is not None and nome in a.PropertiesList:
            return getattr(a, nome)
        return padrao

    def put(self, nome, valor, tipo="App::PropertyString"):
        if self.analise is None:
            return
        _prop(self.analise, tipo, nome)
        setattr(self.analise, nome, valor)

    # ---- criação
    def garantir(self, peca=None):
        """Cria análise e solver CalculiX se não existirem."""
        import ObjectsFem
        doc = self.doc
        if self.analise is None:
            self.analise = ObjectsFem.makeAnalysis(doc, "Simulacao")
            self.analise.Label = "Simulação"
        _prop(self.analise, "App::PropertyString", "SeworkTipo")
        _prop(self.analise, "App::PropertyLink", "SeworkPeca")
        if peca is not None:
            self.analise.SeworkPeca = peca
        if self.solver is None:
            s = ObjectsFem.makeSolverCalculiXCcxTools(doc, "CalculiX")
            self.analise.addObject(s)
            s.AnalysisType = self.analise.SeworkTipo or "static"
        try:
            import FemGui
            FemGui.setActiveAnalysis(self.analise)
        except Exception:
            pass
        return self.analise

    def assinatura_geo(self):
        p = self.peca
        try:
            s = p.Shape
            bb = s.BoundBox
            return "%.4f|%.4f|%.3f,%.3f,%.3f|%d" % (s.Volume, s.Area, bb.XLength, bb.YLength, bb.ZLength,
                                                  len(s.Faces))
        except Exception:
            return ""


# =========================================================================== materiais
_CACHE_MAT = {}

# Biblioteca Sework (aços do mercado brasileiro) aparece primeiro
PASTAS_SEWORK = [("Acos para vasos de pressao", "Sework · Aços para vasos de pressão"),
                 ("Acos estruturais", "Sework · Aços estruturais"),
                 ("Tubos e forjados", "Sework · Tubos e forjados"),
                 ("Acos inoxidaveis", "Sework · Aços inoxidáveis"),
                 ("Acos para construcao mecanica", "Sework · Construção mecânica"),
                 ("Aluminio", "Sework · Alumínio")]
GRUPOS_MAT = [("Steel", "Aços e ferros fundidos"), ("Aluminum", "Alumínio"), ("Iron", "Ferro"),
              ("Copper", "Cobre e latão"), ("Titanium", "Titânio"), ("Alloys", "Ligas"),
              ("Metal", "Outros metais"), ("", "Outros")]


def materiais():
    """Materiais da biblioteca do FreeCAD com módulo de elasticidade definido."""
    if _CACHE_MAT:
        return _CACHE_MAT["lista"]
    lista = []
    try:
        import Materials
        mm = Materials.MaterialManager()
        for uuid in mm.Materials:
            try:
                m = mm.getMaterial(uuid)
                p = m.Properties
            except Exception:
                continue
            if "YoungsModulus" not in p or not p.get("YoungsModulus"):
                continue
            pasta = m.Directory or ""
            grupo = "Outros"
            if m.LibraryName == "Sework":
                grupo = next((nome for chave, nome in PASTAS_SEWORK if chave in pasta), "Sework")
            else:
                for chave, nome in GRUPOS_MAT:
                    if chave and chave in pasta:
                        grupo = nome
                        break
            nome = p.get("Name") or m.Name
            lista.append({"uuid": uuid, "nome": nome, "grupo": grupo, "pasta": pasta,
                          "biblioteca": m.LibraryName, "props": dict(p),
                          "descricao": getattr(m, "Description", "") or ""})
    except Exception as exc:
        FreeCAD.Console.PrintWarning("Sework: biblioteca de materiais indisponível: {}\n".format(exc))
    ordem = {nome: i for i, (_c, nome) in enumerate(PASTAS_SEWORK + GRUPOS_MAT)}
    lista.sort(key=lambda d: (ordem.get(d["grupo"], 99), d["nome"].lower()))
    _CACHE_MAT["lista"] = lista
    return lista


def props_mat(p):
    """Propriedades principais em unidades de engenharia (MPa, kg/m³)."""
    return {
        "E": _q(p.get("YoungsModulus", ""), "MPa") if p.get("YoungsModulus") else None,
        "nu": float(p["PoissonRatio"]) if p.get("PoissonRatio") else None,
        "rho": _q(p.get("Density", ""), "kg/m^3") if p.get("Density") else None,
        "Sy": _q(p.get("YieldStrength", ""), "MPa") if p.get("YieldStrength") else None,
        "Su": _q(p.get("UltimateTensileStrength", ""), "MPa") if p.get("UltimateTensileStrength") else None,
        "norma": p.get("StandardCode", ""), "numero": p.get("MaterialNumber", ""),
    }


def fmt(v, casas=2, unidade=""):
    if v is None:
        return "—"
    s = "{:,.{c}f}".format(v, c=casas).replace(",", "X").replace(".", ",").replace("X", ".")
    return s + (" " + unidade if unidade else "")


# =========================================================================== verificação
def _normal_externa(peca, nome_face):
    """Normal para fora do sólido, ou None se a face não for plana."""
    try:
        f = peca.Shape.getElement(nome_face)
        if "Plane" not in type(f.Surface).__name__:
            return None
        u0, u1, v0, v1 = f.ParameterRange
        n = f.normalAt((u0 + u1) / 2.0, (v0 + v1) / 2.0)
        return n.normalize() if n.Length > 1e-12 else None
    except Exception:
        return None


def _grupos_por_normal(peca, faces):
    """Agrupa as faces pela normal externa. O FreeCAD usa uma única direção por carga (a da
    primeira face); faces opostas ou inclinadas viram cargas separadas, cada uma perpendicular
    à sua face, com a força total dividida pela área."""
    grupos = []
    for nome in faces:
        n = _normal_externa(peca, nome)
        if n is None:
            return None, ("“{}” não é plana: força perpendicular só vale para faces planas. Para faces curvas "
                          "use Pressão, que já age perpendicular à superfície em cada ponto.".format(nome))
        area = peca.Shape.getElement(nome).Area
        for g in grupos:
            if g["normal"].dot(n) > 0.9999:
                g["faces"].append(nome)
                g["area"] += area
                break
        else:
            grupos.append({"normal": n, "faces": [nome], "area": area})
    return grupos, ""


def _orientar_forca(c, normal_externa, puxar=False):
    """Força perpendicular: por padrão empurra a face (para dentro da peça), como a pressão;
    puxar=True inverte. Sem direção escolhida, o FreeCAD usa a normal externa da face (puxa)."""
    alvo = normal_externa if puxar else -normal_externa
    c.Reversed = False
    c.recompute()
    if c.DirectionVector.dot(alvo) < 0:
        c.Reversed = True
        c.recompute()


def _rotulo_carga(c):
    if _eh(c, "Fem::ConstraintPressure"):
        txt = "Pressão {} MPa".format(fmt(_valor_carga(c) or 0.0, 2))
        return txt + (" (puxando)" if c.Reversed else "")
    if _eh(c, "Fem::ConstraintForce"):
        txt = "Força {} N".format(fmt(_valor_carga(c) or 0.0, 0))
        try:
            refs = c.References
            peca, nome = refs[0][0], refs[0][1][0]
            n = _normal_externa(peca, nome)
            if n is not None and abs(c.DirectionVector.dot(n)) > 0.999:
                return txt + (" (empurra)" if c.DirectionVector.dot(n) < 0 else " (puxa)")
        except Exception:
            pass
        return txt + (" (invertida)" if c.Reversed else "")
    return c.Label


def avaliar(est):
    """Estado de cada etapa: {chave: (nivel, resumo, [(nivel, mensagem)])}."""
    r = {}
    tipo = est.tipo if est.analise is not None else "static"

    # 1 estudo e peça
    msgs = []
    peca = est.peca
    if est.analise is None or peca is None:
        r["estudo"] = (VAZIO, "Escolha a peça que será analisada", [(ERRO, "Nenhuma peça escolhida.")])
    else:
        nivel = OK
        try:
            sh = peca.Shape
            nsol = len(sh.Solids)
            if nsol == 0:
                msgs.append((ERRO, "A peça não tem sólido (é só superfície ou esboço). Escolha um sólido."))
            elif nsol > 1:
                msgs.append((AVISO, "A peça tem {} sólidos separados. Eles só funcionam juntos se estiverem "
                                    "fundidos (união booleana); sólidos soltos fazem o cálculo falhar.".format(nsol)))
            if not sh.isValid():
                msgs.append((ERRO, "A geometria tem defeitos. Use Peça › Verificar geometria e corrija antes."))
        except Exception:
            msgs.append((ERRO, "Não foi possível ler a geometria da peça."))
        if any(n == ERRO for n, _m in msgs):
            nivel = ERRO
        elif msgs:
            nivel = AVISO
        r["estudo"] = (nivel, "{} · {}".format(peca.Label, dict(TIPOS)[tipo].split(" — ")[0]), msgs)

    # 2 material
    m = est.material
    if m is None:
        r["material"] = (VAZIO, "Escolha o material", [(ERRO, "Nenhum material definido.")])
    else:
        pm = props_mat(m.Material)
        msgs = []
        if not pm["E"]:
            msgs.append((ERRO, "O material não tem módulo de elasticidade."))
        if pm["nu"] is None:
            msgs.append((ERRO, "O material não tem coeficiente de Poisson."))
        precisa_rho = tipo == "frequency" or any(_eh(c, "Fem::ConstraintSelfWeight") for c in est.cargas)
        if precisa_rho and not pm["rho"]:
            msgs.append((ERRO, "O material não tem densidade (necessária para peso próprio e frequências)."))
        if not pm["Sy"] and tipo == "static":
            msgs.append((AVISO, "Sem limite de escoamento no material: o fator de segurança não será calculado."))
        if m.References:
            msgs.append((AVISO, "O material está ligado só a parte da peça. O assistente usa material único "
                                "para a peça inteira."))
        nivel = ERRO if any(n == ERRO for n, _ in msgs) else (AVISO if msgs else OK)
        r["material"] = (nivel, m.Material.get("Name", m.Label), msgs)

    # 3 fixações
    fx = est.fixacoes
    msgs = []
    for c in fx:
        subs = [s for _o, ss in c.References for s in ss]
        if not c.References:
            msgs.append((ERRO, "“{}” não tem faces escolhidas.".format(c.Label)))
        elif peca is not None and not all(o == peca or _mesma_geo(o, peca) for o, _s in c.References):
            msgs.append((AVISO, "“{}” aponta para outro objeto que não é a peça do estudo.".format(c.Label)))
        elif not all(_faces_validas(o, ss) for o, ss in c.References):
            msgs.append((ERRO, "“{}” aponta para faces que não existem mais (a peça mudou). "
                               "Refaça a seleção.".format(c.Label)))
        del subs
    if not fx:
        if tipo == "frequency":
            r["fixacao"] = (AVISO, "Sem fixação (peça livre)",
                            [(AVISO, "Sem fixação, os 6 primeiros modos saem com frequência ≈ 0 Hz "
                                     "(movimento de corpo rígido). Fixe a peça se ela é presa na prática.")])
        else:
            r["fixacao"] = (VAZIO, "Indique onde a peça é presa",
                            [(ERRO, "Sem fixação a peça “flutua” e o cálculo estático falha "
                                    "(movimento de corpo rígido).")])
    else:
        nivel = ERRO if any(n == ERRO for n, _ in msgs) else (AVISO if msgs else OK)
        r["fixacao"] = (nivel, "1 fixação" if len(fx) == 1 else "{} fixações".format(len(fx)), msgs)

    # 4 cargas
    cg = est.cargas
    msgs = []
    for c in cg:
        if _eh(c, "Fem::ConstraintSelfWeight"):
            continue
        if not c.References:
            msgs.append((ERRO, "“{}” não tem faces escolhidas.".format(c.Label)))
        elif not all(_faces_validas(o, ss) for o, ss in c.References):
            msgs.append((ERRO, "“{}” aponta para faces que não existem mais. Refaça a seleção.".format(c.Label)))
        val = _valor_carga(c)
        if val is not None and abs(val) < 1e-12:
            msgs.append((ERRO, "“{}” está com valor zero.".format(c.Label)))
    if tipo == "frequency":
        r["carga"] = (OK, "Não é necessária em frequências naturais" if not cg else _n_cargas(len(cg)),
                      msgs if cg else [(OK, "Frequências naturais não dependem de cargas.")])
    elif not cg:
        r["carga"] = (VAZIO, "Aplique pelo menos uma carga", [(ERRO, "Sem carga não há o que calcular.")])
    else:
        nivel = ERRO if any(n == ERRO for n, _ in msgs) else (AVISO if msgs else OK)
        r["carga"] = (nivel, _n_cargas(len(cg)), msgs)

    # 5 malha
    me = est.malha
    if me is None or me.FemMesh.NodeCount == 0:
        r["malha"] = (VAZIO, "Gere a malha", [(ERRO, "A malha ainda não foi gerada.")])
    else:
        msgs = []
        fm = me.FemMesh
        if fm.VolumeCount == 0:
            msgs.append((ERRO, "A malha não tem elementos de volume. Gere novamente a partir do sólido."))
        if est.get("SeworkMalhaAssin") and est.get("SeworkMalhaAssin") != est.assinatura_geo():
            msgs.append((ERRO, "A peça mudou depois que a malha foi gerada: gere a malha novamente."))
        if peca is not None and getattr(me, "Shape", None) not in (None, peca):
            msgs.append((AVISO, "A malha foi gerada de outro objeto ({}).".format(me.Shape.Label)))
        if fm.NodeCount > 400000:
            msgs.append((AVISO, "Malha muito grande ({} nós): o cálculo pode levar muitos minutos e usar "
                                "muita memória.".format(fmt(fm.NodeCount, 0))))
        nivel = ERRO if any(n == ERRO for n, _ in msgs) else (AVISO if msgs else OK)
        r["malha"] = (nivel, "{} nós · {} elementos".format(fmt(fm.NodeCount, 0), fmt(fm.VolumeCount, 0)), msgs)

    # 6 cálculo
    bloq = [(k, mm) for k in ("estudo", "material", "fixacao", "carga", "malha")
            for n, mm in r[k][2] if n == ERRO]
    res = est.resultados
    if bloq:
        r["calculo"] = (VAZIO, "1 pendência antes de calcular" if len(bloq) == 1 else
                        "{} pendências antes de calcular".format(len(bloq)), [])
    elif res and not est.get("SeworkResultadoVelho"):
        r["calculo"] = (OK, "Calculado em {}".format(est.get("SeworkCalculadoEm", "—")), [])
    else:
        r["calculo"] = (AVISO, "Pronto para calcular", [])
    r["_bloqueios"] = bloq

    # 7 resultados
    if not res:
        r["resultado"] = (VAZIO, "Aparece depois do cálculo", [])
    else:
        r["resultado"] = (OK, resumo_resultado(est), [])
    # 8 relatório
    r["relatorio"] = (OK if est.get("SeworkRelatorioPDF") else VAZIO,
                      os.path.basename(est.get("SeworkRelatorioPDF")) if est.get("SeworkRelatorioPDF")
                      else "Gere o PDF depois dos resultados", [])
    return r


def _n_cargas(n):
    return "1 carga" if n == 1 else "{} cargas".format(n)


def _eh(obj, tipo):
    tid = getattr(getattr(obj, "Proxy", None), "Type", "") or obj.TypeId
    return obj.isDerivedFrom(tipo) or tid == tipo


def _mesma_geo(o, peca):
    """Corpo × recurso ponta do corpo (mesma forma)."""
    try:
        return peca.isDerivedFrom("PartDesign::Body") and o == peca.Tip
    except Exception:
        return False


def _valor_carga(c):
    try:
        if _eh(c, "Fem::ConstraintForce"):
            return float(Q(c.Force).getValueAs("N"))
        if _eh(c, "Fem::ConstraintPressure"):
            return float(Q(c.Pressure).getValueAs("MPa"))
    except Exception:
        return None
    return None


# =========================================================================== resultados
def estatisticas(est):
    res = est.resultados
    if not res:
        return None
    out = {"modos": []}
    if est.tipo == "frequency":
        for r in res:
            f = getattr(r, "EigenmodeFrequency", 0.0)
            modo = getattr(r, "Eigenmode", 0)
            out["modos"].append((modo, f))
        out["modos"].sort()
        return out
    r = res[0]
    try:
        from femresult import resulttools
        s = resulttools.get_stats(r, "Sabs")
        u = resulttools.get_stats(r, "Uabs")
        out["svm_max"] = s[1] if s else None
        out["u_max"] = u[1] if u else None
    except Exception:
        out["svm_max"] = max(r.vonMises) if r.vonMises else None
        out["u_max"] = max(r.DisplacementLengths) if r.DisplacementLengths else None
    m = est.material
    sy = props_mat(m.Material)["Sy"] if m is not None else None
    out["Sy"] = sy
    out["fs"] = sy / out["svm_max"] if sy and out.get("svm_max") else None
    return out


def resumo_resultado(est):
    e = estatisticas(est)
    if not e:
        return ""
    if est.tipo == "frequency":
        fs = [f for _m, f in e["modos"]]
        return "1º modo: {}".format(fmt(fs[0], 2, "Hz")) if fs else "sem modos"
    txt = "σ máx. {} · δ máx. {}".format(fmt(e.get("svm_max"), 1, "MPa"), fmt(e.get("u_max"), 3, "mm"))
    if e.get("fs"):
        txt += " · FS {}".format(fmt(e["fs"], 2))
    return txt


def mostrar_campo(est, campo):
    """campo: 'tensao' | 'deslocamento' | 'modelo'."""
    p = est.pipeline
    vis = campo != "modelo"
    try:
        for o in est.resultados:
            if getattr(o, "Mesh", None) is not None:
                o.Mesh.ViewObject.Visibility = False
        if est.malha is not None:
            est.malha.ViewObject.Visibility = False
        if p is not None:
            p.ViewObject.Visibility = vis
            if vis:
                p.ViewObject.DisplayMode = "Surface"
                if campo == "tensao":
                    p.ViewObject.Field = "von Mises Stress"
                else:
                    enums = p.ViewObject.getEnumerationsOfProperty("Field") or []
                    if "Displacement Magnitude" in enums:
                        p.ViewObject.Field = "Displacement Magnitude"
                    else:
                        p.ViewObject.Field = "Displacement"
                        try:
                            p.ViewObject.VectorMode = "Magnitude"
                        except Exception:
                            pass
        peca = est.peca
        if peca is not None:
            peca.ViewObject.Visibility = not vis
        for c in est.fixacoes + est.cargas:
            c.ViewObject.Visibility = not vis
    except Exception as exc:
        FreeCAD.Console.PrintWarning("Sework: não foi possível mostrar o resultado: {}\n".format(exc))


# =========================================================================== operações (sem interface)
def gerar_malha(est, tamanho, ordem="2nd", nivel=None):
    """Cria/atualiza a malha Gmsh da peça e gera. Retorna (ok, mensagem)."""
    import ObjectsFem
    m = est.malha
    if m is None:
        m = ObjectsFem.makeMeshGmsh(est.doc, "Malha")
        est.analise.addObject(m)
        m.Label = "Malha"
    m.Shape = est.peca
    m.CharacteristicLengthMax = "{} mm".format(round(tamanho, 4))
    m.CharacteristicLengthMin = "0 mm"
    m.ElementOrder = ordem
    m.SecondOrderLinear = False
    est.doc.recompute()
    from femmesh.gmshtools import GmshTools
    tool = GmshTools(m)
    if hasattr(tool, "run"):
        tool.run(blocking=True)
        if m.FemMesh.NodeCount == 0 and hasattr(tool, "update_properties"):
            tool.update_properties()
    else:
        tool.create_mesh()
    est.doc.recompute()
    if m.FemMesh.NodeCount == 0:
        return False, "O Gmsh não gerou elementos."
    est.put("SeworkMalhaAssin", est.assinatura_geo())
    if nivel is not None:
        est.put("SeworkMalhaNivel", nivel, "App::PropertyInteger")
    est.put("SeworkResultadoVelho", True, "App::PropertyBool")
    return True, ""


def _ccx_absoluto():
    """Caminho absoluto do ccx: preferência do usuário ou o que vem junto com o FreeCAD."""
    pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Ccx").GetString("ccxBinaryPath", "")
    if pref and os.path.isfile(pref):
        return os.path.abspath(pref)
    for nome in ("ccx.exe", "ccx"):
        c = os.path.join(FreeCAD.getHomePath(), "bin", nome)
        if os.path.isfile(c):
            return os.path.abspath(c)
    import shutil
    c = shutil.which("ccx")
    return os.path.abspath(c) if c else None


def calcular(est, modos=None):
    """Roda o CalculiX e carrega os resultados. Retorna (ok, erro, segundos)."""
    from femtools import ccxtools
    s = est.solver
    if modos and est.tipo == "frequency" and hasattr(s, "EigenmodesCount"):
        s.EigenmodesCount = int(modos)
    t0 = time.time()
    try:
        from femresult import resulttools
        resulttools.purge_results(est.analise)
    except Exception:
        pass
    fea = ccxtools.FemToolsCcx(est.analise, s)
    fea.update_objects()
    fea.setup_working_dir()
    # O FreeCAD procura o "ccx" também na pasta atual e pode devolver um caminho relativo;
    # como ele troca de pasta antes de rodar, o caminho relativo falha (WinError 2).
    # ccx_run() chama setup_ccx() de novo, então fixamos o caminho absoluto em toda chamada.
    _setup_original = fea.setup_ccx
    ccx_abs = _ccx_absoluto()
    fea.setup_ccx = lambda ccx_binary=None, **kw: _setup_original(ccx_binary or ccx_abs, **kw)
    fea.setup_ccx()
    if not fea.ccx_binary_present:
        return False, "O CalculiX (ccx) não foi encontrado. Confira em Preferências › FEM › CalculiX.", 0.0
    msg = fea.check_prerequisites()
    if msg:
        return False, "O FreeCAD apontou pré-requisitos faltando:\n" + msg, 0.0
    fea.write_inp_file()
    ret = fea.ccx_run()
    if ret != 0:
        return False, _explicar_erro_ccx(getattr(fea, "ccx_stdout", "") + getattr(fea, "ccx_stderr", "")), 0.0
    fea.load_results()
    if not est.resultados:
        return False, "O cálculo terminou sem resultados. Veja o painel Relatório.", 0.0
    dt = round(time.time() - t0, 1)
    est.put("SeworkCalculadoEm", time.strftime("%d/%m/%Y %H:%M"))
    if not getattr(est, "_em_convergencia", False):
        est.put("SeworkConvergencia", "")
    est.put("SeworkResultadoVelho", False, "App::PropertyBool")
    est.put("SeworkTempoCalculo", dt, "App::PropertyFloat")
    try:
        est.put("SeworkCcxVersao", "%d.%d" % fea.get_ccx_version())
    except Exception:
        pass
    return True, "", dt


LIMITE_CONVERGENCIA = 5.0   # % de variação entre as duas malhas mais finas


def convergencia(est, fatores=(1.0, 0.7, 0.5), ordem="2nd", progresso=None):
    """Recalcula com malhas cada vez mais finas e compara tensão e deslocamento máximos.

    Retorna lista de dicionários {tamanho, nos, svm, u}. A última malha (a mais fina) fica no estudo.
    Grava o resultado em SeworkConvergencia (JSON) para o relatório.
    """
    import json
    m = est.malha
    if m is None:
        raise RuntimeError("Gere a malha antes de verificar a convergência.")
    base = _q(str(m.CharacteristicLengthMax), "mm") or 1.0
    est._em_convergencia = True
    linhas = []
    for i, f in enumerate(fatores):
        if progresso:
            progresso(i, len(fatores))
        tam = base * f
        ok, msg = gerar_malha(est, tam, ordem)
        if not ok:
            raise RuntimeError("Malha de {} falhou: {}".format(fmt(tam, 2, "mm"), msg))
        ok, erro, _dt = calcular(est)
        if not ok:
            raise RuntimeError(erro)
        e = estatisticas(est)
        linhas.append({"tamanho": tam, "nos": est.malha.FemMesh.NodeCount, "svm": e.get("svm_max"),
                       "u": e.get("u_max")})
    est.put("SeworkConvergencia", json.dumps(linhas))
    return linhas


def avaliar_convergencia(linhas):
    """(convergiu_tensao, convergiu_desloc, var_tensao_%, var_desloc_%) entre as duas malhas mais finas."""
    if len(linhas) < 2:
        return None
    a, b = linhas[-2], linhas[-1]

    def var(x, y):
        return abs(y - x) / abs(y) * 100.0 if x and y else None
    vs, vu = var(a["svm"], b["svm"]), var(a["u"], b["u"])
    return (vs is not None and vs <= LIMITE_CONVERGENCIA, vu is not None and vu <= LIMITE_CONVERGENCIA, vs, vu)


def linhas_convergencia(est):
    import json
    try:
        return json.loads(est.get("SeworkConvergencia", "") or "[]")
    except ValueError:
        return []


# =========================================================================== interface
class EstadoIcone(QtWidgets.QLabel):
    def __init__(self, n, parent=None):
        super().__init__(str(n), parent)
        self.setObjectName("SwEtapaNum")
        self.setFixedSize(22, 22)
        self.setAlignment(QtCore.Qt.AlignCenter)
        self.n = n

    def definir(self, estado):
        self.setProperty("estado", estado)
        self.setText({OK: "✓", AVISO: "!", ERRO: "✕"}.get(estado, str(self.n)))
        self.style().unpolish(self)
        self.style().polish(self)


class Etapa(QtWidgets.QFrame):
    abrir = QtCore.Signal(object)

    def __init__(self, n, chave, titulo, sub, parent=None):
        super().__init__(parent)
        self.chave = chave
        self.setObjectName("SwEtapa")
        lay = QtWidgets.QVBoxLayout(self)
        lay.setContentsMargins(10, 8, 10, 8)
        lay.setSpacing(6)
        cab = QtWidgets.QPushButton()
        cab.setObjectName("SwEtapaCab")
        cab.setCursor(QtCore.Qt.PointingHandCursor)
        cab.setFlat(True)
        hl = QtWidgets.QHBoxLayout(cab)
        hl.setContentsMargins(0, 0, 0, 0)
        hl.setSpacing(8)
        self.icone = EstadoIcone(n)
        hl.addWidget(self.icone, 0, QtCore.Qt.AlignTop)
        tl = QtWidgets.QVBoxLayout()
        tl.setSpacing(0)
        self.tit = QtWidgets.QLabel(titulo)
        self.tit.setObjectName("SwEtapaTit")
        self.res = QtWidgets.QLabel(sub)
        self.res.setObjectName("SwEtapaRes")
        self.res.setWordWrap(True)
        tl.addWidget(self.tit)
        tl.addWidget(self.res)
        hl.addLayout(tl, 1)
        cab.setMinimumHeight(40)
        cab.clicked.connect(lambda: self.abrir.emit(self))
        lay.addWidget(cab)
        self.corpo = QtWidgets.QWidget()
        self.corpo.setObjectName("SwEtapaCorpo")
        self.corpo_lay = QtWidgets.QVBoxLayout(self.corpo)
        self.corpo_lay.setContentsMargins(30, 0, 0, 2)
        self.corpo_lay.setSpacing(7)
        lay.addWidget(self.corpo)
        self.msgs = QtWidgets.QVBoxLayout()
        self.msgs.setSpacing(4)
        self.corpo_lay.addLayout(self.msgs)
        self.corpo.setVisible(False)
        self._msgs_key = None

    def aberta(self):
        return self.corpo.isVisible()

    def set_aberta(self, v):
        self.corpo.setVisible(v)
        self.setProperty("aberta", "true" if v else "false")
        self.style().unpolish(self)
        self.style().polish(self)

    def atualizar(self, estado, resumo, mensagens):
        self.icone.definir(estado)
        self.res.setText(resumo)
        key = tuple(mensagens)
        if key == self._msgs_key:
            return
        self._msgs_key = key
        while self.msgs.count():
            w = self.msgs.takeAt(0).widget()
            if w is not None:
                w.deleteLater()
        for nivel, texto in mensagens:
            lb = QtWidgets.QLabel(texto)
            lb.setObjectName("SwMsg")
            lb.setProperty("nivel", nivel)
            lb.setWordWrap(True)
            self.msgs.addWidget(lb)


def _dica(texto):
    lb = QtWidgets.QLabel(texto)
    lb.setObjectName("SwDica")
    lb.setWordWrap(True)
    return lb


def _linha(*widgets):
    w = QtWidgets.QWidget()
    hl = QtWidgets.QHBoxLayout(w)
    hl.setContentsMargins(0, 0, 0, 0)
    hl.setSpacing(6)
    for x in widgets:
        if x == "stretch":
            hl.addStretch(1)
        else:
            hl.addWidget(x)
    return w


def _cartao(valor, rotulo):
    f = QtWidgets.QFrame()
    f.setObjectName("SwCartao")
    v = QtWidgets.QVBoxLayout(f)
    v.setContentsMargins(10, 6, 10, 6)
    v.setSpacing(0)
    a = QtWidgets.QLabel(valor)
    a.setObjectName("SwValor")
    b = QtWidgets.QLabel(rotulo)
    b.setObjectName("SwValorRot")
    b.setWordWrap(True)
    v.addWidget(a)
    v.addWidget(b)
    f.valor = a
    f.rotulo = b
    return f


class Assistente(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("SwFem")
        self.setStyleSheet(SeworkEstilo.tema(QSS))
        self._ocupado = False
        raiz = QtWidgets.QVBoxLayout(self)
        raiz.setContentsMargins(0, 0, 0, 0)
        raiz.setSpacing(0)

        topo = QtWidgets.QFrame()
        topo.setObjectName("SwFemTopo")
        tl = QtWidgets.QHBoxLayout(topo)
        tl.setContentsMargins(12, 10, 12, 10)
        ic = QtWidgets.QLabel()
        ic.setPixmap(QtGui.QIcon(ICON).pixmap(34, 34))
        tl.addWidget(ic)
        tv = QtWidgets.QVBoxLayout()
        tv.setSpacing(0)
        t = QtWidgets.QLabel("Assistente de simulação")
        t.setObjectName("SwFemTitulo")
        self.sub = QtWidgets.QLabel("Siga as etapas na ordem. O cálculo libera quando não houver pendências.")
        self.sub.setObjectName("SwFemSub")
        self.sub.setWordWrap(True)
        tv.addWidget(t)
        tv.addWidget(self.sub)
        tl.addLayout(tv, 1)
        raiz.addWidget(topo)

        area = QtWidgets.QScrollArea()
        area.setWidgetResizable(True)
        area.setFrameShape(QtWidgets.QFrame.NoFrame)
        area.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        cont = QtWidgets.QWidget()
        cont.setObjectName("SwFem")
        self.lista = QtWidgets.QVBoxLayout(cont)
        self.lista.setContentsMargins(10, 10, 10, 10)
        self.lista.setSpacing(8)
        area.setWidget(cont)
        raiz.addWidget(area, 1)

        self.etapas = {}
        for i, (chave, titulo, sub) in enumerate(ETAPAS, 1):
            e = Etapa(i, chave, titulo, sub)
            e.abrir.connect(self._alternar)
            self.etapas[chave] = e
            self.lista.addWidget(e)
            getattr(self, "_montar_" + chave)(e.corpo_lay)
        self.lista.addStretch(1)

        self.timer = QtCore.QTimer(self)
        self.timer.setInterval(900)
        self.timer.timeout.connect(self.atualizar)
        self.timer.start()
        self._doc = None
        self.atualizar()
        self._abrir_proxima()

    # ------------------------------------------------------------ util
    def est(self):
        return Estudo()

    def _alternar(self, etapa):
        abrir = not etapa.aberta()
        for e in self.etapas.values():
            e.set_aberta(False)
        etapa.set_aberta(abrir)
        if abrir:
            self._ao_abrir(etapa.chave)

    def abrir_etapa(self, chave):
        for k, e in self.etapas.items():
            e.set_aberta(k == chave)
        self._ao_abrir(chave)

    def _abrir_proxima(self):
        r = avaliar(self.est())
        for chave, _t, _s in ETAPAS:
            if r[chave][0] in (VAZIO, ERRO):
                self.abrir_etapa(chave)
                return
        self.abrir_etapa("resultado")

    def _ao_abrir(self, chave):
        if chave == "material":
            self._carregar_materiais()
        if chave in ("fixacao", "carga"):
            self._listar_restricoes()

    def _erro(self, texto):
        QtWidgets.QMessageBox.warning(self, "Assistente de simulação", texto)

    def _transacao(self, nome, fn):
        doc = FreeCAD.ActiveDocument
        if doc is None:
            self._erro("Abra ou crie um documento com a peça primeiro.")
            return None
        doc.openTransaction(nome)
        try:
            out = fn()
            doc.recompute()
            doc.commitTransaction()
            return out
        except Exception as exc:
            doc.abortTransaction()
            FreeCAD.Console.PrintError("Sework FEM: {}\n".format(exc))
            self._erro("Não foi possível concluir: {}".format(exc))
            return None
        finally:
            self.atualizar(force=True)

    # ------------------------------------------------------------ 1 estudo
    def _montar_estudo(self, lay):
        lay.addWidget(_dica("Selecione a peça na vista 3D ou na árvore e clique em “Usar peça selecionada”. "
                            "Para peças feitas na bancada Peça, selecione o corpo (Body)."))
        self.cb_tipo = QtWidgets.QComboBox()
        for k, t in TIPOS:
            self.cb_tipo.addItem(t, k)
        self.cb_tipo.currentIndexChanged.connect(self._tipo_mudou)
        lay.addWidget(QtWidgets.QLabel("Tipo de análise"))
        lay.addWidget(self.cb_tipo)
        b = QtWidgets.QPushButton("Usar peça selecionada")
        b.clicked.connect(self._usar_peca)
        lay.addWidget(_linha(b, "stretch"))
        self.lb_peca = QtWidgets.QLabel("")
        self.lb_peca.setWordWrap(True)
        lay.addWidget(self.lb_peca)

    def _tipo_mudou(self):
        if getattr(self, "_carregando", False):
            return
        k = self.cb_tipo.currentData()
        est = self.est()
        if est.analise is None:
            return

        def fn():
            est.put("SeworkTipo", k)
            s = est.solver
            if s is not None:
                s.AnalysisType = k
                if k == "frequency" and hasattr(s, "EigenmodesCount") and s.EigenmodesCount < 1:
                    s.EigenmodesCount = 6
            est.put("SeworkResultadoVelho", True, "App::PropertyBool")
        self._transacao("Tipo de análise", fn)

    def _usar_peca(self):
        sel = FreeCADGui.Selection.getSelection()
        if not sel:
            self._erro("Selecione a peça (sólido ou corpo) antes.")
            return
        obj = sel[0]
        # recurso dentro de um corpo → o corpo
        try:
            for o in obj.InList:
                if o.isDerivedFrom("PartDesign::Body"):
                    obj = o
                    break
        except Exception:
            pass
        if not hasattr(obj, "Shape") or not obj.Shape.Solids:
            self._erro("“{}” não é um sólido. Selecione a peça (corpo ou sólido).".format(obj.Label))
            return
        k = self.cb_tipo.currentData()

        def fn():
            est = self.est()
            est.garantir(obj)
            est.put("SeworkTipo", k)
            if est.solver is not None:
                est.solver.AnalysisType = k
        self._transacao("Escolher peça", fn)
        self._abrir_proxima()

    def _info_peca(self, est):
        p = est.peca
        if p is None:
            return ""
        try:
            s = p.Shape
            bb = s.BoundBox
            txt = "<b>{}</b><br>Dimensões: {} × {} × {} mm<br>Volume: {} cm³ · {} face(s)".format(
                p.Label, fmt(bb.XLength, 1), fmt(bb.YLength, 1), fmt(bb.ZLength, 1),
                fmt(s.Volume / 1000.0, 2), len(s.Faces))
            m = est.material
            rho = props_mat(m.Material)["rho"] if m is not None else None
            if rho:
                txt += "<br>Massa: {} kg".format(fmt(s.Volume * 1e-9 * rho, 3))
            return txt
        except Exception:
            return p.Label

    # ------------------------------------------------------------ 2 material
    def _montar_material(self, lay):
        lay.addWidget(_dica("Materiais da biblioteca do FreeCAD. Digite para filtrar (ex.: S235, 1020, 6061)."))
        self.busca_mat = QtWidgets.QLineEdit()
        self.busca_mat.setPlaceholderText("Buscar material…")
        self.busca_mat.textChanged.connect(self._filtrar_mat)
        lay.addWidget(self.busca_mat)
        self.ck_so_sy = QtWidgets.QCheckBox("Mostrar só materiais com limite de escoamento")
        self.ck_so_sy.setChecked(True)
        self.ck_so_sy.toggled.connect(lambda _v: self._carregar_materiais(self.busca_mat.text()))
        lay.addWidget(self.ck_so_sy)
        self.cb_mat = QtWidgets.QComboBox()
        self.cb_mat.setMaxVisibleItems(18)
        self.cb_mat.currentIndexChanged.connect(self._mostrar_mat)
        lay.addWidget(self.cb_mat)
        self.lb_mat = QtWidgets.QLabel("")
        self.lb_mat.setWordWrap(True)
        self.lb_mat.setTextFormat(QtCore.Qt.RichText)
        lay.addWidget(self.lb_mat)
        self.sp_sy = QtWidgets.QDoubleSpinBox()
        self.sp_sy.setRange(0.0, 3000.0)
        self.sp_sy.setDecimals(0)
        self.sp_sy.setSuffix(" MPa")
        self.sp_sy.setSpecialValueText("usar o da biblioteca")
        lay.addWidget(QtWidgets.QLabel("Limite de escoamento do certificado (opcional)"))
        lay.addWidget(self.sp_sy)
        b = QtWidgets.QPushButton("Aplicar material à peça")
        b.clicked.connect(self._aplicar_mat)
        lay.addWidget(_linha(b, "stretch"))
        lay.addWidget(_dica("Os valores vêm da biblioteca do FreeCAD. Confirme com o certificado do material "
                            "antes de usar o resultado em projeto."))

    def _carregar_materiais(self, filtro=""):
        lst = materiais()
        atual = self.cb_mat.currentData()
        est = self.est()
        uuid_est = est.material.UUID if est.material is not None and hasattr(est.material, "UUID") else ""
        self.cb_mat.blockSignals(True)
        self.cb_mat.clear()
        f = filtro.lower().strip()
        grupo = None
        modelo = self.cb_mat.model()
        so_sy = self.ck_so_sy.isChecked()
        for d in lst:
            if f and f not in d["nome"].lower() and f not in d["grupo"].lower():
                continue
            sy = props_mat(d["props"])["Sy"]
            if so_sy and not sy and d["uuid"] != uuid_est:
                continue
            if d["grupo"] != grupo:
                grupo = d["grupo"]
                self.cb_mat.addItem("— {} —".format(grupo), None)
                it = modelo.item(self.cb_mat.count() - 1)
                if it is not None:
                    it.setEnabled(False)
            self.cb_mat.addItem("{}   ·   {}".format(d["nome"], "Sy {} MPa".format(fmt(sy, 0)) if sy
                                                   else "sem limite de escoamento"), d["uuid"])
        alvo = atual or uuid_est
        i = self.cb_mat.findData(alvo) if alvo else -1
        if i < 0:
            i = next((j for j in range(self.cb_mat.count()) if self.cb_mat.itemData(j)), -1)
        self.cb_mat.setCurrentIndex(i)
        self.cb_mat.blockSignals(False)
        self._mostrar_mat()

    def _filtrar_mat(self, txt):
        self._carregar_materiais(txt)

    def _mat_sel(self):
        u = self.cb_mat.currentData()
        return next((d for d in materiais() if d["uuid"] == u), None)

    def _mostrar_mat(self):
        d = self._mat_sel()
        if d is None:
            self.lb_mat.setText("")
            return
        p = props_mat(d["props"])
        linhas = [("Módulo de elasticidade", fmt(p["E"] / 1000.0 if p["E"] else None, 1, "GPa")),
                  ("Coeficiente de Poisson", fmt(p["nu"], 2)),
                  ("Densidade", fmt(p["rho"], 0, "kg/m³")),
                  ("Limite de escoamento", fmt(p["Sy"], 0, "MPa")),
                  ("Resistência à tração", fmt(p["Su"], 0, "MPa"))]
        if p["norma"]:
            linhas.append(("Referência na biblioteca", "{} {}".format(p["norma"], p["numero"]).strip()))
        html = "<table cellspacing='0' cellpadding='2'>" + "".join(
            "<tr><td style='color:#5B6876'>{}</td><td style='padding-left:10px'><b>{}</b></td></tr>".format(a, b)
            for a, b in linhas) + "</table>"
        if d.get("descricao"):
            html += "<p style='color:#5B6876; font-size:8pt; margin-top:4px'>{}</p>".format(d["descricao"])
        self.lb_mat.setText(SeworkEstilo.tema(html))

    def _aplicar_mat(self):
        d = self._mat_sel()
        if d is None:
            self._erro("Escolha um material na lista.")
            return
        est = self.est()
        if est.analise is None:
            self._erro("Escolha a peça na etapa 1 primeiro.")
            return

        def fn():
            import ObjectsFem
            import Materials
            m = est.material
            if m is None:
                m = ObjectsFem.makeMaterialSolid(est.doc, "Material")
                est.analise.addObject(m)
            mat = Materials.MaterialManager().getMaterial(d["uuid"])
            props = dict(mat.Properties)
            sy_cert = self.sp_sy.value()
            if sy_cert > 0:
                props["YieldStrength"] = "{} MPa".format(sy_cert)
            est.put("SeworkSyCertificado", float(sy_cert), "App::PropertyFloat")
            m.Material = props
            if hasattr(m, "UUID"):
                m.UUID = mat.UUID
            m.References = []
            m.Label = "Material · " + d["nome"]
            est.put("SeworkResultadoVelho", True, "App::PropertyBool")
        self._transacao("Material", fn)
        self._abrir_proxima()

    # ------------------------------------------------------------ 3 fixações
    def _montar_fixacao(self, lay):
        lay.addWidget(_dica("Selecione na peça as faces que ficam presas (furos de parafuso, base soldada, "
                            "apoio) — use Ctrl para várias — e clique em “Fixar faces selecionadas”."))
        b = QtWidgets.QPushButton("Fixar faces selecionadas")
        b.clicked.connect(self._fixar)
        lay.addWidget(_linha(b, "stretch"))
        self.lista_fix = QtWidgets.QListWidget()
        self.lista_fix.setMaximumHeight(110)
        lay.addWidget(self.lista_fix)
        br = QtWidgets.QPushButton("Remover selecionada")
        br.clicked.connect(lambda: self._remover(self.lista_fix))
        lay.addWidget(_linha(br, "stretch"))

    def _faces_selecionadas(self, est):
        """Faces selecionadas convertidas para nomes de face da peça do estudo."""
        peca = est.peca
        if peca is None:
            return None, "Escolha a peça na etapa 1 primeiro."
        alvo = peca.Shape.Faces
        diag = max(peca.Shape.BoundBox.DiagonalLength, 1.0)
        nomes = []
        for s in FreeCADGui.Selection.getSelectionEx():
            for nome, sub in zip(s.SubElementNames, s.SubObjects):
                nome = nome.split(".")[-1]
                if not nome.startswith("Face"):
                    continue
                if s.Object == peca or _mesma_geo(s.Object, peca):
                    if nome not in nomes:
                        nomes.append(nome)
                    continue
                # outro objeto (ex.: recurso do corpo): casa pela geometria
                for i, f in enumerate(alvo, 1):
                    if abs(f.Area - sub.Area) <= 1e-6 * max(f.Area, 1.0) and \
                            f.CenterOfMass.distanceToPoint(sub.CenterOfMass) <= 1e-6 * diag:
                        if "Face%d" % i not in nomes:
                            nomes.append("Face%d" % i)
                        break
        if not nomes:
            return None, "Selecione uma ou mais faces da peça na vista 3D (Ctrl + clique para várias)."
        return nomes, ""

    def _fixar(self):
        est = self.est()
        faces, msg = self._faces_selecionadas(est)
        if not faces:
            self._erro(msg)
            return

        def fn():
            import ObjectsFem
            c = ObjectsFem.makeConstraintFixed(est.doc, "Fixacao")
            c.References = [(est.peca, tuple(faces))]
            c.Label = "Fixação ({} face{})".format(len(faces), "s" if len(faces) > 1 else "")
            est.analise.addObject(c)
            est.put("SeworkResultadoVelho", True, "App::PropertyBool")
        self._transacao("Fixação", fn)
        FreeCADGui.Selection.clearSelection()

    def _listar_restricoes(self):
        est = self.est()
        for lista, objs in ((getattr(self, "lista_fix", None), est.fixacoes),
                            (getattr(self, "lista_carga", None), est.cargas)):
            if lista is None:
                continue
            nomes = [o.Name for o in objs]
            atuais = [lista.item(i).data(QtCore.Qt.UserRole) for i in range(lista.count())]
            rot = [o.Label + "  —  " + _descr_restricao(o) for o in objs]
            atuais_rot = [lista.item(i).text() for i in range(lista.count())]
            if nomes == atuais and rot == atuais_rot:
                continue
            lista.clear()
            for o, r in zip(objs, rot):
                it = QtWidgets.QListWidgetItem(r)
                it.setData(QtCore.Qt.UserRole, o.Name)
                lista.addItem(it)

    def _remover(self, lista):
        it = lista.currentItem()
        if it is None:
            return
        nome = it.data(QtCore.Qt.UserRole)

        def fn():
            est = self.est()
            est.doc.removeObject(nome)
            est.put("SeworkResultadoVelho", True, "App::PropertyBool")
        self._transacao("Remover", fn)

    # ------------------------------------------------------------ 4 cargas
    def _montar_carga(self, lay):
        lay.addWidget(_dica("Selecione as faces onde a carga atua e escolha o tipo. A força é dividida entre "
                            "as faces selecionadas. Perpendicular à face, ela empurra a face (como a pressão); "
                            "confira o sentido das setas na vista 3D e use “Inverter” se precisar."))
        self.cb_carga = QtWidgets.QComboBox()
        self.cb_carga.addItem("Força total (N)", "forca")
        self.cb_carga.addItem("Pressão (MPa)", "pressao")
        self.cb_carga.addItem("Peso próprio (gravidade, −Z)", "peso")
        self.cb_carga.currentIndexChanged.connect(self._tipo_carga)
        lay.addWidget(self.cb_carga)
        self.sp_carga = QtWidgets.QDoubleSpinBox()
        self.sp_carga.setDecimals(3)
        self.sp_carga.setRange(0.0, 1e9)
        self.sp_carga.setValue(1000.0)
        self.sp_carga.setSuffix(" N")
        self.cb_dir = QtWidgets.QComboBox()
        self.cb_dir.addItem("Perpendicular à face", "normal")
        self.cb_dir.addItem("Na direção de uma aresta (selecione também a aresta)", "aresta")
        self.ck_inv = QtWidgets.QCheckBox("Inverter sentido (puxar a face em vez de empurrar)")
        self.ck_inv.setToolTip("Vale para a próxima carga aplicada. Para uma carga já aplicada, use "
                               "“Inverter sentido da selecionada”.")
        lay.addWidget(self.sp_carga)
        lay.addWidget(self.cb_dir)
        lay.addWidget(self.ck_inv)
        b = QtWidgets.QPushButton("Aplicar carga")
        b.clicked.connect(self._aplicar_carga)
        lay.addWidget(_linha(b, "stretch"))
        self.lista_carga = QtWidgets.QListWidget()
        self.lista_carga.setMaximumHeight(110)
        lay.addWidget(self.lista_carga)
        br = QtWidgets.QPushButton("Remover selecionada")
        br.clicked.connect(lambda: self._remover(self.lista_carga))
        bi = QtWidgets.QPushButton("Inverter sentido da selecionada")
        bi.setToolTip("Troca o sentido da força ou da pressão escolhida na lista")
        bi.clicked.connect(self._inverter_carga)
        lay.addWidget(_linha(bi, br, "stretch"))

    def _inverter_carga(self):
        it = self.lista_carga.currentItem()
        if it is None:
            self._erro("Escolha na lista a carga que quer inverter.")
            return
        nome = it.data(QtCore.Qt.UserRole)

        def fn():
            est = self.est()
            c = est.doc.getObject(nome)
            if c is None or not hasattr(c, "Reversed"):
                raise RuntimeError("Esta carga não tem sentido para inverter (peso próprio é sempre −Z).")
            c.Reversed = not c.Reversed
            c.Label = _rotulo_carga(c)
            est.put("SeworkResultadoVelho", True, "App::PropertyBool")
        self._transacao("Inverter carga", fn)

    def _tipo_carga(self):
        k = self.cb_carga.currentData()
        self.sp_carga.setVisible(k != "peso")
        self.cb_dir.setVisible(k == "forca")
        self.ck_inv.setVisible(k != "peso")
        self.sp_carga.setSuffix(" N" if k == "forca" else " MPa")
        if k == "pressao" and self.sp_carga.value() > 1000:
            self.sp_carga.setValue(1.0)

    def _aplicar_carga(self):
        est = self.est()
        if est.analise is None:
            self._erro("Escolha a peça na etapa 1 primeiro.")
            return
        k = self.cb_carga.currentData()
        faces = None
        if k != "peso":
            faces, msg = self._faces_selecionadas(est)
            if not faces:
                self._erro(msg)
                return
            if self.sp_carga.value() <= 0:
                self._erro("Informe um valor maior que zero.")
                return
        aresta = None
        if k == "forca" and self.cb_dir.currentData() == "aresta":
            for s in FreeCADGui.Selection.getSelectionEx():
                for nome in s.SubElementNames:
                    if nome.split(".")[-1].startswith("Edge"):
                        aresta = (s.Object, [nome.split(".")[-1]])
                        break
            if aresta is None:
                self._erro("Para a direção por aresta, selecione também uma aresta (Ctrl + clique).")
                return
        val = self.sp_carga.value()
        inv = self.ck_inv.isChecked()

        grupos = None
        if k == "forca" and aresta is None:
            grupos, msg = _grupos_por_normal(est.peca, faces)
            if grupos is None:
                self._erro(msg)
                return

        def fn():
            import ObjectsFem
            if k == "forca" and grupos is not None:
                area_total = sum(g["area"] for g in grupos)
                for g in grupos:
                    c = ObjectsFem.makeConstraintForce(est.doc, "Forca")
                    c.References = [(est.peca, tuple(g["faces"]))]
                    c.Force = "{} N".format(val * g["area"] / area_total)
                    est.analise.addObject(c)
                    _orientar_forca(c, g["normal"], puxar=inv)
                    c.Label = _rotulo_carga(c)
                est.put("SeworkResultadoVelho", True, "App::PropertyBool")
                return
            if k == "forca":
                c = ObjectsFem.makeConstraintForce(est.doc, "Forca")
                c.References = [(est.peca, tuple(faces))]
                c.Force = "{} N".format(val)
                c.Direction = aresta
                c.Reversed = inv
                c.Label = _rotulo_carga(c)
            elif k == "pressao":
                c = ObjectsFem.makeConstraintPressure(est.doc, "Pressao")
                c.References = [(est.peca, tuple(faces))]
                c.Pressure = "{} MPa".format(val)
                c.Reversed = inv
                c.Label = _rotulo_carga(c)
            else:
                if any(_eh(o, "Fem::ConstraintSelfWeight") for o in est.cargas):
                    raise RuntimeError("O peso próprio já está aplicado.")
                c = ObjectsFem.makeConstraintSelfWeight(est.doc, "PesoProprio")
                c.Label = "Peso próprio"
            est.analise.addObject(c)
            est.put("SeworkResultadoVelho", True, "App::PropertyBool")
        self._transacao("Carga", fn)
        FreeCADGui.Selection.clearSelection()

    # ------------------------------------------------------------ 5 malha
    def _montar_malha(self, lay):
        lay.addWidget(_dica("Comece com a malha média. Se a tensão máxima mudar muito ao refinar, "
                            "a malha ainda não está boa o bastante naquela região."))
        self.sl_malha = QtWidgets.QSlider(QtCore.Qt.Horizontal)
        self.sl_malha.setRange(0, len(NIVEIS_MALHA) - 1)
        self.sl_malha.setValue(2)
        self.sl_malha.setTickPosition(QtWidgets.QSlider.TicksBelow)
        self.sl_malha.valueChanged.connect(self._nivel_malha)
        lay.addWidget(self.sl_malha)
        self.lb_nivel = QtWidgets.QLabel("")
        lay.addWidget(self.lb_nivel)
        self.ck_ordem2 = QtWidgets.QCheckBox("Elementos de 2ª ordem (recomendado para tensões)")
        self.ck_ordem2.setChecked(True)
        lay.addWidget(self.ck_ordem2)
        b = QtWidgets.QPushButton("Gerar malha")
        b.setObjectName("SwPrimario")
        b.clicked.connect(self._gerar_malha)
        lay.addWidget(_linha(b, "stretch"))
        self.lb_malha = QtWidgets.QLabel("")
        self.lb_malha.setWordWrap(True)
        lay.addWidget(self.lb_malha)

    def _tamanho(self, est):
        p = est.peca
        if p is None:
            return None
        d = p.Shape.BoundBox.DiagonalLength
        return max(d / NIVEIS_MALHA[self.sl_malha.value()][1], 0.05)

    def _nivel_malha(self):
        est = self.est()
        t = self._tamanho(est)
        nome = NIVEIS_MALHA[self.sl_malha.value()][0]
        self.lb_nivel.setText("{} — elemento de até {}".format(nome, fmt(t, 2, "mm")) if t else nome)

    def _gerar_malha(self):
        est = self.est()
        if est.peca is None:
            self._erro("Escolha a peça na etapa 1 primeiro.")
            return
        t = self._tamanho(est)
        ordem = "2nd" if self.ck_ordem2.isChecked() else "1st"
        QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
        t0 = time.time()
        ok = False
        est.doc.openTransaction("Malha")
        try:
            ok, _msg = gerar_malha(est, t, ordem, self.sl_malha.value())
            est.doc.commitTransaction()
            if ok:
                est.malha.ViewObject.Visibility = True
        except Exception as exc:
            est.doc.abortTransaction()
            FreeCAD.Console.PrintError("Sework FEM: malha: {}\n".format(exc))
        finally:
            QtWidgets.QApplication.restoreOverrideCursor()
        if ok:
            fm = est.malha.FemMesh
            self.lb_malha.setText("Malha gerada em {} s: {} nós, {} elementos.".format(
                fmt(time.time() - t0, 1), fmt(fm.NodeCount, 0), fmt(fm.VolumeCount, 0)))
            self._abrir_proxima()
        else:
            self._erro("O Gmsh não conseguiu gerar a malha.\n\nCausas comuns: geometria com defeito, "
                       "faces muito pequenas ou malha grossa demais para detalhes finos. Tente um nível "
                       "mais fino ou corrija a geometria. Detalhes no painel Relatório.")
        self.atualizar(force=True)

    # ------------------------------------------------------------ 6 cálculo
    def _montar_calculo(self, lay):
        self.box_pend = QtWidgets.QVBoxLayout()
        self.box_pend.setSpacing(4)
        lay.addLayout(self.box_pend)
        self.lb_modos = QtWidgets.QLabel("Número de modos")
        self.sp_modos = QtWidgets.QSpinBox()
        self.sp_modos.setRange(1, 50)
        self.sp_modos.setValue(6)
        lay.addWidget(_linha(self.lb_modos, self.sp_modos, "stretch"))
        self.bt_calc = QtWidgets.QPushButton("Calcular")
        self.bt_calc.setObjectName("SwPrimario")
        self.bt_calc.clicked.connect(self._calcular)
        lay.addWidget(_linha(self.bt_calc, "stretch"))
        self.lb_calc = QtWidgets.QLabel("")
        self.lb_calc.setWordWrap(True)
        lay.addWidget(self.lb_calc)

    def _pendencias(self, r):
        key = tuple(r["_bloqueios"])
        if getattr(self, "_pend_key", None) == key:
            return
        self._pend_key = key
        while self.box_pend.count():
            w = self.box_pend.takeAt(0).widget()
            if w is not None:
                w.deleteLater()
        nomes = {k: t for k, t, _s in ETAPAS}
        if not r["_bloqueios"]:
            lb = QtWidgets.QLabel("Tudo pronto: nenhuma pendência encontrada.")
            lb.setObjectName("SwMsg")
            lb.setProperty("nivel", "ok")
            lb.setWordWrap(True)
            self.box_pend.addWidget(lb)
            return
        for chave, msg in r["_bloqueios"]:
            b = QtWidgets.QPushButton("✕  {} — {}  ›".format(nomes[chave], msg))
            b.setObjectName("SwMsg")
            b.setProperty("nivel", "erro")
            b.setStyleSheet("text-align: left;")
            b.setCursor(QtCore.Qt.PointingHandCursor)
            b.clicked.connect(lambda _=False, c=chave: self.abrir_etapa(c))
            self.box_pend.addWidget(b)

    def _calcular(self):
        est = self.est()
        r = avaliar(est)
        if r["_bloqueios"]:
            self.abrir_etapa(r["_bloqueios"][0][0])
            return
        QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
        self.lb_calc.setText("Calculando… o FreeCAD fica ocupado até o fim.")
        QtWidgets.QApplication.processEvents()
        try:
            ok, erro, dt = calcular(est, self.sp_modos.value())
        except Exception as exc:
            import traceback
            FreeCAD.Console.PrintError("Sework FEM: cálculo:\n" + traceback.format_exc())
            ok, erro, dt = False, "Falha inesperada: {}".format(exc), 0.0
        finally:
            QtWidgets.QApplication.restoreOverrideCursor()
        if not ok:
            self.lb_calc.setText("")
            self._erro(erro)
        else:
            self.lb_calc.setText("Concluído em {} s.".format(fmt(dt, 1)))
            mostrar_campo(est, "tensao" if est.tipo == "static" else "deslocamento")
            self.abrir_etapa("resultado")
        self.atualizar(force=True)

    # ------------------------------------------------------------ 7 resultados
    def _montar_resultado(self, lay):
        self.grid_res = QtWidgets.QGridLayout()
        self.grid_res.setSpacing(6)
        self.c_tensao = _cartao("—", "Tensão máxima de von Mises")
        self.c_desl = _cartao("—", "Deslocamento máximo")
        self.c_fs = _cartao("—", "Fator de segurança (escoamento ÷ tensão máxima)")
        self.grid_res.addWidget(self.c_tensao, 0, 0)
        self.grid_res.addWidget(self.c_desl, 0, 1)
        self.grid_res.addWidget(self.c_fs, 1, 0, 1, 2)
        lay.addLayout(self.grid_res)
        self.lb_interp = QtWidgets.QLabel("")
        self.lb_interp.setObjectName("SwMsg")
        self.lb_interp.setWordWrap(True)
        lay.addWidget(self.lb_interp)
        self.tab_modos = QtWidgets.QTableWidget(0, 2)
        self.tab_modos.setHorizontalHeaderLabels(["Modo", "Frequência (Hz)"])
        self.tab_modos.verticalHeader().setVisible(False)
        self.tab_modos.horizontalHeader().setStretchLastSection(True)
        self.tab_modos.setMaximumHeight(180)
        lay.addWidget(self.tab_modos)
        b1 = QtWidgets.QPushButton("Ver tensão")
        b1.clicked.connect(lambda: mostrar_campo(self.est(), "tensao"))
        b2 = QtWidgets.QPushButton("Ver deslocamento")
        b2.clicked.connect(lambda: mostrar_campo(self.est(), "deslocamento"))
        b3 = QtWidgets.QPushButton("Ver modelo")
        b3.clicked.connect(lambda: mostrar_campo(self.est(), "modelo"))
        self.bts_res = _linha(b1, b2, b3, "stretch")
        lay.addWidget(self.bts_res)
        self.bt_conv = QtWidgets.QPushButton("Verificar convergência da malha")
        self.bt_conv.setToolTip("Recalcula com malhas 30% e 50% menores e compara os resultados")
        self.bt_conv.clicked.connect(self._convergencia)
        lay.addWidget(_linha(self.bt_conv, "stretch"))
        self.tab_conv = QtWidgets.QTableWidget(0, 4)
        self.tab_conv.setHorizontalHeaderLabels(["Elemento", "Nós", "σ máx. (MPa)", "δ máx. (mm)"])
        self.tab_conv.verticalHeader().setVisible(False)
        self.tab_conv.horizontalHeader().setStretchLastSection(True)
        self.tab_conv.setMaximumHeight(130)
        lay.addWidget(self.tab_conv)
        self.lb_conv = QtWidgets.QLabel("")
        self.lb_conv.setObjectName("SwMsg")
        self.lb_conv.setWordWrap(True)
        lay.addWidget(self.lb_conv)
        lay.addWidget(_dica("Picos de tensão em cantos vivos e nos pontos de fixação costumam ser efeito do "
                            "modelo (singularidade). Refine a malha: se o pico continuar subindo, avalie a "
                            "tensão um pouco afastada do canto."))

    def _convergencia(self):
        est = self.est()
        r = QtWidgets.QMessageBox.question(
            self, "Convergência da malha",
            "O modelo será calculado mais duas vezes, com elementos 30% e 50% menores. "
            "Pode levar alguns minutos. Continuar?")
        if r != QtWidgets.QMessageBox.Yes:
            return
        QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
        try:
            def prog(i, n):
                self.lb_conv.setText("Calculando malha {} de {}…".format(i + 1, n))
                self.lb_conv.setVisible(True)
                QtWidgets.QApplication.processEvents()
            convergencia(est, ordem=getattr(est.malha, "ElementOrder", "2nd"), progresso=prog)
            mostrar_campo(est, "tensao")
        except Exception as exc:
            QtWidgets.QApplication.restoreOverrideCursor()
            self._erro(str(exc))
            return
        QtWidgets.QApplication.restoreOverrideCursor()
        self.atualizar(force=True)

    def _mostrar_convergencia(self, est):
        linhas = linhas_convergencia(est) if est.tipo == "static" else []
        self.tab_conv.setVisible(bool(linhas))
        self.lb_conv.setVisible(bool(linhas))
        self.bt_conv.setVisible(est.tipo == "static" and bool(est.resultados))
        if not linhas:
            return
        self.tab_conv.setRowCount(len(linhas))
        for i, l in enumerate(linhas):
            for j, v in enumerate((fmt(l["tamanho"], 2, "mm"), fmt(l["nos"], 0), fmt(l["svm"], 1), fmt(l["u"], 4))):
                self.tab_conv.setItem(i, j, QtWidgets.QTableWidgetItem(v))
        av = avaliar_convergencia(linhas)
        cs, cu, vs, vu = av
        if cs and cu:
            txt, nivel = ("Convergiu: entre as duas malhas mais finas a tensão variou {} e o deslocamento {} "
                          "(limite {} %).".format(fmt(vs, 1, "%"), fmt(vu, 1, "%"), fmt(LIMITE_CONVERGENCIA, 0))), "ok"
        elif cu and not cs:
            txt, nivel = ("O deslocamento convergiu ({}), mas a tensão máxima ainda variou {}. Isso costuma indicar "
                          "concentração em canto vivo ou na fixação (singularidade): avalie a tensão um pouco "
                          "afastada do ponto de pico ou arredonde o canto.".format(fmt(vu, 1, "%"), fmt(vs, 1, "%"))), "aviso"
        else:
            txt, nivel = ("Ainda não convergiu (tensão {}, deslocamento {}). Refine mais a malha antes de usar "
                          "o resultado.".format(fmt(vs, 1, "%"), fmt(vu, 1, "%"))), "erro"
        self.lb_conv.setText(txt)
        self.lb_conv.setProperty("nivel", nivel)
        self.lb_conv.style().unpolish(self.lb_conv)
        self.lb_conv.style().polish(self.lb_conv)

    def _mostrar_resultados(self, est):
        self._mostrar_convergencia(est)
        e = estatisticas(est)
        freq = est.tipo == "frequency"
        for w in (self.c_tensao, self.c_desl, self.c_fs):
            w.setVisible(not freq)
        self.tab_modos.setVisible(freq)
        if not e:
            self.lb_interp.setVisible(False)
            return
        if freq:
            self.tab_modos.setRowCount(len(e["modos"]))
            for i, (modo, f) in enumerate(e["modos"]):
                self.tab_modos.setItem(i, 0, QtWidgets.QTableWidgetItem(str(modo)))
                self.tab_modos.setItem(i, 1, QtWidgets.QTableWidgetItem(fmt(f, 3)))
            self.lb_interp.setVisible(False)
            return
        self.c_tensao.valor.setText(fmt(e.get("svm_max"), 1, "MPa"))
        self.c_desl.valor.setText(fmt(e.get("u_max"), 4, "mm"))
        fs = e.get("fs")
        self.c_fs.valor.setText(fmt(fs, 2) if fs else "—")
        if fs is None:
            txt, nivel = "Sem limite de escoamento no material: fator de segurança não calculado.", "aviso"
        elif fs < 1.0:
            txt, nivel = ("A tensão máxima passa do limite de escoamento ({}): o modelo indica escoamento "
                          "nessa região.".format(fmt(e["Sy"], 0, "MPa"))), "erro"
        else:
            txt, nivel = ("A tensão máxima fica abaixo do limite de escoamento ({}). O fator de segurança "
                          "mínimo aceitável é critério do responsável técnico.".format(fmt(e["Sy"], 0, "MPa"))), "ok"
        self.lb_interp.setText(txt)
        self.lb_interp.setProperty("nivel", nivel)
        self.lb_interp.style().unpolish(self.lb_interp)
        self.lb_interp.style().polish(self.lb_interp)
        self.lb_interp.setVisible(True)

    # ------------------------------------------------------------ 8 relatório
    def _montar_relatorio(self, lay):
        lay.addWidget(_dica("Esses dados vão na capa e no bloco de assinatura do relatório."))
        form = QtWidgets.QFormLayout()
        form.setLabelAlignment(QtCore.Qt.AlignLeft)
        self.campos = {}
        for chave, rot in (("SeworkRelTitulo", "Título"), ("SeworkRelCliente", "Cliente"),
                           ("SeworkRelProjeto", "Equipamento / projeto"), ("SeworkRelResp", "Responsável técnico"),
                           ("SeworkRelCrea", "CREA"), ("SeworkRelArt", "ART nº")):
            e = QtWidgets.QLineEdit()
            e.editingFinished.connect(lambda c=chave, w=e: self._salvar_campo(c, w.text()))
            form.addRow(rot, e)
            self.campos[chave] = e
        lay.addLayout(form)
        lay.addWidget(QtWidgets.QLabel("Observações e conclusão do responsável"))
        self.obs = QtWidgets.QPlainTextEdit()
        self.obs.setMaximumHeight(90)
        lay.addWidget(self.obs)
        b = QtWidgets.QPushButton("Gerar relatório PDF")
        b.setObjectName("SwPrimario")
        b.clicked.connect(self._gerar_pdf)
        lay.addWidget(_linha(b, "stretch"))

    def _salvar_campo(self, chave, valor):
        est = self.est()
        if est.analise is not None and est.get(chave) != valor:
            est.put(chave, valor)

    def _gerar_pdf(self):
        est = self.est()
        if not est.resultados:
            self._erro("Calcule o modelo antes de gerar o relatório.")
            return
        for c, w in self.campos.items():
            self._salvar_campo(c, w.text())
        self._salvar_campo("SeworkRelObs", self.obs.toPlainText())
        base = os.path.splitext(est.doc.FileName)[0] if est.doc.FileName else os.path.join(
            os.path.expanduser("~"), est.doc.Label)
        caminho, _f = QtWidgets.QFileDialog.getSaveFileName(
            self, "Salvar relatório", base + " - relatorio FEM.pdf", "PDF (*.pdf)")
        if not caminho:
            return
        QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
        try:
            import SeworkFEMRelatorio
            SeworkFEMRelatorio.gerar(est, caminho)
            est.put("SeworkRelatorioPDF", caminho)
        except Exception as exc:
            QtWidgets.QApplication.restoreOverrideCursor()
            FreeCAD.Console.PrintError("Sework FEM: relatório: {}\n".format(exc))
            import traceback
            FreeCAD.Console.PrintError(traceback.format_exc())
            self._erro("Não foi possível gerar o relatório: {}".format(exc))
            return
        QtWidgets.QApplication.restoreOverrideCursor()
        QtGui.QDesktopServices.openUrl(QtCore.QUrl.fromLocalFile(caminho))
        self.atualizar(force=True)

    # ------------------------------------------------------------ atualização
    def atualizar(self, force=False):
        if not self.isVisible() and not force:
            return
        est = self.est()
        try:
            r = avaliar(est)
        except Exception as exc:
            FreeCAD.Console.PrintLog("Sework FEM: avaliação: {}\n".format(exc))
            return
        for chave, e in self.etapas.items():
            nivel, resumo, msgs = r[chave]
            e.atualizar(nivel, resumo, msgs)
        # campos que dependem do documento
        doc = FreeCAD.ActiveDocument
        mudou = doc is not self._doc
        self._doc = doc
        self._carregando = True
        i = self.cb_tipo.findData(est.tipo)
        if i >= 0 and self.cb_tipo.currentIndex() != i:
            self.cb_tipo.setCurrentIndex(i)
        self._carregando = False
        self.lb_peca.setText(self._info_peca(est))
        if est.get("SeworkMalhaNivel", None) not in (None, "") and mudou:
            self.sl_malha.setValue(int(est.get("SeworkMalhaNivel")))
        self._nivel_malha()
        self._pendencias(r)
        self.bt_calc.setEnabled(not r["_bloqueios"])
        freq = est.tipo == "frequency"
        self.lb_modos.setVisible(freq)
        self.sp_modos.setVisible(freq)
        if freq and est.solver is not None and hasattr(est.solver, "EigenmodesCount") and mudou:
            self.sp_modos.setValue(max(1, est.solver.EigenmodesCount))
        self._listar_restricoes()
        self._mostrar_resultados(est)
        self.bts_res.setVisible(bool(est.resultados) and not freq)
        if mudou:
            for c, w in self.campos.items():
                w.setText(est.get(c, "") or "")
            self.obs.setPlainText(est.get("SeworkRelObs", "") or "")
            if not self.campos["SeworkRelTitulo"].text():
                self.campos["SeworkRelTitulo"].setPlaceholderText("Análise estrutural por elementos finitos")


def _descr_restricao(o):
    try:
        n = sum(len(ss) for _o, ss in o.References)
    except Exception:
        n = 0
    if _eh(o, "Fem::ConstraintSelfWeight"):
        return "gravidade"
    v = _valor_carga(o)
    faces = "{} face{}".format(n, "s" if n != 1 else "")
    if _eh(o, "Fem::ConstraintForce"):
        return "{} · {}".format(fmt(v, 1, "N"), faces)
    if _eh(o, "Fem::ConstraintPressure"):
        return "{} · {}".format(fmt(v, 3, "MPa"), faces)
    return faces


def _explicar_erro_ccx(saida):
    s = saida or ""
    dicas = []
    if "nonpositive jacobian" in s.lower():
        dicas.append("Há elementos distorcidos na malha (jacobiano não positivo). Gere a malha mais fina ou "
                     "desmarque os elementos de 2ª ordem.")
    if "no material was assigned" in s:
        dicas.append("Algum elemento ficou sem material. Aplique o material à peça inteira.")
    if "singular" in s.lower() or "zero pivot" in s.lower():
        dicas.append("O sistema ficou singular: a peça (ou parte dela) pode se mover livremente. "
                     "Confira as fixações e se todos os sólidos estão unidos.")
    if not dicas:
        dicas.append("O CalculiX terminou com erro. Os detalhes estão no painel Relatório.")
    return "\n\n".join(dicas)


# =========================================================================== painel
_estado = {"dock": None}


def mostrar():
    mw = FreeCADGui.getMainWindow()
    dock = _estado["dock"]
    if dock is None or not _vivo(dock):
        # painel de uma carga anterior do módulo: substitui
        for velho in mw.findChildren(QtWidgets.QDockWidget, "SeworkAssistenteFEM"):
            velho.setParent(None)
            velho.deleteLater()
        dock = QtWidgets.QDockWidget("Assistente de simulação", mw)
        dock.setObjectName("SeworkAssistenteFEM")
        dock.setWidget(Assistente(dock))
        dock.setMinimumWidth(360)
        mw.addDockWidget(QtCore.Qt.RightDockWidgetArea, dock)
        _estado["dock"] = dock
    dock.show()
    dock.raise_()
    dock.widget().atualizar(force=True)
    dock.widget()._abrir_proxima()
    return dock


def _vivo(w):
    try:
        w.objectName()
        return True
    except RuntimeError:
        return False


class CmdAssistenteFEM:
    def GetResources(self):
        return {"Pixmap": ICON, "MenuText": "Assistente de simulação",
                "ToolTip": "Passo a passo da análise por elementos finitos: peça, material, fixações, "
                           "cargas, malha, verificação de pendências, resultados e relatório PDF"}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        try:
            FreeCADGui.activateWorkbench("FemWorkbench")
        except Exception:
            pass
        mostrar()
