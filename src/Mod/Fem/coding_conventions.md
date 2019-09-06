# FEM coding_conventions
- These coding rules apply to FEM module code only. Other modules or the base system may use different coding rules especially in naming policy of Python.

## Python and C++
- All files should have a licence header
- Unix line endings are preferend
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

- www.lgtm.com
- TODO: check pylint
- automatic code formater will not be used for existant code
- for new code if someone would like to use a code formater black should be used

## C++
### Naming policy
- CamelCase names

## Spelling
- Take care on spelling. Quit often it is ignored to check for missspelling.
- codespelling could be used
~~~
codespell -q 3 -L aci,aline,alledges,als,ang,beginn,behaviour,bloaded,calculater,cancelled,cancelling,cas,cascade,centimetre,childs,colour,colours,currenty,doubleclick,dum,eiter,elemente,feld,freez,iff,indicies,initialisation,initialise,initialised,initialises,initialisiert,kilometre,lod,mantatory,methode,metres,millimetre,modell,nd,noe,normale,normaly,nto,numer,oder,ot,pres,programm,que,recurrance,rougly,seperator,serie,sinc,strack,substraction,te,thist,thru,tread,vertexes,uint,unter,whitespaces -S *.ts,*.po src/Mod/Fem
~~~
