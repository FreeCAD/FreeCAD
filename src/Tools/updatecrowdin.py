#!/usr/bin/env python3
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2021 Benjamin Nauck <benjamin@nauck.se>                 *
# *   Copyright (c) 2021 Mattias Pierre <github@mattiaspierre.com>          *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Library General Public License (LGPL)   *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""
This utility offers several commands to interact with the FreeCAD project on crowdin.
For it to work, you need a ~/.crowdin-freecad-token file in your user's folder, that contains
the API access token that gives access to the crowdin FreeCAD project.
The API token can also be specified in the CROWDIN_TOKEN environment variable.

The CROWDIN_PROJECT_ID environment variable must be set.

Usage:

    updatecrowdin.py <command> [<arguments>]

Available commands:

    status:              prints a status of the translations
    update:              updates crowdin the current version of .ts files found in the source code
    build:               builds a new downloadable package on crowdin with all translated strings
    download [build_id]: downloads build specified by 'build_id' or latest if build_id is left blank

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
from collections import namedtuple
from functools import lru_cache
from os.path import basename, splitext
from urllib.parse import quote_plus
from urllib.request import Request
from urllib.request import urlopen
from urllib.request import urlretrieve

TsFile = namedtuple('TsFile', ['filename', 'src_path'])

LEGACY_NAMING_MAP = {'Draft.ts': 'draft.ts'}


class CrowdinUpdater:

    BASE_URL = 'https://api.crowdin.com/api/v2'

    def __init__(self, token, project_identifier, multithread=True):
        self.token = token
        self.project_identifier = project_identifier
        self.multithread = multithread

    @lru_cache()
    def _get_project_id(self):
        url = f'{self.BASE_URL}/projects/'
        response = self._make_api_req(url)

        for project in [p['data'] for p in response]:
            if project['identifier'] == project_identifier:
                return project['id']

        raise Exception('No project identifier found!')

    def _make_project_api_req(self, project_path, *args, **kwargs):
        url = f'{self.BASE_URL}/projects/{self._get_project_id()}{project_path}'
        return self._make_api_req(url=url, *args, **kwargs)

    def _make_api_req(self, url, extra_headers={}, method='GET', data=None):
        headers = {'Authorization': 'Bearer ' + load_token(), **extra_headers}

        if type(data) is dict:
            headers['Content-Type'] = 'application/json'
            data = json.dumps(data).encode('utf-8')

        request = Request(url, headers=headers, method=method, data=data)
        return json.loads(urlopen(request).read())['data']

    def _get_files_info(self):
        files = self._make_project_api_req('/files?limit=250')
        return {f['data']['path'].strip('/'): str(f['data']['id']) for f in files}

    def _add_storage(self, filename, fp):
        response = self._make_api_req(f'{self.BASE_URL}/storages', data=fp, method='POST', extra_headers={
            'Crowdin-API-FileName': filename,
            'Content-Type': 'application/octet-stream'
        })
        return response['id']

    def _update_file(self, project_id, ts_file, files_info):
        filename = quote_plus(ts_file.filename)

        with open(ts_file.src_path, 'rb') as fp:
            storage_id = self._add_storage(filename, fp)

        if filename in files_info:
            file_id = files_info[filename]
            self._make_project_api_req(f'/files/{file_id}', method='PUT', data={
                'storageId': storage_id,
                'updateOption': 'keep_translations_and_approvals'
            })
            print(f'{filename} updated')
        else:
            self._make_project_api_req('/files', data={
                'storageId': storage_id,
                'name': filename
            })
            print(f'{filename} uploaded')

    def status(self):
        response = self._make_project_api_req('/languages/progress')
        return [item['data'] for item in response]

    def download(self, build_id):
        filename = f'{self.project_identifier}.zip'
        response = self._make_project_api_req(f'/translations/builds/{build_id}/download')
        urlretrieve(response['url'], filename)
        print('download complete')

    def build(self):
        self._make_project_api_req('/translations/builds', data={}, method='POST')

    def build_status(self):
        response = self._make_project_api_req('/translations/builds')
        return [item['data'] for item in response]

    def update(self, ts_files):
        files_info = self._get_files_info()
        futures = []

        with concurrent.futures.ThreadPoolExecutor() as executor:
            for ts_file in ts_files:
                if self.multithread:
                    future = executor.submit(self._update_file, self.project_identifier, ts_file, files_info)
                    futures.append(future)
                else:
                    self._update_file(self.project_identifier, ts_file, files_info)

        # This blocks until all futures are complete and will also throw any exception
        for future in futures:
            future.result()

def load_token():
    # load API token stored in ~/.crowdin-freecad-token
    config_file = os.path.expanduser('~')+os.sep+".crowdin-freecad-token"
    if os.path.exists(config_file):
        with open(config_file) as file:
            return file.read().strip()
    return None


if __name__ == "__main__":
    command = None

    args = sys.argv[1:]
    if args:
        command = args[0]

    token = os.environ.get('CROWDIN_TOKEN', load_token())
    if command and not token:
        print('Token not found')
        sys.exit()

    project_identifier = os.environ.get('CROWDIN_PROJECT_ID')
    if not project_identifier:
        print('CROWDIN_PROJECT_ID env var must be set')
        sys.exit()

    updater = CrowdinUpdater(token, project_identifier)

    if command == "status":
        status = updater.status()
        for item in status:
            print(f"language: {item['languageId']}")
            print(f"  translation progress: {item['translationProgress']}%")
            print(f"  approval progress: {item['approvalProgress']}%")

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
                print('no builds found')
            elif len(stat) == 1:
                updater.download(stat[0]['id'])
            else:
                print('available builds:')
                for item in stat:
                    print(f"  id: {item['id']} progress: {item['progress']}% status: {item['status']}")
                print('please specify a build id')

    elif command == "update":
        # Find all ts files. However, this contains the lang-specific files too. Let's drop those
        all_ts_files = glob.glob('../**/*.ts', recursive=True)
        # Remove the file extensions
        ts_files_wo_ext = [splitext(f)[0] for f in all_ts_files]
        # Filter out any file that has another file as a substring. E.g. Draft is a substring of Draft_en
        main_ts_files = list(filter(lambda f: not [a for a in ts_files_wo_ext if a in f and f != a], ts_files_wo_ext))
        # Create tuples to map Crowdin name with local path name
        names_and_path = [(f'{basename(f)}.ts', f'{f}.ts') for f in main_ts_files]
        # Accomodate for legacy naming
        ts_files = [TsFile(LEGACY_NAMING_MAP[a] if a in LEGACY_NAMING_MAP else a, b) for (a, b) in names_and_path]

        updater.update(ts_files)
    else:
        print(__doc__)
