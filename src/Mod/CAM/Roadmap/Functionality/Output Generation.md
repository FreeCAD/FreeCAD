
# 🟩 Core Essentials
*Features present or expected in a basic CAM package and necessary to perform the required tasks*

| Feature | Description | Assessment |
|--------|-------------|------------|
| G-code Generation | Translate internal tool path to machine-specific G-code dialect | |
| Output Customization | Line numbers, comments, units (G20/G21) | |

---

# 🟨 Professional Grade
*Features usually present or expected in the state-of-the-art applications*

| Feature | Description | Assessment |
|--------|-------------|------------|
| Preflight Checks | Catch and flag obvious problems before generating output | |
| Post-Processor Customization | Control modal vs explicit axes, tool change blocks, headers/footers | |
| Subprogram Support | Generate G-code with subprograms and subroutines | |
| Setup Page Generation | Instructions, checklists, warnings, and errors for the operator | |
| G-code Decomposition | Break arcs/canned cycles into linear segments or explicit moves | |
| Coordinate Conversion | Convert absolute to relative (G91), center arcs to relative (G91.1) | |

---

# 🟦 Next-Level CAM
*Features that would exceed industry standard*

| Feature | Description | Assessment |
|--------|-------------|------------|
| On-Machine Inspection | Generate code or triggers for probing/inspection routines | |
| Multi-File Output | Support splitting G-code into multiple files | |
| Tool Wear Compensation | Output tool wear adjustments via offsets or tables | |
| Feedback Loop Integration | Closed-loop post processing using machine state | |
| Direct-to-Machine Fabrication | Reimagine CAM → G-code → Machine as a seamless pipeline | |
