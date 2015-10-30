
1) I am not aware the Fem/App/CMakeList.txt also keep a list of py files.

which I think Fem/CMakeList.txt should do

SET(FemScripts_SRCS


2) multiple analysis , is possible with careful operation

select the new geometry and add new mesh and then new analysis. 
double click to activate one Analysis and then click toolbar show result 

3) UnitTest failed at compring inp file, but this file is good to solve 

Comparing /home/qingfeng/FreeCAD/build/Mod/Fem/test_files/cube_static.inp to /tmp/FEM_static/Mesh.inp


4) test without make install may failed, I tested after make install, 
GUI paste example is working. 
