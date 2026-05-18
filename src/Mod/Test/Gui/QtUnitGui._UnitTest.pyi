# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public method signatures for the ``QtUnitGui._UnitTest`` PyCXX type."""

from __future__ import annotations

class _UnitTest:
    """GUI wrapper for the Qt unit-test runner panel."""

    def clearErrorList(self) -> None:
        """Clear the displayed error list."""
        ...

    def insertError(self, failure: str, details: str, /) -> None:
        """Add one failure entry to the error list."""
        ...

    def setUnitTest(self, unit: str, /) -> None:
        """Set the active unit test name."""
        ...

    def getUnitTest(self) -> str:
        """Return the active unit test name."""
        ...

    def setStatusText(self, text: str, /) -> None:
        """Set the status text shown by the runner."""
        ...

    def setProgressFraction(self, fraction: float, color: str = "", /) -> None:
        """Update the progress indicator fraction."""
        ...

    def errorDialog(self, title: str, message: str, /) -> None:
        """Show one error dialog."""
        ...

    def setRunCount(self, count: int, /) -> None:
        """Set the total run count."""
        ...

    def setFailCount(self, count: int, /) -> None:
        """Set the failure count."""
        ...

    def setErrorCount(self, count: int, /) -> None:
        """Set the error count."""
        ...

    def setRemainCount(self, count: int, /) -> None:
        """Set the remaining-test count."""
        ...

    def updateGUI(self) -> None:
        """Refresh the runner GUI."""
        ...

    def addUnitTest(self, unit: str, /) -> None:
        """Add one unit test to the runner list."""
        ...

    def clearUnitTests(self) -> None:
        """Clear the registered unit tests."""
        ...
