from PySide.QtCore import QT_TRANSLATE_NOOP

from Path.Op.Base import ObjectOp


class DressupBase:
    """
    Base class for all dressups to provide common interface with the rest of CAM
    One major example is making sure all dressups export base operation settings
    like coolant, tool controller, etc.
    """

    def setup_coolant_property(self, obj):
        if not hasattr(obj, "CoolantMode"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "CoolantMode",
                "CoolantMode",
                QT_TRANSLATE_NOOP("App::Property", "Default coolant mode."),
            )

            for n in ObjectOp.opPropertyEnumerations():
                if n[0] == "CoolantMode":
                    setattr(obj, n[0], n[1])

    def setup_tool_controller_property(self, obj):
        if not hasattr(obj, "ToolController"):
            obj.addProperty(
                "App::PropertyLink",
                "ToolController",
                "Path",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The tool controller that will be used to calculate the path",
                ),
            )

    def __init__(self, obj, base):

        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "The base path to modify"),
        )

        obj.addProperty(
            "App::PropertyBool",
            "Active",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "Make False, to prevent operation from generating code"
            ),
        )

        self.setup_coolant_property(obj)
        self.setup_tool_controller_property(obj)

    def onDocumentRestored(self, obj):
        """
        Called then document is being restored. Often used for object migrations,
        adding missing properties, etc.
        Do not overwrite - child classes should use dressupOnDocumentRestored().
        """
        self.setup_coolant_property(obj)
        self.setup_tool_controller_property(obj)

    def dressupOnDocumentRestored(self, obj):
        """Overwrite this method for custom handling."""
        pass

    def execute(self, obj):
        """
        Export common properties from base object and
        run dressupExecute()
        """

        if hasattr(obj, "Base") and hasattr(obj.Base, "CoolantMode"):
            obj.CoolantMode = obj.Base.CoolantMode

        if hasattr(obj, "Base") and hasattr(obj.Base, "ToolController"):
            obj.ToolController = obj.Base.ToolController

        return self.dressupExecute(obj)

    def dressupExecute(self, obj):
        """
        Called whenever receiver should be recalculated.
        Should be overwritten by subclasses.
        """
        pass
