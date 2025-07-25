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

| Work Unit                             | Why it should be a priority                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      | Issue(s)                                         |
| ------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------ |
| Handle linking moves consistently     | abstracting the generation of linking moves (rapid moves into, out of, and between parts of a toolpath) to a generator would allow us to factor this logic out of the individual operations.  The ops would get more consistent and easier to maintain. It will allow us to build a more intuitive drilling operation.                                                                                                                                                                                           | #22599, #9405 #16205                             |
| Pocketing Strategies                  | The adaptive clearing strategy need not be a separate operation type.  Factoring out the adaptive algorithm to a generator would allow us to move the adaptive operation into pocket and 3D pocket as a strategy. This would let users switch strategies without deleting and recreating whole operations.                                                                                                                                                                                                       |                                                  |
| stabilize the BTL tool implementation | Incorporating the BTL approach to tools has caused some regressions.                                                                                                                                                                                                                                                                                                                                                                                                                                             | #21430 #21855 #18598 #9466 #22228                |
| Overhaul the Entry dressup(s)         | We currently hav both ramp and leadin/out dressup.  These could be consolidated into a single dressup with multiple strategies possible. Ramp lacks a UI task panel. Ramp also has a helical entry strategy that duplicates code from helix and from adaptive operation.  Adaptive operation has its own helix entry method.  The result is a confusing jumble of functionality and code that is hard to work on                                                                                                 | #16897 #10621 #22137 #8150 #14380 #16144         |
| Helix work                            | Helices are used in at least three different places; Helix Op, Ramp entry, and Adaptive clearing.  This is a lot of duplicated code with inconsistent features. Centralizing the logic to a helix generator would allow us to first put it under unit tests and then incrmentally replace the duplicated logic in each place where it is used. This will provice a more robust solution that is easier to extend and maintain and also give the users a consistent set of features related to helices everywhere | #22469, #13455, #22357, #8149, #17737, PR #21971 |
| Climb / Conventional                  | Overhal all operations to use Climb / Conventional vs CW / CCW where it makes sense. (Laser, Drag Knife, Plasma doesn't care)                                                                                                                                                                                                                                                                                                                                                                                    | #14314                                           |
| Feeds / Speed Warnings                | Improve / fix regression on warning user if a Tool Controller doesn't have feeds & speed applied to it                                                                                                                                                                                                                                                                                                                                                                                                           |                                                  |
| Facemill Op Refactor                  | Facemill operation has several issues including but not limited to strange path generation with large cutters, imporoper terminology in the GUI and other bugs.                                                                                                                                                                                                                                                                                                                                                  | #15994, #16221, #16215, #8272, #15992            |



# Priorities
*(When evaluating Pull Requests, the following priorities will be considered)*

| Priority | Change                                          | Rationale                                                                                                                                                                                                  |
| -------- | ----------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1        | Regression bug fixes                            | Regressions represents a step backward. They impact users and other developers. Regressions may slow development in other areas.                                                                           |
| 2        | Changes related to the initiatives and projects | Projects are grouping together numerous issues including both features and bug fixes.  Multiple people may be working collectively.  Timely response to changes is necessary to avoid rework and conflict. |
| 3        | other bug fixes                                 | Resolving bugs with existing functionality that is not part of a broader initiative.                                                                                                                       |
| 4        | other new features                              | One-off features are either small enough to be reviewed independently or are big enough to warrant discussion by a larger group.  This can be time consuming.                                              |
