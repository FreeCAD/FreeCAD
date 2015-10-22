#! python
# -*- coding: utf-8 -*-
# (c) 2007 Jürgen Riegel  GPL

Usage = """BuildRelease - Build script to build a complete FreeCAD release

Usage:
   BuildRelease [Optionen] ReleaseNbr
   
Options:
 -h, --help          print this help
 -b, --buildPath     specify the output path where the build takes place
 -i, --ini-file      specify the ini file to use
 
This script will build a complete FreeCAD distribution which includes:
* Check out fresh source
* packing source
* Set the Version and Release numbers
* Gathering change log
* completele build FreeCAD
* run tests
* build source docu
* build user docu
* build installer
* upload to source forge 
   
On failure of one of these steps the script will stop.
Each step writes tones of info in the log file.
There is one error log file.

Autor:
  (c) 2007 Juergen Riegel
  juergen.riegel@web.de
	Licence: GPL

Version:
  0.1
"""
#  
# Its inteded only to used by the maintainer

import os, sys, getopt
from subprocess import call,Popen,PIPE
from time import sleep
from zipfile import ZipFile,ZIP_DEFLATED
import tarfile
from string import find
import ConfigParser
import time


# global information 
Release = 0
Major = 0
Minor = 7
Alias = ""
FileName = ""
BuildPath = "D:/ReleaseBuilds"
Log = None
ErrLog = None
Config = None


def CallProcess(args,Msg,ret=True):
	Anim = ['-','\\','|','/']
	
	sys.stdout.write(Msg+':  ')
	Log.write("====== Call: " + args[0] + '\n')
	SVN = Popen(args,
	            stdout=PIPE, stderr = ErrLog)
	
	i = 0
	while(SVN.poll() == None):
		line = SVN.stdout.readline()
		if(line):
			Log.write(line.replace('\n',''))
		sys.stdout.write(chr(8) + Anim[i%4])
		i+=1
		sleep(0.2)
	
	#ErrLog.write(SVN.stdout.read())
	sys.stdout.write(chr(8) + "done\n")
	if(not SVN.returncode == 0 and ret):
		print "Process returns: ",SVN.returncode
		raise

# Step 2 & 3
def CheckOut():
	
	CallProcess([Config.get('Tools','svn'), 
	             "checkout",
				 "-r",
				 `Release`,
				 "https://free-cad.svn.sourceforge.net/svnroot/free-cad/trunk",
				 "../"+FileName],
				 "2) Checking out")

	sys.stdout.write('3) Write version files: ')
	
	Version = open("src/Build/Version.h","w")
	Version.write('#define FCVersionMajor "' + `Major` + '"\n')
	Version.write('#define FCVersionMinor "' + `Minor` + '"\n')
	Version.write('#define FCVersionName "' + Alias + '"\n')
	Version.write('#define FCRevision "' + `Release` + '"\n')
	Version.write('#define FCRepositoryURL "' + "https://free-cad.svn.sourceforge.net/svnroot/free-cad/trunk/src" + '"\n')
	Version.write('#define FCCurrentDateT  "'+time.asctime()+'"  \n')
	Version.close()
	
	Version = open("installer/Version.wxi","w")
	Version.write('<Include> \n')
	Version.write('   <?define FCVersionMajor = ' + `Major` + ' ?>\n')
	Version.write('   <?define FCVersionMinor = ' + `Minor` + ' ?>\n')
	Version.write('   <?define FCVersionRevision =' + `Release` + ' ?>\n')
	Version.write('   <?define FCVersionAlias = "' + Alias + '" ?>\n')
	Version.write('</Include> \n')
	Version.close()
	
	sys.stdout.write('done\n')


#Step 4
def PackSourceZip():

	def addAll(dirFrom, ZipSrcFile):
		for file in os.listdir(dirFrom):                      # for files/dirs here
			if(not file==".svn" and not file== FileName+'_source.zip'):
				pathFrom = os.path.join(dirFrom, file)
				if not os.path.isdir(pathFrom):                   # copy simple files
					ZipSrcFile.write(pathFrom,pathFrom.replace('.\\',FileName+'\\'))
					Log.write("Insert: "+ pathFrom + '\n')
				else:
					addAll(pathFrom,ZipSrcFile)
					
	sys.stdout.write("4) Pack zip source files: ")
	
	SourceFile = ZipFile(FileName+'_source.zip','w',ZIP_DEFLATED,True)
	addAll('.',SourceFile)
	SourceFile.close()
	
	sys.stdout.write("done \n")

# Step 5
def PackSourceTar():

	def addAll(dirFrom, ZipTarFile):
		for file in os.listdir(dirFrom):                      # for files/dirs here
			if(not file==".svn" and not file== FileName+'_source.zip'):
				pathFrom = os.path.join(dirFrom, file)
				if not os.path.isdir(pathFrom):                   # copy simple files
					ZipTarFile.add(pathFrom,pathFrom.replace('.\\',FileName+'\\'))
					Log.write("Insert: "+ pathFrom + '\n')
				else:
					addAll(pathFrom,ZipTarFile)
					
	sys.stdout.write("5) Pack tar source files: ")
	
	SourceFile = tarfile.open(FileName+'_source.tgz','w:gz')
	addAll('.',SourceFile)
	SourceFile.close()
	
	sys.stdout.write("done \n")


# Step 6 & 7	
def BuildAll():
	import fcbt.FileTools
	LibPack = Config.get('Libs','FreeCADLib')
	
	sys.stdout.write('6) Copy resources: ')
	os.mkdir('./bin')
	fcbt.FileTools.cpall(LibPack + '/bin','./bin')
	os.mkdir('./include')
	fcbt.FileTools.cpall(LibPack + '/include','./include')
	os.mkdir('./lib')
	fcbt.FileTools.cpall(LibPack + '/lib','./lib')
	os.mkdir('./doc')
	fcbt.FileTools.cpall(LibPack + '/doc','./doc')
	sys.stdout.write('done\n')
	
	CallProcess(["BuildAll.bat"],
				 "7) Build all")
	
# Step 8 & 9
def HelpFile():
	import wiki2chm
	if not os.path.isdir('doc'):
		os.mkdir('doc')
	if not os.path.isdir('doc/tmp'):
		os.mkdir('doc/tmp')
		
	CallProcess([Config.get('Tools','wget'),'-k', '-r', '-l5', '-P', 'doc/tmp', '-nd', 
	            '-R', '*action=*',
				'-R', '*title=Special*',
				'-R', '*title=Talk*',
				'-R', '*oldid=*',
				'-R', '*printable=yes*',
				'--domains=apps.sourceforge.net',
				'--append-output=doc/tmp/wget.log',
				'http://apps.sourceforge.net/mediawiki/free-cad/index.php?title=Online_Help_Toc'],
				 "8) Download docu")

	sys.stdout.write("9) Fix up CSS: ")
	open('doc/tmp/chm.css','w').write(open('src/Tools/chm.css').read())
	
	wiki2chm.WikiBaseUrl ='http://apps.sourceforge.net/mediawiki/free-cad/'
	wiki2chm.TocPageName ='Online_Help_Toc'
	wiki2chm.BasePath ='doc/tmp/'
	wiki2chm.Output = Log

	wiki2chm.replaceCSS()
	
	wiki2chm.WriteProject()
	wiki2chm.readToc()
	sys.stdout.write("done \n")
	
# Step 10
def CompileHelp():
	import fcbt.FileTools
	CallProcess([Config.get('Tools','hhc'),'doc/tmp/Online_Help_Toc.hhp'],'10)Compile help:',False)
	fcbt.FileTools.cpfile('doc/tmp/FreeCAD.chm','doc/FreeCAD.chm')
	fcbt.FileTools.cpfile('doc/tmp/FreeCAD.chm',FileName+'_helpfile.chm')

def BuildInstaller():
	import fcbt.FileTools
	LibPack = Config.get('Libs','FreeCADLib')
	
	fcbt.FileTools.cpfile('lib/Microsoft_VC80_CRT_x86.msm','installer/Microsoft_VC80_CRT_x86.msm')
	fcbt.FileTools.cpfile('lib/policy_8_0_Microsoft_VC80_CRT_x86.msm','installer/policy_8_0_Microsoft_VC80_CRT_x86.msm')
	CallProcess([Config.get('Tools','candle'),
				 '-out', 'installer\\',
	             'installer\\FreeCAD.wxs',
	             'installer\\FreeCADBase.wxs',
	             'installer\\LibPack.wxs',
	             'installer\\FreeCADDoc.wxs',
	             'installer\\FreeCADModules.wxs',
				 ],'11)Compile installer:',False) 
	CallProcess([Config.get('Tools','light'),
				 '-ext', 'WixUIExtension',
				 '-cultures:en-us',
				 '-out', 'installer\\FreeCAD.msi',
	             'installer\\FreeCAD.wixobj',
	             'installer\\FreeCADBase.wixobj',
	             'installer\\LibPack.wixobj',
	             'installer\\FreeCADDoc.wixobj',
	             'installer\\FreeCADModules.wixobj',
				 ],'12)Build installer:',False)
				 
	fcbt.FileTools.cpfile('installer/FreeCAD.msi',FileName+'_installer.msi')
	
def SendFTP():
	from ftplib import FTP
	ftp = FTP('upload.sf.net')
	Log.write(ftp.login() + '\n')
	Log.write(ftp.cwd("/incoming") + '\n')
	Log.write(ftp.sendcmd('PASV') + '\n')
	Log.write(ftp.sendcmd('TYPE I') + '\n')
	sys.stdout.write('13) Send source ZIP: ')
	f = open(FileName+'_source.zip', "r") 
	Log.write(ftp.storbinary('STOR '+ FileName+'_source.zip', f)  + '\n')
	sys.stdout.write('done\n14) Send source tgz: ')
	f = open(FileName+'_source.tgz', "r") 
	Log.write(ftp.storbinary('STOR '+ FileName+'_source.tgz', f)  + '\n')
	sys.stdout.write('done\n15) Send installer: ')
	f = open(FileName+'_installer.msi', "r") 
	Log.write(ftp.storbinary('STOR '+ FileName+'_installer.msi', f)  + '\n')
	f.close()
	ftp.close()
	
def main():
	global Release, Major, Minor, Alias, FileName, BuildPath, Log, ErrLog, Config
	IniFile = "BuildRelease.ini"
	try:
		opts, args = getopt.getopt(sys.argv[1:], "hb:", ["help","buildPath="])
	except getopt.GetoptError:
		# print help information and exit:
		sys.stderr.write(Usage)
		sys.exit(2)

	# checking on the options
	for o, a in opts:
		if o in ("-h", "--help"):
			sys.stderr.write(Usage)
			sys.exit()
		if o in ("-b", "--buildPath"):
			BuildPath = a
		if o in ("-i", "--ini-file"):
			IniFile = a


	# runing through the files
	if (not len(args) == 1):
		sys.stderr.write(Usage)
	
	Release = int(args[0])
	
	Config = ConfigParser.ConfigParser()
	Config.readfp(open(IniFile))
	
	Alias   = Config.get('Version','Alias')
	Major   = Config.getint('Version','Major')
	Minor   = Config.getint('Version','Minor')

	# creating the directory and switch to
	FileName = 'FreeCAD_' + `Major` + '.' + `Minor` + '.' + `Release`
	print "=== Building:", FileName, '\n'
	BuildPath = BuildPath + '/' + FileName
	# set tool path 
	sys.path.append((BuildPath + '/src/Tools') )
	OldCwd = os.getcwd()
	print "1) Creating Build directory: ", BuildPath
	if not os.path.isdir(BuildPath):
		os.mkdir(BuildPath)
	os.chdir(BuildPath)
	Log = open("BuildRelease.log","w")
	ErrLog = open("BuildReleaseErrors.log","w")
	
	try:
		CheckOut()
		PackSourceZip()
		PackSourceTar()
		BuildAll()
		HelpFile()
		CompileHelp()
		BuildInstaller()
		#SendFTP()
	except:
		Log.close()
		ErrLog.close()
		Err = open("BuildReleaseErrors.log","r")
		sys.stderr.write("!!!!!!!!! Fehler aufgetreten:\n")
		sys.stderr.write(Err.read())
		raise
	
	os.chdir(OldCwd)
	Log.close()
	ErrLog.close()

	print "Press any key"
	sys.stdin.readline()
	
if __name__ == "__main__":
	main()
