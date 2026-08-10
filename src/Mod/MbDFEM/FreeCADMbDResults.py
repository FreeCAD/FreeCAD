# SPDX-License-Identifier: LGPL-2.1-or-later

"""Compatibility wrapper for C++ FreeCADMbD solved ASMT import."""

import MbDFEM


def import_results(assembly, filename):
    """Import solved ASMT result series for *assembly* and its parts."""
    return MbDFEM.importSolvedAsmt(assembly, str(filename))
