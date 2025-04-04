import sys
import os
import subprocess
import platform
from datetime import datetime

import freecad
import FreeCAD

package_manager = "conda"
system = platform.platform().split("-")[0]
arch = platform.machine()

# Windows uses a different syntax
if arch == "AMD64":
    arch = "x86_64"

if "ARCH" in os.environ:
    if os.environ["ARCH"] != "":
        arch = os.environ["ARCH"]

python_version = platform.python_version().split(".")
python_version = "py" + python_version[0] + python_version[1]
date = str(datetime.now()).split(" ")[0]

version_info = FreeCAD.Version()
build_version_suffix = FreeCAD.ConfigGet("BuildVersionSuffix")
dev_version = version_info[0] + "." + version_info[1] + "." + version_info[2] + build_version_suffix
revision = version_info[3].split(" ")[0]

if system == "macOS":
    import jinja2
    print("create plist from template")
    osx_directory = os.path.join(os.path.dirname(__file__), "..", "osx")
    with open(os.path.join(osx_directory, "Info.plist.template")) as template_file:
        template_str = template_file.read()
    template = jinja2.Template(template_str)
    rendered_str = template.render( FREECAD_VERSION="{}-{}".format(dev_version, revision), 
                                    APPLICATION_MENU_NAME="FreeCAD-{}-{}".format(dev_version, revision) )
    with open(os.path.join(osx_directory, "FreeCAD.app", "Contents", "Info.plist"), "w") as rendered_file:
        rendered_file.write(rendered_str)

if "DEPLOY_RELEASE" in os.environ and os.environ["DEPLOY_RELEASE"] == "weekly-builds":
    dev_version = "weekly-builds"
    revision_separator = "-"
else:
    revision_separator = ""
    revision = ""

bundle_name = f"FreeCAD_{dev_version}{revision_separator}{revision}-{package_manager}-{system}-{arch}-{python_version}"

with open("bundle_name.txt", "w") as bundle_name_file:
    bundle_name_file.write(bundle_name)
