#!/usr/bin/env python
from bundlebuilder import buildapp 
    
buildapp(
    mainprogram = "Sc21Viewer.py",
    resources = ["English.lproj" ],
    nibname = "MainMenu",
)   
