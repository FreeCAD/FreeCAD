#!/usr/bin/python
import requests

print("Fetching download statistics from github...")
r = requests.get("https://api.github.com/repos/FreeCAD/FreeCAD/releases")
[
    [print(f"{asset['name']}: {str(asset['download_count'])} downloads") for asset in p["assets"]]
    for p in r.json()
    if "assets" in p
]
