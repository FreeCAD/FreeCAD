/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

 /*********************************************************************
  * @file  MbDCode.cpp
  *
  * @brief Program to assemble a piston crank system.
  *********************************************************************/

#include "../OndselSolver/CADSystem.h"
#include "../OndselSolver/CREATE.h"
#include "../OndselSolver/GESpMatParPvPrecise.h"
#include "../OndselSolver/ASMTAssembly.h"
#include "../OndselSolver/MomentOfInertiaSolver.h"

using namespace MbD;
void sharedptrTest();

int main()
{
	//ASMTAssembly::runFile("C:/Users/askoh/OneDrive/askoh/visualstudio/Ondsel/OndselFreeCAD/build/src/Main/runDragStep.asmt");
	//return 0;
	//auto assembly = ASMTAssembly::assemblyFromFile("C:/Users/askoh/OneDrive/askoh/visualstudio/Ondsel/OndselFreeCAD/build/src/Main/runPreDrag.asmt");
	//assembly->runDraggingLog("C:/Users/askoh/OneDrive/askoh/visualstudio/Ondsel/OndselFreeCAD/build/src/Main/dragging.log");
	//return 0;
	auto assembly = ASMTAssembly::assemblyFromFile(std::string(TEST_DATA_PATH) + "/runPreDragBackhoe3.asmt");
	assembly->runDraggingLog(std::string(TEST_DATA_PATH) + "/draggingBackhoe3.log");
	//return 0;
	ASMTAssembly::runDraggingLogTest3();
	ASMTAssembly::runDraggingLogTest2();
	ASMTAssembly::runDraggingLogTest();
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/pistonAllowZRotation.asmt");
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/Schmidt_Coupling_Ass_1-1.asmt");
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/RevRevJt.asmt");
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/RevCylJt.asmt");
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/CylSphJt.asmt");
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/SphSphJt.asmt");
	ASMTAssembly::readWriteFile(std::string(TEST_DATA_PATH) + "/Gears.asmt");
	ASMTAssembly::readWriteFile(std::string(TEST_DATA_PATH) + "/anglejoint.asmt");
	ASMTAssembly::readWriteFile(std::string(TEST_DATA_PATH) + "/constvel.asmt");
	ASMTAssembly::readWriteFile(std::string(TEST_DATA_PATH) + "/rackscrew.asmt");
	ASMTAssembly::readWriteFile(std::string(TEST_DATA_PATH) + "/planarbug.asmt");
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/cirpendu2.asmt");	//Under constrained. Testing ICKine.
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/quasikine.asmt");	//Under constrained. Testing ICKine.
	ASMTAssembly::readWriteFile(std::string(TEST_DATA_PATH) + "/piston.asmt");
	////ASMTAssembly::runSinglePendulumSuperSimplified();	//Mass is missing
	////ASMTAssembly::runSinglePendulumSuperSimplified2();	//DOF has infinite acceleration due to zero mass and inertias
	ASMTAssembly::runSinglePendulumSimplified();
	ASMTAssembly::runSinglePendulum();
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/piston.asmt");
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/00backhoe.asmt");
	//ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/circular.asmt");	//Needs checking
	//ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/engine1.asmt");	//Needs checking
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/fourbar.asmt");
	//ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/fourbot.asmt");	//Very large but works
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/wobpump.asmt");

	auto cadSystem = std::make_shared<CADSystem>();
	cadSystem->runOndselSinglePendulum();
	cadSystem->runOndselDoublePendulum();
	//cadSystem->runOndselPiston();		//For debugging
	cadSystem->runPiston();
	MomentOfInertiaSolver::example1();
	sharedptrTest();
}
void sharedptrTest() {
	auto assm = ASMTAssembly::With();

	std::shared_ptr<ASMTAssembly> assm1 = assm;	//New shared_ptr to old object. Reference count incremented.
	assert(assm == assm1);
	assert(assm.get() == assm1.get());
	assert(&assm != &assm1);
	assert(assm->constantGravity == assm1->constantGravity);
	assert(&(assm->constantGravity) == &(assm1->constantGravity));

	auto assm2 = std::make_shared<ASMTAssembly>(*assm);	//New shared_ptr to new object. Member variables copy old member variables
	assert(assm != assm2);
	assert(assm.get() != assm2.get());
	assert(&assm != &assm2);
	assert(assm->constantGravity == assm2->constantGravity);	//constantGravity is same object pointed to
	assert(&(assm->constantGravity) != &(assm2->constantGravity)); //Different shared_ptrs of same reference counter
}
