#!/usr/bin/python

print("Fetching download statistics from github...")

import requests

r = requests.get("https://api.github.com/repos/FreeCAD/FreeCAD/releases")
myobj = r.json()
for p in myobj:
    if "assets" in p:
        for asset in p["assets"]:
            print((asset["name"] + ": " + str(asset["download_count"]) + " downloads"))
