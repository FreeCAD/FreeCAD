# 📍 FreeCAD CAM Workbench Roadmap – 2025 and Beyond

This document aims to guide the ongoing development of the CAM Workbench. It sets out shared goals, identifies pain points and opportunities, and invites discussion, collaboration, and contributions from the wider community.

FreeCAD is a volunteer-driven project. This roadmap does not prescribe work or enforce deadlines — it is a living document to help focus effort and encourage alignment across developers, users, and stakeholders.

Like everyone, Maintainers only have so much time that they can allocate to FreeCAD. With many contributors submitting changes, it can be difficult to judge the relative importance of an individual contribution.  This document establishes a shared vision of the direction of the workbench so maintainers have a framework for prioritizing review and approval.

# 🔭 Vision
*(this section establishes non-negotiable big-picture definition of what we are buildin)*

A robust, intuitive, and industry-ready CAM solution that:

- Integrates seamlessly with the FreeCAD modeling workflow, unit schemas, and translation
- Supports professional CNC workflows
- Is usable and intuitive for hobbyist and non-professionals
- Embraces open standards
- Encourages extensibility and script-ability via Python

## ⚙️ Functionality
*(this section describes expected functionality in agnostic terms.  No FreeCAD-specific language)*

Any CAM application meeting the needs above must provide functionality in these areas:
- [Tool Management](<./Functionality/Tool Management.md>)
- [Stock and Work-piece Setup](<./Functionality/Stock and Work-piece Setup.md>)
- [Job Management](<./Functionality/Job Management.md>)
- [Operation Configuration](<./Functionality/Operation Configuration.md>)
- [Simulation and Verification](<./Functionality/Simulation and Verification.md>)
- [Output Generation](<./Functionality/Output Generation.md>)
- [Scripting and automation](<./Functionality/Scripting and automation.md>)

### ADR Log

*(ADRs - Architecture Decision Report)*


*(NOTE: THESE ARE ALL **EXAMPLES**.  NOT ACTUAL/AGREED ADRS.)*

| ADR                           | Description                                                                                                |
| ----------------------------- | ---------------------------------------------------------------------------------------------------------- |
| [ADR-001](<./ADR/ADR-001.md>) | Use Dressups to modify base operations                                                                     |
| [ADR-002](<./ADR/ADR-002.md>) | Internal representation of tool path                                                                       |
| [ADR-003](<./ADR/ADR-003.md>) | Height planes to safely move tools within and between operations                                           |
| [ADR-004](<./ADR/ADR-004.md>) | Standardized vocabulary around rotation of cutter, direction of cut, and conventional vs. climb operations |
| [ADR-005](<./ADR/ADR-005.md>) | Triggering Tool Path Recompute in Task Panels                                                              |

# ⚠️ Pain Points
*(this section functions like an FAQ.  It helps keep users from creating duplicate issues.  It gives new developers a first place to connect)*

Perennial complaints from users:

- Visualization of first rapid move in operation makes it appear as though tool
  returns to origin between ops (It doesn't)
- Climb vs Conventional terminology not consistently used
- Safe Height vs Clearance Height confusing
- Arrays of similar gcode
- Lack of F & S calculation

# 🚀 Initiatives and Projects

*(this section will be a list of initiatives that have been collectively discussed.
These are things that we agree should get attention but are larger than a single issue or pull request.  If you have an idea for a project, create a pull request adding it to this section. Changes will be discussed in the PR and at periodic meetups)*

*(To keep focused and moving forward, we should voluntarily limit this list to ~8-10 items)*

*(Each project will have a corresponding github project to connect related issues and pull requests)*

==TODO:  This list shouldn't contain issues.  That will make it a maintenance nightmare and result in a lot of PRs to main.  Each item may link to a github project.  Separate projects should be created for each work unit but not until we agree on the list.  I've left issue numbers here for now because it seems helpful. -sliptonic==

| Work Unit                             | Why it should be a priority                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         | Goals                                                                                                                                                                                                        |                         | Issue(s)                                         |
| ------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ----------------------- | ------------------------------------------------ |
| stabilize the BTL tool implementation | Incorporating the BTL approach to tools has caused some regressions.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |                                                                                                                                                                                                              |                         | #21430 #21855 #18598 #9466 #22228                |
| Overhaul the Entry dressup(s)         | We currently hav both ramp and leadin/out dressup.  These could be consolidated into a single dressup with multiple strategies possible. Ramp lacks a UI task panel. Ramp also has a helical entry strategy that duplicates code from helix and from adaptive operation.  Adaptive operation has its own helix entry method.  The result is a confusing jumble of functionality and code that is hard to work on                                                                                                                                                                                                                                                                                                                    |                                                                                                                                                                                                              |                         | #16897 #10621 #22137 #8150 #14380 #16144         |
| Helix work                            | Helices are used in at least three different places; Helix Op, Ramp entry, and Adaptive clearing.  This is a lot of duplicated code with inconsistent features. Centralizing the logic to a helix generator would allow us to first put it under unit tests and then incrmentally replace the duplicated logic in each place where it is used. This will provice a more robust solution that is easier to extend and maintain and also give the users a consistent set of features related to helices everywhere                                                                                                                                                                                                                    |                                                                                                                                                                                                              | Circular Holes<br>Entry | #22469, #13455, #22357, #8149, #17737, PR #21971 |
| Climb / Conventional                  | Overhal all operations to use Climb / Conventional vs CW / CCW where it makes sense. (Laser, Drag Knife, Plasma doesn't care)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |                                                                                                                                                                                                              |                         | #14314                                           |
| Feeds / Speed Warnings                | Improve / fix regression on warning user if a Tool Controller doesn't have feeds & speed applied to it                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |                                                                                                                                                                                                              |                         |                                                  |
| Facemill Op Refactor                  | Facemill operation has several issues including but not limited to strange path generation with large cutters, improper terminology in the GUI and other bugs.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |                                                                                                                                                                                                              |                         | #15994, #16221, #16215, #8272, #15992            |
| Circular Hole Improvements            | - Moving between circular hole targets has complexity not present in other operations.  Addressing the shortcomings of linking moves for circular hole operations will allow this logic to be reused in other operations.<br>- Handling of target selection get be generalized to other operations and has overlapping functionality with Sanity<br>- Circular holes generates more complex path commands that need special handling in the simulator<br>- There are numerous other circular hole operation strategies we would like to implement.<br>- Refactoring Post-processor logic should consider centralizing the handling and decomposition of circular hole commands<br>- There is overlap with the Helix work initiative | - linking generator<br>- Improved UI for circular hole targets<br>- Improved UI for 'strategies'<br>- base class logic for decomposing drill commands<br>- framework for tool/op validation and squawks.<br> | Helix<br>Entry <br>     | #22599, #9405 #16205                             |
| Pocket Improvements                   | - Pocketing is a fundamental CAM operation<br>- Handling of intermediate geometry is inconsistent and buggy (extensions)<br>- Proper refactoring allows for deprecating adaptive and strategy switching<br>- Proper handling of intermediate geometry allow for an massively improved facing operation                                                                                                                                                                                                                                                                                                                                                                                                                              | - consolidate and refactor extension logic<br>- Adaptive generator<br>-                                                                                                                                      | Entry                   |                                                  |


Temporarily including this list to make review easier

| ID     | Title                                                                                                                               | Author           |
| ------ | ----------------------------------------------------------------------------------------------------------------------------------- | ---------------- |
| #22728 | CAM: Fix seach tool controller in Operations group                                                                                  | tarman3          |
| #22669 | [CAM] LeadInOut new features                                                                                                        | davidgilkaufman  |
| #22659 | CAM: Fix the format spec used with size_t                                                                                           | chennes          |
| #22602 | CAM: fix G0 regression in drilling                                                                                                  | J-Dunn           |
| #22598 | Fix path dressup array test case                                                                                                    | emmanuel-ferdman |
| #22578 | Core: Generation of python bindings for CAM                                                                                         | z0r0             |
| #22569 | CAM: revert grbl_post regression                                                                                                    | J-Dunn           |
| #22500 | CAM: select rows instead of cells in drilling panel                                                                                 | jffmichi         |
| #22484 | CAM: RampEntry Dressup - Remove X0Y0 from beginning                                                                                 | tarman3          |
| #22468 | CAM: Dressup Tag - Automatic for multiprofile                                                                                       | tarman3          |
| #22405 | CAM: Path.Base.Language - isStraight() and isArc()                                                                                  | tarman3          |
| #22392 | CAM: fix: CAM tests use files from user asset dir                                                                                   | knipknap         |
| #22357 | CAM: Adaptive - Helix generator                                                                                                     | tarman3          |
| #22350 | CAM: Update UI strings for consistency                                                                                              | ryankembrey      |
| #22336 | [CAM] Correctly process Adaptive extensions                                                                                         | dbtayl           |
| #22335 | CAM: Add Turning Tools                                                                                                              | dubstar-04       |
| #22304 | CAM: Task panel - Select shapes from several objects                                                                                | tarman3          |
| #22250 | CAM: Dogbone - fix for Pocket                                                                                                       | tarman3          |
| #22228 | CAM: Various bugfixes for CAM tool management                                                                                       | knipknap         |
| #22226 | CAM: Engrave dual direction                                                                                                         | tarman3          |
| #22204 | CAM: integrate new simulator as MDI widget into main window                                                                         | jffmichi         |
| #22153 | CAM: cleanup: add test for DetachedDocumentObject and deduplicate code                                                              | knipknap         |
| #22151 | CAM: Add a machine editor for mills, lathes and their spindles                                                                      | knipknap         |
| #22080 | CAM: Fix Path.Geom.cmdsForEdge for BSplineCurve                                                                                     | tarman3          |
| #21971 | CAM: Helix improve behavior                                                                                                         | tarman3          |
| #21944 | [CAM] Fix ramp dressup performance                                                                                                  | davidgilkaufman  |
| #21940 | [CAM] Add Turning Operations                                                                                                        | dubstar-04       |
| #21923 | CAM: LinuxCNC post - Start coolant only after the tool back to the workpiece                                                        | tarman3          |
| #21820 | CAM: Mirror Dressup                                                                                                                 | tarman3          |
| #21769 | CAM: Replace the main library editor dialog and add copy & paste & drag & drop support                                              | knipknap         |
| #21756 | CAM: Update PathUtils.py                                                                                                            | papaathome       |
| #21738 | CAM: RetractThreshold                                                                                                               | tarman3          |
| #21700 | CAM: Waterline - Grid Dropcutter                                                                                                    | tarman3          |
| #21605 | CAM: ZigZagOffset - extra offset for ZigZag before Offset                                                                           | tarman3          |
| #21578 | CAM: Dragknife dressup adds unnecessary maneuvers                                                                                   | lagnat           |
| #21508 | CAM: PathShapeTC with class ObjectPathShape                                                                                         | tarman3          |
| #21507 | CAM: Path Compound with Tool Controller                                                                                             | tarman3          |
| #21220 | CAM: Adaptive: Fix bspline processing                                                                                               | dbtayl           |
| #20981 | CAM: adds optional arc move G2/G3 translation to G1 linear moves, to support CNC machines which do not support G2/G3 moves directly | jalapenopuzzle   |
| #16990 | Rename Label2 to Description                                                                                                        | matt-taylor-git  |
| #14960 | [CAM] User friendly error messages                                                                                                  | phaseloop        |
| #6680  | [Path] Kerf widening for profiles                                                                                                   | sliptonic        |
| #6677  | Path: Add a new waterline algorithm named Grid Dropcutter                                                                           | belov-oleg       |

# Priorities
*(When evaluating Pull Requests, the following priorities will be considered)*

| Priority | Change                                          | Rationale                                                                                                                                                                                                  |
| -------- | ----------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1        | Regression bug fixes                            | Regressions represents a step backward. They impact users and other developers. Regressions may slow development in other areas.                                                                           |
| 2        | Changes related to the initiatives and projects | Projects are grouping together numerous issues including both features and bug fixes.  Multiple people may be working collectively.  Timely response to changes is necessary to avoid rework and conflict. |
| 3        | other bug fixes                                 | Resolving bugs with existing functionality that is not part of a broader initiative.                                                                                                                       |
| 4        | other new features                              | One-off features are either small enough to be reviewed independently or are big enough to warrant discussion by a larger group.  This can be time consuming.                                              |
