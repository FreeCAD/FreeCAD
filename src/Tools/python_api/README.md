# API deprecation inventory

This tool discovers structured API deprecations directly from FreeCAD source files.
Source declarations remain authoritative; JSON output is a deterministic index and
is not intended to be edited or committed.

Run it from the repository root:

```sh
python -m src.Tools.python_api check
python -m src.Tools.python_api manifest --output python-api-deprecations.json
python -m src.Tools.python_api list
python -m src.Tools.python_api list --remove-by 27.2
```

The scanner reads regular Python, binding `.pyi`, and `.module.pyi` files without
importing them. It also discovers C++ property aliases declared with:

```cpp
ADD_PROPERTY_DEPRECATED_ALIAS(Property, "OldName", "26.3", "27.2");
```

These produce the same lifecycle records as Python deprecations, with
`kind="property_alias"` and the canonical property name stored as `replacement`.

Structured lifecycle metadata is validated uniformly, including release syntax and
the requirement that the removal release is later than the deprecation release.
Normal `ADD_PROPERTY_ALIAS(...)` declarations are not deprecations and are not
included in the inventory.

The repository scan is covered by the `src/Tools` test suite. Git history, API
signature tracking, and cross-release storage are intentionally outside its scope.
