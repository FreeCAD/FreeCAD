# 📍 FreeCAD CAM Workbench Roadmap – 2025 and Beyond

This document aims to guide the ongoing development of the CAM Workbench. It sets out shared goals, identifies pain points and opportunities, and invites discussion, collaboration, and contributions from the wider community.

FreeCAD is a volunteer-driven project. This roadmap does not prescribe work or enforce deadlines — it is a living document to help focus effort and encourage alignment across developers, users, and stakeholders.

# 🔭 Vision

A robust, intuitive, and industry-ready CAM solution that:

- Integrates seamlessly with the FreeCAD modeling workflows
- Supports professional CNC workflows
- Is usable and intuitive for hobbyist and non-professionals
- Embraces open standards 
- Encourages extensibility and script-ability via Python

## ⚙️ Functionality

Any CAM application meeting the needs above must provide functionality in these areas:
- [Tool Management](<./Tool Management.md>)
- [Stock and Work-piece Setup](<./Stock and Work-piece Setup.md>)
- [Job Management](<./Job Management.md>)
- [Operation Configuration](<./Operation Configuration.md>)
- [Simulation and Verification](<./Simulation and Verification.md>)
- [Output Generation](<./Output Generation.md>)
- [Scripting and automation](<./Scripting and automation.md>)
- [Documentation](<./User Interface & Documentation.md>)

## 🧼 Code Quality
Additionally, as software developers we desire the source code to excel in these areas:
- Modular
- DRY
- Extensible
- Testable

# 📊 Current State
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
| [Documentation](https://github.com/orgs/FreeCAD/projects/21/views/19)                                        |          |         |            |              |

# ⚠️ Pain Points

Perennial complaints from users:

- Visualization of first rapid move in operation makes it appear as though tool
  returns to origin between ops (It doesn't)
- Climb vs Conventional terminology not consistently used
- Safe Height vs Clearance Height confusing
- Arrays of similar gcode
- Lack of F & S calculation

# 🔥 Short-Term Goals (6–12 months)
 -

# 🧱 Mid-Term Goals (12–24 months)
 -

# 🚀 Long-Term Goals
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


