2020 February

These files provide read only resources for the workbench.
- `icons/`: SVG images to use as icons.
- `patterns/`: in SVG, to fill closed shapes.
- `translations/`: translation files generated from Qt tools and Crowdin.
- `ui/`: Qt user interface files made with QtCreator, and which connect
to the corresponding modules in `drafttaskpanels/`.

All files in this directory should be read-only, as they are not meant
to be executed. They are only meant to be read by the program.

The `Draft.qrc` file lists all resources. This file is used
to compile the resources into a single `Draft_rc.py` file
which can then be imported in the modules of the workbench
to provide the correct resource at run time.
