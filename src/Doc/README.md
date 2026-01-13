# FreeCAD Source Documentation

This directory contains the configuration, templates, and entry points used to
generate FreeCAD’s C++ and Python source documentation.

Source documentation is currently generated with **Doxygen** only.
Two different profiles are available at this time:

- a full developer-oriented version (target `DevDoc` and profile `DEV`)
- a lighter web-oriented version (target `WebDoc` and profile `WEB`)


## Overview

The documentation system is built around:

- **Doxygen** for parsing C++ and Python source code
- **CMake** for configuration, preprocessing, and build targets
- Custom HTML templates, layout, styling, and assets for FreeCAD

The generated documentation is available in the build tree under the `doc` dir
and must not be committed to the source repository.


## Directory Structure

Key files and directories:

- `BuildDoc.cfg.in`
  Template for the Doxygen configuration file. Values are substituted by CMake.

- `CMakeLists.txt`
  Main entry point that configures Doxygen, defines inputs, and creates build
  targets.

- `*.dox`
  Doxygen documentation files (groups, pages, tips) and page templates.

- `FreecadDoxygenLayout.xml`
  Custom Doxygen layout definition.

- `templates/`
  HTML header/footer, CSS, JavaScript, icons, and static assets.

- `legacy/`
  Old and deprecated documentation tooling and configurations. Not used by the
  current build.


## Build Profiles

Currently, two documentation profiles are available:

### DevDoc - Developer Documentation

Intended for local development and deep code inspection.

Characteristics:
- Full symbol extraction
- Static members included
- Class and group graphs enabled via Graphviz
- No JavaScript

If not already done, run CMake from your build directory (preferably out-of-source):

```bash
cmake ../<path-to-FreeCAD-source>
```

Build target:

```bash
cmake --build . --target DevDoc
```

Output location:

```bash
<build>/doc/DEV/html/<git-branch>/
```

### WebDoc - Web Documentation

Intended for web publishing and general browsing.

Characteristics:
- Lighter than the full dev version
- Public API only
- Tree view and search enabled via JavaScript
- Paths stripped for cleaner output

If not already done, run CMake from your build directory (preferably out-of-source):

```bash
cmake ../<path-to-FreeCAD-source>
```

Build target:

```bash
cmake --build . --target WebDoc
```

Output location:

```bash
<build>/doc/WEB/html/<git-branch>/
```


## How It Works

1. CMake detects Doxygen (and other available dependencies like Git, Coin, Graphviz).
2. Git metadata (branch, commit) is extracted and embedded in the documentation.
3. `BuildDoc.cfg.in` is configured into a Doxygen config file per profile in the build tree.
4. Templates and assets are copied into the build tree.
5. Doxygen is executed with profile-specific options and outputs the generated files in the `doc` dir.

All inputs, file patterns, exclusions, predefined macros, and layout options are
defined centrally in `CMakeLists.txt` to ensure consistency.


## Customization

Updates and adjustments can be made by editing:

- **Input**: `DOXYGEN_INPUT`, `DOXYGEN_FILE_PATTERNS` in `CMakeLists.txt`
- **Exclusions**: `DOXYGEN_EXCLUDE`, `DOXYGEN_EXCLUDE_PATTERNS` in `CMakeLists.txt`
- **Macros**: `DOXYGEN_PREDEFINED_MACROS`, `DOXYGEN_EXPAND_AS_DEFINED` in `CMakeLists.txt`
- **Doxygen pages**: `mainpage.dox.in`, `doctips.dox`, and `makingDocs.dox`
- **Web output**: HTML, CSS, JS and assets in `templates/`, and `FreecadDoxygenLayout.xml`


## Current Limitations

- Python API documentation is limited to what Doxygen can extract directly (Sphinx is not used currently).
- Cross-linking between C++ and Python APIs is incomplete.
- Targets do not automatically pick up changes in `*.pyi` and `*Py.xml` files. A rebuild is required.


## Planned Next Steps (not implemented yet)

- **Sphinx** as the main documentation generator
- **Breathe** to import Doxygen XML output into Sphinx
- Unified **C++ and Python API documentation**
- Integration into the main FreeCAD website

Additionally, everyone is welcome to contribute to better user and developer documentation content
alongside API references. See the [Developers Handbook](https://freecad.github.io/DevelopersHandbook),
namely its [C++ API documentation section](https://freecad.github.io/DevelopersHandbook/bestpractices/c++practices#api-documentation).


## Notes

- The `CONTRIBUTORS` file is embedded into the FreeCAD GUI resources.
- `legacy/` is retained for reference only and should not be extended.
- For advanced Doxygen options, refer to the official Doxygen documentation and inline comments in `BuildDoc.cfg.in`.
