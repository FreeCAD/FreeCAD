+ the following tests fail on ci, TODO somehow fix, even if they pass on my local machine:


make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_solver_calculix.TestSolverCalculix.test_constraint_centrif
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_solver_calculix.TestSolverCalculix.test_constraint_contact_solid_solid
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_solver_z88.TestSolverZ88.test_ccx_cantilever_ele_tria6
