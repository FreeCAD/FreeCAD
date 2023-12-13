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
	ASMTAssembly::runFile("../testapp/RevRevJt.asmt");
	ASMTAssembly::runFile("../testapp/RevCylJt.asmt");
	ASMTAssembly::runFile("../testapp/CylSphJt.asmt");
	ASMTAssembly::runFile("../testapp/SphSphJt.asmt");
	MBDynSystem::runFile("../testapp/MBDynCase(Cosine-half drive).mbd");
	MBDynSystem::runFile("../testapp/MBDynCase(Sine-forever drive).mbd");
	MBDynSystem::runFile("../testapp/MBDynCase9orig.mbd");	//SimulationStoppingError
	MBDynSystem::runFile("../testapp/MBDynCase8orig.mbd");	//Incompatible geometry at t=3.15
	MBDynSystem::runFile("../testapp/MBDynCase5orig.mbd");	//Test Product::integrateWRT
	ASMTAssembly::readWriteFile("../testapp/Gears.asmt");
	ASMTAssembly::readWriteFile("../testapp/anglejoint.asmt");
	ASMTAssembly::readWriteFile("../testapp/constvel.asmt");
	ASMTAssembly::readWriteFile("../testapp/rackscrew.asmt");
	ASMTAssembly::readWriteFile("../testapp/planarbug.asmt");
	MBDynSystem::runFile("../testapp/InitialConditions.mbd");
	MBDynSystem::runFile("../testapp/SphericalHinge.mbd");
	ASMTAssembly::runFile("../testapp/cirpendu2.asmt");	//Under constrained. Testing ICKine.
	ASMTAssembly::runFile("../testapp/quasikine.asmt");	//Under constrained. Testing ICKine.
	ASMTAssembly::readWriteFile("../testapp/piston.asmt");
	//MBDynSystem::runFile("../testapp/MBDynCaseDebug2.mbd");
	//return 0;
	MBDynSystem::runFile("../testapp/MBDynCase2.mbd");
	//MBDynSystem::runFile("../testapp/MBDynCase.mbd");	//Very large but works
	MBDynSystem::runFile("../testapp/CrankSlider2.mbd");
	//MBDynSystem::runFile("../testapp/crank_slider.mbd");	//Needs integration of product
	////ASMTAssembly::runSinglePendulumSuperSimplified();	//Mass is missing
	////ASMTAssembly::runSinglePendulumSuperSimplified2();	//DOF has infinite acceleration due to zero mass and inertias
	ASMTAssembly::runSinglePendulumSimplified();
	ASMTAssembly::runSinglePendulum();
	ASMTAssembly::runFile("../testapp/piston.asmt");
	ASMTAssembly::runFile("../testapp/00backhoe.asmt");
	//ASMTAssembly::runFile("../testapp/circular.asmt");	//Needs checking
	//ASMTAssembly::runFile("../testapp/engine1.asmt");	//Needs checking
	ASMTAssembly::runFile("../testapp/fourbar.asmt");
	//ASMTAssembly::runFile("../testapp/fourbot.asmt");	//Very large but works
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
