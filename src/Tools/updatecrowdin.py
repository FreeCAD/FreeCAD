#!/usr/bin/env python3

# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2021 Benjamin Nauck <benjamin@nauck.se>                 *
# *   Copyright (c) 2021 Mattias Pierre <github@mattiaspierre.com>          *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""
This utility offers several commands to interact with the FreeCAD project on
crowdin. For it to work, you need a ~/.crowdin-freecad-token file in your
user's folder, that contains the API access token that gives access to the
crowdin FreeCAD project. The API token can also be specified in the
CROWDIN_TOKEN environment variable.

The CROWDIN_PROJECT_ID environment variable can be used to use this script
in other projects.

Usage:

    updatecrowdin.py <command> [<arguments>]

Available commands:

    gather:              update all ts files found in the source code
                         (runs updatets.py)
    status:              prints a status of the translations
    update / upload:     updates crowdin the current version of .ts files
                         found in the source code
    build:               builds a new downloadable package on crowdin with all
                         translated strings
    build-status:        shows the status of the current builds available on
                         crowdin
    download [build_id]: downloads build specified by 'build_id' or latest if
                         build_id is left blank
    apply / install:     applies downloaded translations to source code
                         (runs updatefromcrowdin.py)

Example:

    ./updatecrowdin.py update

Setting the project name adhoc:

    CROWDIN_PROJECT_ID=some_project ./updatecrowdin.py update
"""

# See crowdin API docs at https://crowdin.com/page/api

import concurrent.futures
import glob
import json
import os
import sys
import shutil
import subprocess
import tempfile
import zipfile
import re
from collections import namedtuple
from functools import lru_cache
from os.path import basename, splitext
from urllib.parse import quote_plus
from urllib.request import Request
from urllib.request import urlopen
from urllib.request import urlretrieve
from PySide2 import QtCore

TsFile = namedtuple("TsFile", ["filename", "src_path"])

LEGACY_NAMING_MAP = {"Draft.ts": "draft.ts"}

# Locations that require QM file generation (predominantly Python workbenches)
GENERATE_QM = {
    "AddonManager",
    "Arch",
    "Cloud",
    "Draft",
    "Inspection",
    "Material",
    "OpenSCAD",
    "Tux",
}

# locations list contains Module name, relative path to translation folder and relative path to qrc file

locations = [
    [
        "AddonManager",
        "../Mod/AddonManager/Resources/translations",
        "../Mod/AddonManager/Resources/AddonManager.qrc",
    ],
    ["App", "../App/Resources/translations", "../App/Resources/App.qrc"],
    ["Arch", "../Mod/Arch/Resources/translations", "../Mod/Arch/Resources/Arch.qrc"],
    [
        "draft",
        "../Mod/Draft/Resources/translations",
        "../Mod/Draft/Resources/Draft.qrc",
    ],
    ["Base", "../Base/Resources/translations", "../Base/Resources/Base.qrc"],
    [
        "Drawing",
        "../Mod/Drawing/Gui/Resources/translations",
        "../Mod/Drawing/Gui/Resources/Drawing.qrc",
    ],
    [
        "Fem",
        "../Mod/Fem/Gui/Resources/translations",
        "../Mod/Fem/Gui/Resources/Fem.qrc",
    ],
    ["FreeCAD", "../Gui/Language", "../Gui/Language/translation.qrc"],
    [
        "Inspection",
        "../Mod/Inspection/Gui/Resources/translations",
        "../Mod/Inspection/Gui/Resources/Inspection.qrc",
    ],
    [
        "Mesh",
        "../Mod/Mesh/Gui/Resources/translations",
        "../Mod/Mesh/Gui/Resources/Mesh.qrc",
    ],
    [
        "MeshPart",
        "../Mod/MeshPart/Gui/Resources/translations",
        "../Mod/MeshPart/Gui/Resources/MeshPart.qrc",
    ],
    [
        "OpenSCAD",
        "../Mod/OpenSCAD/Resources/translations",
        "../Mod/OpenSCAD/Resources/OpenSCAD.qrc",
    ],
    [
        "Part",
        "../Mod/Part/Gui/Resources/translations",
        "../Mod/Part/Gui/Resources/Part.qrc",
    ],
    [
        "PartDesign",
        "../Mod/PartDesign/Gui/Resources/translations",
        "../Mod/PartDesign/Gui/Resources/PartDesign.qrc",
    ],
    [
        "Path",
        "../Mod/Path/Gui/Resources/translations",
        "../Mod/Path/Gui/Resources/Path.qrc",
    ],
    [
        "Points",
        "../Mod/Points/Gui/Resources/translations",
        "../Mod/Points/Gui/Resources/Points.qrc",
    ],
    [
        "ReverseEngineering",
        "../Mod/ReverseEngineering/Gui/Resources/translations",
        "../Mod/ReverseEngineering/Gui/Resources/ReverseEngineering.qrc",
    ],
    [
        "Robot",
        "../Mod/Robot/Gui/Resources/translations",
        "../Mod/Robot/Gui/Resources/Robot.qrc",
    ],
    [
        "Sketcher",
        "../Mod/Sketcher/Gui/Resources/translations",
        "../Mod/Sketcher/Gui/Resources/Sketcher.qrc",
    ],
    [
        "Spreadsheet",
        "../Mod/Spreadsheet/Gui/Resources/translations",
        "../Mod/Spreadsheet/Gui/Resources/Spreadsheet.qrc",
    ],
    [
        "StartPage",
        "../Mod/Start/Gui/Resources/translations",
        "../Mod/Start/Gui/Resources/Start.qrc",
    ],
    [
        "Test",
        "../Mod/Test/Gui/Resources/translations",
        "../Mod/Test/Gui/Resources/Test.qrc",
    ],
    [
        "TechDraw",
        "../Mod/TechDraw/Gui/Resources/translations",
        "../Mod/TechDraw/Gui/Resources/TechDraw.qrc",
    ],
    ["Tux", "../Mod/Tux/Resources/translations", "../Mod/Tux/Resources/Tux.qrc"],
    [
        "Web",
        "../Mod/Web/Gui/Resources/translations",
        "../Mod/Web/Gui/Resources/Web.qrc",
    ],
]

THRESHOLD = 25  # how many % must be translated for the translation to be included in FreeCAD


class CrowdinUpdater:

    BASE_URL = "https://api.crowdin.com/api/v2"

    def __init__(self, token, project_identifier, multithread=True):
        self.token = token
        self.project_identifier = project_identifier
        self.multithread = multithread

    @lru_cache()
    def _get_project_id(self):
        url = f"{self.BASE_URL}/projects/"
        response = self._make_api_req(url)

        for project in [p["data"] for p in response]:
            if project["identifier"] == project_identifier:
                return project["id"]

        raise Exception("No project identifier found!")

    def _make_project_api_req(self, project_path, *args, **kwargs):
        url = f"{self.BASE_URL}/projects/{self._get_project_id()}{project_path}"
        return self._make_api_req(url=url, *args, **kwargs)

    def _make_api_req(self, url, extra_headers={}, method="GET", data=None):
        headers = {"Authorization": "Bearer " + load_token(), **extra_headers}

        if type(data) is dict:
            headers["Content-Type"] = "application/json"
            data = json.dumps(data).encode("utf-8")

        request = Request(url, headers=headers, method=method, data=data)
        return json.loads(urlopen(request).read())["data"]

    def _get_files_info(self):
        files = self._make_project_api_req("/files?limit=250")
        return {f["data"]["path"].strip("/"): str(f["data"]["id"]) for f in files}

    def _add_storage(self, filename, fp):
        response = self._make_api_req(
            f"{self.BASE_URL}/storages",
            data=fp,
            method="POST",
            extra_headers={
                "Crowdin-API-FileName": filename,
                "Content-Type": "application/octet-stream",
            },
        )
        return response["id"]

    def _update_file(self, project_id, ts_file, files_info):
        filename = quote_plus(ts_file.filename)

        with open(ts_file.src_path, "rb") as fp:
            storage_id = self._add_storage(filename, fp)

        if filename in files_info:
            file_id = files_info[filename]
            self._make_project_api_req(
                f"/files/{file_id}",
                method="PUT",
                data={
                    "storageId": storage_id,
                    "updateOption": "keep_translations_and_approvals",
                },
            )
            print(f"{filename} updated")
        else:
            self._make_project_api_req("/files", data={"storageId": storage_id, "name": filename})
            print(f"{filename} uploaded")

    def status(self):
        response = self._make_project_api_req("/languages/progress?limit=100")
        return [item["data"] for item in response]

    def download(self, build_id):
        filename = f"{self.project_identifier}.zip"
        response = self._make_project_api_req(f"/translations/builds/{build_id}/download")
        urlretrieve(response["url"], filename)
        print("download of " + filename + " complete")

    def build(self):
        self._make_project_api_req("/translations/builds", data={}, method="POST")

    def build_status(self):
        response = self._make_project_api_req("/translations/builds")
        return [item["data"] for item in response]

    def update(self, ts_files):
        files_info = self._get_files_info()
        futures = []

        with concurrent.futures.ThreadPoolExecutor() as executor:
            for ts_file in ts_files:
                if self.multithread:
                    future = executor.submit(
                        self._update_file, self.project_identifier, ts_file, files_info
                    )
                    futures.append(future)
                else:
                    self._update_file(self.project_identifier, ts_file, files_info)

        # This blocks until all futures are complete and will also throw any exception
        for future in futures:
            future.result()


def load_token():
    # load API token stored in ~/.crowdin-freecad-token
    config_file = os.path.expanduser("~") + os.sep + ".crowdin-freecad-token"
    if os.path.exists(config_file):
        with open(config_file) as file:
            return file.read().strip()
    return None


def updateqrc(qrcpath, lncode):

    "updates a qrc file with the given translation entry"

    # print("opening " + qrcpath + "...")

    # getting qrc file contents
    if not os.path.exists(qrcpath):
        print("ERROR: Resource file " + qrcpath + " doesn't exist")
        sys.exit()
    f = open(qrcpath, "r")
    resources = []
    for l in f.readlines():
        resources.append(l)
    f.close()

    # checking for existing entry
    name = "_" + lncode + ".qm"
    for r in resources:
        if name in r:
            # print("language already exists in qrc file")
            return

    # find the latest qm line
    pos = None
    for i in range(len(resources)):
        if ".qm" in resources[i]:
            pos = i
    if pos is None:
        print("No existing .qm file in this resource. Appending to the end position")
        for i in range(len(resources)):
            if "</qresource>" in resources[i]:
                pos = i - 1
    if pos is None:
        print("ERROR: couldn't add qm files to this resource: " + qrcpath)
        sys.exit()

    # inserting new entry just after the last one
    line = resources[pos]
    if ".qm" in line:
        line = re.sub("_.*\.qm", "_" + lncode + ".qm", line)
    else:
        modname = os.path.splitext(os.path.basename(qrcpath))[0]
        line = "        <file>translations/" + modname + "_" + lncode + ".qm</file>\n"
        # print "ERROR: no existing qm entry in this resource: Please add one manually " + qrcpath
        # sys.exit()
    # print("inserting line: ",line)
    resources.insert(pos + 1, line)

    # writing the file
    f = open(qrcpath, "w")
    for r in resources:
        f.write(r)
    f.close()
    print("successfully updated ", qrcpath)


def updateTranslatorCpp(lncode):

    "updates the Translator.cpp file with the given translation entry"

    cppfile = os.path.join(os.path.dirname(__file__), "..", "Gui", "Language", "Translator.cpp")
    l = QtCore.QLocale(lncode)
    lnname = l.languageToString(l.language())

    # read file contents
    f = open(cppfile, "r")
    cppcode = []
    for l in f.readlines():
        cppcode.append(l)
    f.close()

    # checking for existing entry
    lastentry = 0
    for i, l in enumerate(cppcode):
        if l.startswith("    d->mapLanguageTopLevelDomain[QT_TR_NOOP("):
            lastentry = i
            if '"' + lncode + '"' in l:
                # print(lnname+" ("+lncode+") already exists in Translator.cpp")
                return

    # find the position to insert
    pos = lastentry + 1
    if pos == 1:
        print("ERROR: couldn't update Translator.cpp")
        sys.exit()

    # inserting new entry just before the above line
    line = '    d->mapLanguageTopLevelDomain[QT_TR_NOOP("' + lnname + '")] = "' + lncode + '";\n'
    cppcode.insert(pos, line)
    print(lnname + " (" + lncode + ") added Translator.cpp")

    # writing the file
    f = open(cppfile, "w")
    for r in cppcode:
        f.write(r)
    f.close()


def doFile(tsfilepath, targetpath, lncode, qrcpath):

    "updates a single ts file, and creates a corresponding qm file"

    basename = os.path.basename(tsfilepath)[:-3]
    # filename fixes
    if basename + ".ts" in LEGACY_NAMING_MAP.values():
        basename = list(LEGACY_NAMING_MAP)[
            list(LEGACY_NAMING_MAP.values()).index(basename + ".ts")
        ][:-3]
    newname = basename + "_" + lncode + ".ts"
    newpath = targetpath + os.sep + newname
    shutil.copyfile(tsfilepath, newpath)
    if basename in GENERATE_QM:
        print(f"Generating QM for {basename}")
        try:
            subprocess.run(
                [
                    "lrelease",
                    newpath,
                ],
                timeout=5,
            )
        except Exception as e:
            print(e)
        newqm = targetpath + os.sep + basename + "_" + lncode + ".qm"
        if not os.path.exists(newqm):
            print("ERROR: failed to create " + newqm + ", aborting")
            sys.exit()
        updateqrc(qrcpath, lncode)


def doLanguage(lncode):

    "treats a single language"

    if lncode == "en":
        # never treat "english" translation... For now :)
        return
    prefix = ""
    suffix = ""
    if os.name == "posix":
        prefix = "\033[;32m"
        suffix = "\033[0m"
    print("Updating files for " + prefix + lncode + suffix + "...", end="")
    for target in locations:
        basefilepath = os.path.join(tempfolder, lncode, target[0] + ".ts")
        targetpath = os.path.abspath(target[1])
        qrcpath = os.path.abspath(target[2])
        doFile(basefilepath, targetpath, lncode, qrcpath)
    print(" done")


def applyTranslations(languages):

    global tempfolder
    currentfolder = os.getcwd()
    tempfolder = tempfile.mkdtemp()
    print("creating temp folder " + tempfolder)
    src = os.path.join(currentfolder, "freecad.zip")
    dst = os.path.join(tempfolder, "freecad.zip")
    if not os.path.exists(src):
        print('freecad.zip file not found! Aborting. Run "download" command before this one.')
        sys.exit()
    shutil.copyfile(src, dst)
    os.chdir(tempfolder)
    zfile = zipfile.ZipFile("freecad.zip")
    print("extracting freecad.zip...")
    zfile.extractall()
    os.chdir(currentfolder)
    for ln in languages:
        if not os.path.exists(os.path.join(tempfolder, ln)):
            print("ERROR: language path for " + ln + " not found!")
        else:
            doLanguage(ln)


if __name__ == "__main__":
    command = None

    args = sys.argv[1:]
    if args:
        command = args[0]

    token = os.environ.get("CROWDIN_TOKEN", load_token())
    if command and not token:
        print("Token not found")
        sys.exit()

    project_identifier = os.environ.get("CROWDIN_PROJECT_ID")
    if not project_identifier:
        project_identifier = "freecad"
        # print('CROWDIN_PROJECT_ID env var must be set')
        # sys.exit()

    updater = CrowdinUpdater(token, project_identifier)

    if command == "status":
        status = updater.status()
        status = sorted(status, key=lambda item: item["translationProgress"], reverse=True)
        print(
            len([item for item in status if item["translationProgress"] > THRESHOLD]),
            " languages with status > " + str(THRESHOLD) + "%:",
        )
        print("    ")
        sep = False
        prefix = ""
        suffix = ""
        if os.name == "posix":
            prefix = "\033[;32m"
            suffix = "\033[0m"
        for item in status:
            if item["translationProgress"] > 0:
                if (item["translationProgress"] < THRESHOLD) and (not sep):
                    print("    ")
                    print("Other languages:")
                    print("    ")
                    sep = True
                print(
                    prefix
                    + item["languageId"]
                    + suffix
                    + " "
                    + str(item["translationProgress"])
                    + "% ("
                    + str(item["approvalProgress"])
                    + "% approved)"
                )
                # print(f"  translation progress: {item['translationProgress']}%")
                # print(f"  approval progress:    {item['approvalProgress']}%")

    elif command == "build-status":
        for item in updater.build_status():
            print(f"  id: {item['id']} progress: {item['progress']}% status: {item['status']}")

    elif command == "build":
        updater.build()

    elif command == "download":
        if len(args) == 2:
            updater.download(args[1])
        else:
            stat = updater.build_status()
            if not stat:
                print("no builds found")
            elif len(stat) == 1:
                updater.download(stat[0]["id"])
            else:
                print("available builds:")
                for item in stat:
                    print(
                        f"  id: {item['id']} progress: {item['progress']}% status: {item['status']}"
                    )
                print("please specify a build id")

    elif command in ["update", "upload"]:
        # Find all ts files. However, this contains the lang-specific files too. Let's drop those
        all_ts_files = glob.glob("../**/*.ts", recursive=True)
        # Remove the file extensions
        ts_files_wo_ext = [splitext(f)[0] for f in all_ts_files]
        # Filter out any file that has another file as a substring. E.g. Draft is a substring of Draft_en
        main_ts_files = list(
            filter(
                lambda f: not [a for a in ts_files_wo_ext if a in f and f != a],
                ts_files_wo_ext,
            )
        )
        # Create tuples to map Crowdin name with local path name
        names_and_path = [(f"{basename(f)}.ts", f"{f}.ts") for f in main_ts_files]
        # Accommodate for legacy naming
        ts_files = [
            TsFile(LEGACY_NAMING_MAP[a] if a in LEGACY_NAMING_MAP else a, b)
            for (a, b) in names_and_path
        ]
        updater.update(ts_files)

    elif command in ["apply", "install"]:
        print("retrieving list of languages...")
        status = updater.status()
        status = sorted(status, key=lambda item: item["translationProgress"], reverse=True)
        languages = [
            item["languageId"] for item in status if item["translationProgress"] > THRESHOLD
        ]
        applyTranslations(languages)
        print("Updating Translator.cpp...")
        for ln in languages:
            updateTranslatorCpp(ln)

    elif command == "updateTranslator":
        print("retrieving list of languages...")
        status = updater.status()
        status = sorted(status, key=lambda item: item["translationProgress"], reverse=True)
        languages = [
            item["languageId"] for item in status if item["translationProgress"] > THRESHOLD
        ]
        print("Updating Translator.cpp...")
        for ln in languages:
            updateTranslatorCpp(ln)

    elif command == "gather":
        import updatets

        updatets.main()

    else:
        print(__doc__)
