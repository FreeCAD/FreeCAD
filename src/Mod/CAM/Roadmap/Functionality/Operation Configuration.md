# Requirements

## 🟩 Core Essentials
*Features present or expected in a basic CAM package and necessary to perform the required tasks*

### All Operations

| Feature                           | Description                                        | Assessment                                                                                                                                                                                   |
| --------------------------------- | -------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Operation tool control Parameters | Configure spindle speed, feed rate, tool selection | tools are configured via the Tool Controller.  The functionality is COMPLETE but not intuitive.  Users expect to configure this within an operation and need to exit the op and edit the TC. |
| Operation Depth parameters        | start/end depth,                                   | COMPLETE.  <br>Terminology used throughout CAM is inconsistent.  <br>                                                                                                                        |

### Pocketing

| Feature             | Description                              | Assessment                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| ------------------- | ---------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Entry Strategies    | Provide helical and ramp entry           | Partially complete via dressup. <br>specific strategies are possible but limited and buggy                                                                                                                                                                                                                                                                                                                                                           |
| Entry Start Point   | Allow user to specify entry point        | Start point selection is poorly implemented.  It's not allowed at all in adaptive.<br>When used in other pocket ops (millface) only a single point is allowed.  If the pocket algorithm needs to retract and re-enter, it will use whichever point is needed.  <br><br>Pocket entry point should be studied to evaluate the current state-of-the-art.  Either remove it and let the op set start point automatically or reimplement it consistently.<br><br>Retain starting position from hole to hole when set. Angle in XY set by first hole. <br>
ie retain starting angle on XY plane as determined by first hole. <br>
Minimising rapids between holes gains nothing over all and leads to rather arbitrary starting points. |
| Clearing Strategies | zigzag, offset, line, grid, adaptive, Spiral                 | INCOMPLETE. Spiral pattern is missing and is desirable for some use cases                                                                                                                                                                                                                                                                                                                                                        |
| Stock to Leave      | Set material to leave for finishing pass | COMPLETE.  Terminology is inconsistent.                                                                                                                                                                                                                                                                                                                                                                                                              |
| Stepover Control    | Set amount of material removed per pass  | COMPLETE                                                                                                                                                                                                                                                                                                                                                                                                                                             |

### Contouring (Profile)

| Feature               | Description                         | Assessment                                                           |
| --------------------- | ----------------------------------- | -------------------------------------------------------------------- |
| Entry/Exit Strategies | Specify entry/exit paths            | Partially Complete via dressup. <br>strategies are limited and buggy |
| Stock to Leave        | Set material to leave for finishing | COMPLETE with inconsistent terminology                               |
| Holding Tags          | Add tags to retain part             | COMPLETE via dressup                                                 |

### Drilling Holes

| Feature                                | Description                                      | Assessment                   |
| -------------------------------------- | ------------------------------------------------ | ---------------------------- |
| Canned Cycles                          | Support G81, G82, G83, G73                       | COMPLETE                     |
| Multiple Targets                       | Handle multiple targets in one op                | COMPLETE                     |
| Drill Ordering                         | Allow user-defined target order. Don't ignore selected objects                 | Incomplete                   |
| Obstructed Faces                       | Support targets across intermediate obstructions | Incomplete                   |
| enable and disable indidvidual targets |                                                  | Requires deleting the target |
|                                        |                                                  |                              |
|                                        |                                                  |                              |

| Gcode | Canned Cycle          | Assessment |
| ----- | --------------------- | ---------- |
| G81   | Drilling Cycle        |  WORKS, excessive air cutting, see #22622          |
| G82   | Drilling Cycle, Dwell |  WORKS, excessive air cutting, see #22622          |
| G83   | Peck Drilling Cycle   |  WORKS, excessive air cutting, see #22622          |
| G73   | Chip Breaking Drill   |  WORKS, excessive air cutting, see #22622          |

### Engraving

| Feature            | Description                                                                                                                                               | Assessment |
| ------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------- |
| Engrave text       | Allow operating on a text                                                                                                                                 | DONE       |
| Engrave centerline | TrueType fonts usually don't work well for engraving.<br>Single-line fonts are rare and limited. Engraving software that follows font centerline is ideal | NONE       |
|                    |                                                                                                                                                           |            |

---

## 🟨 Professional Grade
*Features usually present or expected in the state-of-the-art applications*

### All Operations

| Feature                     | Description                                                | Assessment |
| --------------------------- | ---------------------------------------------------------- | ---------- |
| Model-Based Depth Selection | Set relevant depths by clicking features of model or stock | DONE       |
| Tool selection | Only allow selecting tools that function with the operation| INCOMPLETE|

### Pocketing

| Feature      | Description                                | Assessment                                            |
| ------------ | ------------------------------------------ | ----------------------------------------------------- |
| REST Milling | Target leftover material from previous ops | Partially implemented.<br>Buggy<br>Poor visualization |

### Contouring (Profile)

| Feature           | Description                          | Assessment |
| ----------------- | ------------------------------------ | ---------- |
| Multiple Passes   | Support multi-pass or kerf-widening  | NO         |
| Holding Tag Shape | Customize size/shape of holding tabs | DONE       |

### Engraving

| Feature | Description | Assessment |
|--------|-------------|------------|
| *(placeholder)* | *(To be defined)* | |

### Drilling Holes

| Feature            | Description                                   | Assessment |
| ------------------ | --------------------------------------------- | ---------- |
| Order Optimization | Automatically sort drill order for efficiency | NO         |
| Efficient Cycles   | Minimize wasted travel in canned cycles       | NO         |

#### Boring Cycles

| Gcode | Canned Cycle             | Assessment |
| ----- | ------------------------ | ---------- |
| G85   | Boring Cycle, Feed Out   |            |
| G86   | Spindle Stop, Rapid Out  |            |
| G87   | Back Boring Cycle        |            |
| G88   | Spindle Stop, Manual Out |            |
| G89   | Dwell, Feed Out          |            |

#### Tapping Cycles

| Gcode | Canned Cycle                                | Assessment |
|-------|---------------------------------------------|------------|
| G33.1 | Rigid Tapping                               | |
| G74   | Left-hand Tapping with Dwell                | |
| G84   | Right-hand Tapping with Dwell               | |

### Threadmilling

| Feature | Description | Assessment |
|--------|-------------|------------|
| non-synchronised theadmilling | WORKING | |

### V-Carving

| Feature                          | Description                                                                       | Assessment |
| -------------------------------- | --------------------------------------------------------------------------------- | ---------- |
| Compute accurate Vcarve toolpath | Use a V-shaped cutter at varying depth to create varying glyph width in the carve | DONE       |

### Deburring

| Feature | Description | Assessment |
|--------|-------------|------------|
| *(placeholder)* | *(To be defined)* | |

### Helical Clearing

| Feature | Description | Assessment |
|--------|-------------|------------|
| clear cyl. hole | needs outside helix profiling; refuses hole less than twice tool size | |

### Adaptive Clearing

| Feature | Description | Assessment |
|--------|-------------|------------|
| *(placeholder)* | *(To be defined)* | |

### Lathe Operations

| Feature | Description | Assessment |
|--------|-------------|------------|
| *(placeholder)* | *(To be defined)* | |

### Indexed Multi-Axis

| Feature | Description | Assessment |
|--------|-------------|------------|
| *(placeholder)* | *(To be defined)* | |

### Continuous 4th and 5th Axis

| Feature | Description | Assessment |
|--------|-------------|------------|
| *(placeholder)* | *(To be defined)* | |

---

## 🟦 Next-Level CAM
*Features that would exceed industry standard*

| Feature | Description | Assessment |
|--------|-------------|------------|
| framework to support extremely esoteric strategies | -Jeweling, fluting, guilloche | |
