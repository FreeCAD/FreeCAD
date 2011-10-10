def ApplyingBC_IC(Casedir,YoungModulus,PoissonCoeff,OUTER_GRID_No1,OUTER_GRID_No2,OUTER_GRID_No3) :
    # Variables generales
    import os,subprocess
    #
    AC_file = open (str(Casedir + "/" + "Applied_Conditions.txt"),'w')
    #
    # a) BOUNDARY conditions in order to prevent the billet to any solid displacement
    AC_file.write("** \n")
    AC_file.write("** BOUNDARY Conditions :\n")
    AC_file.write("*BOUNDARY\n")
    AC_file.write("%i, 1, 3\n" %OUTER_GRID_No1)
    AC_file.write("*BOUNDARY\n")
    AC_file.write("%i, 2, 3\n" %OUTER_GRID_No2)
    AC_file.write("*BOUNDARY \n")
    AC_file.write("%i, 3\n" %OUTER_GRID_No3)
    AC_file.write("** \n")
    #
    # b) MATERIAL description :
    MAT_NAME = "EL"
    AC_file.write("*MATERIAL, NAME=%s \n" %MAT_NAME)
    AC_file.write("*ELASTIC \n")
    AC_file.write("%f, %f \n" %(YoungModulus,PoissonCoeff)) 
    #    Assign the material properties to the desired group of elements :
    AC_file.write("*SOLID SECTION, ELSET=Eall, MATERIAL=%s \n" %MAT_NAME)
    AC_file.write("** \n")
    #
    # c) Define the load :
    AC_file.write("** LOAD :\n")
    #-Load_1: Points Loading :
    #-Load_1: AC_file.write("*NSET, NSET=LOAD \n")  # list of Nodes where Concentrated Load will be applied
    #-Load_1: AC_file.write("??, ??, ??, ... \n") 
    #-Load_2 : For applying Internal (residual) stresses (cf.  user-subroutine "sigini.f") :
    AC_file.write("*INITIAL CONDITIONS, TYPE=STRESS, USER \n")
    #
    # d) FEM-calculation  pilote :
    AC_file.write("** \n")
    AC_file.write("*STEP \n")
    AC_file.write("*STATIC \n")
    #-Load_1: AC_file.write("*CLOAD \n")  #  for Applying Concentrated Load 
    #-Load_1: AC_file.write("LOAD,3,Magnitude_per_Node \n")  # force acting in the Z-dir, force magnitude per Node

    #
    # e) Specify  the OUTPUT requested :
    AC_file.write("** OUTPUT REQUESTS \n")
    # If one wants to store each 0^th step the results of present calculation for future restarts
    # AC_file.write("*Restart, write, frequency=0 \n")  
    AC_file.write("*NODE FILE \n")  
    AC_file.write("U \n")   #  displacements expressed in the GLOBAL Coord System
    # AC_file.write("** \n")
    AC_file.write("*EL FILE \n")
    AC_file.write("S \n")  #  (Cauchy) Stresses 
    #
    AC_file.write("*NODE PRINT , NSET=Nall \n")  
    AC_file.write("U \n")   #  displacements expressed in the GLOBAL Coord System
    # AC_file.write("** \n")
    AC_file.write("*EL PRINT , ELSET=Eall \n")
    AC_file.write("S \n")  #  (Cauchy) Stresses 
    # AC_file.write("** \n")
    #
    AC_file.write("*END STEP")
    #
    AC_file.close() 
    os.chdir(str(Casedir))
    if os.name != "posix":
        process = subprocess.Popen("type geometry_fe_input.inp Applied_Conditions.txt> final_fe_input.inp",shell=True)
        process.wait()
        process = subprocess.Popen("del /Q geometry_fe_input.inp",shell=True)
        process.wait()
    else:
        CommandLine = "cat Applied_Conditions.txt >> geometry_fe_input.inp"


    #
    return
