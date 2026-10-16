# FreeCAD Sework — addon de identidade visual

Addon em Python que aplica a marca Sework no FreeCAD oficial, sem compilar.

## O que ele faz

| Item | Como |
|---|---|
| Splash Sework | copia `Resources/splash_image.png` para `<dados do usuário>/Gui/images/` — o FreeCAD usa esse arquivo antes da splash padrão (vale a partir da 2ª abertura) |
| Título "FreeCAD Sework x.y.z" e ícone da janela | `QApplication.setApplicationName` + `setWindowIcon` ao iniciar |
| Tema "Sework Claro" | pacote de preferências: acento #003A70, fundo 3D #F5F8FC, pré-seleção #F7BE00, corpo ativo amarelo. Aplicado sozinho na 1ª execução; também em Preferências → Geral → Pacotes de preferências |
| Bancada "Sework" | menu e barra com *Aplicar tema Sework*, *Sobre o FreeCAD Sework* e *Voltar à splash original* |

## Instalar

1. Instale o FreeCAD oficial (1.0 ou mais novo) e abra uma vez.
2. Rode `instalar.ps1` (botão direito → *Executar com o PowerShell*). Ele copia a pasta `Sework` para `%APPDATA%\FreeCAD\Mod` (e para `v*\Mod`, nas versões que usam pasta por versão).
3. Abra o FreeCAD, feche e abra de novo.

Instalação manual: copie a pasta `Sework` para a pasta `Mod` do usuário (no FreeCAD: *Ajuda → Abrir pasta de dados do usuário* — o nome do menu varia por versão).

## Opções (Ferramentas → Editar parâmetros → BaseApp/Preferences/Mod/Sework)

- `RenameWindow` (bool, padrão true) — título/ícone Sework
- `InstallSplash` (bool, padrão true) — instalar a splash
- `ApplyThemeOnFirstRun` (bool, padrão true) — aplicar o tema na 1ª execução

## Opcional: `extras/branding.xml`

Copiado para a pasta `bin` da instalação (exige administrador), ajusta o nome do programa e as cores dos textos da splash desde o primeiro instante.

## Trocar a arte

- Splash: substitua `Resources/splash_image.png` (960 × 540 px). Deixe claras a faixa superior esquerda (o FreeCAD escreve a versão em x=6, y=75) e a borda inferior esquerda (mensagens de carregamento).
- Ícone: `Resources/icons/sework.png` (256 px) e `sework.ico`.
- O "S" do ícone e a splash são provisórios, montados com a fonte Poppins e as cores oficiais. Coloque aqui o símbolo oficial da marca.

## Licença

Este addon é distribuído sob LGPL-2.1-or-later, a mesma licença do FreeCAD.
