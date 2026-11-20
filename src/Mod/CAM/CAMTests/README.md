# CAM tests

This directory contains the tests of the CAM workbench. The tests use `unittest`.

## Run the tests

Run all CAM tests from the build directory:

```
bin/FreeCADCmd -t TestCAMApp
```

Run the tests that need the GUI:

```
bin/FreeCAD -t TestCAMGui
```

Run one test class or one test:

```
bin/FreeCADCmd -t CAMTests.TestLinuxCNCPost
bin/FreeCADCmd -t CAMTests.TestLinuxCNCPost.test_blend_mode_exact_path
```

`CAMTests` puts each test class in the package namespace. Thus the name after `CAMTests.` is the class name, not the file name. `TestCAMApp.<class>` also works.

To run all classes of a file that has no class with the file name, use the file name after `CAMTests.`:

```
bin/FreeCADCmd -t CAMTests.TestPostContract
```

## Add a test file

1. Name the file `Test<Subject>.py` and put it in this directory.
2. Add the file to `Tests_SRCS` in `src/Mod/CAM/CMakeLists.txt`.
3. Run `cmake` again.

CMake writes the list of test modules from `Tests_SRCS`. A file that is not in `Tests_SRCS` does not run, and no error shows. Do not add the file to `TestCAMApp.py`.

A test that needs the GUI runs from `TestCAMGui.py`. Import its test classes there, and add its module to `_EXCLUDE_MODULES` in `__init__.py`.

## Post-processor tests

There are two kinds of post. A post is one kind or the other, and its tests use only the entry of that kind.

| Kind of post | How to identify it | Entry that a test calls |
|---|---|---|
| Machine-based | The post file declares `POST_TYPE = "machine"`. The Job has a Machine. | `export2()` |
| Legacy | The post file has no `POST_TYPE`. The post reads `PostProcessorArgs`. | `export()` |

Write new tests for machine-based posts. Do not add `export()` tests for a machine-based post: `export()` does not run the code that a Job with a Machine uses.

### Where a test goes

| Subject | Module |
|---|---|
| Output of one machine-based post | `Test<Post>Post.py`, on `MachinePostTestBase` |
| Behaviour that all machine-based posts share | `TestPostOutput.py`, class `TestExport2Integration` |
| Behaviour that each machine-based post must have | `TestPostContract.py` (no test code is necessary for a new post) |
| Legacy posts | `TestTestPost.py`, `TestPostGCodes.py`, `TestPostMCodes.py`, and the legacy test file of the post |

`TestPostContract.py` finds each machine-based post and makes one test class for it, `TestPostContract_<post>`. The pipeline checks run on the base `PostProcessor`, and on a post only if it replaces a pipeline method of the base class.

### Shared support

`PostTestSupport.py` contains the shared support. Use it. Do not copy a machine or a helper into a test file.

| Name | Use |
|---|---|
| `make_machine()` | A three-axis machine with one toolhead. `quiet=True` removes the header, the comments and the tool change. |
| `make_rotary_machine()` | A table-table machine: `"AC"` or `"BC"`. |
| `run_export2()` | Runs `export2()` for a job on a machine. |
| `MachinePostTestBase` | Base class for the tests of one machine-based post on a mock job. |
| `LegacyPostTestBase` | Base class for the tests of a legacy post on the `boxtest.fcstd` Job, through `export()`. |

A test class for a machine-based post:

```python
class TestMyPost(PostTestSupport.MachinePostTestBase):
    POST_NAME = "my"
    MACHINE_NAME = "Test My Machine"

    def configure_machine(self, machine):
        machine.postprocessor_properties["my_property"] = True

    def test_rapid_move(self):
        lines = self.lines(["G0 X1 Y2 Z3"])
        self.assert_line("G0 X1.000 Y2.000 Z3.000", lines)
```

`PostTestMocks.py` contains the mock Job, operation and Tool Controller. Use a real Job only when the test needs a document.
