try:
    from PySide6.QtOpenGL import *
    from PySide6.QtOpenGLWidgets import QOpenGLWidget
    from PySide6.QtOpenGLWidgets import QOpenGLWidget as QGLWidget
except ImportError:
    from PySide2.QtOpenGL import *
    from PySide2.QtWidgets import QOpenGLWidget