#SUBDIRS=App Gui

# Change data dir from default ($(prefix)/share) to $(prefix)
datadir = $(prefix)/Mod/OpenSCAD
data_DATA = \
	    Init.py InitGui.py \
	    OpenSCAD_rc.py \
	    OpenSCAD2Dgeom.py \
	    OpenSCADFeatures.py \
	    OpenSCADUtils.py \
	    OpenSCADCommands.py \
	    exportCSG.py \
	    importCSG.py \
	    tokrules.py \
	    colorcodeshapes.py \
	    expandplacements.py \
	    replaceobj.py

nobase_data_DATA = \
		   ply/lex.py \
		   ply/README \
		   ply/yacc.py \
		   ply/__init__.py


EXTRA_DIST = \
		$(data_DATA) $(nobase_data_DATA) \
		CMakeLists.txt \
		OpenSCAD.dox \
		exportVersions.txt \
		importVersions.txt 
