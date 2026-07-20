# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``FreeCAD`` application module.

This source-adjacent stub file carries the callable surface together with the
simple helper aliases, reexports, module globals, and typing-only support that
those signatures use.
"""

from __future__ import annotations

from collections.abc import Sequence
from typing import TYPE_CHECKING, Literal, TypeAlias, overload

from Base.Metadata import module
from . import Console as Console  # pylint: disable=no-name-in-module,unused-import
from . import Units as Units  # pylint: disable=no-name-in-module,unused-import

module(
    Name="FreeCAD",
    Namespace="App",
    Include="ApplicationPy.h",
    CallbackOwner="ApplicationPy",
    CallbackPrefix="s",
)

if TYPE_CHECKING:
    from Part import Feature as _PartFeature

_FileTypeModules: TypeAlias = dict[str, str | list[str] | None]
_LogLevelName: TypeAlias = Literal["Default", "Error", "Warning", "Message", "Log", "Trace"]
GuiUp: int
ActiveDocument: Document | None

# Parameter and configuration access
def ParamGet(path: str, /) -> ParameterGrp:
    """Return the parameter group rooted at one application preference path."""
    ...

def saveParameter(name: str = "User parameter", /) -> None:
    """Persist one named parameter tree to disk, defaulting to the user parameter set."""
    ...

def Version() -> list[str]:
    """Return the FreeCAD version components as strings."""
    ...

def ConfigGet(key: str, /) -> str:
    """Return one application configuration value by key."""
    ...

def ConfigSet(key: str, value: str, /) -> None:
    """Store one application configuration value by key."""
    ...

def ConfigDump() -> dict[str, str]:
    """Return the current flat application configuration mapping."""
    ...

# Import and export registration
def addImportType(extension: str, module: str, /) -> None:
    """Register one importer module for a file extension."""
    ...

def changeImportModule(extension: str, old_module: str, new_module: str, /) -> None:
    """Replace one importer module registration for a file extension."""
    ...

@overload
def getImportType() -> _FileTypeModules:
    """Return the full extension-to-module map for all registered importers."""
    ...

@overload
def getImportType(extension: str, /) -> list[str]:
    """Return the importer modules registered for one specific extension."""
    ...

def addExportType(extension: str, module: str, /) -> None:
    """Register one exporter module for a file extension."""
    ...

def addTranslatableExportType(description: str, extensions: list[str], module: str, /) -> None:
    """Register one exporter together with a translated file-dialog description.

    ``description`` should be registered with the translation system using the
    ``FileFormat`` context.
    """
    ...

def changeExportModule(extension: str, old_module: str, new_module: str, /) -> None:
    """Replace one exporter module registration for a file extension."""
    ...

@overload
def getExportType() -> _FileTypeModules:
    """Return the full extension-to-module map for all registered exporters."""
    ...

@overload
def getExportType(extension: str, /) -> list[str]:
    """Return the exporter modules registered for one specific extension."""
    ...

# Resource and user paths
def getResourceDir() -> str:
    """Return the root resource directory shipped with FreeCAD."""
    ...

def getLibraryDir() -> str:
    """Return the directory that contains FreeCAD shared libraries."""
    ...

def getTempPath() -> str:
    """Return the temporary directory used by FreeCAD."""
    ...

def getUserCachePath() -> str:
    """Return the user cache directory used by FreeCAD."""
    ...

def getUserConfigDir() -> str:
    """Return the user configuration directory."""
    ...

def getUserAppDataDir() -> str:
    """Return the user application-data directory."""
    ...

def getUserMacroDir(actual: bool = False, /) -> str:
    """Return the standard user macro directory, or the effective user-defined path."""
    ...

def getHelpDir() -> str:
    """Return the directory that contains bundled help resources."""
    ...

def getHomePath() -> str:
    """Return the current FreeCAD home directory."""
    ...

# Document lifecycle
def loadFile(path: str, doc: str = "", module: str = "", /) -> None:
    """Load one file by delegating to an importer module.

    When ``module`` is empty, FreeCAD chooses an importer from the file extension.
    If several importers match, the first registered importer is used; if none
    matches, an exception is raised.
    """
    ...

def open(name: str, hidden: bool = False, temporary: bool = False) -> Document:
    """Open a document file and return the created document; see ``openDocument``."""
    ...

def openDocument(name: str, hidden: bool = False, temporary: bool = False) -> Document:
    """Create a document, load an existing project file, and return it.

    ``hidden`` suppresses creation of the document 3D view. ``temporary`` hides
    the document in the tree view. An I/O exception is raised when the file does
    not exist or cannot be loaded. Non-file restore failures may leave the
    created document open for recovery.
    """
    ...

def newDocument(
    name: str | None = None,
    label: str | None = None,
    hidden: bool = False,
    temp: bool = False,
) -> Document:
    """Create and return a new document.

    ``name`` is the internal document name and is made unique automatically.
    ``label`` is the optional user-changeable label. ``hidden`` suppresses
    creation of the document 3D view. ``temp`` marks the document as temporary
    so it is not saved.
    """
    ...

def closeDocument(document: str | Document, /) -> None:
    """Close one document by name or object."""
    ...

def writeRecoverySnapshotToTransientDir(
    document: Document,
    /,
    *,
    compressed: bool = True,
    save_binary_brep: bool = True,
    save_thumbnail: bool = False,
) -> bool:
    """Write one recovery snapshot for a document into its transient directory."""
    ...

# Document queries and observers
def activeDocument() -> Document | None:
    """Return the current active document, if any."""
    ...

def setActiveDocument(name: str, /) -> None:
    """Make one named document the active document."""
    ...

def getDocument(name: str, /) -> Document:
    """Return one loaded document by name, raising if no such document exists."""
    ...

def listDocuments(sort: bool = False, /) -> dict[str, Document]:
    """Return the loaded documents keyed by name, optionally sorted by dependency order."""
    ...

def addDocumentObserver(observer: object, /) -> None:
    """Register one document observer object."""
    ...

def removeDocumentObserver(observer: object, /) -> None:
    """Unregister one document observer object."""
    ...

# Logging, dependency queries, and transactions
def setLogLevel(tag: str, level: _LogLevelName | int, /) -> None:
    """Set one named log channel to a numeric or named level."""
    ...

def getLogLevel(tag: str, /) -> int:
    """Return the numeric level of one named log channel."""
    ...

def checkLinkDepth(depth: int, /) -> int:
    """Clamp or validate one proposed link depth value."""
    ...

def getLinksTo(
    obj: DocumentObject | None = None,
    options: int = 0,
    maxCount: int = 0,
    /,
) -> tuple[DocumentObject, ...]:
    """Return objects that link to the given object.

    ``options & 1`` searches recursively. ``options & 2`` checks link arrays.
    ``maxCount`` limits the number of returned links.
    """
    ...

def getDependentObjects(
    obj: DocumentObject | Sequence[DocumentObject],
    options: int = 0,
    /,
) -> tuple[DocumentObject, ...]:
    """Return objects that depend on one object or object sequence.

    The result includes the input objects. ``options & 1`` sorts the result in
    topological order; ``options & 2`` excludes Link-type dependencies.
    """
    ...

def setActiveTransaction(name: str, persist: bool = False, /) -> int:
    """Start or select the active transaction and return its identifier.

    While active, document changes open transactions with this application-wide
    name and identifier. ``persist`` is kept for compatibility and has no effect.
    """
    ...

def getActiveTransaction() -> tuple[str, int] | None:
    """Return the current transaction name and identifier, if any."""
    ...

def closeActiveTransaction(abort: bool = False, id: int = 0, /) -> None:
    """Close or abort the current transaction."""
    ...

def isRestoring() -> bool:
    """Return whether FreeCAD is currently restoring document state."""
    ...

def checkAbort() -> None:
    """Raise ``Base.FreeCADAbort`` when the current operation was aborted.

    This only works while a sequencer or Python progress indicator is active,
    such as during document restore or recomputation. Users usually request the
    abort by pressing Esc.
    """
    ...
