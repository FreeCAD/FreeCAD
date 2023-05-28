#include<algorithm>

#include "System.h"
#include "Part.h"
#include "Joint.h"
#include "SystemSolver.h"
#include "Time.h"
#include "CREATE.h"

using namespace MbD;

System::System() {
	initialize();
}

System::System(const char* str) : Item(str) {
	initialize();
}

void MbD::System::initialize()
{
	time = CREATE<Time>::With();
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
	while (true)
	{
		initializeLocally();
		initializeGlobally();
		if (!hasChanged) break;
	}
	postInput();
	outputInput();
	systemSolver->runAllIC();
	outputInitialConditions();
	systemSolver->runBasicKinematic();
	outputTimeSeries();
}

void MbD::System::outputInput()
{
}

void MbD::System::outputInitialConditions()
{
}

void MbD::System::outputTimeSeries()
{
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

std::shared_ptr<std::vector<std::string>> MbD::System::discontinuitiesAtIC()
{
	return std::shared_ptr<std::vector<std::string>>();
}

void MbD::System::partsJointsMotionsDo(const std::function<void(std::shared_ptr<Item>)>& f)
{
	std::for_each(parts->begin(), parts->end(), f);
	std::for_each(jointsMotions->begin(), jointsMotions->end(), f);
}

void MbD::System::logString(std::string& str)
{
	std::cout << str << std::endl;
}

double MbD::System::mbdTimeValue()
{
	return time->getValue();
}
