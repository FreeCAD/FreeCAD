# Create HTML documentation from FreeCAD's Python modules and classes.
# (c) 2007 Werner Mayer
#
#***************************************************************************
#*   Copyright (c) 2007 Werner Mayer <werner.wm.mayer@gmx.de>              *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import pydoc, pkgutil, sys, os, dircache, zipfile

def generateDoc():
		# Get the path to the FreeCAD module relative to this directory
		toolspath = os.path.dirname(__file__)
		homepath = toolspath + '/../../'
		homepath = os.path.realpath(homepath)
		binpath = os.path.join(homepath, 'bin')
		docpath = os.path.join(homepath, 'doc')
		modpath = os.path.join(homepath, 'Mod')

		# Change to the doc directory
		cwd = os.getcwd()
		print 'Change to ' + docpath
		os.chdir(homepath)
		if os.path.exists('doc') == False:
				os.mkdir('doc')
		os.chdir('doc')

		# Add the bin path to the system path
		if os.name == 'nt':
				os.environ['PATH'] = os.environ['PATH'] + ';' + binpath
		else:
				os.environ['PATH'] = os.environ['PATH'] + ':' + binpath
	
		# Import FreeCAD module
		sys.path.append(binpath)
		print 'Write documentation for module \'FreeCAD\''
		pydoc.writedoc('FreeCAD')
		print ''
	
		# Module directory
		ModDirs = dircache.listdir(modpath)

		# Search for module paths and append them to Python path 
		#for Dir in ModDirs:
		#	if (Dir != '__init__.py'):
		#			sys.path.append( os.path.join(modpath,Dir) )

		# Walk through the module paths again and try loading the modules to create HTML files 
		for Dir in ModDirs:
				dest = os.path.join(modpath,Dir)
				print 'Write documentation for module \'' + Dir + '\''
				if (Dir != '__init__.py'):
						writedocs(dest)
						print ''
						
		# Now we must create a document and create instances of all Python classes which
		# cannot be directly created by a module.

		# Create a ZIP archive from all HTML files
		print 'Creating ZIP archive \'docs.zip\'...'
		zip = zipfile.ZipFile('docs.zip', 'w')
		for file in os.listdir('.'):
				if not os.path.isdir(file):
						if file.find('.html') > 0:
								print '  Adding file ' + file + ' to archive'
								zip.write(file)
	
		print 'done.'
		zip.close()
	
		# Remove all HTML files
		print 'Cleaning up HTML files...'
		for file in os.listdir('.'):
				if not os.path.isdir(file):
						if file.find('.html') > 0:
								print '  Removing ' + file
								os.remove(file)
		
		os.chdir(cwd)
		print 'done.'

def writedocs(dir, pkgpath='', done=None):
    """Write out HTML documentation for all modules in a directory tree."""
    if done is None: done = {}
    for importer, modname, ispkg in pkgutil.walk_packages([dir], pkgpath):
    		# Ignore all debug modules
    		if modname[-2:] != '_d':
        		pydoc.writedoc(modname)
    return

if __name__ == "__main__":
		generateDoc()
