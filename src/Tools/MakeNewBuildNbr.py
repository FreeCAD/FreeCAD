# FreeCAD MakeNewBuildNbr script  
# (c) 2002 Jürgen Riegel
#
# Increase the Build Number in Version.h

#***************************************************************************
#*   (c) Jürgen Riegel (juergen.riegel@web.de) 2002                        *   
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
#*   Juergen Riegel 2002                                                   *
#***************************************************************************/

import time

# reading the last Version information
[FCVersionMajor,FCVersionMinor,FCVersionBuild,FCVersionDisDa,dummy] = open("../Version.h",'r').readlines()

# increasing build number
BuildNumber = int(FCVersionBuild[23:-1]) +1

# writing new Version.h File
open("../Version.h",'w').writelines([FCVersionMajor,FCVersionMinor,FCVersionBuild[:23]+`BuildNumber`+'\n',FCVersionDisDa[:23]+ '"'+time.asctime()+'"'])

# writing the ChangeLog.txt
open("../ChangeLog.txt",'a').write("\nVersion: V"+FCVersionMajor[23:-1]+"."+FCVersionMinor[23:-1]+"B"+`BuildNumber`+" Date: "+time.asctime()+' +++++++++++++++++++++++++++++++\n')

