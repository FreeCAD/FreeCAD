# 🟩 Core Essentials
*Features present or expected in a basic CAM package and necessary to perform the required tasks*

- Translate the internal tool path representation to G-code
	- Generate Machine-specific gcode dialect
- Allow customization of output
	- line numbers
	- comments
	- Output gcode in either mm or inches (G20/G21)

# 🟨 Professional Grade
*Features usually present or expected in the state-of-the art applications*

- Catch and flag obvious problems before generating output
- Customize the Post-processor
	- Modal vs explicit axis output
    - Tool change pre/post blocks
    - Custom header/footer
- Generate gcode with subprograms and subroutines
- Produce a 'setup page' for machine operator
	- setup instructions
	- check list
	- warnings and errors


# 🟦 Next-Level CAM
*Features that would exceed industry standard*

- On Machine inspection
- Support splitting gcode into multiple files
- Automatic tool wear compensation output (D offsets or tool tables)
- Integration with machine feedback loop (closed-loop post)
- Direct-to-machine fabrication.  The entire pipeline of CAM->Gcode->machine
  should be reconsidered.  The workflow should feel more like 3D printing with a direct closed loop process
