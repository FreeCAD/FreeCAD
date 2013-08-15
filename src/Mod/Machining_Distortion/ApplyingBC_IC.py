def ApplyingBC_IC(Casedir,YoungModulus,PoissonCoeff,OUTER_GRID_No1,OUTER_GRID_No2,OUTER_GRID_No3,Meshobject) :
    # Variables generales
    import os,subprocess
    #
    AC_file = open (str(Casedir + "/" + "Applied_Conditions.txt"),'w')
    #
    #Setup local coordinate system by using *TRANSFORM keyword to avoid any rigid body motions during isostatic clamping
    #Outer_Grid_no_1 is origin, no_2 is on x-axis and no_3 is in local XY plane
    #Therefore 
    #OUTER_GRID_No2-OUTER_GRID_No1,OUTER_GRID_No3-OUTER_GRID_No1 is the input for the TRansform keyword
    AllNodes = Meshobject.FemMesh.Nodes
    GridNode1 = AllNodes[OUTER_GRID_No1]
    GridNode2 = AllNodes[OUTER_GRID_No2]
    GridNode3 = AllNodes[OUTER_GRID_No3]
    print GridNode1
    a_x = GridNode2[0]-GridNode1[0]
    a_y = GridNode2[1]-GridNode1[1]
    a_z = GridNode2[2]-GridNode1[2]
    b_x = GridNode3[0]-GridNode1[0]
    b_y = GridNode3[1]-GridNode1[1]
    b_z = GridNode3[2]-GridNode1[2]
    AC_file.write("** \n")
    AC_file.write("** Node Set for transformation card :\n")
    AC_file.write("*Nset, nset=transformed_nodes\n")
    AC_file.write(str(OUTER_GRID_No1)+","+str(OUTER_GRID_No2)+","+str(OUTER_GRID_No3)+"\n")
    AC_file.write("** \n")
    AC_file.write("** Transformation to avoid rigid body motions :\n")
    AC_file.write("*TRANSFORM, nset=transformed_nodes\n")
    AC_file.write(str(a_x)+","+str(a_y)+","+str(a_z)+","+str(b_x)+","+str(b_y)+","+str(b_z)+"\n")
    AC_file.write("** \n")
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
    fnull = open(os.devnull, 'w')
    if os.name != "posix":
        process = subprocess.Popen("type geometry_fe_input.inp Applied_Conditions.txt> final_fe_input.inp",shell=True)
        process.wait()
        process = subprocess.Popen("del /Q geometry_fe_input.inp",shell=True)
        process.wait()
    else:
        commandline = "cat Applied_Conditions.txt >> geometry_fe_input.inp"
        result = subprocess.call(commandline, shell = True, stdout = fnull, stderr = fnull)
    #
    fnull.close()
    return
