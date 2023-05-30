#include<algorithm>

#include "System.h"
#include "Part.h"
#include "Joint.h"
#include "ForceTorqueItem.h"
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
	forcesTorques = std::make_shared<std::vector<std::shared_ptr<ForceTorqueItem>>>();
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
	partsJointsMotionsForcesTorquesDo([&](std::shared_ptr<Item> item) { item->postInput(); });
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
	partsJointsMotionsDo([](const auto& item) { item->initializeLocally(); });
	systemSolver->initializeLocally();
}

void System::initializeGlobally()
{
	partsJointsMotionsDo([](const auto& item) { item->initializeGlobally(); });
	systemSolver->initializeGlobally();
}

std::shared_ptr<std::vector<std::string>> MbD::System::discontinuitiesAtIC()
{
	return std::shared_ptr<std::vector<std::string>>();
}

void MbD::System::jointsMotionsDo(const std::function<void(std::shared_ptr<Joint>)>& f)
{
	std::for_each(jointsMotions->begin(), jointsMotions->end(), f);
}

void MbD::System::partsJointsMotionsDo(const std::function<void(std::shared_ptr<Item>)>& f)
{
	std::for_each(parts->begin(), parts->end(), f);
	std::for_each(jointsMotions->begin(), jointsMotions->end(), f);
}

void MbD::System::partsJointsMotionsForcesTorquesDo(const std::function<void(std::shared_ptr<Item>)>& f)
{
	std::for_each(parts->begin(), parts->end(), f);
	std::for_each(jointsMotions->begin(), jointsMotions->end(), f);
	std::for_each(forcesTorques->begin(), forcesTorques->end(), f);
}

void MbD::System::logString(std::string& str)
{
	std::cout << str << std::endl;
}

double MbD::System::mbdTimeValue()
{
	return time->getValue();
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> MbD::System::essentialConstraints2()
{
	auto essenConstraints = std::make_shared<std::vector<std::shared_ptr<Constraint>>>();
	this->partsJointsMotionsDo([&](const auto& item) { item->fillEssenConstraints(essenConstraints); });
	return essenConstraints;
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> MbD::System::displacementConstraints()
{
	auto dispConstraints = std::make_shared<std::vector<std::shared_ptr<Constraint>>>();
	this->jointsMotionsDo([&](const auto& joint) { joint->fillDispConstraints(dispConstraints); });
	return dispConstraints;
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> MbD::System::perpendicularConstraints2()
{
	auto perpenConstraints = std::make_shared<std::vector<std::shared_ptr<Constraint>>>();
	this->jointsMotionsDo([&](const auto& joint) { joint->fillPerpenConstraints(perpenConstraints); });
	return perpenConstraints;
}
