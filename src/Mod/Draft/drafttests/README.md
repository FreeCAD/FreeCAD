2020 February

These files provide the unit tests classes based on the standard Python
`unittest` module.

These files should be imported from the main `TestDraft.py` module
which is the one registered in the program in `InitGui.py`.

Each module should define a class derived from `unittest.TestCase`,
and the individual methods of the class correspond to the individual
unit tests which try a specific function in the workbench.

The tests should be callable from the terminal.
```
program -t TestDraft  # all tests
program -t drafttests.test_creation  # only creation functions

# A specific test
program -t drafttests.test_creation.DraftCreation.test_line
```

For more information see the thread:
[New unit tests for Draft Workbench](https://forum.freecadweb.org/viewtopic.php?f=23&t=40405)
