# Python API deprecation inventory

This tool discovers Python API deprecations directly from FreeCAD source files. The
source decorators remain authoritative; JSON output is a deterministic index and is
not intended to be edited or committed.

Run it from the repository root:

```sh
python -m src.Tools.python_api check
python -m src.Tools.python_api manifest --output python-api-deprecations.json
python -m src.Tools.python_api list
python -m src.Tools.python_api list --remove-by 27.2
```

The scanner reads regular Python, binding `.pyi`, and `.module.pyi` files without
importing them. Binding classes with public names that differ from their native
wrapper names must declare `@export(PythonName="...")`. Structured lifecycle metadata is
validated. Positional PEP 702 decorators, string-valued `deprecated_attributes`
metadata, and docstring-only deprecations are rejected as errors.

The repository scan is covered by the `src/Tools` test suite. Git history, API
signature tracking, and cross-release storage are intentionally outside its scope.
