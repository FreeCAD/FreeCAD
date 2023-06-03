#pragma once
#include <memory>
#include <vector>
#include<functional> 

#include "Solver.h"
#include "System.h"
#include "Constraint.h"
//#include "NewtonRaphson.h"
#include "KineIntegrator.h"

namespace MbD {
	class System;
	class NewtonRaphson;

	class SystemSolver : public Solver
	{
		//system parts jointsMotions forcesTorques sensors variables icTypeSolver setsOfRedundantConstraints errorTolPosKine errorTolAccKine 
		//iterMaxPosKine iterMaxAccKine basicIntegrator tstartPasts tstart hmin hmax tend toutFirst hout direction corAbsTol corRelTol 
		//intAbsTol intRelTol iterMaxDyn orderMax translationLimit rotationLimit 
	public:
		SystemSolver(System* x) : system(x) {
			initialize();
		}
		void initialize() override;
		void initializeLocally() override;
		void initializeGlobally() override;
		void runAllIC();
		void runPosIC();
		void runVelIC();
		void runAccIC();
		bool needToRedoPosIC();
		void preCollision();
		void runCollisionDerivativeIC();
		void runBasicCollision();
		void runBasicKinematic();
		void partsJointsMotionsDo(const std::function <void(std::shared_ptr<Item>)>& f);
		void logString(std::string& str);
		std::shared_ptr<std::vector<std::shared_ptr<Part>>> parts();
		//std::shared_ptr<std::vector<ContactEndFrame>> contactEndFrames();
		//std::shared_ptr<std::vector<UHolder>> uHolders();
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essentialConstraints2();
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> displacementConstraints();
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpendicularConstraints2();

		System* system; //Use raw pointer when pointing backwards.
		std::shared_ptr<NewtonRaphson> icTypeSolver;
		std::shared_ptr<std::vector<std::vector<std::shared_ptr<Constraint>>>> setsOfRedundantConstraints;

		double errorTolPosKine = 1.0e-6;
		double errorTolAccKine = 1.0e-6;
		size_t iterMaxPosKine = 25;
		size_t iterMaxAccKine = 25;
		std::shared_ptr <KineIntegrator> basicIntegrator;
		std::shared_ptr<std::vector<double>> tstartPasts;
		double tstart = 0;
		double tend = 25;
		double toutFirst = 0.0;
		double hmin = 1.0e-9;
		double hmax = 1.0;
		double hout = 1.0e-1;
		double direction = 1;
		double corAbsTol = 0;
		double corRelTol = 0;
		double intAbsTol = 0;
		double intRelTol = 0;
		size_t iterMaxDyn = 0;
		size_t orderMax = 0;
		double translationLimit = 0;
		double rotationLimit = 0;
	};
}

