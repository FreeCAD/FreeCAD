# Headless native FreeCAD examples

These examples provide a small Python runner using FreeCAD alone.
It requires an official FreeCAD installation; it does not compile or replace the
application. No running GUI or additional Python package is required.

## Generate a part

Use system Python to launch a **trusted** model file exposing `build(params)`.
The function returns a FreeCAD document object with solid geometry, including
an `App::Part` assembly. Geometry is resolved through `Part.getShape`.
The runner creates a new output directory, starts FreeCADCmd with a timeout,
saves the native document & STEP, then reopens both for consistency checks.

macOS:

```sh
python3 contrib/headless/run.py contrib/headless/plate.py \
  --freecad /Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd \
  --params '{"length":60,"width":24,"thickness":5}' \
  --out /tmp/freecad-plate
```

Windows PowerShell (substitute your installed executable path):

```powershell
py -3.11 contrib/headless/run.py contrib/headless/plate.py `
  --freecad 'C:\Program Files\FreeCAD 1.1\bin\FreeCADCmd.exe' `
  --out "$env:TEMP\freecad-plate"
```

Outputs: `model.FCStd`, `model.step`, `request.json`, `result.json`, `freecad.log`.
Exit status is nonzero on failure, including when FreeCAD swallows a script error.
The output directory must not exist, preventing stale-success results & accidental
overwrites. Model scripts run as the current user, not in a security sandbox.

## Editable features

`plate.py` produces ordinary `Part::Box`, `Part::Cylinder`, `Part::MultiFuse`
& `Part::Cut` objects. Dimensions & hole positions use FreeCAD expressions.
Open `model.FCStd` in FreeCAD to edit the feature tree directly; no custom Python
proxy or addon is needed to reopen it.
The runner stores result visibility & a fitted isometric camera in native GUI
metadata so FreeCAD 1.1.4 opens headless output visibly, with construction hidden.

`revise_plate.py` reopens that document, changes length/thickness, verifies
expressions recompute, then adds another native cylinder/cut feature. Pass
`{"document":"/absolute/path/to/model.FCStd"}` as its parameters.

`import_step.py` accepts `{"source":"/absolute/path/to/input.step"}`. It imports
named parts, nested assembly containers & placements using FreeCAD's native
STEP importer. It does not recover original CAD feature history. Assembly
roundtrips compare component names, hierarchy, solid counts & placed bounds.
`assembly.py` exercises nested translation & rotation independently of external
fixtures. Export temporarily bakes a sole assembly root's placement into its
children because the native exporter otherwise drops that placement; an aborted
transaction restores the editable document, which is checked again afterward.
User STEP fixtures are external inputs & are not included in the repository.

## Real-process end-to-end exercise

```sh
python3 contrib/headless/e2e.py \
  --freecad /Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd \
  --out /tmp/freecad-e2e \
  --step /absolute/path/to/real-part.step
```

Runs separate FreeCAD processes for generation, a parameter variant, reopening
& revising saved native features, invalid-input rejection, & each optional STEP
fixture. Checks analytic plate volume/dimensions before & after native/STEP
roundtrips, plus preservation of the original document. Windows uses the same
script with `py -3.11` & its native FreeCADCmd executable.

Initial verification on 2026-10-09 used official FreeCAD 1.1.4 binaries on
macOS arm64 & Windows x86_64. The assembly revision also passed all three
external STEP fixtures on both hosts, retaining 1, 7 & 15 solids, part labels,
nesting & placed component bounds through native & STEP roundtrips. Final eight
scenarios, including the translated/rotated assembly, passed on Mac 26.3rc1 &
Windows 1.1.4. Private fixtures are never uploaded to CI.

## Roundtrip behavior

| Issue | Resolution & evidence |
| --- | --- |
| Headless documents opening hidden | Result visibility & fitted camera are persisted. Native generated plate, bead & assembly output were opened visually. |
| Headless STEP import flattening assembly structure | Native `Import.insert`/`Import.export` replace flattened shape import/export. Component structure & placed bounds are checked on both roundtrips. |
| STEP export dropping moved/rotated root placement | Transactional export normalization preserves world geometry & restores native placements. Nested placement fixture covers this. |

## Volume qualification

Volume uses the installed runtime's `Shape.Volume` integration. The runner rejects
non-finite or non-positive results & labels reported volumes as unqualified.
Default OCCT integration can overstate curved-solid volume, so these values check
serialization consistency rather than manufacturing accuracy. The examples do
not change FreeCAD's native integration or require a custom C++ API.

## Scope of this first prototype

Shape validity, closed solids, positive volume & roundtrip consistency are checked.
Roundtrip volume has a declared 10 ppm sanity limit; drift above
0.1 ppm & 0.00001 mm³ is reported as a warning. Those are serialization checks,
not a geometric accuracy guarantee; analytic plate checks remain tighter.
Assembly interference validation, stable face selection after topology changes
& a finished GUI workbench remain outside this prototype.
The prototype establishes native editable documents & unattended execution first.
