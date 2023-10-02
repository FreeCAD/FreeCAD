# FEM coding_conventions
These coding rules apply to FEM module code only. Other modules or the base system may use different coding rules especially in naming policy of Python.


## Spelling
- Be mindful of spelling. Spell checks are quite often neglected.
- Utilize [codespell](https://github.com/codespell-project/codespell) to discover and quickly correct spelling errors.  

  ```bash
  # Find typos
  codespell -q 2 -S *.ts -S *.dyn -S *.svg -L childs,dof,dum,freez,methode,nd,normaly,programm,som,uint,vertexes,inout  src/Mod/Fem/

  # Interactively fix said typos
  codespell -i 3 -w -S *.ts -S *.dyn -S *.svg -L childs,dof,dum,freez,methode,nd,normaly,programm,som,uint,vertexes,inout  src/Mod/Fem/
  ```

  **Notes:**  
  1) We recommend running the dev version as it uses the most up to date typo dictionaries.  
  2) To find the most amount of typos we recommend running a quick `pip install --upgrade`  
  See the [codespell docs](https://github.com/codespell-project/codespell#updating) for more info.


## Python and C++
### Code formatting
- All files should have a license header
- Unix line endings will be used:
    - a .gitattributes file has been added to ensure line endings of text files are LF
    - use `?w=1` in link address to suppress line ending changes on github
- never use mixed line endings on one file
- 4 Spaces for indent (in this file too ;-))
- no trailing white spaces


## Python
### Code formatting
- except W503 all Python code is pep8 compliant
- maximal line length is 100
- double quotes as string identifier

### Exceptione
- Do not use bare 'except'.
- Be more specific. If not possible use:
- Either use 'except Exception' or if really everything should be caught 'except BaseException'
- https://stackoverflow.com/a/18982772
- https://github.com/PyCQA/pycodestyle/issues/703

### Imports
- Only one import per line.
    - on import from 'some_package' import 'some_module'. There should only be one 'some_module' per line.
    - on import from 'some_module' import 'some_method'. There should only be one 'some_method' per line.
- Each import group should be sorted alphabetically.
- First the 'import some_module' ones, afterwards the 'from some_module import something'.
- First absolute path imports than relative path imports.
- On relative path imports first the one dot ones, afterwards the two dot ones.
- Star import should not be used at all (from some_module import *).
- Python imports should be grouped into groups:
    - Standard library imports
    - One empty line
    - Third-party imports
    - One empty line
    - non Gui FreeCAD specific imports
        - from module FreeCAD
        - One empty line
        - other modules, but not FEM
        - One empty line
        - FEM specific imports
            - absolute imports
            - One empty line
            - relative imports
        - One empty line
    - FreeCAD Gui imports: 
        - The import of Gui modules should be guarded by a 'if FreeCAD.GuiUp:'
        - On Gui only modules the guard is not needed
        - Same as above but without a empty line
        - Standard library imports
        - Third-party Gui imports
        - FreeCAD-specific imports from module FreeCADGui
        - other FreeCAD Gui imports
        - FEM Gui imports
- The above paragraphs highly reduces merge conflicts.

### Naming policy
- snake_case_names
- ClassNames, variable_names_without_capitals and CONSTANTS_USE_CAPITALS, functions_without_capitals
- Function expected to return a value should indicate what is expected, so is_mesh_valid is a good name, but check_mesh is not a good name
- Class names, method names and variable that are locally and not supposed to be used for scripting should start with underscore like _MyInternalClass

### Python code formatting tools
- **flake8** in source code directory on Linux shell
```bash
find src/Mod/Fem/ -name "*\.py" | xargs -I [] flake8 --ignore=E266,W503 --max-line-length=100 []
```
- [LGTM](https://lgtm.com/projects/g/FreeCAD/FreeCAD/latest/files/src/Mod/Fem/)
- TODO: check pylint
- Automatic code formatter will not be used for existent code
- For new code if someone would like to use a code formatter black should be used

### Coding
- print() vs. FreeCAD.Console.PrintMessage()
    - `FreeCAD.Console.PrintMessage()` or Log or Error should be used
    - `print()` should be used for debugging only
    - [forum topic](https://forum.freecad.org/viewtopic.php?f=10&t=39110) 
    - BTW: Console prints need a new line where as print does not need one
- type checking:
    - do not use hasattr(obj, "Proxy") and obj.Proxy.Type
    - use method is_of_typ(obj, "TypeString") from module femtool.femutils
- ActiveDocument
    - try to avoid the use of App.ActiveDocument or FreeCAD.ActiveDocument
    - instead try to use some_obj.Document instead
    - try to avoid the use of Gui.ActiveDocument or FreeCADGui.ActiveDocument
    - instead try to use some_obj.ViewObject.Document or some_view_obj.Document
    - activeDocument() is more robust than ActiveDocument
    - [forum topic](https://forum.freecad.org/viewtopic.php?f=10&t=44133)
    - FreeCAD Python console
        - in code examples which will be copied in FreeCAD Python console
        - it is common to use App.ActiveDocument.some_obj or method
    
### Documenting
Python style is preferred over Doxygen style
    - see `ccx` tools module in fem tools package
    - see [forum topic](https://forum.freecad.org/viewtopic.php?f=10&t=37094)

### Module structure
- task panels should go into a separate package too
    - according pep8 imports should be on module beginning
    - if task panel class in inside viewprovider module, the imports needed for task panel are module beginning too
    - might be some special plot module or what ever is needed
    - if this is not available the object can not even created
    - if task panel is separate the object can be createdh


## C++
### Naming policy
- CamelCase names

### Code formatting
- slashes
    - Do not use to many slashes in a row. This could cause trouble with Doxygen.
    - see [PR with comment](https://github.com/FreeCAD/FreeCAD/pull/2757#discussion_r355218913)


## Icons
### Naming plicy
- Command icons use the command name.
- see [forum topic](https://forum.freecad.org/viewtopic.php?f=18&t=43379)
