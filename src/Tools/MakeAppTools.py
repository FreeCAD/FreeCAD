import os, sys, re,FCFileTools
verbose = 0
dcount = fcount = 0

def replaceTemplate(dirName, oldName, newName):
	"""
	modify contents from dirName and below, replace oldName by newName
	"""
	for file in os.listdir(dirName):
		pathName = os.path.join(dirName, file)
		if not os.path.isdir(pathName):
			try:
				print(pathName)
				origFile = open(pathName)									# open file
				lines = origFile.readlines()								# read the file...
				origFile.close()											# ... and close it
				output = open(pathName,"w")									# open the file again
				for line in lines:
					if (line.find(oldName) != -1):							# search for 'oldName' and replace it
						line = line.replace(oldName, newName)
					output.write(line)										# write the modified line back
				output.close												# close the file
			except:
				print('Error modifying', pathName, '--skipped')
				print(sys.exc_info()[0], sys.exc_info()[1])
		else:
			try:
				replaceTemplate(pathName, oldName, newName)
			except:
				print('Error changing to directory', pathName, '--skipped')
				print(sys.exc_info()[0], sys.exc_info()[1])


def copyTemplate(dirFrom, dirTo, oldName, newName, MatchFile, MatchDir):
	"""
	copy contents of dirFrom and below to dirTo
	"""
	global dcount, fcount
	for file in os.listdir(dirFrom):										# for files/dirs here
		print(file)
		pathFrom = os.path.join(dirFrom, file)
		pathTo   = os.path.join(dirTo,   file)								# extend both paths
		if (pathTo.find(oldName) != -1):
			pathTo = pathTo.replace(oldName, newName)						# rename file if 'oldName' is found
		if not os.path.isdir(pathFrom):										# copy simple files
			hit = 0
			for matchpat in MatchFile:
				if(re.match(matchpat,file)):
					hit = 1
					break
			if hit:
				print('Ignore file '+file)
				continue
			try:
				if verbose > 1: print('copying', pathFrom, 'to', pathTo)
				FCFileTools.cpfile(pathFrom, pathTo)
				fcount = fcount+1
			except:
				print('Error copying', pathFrom, 'to', pathTo, '--skipped')
				print(sys.exc_info()[0], sys.exc_info()[1])
		else:
			hit = 0
			for matchpat in MatchDir:
				if(re.match(matchpat,file)):
					hit = 1
					break
			if hit:
				print('Ignore directory '+file)
				continue
			if verbose: print('copying dir', pathFrom, 'to', pathTo)
			try:
				os.mkdir(pathTo)																# make new subdir
				copyTemplate(pathFrom, pathTo, oldName, newName, MatchFile, MatchDir)			# recur into subdirs
				dcount = dcount+1
			except:
				print('Error creating', pathTo, '--skipped')
				print(sys.exc_info()[0], sys.exc_info()[1])
