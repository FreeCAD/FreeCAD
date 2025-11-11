# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Metadata import export
from Part.App.ShapeFix.ShapeFix_Root import ShapeFix_Root

@export(
    PythonName="Part.ShapeFix.Shell",
    Twin="ShapeFix_Shell",
    TwinPointer="ShapeFix_Shell",
    Include="ShapeFix_Shell.hxx",
    FatherInclude="Mod/Part/App/ShapeFix/ShapeFix_RootPy.h",
    Constructor=True,
)
class ShapeFix_Shell(ShapeFix_Root):
    """
    Root class for fixing operations

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    FixOrientationMode: bool = ...
    """Mode for applying fixes of orientation of faces"""

    FixFaceMode: bool = ...
    """Mode for applying fixes using ShapeFix_Face"""

    def init(self) -> None:
        """
        Initializes by shell
        """
        ...

    def fixFaceTool(self) -> None:
        """
        Returns tool for fixing faces
        """
        ...

    def perform(self) -> None:
        """
        Iterates on subshapes and performs fixes
        """
        ...

    def shell(self) -> None:
        """
        Returns fixed shell (or subset of oriented faces)
        """
        ...

    def numberOfShells(self) -> None:
        """
        Returns the number of obtained shells
        """
        ...

    def shape(self) -> None:
        """
        In case of multiconnexity returns compound of fixed shells and one shell otherwise
        """
        ...

    def errorFaces(self) -> None:
        """
        Returns not oriented subset of faces
        """
        ...

    def fixFaceOrientation(self) -> None:
        """
        Fixes orientation of faces in shell.
        Changes orientation of face in the shell, if it is oriented opposite
        to neighbouring faces. If it is not possible to orient all faces in the
        shell (like in case of mebious band), this method orients only subset
        of faces. Other faces are stored in Error compound.
        Modes :
        isAccountMultiConex - mode for account cases of multiconnexity.
        If this mode is equal to Standard_True, separate shells will be created
        in the cases of multiconnexity. If this mode is equal to Standard_False,
        one shell will be created without account of multiconnexity. By default - Standard_True;
        NonManifold - mode for creation of non-manifold shells.
        If this mode is equal to Standard_True one non-manifold will be created from shell
        contains multishared edges. Else if this mode is equal to Standard_False only
        manifold shells will be created. By default - Standard_False.
        """
        ...

    def setNonManifoldFlag(self) -> None:
        """
        Sets NonManifold flag
        """
        ...
