from _Part import *
from _Part import __sortEdges__

if not "freecad" in __package__:
    import warnings
    warnings.warn("use 'from freecad import Part'", FutureWarning, stacklevel=2)
