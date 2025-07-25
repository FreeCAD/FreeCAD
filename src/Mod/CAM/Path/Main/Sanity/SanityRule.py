import FreeCAD
from abc import ABC, abstractmethod
from typing import List, Dict, Any
from Path.Main.Sanity.Squawk import Squawk, SquawkType

translate = FreeCAD.Qt.translate


class SanityRule(ABC):
    """Base class for all sanity check rules"""

    def __init__(self, name: str, description: str):
        self.name = name
        self.description = description

    @abstractmethod
    def check(self, job, context: dict[str, Any] | None = None) -> List[Squawk]:
        """
        Execute the rule against the given job and context

        Args:
            job: The CAM job to validate
            context: Additional data needed for validation

        Returns:
            List[Squawk]: List of issues found (empty if none)
        """
        pass

    def __str__(self):
        return f"{self.name}: {self.description}"


class ToolControllerFeedrateRule(SanityRule):
    """Check if tool controllers have feedrates configured"""

    def __init__(self):
        super().__init__(
            "Tool Controller Feedrate",
            "Checks if tool controllers have feedrates configured",
        )

    def check(self, job, context: Dict[str, Any]) -> List[Squawk]:
        squawks = []
        for tc in job.Tools.Group:
            if tc.HorizFeed.Value == 0.0:
                squawks.append(
                    Squawk(
                        operator="CAMSanity",
                        note=translate("CAM_Sanity", "Tool Controller '{}' has no feedrate").format(
                            tc.Label
                        ),
                        squawk_type=SquawkType.WARNING,
                    )
                )
        return squawks


class LegacyToolsRule(SanityRule):
    """Check if tool controllers have feedrates configured"""

    def __init__(self):
        super().__init__(
            "Tool Controller Feedrate",
            "Checks if tool controllers have feedrates configured",
        )

    def check(self, job, context: dict[str, Any] | None = None) -> List[Squawk]:
        squawks = []
        for TC in job.Tools.Group:
            if TC.HorizFeed.Value == 0.0:
                squawks.append(
                    Squawk(
                        operator="CAMSanity",
                        note=translate(
                            "CAM_Sanity",
                            "Tool number {} is a legacy tool. Legacy tools not \
                    supported by Path-Sanity",
                        ).format(TC.ToolNumber),
                        squawk_type=SquawkType.WARNING,
                    )
                )
        return squawks
