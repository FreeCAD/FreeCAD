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
//using namespace CAD;
void runSpMat();

int main()
{
	//ASMTAssembly::runFile("C:\\Users\\askoh\\OneDrive\\askoh\\visualworks\\vw8.1\\askoh\\64bit\\CADSM\\current\\asm\\00piston.asmt");

	auto externalSys = std::make_shared<CADSystem>();
	externalSys->runOndselPiston();
	//externalSys->runPiston();
	//runSpMat();
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