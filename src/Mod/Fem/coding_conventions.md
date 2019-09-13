# FEM coding_conventions
- These coding rules apply to FEM module code only. Other modules or the base system may use different coding rules especially in naming policy of Python.


## Python and C++
- All files should have a license header
- Unix line endings are preferred
- never use mixed line endings on one file
- 4 Spaces for indent
- no trailing white spaces


## Python
#### Code formatting
- except W503 all Python code is pep8 compliant
- maximal line length is 100
- double quotes as string identifier
- One import per line, no * imports allowed as it makes harder to validate code
- the import of FreeCADGui should be guarded by a 'if FreeCAD.GuiUp:'

### Naming policy
- snake_case_names
- ClassNames, variable_names_without_capitals and CONSTANTS_USE_CAPITALS, functions_without_capitals
- Function expected to return a value should indicate what is expected, so is_mesh_valid is a good name, but check_mesh is not a good name
- Class names, methode names and variable that are locally and not supposed to be used for scripting should start with underscore like _MyInternalClass

### Python code formatting tools
- flake8
    - in source code directory on Linux shell
~~~
find src/Mod/Fem/ -name "*\.py" | grep -v InitGui.py | xargs -I [] flake8 --ignore=E266,W503 --max-line-length=100 []
~~~

- [LGTM](www.lgtm.com)
- TODO: check pylint
- automatic code formatter will not be used for existent code
- for new code if someone would like to use a code formatter black should be used

### Coding
- print() vs. FreeCAD.Console.PrintMessage()
    - FreeCAD.Console.PrintMessage() or Log or Error should be used
    - print() should be used for debugging only
    - forum topic https://forum.freecadweb.org/viewtopic.php?f=10&t=39110
    - BTW: Console prints need a new line where as print does not need one


## C++
### Naming policy
- CamelCase names

## Spelling
- Be mindful of spelling. Spell checks are quite often neglected.
- [codespell]((https://github.com/codespell-project/codespell#updating) could be used  

~~~
codespell -q 2 -S *.ts  -L childs,dof,dum,methode,nd,normaly,uint,vertexes,freez  src/Mod/Fem/
~~~
