#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013 - Juergen Riegel <FreeCAD@juergen-riegel.net>      *  
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




def readResult(frd_input) :
    input = open(frd_input,"r")
    nodes_x = []
    nodes_y = []
    nodes_z = []
    disp_x = []
    disp_y = []
    disp_z = []
    displaced_nodes_x = []
    displaced_nodes_y = []
    displaced_nodes_z = []

    disp_found = False
    nodes_found = True
    while True:
      line=input.readline()
      if not line: break
      #first lets extract the node and coordinate information from the results file
      if nodes_found and (line[1:3] == "-1"):
        nodes_x.append(float(line[13:25]))
        nodes_y.append(float(line[25:37]))
        nodes_z.append(float(line[37:49]))
      #Check if we found displacement section
      if line[5:9] == "DISP":
         disp_found = True
      #we found a displacement line in the frd file
      if disp_found and (line[1:3] == "-1"):
         disp_x.append(float(line[13:25]))
         disp_y.append(float(line[25:37]))
         disp_z.append(float(line[37:49]))
      #Check for the end of a section   
      if line[1:3] == "-3":
         #the section with the displacements and the nodes ended
         disp_found = False  
         nodes_found = False
      
    input.close()
