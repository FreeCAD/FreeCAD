#pragma once
#include <memory>

#include "Solver.h"
#include "System.h"
#include "NewtonRaphson.h"

namespace MbD {
	class System;
	class SystemSolver : public Solver
	{
		//system parts jointsMotions forcesTorques sensors variables icTypeSolver setsOfRedundantConstraints errorTolPosKine errorTolAccKine 
		//iterMaxPosKine iterMaxAccKine basicIntegrator tstartPasts tstart hmin hmax tend toutFirst hout direction corAbsTol corRelTol 
		//intAbsTol intRelTol iterMaxDyn orderMax translationLimit rotationLimit 
	public:
		SystemSolver(System* x) : system(x) {
		}
		void initializeLocally();
		void initializeGlobally();
		void runAllIC();
		void runBasicKinematic();

		std::shared_ptr<NewtonRaphson> icTypeSolver;
		System* system;

		double tstart = 0;
		double tend = 10;
		double toutFirst = 0.0;
		double errorTolPosKine = 1.0e-6;
		int iterMaxPosKine = 100;
		double hmin = 1.0e-9;
		double hmax = 1.0;
		double hout = 1.0e-1;
		double direction = 1;
	};
}

