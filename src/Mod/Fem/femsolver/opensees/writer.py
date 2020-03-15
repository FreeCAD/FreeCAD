# ***************************************************************************
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "OpenSees Writer"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import FreeCAD
import time
from .. import writerbase as FemInputWriter


class FemInputWriterOpenSees(FemInputWriter.FemInputWriter):
    def __init__(
        self,
        analysis_obj,
        solver_obj,
        mesh_obj,
        member,
        dir_name=None
    ):
        FemInputWriter.FemInputWriter.__init__(
            self,
            analysis_obj,
            solver_obj,
            mesh_obj,
            member,
            dir_name
        )
        # working dir and input file
        from os.path import join
        # self.main_file_name = self.mesh_object.Name + ".in"
        self.main_file_name = "opensees_example.tcl"
        self.file_name = join(self.dir_name, self.main_file_name)
        FreeCAD.Console.PrintLog(
            "FemInputWriterCcx --> self.dir_name  -->  " + self.dir_name + "\n"
        )
        FreeCAD.Console.PrintLog(
            "FemInputWriterCcx --> self.main_file_name  -->  " + self.main_file_name + "\n"
        )
        FreeCAD.Console.PrintMessage(
            "FemInputWriterCcx --> self.file_name  -->  " + self.file_name + "\n"
        )

    def write_opensees_input_file(self):

        timestart = time.clock()

        inpfile = open(self.file_name, "w")

        inpfile.write(example_input_file)
        inpfile.close()
        writing_time_string = (
            "Writing time input file: {} seconds"
            .format(round((time.clock() - timestart), 2))
        )
        FreeCAD.Console.PrintMessage(writing_time_string + " \n\n")
        return self.file_name


example_input_file = """# ----------------------------
# Start of model generation
# ----------------------------
# kgf-m
# Create ModelBuilder with 3 dimensions and 6 DOF/node
model basic -ndm 3 -ndf 3

#Basic units
set m   1.0; # meter for length
set sec 1.0; # second for time
set kg  1.0; # Kilogram for mass

#Other units
# angle
#set rad 1.0;
#set deg [expr $PI/180.0*$rad];

# length
set cm  0.01;
set in  0.0254;
set ft [expr 12.0*$in];

# mass
set lbs 0.4536;
set kip [expr 1000.0*$lbs];
set ton [expr 1000.0*$kg]

# force
set N   [expr 0.1 * $kg * $m / ($sec * $sec)] ;
set KN [expr 1000.0*$N];
set MN [expr 1000.0*$KN];

# pressure
set Pa  [expr 1.0*$N/pow($m, 2)];
set KPa [expr 1000.0*$Pa];
set GPa [expr 1000.0*$KPa];
set pcf [expr $lbs/pow($ft,3)];	# pcf = #/cubic foot
set ksi [expr $kip/pow($in,2)];
set psi [expr $ksi/1000.];


# create the material
nDMaterial ElasticIsotropic   1   [expr 210*$GPa]   0.3  7900

# Define geometry
# ---------------

# define some  parameters
set eleArgs "1"

#set element stdBrick
set element bbarBrick

set nz 2
set nx 6
set ny 2

set nn [expr ($nz+1)*($nx+1)*($ny+1) ]

# mesh generation
block3D $nx $ny $nz   8 1  $element  $eleArgs {

    1   4 1 0
    2   -4 1 0
    3   -4 0 0
    4   4 0 0
    5   4 1 1
    6   -4 1 1
    7   -4 0 1
    8   4 0 1
}


set load [expr 900*$N]

# Constant point load
pattern Plain 1 Linear {
   load $nn  0  0 $load
}

# boundary conditions
fixX 4   1 1 1  1 1 1

# --------------------------------------------------------------------
# Start of static analysis (creation of the analysis & analysis itself)
# --------------------------------------------------------------------

# Load control with variable load steps
#                       init   Jd  min   max
integrator LoadControl  1.0  1

# Convergence test
#                  tolerance maxIter displayCode
test NormUnbalance     1.0e-10    20     0

# Solution algorithm
algorithm Newton

# DOF numberer
numberer RCM

# Cosntraint handler
constraints Plain

# System of equations solver
system ProfileSPD

# Analysis for gravity load
analysis Static

# Perform the analysis
analyze 5


# --------------------------
# End of static analysis
# --------------------------

# ----------------------------
# Start of recorder generation
# ----------------------------

recorder Node -file Node.out -time -node $nn -dof 1 2 3 disp
#recorder plot Node.out CenterNodeDisp 625 10 625 450 -columns 1 2

recorder display ShakingBeam 0 0 300 300 -wipe
prp -100 100 120.5
vup 0 1 0
display 1 4 1

# --------------------------
# End of recorder generation
# --------------------------


# ---------------------------------------
# Create and Perform the dynamic analysis
# ---------------------------------------

# Remove the static analysis & reset the time to 0.0
wipeAnalysis
setTime 0.0

# Now remove the loads and let the beam vibrate
remove loadPattern 1

# add some mass proportional damping
rayleigh 0.01 0.0 0.0 0.0

# Create the transient analysis
test EnergyIncr     1.0e-10    20   0
algorithm Newton
numberer RCM
constraints Plain
integrator Newmark 0.5 0.25
#integrator GeneralizedMidpoint 0.50
analysis Transient


# Perform the transient analysis (20 sec)
#       numSteps  dt
analyze 100 2.0
"""

##  @}
