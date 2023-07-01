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
#include "PrescribedMotion.h"

//using namespace CAD;

namespace MbD {
	class Part;
	class Joint;
	class SystemSolver;
	class Time;
	class Constraint;
	class ForceTorqueItem;
	//class CAD::CADSystem;
	class CADSystem;

	class System : public Item
	{
		//ToDo: Needed members admSystem namedItems mbdTime parts jointsMotions forcesTorques sensors variables hasChanged mbdSystemSolver
	public:
		System();
		System(const char* str);
		System* root() override;
		void initialize() override;
		void initializeLocally() override;
		void initializeGlobally() override;
		void clear();
		void runKINEMATIC();
		void outputInput();
		void outputTimeSeries();
		std::shared_ptr<std::vector<std::string>> discontinuitiesAtIC();
		void jointsMotionsDo(const std::function <void(std::shared_ptr<Joint>)>& f);
		void partsJointsMotionsDo(const std::function <void(std::shared_ptr<Item>)>& f);
		void partsJointsMotionsForcesTorquesDo(const std::function <void(std::shared_ptr<Item>)>& f);
		void logString(std::string& str) override;
		double mbdTimeValue();
		void mbdTimeValue(double t);
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essentialConstraints2();
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> displacementConstraints();
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpendicularConstraints();
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allRedundantConstraints();
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints();
		void addPart(std::shared_ptr<Part> part);
		void addJoint(std::shared_ptr<Joint> joint);
		void addMotion(std::shared_ptr<PrescribedMotion> motion);

		double maximumMass();
		double maximumMomentOfInertia();
		double translationLimit();
		double rotationLimit();
		void outputFor(AnalysisType type);

		//CAD::CADSystem* externalSystem;	//Use raw pointer to point backwards
		CADSystem* externalSystem;	//Use raw pointer to point backwards
		std::shared_ptr<std::vector<std::shared_ptr<Part>>> parts;
		std::shared_ptr<std::vector<std::shared_ptr<Joint>>> jointsMotions;
		std::shared_ptr<std::vector<std::shared_ptr<ForceTorqueItem>>> forcesTorques;
		bool hasChanged = false;
		std::shared_ptr<SystemSolver> systemSolver;

		std::shared_ptr<Time> time;
	};
}
