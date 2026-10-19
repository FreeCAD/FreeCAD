<img src="/src/Gui/Icons/freecad.svg" height="100px" width="100px" alt="FreeCAD Sework">

# FreeCAD Sework

### Modelagem 3D paramétrica em português, com a cara e as ferramentas da Sework Engenharia

[Problemas e sugestões](https://github.com/sework-engenharia/FreeCAD-Sework/issues) •
[Como contribuir](CONTRIBUTING.md) •
[FreeCAD original](https://github.com/FreeCAD/FreeCAD) •
[Documentação do FreeCAD](https://wiki.freecad.org)

<img src="/.github/images/sework/02-peca.png" width="800" alt="Peça modelada no FreeCAD Sework">

O FreeCAD Sework é uma distribuição do [FreeCAD](https://www.freecad.org), o modelador 3D paramétrico de
código aberto, preparada pela [Sework Engenharia](https://github.com/sework-engenharia) para quem trabalha com
projeto mecânico no Brasil. Ele mantém todo o FreeCAD e acrescenta interface em português, identidade visual
própria e ferramentas que encurtam o caminho entre a ideia e o desenho ou a simulação.

> [!NOTE]
> Projeto em desenvolvimento. O instalador para Windows está em testes e a versão para Linux está planejada.
> O FreeCAD Sework não é um projeto oficial do FreeCAD nem é mantido pela comunidade do FreeCAD.

Visão geral
-----------

* **Tudo em português** Menus, painéis, nomes de propriedades e opções de listas traduzidos, inclusive termos
  que a tradução oficial ainda não cobre.

* **Faixa de ferramentas** Os comandos principais organizados em abas (Início, Esboço, Peça, Sólidos, Montagem,
  Desenho, Planilha, Simulação e Sework), com ícones grandes e rótulos claros. A aba Esboço aparece sozinha ao editar
  um esboço.

* **Tela inicial** Começa uma peça, montagem, desenho técnico, esboço ou planilha com um clique e mostra os
  arquivos recentes.

* **Assistente de simulação** Análise por elementos finitos em etapas guiadas: peça, material, fixações,
  cargas, malha, verificação de pendências, resultados e relatório em PDF.

* **Tutorial da primeira peça** Passo a passo interativo (esboço, cotas, extrusão, furo, arredondamento e
  salvar) que acompanha o que você faz na tela.

* **Guia ilustrado** Ao editar extrusão, cavidade, furo, arredondamento, chanfro, padrões e outras operações,
  o painel de tarefas mostra um desenho do que a operação faz no modo escolhido.

* **Identidade visual Sework** Tema claro nas cores da empresa, ícones redesenhados, tela de abertura e
  modelos de folha A1 a A4 para desenho técnico.

* **Testes automatizados** Um conjunto de testes confere tradução, aparência, assistente de simulação e
  comportamento da interface a cada versão.

* **Base sólida** Tudo o que o FreeCAD oferece continua disponível: núcleo geométrico OpenCASCADE, cena 3D
  Coin3D, interface Qt e programação em Python.

Imagens
-------

| | |
|:---:|:---:|
| <img src="/.github/images/sework/01-inicio.png" width="400" alt="Tela inicial"><br>Tela inicial | <img src="/.github/images/sework/03-esboco.png" width="400" alt="Edição de esboço"><br>Esboço com restrições |
| <img src="/.github/images/sework/04-desenho.png" width="400" alt="Desenho técnico"><br>Desenho técnico no modelo A3 Sework | <img src="/.github/images/sework/05-simulacao.png" width="400" alt="Simulação"><br>Assistente de simulação com resultado de tensões |

Instalação
----------

O instalador para Windows está em fase de testes e será publicado na
[página de versões](https://github.com/sework-engenharia/FreeCAD-Sework/releases) assim que estiver aprovado.
Ele instala o FreeCAD Sework ao lado de um FreeCAD original já existente, sem substituí-lo.

A versão para Linux está planejada para depois da aprovação da versão Windows.

Versões do código
-----------------

| Ramo | Base | Uso |
|---|---|---|
| `sework-1.1` | FreeCAD 1.1.4 (estável) | Versão usada para gerar os instaladores |
| `sework-main` | Ramo de desenvolvimento do FreeCAD | Onde as novidades são feitas e testadas primeiro |

O complemento Sework (interface, tradução, assistentes e tema) fica em [`sework-addon/`](sework-addon/), os
roteiros de compilação em [`sework-build/`](sework-build/) e as correções no código do FreeCAD em
[`sework-patches/`](sework-patches/).

Compilação
----------

No Windows, o roteiro [`sework-build/compilar.ps1`](sework-build/compilar.ps1) confere os pré-requisitos e
compila. É preciso:

- Visual Studio 2022 com a carga de trabalho *Desenvolvimento para desktop com C++*;
- Git for Windows;
- [pixi](https://pixi.sh);
- pelo menos 40 GB livres em disco.

Rode `sework-build\compilar.ps1 -SomenteVerificar` para conferir o ambiente e depois sem o parâmetro para
compilar. Para detalhes gerais sobre a compilação do FreeCAD, veja o
[Manual do Desenvolvedor do FreeCAD](https://freecad.github.io/DevelopersHandbook/gettingstarted/) (em inglês).

Como relatar problemas
----------------------

Para relatar um problema:

- Procure nos [problemas já abertos](https://github.com/sework-engenharia/FreeCAD-Sework/issues) para não
  repetir um relato;
- Informe a versão em `Ajuda > Sobre o FreeCAD > Copiar para a área de transferência`;
- Descreva passo a passo como reproduzir o problema;
- Se puder, anexe um arquivo de exemplo (o `.FCStd` compactado em ZIP);
- Se o problema também acontece no FreeCAD original, ele provavelmente deve ser relatado no
  [repositório do FreeCAD](https://github.com/FreeCAD/FreeCAD/issues).

Uso e ajuda
-----------

Por ser baseado no FreeCAD, toda a documentação dele vale para o FreeCAD Sework (wiki em inglês; muitas
páginas têm versão em português pelo seletor de idioma):

- [Primeiros passos](https://wiki.freecad.org/Getting_started)
- [Lista de recursos](https://wiki.freecad.org/Feature_list)
- [Perguntas frequentes](https://wiki.freecad.org/FAQ)
- [Bancadas de trabalho](https://wiki.freecad.org/Workbenches)
- [Programação em Python](https://wiki.freecad.org/Power_users_hub)

Licença
-------

O FreeCAD Sework é distribuído sob a mesma licença do FreeCAD, a
[GNU LGPL versão 2.1 ou posterior](LICENSE). O texto da licença é mantido no original em inglês, que é a
versão com validade jurídica.

Créditos
--------

O FreeCAD Sework existe graças ao trabalho de milhares de pessoas da comunidade do
[FreeCAD](https://www.freecad.org), que desenvolvem e mantêm o programa. Se o FreeCAD é útil para
você, considere [apoiar o projeto original](https://wiki.freecad.org/Donate).
