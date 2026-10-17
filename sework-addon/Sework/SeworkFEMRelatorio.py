# SPDX-License-Identifier: LGPL-2.1-or-later
"""Relatório PDF da simulação no padrão visual Sework.

Poppins, azul #003A70, amarelo #F7BE00, escala 34/24/18/16/14 px. Capa desenhada à mão,
corpo em QTextDocument paginado com cabeçalho e rodapé em todas as páginas.
"""
import os
import tempfile
import time

import FreeCAD
import FreeCADGui
from PySide import QtCore, QtGui

import SeworkFEM as F

HERE = os.path.dirname(os.path.abspath(__file__))
LOGO = os.path.join(HERE, "Resources", "icons", "sework.png")
NAVY, YEL, INK, INK2, LINHA, FUNDO = "#003A70", "#F7BE00", "#1F2A36", "#5B6876", "#DCE4EC", "#F5F8FC"
DPI = 300
MARGEM_MM = 18.0

CSS = """
body { font-family: 'Poppins'; font-size: 14px; color: %(INK)s; }
h1 { font-size: 24px; color: %(NAVY)s; font-weight: 700; margin-top: 18px; margin-bottom: 6px; }
h2 { font-size: 18px; color: %(NAVY)s; font-weight: 600; margin-top: 12px; margin-bottom: 4px; }
p { margin-top: 2px; margin-bottom: 6px; line-height: 140%%; }
td, th { font-size: 14px; padding: 5px 8px; }
th { background-color: %(NAVY)s; color: #FFFFFF; font-weight: 600; text-align: left; }
.rot { color: %(INK2)s; }
.destaque { font-size: 16px; font-weight: 700; color: %(NAVY)s; }
.nota { font-size: 12px; color: %(INK2)s; }
.legenda { font-size: 12px; color: %(INK2)s; }
""" % dict(INK=INK, NAVY=NAVY, INK2=INK2)


# ------------------------------------------------------------------ imagens da vista 3D
def _vista():
    gd = FreeCADGui.ActiveDocument
    v = gd.ActiveView if gd else None
    if v is None or not hasattr(v, "saveImage"):
        for mv in gd.mdiViewsOfType("Gui::View3DInventor") if gd else []:
            return mv
    return v


def _captura(est, modo, pasta):
    v = _vista()
    if v is None:
        return None
    caminho = os.path.join(pasta, modo + ".png")
    peca = est.peca
    malha = est.malha
    try:
        if modo == "modelo":
            F.mostrar_campo(est, "modelo")
            if malha is not None:
                malha.ViewObject.Visibility = False
        elif modo == "malha":
            F.mostrar_campo(est, "modelo")
            if malha is not None:
                malha.ViewObject.Visibility = True
                if peca is not None:
                    peca.ViewObject.Visibility = False
            for c in est.fixacoes + est.cargas:
                c.ViewObject.Visibility = False
        else:
            F.mostrar_campo(est, modo)
        FreeCADGui.updateGui()
        v.viewIsometric()
        v.fitAll()
        FreeCADGui.updateGui()
        v.saveImage(caminho, 1800, 1150, "White")
        return caminho if os.path.isfile(caminho) else None
    except Exception as exc:
        FreeCAD.Console.PrintWarning("Sework: captura '{}' falhou: {}\n".format(modo, exc))
        return None


# ------------------------------------------------------------------ html
def _tab(linhas, cab=None, larguras=None):
    h = "<table width='100%' cellspacing='0' cellpadding='0' style='border-collapse:collapse; margin-bottom:8px'>"
    if cab:
        h += "<tr>" + "".join("<th%s>%s</th>" % (
            " width='%s'" % larguras[i] if larguras else "", c) for i, c in enumerate(cab)) + "</tr>"
    for i, ln in enumerate(linhas):
        bg = FUNDO if i % 2 == 0 else "#FFFFFF"
        h += "<tr>" + "".join("<td style='background-color:%s; border-bottom:1px solid %s'%s>%s</td>" % (
            bg, LINHA, " width='%s'" % larguras[j] if (larguras and not cab) else "", c)
            for j, c in enumerate(ln)) + "</tr>"
    return h + "</table>"


def _par(rotulo, valor):
    return ["<span class='rot'>%s</span>" % rotulo, "<b>%s</b>" % valor]


def _img(doc, nome, caminho, legenda, largura):
    """Imagem e legenda na mesma célula de tabela, para não se separarem na quebra de página."""
    if not caminho:
        return "<p class='nota'>(imagem indisponível)</p>"
    im = QtGui.QImage(caminho)
    url = QtCore.QUrl("img://" + nome)
    doc.addResource(QtGui.QTextDocument.ImageResource, url, im)
    alt = int(largura * im.height() / max(im.width(), 1))
    return ("<table width='100%%' cellspacing='0' cellpadding='0' style='margin-top:6px; margin-bottom:6px'>"
            "<tr><td align='center'><img src='img://%s' width='%d' height='%d'><br>"
            "<span class='legenda'>%s</span></td></tr></table>") % (nome, largura, alt, legenda)


def _faces(o):
    try:
        return ", ".join(s for _ob, ss in o.References for s in ss) or "—"
    except Exception:
        return "—"


def _corpo(est, doc, imgs, largura):
    tipo = est.tipo
    peca = est.peca
    m = est.material
    pm = F.props_mat(m.Material) if m is not None else {}
    e = F.estatisticas(est) or {}
    malha = est.malha
    sh = peca.Shape
    bb = sh.BoundBox
    massa = sh.Volume * 1e-9 * pm["rho"] if pm.get("rho") else None
    h = []

    h.append("<h1>1. Objetivo</h1>")
    if tipo == "static":
        h.append("<p>Avaliar as tensões e os deslocamentos de <b>%s</b> sob as cargas e fixações descritas neste "
                 "relatório, por análise estática linear pelo método dos elementos finitos.</p>" % peca.Label)
    else:
        h.append("<p>Determinar as frequências naturais e os modos de vibrar de <b>%s</b> nas condições de "
                 "fixação descritas neste relatório, pelo método dos elementos finitos.</p>" % peca.Label)

    h.append("<h1>2. Modelo geométrico</h1>")
    h.append(_img(doc, "modelo", imgs.get("modelo"), "Figura 1 — Modelo com fixações e cargas", largura))
    h.append(_tab([_par("Peça", peca.Label),
                   _par("Dimensões externas", "%s × %s × %s mm" % (F.fmt(bb.XLength, 1), F.fmt(bb.YLength, 1),
                                                                 F.fmt(bb.ZLength, 1))),
                   _par("Volume", F.fmt(sh.Volume / 1000.0, 2, "cm³")),
                   _par("Massa", F.fmt(massa, 3, "kg") if massa else "—")], larguras=["40%", "60%"]))

    h.append("<h1>3. Material</h1>")
    if m is not None:
        lin = [_par("Material", m.Material.get("Name", m.Label)),
               _par("Módulo de elasticidade", F.fmt(pm["E"] / 1000.0 if pm.get("E") else None, 1, "GPa")),
               _par("Coeficiente de Poisson", F.fmt(pm.get("nu"), 2)),
               _par("Densidade", F.fmt(pm.get("rho"), 0, "kg/m³")),
               _par("Limite de escoamento", F.fmt(pm.get("Sy"), 0, "MPa") +
                    (" (certificado)" if est.get("SeworkSyCertificado", 0) else "")),
               _par("Resistência à tração", F.fmt(pm.get("Su"), 0, "MPa"))]
        if pm.get("norma"):
            lin.append(_par("Referência na biblioteca", ("%s %s" % (pm["norma"], pm.get("numero", ""))).strip()))
        h.append(_tab(lin, larguras=["40%", "60%"]))
        h.append("<p class='nota'>Propriedades da biblioteca de materiais do FreeCAD Sework, modelo linear "
                 "elástico isotrópico. Devem ser confirmadas com o certificado do material empregado.</p>")

    h.append("<h1>4. Condições de contorno</h1>")
    h.append("<h2>Fixações</h2>")
    fx = est.fixacoes
    h.append(_tab([[o.Label, "Engaste (deslocamentos nulos)" if F._eh(o, "Fem::ConstraintFixed") else
                    "Deslocamento imposto", _faces(o)] for o in fx] or [["—", "Peça livre", "—"]],
                  cab=["Nome", "Tipo", "Faces"], larguras=["34%", "36%", "30%"]))
    h.append("<h2>Cargas</h2>")
    cg = est.cargas
    linhas = []
    for o in cg:
        if F._eh(o, "Fem::ConstraintSelfWeight"):
            linhas.append([o.Label, "Peso próprio (gravidade)", "—", "Peça inteira"])
        elif F._eh(o, "Fem::ConstraintForce"):
            dire = "Direção da aresta de referência" if o.Direction else "Perpendicular à face"
            linhas.append([o.Label, "Força total — " + dire + (" (invertida)" if o.Reversed else ""),
                           F.fmt(F._valor_carga(o), 1, "N"), _faces(o)])
        elif F._eh(o, "Fem::ConstraintPressure"):
            linhas.append([o.Label, "Pressão" + (" (invertida)" if o.Reversed else ""),
                           F.fmt(F._valor_carga(o), 3, "MPa"), _faces(o)])
    h.append(_tab(linhas or [["—", "Sem cargas", "—", "—"]], cab=["Nome", "Tipo", "Valor", "Faces"],
                  larguras=["24%", "40%", "16%", "20%"]))

    h.append("<h1>5. Malha</h1>")
    if malha is not None:
        fm = malha.FemMesh
        ordem2 = getattr(malha, "ElementOrder", "2nd") == "2nd"
        h.append(_img(doc, "malha", imgs.get("malha"), "Figura 2 — Malha de elementos finitos", largura))
        h.append(_tab([_par("Gerador", "Gmsh"),
                       _par("Elemento", "Tetraédrico de %s (%d nós)" % ("2ª ordem" if ordem2 else "1ª ordem",
                                                                        10 if ordem2 else 4)),
                       _par("Tamanho máximo do elemento", str(getattr(malha, "CharacteristicLengthMax", "—"))),
                       _par("Nós", F.fmt(fm.NodeCount, 0)),
                       _par("Elementos", F.fmt(fm.VolumeCount, 0))], larguras=["40%", "60%"]))

    h.append("<h1>6. Solução</h1>")
    h.append(_tab([_par("Solver", "CalculiX" + (" " + est.get("SeworkCcxVersao") if est.get("SeworkCcxVersao") else "")),
                   _par("Tipo de análise", "Estática linear" if tipo == "static" else
                        "Frequências naturais (%d modos)" % len(e.get("modos", []))),
                   _par("Calculado em", est.get("SeworkCalculadoEm", "—")),
                   _par("Tempo de cálculo", F.fmt(est.get("SeworkTempoCalculo", None) or None, 1, "s"))],
                  larguras=["40%", "60%"]))

    h.append("<h1>7. Resultados</h1>")
    if tipo == "static":
        fs = e.get("fs")
        h.append(_tab([_par("Tensão máxima de von Mises", "<span class='destaque'>%s</span>" % F.fmt(e.get("svm_max"), 1, "MPa")),
                       _par("Deslocamento máximo", "<span class='destaque'>%s</span>" % F.fmt(e.get("u_max"), 4, "mm")),
                       _par("Limite de escoamento do material", F.fmt(e.get("Sy"), 0, "MPa")),
                       _par("Fator de segurança ao escoamento (Sy ÷ σmáx)",
                            "<span class='destaque'>%s</span>" % (F.fmt(fs, 2) if fs else "—"))],
                      larguras=["55%", "45%"]))
        h.append(_img(doc, "tensao", imgs.get("tensao"),
                      "Figura 3 — Tensão equivalente de von Mises: escala de 0 (azul) a %s (vermelho)"
                      % F.fmt(e.get("svm_max"), 1, "MPa"), largura))
        h.append(_img(doc, "deslocamento", imgs.get("deslocamento"),
                      "Figura 4 — Deslocamento total: escala de 0 (azul) a %s (vermelho)"
                      % F.fmt(e.get("u_max"), 4, "mm"), largura))
    else:
        h.append(_tab([[str(md), F.fmt(f, 3, "Hz")] for md, f in e.get("modos", [])],
                      cab=["Modo", "Frequência"], larguras=["30%", "70%"]))
        h.append(_img(doc, "deslocamento", imgs.get("deslocamento"), "Figura 3 — Forma do modo exibido", largura))

    conv = F.linhas_convergencia(est) if tipo == "static" else []
    if conv:
        h.append("<h2>Convergência da malha</h2>")
        h.append(_tab([[F.fmt(l["tamanho"], 2, "mm"), F.fmt(l["nos"], 0), F.fmt(l["svm"], 1, "MPa"),
                        F.fmt(l["u"], 4, "mm")] for l in conv],
                      cab=["Elemento máx.", "Nós", "σ máx.", "δ máx."], larguras=["25%", "20%", "27%", "28%"]))
        cs, cu, vs, vu = F.avaliar_convergencia(conv)
        h.append("<p>Entre as duas malhas mais finas, a tensão máxima variou %s e o deslocamento máximo %s "
                 "(critério adotado: até %s %%). %s</p>" % (
                     F.fmt(vs, 1, "%"), F.fmt(vu, 1, "%"), F.fmt(F.LIMITE_CONVERGENCIA, 0),
                     "Os resultados convergiram." if (cs and cu) else
                     ("O deslocamento convergiu; a variação da tensão indica concentração localizada que deve ser "
                      "avaliada pelo responsável técnico." if cu else "Os resultados ainda variam com a malha.")))
    h.append("<h1>8. Premissas e limitações</h1>")
    prem = ["Material linear elástico, homogêneo e isotrópico; não há plasticidade no modelo.",
            "Pequenos deslocamentos e pequenas deformações (sem não linearidade geométrica).",
            "Fixações ideais: as faces engastadas não se deslocam, o que tende a ser mais rígido que o apoio real.",
            "Não foram modelados contato, folgas, pré-carga de parafusos, soldas ou tensões residuais.",
            "Picos de tensão em cantos vivos e nas bordas das fixações podem ser singularidades numéricas; "
            "a convergência deve ser verificada com refino da malha nessas regiões.",
            "Os resultados valem apenas para as cargas e condições descritas neste relatório."]
    if tipo == "frequency":
        prem[1] = "Análise modal linear, sem amortecimento e sem pré-tensão."
    h.append("<ul>" + "".join("<li style='margin-bottom:4px'>%s</li>" % p for p in prem) + "</ul>")

    h.append("<h1>9. Conclusão</h1>")
    if tipo == "static":
        fs = e.get("fs")
        if fs is None:
            auto = ("O material não possui limite de escoamento na biblioteca; o fator de segurança não foi calculado. "
                    "A tensão máxima obtida foi de %s." % F.fmt(e.get("svm_max"), 1, "MPa"))
        elif fs < 1:
            auto = ("A tensão máxima de von Mises (%s) supera o limite de escoamento do material (%s), indicando "
                    "escoamento na região mais solicitada nas condições analisadas."
                    % (F.fmt(e.get("svm_max"), 1, "MPa"), F.fmt(e.get("Sy"), 0, "MPa")))
        else:
            auto = ("A tensão máxima de von Mises (%s) fica abaixo do limite de escoamento do material (%s), "
                    "com fator de segurança ao escoamento de %s nas condições analisadas."
                    % (F.fmt(e.get("svm_max"), 1, "MPa"), F.fmt(e.get("Sy"), 0, "MPa"), F.fmt(fs, 2)))
    else:
        modos = e.get("modos", [])
        auto = ("A primeira frequência natural obtida é de %s." % F.fmt(modos[0][1], 2, "Hz")) if modos else ""
    h.append("<p>%s</p>" % auto)
    obs = (est.get("SeworkRelObs", "") or "").strip()
    if obs:
        h.append("<p>%s</p>" % obs.replace("\n", "<br>"))
    h.append("<p class='nota'>A aceitação do resultado e o fator de segurança mínimo exigido são definidos pelo "
             "responsável técnico.</p>")

    resp = est.get("SeworkRelResp", "") or ""
    crea = est.get("SeworkRelCrea", "") or ""
    art = est.get("SeworkRelArt", "") or ""
    h.append("<br><br><br><table width='100%'><tr><td width='15%'></td>"
             "<td width='70%' align='center' style='border-top:1px solid #1F2A36; padding-top:6px'>"
             "<b>{}</b><br><span class='rot'>{}{}</span></td><td width='15%'></td></tr></table>".format(
                 resp or "Responsável técnico", ("CREA " + crea) if crea else "", (" · ART " + art) if art else ""))
    return "<html><body>%s</body></html>" % "".join(h)


# ------------------------------------------------------------------ pintura
def _mm(px_por_mm, v):
    return v * px_por_mm


def _fonte(px, peso=QtGui.QFont.Normal):
    f = QtGui.QFont("Poppins")
    f.setPixelSize(int(px))
    f.setWeight(peso)
    return f


def _capa(p, W, H, k, est):
    """k = pixels do dispositivo por mm."""
    tit = est.get("SeworkRelTitulo", "") or "Análise estrutural por elementos finitos"
    p.fillRect(QtCore.QRectF(0, 0, W, 95 * k), QtGui.QColor(NAVY))
    p.fillRect(QtCore.QRectF(0, 95 * k, W, 3 * k), QtGui.QColor(YEL))
    if os.path.isfile(LOGO):
        p.drawImage(QtCore.QRectF(MARGEM_MM * k, 22 * k, 26 * k, 26 * k), QtGui.QImage(LOGO))
    p.setPen(QtGui.QColor("#FFFFFF"))
    p.setFont(_fonte(5.0 * k, QtGui.QFont.DemiBold))
    p.drawText(QtCore.QRectF((MARGEM_MM + 32) * k, 24 * k, W, 10 * k), "SEWORK ENGENHARIA")
    p.setFont(_fonte(3.6 * k))
    p.drawText(QtCore.QRectF((MARGEM_MM + 32) * k, 33 * k, W, 10 * k), "Engenharia mecânica e segurança do trabalho")
    p.setFont(_fonte(8.5 * k, QtGui.QFont.Bold))
    p.drawText(QtCore.QRectF(MARGEM_MM * k, 55 * k, W - 2 * MARGEM_MM * k, 30 * k),
               QtCore.Qt.TextWordWrap, tit)
    p.setPen(QtGui.QColor(INK))
    p.setFont(_fonte(5.0 * k, QtGui.QFont.DemiBold))
    p.drawText(QtCore.QRectF(MARGEM_MM * k, 112 * k, W, 12 * k), "Relatório técnico de simulação")
    linhas = [("Peça analisada", est.peca.Label if est.peca else "—"),
              ("Cliente", est.get("SeworkRelCliente", "") or "—"),
              ("Equipamento / projeto", est.get("SeworkRelProjeto", "") or "—"),
              ("Tipo de análise", "Estática linear" if est.tipo == "static" else "Frequências naturais"),
              ("Data", time.strftime("%d/%m/%Y")),
              ("Responsável técnico", est.get("SeworkRelResp", "") or "—"),
              ("CREA", est.get("SeworkRelCrea", "") or "—"),
              ("ART nº", est.get("SeworkRelArt", "") or "—")]
    y = 130 * k
    for rot, val in linhas:
        p.setPen(QtGui.QColor(INK2))
        p.setFont(_fonte(3.7 * k))
        p.drawText(QtCore.QRectF(MARGEM_MM * k, y, 60 * k, 9 * k), rot)
        p.setPen(QtGui.QColor(INK))
        p.setFont(_fonte(3.9 * k, QtGui.QFont.DemiBold))
        p.drawText(QtCore.QRectF((MARGEM_MM + 62) * k, y, W - (MARGEM_MM * 2 + 62) * k, 9 * k), val)
        y += 9 * k
        p.setPen(QtGui.QPen(QtGui.QColor(LINHA), 0.25 * k))
        p.drawLine(QtCore.QPointF(MARGEM_MM * k, y - 1.5 * k), QtCore.QPointF(W - MARGEM_MM * k, y - 1.5 * k))
    _rodape(p, W, H, k, None, None)


def _cabecalho(p, W, k, est):
    p.setPen(QtGui.QColor(NAVY))
    p.setFont(_fonte(3.2 * k, QtGui.QFont.DemiBold))
    tit = "Relatório de simulação · " + (est.peca.Label if est.peca else "")
    p.drawText(QtCore.QRectF(MARGEM_MM * k, 8 * k, W - 60 * k, 6 * k), QtCore.Qt.AlignVCenter, tit)
    p.setPen(QtGui.QColor(INK2))
    p.setFont(_fonte(3.0 * k))
    p.drawText(QtCore.QRectF(W - (MARGEM_MM + 60) * k, 8 * k, 60 * k, 6 * k),
               QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter, "SEWORK ENGENHARIA")
    p.fillRect(QtCore.QRectF(MARGEM_MM * k, 15 * k, W - 2 * MARGEM_MM * k, 0.5 * k), QtGui.QColor(NAVY))


def _rodape(p, W, H, k, pagina, total):
    p.fillRect(QtCore.QRectF(MARGEM_MM * k, H - 13 * k, W - 2 * MARGEM_MM * k, 0.6 * k), QtGui.QColor(YEL))
    p.setPen(QtGui.QColor(INK2))
    p.setFont(_fonte(2.9 * k))
    p.drawText(QtCore.QRectF(MARGEM_MM * k, H - 11.5 * k, 120 * k, 6 * k), QtCore.Qt.AlignVCenter,
               "Sework Engenharia · Patos de Minas/MG · Gerado no FreeCAD Sework")
    if pagina:
        p.drawText(QtCore.QRectF(W - (MARGEM_MM + 50) * k, H - 11.5 * k, 50 * k, 6 * k),
                   QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter, "Página %d de %d" % (pagina, total))


def gerar(est, caminho):
    pasta = tempfile.mkdtemp(prefix="sework_rel_")
    # estado de visibilidade para restaurar depois
    objs = [o for o in est.doc.Objects if hasattr(o, "ViewObject") and o.ViewObject is not None]
    vis = {o.Name: o.ViewObject.Visibility for o in objs}
    imgs = {}
    try:
        imgs["modelo"] = _captura(est, "modelo", pasta)
        imgs["malha"] = _captura(est, "malha", pasta)
        if est.tipo == "static":
            imgs["tensao"] = _captura(est, "tensao", pasta)
        imgs["deslocamento"] = _captura(est, "deslocamento", pasta)
    finally:
        for o in objs:
            try:
                o.ViewObject.Visibility = vis.get(o.Name, o.ViewObject.Visibility)
            except Exception:
                pass

    w = QtGui.QPdfWriter(caminho)
    w.setPageSize(QtGui.QPageSize(QtGui.QPageSize.A4))
    w.setPageMargins(QtCore.QMarginsF(0, 0, 0, 0))
    w.setResolution(DPI)
    w.setTitle("Relatório de simulação — " + (est.peca.Label if est.peca else ""))
    w.setCreator("FreeCAD Sework")
    k = DPI / 25.4                       # pixels do dispositivo por mm
    W, H = w.width(), w.height()
    escala = DPI / 96.0                  # px do documento → dispositivo
    topo, base = 22 * k, 18 * k
    larg_doc = (W - 2 * MARGEM_MM * k) / escala
    alt_doc = (H - topo - base) / escala

    doc = QtGui.QTextDocument()
    doc.setDefaultStyleSheet(CSS)
    f = QtGui.QFont("Poppins")
    f.setPixelSize(14)
    doc.setDefaultFont(f)
    doc.setDocumentMargin(0)
    doc.setHtml(_corpo(est, doc, imgs, int(larg_doc * 0.78)))
    doc.setTextWidth(larg_doc)
    doc.setPageSize(QtCore.QSizeF(larg_doc, alt_doc))
    paginas = doc.pageCount()
    total = paginas + 1

    p = QtGui.QPainter(w)
    p.setRenderHint(QtGui.QPainter.Antialiasing)
    p.setRenderHint(QtGui.QPainter.TextAntialiasing)
    _capa(p, W, H, k, est)
    for i in range(paginas):
        w.newPage()
        _cabecalho(p, W, k, est)
        _rodape(p, W, H, k, i + 2, total)
        p.save()
        p.translate(MARGEM_MM * k, topo)
        p.scale(escala, escala)
        p.translate(0, -i * alt_doc)
        doc.drawContents(p, QtCore.QRectF(0, i * alt_doc, larg_doc, alt_doc))
        p.restore()
    p.end()
    return caminho
