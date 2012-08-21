import os,sys,string,math,shutil,glob,subprocess,tempfile
from time import sleep
from os.path import join


##GUI related stuff
from PyQt4 import QtCore, QtGui
from lsf_submission_gui import Ui_DialogSubmitwithLSF
##------------------------------------------------



class submission(QtGui.QDialog, Ui_DialogSubmitwithLSF):
    def __init__(self,parent=None,working_directory):
        QtGui.QDialog.__init__(self,parent)
        self.setupUi(self)
        self.working_directory = working_directory
        self.number_of_jobs=0
        QtCore.QObject.connect(self.start_calculation, QtCore.SIGNAL("clicked()"), self.launch_calculation)
        QtCore.QObject.connect(self.close, QtCore.SIGNAL("clicked()"), self.quit)


    def build_lsf_script(self,current_dir):
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
        lsf_input.write(str("datadir=\" + current_dir + "\""))
        lsf_input.write("cd $datadir\n")
        lsf_input.write("ccx -i geometry_fe_input\n")
        lsf_input.close()
        

    def setValues(self):
        #Get an Idea of how many jobs we have to calculate
        for root,dirs,files in os.walk(str(self.working_directory)):
            number_of_jobs += 1
        print number_of_jobs
        self.progressBar.setMinimum(0)
        self.progressBar.setMaximum(number_of_jobs)

    def launch_job(self,current_dir):
        commandline = "bsub < " + os.path.join(current_dir,"job.lsf")
        print commandline
        result = subprocess.call(commandline, shell = True, stdout = fnull, stderr = fnull)
        
    def checkJob(self,current_dir):
        #Check if the current job is finished or still running
        running = True
        file_to_check = #Probably use glob to get the LSF output file right
        if (proper ending  in file):
            running = False
        
        return running

    def launch_calculation(self):
        current_job_number = 0
        for root,dirs,files in os.walk(str(self.working_directory)):
            if 'final_fe_input.inp' in files:
                build_lsf_script(root)
                launch_job(root)
                emergency_exit = 0
                while(check_job(root)):
                    #Check if the job is still running or already finished and wait 30s between calls
                    time.sleep(30)
                    emergency_exit +=1
                    if emergency_exit > 60:
                        #Lets continue to the next job as this job seems to have problems
                        current_job_number += 1
                        updateProgressbar(current_job_number)
                        continue
                current_job_number += 1
                updateProgressbar(current_job_number)
                
    def quit(self):
        self.close()

    def updateProgressbar(self,current_job_number):
        self.progressbar.setValue(current_job_number/self.number_of_jobs)   





