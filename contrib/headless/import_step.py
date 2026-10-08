# SPDX-License-Identifier: LGPL-2.1-or-later
"""Bring a STEP solid/compound into an editable FreeCAD document.

Imports geometry, not the originating application's feature history.
"""

from pathlib import Path

import FreeCAD as App
import Import
import Part


def build(params):
    source = Path(params["source"]).resolve(strict=True)
    doc = App.newDocument("ImportedModel")
    Import.insert(str(source), doc.Name, merge=False, useLinkGroup=False, mode=0)
    doc.recompute()
    roots = [item for item in doc.RootObjects if not Part.getShape(item).isNull()]
    if not roots:
        raise ValueError("STEP file contains no shape")
    if len(roots) == 1:
        result = roots[0]
    else:
        result = doc.addObject("App::Part", "ImportedAssembly")
        result.Label = source.stem
        result.Group = roots
    result.addProperty("App::PropertyBool", "HeadlessAssembly", "Import")
    result.HeadlessAssembly = True
    return result
