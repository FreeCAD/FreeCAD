.. SPDX-License-Identifier: LGPL-2.1-or-later

Sketch Notes
============

Overview
--------

Sketch notes are non-geometric annotations attached to a sketch while it is in
edit mode. They are intended as working notes for design intent, reminders, and
temporary documentation without affecting the sketch solver or downstream CAD
features.

Current behavior
----------------

Sketch notes currently have these properties:

* They are stored on the sketch object, not as geometry.
* They are shown only while the sketch is being edited.
* They stay screen-aligned and keep a fixed pixel size while zooming.
* They can be created with the ``New Note`` command.
* ``New Note`` uses click-to-place behavior.
* The note body is directly editable.
* The top-left handle moves the note.
* The lower-right handle resizes the note.
* Notes can be hidden or shown in edit mode.
* Notes are ignored by solving, shape generation, and sketch consumption by
  other features.

Data model
----------

The feature is implemented with sketch-owned properties on
``Sketcher::SketchObject``:

* ``NoteTexts``
* ``NotePositions``
* ``NoteSizes``

These properties are stored independently of ``Geometry`` and ``Constraints``.
This is the key design choice that keeps notes out of solver and topology code.

Implementation summary
----------------------

App layer
~~~~~~~~~

``src/Mod/Sketcher/App/SketchObject.*`` stores note text, sketch-space anchor
positions, and screen-space sizes.

GUI layer
~~~~~~~~~

``src/Mod/Sketcher/Gui/ViewProviderSketch.*`` creates and manages note widgets
as edit-mode overlays on top of the sketch viewer.

The widgets are plain Qt child widgets of the Sketcher viewport. Their anchor is
stored in sketch coordinates, but they are rendered in screen space.

Command layer
~~~~~~~~~~~~~

``src/Mod/Sketcher/Gui/CommandSketcherTools.cpp`` provides:

* ``Sketcher_AddNote``
* ``Sketcher_ShowNotes``
* ``Sketcher_HideNotes``
* the note dropdown group command used by the toolbar

Workbench integration
~~~~~~~~~~~~~~~~~~~~~

``src/Mod/Sketcher/Gui/Workbench.cpp`` adds note commands to the Sketch menu
and the Sketcher tools toolbar.

Layout behavior
---------------

Notes are screen-fixed overlays, so they do not scale with the sketch. This can
cause crowding when zooming into a dense sketch.

The current implementation uses a simple avoidance rule:

* project the visible sketch bounding box into screen coordinates
* if a note overlaps that box, push the note just outside the visible sketch
  area
* clamp the final note position to the viewport

This reduces note overlap with the active sketch, but it is not a full layout or
routing system. With many notes in a small area, notes may still crowd each
other.

Interaction model
-----------------

Creation
~~~~~~~~

Choose ``New Note``, then click in the sketch to place the note.

Editing
~~~~~~~

Click in the note body and type. ``Ctrl+Enter`` clears focus.

Moving
~~~~~~

Drag the small handle in the top-left corner of the note.

Resizing
~~~~~~~~

Drag the lower-right handle.

Visibility
~~~~~~~~~~

Use the note dropdown or the Sketch menu to hide or show notes while editing.

Known limitations
-----------------

* Notes are currently visible only in sketch edit mode.
* Notes are Qt overlays rather than Coin scene objects.
* Notes do not currently route around each other intelligently.
* Note visibility is view-provider state, not yet a persisted sketch property.
* There is no dedicated test coverage yet for note creation, movement, resize,
  or visibility toggling.

Why notes are not geometry
--------------------------

Treating notes as geometry would force them into:

* solver setup
* geometry persistence and indexing
* shape generation
* selection and subelement mapping

That would add complexity in the wrong place for a feature that should behave as
documentation only.

Keeping notes as sketch-owned properties with GUI overlays isolates the feature
from the core CAD model and makes the behavior much easier to reason about.
