import os,sys,string,math,shutil,glob,subprocess
from time import sleep
from os.path import join

##GUI related stuff
from PyQt4 import QtCore, QtGui
from postprocess import Ui_dialog
##------------------------------------------------


from calculix_postprocess import calculix_postprocess
from calculix_postprocess import get_sigini_values

import FreeCAD,Fem,Part


class MyForm(QtGui.QDialog,Ui_dialog):
    def __init__(self, parent=None):
        QtGui.QDialog.__init__(self, parent)
        self.setupUi(self)
        #Define some global variables
        self.dirname = QtCore.QString("")
        #Connect Signals and Slots
        QtCore.QObject.connect(self.button_select_results_folder, QtCore.SIGNAL("clicked()"), self.select_output)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), self.onAbbrechen)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), self.onAbbrechen)
        QtCore.QObject.connect(self.button_start_postprocessing, QtCore.SIGNAL("clicked()"), self.start_PostProcessing)    
        

    def select_output(self):
        self.dirname=QtGui.QFileDialog.getExistingDirectory(None, 'Open working directory', '', QtGui.QFileDialog.ShowDirsOnly)
        self.button_start_postprocessing.setEnabled(True)

    def onAbbrechen(self):
        self.close()

    def start_PostProcessing(self):
        outputfile = open(str(self.dirname + "/postprocessing_input.txt"),"wb")
        sort_tuple = (1,1.1,1.1,1.1,1.1,1.1,1.1,1.1,1.1,1.1,1.1,1.1)
        output_list = []
        lc_coeff = []
        ltc_coeff = []
        sigini = True
        for root, dirs, files in os.walk(str(self.dirname)):
           if 'final_fe_input.frd' in files:
                bbox_orig,\
                bbox_distorted,\
                relationship,\
                max_disp_x,\
                min_disp_x,\
                max_disp_y,\
                min_disp_y,\
                max_disp_z,\
                min_disp_z = calculix_postprocess(os.path.join(root,'final_fe_input.frd'))
                if sigini:
                    sigini = False
                    lc_coeff,ltc_coeff = get_sigini_values(os.path.join(root,'sigini_input.txt'))
                                
                #Get the current Offset-Level and rotation values from the folder-name
                current_z_level = int(root[(root.rfind("_z_l")+4):])
                x_rot = int(root[(root.rfind("x_rot")+5):(root.rfind("_y_rot"))])
                y_rot = int(root[(root.rfind("y_rot")+5):(root.rfind("_z_rot"))])
                z_rot = int(root[(root.rfind("z_rot")+5):(root.rfind("_z_l"))])
                #Get also the coefficients
                #now generate an output tuple and add it to the output-list
                output_list.append((current_z_level,x_rot,y_rot,z_rot,relationship,max_disp_x,min_disp_x,max_disp_y,min_disp_y,max_disp_z,min_disp_z))

        sorted_list = sorted(output_list, key=lambda sort_tuple: sort_tuple[0])
        for item in sorted_list:
            if abs(item[5]) > abs(item[6]):
                abs_disp_x = abs(item[5])
            else:
                abs_disp_x = abs(item[6])
            if abs(item[7]) > abs(item[8]):
                abs_disp_y = abs(item[7])
            else:
                abs_disp_y = abs(item[8])
            if abs(item[9]) > abs(item[10]):
                abs_disp_z = abs(item[9])
            else:
                abs_disp_z = abs(item[10])

            outputfile.write(
                str(item[0]) + " " +
                str(item[1]) + " " +
                str(item[2]) + " " +
                str(item[3]) + " " +
                str(item[4]) + " " +
                str(item[5]) + " " +
                str(item[6]) + " " +
                str(item[7]) + " " +
                str(item[8]) + " " +
                str(item[9]) + " " +
                str(item[10]) + " " +
                str(abs_disp_x) + " " +
                str(abs_disp_y) + " " +
                str(abs_disp_z) + "\n")
                
        outputfile.close()
        #Now create a batch file for GnuPlot and start the generation of the postprocess image generation
        self.start_gnu_plot(sorted_list,lc_coeff,ltc_coeff)
        
        
    def start_gnu_plot(self,list,lc_coeff,ltc_coeff):
        gnu_plot_input_file = open(str(self.dirname + "/gnu_plot_input.txt"),"wb")
        gnu_plot_input_file.write(
                                    "set term png\n" +
                                    "set output \"max_disp_z.png\"\n"+
                                    "set surface\n" +
                                    "set grid\n"+
                                    "set hidden3d\n"+
                                    "set dgrid3d " + str(len(list)-1) + "," + str(len(list)-1) + ",100\n" +
                                    "set view 80,05,1.3,1.0\n"+
                                    "set title \"Abs Displacement in Z vs. Z-Level Offset and Rotation around Z-Axis\" 0,-2\n"+
                                    "show title\n"+
                                    "set label \"L Coefficients used for the calculation:" + lc_coeff[0] + "," + lc_coeff[1] + "," + lc_coeff[2] + "," + lc_coeff[3] + "," + lc_coeff[4] + "," + lc_coeff[5][:-1] + "\" at screen 0.1, screen 0.95 left font \"Arial,8\"\n"+ 
                                    "set label \"LT Coefficients used for the calculation:" + ltc_coeff[0] + "," + ltc_coeff[1] + "," + ltc_coeff[2] + "," + ltc_coeff[3] + "," + ltc_coeff[4] + "," + ltc_coeff[5][:-1] + "\" at screen 0.1, screen 0.93 left font \"Arial,8\"\n"+
                                    "set label \"Z-Offset\\nin [mm]\" at screen 0.5, screen 0.1 center rotate by 0\n"+ 
                                    "set label \"Rotation around Z-Axis\\nin [" + str(chr(248)) +"]\" at screen 0.91, screen 0.2 center rotate by 50\n"+ 
                                    "set label \"Max Displacement Z direction\\nin [mm]\" at screen 0.03, screen 0.5 center rotate by 90\n"+ 
                                    "set xtics in nomirror offset character 0,-0.5\n"+
                                    "splot \"postprocessing_input.txt\" u 1:4:14 with pm3d title \"\"\n" + 
                                    "exit" )
                                    

                                    
        gnu_plot_input_file.close()
        os.chdir(str(self.dirname))
        fnull = open(os.devnull, 'w')
        commandline = FreeCAD.getHomePath() + "bin/gnuplot/gnuplot gnu_plot_input.txt"
        result = subprocess.call(commandline, shell = True, stdout = fnull, stderr = fnull)
        fnull.close()



