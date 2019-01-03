

# shell and operating system
import os, sys, re
verbose = 0
dcount = fcount = 0
maxfileload = 100000
blksize = 1024 * 8

def cpfile(pathFrom, pathTo, maxfileload=maxfileload):
    """
    copy file pathFrom to pathTo, byte for byte
    """
    if os.path.getsize(pathFrom) <= maxfileload:
        bytesFrom = open(pathFrom, 'rb').read()   # read small file all at once
        bytesTo   = open(pathTo, 'wb')
        bytesTo.write(bytesFrom)                  # need b mode on Windows
        #bytesTo.close()
        #bytesFrom.close()
    else:
        fileFrom = open(pathFrom, 'rb')           # read big files in chunks
        fileTo   = open(pathTo,   'wb')           # need b mode here too 
        while 1:
            bytesFrom = fileFrom.read(blksize)    # get one block, less at end
            if not bytesFrom: break               # empty after last chunk
            fileTo.write(bytesFrom)
        #fileFrom.close()
        #fileTo.close()



def cpall(dirFrom, dirTo):
    """
    copy contents of dirFrom and below to dirTo
    """
    global dcount, fcount
    for file in os.listdir(dirFrom):                      # for files/dirs here
        #print file
        pathFrom = os.path.join(dirFrom, file)
        pathTo   = os.path.join(dirTo,   file)            # extend both paths
        if not os.path.isdir(pathFrom):                   # copy simple files
            try:
                if verbose > 1: print('copying', pathFrom, 'to', pathTo)
                cpfile(pathFrom, pathTo)
                fcount = fcount+1
            except:
                print('Error copying', pathFrom, 'to', pathTo, '--skipped')
                print(sys.exc_type, sys.exc_value)
        else:
            if verbose: print('copying dir', pathFrom, 'to', pathTo)
            try:
                os.mkdir(pathTo)                          # make new subdir
                cpall(pathFrom, pathTo)                   # recur into subdirs
                dcount = dcount+1
            except:
                print('Error creating', pathTo, '--skipped')
                print(sys.exc_type, sys.exc_value)

def SetUpFilter(MatchList):
    RegList = []
    for regexp in MatchList:
        a = re.compile(regexp)
        RegList.append(a)
    return RegList

def cpallWithFilter(dirFrom, dirTo,MatchList):
    """
    copy contents of dirFrom and below to dirTo without match
    """
    global dcount, fcount
    for file in os.listdir(dirFrom):                      # for files/dirs here
        hitt = 0
        for matchpat in MatchList:
            if(re.match(matchpat,file)):
               hitt = 1
#               print 'Refuse: '+file
        if hitt == 0:
            pathFrom = os.path.join(dirFrom, file)
            pathTo   = os.path.join(dirTo,   file)            # extend both paths
            if not os.path.isdir(pathFrom):                   # copy simple files
                try:
                    if verbose > 1: print('copying', pathFrom, 'to', pathTo)
                    cpfile(pathFrom, pathTo)
                    fcount = fcount+1
                except:
                    print('Error copying', pathFrom, 'to', pathTo, '--skipped')
                    print(sys.exc_type, sys.exc_value)
            else:
                if verbose: print('copying dir', pathFrom, 'to', pathTo)
                try:
                    os.mkdir(pathTo)                            # make new subdir
                    cpallWithFilter(pathFrom, pathTo,MatchList) # recur into subdirs
                    dcount = dcount+1
                except:
                    print('Error creating', pathTo, '--skipped')
                    print(sys.exc_type, sys.exc_value)

################################################################
# Use: "python rmall.py directoryPath directoryPath..."
# recursive directory tree deletion: removes all files and 
# directories at and below directoryPaths; recurs into subdirs
# and removes parent dir last, because os.rmdir requires that 
# directory is empty; like a Unix "rm -rf directoryPath" 
################################################################ 

fcount = dcount = 0

def rmall(dirPath):                             # delete dirPath and below
    global fcount, dcount
    namesHere = os.listdir(dirPath)
    for name in namesHere:                      # remove all contents first
        path = os.path.join(dirPath, name)
        if not os.path.isdir(path):             # remove simple files
            os.remove(path)
            fcount = fcount + 1
        else:                                   # recur to remove subdirs
            rmall(path)
    os.rmdir(dirPath)                           # remove now-empty dirPath
    dcount = dcount + 1

