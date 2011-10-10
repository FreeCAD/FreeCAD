import os,sys,string,math,shutil,glob,subprocess,tempfile
from time import sleep
from os.path import join

##GUI related stuff
from PyQt4 import QtCore, QtGui
from User_Interface_Mach_Dist import Ui_dialog
##------------------------------------------------

#from read_generalINFOS  import  read_generalINFOS
#from determine_ElementType  import  determine_ElementType
#from mesh_bdf2inp  import mesh_bdf2inp
from ApplyingBC_IC  import ApplyingBC_IC
from calculix_postprocess import calculix_postprocess

import FreeCAD,Fem,Part


class MyForm(QtGui.QDialog,Ui_dialog):
    def __init__(self, parent=None):
        QtGui.QDialog.__init__(self, parent)
        self.setupUi(self)
        os.chdir("c:/")
        #Define some global variables
        self.filename = QtCore.QString("")
        self.dirname = QtCore.QString("")
        self.params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Machining_Distortion")
        #Connect Signals and Slots
        QtCore.QObject.connect(self.button_select_file, QtCore.SIGNAL("clicked()"), self.select_file)
        QtCore.QObject.connect(self.button_select_output, QtCore.SIGNAL("clicked()"), self.select_output)
        QtCore.QObject.connect(self.button_dialog, QtCore.SIGNAL("accepted()"), self.onAbbrechen)
        QtCore.QObject.connect(self.button_dialog, QtCore.SIGNAL("rejected()"), self.onAbbrechen)
        QtCore.QObject.connect(self.button_start_calculation, QtCore.SIGNAL("clicked()"), self.start_calculation)
        QtCore.QObject.connect(self.button_add_to_table, QtCore.SIGNAL("clicked()"), self.add_to_table)
        QtCore.QObject.connect(self.select_L_file, QtCore.SIGNAL("clicked()"), self.add_L_data)
        QtCore.QObject.connect(self.select_LT_file, QtCore.SIGNAL("clicked()"), self.add_LT_data)


        #Define the table headers as we are not able to use the QT Designer for that
        self.JobTable.clear()
        self.JobTable.setHorizontalHeaderLabels(
        ["Input File","Output Folder","Z-Offset From","Z-Offset To","Z-Intervall","X-Rot From","X-Rot To","X-Rot Intervall",
        "Y-Rot From","Y-Rot To","Y-Rot Intervall","Z-Rot From","Z-Rot To","Z-Rot Intervall","Young Modulus","Poisson Ratio",
        "LC1","LC2","LC3","LC4","LC5","LC6","LTC1","LTC2","LTC3","LTC4","LTC5","LTC6","Plate Thickness"])

    def add_to_table(self):
        self.JobTable.insertRow(0)
        item = QtGui.QTableWidgetItem(self.filename)
        self.JobTable.setItem(0,0,item)
        item = QtGui.QTableWidgetItem(self.dirname)
        self.JobTable.setItem(0,1,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.spinBox_z_level_from.value())
        self.JobTable.setItem(0,2,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.spinBox_z_level_to.value())
        self.JobTable.setItem(0,3,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.spinBox_z_level_intervall.value())
        self.JobTable.setItem(0,4,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.spinBox_misalignment_x_from.value())
        self.JobTable.setItem(0,5,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.spinBox_misalignment_x_to.value())
        self.JobTable.setItem(0,6,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.spinBox_misalignment_x_intervall.value())
        self.JobTable.setItem(0,7,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.spinBox_misalignment_y_from.value())
        self.JobTable.setItem(0,8,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.spinBox_misalignment_y_to.value())
        self.JobTable.setItem(0,9,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.spinBox_misalignment_y_intervall.value())
        self.JobTable.setItem(0,10,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.spinBox_misalignment_z_from.value())
        self.JobTable.setItem(0,11,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.spinBox_misalignment_z_to.value())
        self.JobTable.setItem(0,12,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.spinBox_misalignment_z_intervall.value())
        self.JobTable.setItem(0,13,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.spinBox_young_modulus.value())
        self.JobTable.setItem(0,14,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.spinBox_poisson_ratio.value())
        self.JobTable.setItem(0,15,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.lc1.value())
        self.JobTable.setItem(0,16,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.lc2.value())
        self.JobTable.setItem(0,17,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.lc3.value())
        self.JobTable.setItem(0,18,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.lc4.value())
        self.JobTable.setItem(0,19,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.lc5.value())
        self.JobTable.setItem(0,20,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.lc6.value())
        self.JobTable.setItem(0,21,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.ltc1.value())
        self.JobTable.setItem(0,22,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.ltc2.value())
        self.JobTable.setItem(0,23,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.ltc3.value())
        self.JobTable.setItem(0,24,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.ltc4.value())
        self.JobTable.setItem(0,25,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.ltc5.value())
        self.JobTable.setItem(0,26,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.ltc6.value())
        self.JobTable.setItem(0,27,item)
        item = QtGui.QTableWidgetItem()
        item.setData(QtCore.Qt.DisplayRole,self.spinBox_Plate_Thickness.value())
        self.JobTable.setItem(0,28,item)
        # This is how to get the data back test2 = self.JobTable.item(0,1).data(QtCore.Qt.DisplayRole).toInt()
        self.button_add_to_table.setEnabled(False)
        self.filename.clear()
        self.button_start_calculation.setEnabled(True)
        

            
        
    def add_L_data(self):
        l_filename = QtGui.QFileDialog.getOpenFileName(None, 'Open file','','R-Script File for L Coefficients (*.txt)')
        values = self.parse_R_output(l_filename)
        self.lc1.setValue(values[0]),self.lc2.setValue(values[1]),self.lc3.setValue(values[2]),
        self.lc4.setValue(values[3]),self.lc5.setValue(values[4]),self.lc6.setValue(values[5])
        
        
    def add_LT_data(self):
        lt_filename = QtGui.QFileDialog.getOpenFileName(None, 'Open file','','R-Script File for LT Coefficients (*.txt)')
        values = self.parse_R_output(lt_filename)
        self.ltc1.setValue(values[0]),self.ltc2.setValue(values[1]),self.ltc3.setValue(values[2]),
        self.ltc4.setValue(values[3]),self.ltc5.setValue(values[4]),self.ltc6.setValue(values[5])
        

    def parse_R_output(self,filename):
        file = open(str(filename))
        lines = file.readlines()
        found = False
        coeff = []
        for line in lines:
            if line[0:9] == "c0 to c5:":
                found = True
                coeff.append(float(line[15:]))
                continue
            if found and line[0:4] == "MSE:":
                found = False
            if found:
                coeff.append(float(line[15:]))
        
        file.close()
        return coeff[0],coeff[1],coeff[2],coeff[3],coeff[4],coeff[5]
        
  
    def select_file(self):
        self.filename=QtGui.QFileDialog.getOpenFileName(None, 'Open file','','Nastran Files (*.bdf)')
        self.button_add_to_table.setEnabled(not self.dirname.isEmpty() and not self.filename.isEmpty())

    def select_output(self):
        self.dirname=QtGui.QFileDialog.getExistingDirectory(None, 'Open working directory', '', QtGui.QFileDialog.ShowDirsOnly)
        self.button_add_to_table.setEnabled(not self.dirname.isEmpty() and not self.filename.isEmpty())

    def onAbbrechen(self):
        self.close()

        
    def start_calculation(self):
        self.button_add_to_table.setEnabled(False)
        self.button_select_file.setEnabled(False)
        self.button_select_output.setEnabled(False)
        self.button_start_calculation.setEnabled(False)
        os.chdir("/")
        ##Get values from the GUI
        if ( os.path.exists(str(self.dirname)) ):
            os.chdir("c:/")
            shutil.rmtree(str(self.dirname))
        
        os.mkdir(str(self.dirname))
        batch = open(str(self.dirname + "/" + "lcmt_CALCULIX_Calculation_batch.bat"),'wb')
        #Tell calculixs solver spooles how many cpus to use
        #batch.write("export CCX_NPROC=" + str(self.params.GetInt("NumberCPUs")) + "\n")
        #If we have a tcsh
        #batch.write("setenv CCX_NPROC 4\n")

        #Now do the calculation stuff for each row in the  table
        for job in range (0,self.JobTable.rowCount()):
            #Extract the data from the table
            current_file_name = self.JobTable.item(job,0).text()
            z_offset_from = self.JobTable.item(job,2).data(QtCore.Qt.DisplayRole).toInt()[0]
            z_offset_to = self.JobTable.item(job,3).data(QtCore.Qt.DisplayRole).toInt()[0]
            z_offset_intervall = self.JobTable.item(job,4).data(QtCore.Qt.DisplayRole).toInt()[0]
            x_rot_from = self.JobTable.item(job,5).data(QtCore.Qt.DisplayRole).toInt()[0]
            x_rot_to = self.JobTable.item(job,6).data(QtCore.Qt.DisplayRole).toInt()[0]
            x_rot_intervall = self.JobTable.item(job,7).data(QtCore.Qt.DisplayRole).toInt()[0]
            y_rot_from = self.JobTable.item(job,8).data(QtCore.Qt.DisplayRole).toInt()[0]
            y_rot_to = self.JobTable.item(job,9).data(QtCore.Qt.DisplayRole).toInt()[0]
            y_rot_intervall = self.JobTable.item(job,10).data(QtCore.Qt.DisplayRole).toInt()[0]
            z_rot_from = self.JobTable.item(job,11).data(QtCore.Qt.DisplayRole).toInt()[0]
            z_rot_to = self.JobTable.item(job,12).data(QtCore.Qt.DisplayRole).toInt()[0]
            z_rot_intervall = self.JobTable.item(job,13).data(QtCore.Qt.DisplayRole).toInt()[0]
            young_modulus = self.JobTable.item(job,14).data(QtCore.Qt.DisplayRole).toDouble()[0]
            poisson_ratio = self.JobTable.item(job,15).data(QtCore.Qt.DisplayRole).toDouble()[0]
            lc1 = self.JobTable.item(job,16).data(QtCore.Qt.DisplayRole).toDouble()[0]
            lc2 = self.JobTable.item(job,17).data(QtCore.Qt.DisplayRole).toDouble()[0]
            lc3 = self.JobTable.item(job,18).data(QtCore.Qt.DisplayRole).toDouble()[0]
            lc4 = self.JobTable.item(job,19).data(QtCore.Qt.DisplayRole).toDouble()[0]
            lc5 = self.JobTable.item(job,20).data(QtCore.Qt.DisplayRole).toDouble()[0]
            lc6 = self.JobTable.item(job,21).data(QtCore.Qt.DisplayRole).toDouble()[0]
            ltc1 = self.JobTable.item(job,22).data(QtCore.Qt.DisplayRole).toDouble()[0]
            ltc2 = self.JobTable.item(job,23).data(QtCore.Qt.DisplayRole).toDouble()[0]
            ltc3 = self.JobTable.item(job,24).data(QtCore.Qt.DisplayRole).toDouble()[0]
            ltc4 = self.JobTable.item(job,25).data(QtCore.Qt.DisplayRole).toDouble()[0]
            ltc5 = self.JobTable.item(job,26).data(QtCore.Qt.DisplayRole).toDouble()[0]
            ltc6 = self.JobTable.item(job,27).data(QtCore.Qt.DisplayRole).toDouble()[0]
            plate_thickness = self.JobTable.item(job,28).data(QtCore.Qt.DisplayRole).toDouble()[0]
            filename_without_suffix = self.JobTable.item(job,0).text().split("/").takeLast().split(".")[0]
            meshobject = Fem.read(str(current_file_name))
            #Perform PCA
            Fem.SMESH_PCA(meshobject)
            #Do min routine
            Fem.minBoundingBox(meshobject)
            #Now get the Node Numbers for the Boundary Conditions
            node_numbers = []
            node_numbers = Fem.getBoundary_Conditions(meshobject)

            #Now we have set up the initial geometry for the calculations. Lets generate an ABAQUS input file now for each z-level with exactly the same
            #boundary conditions


            #1. Lets translate the geometry to the initial desired z-level

            #2. Generate a Folder for the current calculation z-level and output the ABAQUS Geometry and the boundary_conditions
            #Lets first generate a subfolder with the current filename
            os.mkdir(str(self.dirname + "/" + filename_without_suffix))
            i = z_offset_from
            while i <= z_offset_to:
                j = x_rot_from
                while j <= x_rot_to:
                    k = y_rot_from
                    while k <= y_rot_to:
                        l = z_rot_from
                        while l <= z_rot_to:
                            rotation_around_x = FreeCAD.Base.Placement(FreeCAD.Base.Vector(0,0,0),FreeCAD.Base.Vector(1,0,0),j)
                            rotation_around_y = FreeCAD.Base.Placement(FreeCAD.Base.Vector(0,0,0),FreeCAD.Base.Vector(0,1,0),k)
                            rotation_around_z = FreeCAD.Base.Placement(FreeCAD.Base.Vector(0,0,0),FreeCAD.Base.Vector(0,0,1),l)
                            translate = FreeCAD.Base.Placement(FreeCAD.Base.Vector(0,0,i),FreeCAD.Base.Vector(0,0,0),0.0)
                            translation = rotation_around_x.multiply(rotation_around_y).multiply(rotation_around_z).multiply(translate)
                            #Now lets check if the part is still in the billet due to the rotation. If not, we directly skip to the next rotation value
                            if(Fem.checkBB(meshobject,translation,plate_thickness)):
                                print "Too heavy rotations"
                                l= l + z_rot_intervall
                                continue
                            print "it seems that nothing changed"
                            #Use the placedment as optional argument for the write() method
                            #translated_mesh.setTransform(translation)
                            Case_Dir = str(self.dirname) + "/" + filename_without_suffix + "/" + filename_without_suffix +\
                            "_"+"x_rot"+ str(int(j))+ \
                            "_"+"y_rot"+ str(int(k))+ \
                            "_"+"z_rot"+ str(int(l))+ \
                            "_"+"z_l"+ str(int(i))
                            if ( os.path.exists(str(Case_Dir)) ):
                                os.chdir(str(self.dirname))
                                shutil.rmtree(str(Case_Dir))

                            os.mkdir(str(Case_Dir))

                            os.chdir("c:/")
                            #Lets generate a sigini Input Deck for the calculix user subroutine
                            sigini_input = open (str(Case_Dir + "/" + "sigini_input.txt"),'wb')
                            
                            #Write plate thickness to the sigini_file
                            sigini_input.write(str(plate_thickness) + "\n")
                            #Now write the Interpolation coefficients, first the L and then the LC ones
                            sigini_input.write(\
                            str(lc1) + "," + \
                            str(lc2) + "," + \
                            str(lc3) + "," + \
                            str(lc4) + "," + \
                            str(lc5) + "," + \
                            str(lc6) + "\n")
                            sigini_input.write(\
                            str(ltc1) + "," + \
                            str(ltc2) + "," + \
                            str(ltc3) + "," + \
                            str(ltc4) + "," + \
                            str(ltc5) + "," + \
                            str(ltc6) + "\n")
                            sigini_input.close()
                            #Check if the 
                            meshobject.writeABAQUS(str(Case_Dir + "/" + "geometry_fe_input.inp"), translation)
                            ApplyingBC_IC(Case_Dir, young_modulus,poisson_ratio,node_numbers[0],node_numbers[1],node_numbers[2])
                            #Now lets generate a LSF Job-File to be used by the Airbus Clusters
                            lsf_input = open (str(Case_Dir + "/" + "job.lsf"),"wb")
                            lsf_input.write("#!/bin/bash\n")
                            lsf_input.write("export CCX_NPROC=" + str(self.params.GetInt("NumberCPUs")) + "\n")
                            lsf_input.write("#BSUB -n "+ str(self.params.GetInt("NumberCPUs")) + "\n")
                            lsf_input.write("#BSUB -W 10:00\n")
                            lsf_input.write("#BSUB -o %J.out\n")
                            lsf_input.write("#BSUB -e %J.err\n")
                            lsf_input.write("#BSUB -J calculix\n")
                            lsf_input.write("#BSUB -q loc_all_hiio\n")
                            lsf_input.write(str("datadir=\"" + self.params.GetString("Linux Home Path") + "/" + self.dirname[str(self.dirname).rfind("/")+1:] + "/" + filename_without_suffix + "/" + filename_without_suffix + 
                            "_"+"x_rot"+ str(int(j))+
                            "_"+"y_rot"+ str(int(k))+
                            "_"+"z_rot"+ str(int(l))+
                            "_"+"z_l"+ str(int(i)) + "\"\n"))
                            lsf_input.write("cd $datadir\n")
                            lsf_input.write(self.params.GetString("Solver Link") + " -i final_fe_input\n")
                            lsf_input.close()
                            
                            batch.write(str("cd \"" + self.params.GetString("Linux Home Path") + "/" + self.dirname[str(self.dirname).rfind("/")+1:] + "/" + filename_without_suffix + "/" + filename_without_suffix + 
                            "_"+"x_rot"+ str(int(j))+
                            "_"+"y_rot"+ str(int(k))+
                            "_"+"z_rot"+ str(int(l))+
                            "_"+"z_l"+ str(int(i)) + "\"\n"))
                            batch.write("job.lsf\n")
                            l= l + z_rot_intervall
                        k = k + y_rot_intervall
                    j = j + x_rot_intervall
                i = i+ z_offset_intervall
        print "Ale Schleifen beendet"
        batch.write("cd \"" + self.params.GetString("Linux Home Path") + "\"\n")
        batch.write("find \"" + str(self.dirname[str(self.dirname).rfind("/")+1:] + "/") + "\" -name \"sigini_output.txt\" -exec rm -f {} \;\n")
        batch.write("find \"" + str(self.dirname[str(self.dirname).rfind("/")+1:] + "/") + "\" -name \"*.out\" -exec rm -f {} \;\n")
        batch.write("find \"" + str(self.dirname[str(self.dirname).rfind("/")+1:] + "/") + "\" -name \"*.err\" -exec rm -f {} \;\n")
        batch.write("find \"" + str(self.dirname[str(self.dirname).rfind("/")+1:] + "/") + "\" -name \"*.dat\" -exec rm -f {} \;\n")
        batch.write("find \"" + str(self.dirname[str(self.dirname).rfind("/")+1:] + "/") + "\" -name \"*.sta\" -exec rm -f {} \;\n")
        batch.write("tar cf \"" + str(self.dirname[str(self.dirname).rfind("/")+1:]  + ".tar\" \"" + str(self.dirname[str(self.dirname).rfind("/")+1:] + "/") + "\"\n"))
        batch.write("rm -rf \"" + str(self.dirname[str(self.dirname).rfind("/")+1:] + "/") + "\"\n")
        batch.close()

        os.chdir("c:/")
        fnull = open(os.devnull, 'w')
        #Generate the full zip name:
        zipname = tempfile.gettempdir() + "/" + str(self.dirname)[str(self.dirname).rfind("/")+1:] + ".zip"
        #Check if the zip file already exists. If yes, then we have to remove it
        if os.path.exists(zipname):
            try:
                os.remove(zipname)
            except Exception,e:
                print e
        
        #Zip the whole directory structure now and save the zip file in the temp folder for further processing
        commandline = FreeCAD.getHomePath() + "bin/7z a -tzip -mx=0 " + "\"" + zipname + "\" " + "\"" + str(self.dirname) + "\""
        result = subprocess.call(commandline, shell = True, stdout = fnull, stderr = fnull)
        #somehow we have to check for a false return code!
        if not result:
            shutil.rmtree(str(self.dirname))
            
        #Now send the zip file to the server for calculation
        commandline = FreeCAD.getHomePath() + "bin/pscp -r -l "+ self.params.GetString("Linux User Name") + " -pw " + self.params.GetString("Linux Password") + " " + \
        "\"" + zipname + "\" " + self.params.GetString("Servername") + ":" + self.params.GetString("Linux Home Path")
        result = subprocess.call(commandline, shell = True, stdout = fnull, stderr = fnull)
        #Now unzip, change into the directory and start the batch file
        commandline = FreeCAD.getHomePath() + "bin/plink -batch -l "+ self.params.GetString("Linux User Name") + " -pw " + self.params.GetString("Linux Password") + " " + \
        self.params.GetString("Servername") + " unzip -o \"" + self.params.GetString("Linux Home Path") + "/" + str(self.dirname)[str(self.dirname).rfind("/")+1:] + ".zip\""
        print commandline
        result = subprocess.call(commandline, shell = True, stdout = fnull, stderr = fnull)
        commandline = FreeCAD.getHomePath() + "bin/plink -batch -l "+ self.params.GetString("Linux User Name") + " -pw " + self.params.GetString("Linux Password") + " " + \
        self.params.GetString("Servername") + " chmod +x -R \"" + self.params.GetString("Linux Home Path") + "/" + str(self.dirname)[str(self.dirname).rfind("/")+1:] + "\""
        print commandline
        result = subprocess.call(commandline, shell = True, stdout = fnull, stderr = fnull)
        commandline = FreeCAD.getHomePath() + "bin/plink -batch -l "+ self.params.GetString("Linux User Name") + " -pw " + self.params.GetString("Linux Password") + " " + \
        self.params.GetString("Servername") + " chmod +x -R \"" + self.params.GetString("Linux Home Path") + "/" + str(self.dirname)[str(self.dirname).rfind("/")+1:] + "\""
        print commandline
        result = subprocess.call(commandline, shell = True, stdout = fnull, stderr = fnull)
        #Now we copy the batch file one level ahead as otherwise we cannot delete the calculation folder 
        commandline = FreeCAD.getHomePath() + "bin/plink -batch -l "+ self.params.GetString("Linux User Name") + " -pw " + self.params.GetString("Linux Password") + " " + \
        self.params.GetString("Servername") + " mv \"" + self.params.GetString("Linux Home Path") + "/" + str(self.dirname)[str(self.dirname).rfind("/")+1:] + "/lcmt_CALCULIX_Calculation_batch.bat\" " + self.params.GetString("Linux Home Path") 
        print commandline
        result = subprocess.call(commandline, shell = True, stdout = fnull, stderr = fnull)
        #commandline = "plink -batch -l UN -pw PW dynabox \'/home/rmjzettl/" + str(self.dirname[str(self.dirname).rfind("/")+1:] + "/") + "lcmt_CALCULIX_Calculation_batch.bat\'"
        #result = subprocess.call(commandline, shell = True, stdout = fnull, stderr = fnull)
        #commandline = "pscp -r -l UN -pw PW dynabox:\"/home/rmjzettl/"+ str(self.dirname)[str(self.dirname).rfind("/")+1:] + ".tar\" " + str(self.dirname)[0:3]
        #result = subprocess.call(commandline, shell = True, stdout = fnull, stderr = fnull)
        #commandline = "plink -batch -l UN -pw PW dynabox rm -f \"/home/rmjzettl/"+ str(self.dirname)[str(self.dirname).rfind("/")+1:] + ".tar\""
        #result = subprocess.call(commandline, shell = True, stdout = fnull, stderr = fnull)
        #commandline = "plink -batch -l UN -pw PW dynabox rm -f \"/home/rmjzettl/"+ str(self.dirname)[str(self.dirname).rfind("/")+1:] + ".zip\""
        #result = subprocess.call(commandline, shell = True, stdout = fnull, stderr = fnull)
        #commandline = "7z x \"" + str(self.dirname)[0:3] + str(self.dirname)[str(self.dirname).rfind("/")+1:] + ".tar\" -o\"" + str(self.dirname[0:str(self.dirname).rfind("/")]) + "\""
        #result = subprocess.call(commandline, shell = True, stdout = fnull, stderr = fnull)
        #commandline = "del /Q \"" + str(self.dirname)[0:3] + str(self.dirname)[str(self.dirname).rfind("/")+1:] + ".tar\""
        #result = subprocess.call(commandline, shell = True, stdout = fnull, stderr = fnull)
        #commandline = "del /Q \"" + str(self.dirname)[0:3] + str(self.dirname)[str(self.dirname).rfind("/")+1:] + ".zip\""
        #result = subprocess.call(commandline, shell = True, stdout = fnull, stderr = fnull)

        fnull.close()
        #Reset the GUI
        os.chdir("c:/")
        #Reset the table to be fully empty
        i = self.JobTable.rowCount()
        while i > 0:
            print i
            self.JobTable.removeRow(i-1)
            i = i-1

        print "after"
        self.JobTable.setHorizontalHeaderLabels(
        ["Input File","Output Folder","Z-Offset From","Z-Offset To","Z-Intervall","X-Rot From","X-Rot To","X-Rot Intervall",
        "Y-Rot From","Y-Rot To","Y-Rot Intervall","Z-Rot From","Z-Rot To","Z-Rot Intervall","Young Modulus","Poisson Ratio",
        "LC1","LC2","LC3","LC4","LC5","LC6","LTC1","LTC2","LTC3","LTC4","LTC5","LTC6","Plate Thickness"])
        self.button_select_file.setEnabled(True)
        self.button_select_output.setEnabled(True)


##Main function to be used for GUI setup and entering into the main loop
if __name__ == "__main__":
    app = QtGui.QApplication(sys.argv)
    myapp = MyForm()
    myapp.show()
    sys.exit(app.exec_())









