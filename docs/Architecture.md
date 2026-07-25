# MbDFEM Architecture

## Purpose

This document describes the overall software architecture of the MbDFEM
(Assembly + Multibody Dynamics + Finite Element Analysis) workbench for
FreeCAD.

It is intended to remain stable throughout the lifetime of the project.

---

# Vision

MbDFEM extends FreeCAD with a tightly integrated environment capable of

- Mechanical assembly
- Multibody dynamics (MbD)
- Finite element analysis (FEM)
- Coupled MbD/FEM simulation
- Visualization
- Post-processing

while preserving FreeCAD's document model.

---

# Design Principles

## 1. Native FreeCAD Objects

Every user-visible object should be a FreeCAD DocumentObject.

Avoid maintaining duplicate object trees.

---

## 2. Separate Model and GUI

Maintain a strict separation between

- App
- Gui

The App module contains all engineering data.

The Gui module only displays and edits that data.

---

## 3. Composition over Inheritance

Represent engineering systems using composition.

Example

Assembly
    contains Parts

Part
    contains Bodies

Body
    contains Features

---

## 4. Solver Independence

MbDFEM should not depend on a single solver.

Possible back ends include

- MBDyn
- CalculiX
- Chrono
- OpenSees
- Future solvers

The workbench owns the engineering model, not the solver.

---

# Overall Architecture

```
FreeCAD Document
│
├── MbDAssembly
│   ├── MbDPart
│   ├── MbDJoint
│   ├── MbDForce
│   ├── MbDMarker
│   └── MbDSolver
│
└── FEMModel
    ├── Materials
    ├── Constraints
    ├── Loads
    └── Mesh
```

---

# Major Components

## MbDAssembly

Top-level multibody system.

Responsibilities

- owns all MbD objects
- manages simulation
- stores global settings

Suggested base class

```
App::DocumentObjectGroup
```

---

## MbDPart

Represents one rigid body.

Stores

- mass
- inertia
- reference frame
- graphics
- geometry links

---

## MbDJoint

Represents a kinematic constraint.

Examples

- Revolute
- Prismatic
- Cylindrical
- Universal
- Ball
- Fixed

---

## MbDMarker

Reference coordinate system attached to a body.

Used by

- joints
- loads
- sensors

---

## MbDForce

External force or torque.

Examples

- Gravity
- Spring
- Damper
- User function
- Actuator

---

## MbDSolver

Stores simulation parameters.

Examples

- time step
- end time
- integrator
- tolerances

Responsible for launching the external solver.

---

## FEMModel

Represents the finite element portion of the model.

Should reuse as much of the existing FEM workbench as possible.

---

# Object Relationships

```
Assembly
│
├── Parts
│      │
│      ├── Markers
│      └── Geometry
│
├── Joints
│
├── Forces
│
└── Solver
```

---

# Document Structure

Every engineering object is stored directly inside the FreeCAD document.

Relationships use

- App::PropertyLink
- App::PropertyLinkList

Avoid duplicate ownership trees.

---

# Dependency Graph

FreeCAD already computes execution order from PropertyLinks.

MbDFEM should leverage this mechanism rather than introducing a second graph.

---

# GUI Classes

Each App object has a corresponding ViewProvider.

Example

```
MbDAssembly
        │
        ▼
ViewProviderMbDAssembly

MbDPart
        │
        ▼
ViewProviderMbDPart
```

Responsibilities

- icons
- tree display
- colors
- selection
- visualization

---

# Simulation Pipeline

```
CAD Model

↓

Assembly

↓

Mass Properties

↓

Joints

↓

Solver Input

↓

External Solver

↓

Results

↓

Visualization

↓

Animation

↓

Stress Recovery

↓

Post-processing
```

---

# Future Extensions

Possible future capabilities

- Flexible bodies
- Contact
- Collision detection
- Control systems
- Optimization
- Design studies
- Co-simulation
- Real-time simulation
- GPU acceleration
- AI-assisted model creation

---

# Coding Guidelines

- Modern C++
- RAII
- Smart pointers where appropriate
- const correctness
- Small classes
- Single Responsibility Principle
- Unit-tested algorithms
- Minimal global state

---

# Long-Term Goals

1. Functional MbD workbench

2. MbD/FEM coupling

3. Plugin solver architecture

4. High-performance simulation

5. Commercial-quality engineering environment

6. Contribution back to FreeCAD where appropriate

---

# Living Document

This document should evolve with the project.

Architectural decisions should be recorded here before implementation whenever practical.