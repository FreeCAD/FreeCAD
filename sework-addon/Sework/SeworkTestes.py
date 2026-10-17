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

    def test_qss_completa_gera_sem_erro(self):
        for esc in (False, True):
            self.p.SetBool("TemaEscuro", esc)
            self.assertGreater(len(self.E.qss()), 5000)


class T04Glossario(unittest.TestCase):
    def test_termos(self):
        import SeworkI18n as I
        tr = I.SeworkTranslator(I.EXTRA, "")
        casos = {("CmdPartDesignPad", "Pad"): "Extrusão", ("CmdPartDesignPocket", "Pocket"): "Cavidade",
                 ("CmdPartDesignFillet", "Fillet"): "Arredondamento", ("CmdPartBox", "Cube"): "Caixa",
                 ("CmdTechDrawSectionView", "Section View"): "Vista em corte"}
        for (ctx, src), esperado in casos.items():
            self.assertEqual(tr.translate(ctx, src), esperado)

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


# ======================================================================= execução
RAPIDOS = [T01Modulos, T02Formatacao, T03Tema, T04Glossario, T05Materiais, T06Pendencias]
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
    aprov = total - len(res.failures) - len(res.errors)
    linhas = ["FreeCAD Sework — testes automáticos — %s" % time.strftime("%d/%m/%Y %H:%M"),
              "%d de %d aprovados em %.1f s" % (aprov, total, time.time() - t0), ""]
    if NUMEROS:
        linhas.append("Validação numérica (FEM × analítico):")
        for g, fem, ana, un in NUMEROS:
            linhas.append("  %-42s FEM %10.5g  analítico %10.5g %-3s  diferença %+.2f %%"
                          % (g, fem, ana, un, 100.0 * (fem - ana) / ana))
        linhas.append("")
    for st, nome, dt, det in res.linhas:
        linhas.append("[%s] %s (%.1f s)" % ({"ok": " OK ", "falhou": "FALHOU", "erro": " ERRO "}[st], nome, dt))
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
                "erro": "<span style='color:#B5523F'>✕</span>"}[st]
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
