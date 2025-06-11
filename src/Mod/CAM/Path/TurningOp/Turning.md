# FreeCAD Turning
The turning operations use liblathe.

Liblathe can be installed from PyPi:

```
pip install liblathe
```

or via the liblathe addon.


# TODO:
- Tool support
- Dressup support

# Done:
- Operation names should drop 'Path'
- Remove TurnHelpers or Rename to TurnHelpers
- GUI files should drop the Gui? This is common throughout the code to have Op and OpGui
- All operations import liblathe.base.op as LLP
- Check import orders Cam -> Turn