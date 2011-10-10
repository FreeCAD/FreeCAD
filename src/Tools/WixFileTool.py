#! python
# (c) 2005 Juergen Riegel

Usage = """
WIX file lister
Usage:
   WixFileTool [Optionen] srcDir destDir
Optionen:
 
Author:
  (c) 2005 Juergen Riegel
  juergen.riegel@web.de

Version:
  0.1
"""

import os,sys,string,re,glob

def ProcessDir(dummy,dirname,filesindir):
  print dirname

if __name__ == '__main__':
  if(len(sys.argv) < 3):
    sys.stdout.write(Usage)
    sys.exit(1)
  else:
    i=0
    for name in glob.glob(sys.argv[1]):
      i=i+1
      print "<File Id='DLL%d'    Name='ERR%d' LongName='%s' src='%s' DiskId='1' />" % (i,i,name,sys.argv[2])

        
