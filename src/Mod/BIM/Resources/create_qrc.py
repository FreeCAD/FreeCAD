#!/usr/bin/python

import os
txt = "<RCC>\n    <qresource>\n"
cdir = os.path.dirname(__file__)
for subdir in ["geometry", "icons", "icons/IFC", "translations", "ui"]:
    subpath = os.path.join(cdir, subdir)
    for f in sorted(os.listdir(subpath)):
        if f not in ["Arch.ts", "BIM.ts", "IFC"]:
            ext = os.path.splitext(f)[1]
            if ext.lower() in [".qm", ".svg", ".ui", ".png", "brep"]:
                txt += "        <file>" + subdir + "/" + f + "</file>\n"
txt += "    </qresource>\n</RCC>\n"
with open(os.path.join(cdir, "Arch.qrc"), "w") as resfile:
    resfile.write(txt)
