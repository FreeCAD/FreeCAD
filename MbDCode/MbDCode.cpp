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

#include <iostream>	
#include <fstream>	
#include <filesystem>

#include "CADSystem.h"
#include "CREATE.h"
#include "GESpMatParPvPrecise.h"
#include "ASMTAssembly.h"

using namespace MbD;
void runSpMat();

int main()
{
	ASMTAssembly::runFile("piston.asmt");
	ASMTAssembly::runFile("00backhoe.asmt");
	ASMTAssembly::runFile("circular.asmt");
	ASMTAssembly::runFile("cirpendu.asmt");	//Under constrained. Testing ICKine.
	ASMTAssembly::runFile("engine1.asmt");
	ASMTAssembly::runFile("fourbar.asmt");
	ASMTAssembly::runFile("fourbot.asmt");
	ASMTAssembly::runFile("wobpump.asmt");

	auto cadSystem = std::make_shared<CADSystem>();
	cadSystem->runOndselPiston();
	cadSystem->runPiston();
	runSpMat();
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