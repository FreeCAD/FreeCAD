# this directory is the entry point for extension modules. Any package
# installed to a directory "freecad" which is in sys.path can be imported
# with "from freecad import package" and is checked for a init_gui.py 
# module, which is import at startup. (FreeCADGuiInit.py)

add_subdirectory(freecad)