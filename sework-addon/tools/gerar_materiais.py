"""Gera os cartões de material (.FCMat) da biblioteca Sework.

Valores de resistência = mínimos da especificação (limite de escoamento mínimo e limite
inferior da resistência à tração), conferidos em fontes públicas em 17/10/2026:
- ASTM A36, A572 Gr 50, NBR 7007 MR250/AR350: catálogo ArcelorMittal Perfis e Barras (2017)
- A283 Gr C: aasteel.com/a-283 · A285 Gr C: aasteel.com/sa-285 e SSAB
- A516 Gr 60/70: epcland.com · A515 Gr 70: aasteel.com/sa-515
- A106 Gr B: projectmaterials.com · A53 Gr B: fedsteel.com · A105: projectmaterials.com
- A500 Gr B (redondo): Wikipedia ASTM A500 · A240 304/304L: tubingchina (ASTM A213/A240);
  316: Rolled Alloys datasheet · 6061-T6 (B209): matmatch.com
Módulo de elasticidade: ASME B31.1 a 21 °C (aço carbono 29,5e6 psi ≈ 203 GPa; austenítico
28,3e6 psi ≈ 195 GPa), via Engineering ToolBox. Alumínio 69 GPa (AA/ASTM, matmatch).
Densidade e Poisson: valores típicos.
"""
import os
import sys
import uuid

NS = uuid.UUID("5e40c0a0-5e40-4c0a-8e40-5e40c0a05e40")
M_FATHER = "9cdda8b6-b606-4778-8f13-3934d8668e67"
M_STD = "1e2c0088-904a-4537-925f-64064c07d700"
M_LIN = "7b561d1d-fb9b-44f6-9da9-56a4f74d7536"

AC = dict(E=203000, nu=0.3, rho=7850)       # aço carbono / baixa liga
INOX = dict(E=195000, nu=0.3, rho=8000)     # austenítico
AL = dict(E=69000, nu=0.33, rho=2700)

# (pasta, nome, norma, grau, tipo, Sy, Su, base, fonte)
MATERIAIS = [
    ("Acos estruturais", "ASTM A36", "ASTM A36", "", "Aço carbono estrutural", 250, 400, AC, "ArcelorMittal"),
    ("Acos estruturais", "NBR 7007 MR250", "ABNT NBR 7007", "MR250", "Aço carbono estrutural", 250, 400, AC, "ArcelorMittal"),
    ("Acos estruturais", "NBR 7007 AR350", "ABNT NBR 7007", "AR350", "Aço de alta resistência e baixa liga", 350, 450, AC, "ArcelorMittal"),
    ("Acos estruturais", "ASTM A572 Gr 50", "ASTM A572", "Grade 50", "Aço de alta resistência e baixa liga", 345, 450, AC, "ArcelorMittal"),
    ("Acos estruturais", "ASTM A283 Gr C", "ASTM A283", "Grade C", "Aço carbono para chapas", 205, 380, AC, "aasteel.com"),
    ("Acos estruturais", "ASTM A500 Gr B (tubo redondo)", "ASTM A500", "Grade B", "Tubo estrutural conformado a frio", 290, 400, AC, "ASTM A500 (Wikipedia)"),
    ("Acos para vasos de pressao", "ASTM A285 Gr C (SA-285 C)", "ASTM A285 / ASME SA-285", "Grade C", "Chapa para vaso de pressão", 205, 380, AC, "aasteel.com, SSAB"),
    ("Acos para vasos de pressao", "ASTM A516 Gr 60 (SA-516 60)", "ASTM A516 / ASME SA-516", "Grade 60", "Chapa para vaso de pressão", 220, 415, AC, "epcland.com"),
    ("Acos para vasos de pressao", "ASTM A516 Gr 70 (SA-516 70)", "ASTM A516 / ASME SA-516", "Grade 70", "Chapa para vaso de pressão", 260, 485, AC, "epcland.com"),
    ("Acos para vasos de pressao", "ASTM A515 Gr 70 (SA-515 70)", "ASTM A515 / ASME SA-515", "Grade 70", "Chapa para vaso de pressão", 260, 485, AC, "aasteel.com"),
    ("Tubos e forjados", "ASTM A106 Gr B (SA-106 B)", "ASTM A106 / ASME SA-106", "Grade B", "Tubo sem costura", 240, 415, AC, "projectmaterials.com"),
    ("Tubos e forjados", "ASTM A53 Gr B (SA-53 B)", "ASTM A53 / ASME SA-53", "Grade B", "Tubo com e sem costura", 240, 415, AC, "fedsteel.com"),
    ("Tubos e forjados", "ASTM A105 (SA-105)", "ASTM A105 / ASME SA-105", "", "Forjado (flanges, conexões)", 250, 485, AC, "projectmaterials.com"),
    ("Acos inoxidaveis", "AISI 304 (A240)", "ASTM A240 / ASME SA-240", "Type 304", "Aço inoxidável austenítico", 205, 515, INOX, "ASTM A213/A240 via tubingchina"),
    ("Acos inoxidaveis", "AISI 304L (A240)", "ASTM A240 / ASME SA-240", "Type 304L", "Aço inoxidável austenítico", 170, 485, INOX, "ASTM A213/A240 via tubingchina"),
    ("Acos inoxidaveis", "AISI 316 (A240)", "ASTM A240 / ASME SA-240", "Type 316", "Aço inoxidável austenítico", 205, 515, INOX, "Rolled Alloys"),
    ("Aluminio", "Alumínio 6061-T6 (B209)", "ASTM B209", "6061-T6", "Liga de alumínio tratada", 240, 290, AL, "matmatch.com"),
    # Construção mecânica: escoamento depende do estado (laminado, trefilado, normalizado) -> sem valor
    ("Acos para construcao mecanica", "SAE 1020", "SAE J403", "1020", "Aço carbono para construção mecânica", None, None, AC, ""),
    ("Acos para construcao mecanica", "SAE 1045", "SAE J403", "1045", "Aço carbono para construção mecânica", None, None, AC, ""),
]


def cartao(pasta, nome, norma, grau, tipo, sy, su, base, fonte):
    u = str(uuid.uuid5(NS, nome))
    if sy:
        desc = ("Limites mínimos da especificação {}{}. Fonte da conferência: {}. Módulo de elasticidade "
                "conforme ASME B31.1 a 21 °C; densidade e Poisson típicos. Confirme com o certificado do "
                "material.").format(norma, (" " + grau) if grau else "", fonte)
    else:
        desc = ("O limite de escoamento do {} depende do estado de fornecimento (laminado, trefilado, "
                "normalizado) e não tem mínimo de norma: use o valor do certificado. Módulo de elasticidade "
                "conforme ASME B31.1 a 21 °C; densidade e Poisson típicos.").format(nome)
    lin = ['    Density: "{} kg/m^3"'.format(base["rho"]), '    PoissonRatio: "{}"'.format(base["nu"]),
           '    YoungsModulus: "{} MPa"'.format(base["E"])]
    if sy:
        lin.append('    YieldStrength: "{} MPa"'.format(sy))
    if su:
        lin.append('    UltimateTensileStrength: "{} MPa"'.format(su))
    return "\n".join([
        "---",
        "# Biblioteca de materiais Sework Engenharia",
        "General:",
        '  UUID: "{}"'.format(u),
        '  Author: "Sework Engenharia"',
        '  License: "CC-BY-4.0"',
        '  Name: "{}"'.format(nome),
        '  Description: "{}"'.format(desc.replace('"', "'")),
        "Models:",
        "  Father:",
        "    UUID: '{}'".format(M_FATHER),
        '    Father: "Metal"',
        "  MaterialStandard:",
        "    UUID: '{}'".format(M_STD),
        '    KindOfMaterial: "{}"'.format(tipo),
        '    MaterialNumber: "{}"'.format(grau),
        '    StandardCode: "{}"'.format(norma),
        "  LinearElastic:",
        "    UUID: '{}'".format(M_LIN),
    ] + lin) + "\n"


def main(destino):
    import re
    for m in MATERIAIS:
        pasta = os.path.join(destino, m[0])
        os.makedirs(pasta, exist_ok=True)
        arq = re.sub(r"[^A-Za-z0-9_.-]+", "_", m[1].replace("í", "i")).strip("_") + ".FCMat"
        with open(os.path.join(pasta, arq), "w", encoding="utf-8") as f:
            f.write(cartao(*m))
    print(len(MATERIAIS), "cartões em", destino)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "..", "Sework", "Resources", "Materials"))
