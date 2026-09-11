# SPDX-License-Identifier: LGPL-2.1-or-later
try:
    from . import TestThinExtrudeFeature as _shared
except ImportError:
    import TestThinExtrudeFeature as _shared


class TestRib(_shared.TestThinExtrudeFeature):
    """The dedicated adapter must preserve every shared-engine behavior."""

    featureType = "PartDesign::Rib"
