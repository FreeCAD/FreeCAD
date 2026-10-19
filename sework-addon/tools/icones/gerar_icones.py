# SPDX-License-Identifier: LGPL-2.1-or-later
"""Gera os ícones Sework (estilo do guia ilustrado) e monta o pacote do tema.

Uso:
    python gerar_icones.py <pasta_tema_recolorido> <saida.zip> [--folha folha.png]

1. Gera um SVG por nome de pixmap (famílias 3D, Esboço e Geral).
2. Sobrepõe esses SVGs ao tema recolorido (gerar_tema_icones.py), que cobre
   os demais ícones do FreeCAD.
3. Grava o zip do tema, com VERSAO.txt.
"""
import datetime
import os
import sys
import zipfile

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import familia_3d
import familia_esboco
import familia_geral
import familia_painel
import familia_desenho

VERSAO = "Sework Guia 2.0"


def todos():
    out = {}
    for fam in (familia_3d, familia_esboco, familia_geral, familia_painel, familia_desenho):
        for nome, fn in fam.ICONS.items():
            base = nome[4:] if nome.startswith("SUB:") else os.path.basename(nome)
            if base.endswith(".svg"):
                base = base[:-4]
            out[base] = fn
    return out


def main():
    tema, saida = sys.argv[1], sys.argv[2]
    icons = todos()
    cache = {}
    novos = {}
    for nome, fn in sorted(icons.items()):
        if fn not in cache:
            cache[fn] = fn().svg()
        novos[nome + ".svg"] = cache[fn]
        # O FreeCAD procura ícones "actions/Nome" numa subpasta actions/ do tema
        if "/" not in nome:
            novos["actions/" + nome + ".svg"] = cache[fn]
    base = {f: os.path.join(tema, f) for f in os.listdir(tema) if f.endswith(".svg")}
    hoje = datetime.date.today().strftime("%d/%m/%Y")
    with zipfile.ZipFile(saida, "w", zipfile.ZIP_DEFLATED) as z:
        for f, p in sorted(base.items()):
            if f not in novos:
                z.write(p, f)
        for f, s in sorted(novos.items()):
            z.writestr(f, s)
        z.writestr("VERSAO.txt", "%s - %s - %d ícones redesenhados + %d recoloridos\n"
                   % (VERSAO, hoje, len(novos), len([f for f in base if f not in novos])))
    print("redesenhados:", len(novos), "| recoloridos mantidos:", len([f for f in base if f not in novos]))
    if "--folha" in sys.argv:
        import folha
        vistos, itens = set(), []
        for nome, fn in sorted(icons.items()):
            if fn in vistos:
                continue
            vistos.add(fn)
            itens.append((nome, cache[fn]))
        folha.folha(itens, sys.argv[sys.argv.index("--folha") + 1], sizes=(48, 24), cols=10)


if __name__ == "__main__":
    main()
