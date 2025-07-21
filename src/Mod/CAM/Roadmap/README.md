# 📍 FreeCAD CAM Workbench Roadmap – 2025 and Beyond

This document aims to guide the ongoing development of the CAM Workbench. It sets out shared goals, identifies pain points and opportunities, and invites discussion, collaboration, and contributions from the wider community.

FreeCAD is a volunteer-driven project. This roadmap does not prescribe work or enforce deadlines — it is a living document to help focus effort and encourage alignment across developers, users, and stakeholders.

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

## 🧼 Code Overview
*(this section provides a starting point for developers to understand how CAM is organized.)*
*( It also gives an honest assessment of the code quality and areas of improvement with an eye toward good coding practices)*

Additionally, as software developers we desire the source code to excel in these areas:
- Modular
- DRY
- Extensible
- Testable

### Introduction to CAM code
- [Tool Subsystem](<./Current State/Tools.md>)
- [Job System, templates](<Jobs.md>)
- [Operations](<./Current State/Operations.md>)
- [Generators](<./Current State/Generators.md>)
- [Dressups](<./Current State/Dressups.md>)
- [Post processing system](<./Current State/PostProcessing.md>)
- [Sanity Report Generation](<./Current State/Sanity.md>)
- [Preference System](<./Current State/Preferences.md>)
- [Path Visualization and Simulator](<./Current State/Visualization.md>)

### ADR Log
| ADR                           | Description                            |
|-------------------------------|----------------------------------------|
| [ADR-001](<./ADR/ADR-001.md>) | Use Dressups to modify base operations |
| [ADR-002](<./ADR/ADR-002.md>) | Internal representation of tool path   |
| [ADR-003](<./ADR/ADR-003.md>) | ADR-003: Safe height and Clearance height in operations  |
| [ADR-004](<./ADR/ADR-004.md>) | Standardized vocabulary around rotation of cutter, direction of cut, and conventional vs. climb operations |


# 📊 Current State
*(This section is a report card.  As much as possible, the scoring will be automated though subjective considerations will occur)*

- **Completeness**:  Degree to which all desired functionality is implemented.
- **Correctness**: Degree to which the implemented functionality meets user expectations
- **Performant**: Degree to which the functionality performs as designed fast and without errors
- **Maintainable**: Degree to which the functionality is well implemented in source code

|                                                                                    | Complete | Correct | Performant | Maintainable |
| -----------------------------------------------------------------------            | -------- | ------- | ---------- | ------------ |
| [Tool Management](https://github.com/orgs/FreeCAD/projects/21/views/16)            |          |         |            |              |
| [Stock and work-piece Setup](https://github.com/orgs/FreeCAD/projects/21/views/17) |          |         |            |              |
| [Job Management](https://github.com/orgs/FreeCAD/projects/21/views/15)             |          |         |            |              |
| [Operation Configuration](https://github.com/orgs/FreeCAD/projects/21/views/11)                                                            |          |         |            |              |
| [Simulation and Verification](https://github.com/orgs/FreeCAD/projects/21/views/13)                                                        |          |         |            |              |
| [Output generation](https://github.com/orgs/FreeCAD/projects/21/views/14)                                                     |          |         |            |              |
| [Scripting and Automation](https://github.com/orgs/FreeCAD/projects/21/views/18)                                              |          |         |            |              |

# ⚠️ Pain Points
*(this section functions like an FAQ.  It helps keep users from creating duplicate issues.  It gives new developers a first place to connect)*

Perennial complaints from users:

- Visualization of first rapid move in operation makes it appear as though tool
  returns to origin between ops (It doesn't)
- Climb vs Conventional terminology not consistently used
- Safe Height vs Clearance Height confusing
- Arrays of similar gcode
- Lack of F & S calculation

# 🔥 Short-Term Goals (1-3 months)
| Work Unit | Why it should be a priority | Issue(s) |
| -------| -----------------------------| -----------|
| Handle linking moves consistently | abstracting the generation of linking moves (rapid moves into, out of, and between parts of a toolpath) to a generator would allow us to factor this logic out of the individual operations.  The ops would get more consistent and easier to maintain. It will allow us to build a more intuitive drilling operation.| #22599, #9405 #16205 |
| Pocketing Strategies| The adaptive clearing strategy need not be a separate operation type.  Factoring out the adaptive algorithm to a generator would allow us to move the adaptive operation into pocket and 3D pocket as a strategy. This would let users switch strategies without deleting and recreating whole operations. | |
| stabilize the BTL tool implementation| Incorporating the BTL approach to tools has caused some regressions. | #21430 #21855 #18598 #9466 #22228 |
| Document schema for CNC | Users are still confused by the warning dialog when creating a CAM Job. Improving the dialog should be possible to allow them to change the schema and set future defaults right at that time.| #22549, #21604|
| Overhaul the Entry dressup(s)| We currently hav both ramp and leadin/out dressup.  These could be consolidated into a single dressup with multiple strategies possible. Ramp lacks a UI task panel. Ramp also has a helical entry strategy that duplicates code from helix and from adaptive operation.  Adaptive operation has its own helix entry method.  The result is a confusing jumble of functionality and code that is hard to work on | #16897 #10621 #22137 #8150 #14380 #16144 |
| Helix work| Helices are used in at least three different places; Helix Op, Ramp entry, and Adaptive clearing.  This is a lot of duplicated code with inconsistent features. Centralizing the logic to a helix generator would allow us to first put it under unit tests and then incrmentally replace the duplicated logic in each place where it is used. This will provice a more robust solution that is easier to extend and maintain and also give the users a consistent set of features related to helices everywhere |#22469, #13455, #22357, #8149, #17737, PR #21971 |




# 🧱 Mid-Term Goals (3-12 months)
*(Mid term goals are probably 2+ feature releases in the future)*


# 🚀 Long-Term Goals
*(this section will be a list of links to large issues, FreeCAD Enhancement Proposals (FEP) or github discussions. Pre-development planning)*

    - Support for 5-axis milling
    - STEP-NC support
    - OMI

# 📢 How to Contribute

    Improve GitHub issues
    Unit Tests
    Code Reviews
    Resolve issues


# 🧠 Open Questions
    -
