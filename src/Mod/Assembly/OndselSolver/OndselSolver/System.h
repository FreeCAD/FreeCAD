/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
/*****************************************************************//**
 * \file   System.h
 * \brief  Multibody system of parts, joints, forces.
 * 
 * \author askoh
 * \date   May 2023
 *********************************************************************/

#pragma once

#include <memory>
#include <vector>
#include <functional>

#include "Item.h"
#include "LimitIJ.h"

namespace MbD {
	class Part;
	class Joint;
	class SystemSolver;
	class Time;
	class Constraint;
	class PrescribedMotion;
	class ForceTorqueItem;
	class ExternalSystem;

	class System : public Item
	{
		//ToDo: Needed members admSystem namedItems mbdTime parts jointsMotions forcesTorques sensors variables hasChanged mbdSystemSolver
	public:
		System();
		System(const std::string& str);
		System* root() override;
		void initialize() override;
		void initializeLocally() override;
		void initializeGlobally() override;
		void clear();
		void runPreDrag(std::shared_ptr<System> self);
		void runDragStep(std::shared_ptr<System> self, std::shared_ptr<std::vector<std::shared_ptr<Part>>> dragParts);
		void runKINEMATIC(std::shared_ptr<System> self);
		std::shared_ptr<std::vector<std::string>> discontinuitiesAtIC();
		void jointsMotionsDo(const std::function <void(std::shared_ptr<Joint>)>& f);
		void partsJointsMotionsDo(const std::function <void(std::shared_ptr<Item>)>& f);
		void partsJointsMotionsForcesTorquesDo(const std::function <void(std::shared_ptr<Item>)>& f);
		void partsJointsMotionsLimitsForcesTorquesDo(const std::function <void(std::shared_ptr<Item>)>& f);
		void partsJointsMotionsLimitsDo(const std::function <void(std::shared_ptr<Item>)>& f);
		void logString(const std::string& str) override;
		double mbdTimeValue();
		void mbdTimeValue(double t);
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essentialConstraints();
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> displacementConstraints();
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpendicularConstraints();
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allRedundantConstraints();
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints();
		void addPart(std::shared_ptr<Part> part);
		void addJoint(std::shared_ptr<Joint> joint);
		void addMotion(std::shared_ptr<PrescribedMotion> motion);
		void addLimit(std::shared_ptr<LimitIJ> limit);
		void addForceTorque(std::shared_ptr<ForceTorqueItem> forTor);

		double maximumMass();
		double maximumMomentOfInertia();
		double translationLimit();
		double rotationLimit();
		void outputFor(AnalysisType type);
		bool limitsSatisfied();
		void deactivateLimits();

		std::shared_ptr<ExternalSystem> externalSystem;
		std::shared_ptr<std::vector<std::shared_ptr<Part>>> parts;
		std::shared_ptr<std::vector<std::shared_ptr<Joint>>> jointsMotions;
		std::shared_ptr<std::vector<std::shared_ptr<LimitIJ>>> limits;
		std::shared_ptr<std::vector<std::shared_ptr<ForceTorqueItem>>> forcesTorques;
		bool hasChanged = false;
		std::shared_ptr<SystemSolver> systemSolver;

		std::shared_ptr<Time> time;
	};
}
