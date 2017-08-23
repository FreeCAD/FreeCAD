try:
    # if FreeCADInit.py was called allready, we can import FreeCAD directly
    # as the sys.path is setup
    import FreeCAD as App
    import FreeCADGui as Gui
except ImportError:
    # if we want to import from freecad without starting the gui,
    # we first have to find the FreeCAD (shared-library)
    # the location of this file was setup with cmake.
    import sys
    __path__.append("@CMAKE_INSTALL_LIBDIR@")
    from .import FreeCAD as App
    from .import FreeCADGui as Gui

__path__.append(App.getHomePath() + 'Mod')
