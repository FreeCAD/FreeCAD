"""Debug script to see what selection returns for Link Array elements."""
import FreeCAD as App
from FreeCAD import Gui

# Listen to selection changes
class SelectionObserver:
    def addSelection(self, doc, obj, sub, pnt):
        print(f"Selection added:")
        print(f"  Document: {doc}")
        print(f"  Object: {obj}")
        print(f"  SubElement: {sub}")
        print(f"  Point: {pnt}")

        # Also check the full selection
        sel = Gui.Selection.getSelectionEx()
        if sel:
            for s in sel:
                print(f"  SelectionEx Object: {s.ObjectName}")
                print(f"  SelectionEx SubElementNames: {s.SubElementNames}")

obs = SelectionObserver()
Gui.Selection.addObserver(obs)
print("Selection observer installed. Now click on a Link Array element.")
print("To remove observer: Gui.Selection.removeObserver(obs)")
