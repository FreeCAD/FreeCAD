import _PartDesign   # TODO: where is this needed?
makeFilletArc = _PartDesign.makeFilletArc


if not "freecad" in __package__:
    import warnings
    warnings.warn("use 'from freecad import PartDesign'", FutureWarning, stacklevel=2)
