#include<algorithm>

#include "System.h"

using namespace MbD;

System::System() {
	time = std::make_shared<Time>();
	parts = std::make_shared<std::vector<std::shared_ptr<Part>>>();
	jointsMotions = std::make_shared<std::vector<std::shared_ptr<Joint>>>();
	systemSolver = std::make_shared<SystemSolver>(this);
}

System::System(const char* str) : Item(str) {
	time = std::make_shared<Time>();
	parts = std::make_shared<std::vector<std::shared_ptr<Part>>>();
	jointsMotions = std::make_shared<std::vector<std::shared_ptr<Joint>>>();
	systemSolver = std::make_shared<SystemSolver>(this);
}

void System::addPart(std::shared_ptr<Part> part)
{
	part->setSystem(*this);
	parts->push_back(part);
}

void System::runKINEMATICS()
{
	//Smalltalk code
	//admSystem   preMbDrun.

	//	[self initializeLocally.
	//	self initializeGlobally.
	//	self hasChanged]
	//				whileTrue.
	//	self partsJointsMotionsForcesTorquesDo : [:item | item postInput] .
	//	admSystem outputFor : #INPUT.
	//	mbdSystemSolver runAllIC.
	//	admSystem outputFor : #'INITIAL CONDITIONS'.
	//	mbdSystemSolver runBasicKinematic.
	//	admSystem postMbDrun

	while (true)
	{
		initializeLocally();
		initializeGlobally();
		if (!hasChanged) break;
	}
	systemSolver->runAllIC();
	systemSolver->runBasicKinematic();
}

void System::initializeLocally() 
{
	hasChanged = false;
	time->value = systemSolver->tstart;
	std::for_each(parts->begin(), parts->end(), [](const auto& part) { part->initializeLocally(); });
	std::for_each(jointsMotions->begin(), jointsMotions->end(), [](const auto& joint) { joint->initializeLocally();	});
	systemSolver->initializeLocally();
}

void System::initializeGlobally()
{
	std::for_each(parts->begin(), parts->end(), [](const auto& part) { part->initializeGlobally(); });
	std::for_each(jointsMotions->begin(), jointsMotions->end(), [](const auto& joint) { joint->initializeGlobally();	});
	systemSolver->initializeGlobally();
}
