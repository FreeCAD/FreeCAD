import os,sys,string,math,shutil,glob,subprocess,tempfile,time
from os.path import join


##GUI related stuff
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import QObject, pyqtSignal,pyqtSlot
from lsf_submission_gui import Ui_DialogSubmitwithLSF
##------------------------------------------------


class ThreadWorker(QtCore.QThread):
    updateProgressBar = QtCore.pyqtSignal(int)
    finished_calcs = QtCore.pyqtSignal()
    def __init__(self,current_working_directory):
	super(ThreadWorker, self).__init__(parent=None)
	self.working_directory = current_working_directory

    def run(self):
 	print 'Worker Thread startet'
	self.launch_calculation()
	
	
    def launch_job(self,current_dir):
	initial_directory = os.getcwd()
        #print initial_directory
	#Now change to the current folder to make it easier for calculix
	os.chdir(current_dir)
        #print current_dir
        commandline = "/projects/MateriauxProcedes/FreeCAD/freecad_build/bin/ccx -i geometry_fe_input >& output.txt"
        result = subprocess.Popen(commandline, shell = True)
	os.chdir(initial_directory)
        #print os.getcwd()

    def checkJob(self,current_dir):
        #Check if the current job is finished or still running
        running = True
        #try to open the output_file now
        try:
           file_to_check = open(str(os.path.join(current_dir,"output.txt")),"r")
        except IOError:
           print "file not yet ready"
           return running
	find_str = " Job finished\n"
        file_to_check.seek (0, 2) # Put the file position pointer to the end of file
        fsize = file_to_check.tell() # Get current position
    	file_to_check.seek (max (fsize-500, 0), 0) # Set pos @ last 500 chars
        lines = file_to_check.readlines()       # Read to end
	file_to_check.close()
        if find_str in lines:
	   running = False
	   
	return running

    def launch_calculation(self):
        current_job_number = 0
        for root,dirs,files in os.walk(str(self.working_directory)):
            if 'geometry_fe_input.inp' in files:
                #build_launch_script(root) Not necessary for the moment
                self.launch_job(root)
                emergency_exit = 0
                while(self.checkJob(root)):
                    #Check if the job is still running or already finished and wait 30s between calls
                    time.sleep(1)
                    emergency_exit +=1
                    if emergency_exit > 10:
                        #Lets continue to the next job as this job seems to have problems
                        current_job_number += 1
			print "No convergence found . Skipping over to next job"
			self.updateProgressBar.emit(current_job_number)
                        break
                current_job_number += 1
                self.updateProgressBar.emit(current_job_number)
	
	self.finished_calcs.emit()
	



class Submission(QtGui.QDialog, Ui_DialogSubmitwithLSF):

    def __init__(self,working_dir,parent=None):
	super(Submission, self).__init__(parent)
        QtGui.QDialog.__init__(self,parent)
        self.setupUi(self)
        self.working_directory = working_dir
	self.number_of_jobs=0
	self.setValues()
        self.progressBar.setValue(0.0)
	self.thread = ThreadWorker(working_dir)
	self.thread.updateProgressBar.connect(self.progressBar.setValue)
	self.thread.finished_calcs.connect(self.setValuesAfterCalc)
        self.start_calculation.clicked.connect(self.launchWorkingThread)
        self.close_dialog.clicked.connect(self.quit_dialog)


    def build_launch_script(self,current_dir):
        #Now lets generate a LSF Job-File to be used by the Airbus Clusters
        lsf_input = open (str(os.path.join(current_dir,"job.lsf")),"wb")
        lsf_input.write("#!/bin/bash\n")
        lsf_input.write("export CCX_NPROC=" + str(self.numbercpus.value() ) + "\n")
        lsf_input.write("#BSUB -n "+ str(self.numbercpus.value()) + "\n")
        lsf_input.write("#BSUB -W 10:00\n")
        lsf_input.write("#BSUB -o %J.out\n")
        lsf_input.write("#BSUB -e %J.err\n")
        lsf_input.write("#BSUB -J calculix\n")
        lsf_input.write("#BSUB -q loc_dev_par\n")
        lsf_input.write(str("datadir=\"" + current_dir + "\""))
        lsf_input.write("cd $datadir\n")
        lsf_input.write("ccx -i geometry_fe_input\n")
        lsf_input.close()
        

    def setValues(self):
        #Get an Idea of how many jobs we have to calculate
        for root,dirs,files in os.walk(str(self.working_directory)):
            self.number_of_jobs += 1
        #Subtract one job as the root itself is also printed 
	self.number_of_jobs -=1
        self.progressBar.setMinimum(0)
        self.progressBar.setMaximum(self.number_of_jobs)

    

    def launchWorkingThread(self):
	#inactivate the buttons to avoid errors
	self.start_calculation.setEnabled(False)
	self.close_dialog.setEnabled(False)
	self.thread.start()

	
    def setValuesAfterCalc(self):
	self.close_dialog.setEnabled(True)
	
                
    def quit_dialog(self):
        self.close()

