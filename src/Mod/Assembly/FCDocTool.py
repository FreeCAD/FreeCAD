#! python
# -*- coding: utf-8 -*-
#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2012                                                    *  
#*   Juergen Riegel <FreeCAD@juergen-riegel.net>                           *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

__title__="FreeCAD document tool"
__author__ = "Juergen Riegel <FreeCAD@juergen-riegel.net>"
__url__ = "http://free-cad.sourceforge.net"

'''
General description:

    Command line tool and lib for exploring FreeCAD documents

User manual:

    TODO

How it works / how to extend:
	TODO
'''
import zipfile
from xml.dom.minidom import parse, parseString


class Document:
	""" Document representation """
	def __init__(self,DocFile):
		self.FileName = DocFile 
		self.ZFile = zipfile.ZipFile(DocFile,'r')
		DStr = self.ZFile.read('Document.xml')
		self.DDom = parseString(DStr)

	def fileInfo(self):
		ret = ''
		for i in self.ZFile.infolist():
			i += i.filename
			i += '\n'
		return ret
		

if __name__ == "__main__":
	from optparse import OptionParser

	parser = OptionParser()
	parser.add_option("-f", "--file", dest="filename",
					  help="write report to FILE", metavar="FILE")
	parser.add_option("-l", "--list",
					  action="store_false", dest="verbose", default=True,
					  help="don't print status messages to stdout")

	(options, args) = parser.parse_args()
	print (options,args)
	d = Document(args[0])
	
