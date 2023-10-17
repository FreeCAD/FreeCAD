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
#include "CADSystem.h"
#include "CREATE.h"
#include "GESpMatParPvPrecise.h"
#include "ASMTAssembly.h"
#include "MBDynSystem.h"
#include "MomentOfInertiaSolver.h"

using namespace MbD;
void runSpMat();

int main()
{
	MBDynSystem::runFile("MBDynCase.mbd");		//To be completed
	MBDynSystem::runFile("crank_slider.mbd");		//To be completed
	//ASMTAssembly::runSinglePendulumSuperSimplified();	//Mass is missing
	//ASMTAssembly::runSinglePendulumSuperSimplified2();	//DOF has infinite acceleration due to zero mass and inertias
	ASMTAssembly::runSinglePendulumSimplified();
	ASMTAssembly::runSinglePendulum();
	ASMTAssembly::runFile("piston.asmt");
	ASMTAssembly::runFile("00backhoe.asmt");
	//ASMTAssembly::runFile("circular.asmt");	//Needs checking
	//ASMTAssembly::runFile("cirpendu.asmt");	//Under constrained. Testing ICKine.
	//ASMTAssembly::runFile("engine1.asmt");	//Needs checking
	ASMTAssembly::runFile("fourbar.asmt");
	//ASMTAssembly::runFile("fourbot.asmt");	//Very large but works
	ASMTAssembly::runFile("wobpump.asmt");

	auto cadSystem = std::make_shared<CADSystem>();
	cadSystem->runOndselSinglePendulum();
	cadSystem->runOndselDoublePendulum();
	//cadSystem->runOndselPiston();		//For debugging
	cadSystem->runPiston();
	runSpMat();
	MomentOfInertiaSolver::example1();
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
