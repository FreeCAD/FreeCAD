# TrutnaCAD feature log

TrutnaCAD is FreeCAD reworked to feel like Autodesk Fusion. This file records every
feature change that was requested, in order, with enough detail to rebuild the
same behaviour on a fresh FreeCAD checkout. Code may differ; the effect should not.

Branch: `trutnaCAD`. Each entry gives the requested behaviour, then how it is built.

---

## 1. Fuzzy command search on S (sketch mode)

**Requested:** pressing S while editing a sketch opens a search bar; type what you
want to create (point, line, arc, constraints, …) and run it.

**Behaviour**
- Popup appears at the mouse pointer: a text field over a list of matching tools,
  each with its icon and a dimmed category (Create, Constraint, Dimension, Modify,
  Select, View, Sketch).
- Up/Down move, Enter or click runs, Esc closes. Recently used tools are listed
  first when the box is empty.
- Matching: every typed word must start a word of the tool's name or of its
  keywords (Fusion terms such as "fix" → Lock/Block, "mirror", "pattern"). Typo
  tolerance ("cnstr") is only a fallback when nothing matches properly.
- Tools that need a selection first are shown dimmed with a hint, not hidden.

**Implementation**
- `src/Gui/CommandSearch.py` holds the shared popup, matching and keyword table.
- `src/Mod/Sketcher/SketcherCommandSearch.py` registers `Sketcher_CommandSearch`
  (Accel "S", `CmdType: ForEdit`, active only while a sketch is in edit), listing
  every Sketcher command that has an action except a few non-editing ones.
  Python commands need `ForEdit`, otherwise FreeCAD disables them while the sketch
  task dialog is open.
- It is imported from the Sketcher's `InitGui.py` and added to the sketch
  edit-mode menu (`SketcherAddWorkbenchSketchEditModeActions` in
  `Mod/Sketcher/Gui/Workbench.cpp`); a command's shortcut only works once it is
  placed in a menu or toolbar.

## 2. No single-key shortcuts

**Requested:** no single-press keyboard shortcuts at all; tools are reached from the
toolbar or the S search.

**Implementation**
- Cleared every single-letter `sAccel` in `Mod/Sketcher/Gui/CommandConstraints.cpp`
  and `CommandSketcherTools.cpp` (D, A, H, V, O, L, I, P, N, T, R, E, S, W, C).
  Multi-key sequences ("G, L" …) remain.
- Cleared the global "S, B", "S, F", "S, G", "S, D" sequences (Std_SelBack,
  Std_SelForward, Std_LinkSelectLinked, Std_LinkSelectLinkedFinal) because a key
  that starts a longer sequence makes FreeCAD's shortcut manager wait 300 ms.
- The view keys 0–6 are still single-key shortcuts (not yet requested).
- Exceptions requested later, in sketch mode: L draws a line (20), D starts a dimension (23),
  X toggles construction (25).

## 3. Esc does not leave the sketch

**Requested:** pressing Esc must not exit sketch editing.

**Implementation:** default of the `LeaveSketchWithEscape` preference changed to
false in `ViewProviderSketch.cpp`, `TaskDlgEditSketch.cpp` and
`SketcherSettings.ui`. Esc still cancels the active tool or closes popups.

## 4. Toolbar layout

**Requested:** remove the first five buttons (New, Open, Save, Undo, Redo); the
workbench ("mode") drop-down is first on the left, like Fusion; group the view
options under one button; Mass Properties under Measure; Validate Sketch inside
the Sketch drop-down, which leads the second row.

**Implementation**
- `StdWorkbench::setupToolBars` (`src/Gui/Workbench.cpp`): File toolbar removed;
  Edit toolbar keeps only Refresh; the Workbench selector toolbar is created first.
- `Std_ViewOptions` (`src/Gui/CommandView.cpp`): a command whose toolbar button
  only opens a menu (Fit All, Fit Selection, Align to Selection, Standard Views ▸,
  Draw Style ▸). Built with a small `Action` subclass that gives its QToolButton
  an InstantPopup menu filled from the commands' own actions.
- `Std_MeasureGroup` (`src/Mod/Measure/Gui/Command.cpp`): a `GroupCommand` with
  Measure and Mass Properties, not remembering the last pick.
- `PartDesign_CompSketches` gains `Sketcher_ValidateSketch`, is placed first in
  the Part Design modeling toolbar, and is named "Sketch".
- Part's selection-filter button is placed after `Std_ViewOptions`
  (`Mod/Part/Gui/WorkbenchManipulator.cpp`).

## 5. Fusion-style grouped toolbar (ribbon)

**Requested:** make the toolbars work like Fusion: a single row, mode selector
first, labelled groups whose label opens a menu of every tool; Part Design and
sketch mode first.

**Behaviour**
- Part Design: CREATE, MODIFY, CONSTRUCT, INSPECT, VIEW, SELECT.
- Sketch editing: CREATE, MODIFY, CONSTRAINTS, INSPECT, VIEW, SELECT, and a green
  FINISH SKETCH.
- Each group shows a few pinned tools as icons; its label (e.g. "CREATE ▾") opens
  the full menu, with drop-down commands as sub-menus.
- Other workbenches keep FreeCAD's normal toolbars, shown as the user set them up.

**Implementation**
- `src/Gui/Ribbon.py` (installed to `Ext/freecad/gui`, started from
  `FreeCADGuiInit.py` with a zero-delay timer). Layouts are plain lists at the top
  of the file.
- It hides the other toolbars (keeping Workbench and Edit) and also hides their
  View > Toolbars entries, because FreeCAD skips toolbars with hidden entries when
  saving visibility. On leaving a ribbon context it restores the entries and calls
  `reloadActive()` on the workbench.
- Context comes from the active workbench plus whether a sketch is in edit;
  updates are triggered by `workbenchActivated` and a Gui document observer.
- Commands with no QAction yet (e.g. the Part Design datums) get stand-in actions
  that run the command and refresh their enabled state every 500 ms.

## 6. One tool for join, cut and intersect

**Requested:** additive and subtractive operations become the same button, with a
toggle in the dialog.

**Behaviour:** Extrude, Revolve, Sweep, Loft, Coil and the primitives have an
**Operation** choice (Join / Cut / Intersect) in their dialogs. A cutting Extrude
goes into the material like Pocket did. Pocket, Groove and the subtractive
loft/pipe/helix/primitives are removed from menus, toolbars and the ribbon, but
still exist so old files and scripts work.

**Implementation**
- `FeatureAddSub::defineAdditive` exposes the existing `Operation` enumeration
  with Union, Subtraction and Common. `onDocumentRestored` widens the enum again
  for files saved with the old Union-only list.
- `Pad::getProfileNormal` flips the normal when the feature is subtractive (as
  `Pocket::getProfileNormal` does).
- `Revolution` uses the boolean maker (cut/common) when not Union.
- The dialogs' existing `comboOperation` (`TaskFeatureAddSubParameters`) is shown
  whenever the feature offers more than one operation, labelled Join/Cut/Intersect.
- Note: a cutting Extrude saved by TrutnaCAD falls back to Join in upstream FreeCAD.

## 7. Fusion names

**Requested:** rename tools that overlap with Fusion to Fusion's names.

**Renames (menu text and dialog titles)**
- Part Design: Pad → Extrude, Additive Loft → Loft, Additive Pipe → Sweep,
  Additive Helix → Coil, Additive Primitive → Primitive (items: Box, Cylinder, …),
  Thickness → Shell, Boolean → Combine, Linear Pattern → Rectangular Pattern,
  Polar Pattern → Circular Pattern, Path Pattern → Pattern on Path,
  New Sketch → Create Sketch, Datum Plane/Line/Point → Plane/Axis/Point,
  New Part → New Component. FreeCAD's own concentric "Circular Pattern" became
  "Concentric Pattern" to avoid a clash.
- Sketch: Rectangle → 2-Point Rectangle, Centered Rectangle → Center Rectangle,
  Circle From Center → Center Diameter Circle, Circle From 3 Points → 3-Point
  Circle, Arc From Center → Center Point Arc, Arc From 3 Points → 3-Point Arc,
  Ellipse From Center → Ellipse, Slot → Center to Center Slot, Arc Slot → Center
  Point Arc Slot, B-Spline → Control Point Spline, B-Spline From Knots → Fit Point
  Spline, Trim Edge → Trim, Extend Edge → Extend, Split Edge → Break,
  Move / Array Transform → Move/Copy, Scale → Sketch Scale, Toggle Construction
  Geometry → Normal/Construction, External Projection → Project, External
  Intersection → Intersect, Dimension → Sketch Dimension, constraints drop the
  "Constraint" suffix (Coincident, Parallel, …), Symmetric → Symmetry,
  Lock Position → Fix, Leave Sketch → Finish Sketch.

**Implementation:** `sMenuText` in each command's constructor; dialog titles in the
`Task*Parameters.cpp` files and `ViewProviderDatum.cpp`.

## 8. Re-select profiles

**Requested:** allow re-selecting the profile(s) for operations that use profiles.

**Behaviour:** Extrude, Revolve and Coil dialogs (and Pocket/Groove for old files)
start with a **Profiles** row showing what is used ("Sketch: 2 regions"). Select
lets you click closed sketch regions or faces in the 3D view to toggle them in or
out; the feature updates live. Whole Sketch goes back to every region. Cancel
restores the original. Loft and Sweep keep their single-profile button; Hole
works from points and is unchanged.

**Implementation:** `src/Mod/PartDesign/Gui/TaskProfileParameters.{h,cpp}`, a task
box with its own selection observer and gate. Sketch regions are the sketch's
`InternalFaceN` elements (needs `MakeInternals`); the feature's `Profile` sub-list
holds the picks and `AllowMultiFace` is set so sketch sub-elements are honoured.
While selecting, the profile and base solid are shown and the feature hidden.

## 9. TrutnaCAD logo

**Requested:** change the logo to TrutnaCAD.

**Behaviour:** window title, app icon, splash screen and About image show
TrutnaCAD (an extruded white "T" with a dashed sketch line on a navy tile;
wordmark "Trutna" + "CAD" in teal; "Built on FreeCAD" on the splash).

**Implementation:** FreeCAD's branding support. `src/Main/branding/branding.xml`
(copied next to the executable) sets Application, WindowTitle, WindowIcon,
SplashScreen and AboutImage; images `trutnacad.svg` and `trutnacad-splash.svg` go
to `share/branding`. ExeName/ExeVendor stay "FreeCAD" so user settings stay put.

## 10. TrutnaCAD everywhere

**Requested:** replace FreeCAD with TrutnaCAD wherever it appears, not just the logo.

**Behaviour:** all user-visible text says TrutnaCAD, in every language. Kept as
FreeCAD on purpose: credits ("FreeCAD contributors/developers"), FreeCAD's own
forum, wiki, website, bug tracker and donation pages, version references
("FreeCAD 1.0"), and code or paths (FreeCADGui, FreeCAD.Console, …/FreeCAD/…).

**Implementation**
- `Gui::Translator` installs a `BrandingTranslator` (in
  `src/Gui/Language/Translator.cpp`) in front of the language translators. For
  any source text containing "FreeCAD" it fetches the language's translation and
  replaces the product name with the branding `Application` value, using a regex
  that skips the exceptions above. It is reinstalled after each language file
  loads so it stays first.
- macOS takes the menu-bar and Dock name from the program file, so the build
  copies the executable to `bin/TrutnaCAD` (a symlink is resolved and would still
  show FreeCAD); launch that one.

## 11. No "development version" warning on the splash

**Requested:** remove the "WARNING: This is a development version" text from the
loading screen.

**Implementation:** `<SplashWarningColor></SplashWarningColor>` in branding.xml;
the splash draws the warning only when that colour is valid.

## 12. Return presses OK

**Requested:** in a task dialog with an OK button, pressing Return clicks OK.

**Implementation:** `TaskView` installs an application event filter
(`src/Gui/TaskView/TaskView.cpp`). Return/Enter pressed outside the task panel
(e.g. in the 3D view) clicks the dialog's OK button. It leaves Return alone in
text fields, spin boxes, text editors, lists and trees, while a popup or modal
window is open, and for dialogs without an OK button. Sketch editing opts out
(dynamic property `ignoreReturnOutsidePanel` set in `TaskDlgEditSketch`) because
sketch tools use Return and its OK finishes the sketch. Inside the panel FreeCAD
already pressed the default button.

## 13. S search outside sketches

**Requested:** outside sketch mode, S opens the same kind of search over all the
tools on the toolbars available.

**Implementation:** `Std_CommandSearch` in `src/Gui/CommandSearch.py` (Accel "S",
inactive while a sketch is in edit so the sketch search takes over). Entries come
from the visible toolbars (including drop-down button menus) and the ribbon
groups (pinned buttons plus group menus), labelled with the group or toolbar
name, and run by triggering the action. Imported from `FreeCADGuiInit.py` before
the first workbench builds its menus, and placed in the Tools menu so the
shortcut is live.

## 14. Dashed construction icons

**Requested:** the items in the CONSTRUCT menu get dashed lines in their icons to
tell them apart from sketch lines.

**Implementation:** `stroke-dasharray` added to the outline/bar strokes of the
Part Design icons for Plane, Line (Axis), Point, Coordinate System, Shape Binder,
Sub-Shape Binder and Clone (`Mod/PartDesign/Gui/Resources/icons`).

## 15. Task panel as a right-hand pane

**Requested:** the task panel (feature dialogs) is docked on the right by default as
a pane, not a pop-up.

**Behaviour:** the Tasks panel is always docked on the right, about 340 px wide. It can no longer be floated, so it cannot
turn into a separate window (double-clicking a dock's title bar used to do that,
and the floating state was remembered).

**Implementation**
- `src/Gui/Control.cpp`: dock features no longer include `DockWidgetFloatable`
  when a dialog opens or closes.
- `src/Gui/TaskPane.py` (`dock_on_right`, run from `FreeCADGuiInit.py` once the
  window is up): undoes a saved floating or other-side position, docks the panel
  on the right, removes the float option and widens it to 340 px (then hides it
  again unless a dialog is open — see 16).

## 16. Task pane only while a task is active

**Requested:** don't show the task pane when no task is active.

**Behaviour:** the right-hand pane appears when a task dialog opens (feature
dialogs, sketch editing, etc.) and disappears when it is closed with OK/Cancel.
Switching to a document with an open dialog shows it again.

**Implementation:** `src/Gui/TaskView/TaskView.{h,cpp}`: `updateDockVisibility()`
shows the parent `QDockWidget` only when the current document's task has an
`ActiveDialog` or contextual panels. Scheduled with a zero-delay timer
(`scheduleDockVisibilityUpdate()`) from `setShownTaskInfo` (dialog opened /
document switched), `removeDialog`, and `add/removeContextualPanel`, so a dialog
that closes and immediately opens another doesn't flicker the pane.

## 17. Skip the first-start setup page

**Requested:** don't show the "choose a theme color and units" page on every launch;
go straight to the start page.

**Behaviour:** the Start view always opens on the documents (start) page. The
theme/units setup is still available from the "Open First Start Setup" button in
the start page footer.

**Implementation:** `src/Mod/Start/Gui/StartView.cpp`: the constructor sets the
documents widget as current instead of checking the `FirstStart2024` parameter
(which is only cleared by clicking Done, and was never saved when the app was
force-quit).

## 18. No text drawn over the splash image

**Requested:** remove the black "TrutnaCAD" text drawn on the startup splash.

**Behaviour:** the splash shows only the TrutnaCAD splash artwork, with no extra
name or version text on top of it.

**Implementation:** `src/Main/branding/branding.xml` sets `<SplashInfoColor>` to an
empty value. `Gui::SplashScreen::splashImage` only draws the application name and
version when that colour is valid (the default is `#212529`, set in
`Main/MainGui.cpp`).

## 19. Typo-tolerant S search

**Requested:** in the S search, a misspelling shouldn't make every result disappear.

**Behaviour:** misspelled words still find the tool: swapped, missing, extra or wrong
letters ("extrdue", "cirlce", "rectangel", "skecth", "measrue", "mirorr"). About one
mistake per three letters is tolerated. Proper matches still win; typo matches are
shown only when nothing matches properly. If even that finds nothing, the 8 closest
tools are listed, so the list is never empty while you type.

**Implementation:** `src/Gui/CommandSearch.py` (shared by the sketch and toolbar
searches):
- `_edit_distance` (optimal string alignment: insert/delete/substitute/swap) and
  `_typo_distance(token, word)` — distance to the word or to a start of it
  (lengths n-1, n, n+1), cached with `functools.lru_cache`.
- `_typo_match` accepts a token of 3+ letters within `max(1, len // 3)` edits of any
  title/keyword word; used in `Entry.score` after the in-order-letters
  (abbreviation) match fails.
- `CommandSearchPopup.refresh`: when nothing matches at all, ranks every entry by
  `Entry.distance(tokens)` (sum of each token's closest-word distance) and shows the
  `MAX_CLOSEST` best.

## 20. L draws a line in sketch mode

**Requested:** in sketch mode, pressing L starts a line (like Fusion). This is the
one deliberate exception to "no single-key shortcuts" (2).

**Behaviour:** while editing a sketch, L starts the Line tool; click two points to
draw. Typing L in a text or number field still types the letter.

**Implementation:** `src/Mod/Sketcher/Gui/CommandCreateGeo.cpp`:
`CmdSketcherCreateLine` has `sAccel = "L"` (was "G, L"). The command is ForEdit and
its action only sits in the sketch toolbar, so L does nothing outside a sketch.
Other L shortcuts (Assembly joint, Draft/BIM "L, x") live in other workbenches and
don't clash.

## 21. Reject fully redundant constraints, warn on partially redundant ones

**Requested:** adding a fully redundant constraint shows an error and the constraint
is not added; a partially redundant one is added with a warning.

**Behaviour:** after a constraint command (Horizontal, Perpendicular, Symmetric,
dimensions, ...) the sketch is solved. If the new constraint removes no degree of
freedom while the solver reports new redundancy, the command is taken back and an
error notification says "Constraint not added. This constraint is redundant: the
existing constraints already enforce it." For dimensions this happens before the
value dialog opens. If it removes some degrees of freedom but also introduces
redundancy, it is kept with a "Partially redundant constraint" warning. Reference
(non-driving) constraints and conflicting constraints are left to FreeCAD's usual
handling. Drawing, trim, split etc. are not checked.

**Implementation** (`src/Mod/Sketcher/Gui`):
- `ViewProviderSketch::slotConstraintAdded` (fires before each constraint is
  appended, edit mode only) calls `noteConstraintAdded`, which records the first new
  index, the last solve's DoF and the number of redundant + partially redundant
  constraints.
- `Utils.cpp` `rejectRedundantConstraints(obj, cmd)`: only for transactions named
  "Add ... constraint" or "Dimension" (or when called with the command); skips if
  no new constraint is driving; solves; ignores conflicts; if redundancy grew and DoF
  did not drop: aborts the command (`cmd->abortCommand()`, or the pending App
  transaction, or Gui undo of the committed step so no empty undo step is left),
  re-solves and shows `Gui::TranslatedUserError`; if redundancy grew but DoF dropped:
  `Gui::TranslatedUserWarning`. The solver may blame the older equivalent
  constraint, which is why DoF is used to decide.
- Called at the start of `tryAutoRecompute`, and in `finishDatumConstraint`
  (`CommandConstraints.cpp`) before the datum dialog.

## 22. Clear redundancy messages

**Requested:** the redundant-constraint error message (21) was unclear.

**Behaviour:** messages name the constraint, its geometry and the constraints that
already cover it, using the sketch's own numbering (Line 1 = Edge1):
- "Perpendicular not added. Perpendicular (Line 2, Line 1) is already true because
  of: Coincident (end of Line 1, start of Line 2), Horizontal (Line 1), Vertical
  (Line 2). It would only duplicate them."
- "Symmetry partly redundant. Symmetry (start of Line 3, start of Line 4, origin) was
  added, but now Point on curve (start of Line 4, X axis) is redundant. Delete it to
  keep the sketch valid."

**Implementation** (`src/Mod/Sketcher/Gui/Utils.cpp`):
- `constraintTypeName`, `geometryName` ("Line 2", "end of Arc 3", "X axis",
  "origin", "projected geometry"), `describeConstraint`, `describeConstraints`.
- `relatedConstraints`: existing driving constraints sharing geometry with the new
  ones (the solver doesn't say what a redundant constraint duplicates), plus old
  constraints the solver now flags as redundant. Names are built before the command
  is taken back.
- Also fixed in 21: some commands (e.g. Point on curve) never call
  `tryAutoRecompute`, so `noteConstraintAdded` also schedules
  `checkPendingConstraints` with a zero-delay timer. It skips (and retries every
  100 ms) while the document has a pending transaction, e.g. an open dimension
  dialog, and tracks the sketch with `App::DocumentObjectWeakPtrT`.

## 23. D starts a dimension in sketch mode

**Requested:** pressing D in sketch mode does a dimension (like Fusion).

**Behaviour:** while editing a sketch, D starts the Sketch Dimension tool: click
geometry, then click to place; the dimension type follows the selection (length,
angle, radius, ...). Typing D in a text or number field still types the letter.

**Implementation:** `src/Mod/Sketcher/Gui/CommandConstraints.cpp`:
`CmdSketcherDimension` has `sAccel = "D"` (was empty). ForEdit command whose action
only sits in the sketch toolbar, so D does nothing outside a sketch. Other D
shortcuts (TechDraw dimension, Assembly joint, Draft/BIM "D, x") live in other
workbenches.

## 24. Esc closes an error/warning notification

**Requested:** if an error is on screen and Esc is pressed, close it.

**Behaviour:** while a notification bubble (errors and warnings such as "Perpendicular
not added ...") is showing, Esc closes it and does nothing else (it doesn't also
cancel the active sketch tool). Modal error dialogs already closed on Esc.

**Implementation:** `src/Gui/NotificationBox.cpp`, `NotificationLabel::eventFilter`
(already installed application-wide while a bubble is shown): for Esc without
modifiers while visible, `ShortcutOverride` is accepted and swallowed (no Esc shortcut
fires) and the `KeyPress` calls `hideNotificationImmediately()` and is swallowed.

## 25. X toggles construction in sketch mode

**Requested:** with lines (or any geometry) selected, X toggles them between
construction and normal, and back.

**Behaviour:** in sketch mode, X toggles the selected geometry between normal and
construction; pressing X again toggles back. With nothing selected, X switches new
geometry between normal and construction drawing (FreeCAD's existing behaviour of
the same command), e.g. while drawing a line.

**Implementation:**
- `src/Mod/Sketcher/Gui/CommandAlterGeometry.cpp`: `CmdSketcherToggleConstruction`
  `sAccel = "X"` (was "G, N").
- `src/Mod/Part/Gui/CommandFilter.cpp`: cleared `Part_VertexSelection`'s "X, S"; the
  selection filter is on the toolbar everywhere, so X would otherwise wait 300 ms
  for a possible second key.

## 26. Spiral tool in sketch mode

**Requested:** create spirals of different types in sketch mode.

**Behaviour:** Sketch > Geometries > Spiral, the CREATE menu, or S → "spiral"
(also "coil", "involute", ...). A floating panel at the top right of the 3D view
(like Fusion's sketch tool dialogs) with a live preview:
- Type: Archimedean (even spacing), Logarithmic (spacing grows; inner radius > 0),
  Fermat (r² linear in angle; spacing shrinks outwards), Involute (of a base circle,
  as on gear teeth; outer radius follows from base radius and turns).
- Turns, inner (or base) radius, outer radius, start angle, direction (CCW/CW),
  center X/Y. Clicking a sketch point (or having one selected when starting) puts
  the center there. Invalid input shows a red message and disables OK.
- OK adds the spiral fixed in place (Fix/Block constraint) as one undo step
  "Create spiral"; Cancel/Esc or leaving the sketch removes the preview. Settings
  are remembered.
The spiral is a cubic B-spline within 0.01 % of its size of the exact curve
(~1 µm on 10 mm; ends exact).

Also: sketch B-spline information overlays (degree, control polygon, curvature
comb, knot multiplicity, pole weights) are now off by default; they buried the
spiral (and any spline) in green. The existing B-spline toggle commands show them.

**Implementation:**
- `src/Mod/Sketcher/SketcherSpiral.py` (new; imported from `InitGui.py`, listed with
  `Sketcher_CreateSpiral.svg` in `Sketcher_Scripts` in `Mod/Sketcher/CMakeLists.txt`):
  `spiral_points` (exact curve, 48 samples per turn), `spiral_curve`
  (`Part.BSplineCurve.approximate`, degree 3, C2, tolerance 1e-4 × size; falls back to
  `interpolate`), `SpiralPanel` (QDialog with `Qt.Tool`; opens an App transaction,
  replaces the preview geometry on every change, selection observer for the center,
  500 ms timer cancels when sketch edit ends), command `Sketcher_CreateSpiral`
  (ForEdit), and a workbench manipulator that appends it to the menu holding
  `Sketcher_CreatePeriodicBSplineByInterpolation` (a command only gets the action the
  S search needs once it is in a menu; "insert … after" does nothing for a menu's last
  item, hence "append").
- `src/Gui/Ribbon.py`: added to the sketch CREATE menu after the B-splines.
- `src/Gui/CommandSearch.py`: KEYWORDS entry.
- `src/Mod/Sketcher/Gui/EditModeCoinManager.cpp` and `CommandSketcherOverlay.cpp`: the
  five `BSpline*Visible` parameters default to false (both the display and the
  toggle commands).
