# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""This script retrieves a list of standard property sets from the IFC4 official documentation website
   and stores them into a pset_dfinitions.xml files in the current directory. Warning, this can take
   a certain time (there are more than 400 definitions to retrieve)"""

from __future__ import print_function

import re, urllib2, os

MAXTRIES = 3

# read the pset list
print("Getting psets list...")
u = urllib2.urlopen(
    "http://www.buildingsmart-tech.org/ifc/IFC4/Add2/html/annex/annex-b/alphabeticalorder_psets.htm"
)
p = u.read()
u.close()
psets = re.findall(">Pset_(.*?)</a>", p)

# retrieve xml data from each Pset type
psetdefs = ""
failed = []
for i, pset in enumerate(psets):
    print(i + 1, "/", len(psets), ": Retrieving Pset", pset)
    for j in range(MAXTRIES):
        try:
            u = urllib2.urlopen(
                "http://www.buildingsmart-tech.org/ifc/IFC4/Add2/html/psd/Pset_"
                + pset
                + ".xml"
            )
            p = u.read()
            u.close()
        except:
            print("    Connection failed. trying one more time...")
        else:
            break
    else:
        print("    Unable to retrieve ", pset, ". Skipping...")
        failed.append(pset)
    psetdefs += p
psetdefs = psetdefs.replace('<?xml version="1.0" encoding="utf-8"?>', "")
psetdefs = '<?xml version="1.0" encoding="utf-8"?>\n<Root>\n' + psetdefs + "</Root>"

f = open("pset_definitions.xml", "wb")
f.write(psetdefs)
f.close()
print(
    "All done! writing "
    + os.path.join(os.path.abspath(os.curdir), "pset_definitions.xml")
)
if failed:
    print("The following psets failed and were not retrieved:", failed)
