/*********************************************************************
 * @file  MbDCode.cpp
 *
 * @brief Program to assemble a piston crank system.
 *********************************************************************/

#include <iostream>	
#include "System.h"
#include "Constant.h"
#include "FullColumn.h"
#include "FullMatrix.h"
#include "DiagonalMatrix.h"
#include "Part.h"
#include "Joint.h"
#include "CylindricalJoint.h"
#include "RevoluteJoint.h"
#include "ZRotation.h"
#include "PartFrame.h"
#include "MarkerFrame.h"
#include "EndFrameqc.h"
#include "EndFrameqct.h"
#include "Product.h"
#include "Symbolic.h"
#include "SystemSolver.h"
#include "MbDCode.h"
#include "Time.h"
#include "CREATE.h"
#include "GESpMatParPvMarkoFast.h"
#include "GESpMatParPvPrecise.h"
#include "CADSystem.h"

using namespace MbD;
//using namespace CAD;
void runSpMat();

int main()
{
	auto externalSys = std::make_shared<CADSystem>();
	externalSys->runOndselPiston();
	externalSys->runPiston();
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