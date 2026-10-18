# SPDX-License-Identifier: LGPL-2.1-or-later
"""Testes automáticos do FreeCAD Sework.

Rodam dentro do FreeCAD (comando "Testes automáticos" na aba Sework, ou
``import SeworkTestes; SeworkTestes.rodar()`` no console).

- Rápidos: formatação, tema claro/escuro, glossário, materiais, comandos e módulos.
- Verificações do assistente: cada pendência que impediria o cálculo é detectada.
- Validação numérica (lenta): CalculiX comparado com soluções analíticas
  de barra tracionada, viga em balanço e primeira frequência natural.
- Relatório: o PDF é gerado e é um PDF válido.
"""
import math
import os
import tempfile
import time
import traceback
import unittest

import FreeCAD
import FreeCADGui

LOG = os.path.join(FreeCAD.getUserAppDataDir(), "SeworkTestes.log")
NUMEROS = []   # (grandeza, FEM, analítico, unidade) registrados pela validação numérica


# ======================================================================= utilidades
def _doc_teste(nome):
    return FreeCAD.newDocument(nome)


def _fechar(doc):
    try:
        FreeCAD.closeDocument(doc.Name)
    except Exception:
        pass


def _face(obj, cond):
    for i, f in enumerate(obj.Shape.Faces, 1):
        if cond(f):
            return "Face%d" % i
    raise AssertionError("face não encontrada")


def _aresta(obj, cond):
    for i, e in enumerate(obj.Shape.Edges, 1):
        if cond(e):
            return "Edge%d" % i
    raise AssertionError("aresta não encontrada")


def _estudo_viga(doc, L=200.0, b=10.0, h=20.0, tipo="static"):
    """Viga em balanço: comprimento em X, largura b em Y, altura h em Z. Aço S235JR da biblioteca."""
    import Part
    import ObjectsFem
    import Materials
    import SeworkFEM as F
    o = doc.addObject("Part::Feature", "Viga")
    o.Shape = Part.makeBox(L, b, h)
    doc.recompute()
    est = F.Estudo(doc)
    est.analise = None
    est.garantir(o)
    est.put("SeworkTipo", tipo)
    est.solver.AnalysisType = tipo
    m = ObjectsFem.makeMaterialSolid(doc, "Material")
    est.analise.addObject(m)
    d = next(x for x in F.materiais() if x["nome"] in ("S235JR", "Steel-S235JR"))
    mat = Materials.MaterialManager().getMaterial(d["uuid"])
    m.Material = mat.Properties
    fx = ObjectsFem.makeConstraintFixed(doc, "Fixacao")
    fx.References = [(o, _face(o, lambda f: abs(f.CenterOfMass.x) < 1e-6))]
    est.analise.addObject(fx)
    doc.recompute()
    return est, o, F.props_mat(m.Material)


# ======================================================================= rápidos
class T01Modulos(unittest.TestCase):
    def test_importa_modulos(self):
        for nome in ("SeworkGui", "SeworkEstilo", "SeworkRibbon", "SeworkI18n", "SeworkInicio", "SeworkGuia",
                     "SeworkSobre", "SeworkFEM", "SeworkFEMRelatorio"):
            __import__(nome)

    def test_comandos_sework_registrados(self):
        todos = set(FreeCADGui.Command.listAll())
        import SeworkRibbon
        faltando = [c for t in SeworkRibbon.TABS for _p, itens in t["panels"] for c, _s, _l in itens
                    if c.startswith("Sework_") and c not in todos]
        self.assertEqual(faltando, [], "comandos Sework sem registro: %s" % faltando)


class T02Formatacao(unittest.TestCase):
    def test_numero_brasileiro(self):
        from SeworkFEM import fmt
        self.assertEqual(fmt(1234.5, 1), "1.234,5")
        self.assertEqual(fmt(0.0046, 4, "mm"), "0,0046 mm")
        self.assertEqual(fmt(1000000, 0), "1.000.000")
        self.assertEqual(fmt(None), "—")


class T03Tema(unittest.TestCase):
    def setUp(self):
        import SeworkEstilo
        self.E = SeworkEstilo
        self.p = FreeCAD.ParamGet(SeworkEstilo.PARAM_PATH)
        self.antes = self.p.GetBool("TemaEscuro", False)

    def tearDown(self):
        self.p.SetBool("TemaEscuro", self.antes)

    def test_claro_nao_altera(self):
        self.p.SetBool("TemaEscuro", False)
        css = "QLabel { color: #1F2A36; background: #FFFFFF; }"
        self.assertEqual(self.E.tema(css), css)

    def test_escuro_inverte_e_preserva_marca(self):
        self.p.SetBool("TemaEscuro", True)
        out = self.E.tema("QLabel { color: #1F2A36; background: #FFFFFF; } "
                          "#Topo { background: qlineargradient(stop:0 #0A4D8C, stop:1 #002A52); }")
        from PySide import QtGui
        texto = QtGui.QColor(out.split("color: ")[1][:7])
        fundo = QtGui.QColor(out.split("background: ")[1][:7])
        self.assertGreater(texto.lightnessF(), 0.8, "texto deve ficar claro no escuro")
        self.assertLess(fundo.lightnessF(), 0.25, "fundo deve ficar escuro")
        self.assertIn("#0A4D8C", out, "faixa de marca em gradiente não pode mudar")

    def test_texto_do_console_contrasta_com_o_fundo(self):
        """Texto branco gravado com o tema claro (Windows em modo escuro) deve ser corrigido."""
        from PySide import QtGui
        ed = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Editor")
        antes = ed.GetUnsigned("Text", 0)
        try:
            for esc, branco_gravado in ((False, True), (True, False)):
                self.p.SetBool("TemaEscuro", esc)
                ed.SetUnsigned("Text", 0xFFFFFF00 if branco_gravado else 0x1F2A3600)
                self.E.texto_console()
                v = ed.GetUnsigned("Text", 0)
                cor = QtGui.QColor((v >> 24) & 0xFF, (v >> 16) & 0xFF, (v >> 8) & 0xFF)
                if esc:
                    self.assertGreater(cor.lightnessF(), 0.6, "no escuro o texto deve ser claro")
                else:
                    self.assertLess(cor.lightnessF(), 0.4, "no claro o texto deve ser escuro")
        finally:
            if antes:
                ed.SetUnsigned("Text", antes)
            else:
                ed.RemUnsigned("Text")
            self.p.SetBool("TemaEscuro", self.antes)
            self.E.texto_console()

    def test_painel_sobreposto_tem_folha_de_estilo(self):
        """Sem ela, o painel Tasks sobreposto fica branco e cobre a peça na vista 3D (FreeCAD 27)."""
        import SeworkGui
        qss = os.path.join(FreeCAD.getResourceDir(), "Gui", "Stylesheets", "overlay", SeworkGui.OVERLAY_QSS)
        if not os.path.isfile(qss):
            return  # versão sem painel sobreposto: nada a garantir
        SeworkGui.garantir_overlay_qss()
        mw = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/MainWindow")
        self.assertTrue(mw.GetString("OverlayActiveStyleSheet", ""))

    def test_qss_completa_gera_sem_erro(self):
        for esc in (False, True):
            self.p.SetBool("TemaEscuro", esc)
            self.assertGreater(len(self.E.qss()), 5000)


class T03bTrocaDeTema(unittest.TestCase):
    """Regressão: a troca de tema às vezes não pegava porque a nova janela abria antes da antiga gravar."""

    def test_comando_espera_o_processo_atual(self):
        import SeworkGui
        prog, args = SeworkGui.comando_reabrir(1234, r"C:\Program Files\FreeCAD 1.1\bin\FreeCAD.exe")
        txt = " ".join([prog] + list(args))
        self.assertIn("1234", txt)
        if os.name == "nt":
            self.assertIn("Wait-Process", txt)
            self.assertLess(txt.index("Wait-Process"), txt.index("Start-Process"))

    def test_preferencias_gravadas_no_disco(self):
        import SeworkGui
        self.assertTrue(SeworkGui.salvar_parametros())
        cfg = os.path.join(FreeCAD.getUserAppDataDir(), "user.cfg")
        self.assertLess(time.time() - os.path.getmtime(cfg), 10, "user.cfg não foi gravado agora")

    def test_aplicar_tema_e_voltar(self):
        import SeworkGui
        import SeworkEstilo
        antes = SeworkEstilo.escuro()
        try:
            SeworkGui.aplicar_tema(not antes)
            self.assertEqual(SeworkEstilo.escuro(), not antes)
            with open(os.path.join(FreeCAD.getUserAppDataDir(), "user.cfg"), encoding="utf-8") as f:
                txt = f.read()
            self.assertIn('Name="TemaEscuro" Value="%d"' % int(not antes), txt)
        finally:
            SeworkGui.aplicar_tema(antes)


class T04Glossario(unittest.TestCase):
    def test_termos(self):
        import SeworkI18n as I
        tr = I.SeworkTranslator(I.EXTRA, "")
        casos = {("CmdPartDesignPad", "Pad"): "Extrusão", ("CmdPartDesignPocket", "Pocket"): "Cavidade",
                 ("CmdPartDesignFillet", "Fillet"): "Arredondamento", ("CmdPartBox", "Cube"): "Caixa",
                 ("CmdTechDrawSectionView", "Section View"): "Vista em corte"}
        for (ctx, src), esperado in casos.items():
            self.assertEqual(tr.translate(ctx, src), esperado)

    def test_valores_de_lista_do_editor_de_propriedades(self):
        import SeworkI18n as I
        tr = I.SeworkTranslator(I.EXTRA, "")
        c = "App::PropertyEnumeration"
        self.assertEqual(tr.translate(c, "Length"), "Dimensão")
        self.assertEqual(tr.translate(c, "ThroughAll"), "Através de tudo")
        self.assertEqual(tr.translate(c, "One side"), "Um lado")
        self.assertEqual(tr.translate(c, "static"), "estática")
        self.assertIsNone(tr.translate(c, "ValorQueNaoExiste"), "sem tradução deve devolver None (Qt usa o original)")
        # termo ambíguo (estilo de linha × categoria de material) fica sem tradução de propósito
        self.assertIsNone(tr.translate(c, "Solid"))

    def test_painel_usa_os_mesmos_termos(self):
        import SeworkI18n as I
        import SeworkEnums as E
        tr = I.SeworkTranslator(I.EXTRA, "")
        self.assertEqual(tr.translate("PartDesignGui::TaskPadPocketParameters", "Through all"), E.ENUMS["ThroughAll"])

    def test_tradutor_sework_ativo(self):
        """Pega perfil novo sem idioma definido (a tradução Sework não carregava)."""
        from PySide import QtCore
        got = QtCore.QCoreApplication.translate("App::PropertyEnumeration", "ThroughAll")
        self.assertEqual(got, "Através de tudo", "tradução Sework não está instalada (idioma do FreeCAD?)")

    def test_idioma_escolhido_nao_e_sobrescrito(self):
        import SeworkI18n as I
        grp = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/General")
        antes = grp.GetString("Language", "")
        try:
            grp.SetString("Language", "English")
            self.assertFalse(I.idioma_padrao_ptbr())
            self.assertEqual(grp.GetString("Language"), "English")
        finally:
            grp.SetString("Language", antes)

    def test_glossario_nao_vaza_para_outros_contextos(self):
        import SeworkI18n as I
        tr = I.SeworkTranslator(I.EXTRA, "")
        self.assertNotEqual(tr.translate("StdCmdCut", "Cu&t"), "Subtrair")


class T05Materiais(unittest.TestCase):
    def test_biblioteca(self):
        import SeworkFEM as F
        lst = F.materiais()
        self.assertGreater(len(lst), 50)
        d = next((x for x in lst if x["nome"] in ("S235JR", "Steel-S235JR")), None)
        self.assertIsNotNone(d, "S235JR não encontrado na biblioteca")
        p = F.props_mat(d["props"])
        self.assertAlmostEqual(p["E"], 210000.0, delta=1.0)
        self.assertAlmostEqual(p["Sy"], 235.0, delta=0.5)
        self.assertAlmostEqual(p["rho"], 7800.0, delta=1.0)
        self.assertAlmostEqual(p["nu"], 0.3, delta=1e-6)


class T05bMateriaisSework(unittest.TestCase):
    """Biblioteca Sework: mínimos de especificação conferidos (ver tools/gerar_materiais.py)."""
    ESPERADO = {  # nome: (Sy, Su, E)
        "ASTM A36": (250, 400, 203000), "NBR 7007 MR250": (250, 400, 203000),
        "NBR 7007 AR350": (350, 450, 203000), "ASTM A572 Gr 50": (345, 450, 203000),
        "ASTM A283 Gr C": (205, 380, 203000), "ASTM A285 Gr C (SA-285 C)": (205, 380, 203000),
        "ASTM A516 Gr 60 (SA-516 60)": (220, 415, 203000), "ASTM A516 Gr 70 (SA-516 70)": (260, 485, 203000),
        "ASTM A515 Gr 70 (SA-515 70)": (260, 485, 203000), "ASTM A106 Gr B (SA-106 B)": (240, 415, 203000),
        "ASTM A53 Gr B (SA-53 B)": (240, 415, 203000), "ASTM A105 (SA-105)": (250, 485, 203000),
        "ASTM A500 Gr B (tubo redondo)": (290, 400, 203000), "AISI 304 (A240)": (205, 515, 195000),
        "AISI 304L (A240)": (170, 485, 195000), "AISI 316 (A240)": (205, 515, 195000),
        "Alumínio 6061-T6 (B209)": (240, 290, 69000),
    }

    def test_biblioteca_registrada_e_valores(self):
        import SeworkFEM as F
        sew = {d["nome"]: d for d in F.materiais() if d["biblioteca"] == "Sework"}
        if not sew:
            self.skipTest("biblioteca Sework ainda não carregada (reabra o FreeCAD)")
        for nome, (sy, su, e) in self.ESPERADO.items():
            self.assertIn(nome, sew, "material ausente: " + nome)
            p = F.props_mat(sew[nome]["props"])
            self.assertAlmostEqual(p["Sy"], sy, delta=0.5, msg=nome)
            self.assertAlmostEqual(p["Su"], su, delta=0.5, msg=nome)
            self.assertAlmostEqual(p["E"], e, delta=1.0, msg=nome)
            self.assertIsNotNone(p["rho"], nome)
            self.assertIsNotNone(p["nu"], nome)

    def test_sae_sem_escoamento_de_norma(self):
        import SeworkFEM as F
        sew = {d["nome"]: d for d in F.materiais() if d["biblioteca"] == "Sework"}
        if not sew:
            self.skipTest("biblioteca Sework ainda não carregada")
        for nome in ("SAE 1020", "SAE 1045"):
            self.assertIsNone(F.props_mat(sew[nome]["props"])["Sy"], nome)

    def test_sework_aparece_primeiro(self):
        import SeworkFEM as F
        lst = F.materiais()
        if not any(d["biblioteca"] == "Sework" for d in lst):
            self.skipTest("biblioteca Sework ainda não carregada")
        self.assertEqual(lst[0]["biblioteca"], "Sework")


# ======================================================================= verificações do assistente
class T06Pendencias(unittest.TestCase):
    def setUp(self):
        self.doc = _doc_teste("SeworkTesteVerif")

    def tearDown(self):
        _fechar(self.doc)

    def test_estudo_vazio_bloqueia(self):
        import SeworkFEM as F
        r = F.avaliar(F.Estudo(self.doc))
        self.assertTrue(r["_bloqueios"])
        self.assertEqual(r["estudo"][0], F.VAZIO)

    def test_sem_fixacao_bloqueia_estatica(self):
        import ObjectsFem
        import SeworkFEM as F
        est, o, _p = _estudo_viga(self.doc)
        for c in est.fixacoes:
            self.doc.removeObject(c.Name)
        r = F.avaliar(F.Estudo(self.doc))
        self.assertIn("fixacao", [k for k, _m in r["_bloqueios"]])
        del ObjectsFem, o

    def test_sem_fixacao_so_avisa_em_frequencia(self):
        import SeworkFEM as F
        est, _o, _p = _estudo_viga(self.doc, tipo="frequency")
        for c in est.fixacoes:
            self.doc.removeObject(c.Name)
        r = F.avaliar(F.Estudo(self.doc))
        self.assertEqual(r["fixacao"][0], F.AVISO)

    def test_sem_carga_bloqueia_estatica(self):
        import SeworkFEM as F
        _estudo_viga(self.doc)
        r = F.avaliar(F.Estudo(self.doc))
        self.assertIn("carga", [k for k, _m in r["_bloqueios"]])

    def test_carga_zero_bloqueia(self):
        import ObjectsFem
        import SeworkFEM as F
        est, o, _p = _estudo_viga(self.doc)
        c = ObjectsFem.makeConstraintForce(self.doc, "Forca")
        c.References = [(o, _face(o, lambda f: abs(f.CenterOfMass.x - 200) < 1e-6))]
        c.Force = "0 N"
        est.analise.addObject(c)
        self.doc.recompute()
        r = F.avaliar(F.Estudo(self.doc))
        self.assertTrue(any("valor zero" in m for k, m in r["_bloqueios"] if k == "carga"))

    def test_peso_proprio_exige_densidade(self):
        import ObjectsFem
        import SeworkFEM as F
        est, _o, _p = _estudo_viga(self.doc)
        m = est.material
        mat = dict(m.Material)
        mat.pop("Density", None)
        m.Material = mat
        est.analise.addObject(ObjectsFem.makeConstraintSelfWeight(self.doc, "Peso"))
        self.doc.recompute()
        r = F.avaliar(F.Estudo(self.doc))
        self.assertTrue(any("densidade" in mm for k, mm in r["_bloqueios"] if k == "material"))

    def test_referencia_para_face_inexistente(self):
        import SeworkFEM as F
        est, o, _p = _estudo_viga(self.doc)
        est.fixacoes[0].References = [(o, "Face99")]
        r = F.avaliar(F.Estudo(self.doc))
        self.assertTrue(any("não existem" in mm for k, mm in r["_bloqueios"] if k == "fixacao"))

    def test_sem_malha_bloqueia_e_malha_desatualizada(self):
        import ObjectsFem
        import Part
        import SeworkFEM as F
        est, o, _p = _estudo_viga(self.doc, L=100, b=10, h=10)
        c = ObjectsFem.makeConstraintForce(self.doc, "Forca")
        c.References = [(o, _face(o, lambda f: abs(f.CenterOfMass.x - 100) < 1e-6))]
        c.Force = "100 N"
        est.analise.addObject(c)
        self.doc.recompute()
        r = F.avaliar(F.Estudo(self.doc))
        self.assertIn("malha", [k for k, _m in r["_bloqueios"]])
        ok, msg = F.gerar_malha(F.Estudo(self.doc), 10.0, "1st")
        self.assertTrue(ok, msg)
        r = F.avaliar(F.Estudo(self.doc))
        self.assertEqual(r["_bloqueios"], [], r["_bloqueios"])
        o.Shape = Part.makeBox(120, 10, 10)        # a peça muda depois da malha
        self.doc.recompute()
        r = F.avaliar(F.Estudo(self.doc))
        self.assertTrue(any("mudou" in mm for k, mm in r["_bloqueios"] if k == "malha"))


# ======================================================================= validação numérica
class T06bSentidoDaCarga(unittest.TestCase):
    """Força perpendicular empurra a face por padrão e o sentido pode ser invertido depois."""

    def setUp(self):
        self.doc = FreeCAD.newDocument("SeworkTesteSentido")
        self.caixa = self.doc.addObject("Part::Box", "Caixa")
        self.doc.recompute()

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def _forca(self, face, puxar):
        import ObjectsFem
        import SeworkFEM as F
        c = ObjectsFem.makeConstraintForce(self.doc, "Forca")
        c.References = [(self.caixa, (face,))]
        c.Force = "100 N"
        self.doc.recompute()
        n = F._normal_externa(self.caixa, face)
        F._orientar_forca(c, n, puxar=puxar)
        return c, n

    def test_empurra_por_padrao_em_qualquer_face(self):
        for face in ("Face1", "Face2", "Face3", "Face4", "Face5", "Face6"):
            c, n = self._forca(face, puxar=False)
            self.assertLess(c.DirectionVector.dot(n), -0.999, face)

    def test_puxar_e_inverter_depois(self):
        c, n = self._forca("Face1", puxar=True)
        self.assertGreater(c.DirectionVector.dot(n), 0.999)
        c.Reversed = not c.Reversed  # o que o botão "Inverter sentido da selecionada" faz
        self.doc.recompute()
        self.assertLess(c.DirectionVector.dot(n), -0.999)

    def test_faces_opostas_viram_cargas_separadas(self):
        import SeworkFEM as F
        grupos, _msg = F._grupos_por_normal(self.caixa, ["Face1", "Face2", "Face6"])
        self.assertEqual(len(grupos), 3)
        grupos, _msg = F._grupos_por_normal(self.caixa, ["Face6"])
        self.assertEqual(len(grupos), 1)
        self.assertAlmostEqual(grupos[0]["area"], 100.0, places=6)

    def test_face_curva_pede_pressao(self):
        import SeworkFEM as F
        cil = self.doc.addObject("Part::Cylinder", "Cil")
        self.doc.recompute()
        grupos, msg = F._grupos_por_normal(cil, ["Face1"])
        self.assertIsNone(grupos)
        self.assertIn("Pressão", msg)


class T07Validacao(unittest.TestCase):
    """Compara o CalculiX com soluções analíticas clássicas da resistência dos materiais."""

    def setUp(self):
        self.doc = _doc_teste("SeworkTesteValid")

    def tearDown(self):
        _fechar(self.doc)

    def test_barra_tracionada(self):
        # δ = F·L/(E·A)
        import ObjectsFem
        import SeworkFEM as F
        L, a, Fn = 100.0, 10.0, 1000.0
        est, o, p = _estudo_viga(self.doc, L=L, b=a, h=a)
        c = ObjectsFem.makeConstraintForce(self.doc, "Forca")
        c.References = [(o, _face(o, lambda f: abs(f.CenterOfMass.x - L) < 1e-6))]
        c.Force = "%s N" % Fn
        est.analise.addObject(c)
        self.doc.recompute()
        ok, msg = F.gerar_malha(F.Estudo(self.doc), 2.5, "2nd")
        self.assertTrue(ok, msg)
        # regressão: com a pasta atual = bin do FreeCAD o ccx era achado por caminho relativo
        # e falhava com WinError 2 ao trocar de pasta
        antes = os.getcwd()
        try:
            os.chdir(os.path.join(FreeCAD.getHomePath(), "bin"))
            ok, erro, _t = F.calcular(F.Estudo(self.doc))
        finally:
            os.chdir(antes)
        self.assertTrue(ok, erro)
        e = F.estatisticas(F.Estudo(self.doc))
        delta = Fn * L / (p["E"] * a * a)
        NUMEROS.append(("Barra tracionada — alongamento", e["u_max"], delta, "mm"))
        self.assertAlmostEqual(e["u_max"], delta, delta=0.03 * delta,
                               msg="δ FEM %.5f × analítico %.5f mm" % (e["u_max"], delta))

    def test_viga_em_balanco(self):
        # δ = F·L³/(3·E·I) na ponta; σ = M·c/I no meio do vão (fibra superior)
        import ObjectsFem
        import SeworkFEM as F
        L, b, h, Fn = 200.0, 10.0, 20.0, 100.0
        est, o, p = _estudo_viga(self.doc, L=L, b=b, h=h)
        c = ObjectsFem.makeConstraintForce(self.doc, "Forca")
        c.References = [(o, _face(o, lambda f: abs(f.CenterOfMass.x - L) < 1e-6))]
        c.Force = "%s N" % Fn
        c.Direction = (o, [_aresta(o, lambda e: abs(e.BoundBox.XMin - L) < 1e-6 and e.BoundBox.XLength < 1e-6
                                   and e.BoundBox.ZLength > 1)])
        est.analise.addObject(c)
        self.doc.recompute()
        ok, msg = F.gerar_malha(F.Estudo(self.doc), 3.0, "2nd")
        self.assertTrue(ok, msg)
        ok, erro, _t = F.calcular(F.Estudo(self.doc))
        self.assertTrue(ok, erro)
        est = F.Estudo(self.doc)
        e = F.estatisticas(est)
        I = b * h ** 3 / 12.0
        delta = Fn * L ** 3 / (3.0 * p["E"] * I)
        NUMEROS.append(("Viga em balanço — flecha na ponta", e["u_max"], delta, "mm"))
        self.assertAlmostEqual(e["u_max"], delta, delta=0.05 * delta,
                               msg="δ FEM %.4f × analítico %.4f mm" % (e["u_max"], delta))
        # tensão de flexão no meio do vão, fibra superior/inferior, longe do engaste
        res = est.resultados[0]
        malha = res.Mesh.FemMesh
        alvo = []
        for nid, svm in zip(res.NodeNumbers, res.vonMises):
            v = malha.getNodeById(nid)
            if abs(v.x - L / 2) < 3.0 and (abs(v.z - h) < 1e-6 or abs(v.z) < 1e-6) and 1.0 < v.y < b - 1.0:
                alvo.append(svm)
        self.assertTrue(alvo, "nenhum nó no meio do vão")
        sigma = Fn * (L / 2) * (h / 2) / I
        media = sum(alvo) / len(alvo)
        NUMEROS.append(("Viga em balanço — tensão no meio do vão", media, sigma, "MPa"))
        self.assertAlmostEqual(media, sigma, delta=0.07 * sigma,
                               msg="σ FEM %.2f × analítico %.2f MPa" % (media, sigma))

    def test_convergencia_da_malha(self):
        # Barra tracionada: o deslocamento tem de convergir (variação pequena entre malhas)
        import ObjectsFem
        import SeworkFEM as F
        L, a, Fn = 100.0, 10.0, 1000.0
        est, o, _p = _estudo_viga(self.doc, L=L, b=a, h=a)
        c = ObjectsFem.makeConstraintForce(self.doc, "Forca")
        c.References = [(o, _face(o, lambda f: abs(f.CenterOfMass.x - L) < 1e-6))]
        c.Force = "%s N" % Fn
        est.analise.addObject(c)
        self.doc.recompute()
        self.assertTrue(F.gerar_malha(F.Estudo(self.doc), 6.0, "2nd")[0])
        self.assertTrue(F.calcular(F.Estudo(self.doc))[0])
        linhas = F.convergencia(F.Estudo(self.doc))
        self.assertEqual(len(linhas), 3)
        self.assertGreater(linhas[-1]["nos"], linhas[0]["nos"], "a malha tem de ficar mais fina")
        cs, cu, vs, vu = F.avaliar_convergencia(linhas)
        self.assertTrue(cu, "deslocamento não convergiu: %.2f %%" % vu)
        self.assertEqual(len(F.linhas_convergencia(F.Estudo(self.doc))), 3, "resultado não foi gravado")

    def test_primeira_frequencia_natural(self):
        # f1 = (1,8751²/2π)·√(E·I/(ρ·A·L⁴)), flexão em torno do eixo de menor inércia
        import SeworkFEM as F
        L, b, h = 200.0, 10.0, 20.0
        est, _o, p = _estudo_viga(self.doc, L=L, b=b, h=h, tipo="frequency")
        ok, msg = F.gerar_malha(F.Estudo(self.doc), 3.0, "2nd")
        self.assertTrue(ok, msg)
        ok, erro, _t = F.calcular(F.Estudo(self.doc), modos=4)
        self.assertTrue(ok, erro)
        e = F.estatisticas(F.Estudo(self.doc))
        f_fem = min(f for _m, f in e["modos"] if f > 1.0)
        E = p["E"] * 1e6
        rho = p["rho"]
        I = (h * b ** 3 / 12.0) * 1e-12
        A = b * h * 1e-6
        Lm = L / 1000.0
        f1 = (1.8751 ** 2 / (2 * math.pi)) * math.sqrt(E * I / (rho * A * Lm ** 4))
        NUMEROS.append(("Viga em balanço — 1ª frequência natural", f_fem, f1, "Hz"))
        self.assertAlmostEqual(f_fem, f1, delta=0.03 * f1, msg="f1 FEM %.1f × analítico %.1f Hz" % (f_fem, f1))


# ======================================================================= relatório
class T08Relatorio(unittest.TestCase):
    def setUp(self):
        self.doc = _doc_teste("SeworkTesteRel")

    def tearDown(self):
        _fechar(self.doc)

    def test_pdf_valido(self):
        import ObjectsFem
        import SeworkFEM as F
        import SeworkFEMRelatorio as R
        est, o, _p = _estudo_viga(self.doc, L=100, b=10, h=10)
        c = ObjectsFem.makeConstraintPressure(self.doc, "Pressao")
        c.References = [(o, _face(o, lambda f: abs(f.CenterOfMass.x - 100) < 1e-6))]
        c.Pressure = "10 MPa"
        est.analise.addObject(c)
        self.doc.recompute()
        self.assertTrue(F.gerar_malha(F.Estudo(self.doc), 5.0, "1st")[0])
        ok, erro, _t = F.calcular(F.Estudo(self.doc))
        self.assertTrue(ok, erro)
        caminho = os.path.join(tempfile.mkdtemp(prefix="sework_teste_"), "relatorio.pdf")
        R.gerar(F.Estudo(self.doc), caminho)
        self.assertTrue(os.path.isfile(caminho))
        with open(caminho, "rb") as f:
            cab = f.read(5)
        self.assertEqual(cab, b"%PDF-")
        self.assertGreater(os.path.getsize(caminho), 20000)


# ======================================================================= interface
class T09Interface(unittest.TestCase):
    def _faixa(self):
        import SeworkRibbon
        rb = SeworkRibbon._state.get("ribbon")
        if rb is None:
            self.skipTest("faixa Sework não instalada nesta sessão")
        return rb

    def test_novo_documento_da_tela_inicial_mostra_os_planos(self):
        """A tela inicial deve abrir a vista como o Arquivo → Novo (planos da origem à vista)."""
        import re
        import SeworkInicio
        antes = set(FreeCAD.listDocuments())
        SeworkInicio.novo_documento()
        novos = [n for n in FreeCAD.listDocuments() if n not in antes]
        self.assertEqual(len(novos), 1)
        try:
            cam = FreeCADGui.getDocument(novos[0]).ActiveView.getCamera()
            if "PerspectiveCamera" in cam:
                return  # usuário escolheu câmera em perspectiva: escala não se aplica
            m = re.search(r"\bheight\s+([0-9.eE+-]+)", cam)
            self.assertIsNotNone(m, "câmera ortográfica esperada")
            self.assertGreater(float(m.group(1)), 50, "vista deve mostrar a região dos planos (8 a 62 mm)")
        finally:
            FreeCAD.closeDocument(novos[0])

    def test_data_da_folha_em_dd_mm_aaaa(self):
        """A folha Sework criada no Desenho técnico recebe a data no formato brasileiro."""
        import SeworkGui

        class Folha:
            Template = "C:/x/Sework_A3_Paisagem.svg"
            EditableTexts = {"data": "18 Oct 2026", "titulo": "Peça"}

        f = Folha()
        SeworkGui._fix_folha_date(f)
        self.assertRegex(f.EditableTexts["data"], r"^\d{2}/\d{2}/\d{4}$")
        self.assertEqual(f.EditableTexts["titulo"], "Peça")

        class Outra:
            Template = "C:/x/A4_Landscape.svg"
            EditableTexts = {"data": "18 Oct 2026"}

        o = Outra()
        SeworkGui._fix_folha_date(o)
        self.assertEqual(o.EditableTexts["data"], "18 Oct 2026", "folha que não é Sework não deve mudar")

    def test_modulos_sem_nomes_indefinidos(self):
        """Pega função apagada que ainda é chamada (o NameError só apareceria na hora do uso)."""
        import ast
        import builtins
        pasta = os.path.dirname(os.path.abspath(__file__))
        for nome in sorted(os.listdir(pasta)):
            if not (nome.startswith("Sework") and nome.endswith(".py")):
                continue
            with open(os.path.join(pasta, nome), encoding="utf-8") as fh:
                arv = ast.parse(fh.read())
            definidos = {"__file__", "__name__"}
            for n in ast.walk(arv):
                if isinstance(n, (ast.FunctionDef, ast.ClassDef)):
                    definidos.add(n.name)
                elif isinstance(n, ast.Name) and isinstance(n.ctx, ast.Store):
                    definidos.add(n.id)
                elif isinstance(n, (ast.Import, ast.ImportFrom)):
                    definidos.update((a.asname or a.name).split(".")[0] for a in n.names)
                elif isinstance(n, ast.arg):
                    definidos.add(n.arg)
                elif isinstance(n, ast.ExceptHandler) and n.name:
                    definidos.add(n.name)
            usados = {n.id for n in ast.walk(arv) if isinstance(n, ast.Name) and isinstance(n.ctx, ast.Load)}
            faltam = sorted(u for u in usados if u not in definidos and not hasattr(builtins, u))
            self.assertEqual(faltam, [], nome)

    def test_planos_da_origem_no_azul_sework(self):
        from PySide import QtGui, QtWidgets
        import SeworkGui
        doc = FreeCAD.newDocument("SeworkTesteOrigem")
        try:
            body = doc.addObject("PartDesign::Body", "Body")
            t0 = time.time()
            while time.time() - t0 < 1.5:
                QtWidgets.QApplication.processEvents()
                time.sleep(0.05)
            esperado = QtGui.QColor(SeworkGui.cor_origem())
            planos = [f for f in body.Origin.OriginFeatures if f.Role.endswith("_Plane")]
            self.assertEqual(len(planos), 3)
            from pivy import coin
            for p in planos:
                mats = SeworkGui._materiais_coin(p.ViewObject.RootNode, coin, [])
                self.assertTrue(mats, p.Role)
                r, g, b = mats[0].diffuseColor[0].getValue()
                self.assertAlmostEqual(r, esperado.redF(), delta=0.02, msg=p.Role)
                self.assertAlmostEqual(g, esperado.greenF(), delta=0.02, msg=p.Role)
                self.assertAlmostEqual(b, esperado.blueF(), delta=0.02, msg=p.Role)
            hl = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").GetUnsigned("HighlightColor", 0)
            self.assertEqual(hl >> 8, 0xF7BE00, "destaque ao passar o mouse deve ser o amarelo Sework")
        finally:
            FreeCAD.closeDocument(doc.Name)

    def test_seletor_de_bancadas_apos_a_marca(self):
        from PySide import QtWidgets
        rb = self._faixa()
        linha = rb.findChild(QtWidgets.QWidget, "SeworkTabRow")
        lay = linha.layout()
        nomes = [lay.itemAt(i).widget().objectName() for i in range(lay.count()) if lay.itemAt(i).widget()]
        self.assertEqual(nomes[:2], ["SeworkBrand", "SeworkWbBtn"], nomes)

    def test_menu_de_bancadas_lista_bancadas(self):
        from PySide import QtWidgets
        rb = self._faixa()
        m = QtWidgets.QMenu()
        rb._fill_wb_menu(m)
        textos = [a.text() for a in m.actions() if a.text()]
        self.assertGreater(len(textos), 5)
        self.assertTrue(any("Simulação" in t or "FEM" in t for t in textos), textos)

    def test_aplicar_material_sework_pelo_assistente(self):
        """Regressão: aplicar um material Sework pelo painel mantém o limite de escoamento."""
        import Part
        import SeworkFEM as F
        doc = _doc_teste("SeworkTesteAplicar")
        try:
            o = doc.addObject("Part::Feature", "Bloco")
            o.Shape = Part.makeBox(50, 20, 10)
            doc.recompute()
            F.Estudo(doc).garantir(o)
            a = F.Assistente()
            erros = []
            a._erro = erros.append
            a._carregar_materiais("A516")
            d = next(x for x in F.materiais() if x["nome"] == "ASTM A516 Gr 70 (SA-516 70)")
            a.cb_mat.setCurrentIndex(a.cb_mat.findData(d["uuid"]))
            a._aplicar_mat()
            self.assertEqual(erros, [])
            est = F.Estudo(doc)
            p = F.props_mat(est.material.Material)
            self.assertAlmostEqual(p["Sy"], 260.0, delta=0.5)
            self.assertEqual(F.avaliar(est)["material"][0], F.OK)
            a.timer.stop()
            a.deleteLater()
        finally:
            _fechar(doc)

    def test_altura_da_faixa(self):
        rb = self._faixa()
        self.assertGreaterEqual(rb.content.height(), 126)


# ======================================================================= tutorial
class T10Tutorial(unittest.TestCase):
    """Os passos do tutorial são detectados conforme a peça é construída."""

    def test_passos_detectados_em_ordem(self):
        import Part
        import Sketcher
        import SeworkTutorial as T
        doc = _doc_teste("SeworkTesteTutorial")
        try:
            FreeCAD.setActiveDocument(doc.Name)
            self.assertEqual(T.passo_atual(T.estado()), 1, "documento criado → próximo é o corpo")
            body = doc.addObject("PartDesign::Body", "Body")
            xy = [f for f in body.Origin.OriginFeatures if getattr(f, "Role", "") == "XY_Plane"][0]
            sk = body.newObject("Sketcher::SketchObject", "Sketch")
            sk.AttachmentSupport = [(xy, "")]
            sk.MapMode = "FlatFace"
            doc.recompute()
            self.assertEqual(T.passo_atual(T.estado()), 3, "esboço criado → próximo é o retângulo")
            p = [FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(60, 0, 0), FreeCAD.Vector(60, 40, 0), FreeCAD.Vector(0, 40, 0)]
            for i in range(4):
                sk.addGeometry(Part.LineSegment(p[i], p[(i + 1) % 4]))
            for i in range(4):
                sk.addConstraint(Sketcher.Constraint("Coincident", i, 2, (i + 1) % 4, 1))
            doc.recompute()
            self.assertEqual(T.passo_atual(T.estado()), 4, "retângulo → próximo são as cotas")
            sk.addConstraint(Sketcher.Constraint("DistanceX", 0, 1, 0, 2, 60.0))
            sk.addConstraint(Sketcher.Constraint("DistanceY", 1, 1, 1, 2, 40.0))
            doc.recompute()
            pad = body.newObject("PartDesign::Pad", "Pad")
            pad.Profile = sk
            pad.Length = 10
            doc.recompute()
            self.assertEqual(T.passo_atual(T.estado()), 7, "extrusão feita → próximo é o furo")
            sk2 = body.newObject("Sketcher::SketchObject", "Sketch001")
            sk2.AttachmentSupport = [(xy, "")]
            sk2.MapMode = "FlatFace"
            sk2.addGeometry(Part.Circle(FreeCAD.Vector(30, 20, 0), FreeCAD.Vector(0, 0, 1), 6))
            doc.recompute()
            poc = body.newObject("PartDesign::Pocket", "Pocket")
            poc.Profile = sk2
            poc.Type = "ThroughAll"
            poc.Midplane = True
            doc.recompute()
            self.assertTrue(T.estado()[7], "furo não detectado")
            arestas = [i + 1 for i, e in enumerate(poc.Shape.Edges)
                       if e.BoundBox.ZLength > 9 and e.BoundBox.XLength < 1e-6 and e.BoundBox.YLength < 1e-6]
            fil = body.newObject("PartDesign::Fillet", "Fillet")
            fil.Base = (poc, ["Edge%d" % i for i in arestas])
            fil.Radius = 5
            doc.recompute()
            feitos = T.estado()
            self.assertEqual(feitos[:9], [True] * 9, feitos)
            self.assertFalse(feitos[9], "documento não salvo ainda")
        finally:
            _fechar(doc)

    def test_mostrar_onde_encontra_os_botoes(self):
        import SeworkRibbon
        import SeworkTutorial as T
        if SeworkRibbon._state.get("ribbon") is None:
            self.skipTest("faixa Sework não instalada nesta sessão")
        for i in (0, 9):     # Novo e Salvar ficam na barra rápida
            _t, _x, aba, cmds, _k = T.PASSOS[i]
            self.assertTrue(T.destacar(aba, cmds), "botão do passo %d não encontrado" % (i + 1))

    def test_painel_abre(self):
        import SeworkTutorial as T
        w = T.Tutorial()
        self.assertEqual(len(w.itens), len(T.PASSOS))
        w.timer.stop()
        w.deleteLater()


# ======================================================================= execução
RAPIDOS = [T01Modulos, T02Formatacao, T03Tema, T03bTrocaDeTema, T04Glossario, T05Materiais, T05bMateriaisSework, T06Pendencias, T06bSentidoDaCarga,
           T09Interface, T10Tutorial]
COMPLETOS = RAPIDOS + [T07Validacao, T08Relatorio]


class _Resultado(unittest.TextTestResult):
    def __init__(self, *a, **k):
        super().__init__(*a, **k)
        self.linhas = []
        self._t0 = 0.0

    def startTest(self, test):
        super().startTest(test)
        self._t0 = time.time()
        try:
            FreeCADGui.updateGui()
        except Exception:
            pass

    def _reg(self, test, status, det=""):
        self.linhas.append((status, test.id().split(".", 1)[-1], time.time() - self._t0, det))

    def addSuccess(self, test):
        super().addSuccess(test)
        self._reg(test, "ok")

    def addSkip(self, test, reason):
        super().addSkip(test, reason)
        self._reg(test, "pulado", reason)

    def addFailure(self, test, err):
        super().addFailure(test, err)
        self._reg(test, "falhou", str(err[1]))

    def addError(self, test, err):
        super().addError(test, err)
        self._reg(test, "erro", "".join(traceback.format_exception(*err))[-1500:])


def rodar(completo=True, mostrar=True):
    """Roda os testes e devolve (aprovados, total). Grava o log em SeworkTestes.log."""
    aprov, total, linhas = _executar(completo)
    if mostrar and FreeCAD.GuiUp:
        _dialogo(aprov, total, linhas)
    return aprov, total


def _executar(completo):
    del NUMEROS[:]
    suite = unittest.TestSuite()
    for cls in (COMPLETOS if completo else RAPIDOS):
        suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(cls))
    import io
    buf = io.StringIO()
    runner = unittest.TextTestRunner(stream=buf, resultclass=_Resultado, verbosity=2)
    ativo = FreeCAD.ActiveDocument
    t0 = time.time()
    res = runner.run(suite)
    total = res.testsRun
    aprov = total - len(res.failures) - len(res.errors) - len(res.skipped)
    linhas = ["FreeCAD Sework — testes automáticos — %s" % time.strftime("%d/%m/%Y %H:%M"),
              "%d de %d aprovados em %.1f s" % (aprov, total, time.time() - t0), ""]
    if NUMEROS:
        linhas.append("Validação numérica (FEM × analítico):")
        for g, fem, ana, un in NUMEROS:
            linhas.append("  %-42s FEM %10.5g  analítico %10.5g %-3s  diferença %+.2f %%"
                          % (g, fem, ana, un, 100.0 * (fem - ana) / ana))
        linhas.append("")
    for st, nome, dt, det in res.linhas:
        linhas.append("[%s] %s (%.1f s)" % ({"ok": " OK ", "falhou": "FALHOU", "erro": " ERRO ", "pulado": "PULADO"}[st], nome, dt))
        if det:
            linhas.append("      " + det.replace("\n", "\n      "))
    with open(LOG, "w", encoding="utf-8") as f:
        f.write("\n".join(linhas))
    FreeCAD.Console.PrintMessage("\n".join(linhas) + "\n")
    if ativo is not None:
        try:
            FreeCAD.setActiveDocument(ativo.Name)
        except Exception:
            pass
    return aprov, total, res.linhas


def _dialogo(aprov, total, linhas):
    from PySide import QtWidgets
    import SeworkEstilo
    d = QtWidgets.QDialog(FreeCADGui.getMainWindow())
    d.setWindowTitle("Testes automáticos")
    d.resize(720, 560)
    v = QtWidgets.QVBoxLayout(d)
    cor = "#2E8B6A" if aprov == total else "#B5523F"
    topo = QtWidgets.QLabel("<span style='font-size:16pt; font-weight:700; color:%s'>%d de %d aprovados</span>"
                            % (cor, aprov, total))
    v.addWidget(topo)
    lista = QtWidgets.QTextBrowser()
    html = []
    for st, nome, dt, det in linhas:
        simb = {"ok": "<span style='color:#2E8B6A'>✓</span>", "falhou": "<span style='color:#B5523F'>✕</span>",
                "erro": "<span style='color:#B5523F'>✕</span>", "pulado": "<span style='color:#E0A100'>–</span>"}[st]
        html.append("<p style='margin:2px 0'>%s <b>%s</b> <span style='color:#5B6876'>(%.1f s)</span>%s</p>" % (
            simb, nome.replace("_", " "), dt,
            ("<br><span style='color:#7A2E20; font-size:8pt'>%s</span>" % det.replace("\n", "<br>")) if det else ""))
    lista.setHtml(SeworkEstilo.tema("".join(html)))
    v.addWidget(lista, 1)
    rod = QtWidgets.QLabel("Log completo: " + LOG)
    rod.setWordWrap(True)
    v.addWidget(rod)
    b = QtWidgets.QDialogButtonBox(QtWidgets.QDialogButtonBox.Close)
    b.rejected.connect(d.reject)
    v.addWidget(b)
    d.exec()


class CmdTestes:
    def GetResources(self):
        import SeworkFEM
        return {"Pixmap": os.path.join(os.path.dirname(SeworkFEM.ICON), "testes.svg"),
                "MenuText": "Testes automáticos",
                "ToolTip": "Confere o FreeCAD Sework: tema, traduções, materiais, verificações do assistente "
                           "e validação do CalculiX contra soluções analíticas (leva cerca de 1 minuto)"}

    def IsActive(self):
        return True

    def Activated(self):
        from PySide import QtCore, QtWidgets
        QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
        try:
            aprov, total, linhas = _executar(True)
        finally:
            QtWidgets.QApplication.restoreOverrideCursor()
        _dialogo(aprov, total, linhas)
