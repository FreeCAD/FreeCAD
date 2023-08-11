/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ExternalSystem.h"
#include "CADSystem.h"
#include "ASMTAssembly.h"
#include "System.h"

using namespace MbD;

void MbD::ExternalSystem::preMbDrun(std::shared_ptr<System> mbdSys)
{
	if (cadSystem) {
		cadSystem->preMbDrun(mbdSys);
	}
	else if (asmtAssembly) {
		asmtAssembly->preMbDrun(mbdSys);
	}
	else {
		assert(false);
	}
}

void MbD::ExternalSystem::outputFor(AnalysisType type)
{
	if (cadSystem) {
		cadSystem->updateFromMbD();
	}
	else if (asmtAssembly) {
		asmtAssembly->updateFromMbD();
		asmtAssembly->compareResults(type);
	}
	else {
		assert(false);
	}
}

void MbD::ExternalSystem::logString(std::string& str)
{
}

void MbD::ExternalSystem::logString(double value)
{
}

void MbD::ExternalSystem::runOndselPiston()
{
}

void MbD::ExternalSystem::runPiston()
{
}

void MbD::ExternalSystem::postMbDrun()
{
}
