#pragma once
#include <memory>
#include <vector>
#include <functional> 
#include <set> 

#include "Solver.h"
#include "System.h"
//#include "Constraint.h"
//#include "NewtonRaphson.h"
//#include "QuasiIntegrator.h"

namespace MbD {
	class System;
	class Constraint;
	class Solver;
	class QuasiIntegrator;

	class SystemSolver : public Solver
	{
		//system parts jointsMotions forcesTorques sensors variables icTypeSolver setsOfRedundantConstraints errorTolPosKine errorTolAccKine 
		//iterMaxPosKine iterMaxAccKine basicIntegrator tstartPasts tstart hmin hmax tend toutFirst hout direction corAbsTol corRelTol 
		//intAbsTol intRelTol iterMaxDyn orderMax translationLimit rotationLimit 
	public:
		SystemSolver(System* x) : system(x) {
			initialize();
		}
		void setSystem(Solver* sys) override;
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
		void runQuasiKinematic();
		void runPosKine();
		void runVelKine();
		void runAccKine();
		void runPosICKine();
		void runVelICKine();
		void runAccICKine();
		void partsJointsMotionsDo(const std::function <void(std::shared_ptr<Item>)>& f);
		void logString(std::string& str);
		std::shared_ptr<std::vector<std::shared_ptr<Part>>> parts();
		//std::shared_ptr<std::vector<ContactEndFrame>> contactEndFrames();
		//std::shared_ptr<std::vector<UHolder>> uHolders();
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essentialConstraints2();
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> displacementConstraints();
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpendicularConstraints();
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allRedundantConstraints();
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints();
		
		virtual void postNewtonRaphson();
		void partsJointsMotionsForcesTorquesDo(const std::function <void(std::shared_ptr<Item>)>& f);
		void discontinuityBlock();
		double startTime();
		double outputStepSize();
		double maxStepSize();
		double minStepSize();
		double firstOutputTime();
		double endTime();
		void settime(double tnew);

		System* system; //Use raw pointer when pointing backwards.
		std::shared_ptr<Solver> icTypeSolver;
		std::shared_ptr<std::vector<std::shared_ptr<std::set<std::string>>>> setsOfRedundantConstraints;
		
		double errorTolPosKine = 1.0e-6;
		double errorTolAccKine = 1.0e-6;
		int iterMaxPosKine = 25;
		int iterMaxAccKine = 25;
		std::shared_ptr <QuasiIntegrator> basicIntegrator;
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
		int iterMaxDyn = 0;
		int orderMax = 0;
		double translationLimit = 0;
		double rotationLimit = 0;
	};
}

