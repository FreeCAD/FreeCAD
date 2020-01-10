# FEM coding_conventions
These coding rules apply to FEM module code only. Other modules or the base system may use different coding rules especially in naming policy of Python.

## Spelling
- Be mindful of spelling. Spell checks are quite often neglected.
- Utilize [codespell](https://github.com/codespell-project/codespell) to discover and quickly correct spelling errors.  

  ```bash
  # Find typos
  codespell -q 2 -S *.ts -L childs,dof,dum,methode,nd,normaly,uint,vertexes,freez src/Mod/Fem/
  # Interactively fix said typos
  codespell -i 3 -w -S *.ts -L childs,dof,dum,methode,nd,normaly,uint,vertexes,freez src/Mod/Fem/
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
- 4 Spaces for indent
- no trailing white spaces


## Python
### Code formatting
- except W503 all Python code is pep8 compliant
- maximal line length is 100
- double quotes as string identifier
- One import per line, no * imports allowed as it makes harder to validate code
- the import of FreeCADGui should be guarded by a 'if FreeCAD.GuiUp:'

### Naming policy
- snake_case_names
- ClassNames, variable_names_without_capitals and CONSTANTS_USE_CAPITALS, functions_without_capitals
- Function expected to return a value should indicate what is expected, so is_mesh_valid is a good name, but check_mesh is not a good name
- Class names, method names and variable that are locally and not supposed to be used for scripting should start with underscore like _MyInternalClass

### Python code formatting tools
- **flake8** in source code directory on Linux shell
```bash
find src/Mod/Fem/ -name "*\.py" | grep -v InitGui.py | xargs -I [] flake8 --ignore=E266,W503 --max-line-length=100 []
```
- [LGTM](https://lgtm.com/projects/g/FreeCAD/FreeCAD/latest/files/src/Mod/Fem/)
- TODO: check pylint
- Automatic code formatter will not be used for existent code
- For new code if someone would like to use a code formatter black should be used

### Coding
- print() vs. FreeCAD.Console.PrintMessage()
  - `FreeCAD.Console.PrintMessage()` or Log or Error should be used
  - `print()` should be used for debugging only
  - [forum topic](https://forum.freecadweb.org/viewtopic.php?f=10&t=39110) 
  - BTW: Console prints need a new line where as print does not need one

### Documenting
Python style is preferred over Doxygen style
  - see `ccx` tools module in fem tools package
  - see [forum topic](https://forum.freecadweb.org/viewtopic.php?f=10&t=37094)

## C++
### Naming policy
- CamelCase names

### Code formatting
- slashes
    - Do not use to many slashes in a row. This could cause trouble with Doxygen.
    - see [PR with comment](https://github.com/FreeCAD/FreeCAD/pull/2757#discussion_r355218913)
