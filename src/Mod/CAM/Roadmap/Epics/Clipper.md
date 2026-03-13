# STATUS:

Active

# Why it is a priority

Our geometry libraries limit what we can do with CAM. Currently, we are
dependent on an obsolete version of Clipper and unmaintened libarea and kurve
libraries. These libraries do not receive feature updates or bug fixes.
Updating to Clipper 2 and removing our dependency on these libraries will make
the CAM workbench more maintainable and improve tool paths by enabling new
features such as non-heuristic arc fitting.


# Scope

This epic is about upgrading from Clipper 1/libarea/kurve to Clipper 2, and the CAM improvements enabled by that upgrade.

| In  | Out |
| --- | --- |
| Add Clipper 2 to FreeCAD | Research other new libraries to add to FreeCAD |
| Migrate operations from Clipper 1 to Clipper 2 |  |
| Replace uses of libarea and kurve with Clipper 2, and as needed, custom wrappers |  |
| Remove libarea and kurve from the FreeCAD | |
| Expose Clipper 2 functionality to Python |  |
| Implement correct curve fitting using Clipper 2's IntPoint user data feature |  |
| Implement algorithm/performance improvements in Adaptive using Clipper 2 features|  |

# Related Epics
