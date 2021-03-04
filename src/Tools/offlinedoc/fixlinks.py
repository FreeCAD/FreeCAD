#!/usr/bin/env python3

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2021 Yorik van Havre <yorik@uncreated.net>              *  
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

"""This script fixes links like href="/Arch_Wall" into href="/Arch_Wall.html" where needed. Dirty hack, downloadwiki.py should be fixed instead"""

import os,re
files = [f for f in os.listdir("localwiki") if f.endswith(".html")]
for fn in files:
    f = open(os.path.join("localwiki",fn))
    b = f.read()
    f.close()
    b = b.replace("\n","--endl--")
    for href in re.findall("href=\".*?\"",b):
        if (not "." in href) and (not "#" in href):
            repl = href[:-1]+".html\""
            if "href=\"/" in repl:
                repl = repl.replace("href=\"/","href=\"")
            print(fn," : replacing",href,"with",repl)
            b = b.replace(href,repl)
    b = b.replace("--endl--","\n")
    f = open(os.path.join("localwiki",fn),"w")
    f.write(b)
    f.close()

