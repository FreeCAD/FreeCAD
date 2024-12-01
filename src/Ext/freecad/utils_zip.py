# (c) 2024 Werner Mayer LGPL

__author__ = "Werner Mayer"
__url__ = "https://www.freecad.org"
__doc__ = "Helper module to convert zip files"


import zipfile

def rewrite(source: str, target: str):
    source_zip = zipfile.ZipFile(source, "r")
    target_zip = zipfile.ZipFile(target, "w")

    for name in source_zip.namelist():
        target_zip.writestr(name, source_zip.open(name).read())

    source_zip.close()
    target_zip.close()
