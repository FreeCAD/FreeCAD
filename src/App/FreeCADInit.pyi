# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typing additions for the dynamically installed ``FCADLogger`` class."""

from collections.abc import Callable


class FCADLogger:
    """Additional signatures for methods assigned by ``FreeCADInit.py``."""

    def error(self, msg: str, *args: object, **kwargs: object) -> None:
        """Log an error-level message."""
        ...
    def warn(self, msg: str, *args: object, **kwargs: object) -> None:
        """Log a warning-level message."""
        ...

    def msg(self, msg: str, *args: object, **kwargs: object) -> None:
        """Log a message-level message."""
        ...

    def log(self, msg: str, *args: object, **kwargs: object) -> None:
        """Log a log-level message."""
        ...

    def trace(self, msg: str, *args: object, **kwargs: object) -> None:
        """Log a trace-level message."""
        ...

    def info(self, msg: str, *args: object, **kwargs: object) -> None:
        """Log an informational message."""
        ...

    def debug(self, msg: str, *args: object, **kwargs: object) -> None:
        """Log a debug-level message."""
        ...

    def catch(
        self,
        msg: str,
        func: Callable[..., object],
        *args: object,
        **kwargs: object,
    ) -> object | None:
        """Call *func* and catch exceptions at the error level."""
        ...

    def catchWarn(
        self,
        msg: str,
        func: Callable[..., object],
        *args: object,
        **kwargs: object,
    ) -> object | None:
        """Call *func* and catch exceptions at the warning level."""
        ...

    def catchMsg(
        self,
        msg: str,
        func: Callable[..., object],
        *args: object,
        **kwargs: object,
    ) -> object | None:
        """Call *func* and catch exceptions at the message level."""
        ...

    def catchLog(
        self,
        msg: str,
        func: Callable[..., object],
        *args: object,
        **kwargs: object,
    ) -> object | None:
        """Call *func* and catch exceptions at the log level."""
        ...

    def catchTrace(
        self,
        msg: str,
        func: Callable[..., object],
        *args: object,
        **kwargs: object,
    ) -> object | None:
        """Call *func* and catch exceptions at the trace level."""
        ...

    def catchInfo(
        self,
        msg: str,
        func: Callable[..., object],
        *args: object,
        **kwargs: object,
    ) -> object | None:
        """Call *func* and catch exceptions at the message level."""
        ...

    def catchDebug(
        self,
        msg: str,
        func: Callable[..., object],
        *args: object,
        **kwargs: object,
    ) -> object | None:
        """Call *func* and catch exceptions at the log level."""
        ...
