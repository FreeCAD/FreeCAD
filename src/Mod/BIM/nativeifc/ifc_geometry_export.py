# SPDX-License-Identifier: LGPL-2.1-or-later

"""FreeCAD geometry to IFC conversion boundary used by NativeIFC."""

from importers import exportIFC

from . import backend


class GeometryExporter:
    """Own isolated legacy-converter state for one IFC file operation."""

    def __init__(self, ifcfile):
        self.ifcfile = ifcfile
        self.state = exportIFC.create_export_state(ifcfile, backend.get_backend())

    def create_representation(self, context, obj, preferences):
        """Convert one FreeCAD object's geometry into an IFC representation."""

        representation, placement, _shape_type = exportIFC.getRepresentation(
            self.ifcfile,
            context,
            obj,
            preferences=preferences,
            export_state=self.state,
        )
        return representation, placement

    def create_annotation(self, context, obj, history, preferences):
        """Convert one FreeCAD annotation without touching exporter globals."""

        return exportIFC.create_annotation(
            obj,
            self.ifcfile,
            context,
            history,
            preferences,
            export_state=self.state,
        )
