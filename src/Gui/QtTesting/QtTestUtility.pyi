# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.BaseClass import BaseClass
from typing import Final

@export(
    Include="Gui/QtTesting/QtTestUtility.h",
    Namespace="QtTesting",
    Constructor=False,
    Delete=False,
)
class QtTestUtility(BaseClass):
    """
    Interface to the QtTestUtility.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    @staticmethod
    def play(file: str | List[str]) -> Bool:
        """
        Playback a previously recorded test file or list of .xml files.

        The library uses the file extension to match the player. As a result files without
        an explicit .xml extension will not be played.

        Returns True if the tests were successful
        """

    @staticmethod
    def playingTest() -> Bool:
        """
        Indicates if the utility is currently playing a test
        """

    @staticmethod
    def stopTests() -> None:
        """
        Stop test playback
        """

    @staticmethod
    def record(filename: str | None = None) -> None:
        """
        Open the recorder interface and save events.

        If no file is specified a file browser will be opened. Otherwise the events
        are saved in the specified .xml file.

        The library uses the file extension to match the recorder. As a result files without
        an explicit .xml extension will not be recorded.
        """

    @staticmethod
    def stopRecording() -> None:
        """
        Stop recording.
        """

    @staticmethod
    def pauseRecording() -> None:
        """
        Pause recording.
        """

    @staticmethod
    def resumeRecording() -> None:
        """
        Resume recording after a pause.
        """
