# FreeCAD Sework — addon de interface e identidade visual

Addon em Python que dá ao FreeCAD oficial (1.1) a cara da Sework, sem compilar.

## O que ele faz

| Item | Como |
|---|---|
| Faixa de ferramentas (ribbon) | Abas Início, Esboço, Peça, Sólidos, Montagem, Desenho, Planilha e Sework. Comandos principais com ícone grande, demais empilhados, grupos com legenda. Rótulos em português escritos pela Sework. As barras clássicas ficam ocultas. |
| Aba Esboço automática | Ao editar um esboço, a faixa mostra a aba Esboço e volta à anterior ao concluir. |
| Tradução pt-BR completa | `translations/sework_ptbr.qm` (tradução mais recente do repositório do FreeCAD, mais completa que a da versão 1.1.4) + dicionário `SeworkI18n.EXTRA` para o que nem o repositório traduz. Instalada com prioridade sobre a oficial. |
| Tema "Sework Claro" | Acento azul #003A70, pré-seleção amarela #F7BE00, vista 3D em degradê branco → #CBD5E0, menus e painéis claros. Força o esquema claro mesmo com o Windows em modo escuro. |
| Tema de ícones "Sework Sóbrio" | 1.582 ícones do FreeCAD recoloridos para a paleta Sework (azul-aço, âmbar, tijolo, sálvia, cinza frio), contornos em ardósia e degradê vertical em todo preenchimento chapado (efeito 3D). Vale para faixa, menus e árvore do modelo. Fica em `Resources/tema-icones-sework.zip`; o addon descompacta em `<dados do usuário>/SeworkIcones` e registra em `Bitmaps/CustomPathSework`. Liga/desliga na aba Sework (vale ao reabrir). |
| Guia ilustrado nas tarefas | Ao editar Extrusão, Cavidade, Revolução, Ranhura, Furo, Arredondar, Chanfro, Padrão linear/circular, Espelhar, Casca e Ângulo de saída, aparece no topo do painel Tarefas um desenho isométrico do que a função faz **no modo escolhido** (acompanha as trocas de modo), com legenda e instrução curta. Desenhos em `SeworkIlustra.py`, textos e regras em `SeworkGuia.py`. Desligar: parâmetro `GuiaIlustrado`. |
| Qualidade de exibição | Anti-aliasing MSAA 8x (linhas sem serrilhado) e tesselação mais fina (desvio 0,1 %, ângulo 12°) para curvas lisas. Aplicado uma vez; ajuste em `QUALITY` no `SeworkGui.py`. |
| Splash, título e ícone | Splash Sework, "FreeCAD Sework" no título, ícone Sework. |

## Arquivos

- `InitGui.py` — ponto de entrada (carregado pelo FreeCAD)
- `SeworkGui.py` — branding, tema, comandos Sework
- `SeworkRibbon.py` — a faixa de ferramentas (abas e comandos ficam na lista `TABS`)
- `SeworkI18n.py` — tradução com prioridade e dicionário `EXTRA`
- `SeworkPropNames.py` — nomes de propriedades e grupos do painel Dados/Vista (~500 termos, contexto `App::Property`)
- `Sework Claro/` — tema (pacote de preferências; o FreeCAD procura nesta pasta)
- `Resources/` — splash e ícones
- `translations/sework_ptbr.qm` — tradução compilada

## Instalar

1. Instale o FreeCAD 1.1 oficial e abra uma vez.
2. Rode `instalar.ps1` (botão direito → *Executar com o PowerShell*). Copia a pasta `Sework` para `%APPDATA%\FreeCAD\v1-1\Mod` (e `Mod` de outras versões).
3. Abra o FreeCAD.

## Mudar a faixa

Edite a lista `TABS` em `SeworkRibbon.py`. Cada item é `("NomeDoComando", L ou S, "Rótulo")`:
`L` = ícone grande, `S` = pequeno; `\n` quebra o rótulo grande em duas linhas.
Comando que não existir na versão instalada é ignorado.

## Opções (Ferramentas → Editar parâmetros → BaseApp/Preferences/Mod/Sework)

- `RibbonEnabled` (padrão true) — faixa Sework; também alternável pelo comando *Alternar faixa Sework / barras clássicas* na aba Sework
- `RibbonHeight` (padrão 104) — altura da área de comandos, em px
- `SeworkTranslations` (padrão true) — tradução Sework
- `RenameWindow`, `InstallSplash`, `ApplyThemeOnFirstRun` (padrão true)

## Atualizar o tema de ícones

`tools/gerar_tema_icones.py <clone do FreeCAD> <pasta de saída>` gera os SVGs. Paleta e intensidade do degradê
ficam nas funções `recolor()` e `shade()`. Depois compacte a pasta em `Resources/tema-icones-sework.zip`.

## Atualizar a tradução

Com o clone do FreeCAD: `lrelease $(find . -name "*_pt-BR.ts") -qm sework_ptbr.qm`.

## Licença

LGPL-2.1-or-later, a mesma do FreeCAD.
