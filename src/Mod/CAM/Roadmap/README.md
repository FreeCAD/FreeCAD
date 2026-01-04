# 📍 FreeCAD CAM Workbench Road-map – 2025 and Beyond

This document aims to guide the ongoing development of the CAM Workbench. It sets out shared goals, identifies pain points and opportunities, and invites discussion, collaboration, and contributions from the wider community.

FreeCAD is a volunteer-driven project. This road-map does not prescribe work or enforce deadlines — it is a living document to help focus effort and encourage alignment across developers, users, and stakeholders.

Like everyone, Maintainers only have so much time that they can allocate to FreeCAD. With many contributors submitting changes, it can be difficult to judge the relative importance of an individual contribution.  This document establishes a shared vision of the direction of the workbench so maintainers have a framework for prioritizing review and approval.

# 🔭 Vision
*(this section establishes non-negotiable big-picture definition of what we are building)*

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

| ADR                           | Description                                                                                                | Status |
| ----------------------------- | ---------------------------------------------------------------------------------------------------------- | ------ |
| [ADR-002](<./ADR/ADR-002.md>) | Internal representation of tool path                                                                       | Legacy |
| [ADR-007](<./ADR/ADR-007.md>) | Add semantic information to Path Command                                                                   | DRAFT  |


# ⚠️ Pain Points
*(this section functions like an FAQ.  It helps keep users from creating duplicate issues.  It gives new developers a first place to connect)*

Perennial complaints from users:

- Climb vs Conventional terminology not consistently used
- Safe Height vs Clearance Height confusing
- Visualization of first rapid move in operation makes it appear as though tool
  returns to origin between ops (It doesn't)
- Arrays of similar gcode
- Lack of F & S calculation
- Selecting and using tools (tool controllers) is cumbersome

# 🚀 Initiatives and Projects (Epics)

*(this section will be a list of initiatives that have been collectively discussed.
We agree these things should get collective attention because they are larger than a single issue or pull request and have implications on the work of multiple people.  )*

*(If you have an idea for a project, create a pull request adding it to this section. Changes will be discussed in the PR and at periodic meetups)*

*(To keep focused and moving forward, we should voluntarily limit this list to ~8-10 active items)*

*(Each project will have a corresponding GitHub project to connect related issues and pull requests)*

*(You may, of course, work on anything that you like and submit pull requests. However, be warned that pull requests will be judged on the priorities noted below and new features that are outside of the discussed projects will receive additional scrutiny)*


| Epic                                                                            | Description                                                             | Status   |
| ------------------------------------------------------------------------------- | -------------------------------------------------------------------     | -------- |
| Fixing user pain points                                                         | Address the problems listed above in the Pain Points section            | Active   |
| [Better Tool Library](Epics/Better%20Tool%20Library.md)                         | Implement the 'Better Tool Library' approach to tool management         | Active   |
| [Improve Circular Holes](Epics/Circular%20Holes%20Improvement.md)               | Improve the user experience and efficiency of Circular holes operations | Active   |
| [Improve Core Dressups](Epics/ImproveCoreDressups.md)                           | Improve the user experience and feature completion of core dressups     | Active   |
| [Improve Adaptive Operation](Epics/ImproveAdaptiveOperation.md)                 | Improve the user experience and feature completion of Adaptive          | Active   |

# Priorities
*(When evaluating Pull Requests, the following priorities will be considered)*

| Priority | Change                                          | Rationale                                                                                                                                                                                                                                                                                                                                       |
| -------- | ----------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1        | Regression bug fixes                            | Regressions represents a step backward. They impact users and other developers. Regressions may slow development in other areas.                                                                                                                                                                                                                |
| 2        | Changes related to the initiatives and projects | Projects are grouping together numerous issues including both features and bug fixes.  Multiple people may be working collectively.  Timely response to changes is necessary to avoid rework and conflict.                                                                                                                                      |
| 3        | other bug fixes                                 | Resolving bugs with existing functionality that is not part of a broader initiative.                                                                                                                                                                                                                                                            |
| 4        | other new features                              | One-off features are either small enough to be reviewed independently or are big enough to warrant discussion by a larger group.  This can be time consuming.<br>New features that are not part of an existing epic may represent good functionality but will receive heightened scrutiny to ensure they are consistent with established goals. |
