# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``FreeCAD.Console`` helper module.

These functions accept either a single message object or an explicit
``(notifier, message)`` pair. The runtime converts both arguments through
``str()`` when needed, so the public stub keeps them intentionally broad.
"""

from __future__ import annotations

from typing import Literal, TypeAlias, overload

_MessageType: TypeAlias = Literal["Log", "Wrn", "Msg", "Err", "Critical", "Notification"]

# Console output
@overload
def PrintMessage(message: object, /) -> None:
    """Print a plain message using the default empty notifier."""
    ...

@overload
def PrintMessage(notifier: object, message: object, /) -> None:
    """Print a plain message with an explicit notifier prefix."""
    ...

@overload
def PrintLog(message: object, /) -> None:
    """Print a log message using the default empty notifier."""
    ...

@overload
def PrintLog(notifier: object, message: object, /) -> None:
    """Print a log message with an explicit notifier prefix."""
    ...

@overload
def PrintError(message: object, /) -> None:
    """Print an error using the default empty notifier."""
    ...

@overload
def PrintError(notifier: object, message: object, /) -> None:
    """Print an error with an explicit notifier prefix."""
    ...

@overload
def PrintDeveloperError(message: object, /) -> None:
    """Print a developer-only error using the default empty notifier."""
    ...

@overload
def PrintDeveloperError(notifier: object, message: object, /) -> None:
    """Print a developer-only error with an explicit notifier prefix."""
    ...

@overload
def PrintUserError(message: object, /) -> None:
    """Print a user-facing error using the default empty notifier."""
    ...

@overload
def PrintUserError(notifier: object, message: object, /) -> None:
    """Print a user-facing error with an explicit notifier prefix."""
    ...

@overload
def PrintTranslatedUserError(message: object, /) -> None:
    """Print a translated user-facing error using the default empty notifier."""
    ...

@overload
def PrintTranslatedUserError(notifier: object, message: object, /) -> None:
    """Print a translated user-facing error with an explicit notifier prefix."""
    ...

@overload
def PrintWarning(message: object, /) -> None:
    """Print a warning using the default empty notifier."""
    ...

@overload
def PrintWarning(notifier: object, message: object, /) -> None:
    """Print a warning with an explicit notifier prefix."""
    ...

@overload
def PrintDeveloperWarning(message: object, /) -> None:
    """Print a developer-only warning using the default empty notifier."""
    ...

@overload
def PrintDeveloperWarning(notifier: object, message: object, /) -> None:
    """Print a developer-only warning with an explicit notifier prefix."""
    ...

@overload
def PrintUserWarning(message: object, /) -> None:
    """Print a user-facing warning using the default empty notifier."""
    ...

@overload
def PrintUserWarning(notifier: object, message: object, /) -> None:
    """Print a user-facing warning with an explicit notifier prefix."""
    ...

@overload
def PrintTranslatedUserWarning(message: object, /) -> None:
    """Print a translated user-facing warning using the default empty notifier."""
    ...

@overload
def PrintTranslatedUserWarning(notifier: object, message: object, /) -> None:
    """Print a translated user-facing warning with an explicit notifier prefix."""
    ...

@overload
def PrintCritical(message: object, /) -> None:
    """Print a critical message using the default empty notifier."""
    ...

@overload
def PrintCritical(notifier: object, message: object, /) -> None:
    """Print a critical message with an explicit notifier prefix."""
    ...

@overload
def PrintNotification(message: object, /) -> None:
    """Print a notification using the default empty notifier."""
    ...

@overload
def PrintNotification(notifier: object, message: object, /) -> None:
    """Print a notification with an explicit notifier prefix."""
    ...

@overload
def PrintTranslatedNotification(message: object, /) -> None:
    """Print a translated notification using the default empty notifier."""
    ...

@overload
def PrintTranslatedNotification(notifier: object, message: object, /) -> None:
    """Print a translated notification with an explicit notifier prefix."""
    ...

# Observer status
def SetStatus(observer: str, type: _MessageType, status: bool, /) -> None:
    """Enable or disable one message category for a registered console observer."""
    ...

def GetStatus(observer: str, type: _MessageType, /) -> bool | None:
    """Return one message-category state, or None if the observer is unknown."""
    ...

def GetObservers() -> list[str]:
    """Return the observer names registered with the console singleton."""
    ...
