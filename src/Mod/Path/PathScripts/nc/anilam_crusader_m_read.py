# Preliminary backplot support for Anilam Crusader M CNC controller
# This code modified from iso_read.py and emc2_read.py distriuted with HeeksCAD as of Sep 2010
# Kurt Jensen 6 Sep 2010
# Use at your own risk.
import iso_read as iso
import sys

# Override some iso parser methods to interpret arc centers as relative to origin, not relative to start of arc.

class Parser(iso.Parser):
    def __init__(self, writer):
        iso.Parser.__init__(self, writer)
        self.arc_centre_absolute = True
