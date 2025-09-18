import os
from abc import ABC, abstractmethod
from typing import Any, Optional

import FreeCAD
from Path.Main.Sanity.Squawk import Squawk, SquawkType
import Path.Dressup.Utils as PathDressup

translate = FreeCAD.Qt.translate


class SanityRule(ABC):
    """Base class for all sanity check rules"""

    def __init__(self, name: str, description: str):
        self.name = name
        self.description = description

    @abstractmethod
    def check(self, job, context: dict[str, Any] | None = None) -> list[Optional[Squawk]]:
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


class ToolControllerZeroFeedrateRule(SanityRule):
    """Check if tool controllers have feedrates configured"""

    def __init__(self):
        super().__init__(
            "Tool Controller Feedrate",
            "Checks if tool controllers have feedrates configured",
        )

    def check(self, job, legacy_tcs) -> list[Optional[Squawk]]:
        squawks = []
        for tc in job.Tools.Group:

            if tc.ToolNumber in legacy_tcs:
                continue  # Skip legacy tools
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


class SpindleSpeedZeroFeedrateRule(SanityRule):
    """Check if tool controllers have feedrates configured"""

    def __init__(self):
        super().__init__(
            "Spindle Speed Zero Feedrate",
            "Checks if tool controllers have feedrates configured",
        )

    def check(self, job, legacy_tcs) -> list[Optional[Squawk]]:
        squawks = []
        for TC in job.Tools.Group:
            if TC.SpindleSpeed == 0.0:
                squawks.append(
                    Squawk(
                        operator="CAMSanity",
                        note=translate(
                            "CAM_Sanity", "Tool Controller '{}' has no spindlespeed"
                        ).format(TC.Label),
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

    def check(self, job) -> list[Optional[Squawk]]:
        squawks = []
        for TC in job.Tools.Group:
            if not hasattr(TC.Tool, "BitBody"):
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


class ToolUsedByMultipleToolsRule(SanityRule):
    """Check if a tool is used by multiple tool controllers"""

    def __init__(self):
        super().__init__(
            "Tool Used by Multiple Tools",
            "Checks if a tool is used by multiple tool controllers",
        )

    def check(self, job, legacy_tcs: list[int]) -> list[Optional[Squawk]]:
        squawks = []
        data = {"Data": []}
        for TC in job.Tools.Group:
            if TC.ToolNumber in legacy_tcs:
                continue  # Skip legacy tools
            tooldata = data.setdefault(str(TC.ToolNumber), {})
            bitshape = tooldata.setdefault("ShapeType", "")
            if bitshape not in ["", TC.Tool.ShapeType]:
                squawks.append(
                    Squawk(
                        "CAMSanity",
                        translate("CAM_Sanity", "Tool number {} used by multiple tools").format(
                            TC.ToolNumber
                        ),
                        squawk_type=SquawkType.CAUTION,
                    )
                )
        return squawks


class ToolBitShapeNotFoundRule(SanityRule):
    """Check if a tool bit shape is not found"""

    def __init__(self):
        super().__init__(
            "Tool Bit Shape Not Found",
            "Checks if a tool bit shape is not found",
        )

    def check(self, job, legacy_tcs) -> list[Optional[Squawk]]:
        squawks = []
        for TC in job.Tools.Group:
            if TC.ToolNumber in legacy_tcs:
                continue  # Skip legacy tools
            if os.path.isfile(TC.Tool.ShapeType):
                pass
            else:
                squawks.append(
                    Squawk(
                        "CAMSanity",
                        translate("CAM_Sanity", "Toolbit Shape for TC: {} not found").format(
                            TC.ToolNumber
                        ),
                        squawk_type=SquawkType.WARNING,
                    )
                )
        return squawks


class JobNotPostProcessedRule(SanityRule):
    """Check if the job has been post-processed"""

    def __init__(self):
        super().__init__(
            "Job Not Post Processed",
            "Checks if the job has been post-processed",
        )

    def check(self, job) -> list[Optional[Squawk]]:
        squawks = []
        if job.LastPostProcessOutput == "":
            squawks.append(
                Squawk(
                    "CAMSanity",
                    translate("CAM_Sanity", "The Job has not been post-processed"),
                )
            )
        return squawks


class PostProcessedFileMissingRule(SanityRule):
    """Check if the last post-processed file is missing"""

    def __init__(self):
        super().__init__(
            "Post Processed File Missing",
            "Checks if the last post-processed file is missing",
        )

    def check(self, job) -> list[Optional[Squawk]]:
        squawks = []
        if job.LastPostProcessOutput != "":
            if not os.path.isfile(job.LastPostProcessOutput):
                squawks.append(
                    Squawk(
                        "CAMSanity",
                        translate(
                            "CAM_Sanity",
                            "The Job's last post-processed file is missing",
                        ),
                    )
                )
        return squawks


class MaterialNotSpecifiedRule(SanityRule):
    """Check if the material is specified for the job"""

    def __init__(self):
        super().__init__(
            "Material Not Specified",
            "Checks if the material is specified for the job",
        )

    def check(self, job) -> list[Optional[Squawk]]:
        squawks = []
        if getattr(job.Stock, "ShapeMaterial", None) is None or getattr(
            job.Stock.ShapeMaterial, "Name", ""
        ) in ("", "Default"):
            squawks.append(
                Squawk(
                    operator="CAMSanity",
                    note=translate("CAM_Sanity", "Consider Specifying the Stock Material"),
                    squawk_type=SquawkType.TIP,
                )
            )
        return squawks


class ToolControllerNotUsedRule(SanityRule):
    """Check if a tool controller is not used"""

    def __init__(self):
        super().__init__(
            "Tool Controller Not Used",
            "Checks if a tool controller is not used",
        )

    def check(self, job, legacy_tcs) -> list[Optional[Squawk]]:
        squawks = []
        for TC in job.Tools.Group:
            if TC.ToolNumber in legacy_tcs:
                continue  # Skip legacy tools
            used = False
            for op in job.Operations.Group:
                base_op = PathDressup.baseOp(op)
                if hasattr(base_op, "ToolController") and base_op.ToolController is TC:
                    used = True
            if not used:
                squawks.append(
                    Squawk(
                        operator="CAMSanity",
                        note=translate("CAM_Sanity", "Tool number {} is not used").format(
                            TC.ToolNumber
                        ),
                        squawk_type=SquawkType.WARNING,
                    )
                )
        return squawks
