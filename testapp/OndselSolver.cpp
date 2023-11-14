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

#include <filesystem>
#include "../OndselSolver/CADSystem.h"
#include "../OndselSolver/CREATE.h"
#include "../OndselSolver/GESpMatParPvPrecise.h"
#include "../OndselSolver/ASMTAssembly.h"
#include "../OndselSolver/MBDynSystem.h"
#include "../OndselSolver/MomentOfInertiaSolver.h"

using namespace MbD;
void runSpMat();
void sharedptrTest();

int main()
{
	ASMTAssembly::readWriteFile("failingsolve.asmt");
	ASMTAssembly::runFile("cirpendu2.asmt");	//Under constrained. Testing ICKine.
	ASMTAssembly::runFile("quasikine.asmt");	//Under constrained. Testing ICKine.
	ASMTAssembly::readWriteFile("piston.asmt");
	//MBDynSystem::runFile("MBDynCaseDebug2.mbd");
	//return 0;
	MBDynSystem::runFile("MBDynCase2.mbd");
	MBDynSystem::runFile("MBDynCase.mbd");
	MBDynSystem::runFile("CrankSlider2.mbd");
	//MBDynSystem::runFile("crank_slider.mbd");	//Needs integration of product
	////ASMTAssembly::runSinglePendulumSuperSimplified();	//Mass is missing
	////ASMTAssembly::runSinglePendulumSuperSimplified2();	//DOF has infinite acceleration due to zero mass and inertias
	ASMTAssembly::runSinglePendulumSimplified();
	ASMTAssembly::runSinglePendulum();
	ASMTAssembly::runFile("../testapp/piston.asmt");
	ASMTAssembly::runFile("../testapp/00backhoe.asmt");
	//ASMTAssembly::runFile("circular.asmt");	//Needs checking
	//ASMTAssembly::runFile("engine1.asmt");	//Needs checking
	ASMTAssembly::runFile("../testapp/fourbar.asmt");
	//ASMTAssembly::runFile("fourbot.asmt");	//Very large but works
	ASMTAssembly::runFile("../testapp/wobpump.asmt");

	auto cadSystem = std::make_shared<CADSystem>();
	cadSystem->runOndselSinglePendulum();
	cadSystem->runOndselDoublePendulum();
	//cadSystem->runOndselPiston();		//For debugging
	cadSystem->runPiston();
	runSpMat();
	MomentOfInertiaSolver::example1();
	sharedptrTest();
}
void sharedptrTest() {
	auto assm = std::make_shared<ASMTAssembly>();

	auto assm1 = assm;	//New shared_ptr to old object. Reference count incremented.
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
void runSpMat() {
	auto spMat = std::make_shared<SparseMatrix<double>>(3, 3);
	spMat->atijput(0, 0, 1.0);
	spMat->atijput(0, 1, 1.0);
	spMat->atijput(1, 0, 1.0);
	spMat->atijput(1, 1, 1.0);
	spMat->atijput(1, 2, 1.0);
	spMat->atijput(2, 1, 1.0);
	spMat->atijput(2, 2, 1.0);
	auto fullCol = std::make_shared<FullColumn<double>>(3);
	fullCol->atiput(0, 1.0);
	fullCol->atiput(1, 2.0);
	fullCol->atiput(2, 3.0);
	auto matSolver = CREATE<GESpMatParPvPrecise>::With();
	auto answer = matSolver->solvewithsaveOriginal(spMat, fullCol, true);
	auto aAx = spMat->timesFullColumn(answer);
}
