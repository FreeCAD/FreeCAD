# Draft coding conventions

- These coding rules apply to the Draft Workbench.
  Other modules or the base system may use different coding guidelines.
- These coding rules are essentially the same well known PEP 8 guidelines
  that are recommended for all projects that use the Python programming
  language;
  for example, the [FEM Workbench](../Fem)
  uses basically these same conventions.
- Draft is an old workbench, and thus the rules described here
  are not followed strictly at the moment;
  nevertheless, we are working on improving the situation over time.
- New code added to the workbench should adhere to these guidelines.

## General

- All files should have the license header at the beginning.
- Unix line endings should be used.
- Never use mixed line endings in the same file.
- Use 4 spaces for a single level of indentation. Do not use tabs.
- Remove trailing white spaces.
- Keep files at less than 1000 lines in size.
  - Break a module into smaller modules, or into a package
    (a subdirectory with various modules),
    as necessary.
  - A big function can be broken into smaller functions.
  - A big class can be broken into smaller classes.
    Then one class can subclass the other ones
    in order to inherit their methods.

## Python

### Code formatting

- In general, code should follow [PEP 8](https://www.python.org/dev/peps/pep-0008/)
  and [PEP 257](https://www.python.org/dev/peps/pep-0257/) (docstrings).
- Maximum line length should be 80 characters.
  - Find ways to reduce nested blocks of code by using auxiliary variables,
    and functions that encapsulate the operations inside these blocks.
  - If you have more than 3 levels of indentation, this is a sign
    that the code should be refactored.
- Use double quotes for `"strings"`.

### Imports

- Imports should be at the beginning of the file,
  after the license header and module docstring,
  and they should be grouped in three types in order:
  standard library imports, third party imports,
  and then FreeCAD specific imports.
- Import only one module per line.
- Do not use asterisk imports, `from module import *`, as this
  makes it hard to validate imported functions and classes.
- Do not import modules inside a function or class.
- The import of modules that require the graphical interface,
  such as `FreeCADGui`, should be guarded by an `if FreeCAD.GuiUp:` test.
- In general, the code should be structured in such a way
  that console-only functions are separate from their graphical interface
  implementations (GuiCommands).

### Naming policy

- Follow [PEP 8](https://www.python.org/dev/peps/pep-0008/).
- `snake_case_names.py` for modules.
- `variable_names_without_capitals` for variables.
- `CamelCaseClass` for classes.
- `CONSTANTS_USE_CAPITALS` for constants.
- `functions_without_capitals()` for functions and class methods.
- Functions expected to return a value should indicate what is expected,
  so `is_mesh_valid` is a good name, but `check_mesh` is not a good name.
- Class names, method names, and variables that are auxiliary,
  and not meant to be part of the public interface should start
  with an underscore like `_MyInternalClass` or `_my_small_variable`.

### Python code formatting tools

- Using a code editor that automatically checks compliance with PEP 8
  is recommended.
- For example, [Spyder](https://www.spyder-ide.org/)
  contains code checkers `pylint`, `pyflakes`, `pycodestyle`, `jedi`,
  `sphinx`, etc., that automatically test the code as you write it,
  and can provide documentation and hints on the used functions and classes
  in real time, even the ones that you have just written.
- Compliance should be manually checked with [flake8](https://flake8.pycqa.org/en/latest/).
```bash
flake8 --ignore=E226,E266,W503 file.py
```

We may ignore certain errors and warnings.
- E226: spaces around arithmetic operators `*`, `/`, `+`, `-`;
  sometimes we don't need a space.
- E266: only one `#` for comments; we need two `##` for Doxygen documentation.
- W503: break lines before a binary operator like `and` and `or`.
  The W503 warning will be changed in the future so we can ignore it for now.
- See the meaning of the error codes in the
  [pycodestyle documentation](http://pycodestyle.pycqa.org/en/latest/intro.html).

A good way to test entire folders for compliance is to run
the following command.
```bash
find src/Mod/Draft -name '*.py' ! -name InitGui.py -exec flake8 --ignore=E226,E266,W503 --max-line-length=100 '{}' '+'
```

- The online [LGTM service](https://lgtm.com/projects/g/FreeCAD/FreeCAD/latest/files/src/Mod/Draft/)
  is also able to analyze the code and detect problems.
- Avoid automatic code formatters.

## C++

- Consider using [PEP 7](https://www.python.org/dev/peps/pep-0007/).
- In general, use the same naming style as with Python code.

## Spelling

- Take care of spelling mistakes in the default English messages,
  graphical user interfaces, documentation strings, and source code comments.
- Use [codespell](https://github.com/codespell-project/codespell)
  to find common errors.

The following command tests files in the Draft directory,
but skips translation files and certain words which could be custom defined
variables, functions, and classes.
```bash
codespell -q 3 -S '*.ts' -L beginn,childs,eiter,methode,programm src/Mod/Draft
```

Interactively fix the errors found.
```bash
codespell -i 3 -w -S '*.ts' -L beginn,childs,eiter,methode,programm src/Mod/Draft
```
