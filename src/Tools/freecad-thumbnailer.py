#!/usr/bin/python

import sys, zipfile, md5
import getopt
import gnomevfs

opt,par = getopt.getopt(sys.argv[1:],'-s:')
inpfile = gnomevfs.get_local_path_from_uri(par[0])
#inpfile = par[0]
outfile = par[1]

#print "fcthumbnailer"
#print inpfile, outfile

try:
	zfile=zipfile.ZipFile(inpfile)
	files=zfile.namelist()
	#print files
	# check for meta-file if it's really a FreeCAD document
	if files[0] != "Document.xml":
		sys.exit(1)

	image="thumbnails/Thumbnail.png"
	if image in files:
		image=zfile.read(image)
	else:
		freecad=open("/usr/share/freecad/freecad-doc.png")
		image=freecad.read()

	thumb=open(outfile,"wb")
	thumb.write(image)
	thumb.close()

except:
	sys.exit(1)

