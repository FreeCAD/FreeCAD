
# 🟩 Core Essentials
*Features present or expected in a basic CAM package and necessary to perform the required tasks*

| Feature              | Description                                                     | Assessment |
| -------------------- | --------------------------------------------------------------- | ---------- |
| G-code Generation    | Translate internal tool path to machine-specific G-code dialect | DONE       |
| Output Customization | Line numbers, comments, units (G20/G21)                         | DONE       |
| Output review & edit | After the gcode is generated, the user should have the option to review it and edit before saving. They should have the option of editing in an external editor of their choice | Output review is done.  Only uses internal editor which is poor |

---

# 🟨 Professional Grade
*Features usually present or expected in the state-of-the-art applications*

| Feature                      | Description                                                                                                                                  | Assessment                                                                                                          |
| ---------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------- |
| Preflight Checks             | Catch and flag obvious problems before generating output                                                                                     | Sanity check can catch some errors.  Requires running the check manually.                                           |
| Post-Processor Customization | Control modal vs explicit axes, tool change blocks, headers/footers                                                                          | Customization with flags in the Job output tab.  Posts are inconsistent.                                            |
| Advanced Customizatoin       | Allow customization of output beyond the trivial. Allow customizing the post with an editing/customization tool                              | Requires editing the python file.<br>Requires copying the post file to a specific location.  Clunky and unintuitive |
| Subprogram Support           | Generate G-code with subprograms and subroutines                                                                                             | NONE                                                                                                                |
| Setup Page Generation        | Instructions, checklists, warnings, and errors for the operator                                                                              | DONE                                                                                                                |
| G-code Decomposition         | Break arcs/canned cycles into linear segments or explicit moves                                                                              | NONE                                                                                                                |
| Coordinate Conversion        | Convert absolute to relative (G91), center arcs to relative (G91.1)                                                                          | NONE                                                                                                                |
| Coolant Control              | Coolant control should be started at the most desirable point to avoid wasting coolant during a tool change or before it is actually needed. | Current coolant control turns on when the TC is loaded.  Inefficient                                                |
| Advanced g-code generation   | It should be possible to write postprocessors to generate any valid gcode                                                                    | Some gcode features are not possible                                                                                |

---

# 🟦 Next-Level CAM
*Features that would exceed industry standard*

| Feature                       | Description                                               | Assessment |
| ----------------------------- | --------------------------------------------------------- | ---------- |
| On-Machine Inspection         | Generate code or triggers for probing/inspection routines | None       |
| Multi-File Output             | Support splitting G-code into multiple files              | LImited    |
| Tool Wear Compensation        | Output tool wear adjustments via offsets or tables        | None       |
| Feedback Loop Integration     | Closed-loop post processing using machine state           | None       |
| Direct-to-Machine Fabrication | Reimagine CAM → G-code → Machine as a seamless pipeline   | None       |
