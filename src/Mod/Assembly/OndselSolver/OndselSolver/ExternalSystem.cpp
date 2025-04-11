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
//#include <Mod/Assembly/App/AssemblyObject.h>
#include "System.h"

using namespace MbD;

void MbD::ExternalSystem::preMbDrun(std::shared_ptr<System> mbdSys)
{
	if (cadSystem) {
		cadSystem->preMbDrun(mbdSys);
	}
    else if (asmtAssembly) {
        asmtAssembly->preMbDrun(mbdSys);
        //asmtAssembly->externalSystem->preMbDrun(mbdSys);
    }
    else if (freecadAssemblyObject) {
        //freecadAssemblyObject->preMbDrun();
    }
    else {
		assert(false);
	}
}

void MbD::ExternalSystem::preMbDrunDragStep(std::shared_ptr<System> mbdSys, std::shared_ptr<std::vector<std::shared_ptr<Part>>> dragParts)
{
	asmtAssembly->preMbDrunDragStep(mbdSys, dragParts);
}

void MbD::ExternalSystem::updateFromMbD()
{
	if (cadSystem) {
		cadSystem->updateFromMbD();
	}
	else if (asmtAssembly) {
		asmtAssembly->updateFromMbD();
	}
    else if (freecadAssemblyObject) {
        //freecadAssemblyObject->updateFromMbD();
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
		asmtAssembly->outputResults(type);
        //asmtAssembly->externalSystem->outputFor(type);
    }
    else if (freecadAssemblyObject) {
        //freecadAssemblyObject->outputResults(type);
    }
    else {
		assert(false);
	}
}

void MbD::ExternalSystem::logString(const std::string& str)
{
	std::cout << str << std::endl;
}

void MbD::ExternalSystem::logString(double)
{
	assert(false);
}

void MbD::ExternalSystem::runOndselPiston()
{
	assert(false);
}

void MbD::ExternalSystem::runPiston()
{
	assert(false);
}

void MbD::ExternalSystem::postMbDrun()
{
	//Do nothing
}
