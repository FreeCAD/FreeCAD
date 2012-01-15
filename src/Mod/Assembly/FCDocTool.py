#! python
# -*- coding: utf-8 -*-
# (c) 2007 Juergen Riegel LGPL

import zipfile

class Document:
	""" Document representation """
	def __init__(self,DocFile):
		self.FileName = DocFile 
		print "Parsing: ",DocFile
		self.ZFile = zipfile.ZipFile(DocFile,'r')
		DStr = self.ZFile.read('Document.xml')
		print DStr

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
	
