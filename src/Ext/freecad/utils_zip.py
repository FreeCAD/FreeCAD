# (c) 2024 Werner Mayer LGPL

__author__ = "Werner Mayer"
__url__ = "https://www.freecad.org"
__doc__ = "Helper module to convert zip files"


import zipfile

def rewrite(source: str, target: str):
    with zipfile.ZipFile(source, "r") as source_zip, zipfile.ZipFile(target, "w") as target_zip:
        for name in source_zip.namelist():
            target_zip.writestr(name, source_zip.open(name).read())
