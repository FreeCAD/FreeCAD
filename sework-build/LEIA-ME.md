# Compilar o FreeCAD Sework no Windows

Roteiro oficial do FreeCAD (Developers Handbook → Getting Started → Windows com pixi),
automatizado pelo `compilar.ps1`.

## 1. Pré-requisitos (uma vez)

1. **Visual Studio Community 2022** — no instalador, marcar a carga
   **"Desenvolvimento para desktop com C++"**. (É a versão usada pelo conda-forge; versões
   mais novas podem ficar instaladas junto, mas a 2022 precisa existir.)
2. **Git for Windows** — já instalado (você clonou o repositório com ele).
3. **pixi** — no PowerShell:
   ```powershell
   iwr -useb https://pixi.sh/install.ps1 | iex
   ```
   Feche e abra o PowerShell depois.
4. **Caminho sem espaços** — recomendado mover o clone de
   `Downloads\Freecad Sework\FreeCAD-Sework` para algo como `C:\dev\FreeCAD-Sework`
   (ferramentas do ambiente conda/pixi costumam falhar com espaços no caminho).
5. **Espaço em disco** — deixe ao menos 40 GB livres.

## 2. Verificar o ambiente

```powershell
cd C:\dev\FreeCAD-Sework
powershell -ExecutionPolicy Bypass -File .\sework-build\compilar.ps1 -SomenteVerificar
```

O script confere caminho, disco, Git, Visual Studio 2022 com C++, pixi e o patch de correções,
e diz em português o que falta.

## 3. Compilar

```powershell
powershell -ExecutionPolicy Bypass -File .\sework-build\compilar.ps1
```

- Aplica o patch `sework-patches\0001-...` se ainda não estiver aplicado.
- Roda `pixi run configure` e `pixi run build` (registro em `sework-build\compilacao.log`).
- A primeira vez baixa todas as dependências (Qt, OpenCASCADE, VTK, Python…) e compila
  ~2.000 arquivos: espere algumas horas. As próximas são incrementais (minutos).
- Se faltar memória: `...\compilar.ps1 -Nucleos 4`.

## 4. Abrir o FreeCAD compilado

```powershell
pixi run freecad
```

Na primeira abertura ele cria a própria pasta de dados (versão de desenvolvimento).
Feche, rode `sework-addon\instalar.ps1` para copiar o addon Sework para ela e abra de novo.

## 5. Testar as correções

- Ponto de referência → modo **Interseção**: duas arestas que se cruzam (deve criar o ponto)
  e duas que não se cruzam (deve dar erro); aresta + face.
- **Balão** clicando numa face de uma vista do Desenho técnico.
- Preferências → Part → Geral → Experimental → ligar recursos experimentais e conferir o
  botão de espaçamentos variáveis no Padrão linear/circular.
