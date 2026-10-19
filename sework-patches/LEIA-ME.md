# Correções Sework no código do FreeCAD

Patch gerado sobre o commit `3f29b7272e6d` (o mesmo do clone `sework-main`).
Vale quando o fork for compilado — não muda o FreeCAD 1.1.4 instalado.

## O que corrige

| Ferramenta | Antes | Depois |
|---|---|---|
| Ponto de referência → modo **Interseção** (Fixação) | Aparecia como "Não implementado" e não aceitava referências | Aceita aresta+aresta ou aresta+face e coloca o ponto na interseção; recusa referências que não se cruzam |
| **Balão** (Desenho técnico) | Com uma face selecionada, o balão não era posicionado | Ponta do balão no centro de massa da face |
| Padrão linear/circular — **espaçamentos variáveis** | A dica mandava criar o parâmetro "ExperiementalFeature" (erro de digitação) — a opção nunca aparecia | Dica corrigida para "ExperimentalFeatures" (o que o código realmente lê) |

Arquivos: `Mod/Part/App/Attacher.cpp`, `Mod/Part/Gui/AttacherTexts.cpp`,
`Mod/Part/App/LinearPatternExtension.cpp`, `Mod/Part/App/PolarPatternExtension.cpp`,
`Mod/TechDraw/Gui/Command.cpp`.

## Aplicar (PowerShell, na pasta do clone)

```powershell
cd "C:\Users\Ronne\Downloads\Freecad Sework\FreeCAD-Sework"
git apply --check --ignore-whitespace sework-patches\0001-correcoes-ferramentas-inacabadas.patch
git apply --ignore-whitespace sework-patches\0001-correcoes-ferramentas-inacabadas.patch
git add src
git commit -m "Corrige modo Interseção do ponto de referência, balão em face e dica de espaçamento variável" -m "Assisted-by: Claude (Anthropic)"
```

`--ignore-whitespace` evita falha por diferença de quebra de linha (CRLF/LF) do Git no Windows.

## Antes de considerar pronto

1. Compilar o fork e testar:
   - Ponto de referência com duas arestas que se cruzam (modo Interseção) e com duas que **não** se cruzam (deve dar erro).
   - Ponto com aresta + face plana.
   - Balão clicando numa face de uma vista.
2. Se quiser enviar ao FreeCAD oficial: a política de IA do projeto (`AI_POLICY.md`)
   exige que quem envia entenda, revise e teste o código, e que o commit leve a linha
   `Assisted-by:`. Contribuições "claramente geradas por IA" não são aceitas.

## Fora deste patch (exigem trabalho maior)

- Esboço: cota entre ponto/linha/círculo e elipse; perpendicular/igualdade em B-spline (mudanças no solver).
- Assistente de eixo (WizardShaft): depende de "referências robustas", que o FreeCAD ainda não tem.
- Texto no esboço e Restrição de grupo: marcados pelos próprios desenvolvedores como experimental / prévia.
