<!--
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2026 FreeCAD
SPDX-FileNotice: Part of the FreeCAD project.
-->

# BIM module coding conventions

- These coding conventions apply to the BIM module code.
  Other FreeCAD modules or the core/base system may use slightly different coding conventions.
  See the [FreeCAD Developers Handbook](https://freecad.github.io/DevelopersHandbook/) for more info.
- They are essentially the same well-known [PEP 8](https://www.python.org/dev/peps/pep-0008/)
  guidelines recommended for all Python projects.
  The [Draft module](../Draft) and [FEM module](../Fem) use similar conventions.
- New code added to the BIM module should adhere to these conventions.

> [!NOTE] The BIM module (previously called `Arch`) is old with lot of legacy code,
> thus the conventions below are currently not followed strictly.
> Contributions to make the code more coherent and compliant, and improve these conventions are most welcome!


## General

- All files should have the license header at the beginning with SPDX metadata.
  See examples on the [Developers Handbook](https://freecad.github.io/DevelopersHandbook/codeformatting/).
- Unix line endings should be used.
- Never use mixed line endings in the same file.
- Use 4 spaces for a single level of indentation. Do not use tabs.
- Remove trailing whitespaces.
- Keep files at less than 1000 lines in size:
  - Break a module into smaller ones, or into a package (a subdirectory with various modules).
  - Split a module into App and Gui ones (e.g. Object has data and geometry, Object Gui has view provider, task panel).
  - A big function should be broken into smaller functions.
  - A big class should be broken into smaller classes.
    Then one class can subclass the other ones in order to inherit their methods.


## Python

> [!NOTE] As of early 2026, the FreeCAD codebase supports Python version 3.10 to 3.13.
> Many advanced features introduced in these versions and even before are not yet used
> (e.g. type hinting, pattern matching, parallelism/concurrency, etc).

### Code formatting

- In general, code should follow PEP 8 and PEP 257 [docstrings](#source-code-documentation).
- Keep line length below 100 characters if possible with a maximum of 120 (current pylint limit).
  - Find ways to reduce nested blocks of code by using auxiliary variables,
    and functions that encapsulate the operations inside these blocks.
  - If you have more than 3 levels of indentation, this is a sign that the code should be refactored.
- Use double quotes for `"strings"`.

### Imports

- Imports should be at the beginning of the file after the license header and module docstring,
  and they should be grouped in three types in order:
  - Standard library imports
  - Third party imports
  - FreeCAD specific imports
- Import only one module per line.
- Sort each import group alphabetically.
- Do not use wildcard/asterisk imports, `from module import *`, as this
  makes it hard to validate imported functions and classes.
- In general, do not import modules inside a function or class (lazy import).
- The import of modules that require the graphical interface,
  such as `FreeCADGui`, should be guarded by an `if FreeCAD.GuiUp:` test.
- In general, the code should be structured in such a way that console-only functions
  are separate from their graphical interface implementations (GuiCommands).

### Naming

- Follow [PEP 8](https://www.python.org/dev/peps/pep-0008/):
  - `snake_case_names.py` for modules.
  - `variable_names_without_capitals` for variables.
  - `CamelCaseClass` for classes.
  - `CONSTANTS_USE_CAPITALS` for constants.
  - `functions_without_capitals()` for functions and class methods.
- Do not use one-letter variables (e.g. `f` for `face`, `b` for `base`) unless established convention (`i` for `index`)
  or short-lived scope. Use descriptive names for improved readability instead
- Functions expected to return a value should indicate what is expected,
  so `is_mesh_valid` is a good name, but `check_mesh` is not a good name.
- Class names, method names, and variables that are auxiliary,
  and not meant to be part of the public interface should start
  with an underscore like `_MyInternalClass` or `_my_small_variable`.

### Python code formatting tools

- See [here how to set up](https://freecad.github.io/DevelopersHandbook/gettingstarted/) `pre-commit`.
- Using a code editor that automatically checks compliance with PEP 8 is recommended.
- Compliance can be manually checked with [flake8](https://flake8.pycqa.org/en/latest/).

```bash
flake8 --ignore=E226,E266,W503 file.py
```


## Spelling

- Take care of spelling mistakes in the default English messages,
  graphical user interfaces, documentation strings, and source code comments.
- Use [codespell](https://github.com/codespell-project/codespell) to find common errors.

To find typos in the BIM module, but skips translation files
and certain words which could be custom defined variables, functions, and classes:

```bash
codespell -q 3 -S *.ts -S *.svg -L beginn,childs,dof,dum,eiter,freez,inout,methode,nd,normaly,programm,som,uint,vertexes src/Mod/BIM
```

Interactively fix the errors found:

```bash
codespell -i 3 -w -S *.ts -S *.svg -L beginn,childs,dof,dum,eiter,freez,inout,methode,nd,normaly,programm,som,uint,vertexes src/Mod/BIM
```


### Source code documentation

> [!NOTE] As of early 2026, the FreeCAD codebase does not yet follow a consistent source code documentation standard.
> Docstrings are the main approach currently used, in addition to sparse Doxygen comments.
> Contributions in the area are most welcome!

[Python style docstrings](https://peps.python.org/pep-0257/) are used at the module, class, function/method levels.
Inline `# comments` may be used sparingly for implementation details.
They contain a short one-line summary, a blank line, and a more elaborate description clarify functionality.
For methods part of the public API, optional sections based on [numpydoc](https://numpydoc.readthedocs.io/en/latest/format.html)
syntax are recommended:

```py
"""
Short summary.

More elaborate description clarify functionality.

Parameters
----------
Optional description of the function arguments, keywords and their respective types.

list : list of Part::FeaturePython, optional
    A list of objects to include. Defaults to None.
base : Part::FeaturePython, optional
    The base object. Defaults to None.
name : str, optional
    The name to assign. Defaults to None.

Attributes
----------
Optional description of the class attributes (class docstrings only).

x : float
    Description of attribute `x`.
y : int
    Description of attribute `y`.

Methods
-------
Optional description of the class methods (class docstrings only).

color(c='rgb')
    Represent the color.

Returns
-------
Optional description of the returned values and their types.

result : Part::FeaturePython
    The created object.

See Also
--------
Optional section referring to related code (e.g. other useful functions).

package.module.submodule.func_a :
    A somewhat long description of the useful related function.

Notes
----
Optional section providing additional info about the code (e.g. algorithm, references).

Examples
--------
Optional comment explaining the example below.

>>> import FreeCADGui as Gui
>>> obj = make_feature(name="MyFeature")
>>> Gui.ActiveDocument.getObject(obj.Name).Visibility
True
"""
```


## Icons and resources

- Command icons filenames use the same command name.
- Icons and resources use a compliant Creative Commons license (e.g. CC BY-SA-4.0).
