##  How to configure VS code to work with FreeCAD workbenches
  
### Make python intellisense work with freecad
  
1. rename `template-settings.json` to `settings.json`  
2. change `<FreeCAD-Path>' to the directory where the FreeCAD binary is installed.  
  
### Make pylint work with freecad
Either copy `.pylintrc` to the root directory of the workbench  
or:  
In the root directory execute: `python -m pylint --generate-rcfile > .pylintrc`  
Edit `.pylintrc` and under `[Main]` set `extension-pkg-allow-list=` to: `extension-pkg-allow-list=FreeCAD,FreeCADGui`  

