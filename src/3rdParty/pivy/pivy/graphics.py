"""Deprecated alias for pivy.interactive module.

This module provides backward compatibility for code using the old
'pivy.graphics' module name. The module has been renamed to 'interactive'
to better reflect its purpose.

Use 'from pivy import interactive' instead of 'from pivy import graphics'.

This module will be removed in a future version.
"""

import warnings

warnings.warn(
    "pivy.graphics is deprecated and will be removed in a future version. "
    "Use 'from pivy import interactive' instead.",
    DeprecationWarning,
    stacklevel=2
)

# Import everything from interactive
from .interactive import *
