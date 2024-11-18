#! python
# -*- coding: utf-8 -*-
# (c) 2003 Werner Mayer LGPL
# Create a new application module

import os, sys
import MakeAppTools


if len(sys.argv) != 2:
    sys.stdout.write("Please enter a name for your application.\n")
    sys.exit()

Application = sys.argv[1]

# create directory ../Mod/<Application>
if not os.path.isdir("../Mod/" + Application):
    os.mkdir("../Mod/" + Application)
else:
    sys.stdout.write(Application + " already exists. Please enter another name.\n")
    sys.exit()


# copying files from _TEMPLATE_ to ../Mod/<Application>
sys.stdout.write("Copying files...")
MakeAppTools.copyTemplate("_TEMPLATE_", "../Mod/" + Application, "_TEMPLATE_", Application)
sys.stdout.write("Ok\n")

# replace the _TEMPLATE_ string by <Application>
sys.stdout.write("Modifying files...\n")
MakeAppTools.replaceTemplate("../Mod/" + Application, "_TEMPLATE_", Application)
# make the configure script executable
# os.chmod("../Mod/" + Application + "/configure", 0777);
sys.stdout.write("Modifying files done.\n")

sys.stdout.write(Application + " module created successfully.\n")
