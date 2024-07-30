/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include<algorithm>

#include "System.h"
#include "Part.h"
#include "Joint.h"
#include "ForceTorqueItem.h"
#include "SystemSolver.h"
#include "SymTime.h"
#include "CREATE.h"
#include "ExternalSystem.h"
#include "PrescribedMotion.h"

using namespace MbD;

System::System() {
	externalSystem = std::make_shared<ExternalSystem>();
	time = std::make_shared<Time>();
	parts = std::make_shared<std::vector<std::shared_ptr<Part>>>();
	jointsMotions = std::make_shared<std::vector<std::shared_ptr<Joint>>>();
	limits = std::make_shared<std::vector<std::shared_ptr<LimitIJ>>>();
	forcesTorques = std::make_shared<std::vector<std::shared_ptr<ForceTorqueItem>>>();
	systemSolver = std::make_shared<SystemSolver>(this);
}

System::System(const std::string& str) : Item(str) {
}

System* MbD::System::root()
{
	return this;
}

void System::initialize()
{
}

void System::addPart(std::shared_ptr<Part> part)
{
	part->setSystem(this);
	parts->push_back(part);
}

void MbD::System::addJoint(std::shared_ptr<Joint> joint)
{
	joint->owner = this;
	jointsMotions->push_back(joint);
}

void MbD::System::addMotion(std::shared_ptr<PrescribedMotion> motion)
{
	motion->owner = this;
	jointsMotions->push_back(motion);
}

void MbD::System::addLimit(std::shared_ptr<LimitIJ> limit)
{
	limit->owner = this;
	limits->push_back(limit);
}

void MbD::System::addForceTorque(std::shared_ptr<ForceTorqueItem> forTor)
{
	forTor->owner = this;
	forcesTorques->push_back(forTor);
}

void System::runKINEMATIC(std::shared_ptr<System> self)
{
	externalSystem->preMbDrun(self);
	while (true)
	{
		initializeLocally();
		initializeGlobally();
		if (!hasChanged) break;
	}
	partsJointsMotionsLimitsForcesTorquesDo([](std::shared_ptr<Item> item) { item->postInput(); });
	externalSystem->outputFor(INPUT);
	systemSolver->runAllIC();
	externalSystem->outputFor(INITIALCONDITION);
	systemSolver->runBasicKinematic();
	externalSystem->postMbDrun();
}

void System::initializeLocally()
{
	hasChanged = false;
	time->value = systemSolver->tstart;
	partsJointsMotionsLimitsForcesTorquesDo([](std::shared_ptr<Item> item) { item->initializeLocally(); });
	systemSolver->initializeLocally();
}

void System::initializeGlobally()
{
	partsJointsMotionsLimitsForcesTorquesDo([](std::shared_ptr<Item> item) { item->initializeGlobally(); });
	systemSolver->initializeGlobally();
}

void System::clear()
{
	name = std::string();
	parts->clear();
	jointsMotions->clear();
	forcesTorques->clear();
}

void MbD::System::runPreDrag(std::shared_ptr<System> self)
{
	externalSystem->preMbDrun(self);
	while (true)
	{
		initializeLocally();
		initializeGlobally();
		if (!hasChanged) break;
	}
	partsJointsMotionsLimitsForcesTorquesDo([](std::shared_ptr<Item> item) { item->postInput(); });
	systemSolver->runPreDrag();
	externalSystem->updateFromMbD();
}

void MbD::System::runDragStep(std::shared_ptr<System> self, std::shared_ptr<std::vector<std::shared_ptr<Part>>> dragParts)
{
	externalSystem->preMbDrunDragStep(self, dragParts);
	partsJointsMotionsLimitsForcesTorquesDo([](std::shared_ptr<Item> item) { item->postInput(); });
	systemSolver->runDragStep(dragParts);
	externalSystem->updateFromMbD();
}

std::shared_ptr<std::vector<std::string>> System::discontinuitiesAtIC()
{
	return std::make_shared<std::vector<std::string>>();
}

void System::jointsMotionsDo(const std::function<void(std::shared_ptr<Joint>)>& f)
{
	std::for_each(jointsMotions->begin(), jointsMotions->end(), f);
}

void System::partsJointsMotionsDo(const std::function<void(std::shared_ptr<Item>)>& f)
{
	std::for_each(parts->begin(), parts->end(), f);
	std::for_each(jointsMotions->begin(), jointsMotions->end(), f);
}

void System::partsJointsMotionsForcesTorquesDo(const std::function<void(std::shared_ptr<Item>)>& f)
{
	std::for_each(parts->begin(), parts->end(), f);
	std::for_each(jointsMotions->begin(), jointsMotions->end(), f);
	std::for_each(forcesTorques->begin(), forcesTorques->end(), f);
}

void MbD::System::partsJointsMotionsLimitsDo(const std::function<void(std::shared_ptr<Item>)>& f)
{
	std::for_each(parts->begin(), parts->end(), f);
	std::for_each(jointsMotions->begin(), jointsMotions->end(), f);
	std::for_each(limits->begin(), limits->end(), f);
}

void MbD::System::partsJointsMotionsLimitsForcesTorquesDo(const std::function<void(std::shared_ptr<Item>)>& f)
{
	std::for_each(parts->begin(), parts->end(), f);
	std::for_each(jointsMotions->begin(), jointsMotions->end(), f);
	std::for_each(limits->begin(), limits->end(), f);
	std::for_each(forcesTorques->begin(), forcesTorques->end(), f);
}

void System::logString(const std::string& str)
{
	externalSystem->logString(str);
}

double System::mbdTimeValue()
{
	return time->getValue();
}

void System::mbdTimeValue(double t)
{
	time->setValue(t);
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> System::essentialConstraints()
{
	auto essenConstraints = std::make_shared<std::vector<std::shared_ptr<Constraint>>>();
	this->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->fillEssenConstraints(essenConstraints); });
	return essenConstraints;
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> System::displacementConstraints()
{
	auto dispConstraints = std::make_shared<std::vector<std::shared_ptr<Constraint>>>();
	this->jointsMotionsDo([&](std::shared_ptr<Joint> joint) { joint->fillDispConstraints(dispConstraints); });
	return dispConstraints;
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> System::perpendicularConstraints()
{
	auto perpenConstraints = std::make_shared<std::vector<std::shared_ptr<Constraint>>>();
	this->jointsMotionsDo([&](std::shared_ptr<Joint> joint) { joint->fillPerpenConstraints(perpenConstraints); });
	return perpenConstraints;
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> System::allRedundantConstraints()
{
	auto redunConstraints = std::make_shared<std::vector<std::shared_ptr<Constraint>>>();
	this->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->fillRedundantConstraints(redunConstraints); });
	return redunConstraints;
}

std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> System::allConstraints()
{
	auto constraints = std::make_shared<std::vector<std::shared_ptr<Constraint>>>();
	this->partsJointsMotionsLimitsDo([&](std::shared_ptr<Item> item) { item->fillConstraints(constraints); });
	return constraints;
}

double System::maximumMass()
{
	auto maxPart = std::max_element(parts->begin(), parts->end(), [](auto& a, auto& b) { return a->m < b->m; });
	return maxPart->get()->m;
}

double System::maximumMomentOfInertia()
{
	double max = 0.0;
	for (size_t i = 0; i < parts->size(); i++)
	{
		auto& part = parts->at(i);
		for (size_t j = 0; j < 3; j++)
		{
			auto& aJ = part->aJ;
			auto aJi = aJ->at(j);
			if (max < aJi) max = aJi;
		}
	}
	return max;
}

double System::translationLimit()
{
	return systemSolver->translationLimit;
}

double System::rotationLimit()
{
	return systemSolver->rotationLimit;
}

void System::outputFor(AnalysisType type)
{
	externalSystem->outputFor(type);
}

bool MbD::System::limitsSatisfied()
{
	return  std::all_of(limits->cbegin(), limits->cend(), [](auto& limit) { return limit->satisfied(); });
}

void MbD::System::deactivateLimits()
{
	std::for_each(limits->cbegin(), limits->cend(), [](auto& limit) { limit->deactivate(); });
}
