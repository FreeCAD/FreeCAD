# FreeCAD report memory leaks script to get provide the log file of Visual Studio in more readable file.
# (c) 2006 Werner Mayer
#

#***************************************************************************
#*   Copyright (c) 2006 Werner Mayer <werner.wm.mayer@gmx.de>              *
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

#!/usr/bin/python
import os,sys,string,re

# Open the memory leak file
file = open("MemLog.txt")
lines = file.readlines()
file.close()

d=dict()
l=list()
for line in lines:
    r=re.search("\\(#\\s*\\d+\\)",line)
    if r != None:
        s=line[r.start():r.end()]
        t=re.search("^Leak",line)
        if t != None:
            m=d[s]
            l.append(m)
        else:
            d[s]=line

file = open("MemLog_leaks.txt","w")
for line in l:
    line = string.replace(line,'Alloc','Leak')
    file.write(line)
file.close()
    