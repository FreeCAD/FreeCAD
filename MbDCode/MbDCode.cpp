/*********************************************************************
 * @file  MbDCode.cpp
 *
 * @brief Program to assemble a piston crank system.
 *********************************************************************/

#include <iostream>	
#include "System.h"
#include "Constant.h"
#include "FullColumn.h"
#include "FullMatrix.h"
#include "DiagonalMatrix.h"
#include "Part.h"
#include "Joint.h"
#include "CylindricalJoint.h"
#include "RevoluteJoint.h"
#include "ZRotation.h"
#include "PartFrame.h"
#include "MarkerFrame.h"
#include "EndFrameqc.h"
#include "EndFrameqct.h"
#include "Product.h"
#include "Symbolic.h"
#include "SystemSolver.h"
#include "MbDCode.h"
#include "Time.h"
#include "CREATE.h"

using namespace MbD;

int main()
{
	std::cout << "Hello World!\n";
	System& TheSystem = System::getInstance();
	std::string name = "TheSystem";
	TheSystem.setName(name);
	std::cout << "TheSystem.getName() " << TheSystem.getName() << std::endl;
	auto& systemSolver = TheSystem.systemSolver;
	systemSolver->errorTolPosKine = 1.0e-6;
	systemSolver->errorTolAccKine = 1.0e-6;
	systemSolver->iterMaxPosKine = 25;
	systemSolver->iterMaxAccKine = 25;
	systemSolver->tstart = 0;
	systemSolver->tend = 25.0;
	systemSolver->hmin = 2.5e-8;
	systemSolver->hmax = 25.0;
	systemSolver->hout = 1.0;
	systemSolver->corAbsTol = 1.0e-6;
	systemSolver->corRelTol = 1.0e-6;
	systemSolver->intAbsTol = 1.0e-6;
	systemSolver->intRelTol = 1.0e-6;
	systemSolver->iterMaxDyn = 4;
	systemSolver->orderMax = 5;
	systemSolver->translationLimit = 9.6058421285615e9;
	systemSolver->rotationLimit = 0.5;

	std::string str;
	FColDsptr qX, qE, qXdot, omeOpO;
	FColDsptr rpmp;
	FMatDsptr aApm;
	FRowDsptr fullRow;
	//
	auto assembly1 = CREATE<Part>::With("/Assembly1");
	std::cout << "assembly1->getName() " << assembly1->getName() << std::endl;
	assembly1->m = 0.0;
	assembly1->aJ = std::make_shared<DiagonalMatrix<double>>(ListD{ 0, 0, 0 });
	qX = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0, 1 });
	assembly1->setqX(qX);
	assembly1->setqE(qE);
	//qXdot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	//omeOpO = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0, 1 });
	//assembly1->setqXdot(qXdot);
	//assembly1->setomeOpO(omeOpO);
	std::cout << "assembly1->getqX() " << *assembly1->getqX() << std::endl;
	std::cout << "assembly1->getqE() " << *assembly1->getqE() << std::endl;
	TheSystem.addPart(assembly1);
	{
		auto& partFrame = assembly1->partFrame;
		auto marker2 = CREATE<MarkerFrame>::With("/Assembly1/Marker2");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.0, 0.0, 0.0 });
		marker2->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{ 1, 0, 0 },
			{ 0, 1, 0 },
			{ 0, 0, 1 }
			});
		marker2->setaApm(aApm);
		partFrame->addMarkerFrame(marker2);
		//
		auto marker1 = CREATE<MarkerFrame>::With("/Assembly1/Marker1");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.0, 2.8817526385684, 0.0 });
		marker1->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{ 1, 0, 0 },
			{ 0, 0, 1 },
			{ 0, -1, 0 }
			});
		marker1->setaApm(aApm);
		partFrame->addMarkerFrame(marker1);
	}
	assembly1->asFixed();
	//
	auto crankPart1 = CREATE<Part>::With("/Assembly1/Part1");
	std::cout << "crankPart1->getName() " << crankPart1->getName() << std::endl;
	crankPart1->m = 0.045210530089461;
	crankPart1->aJ = std::make_shared<DiagonalMatrix<double>>(ListD{ 1.7381980042084e-4, 0.003511159968501, 0.0036154518487535 });
	qX = std::make_shared<FullColumn<double>>(ListD{ 0.38423368514246, 2.6661567755108e-17, -0.048029210642807 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0.0, 0.0, 0.0, 1.0 });
	crankPart1->setqX(qX);
	crankPart1->setqE(qE);
	TheSystem.parts->push_back(crankPart1);
	{
		auto& partFrame = crankPart1->partFrame;
		auto marker1 = CREATE<MarkerFrame>::With("/Assembly1/Part1/Marker1");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ -0.38423368514246, -2.6661567755108e-17, 0.048029210642807 });
		marker1->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{ 1, 0, 0 },
			{ 0, 1, 0 },
			{ 0, 0, 1 }
			});
		marker1->setaApm(aApm);
		partFrame->addMarkerFrame(marker1);
		//
		auto marker2 = CREATE<MarkerFrame>::With("/Assembly1/Part1/Marker2");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.38423368514246, -2.6661567755108e-17, 0.048029210642807 });
		marker2->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{ 1, 0, 0 },
			{ 0, 1, 0 },
			{ 0, 0, 1 }
			});
		marker2->setaApm(aApm);
		partFrame->addMarkerFrame(marker2);
	}
	//
	auto conrodPart2 = CREATE<Part>::With("/Assembly1/Part2");
	std::cout << "conrodPart2->getName() " << conrodPart2->getName() << std::endl;
	conrodPart2->m = 0.067815795134192;
	conrodPart2->aJ = std::make_shared<DiagonalMatrix<double>>(ListD{ 2.6072970063126e-4, 0.011784982468533, 0.011941420288912 });
	qX = std::make_shared<FullColumn<double>>(ListD{ 0.38423368514246, 0.49215295678475, 0.048029210642807 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0.0, 0.0, 0.89871703427292, 0.43852900965351 });
	conrodPart2->setqX(qX);
	conrodPart2->setqE(qE);
	TheSystem.parts->push_back(conrodPart2);
	{
		auto& partFrame = conrodPart2->partFrame;
		auto marker1 = CREATE<MarkerFrame>::With("/Assembly1/Part2/Marker1");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ -0.6243797383565, 1.1997705489799e-16, -0.048029210642807 });
		marker1->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{1.0, 2.7755575615629e-16, 0.0},
			{-2.7755575615629e-16, 1.0, 0.0},
			{0.0, 0.0, 1.0} 
			});
		marker1->setaApm(aApm);
		partFrame->addMarkerFrame(marker1);
		//
		auto marker2 = CREATE<MarkerFrame>::With("/Assembly1/Part2/Marker2");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.6243797383565, -2.1329254204087e-16, -0.048029210642807 });
		marker2->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{1.0, 2.4980018054066e-16, 2.2204460492503e-16},
			{-2.4980018054066e-16, 1.0, 4.1633363423443e-17},
			{-2.2204460492503e-16, -4.1633363423443e-17, 1.0} 
			});
		marker2->setaApm(aApm);
		partFrame->addMarkerFrame(marker2);
	}
	//
	auto pistonPart3 = CREATE<Part>::With("/Assembly1/Part3");
	std::cout << "pistonPart3->getName() " << pistonPart3->getName() << std::endl;
	pistonPart3->m = 1.730132083368;
	pistonPart3->aJ = std::make_shared<DiagonalMatrix<double>>(ListD{ 0.19449049546716, 0.23028116340971, 0.23028116340971 });
	qX = std::make_shared<FullColumn<double>>(ListD{ -1.283972762056e-18, 1.4645980199976, -4.7652385308244e-17 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0.70710678118655, 0.70710678118655, 0.0, 0.0 });
	pistonPart3->setqX(qX);
	pistonPart3->setqE(qE);
	TheSystem.parts->push_back(pistonPart3);
	{
		auto& partFrame = pistonPart3->partFrame;
		auto marker1 = CREATE<MarkerFrame>::With("/Assembly1/Part3/Marker1");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ -0.48029210642807, 7.6201599718927e-18, -2.816737703896e-17 });
		marker1->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{9.2444637330587e-33, 1.0, 2.2204460492503e-16},
			{1.0, -9.2444637330587e-33, -1.0785207688569e-32},
			{-1.0785207688569e-32, 2.2204460492503e-16, -1.0} 
			});
		marker1->setaApm(aApm);
		partFrame->addMarkerFrame(marker1);
		//
		auto marker2 = CREATE<MarkerFrame>::With("/Assembly1/Part3/Marker2");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.48029210642807, 1.7618247880058e-17, 2.5155758471256e-17 });
		marker2->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{6.9388939039072e-18, -6.4146353042213e-50, 1.0},
			{1.0, -6.9388939039072e-18, 6.9388939039072e-18},
			{-6.9388939039072e-18, 1.0, -7.4837411882581e-50} 
			});
		marker2->setaApm(aApm);
		partFrame->addMarkerFrame(marker2);
	}
	//
	auto revJoint1 = CREATE<RevoluteJoint>::With("/Assembly1/Joint1");
	std::cout << "revJoint1->getName() " << revJoint1->getName() << std::endl;
	revJoint1->connectsItoJ(assembly1->partFrame->endFrame("/Assembly1/Marker2"), crankPart1->partFrame->endFrame("/Assembly1/Part1/Marker1"));
	TheSystem.jointsMotions->push_back(revJoint1);

	auto revJoint2 = CREATE<RevoluteJoint>::With("/Assembly1/Joint2");
	std::cout << "revJoint2->getName() " << revJoint2->getName() << std::endl;
	revJoint2->connectsItoJ(crankPart1->partFrame->endFrame("/Assembly1/Part1/Marker2"), conrodPart2->partFrame->endFrame("/Assembly1/Part2/Marker1"));
	TheSystem.jointsMotions->push_back(revJoint2);
	
	auto revJoint3 = CREATE<RevoluteJoint>::With("/Assembly1/Joint3");
	std::cout << "revJoint3->getName() " << revJoint3->getName() << std::endl;
	revJoint3->connectsItoJ(conrodPart2->partFrame->endFrame("/Assembly1/Part2/Marker2"), pistonPart3->partFrame->endFrame("/Assembly1/Part3/Marker1"));
	TheSystem.jointsMotions->push_back(revJoint3);

	auto cylJoint4 = CREATE<CylindricalJoint>::With("/Assembly1/Joint4");
	std::cout << "cylJoint4->getName() " << cylJoint4->getName() << std::endl;
	cylJoint4->connectsItoJ(pistonPart3->partFrame->endFrame("/Assembly1/Part3/Marker2"), assembly1->partFrame->endFrame("/Assembly1/Marker1"));
	TheSystem.jointsMotions->push_back(cylJoint4);

	auto rotMotion1 = CREATE<ZRotation>::With("/Assembly1/Motion1");
	rotMotion1->connectsItoJ(assembly1->partFrame->endFrame("/Assembly1/Marker2"), crankPart1->partFrame->endFrame("/Assembly1/Part1/Marker1"));
	std::cout << "rotMotion1->getName() " << rotMotion1->getName() << std::endl;
	auto omega = std::make_shared<Constant>(6.2831853071796);
	auto timeScale = std::make_shared<Constant>(0.04);
	auto time = std::make_shared<Product>(timeScale, TheSystem.time);
	rotMotion1->phiBlk = std::make_shared<Product>(omega, time);
	std::cout << "rotMotion1->phiBlk " << *(rotMotion1->phiBlk) << std::endl;
	TheSystem.jointsMotions->push_back(rotMotion1);
	//
	TheSystem.runKINEMATICS();
}