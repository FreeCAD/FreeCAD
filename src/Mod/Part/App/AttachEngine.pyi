# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from Base.BaseClass import BaseClass
from Base.Placement import Placement
from App.DocumentObject import DocumentObject
from typing import Final, Optional

@export(
    Include="Mod/Part/App/Attacher.h",
    Namespace="Attacher",
    Constructor=True,
    Delete=True,
)
class AttachEngine(BaseClass):
    """
    AttachEngine abstract class - the functionality of AttachableObject, but outside of DocumentObject

    Author: DeepSOIC (vv.titov@gmail.com)
    Licence: LGPL
    DeveloperDocu: AttachEngine abstract class
    """

    AttacherType: Final[str] = ""
    """Type of engine: 3d, plane, line, or point."""

    Mode: str = ""
    """Current attachment mode."""

    References: object = None
    """Current attachment mode."""

    AttachmentOffset: object = None
    """Current attachment mode."""

    Reverse: bool = False
    """If True, Z axis of attached placement is flipped. X axis is flipped in addition (CS has to remain right-handed)."""

    Parameter: float = 0.0
    """Value of parameter for some curve attachment modes. Range of 0..1 spans the length of the edge (parameter value can be outside of the range for curves that allow extrapolation."""

    CompleteModeList: Final[list] = []
    """List of all attachment modes of all AttachEngines. This is the list of modes in MapMode enum properties of AttachableObjects."""

    ImplementedModes: Final[list] = []
    """List of all attachment modes of all AttachEngines. This is the list of modes in MapMode enum properties of AttachableObjects."""

    CompleteRefTypeList: Final[list] = []
    """List of all reference shape types recognized by AttachEngine."""

    def getModeInfo(self, mode: str, /) -> dict:
        """
        getModeInfo(mode): returns supported reference combinations, user-friendly name, and so on.
        """
        ...

    def getRefTypeOfShape(self, shape: str, /) -> str:
        """
        getRefTypeOfShape(shape): returns shape type as interpreted by AttachEngine. Returns a string.
        """
        ...

    def isFittingRefType(self, type_shape: str, type_needed: str, /) -> bool:
        """
        isFittingRefType(type_shape, type_needed): tests if shape type, specified by type_shape (string), fits a type required by attachment mode type_needed (string). e.g. 'Circle' fits a requirement of 'Edge', and 'Curve' doesn't fit if a 'Circle' is required.
        """
        ...

    def downgradeRefType(self, type: str, /) -> str:
        """
        downgradeRefType(type): returns next more general type. E.g. downgradeType('Circle') yields 'Curve'.
        """
        ...

    def getRefTypeInfo(self, type: str, /) -> dict:
        """
        getRefTypeInfo(type): returns information (dict) on shape type. Keys:'UserFriendlyName', 'TypeIndex', 'Rank'. Rank is the number of times reftype can be downgraded, before it becomes 'Any'.
        """
        ...

    @constmethod
    def copy(self) -> "AttachEngine":
        """
        copy(): returns a new instance of AttachEngine.
        """
        ...

    @constmethod
    def calculateAttachedPlacement(self, orig_placement: Placement, /) -> Optional[Placement]:
        """
        calculateAttachedPlacement(orig_placement): returns result of attachment, based
        on current Mode, References, etc. AttachmentOffset is included.

        original_placement is the previous placement of the object being attached. It
        is used to preserve orientation for Translate attachment mode. For other modes,
        it is ignored.

        Returns the new placement. If not attached, returns None. If attachment fails,
        an exception is raised.
        """
        ...

    def suggestModes(self) -> dict:
        """
        suggestModes(): runs mode suggestion routine and returns a dictionary with
        results and supplementary information.

        Keys:
        'allApplicableModes': list of modes that can accept current references. Note
        that it is only a check by types, and does not guarantee the modes will
        actually work.

        'bestFitMode': mode that fits current references best. Note that the mode may
        not be valid for the set of references; check for if 'message' is 'OK'.

        'error': error message for when 'message' is 'UnexpectedError' or
        'LinkBroken'.

        'message': general result of suggestion. 'IncompatibleGeometry', 'NoModesFit':
        no modes accept current set of references; 'OK': some modes do accept current
        set of references (though it's not guarantted the modes will work - surrestor
        only checks for correct types); 'UnexpectedError': should never happen.

        'nextRefTypeHint': what more can be added to references to reach other modes
        ('reachableModes' provide more extended information on this)

        'reachableModes': a dict, where key is mode, and value is a list of sequences
        of references that can be added to fit that mode.

        'references_Types': a list of types of geometry linked by references (that's
        the input information for suggestor, actually).
        """
        ...

    def readParametersFromFeature(self, document_object: DocumentObject, /) -> None:
        """
        readParametersFromFeature(document_object): sets AttachEngine parameters (References, Mode, etc.) by reading out properties of AttachableObject-derived feature.
        """
        ...

    def writeParametersToFeature(self, document_object: DocumentObject, /) -> None:
        """
        writeParametersToFeature(document_object): updates properties of
        AttachableObject-derived feature with current AttachEngine parameters
        (References, Mode, etc.).

        Warning: if a feature linked by AttachEngine.References was deleted, this method
        will crash FreeCAD.
        """
        ...
