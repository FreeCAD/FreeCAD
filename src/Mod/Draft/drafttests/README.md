# General

2020 May

These files provide the unit tests classes based on the standard Python
`unittest` module.

These files should be imported from the main `TestDraft.py`
and `TestDraftGui.py` modules which are registered in the program
in `Init.py` and `InitGui.py` depending on if they require
the graphical interface or not.

Each module should define a class derived from `unittest.TestCase`,
and the individual methods of the class correspond to the individual
unit tests which try a specific function in the workbench.

The tests should be callable from the terminal.
```bash
# All tests that don't require the graphical interface
program --console -t TestDraft

# Only creation tests
program --console -t drafttests.test_creation

# A specific test inside a module and class
program --console -t drafttests.test_creation.DraftCreation.test_line
```

Where `program` is the name of the FreeCAD executable.

Most tests should be designed to pass even without the graphical interface,
meaning that they should run in console mode.

The exception to this are particular tests that explicitly use
the graphical interface.
```bash
# All tests that require the graphical interface
program -t TestDraftGui
```

For more information see the thread:
[New unit tests for Draft Workbench](https://forum.freecad.org/viewtopic.php?f=23&t=40405)

# To do

Not every single function in the workbench is tested, so new unit tests
can be written. This will improve reliability of the workbench,
making it easier to discover bugs and regressions.

See the individual modules to check what is missing.

In particular, unit tests for the import and export modules (SVG, DXF, DWG)
are required.
