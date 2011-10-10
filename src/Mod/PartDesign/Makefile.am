SUBDIRS=App Gui Scripts

# Create the file __init__.py so that we can import as
# from PartDesign.Scripts import ...
#install-exec-hook:
#		echo "import PartDesign" > $(DESTDIR)$(datadir)/__init__.py
#
#uninstall-hook:
#		rm -f $(DESTDIR)$(datadir)/__init__.py

# Change data dir from default ($(prefix)/share) to $(prefix)
datadir = $(prefix)/Mod/PartDesign

data_DATA = Init.py InitGui.py TestPartDesignGui.py TestPartDesignApp.py __init__.py

EXTRA_DIST = \
		$(data_DATA) \
		CMakeLists.txt \
		partdesign.dox

