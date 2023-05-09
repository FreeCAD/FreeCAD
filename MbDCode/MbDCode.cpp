/*********************************************************************
 * @file  MbDCode.cpp
 *
 * @brief Program to assemble a piston crank system.
 *********************************************************************/

#include <iostream>	
#include "System.h"
#include "FullColumn.h"
#include "FullMatrix.h"
#include "Part.h"
#include "Joint.h"
#include "CylindricalJoint.h"
#include "RevoluteJoint.h"
#include "ZRotation.h"
#include "EndFrameqc.h"
#include "MbDCode.h"

using namespace MbD;

int main()
{
	std::cout << "Hello World!\n";
	//System& TheSystem = System::getInstance();
	System& TheSystem = System::getInstance("TheSystem");
	std::cout << "TheSystem.getName() " << TheSystem.getName() << std::endl;
	auto systemSolver = TheSystem.systemSolver;
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
	FColDsptr qX, qE;
	FColDsptr rpmp;
	FMatDsptr aApm;
	FRowDsptr fullRow;
	//
	auto assembly1 = std::make_shared<Part>("Assembly1");
	std::cout << "assembly1->getName() " << assembly1->getName() << std::endl;
	qX = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0, 1 });
	assembly1->setqX(qX);
	assembly1->setqE(qE);
	std::cout << "assembly1->getqX() " << assembly1->getqX()->toString() << std::endl;
	std::cout << "assembly1->getqE() " << assembly1->getqE()->toString() << std::endl;
	TheSystem.addPart(assembly1);
	{
		auto partFrame = assembly1->partFrame;
		auto marker1 = std::make_shared<MarkerFrame>("Marker1");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.38423366582893, 6.8384291794733e-9, -0.048029210642807 });
		marker1->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{ 1, 0, 0 },
			{ 0, 0, 1 },
			{ 0, -1, 0 }
			});
		marker1->setaApm(aApm);
		partFrame->addMarkerFrame(marker1);
		//
		auto marker2 = std::make_shared<MarkerFrame>("Marker2");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.0, 0.0, 0.0 });
		marker2->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{ 1, 0, 0 },
			{ 0, 1, 0 },
			{ 0, 0, 1 }
			});
		marker2->setaApm(aApm);
		partFrame->addMarkerFrame(marker2);
	}
	assembly1->asFixed();
	//
	auto crankPart1 = std::make_shared<Part>("Part1");
	qX = std::make_shared<FullColumn<double>>(ListD{ 0.38423366582893, 6.8384291794733e-9, -0.048029210642807 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0.0, 0.0, 1.4248456266393e-10, 1.0 });
	crankPart1->setqX(qX);
	crankPart1->setqE(qE);
	TheSystem.parts->push_back(crankPart1);
	{
		auto partFrame = crankPart1->partFrame;
		auto marker1 = std::make_shared<MarkerFrame>("Marker1");
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
		auto marker2 = std::make_shared<MarkerFrame>("Marker2");
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
	auto conrodPart2 = std::make_shared<Part>("Part2");
	qX = std::make_shared<FullColumn<double>>(ListD{ 0.38423366582893,  0.49215308269277,  0.048029210642807 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0.0, 0.0, 0.89871701272344, 0.4385290538168 });
	conrodPart2->setqX(qX);
	conrodPart2->setqE(qE);
	TheSystem.parts->push_back(conrodPart2);
	{
		auto partFrame = conrodPart2->partFrame;
		auto marker1 = std::make_shared<MarkerFrame>("Marker1");
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
		auto marker2 = std::make_shared<MarkerFrame>("Marker2");
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
	auto pistonPart3 = std::make_shared<Part>("Part3");
	qX = std::make_shared<FullColumn<double>>(ListD{ -1.284772285311e-18, 1.4645982581368, -4.788228906425e-17 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0.70710678118655, 0.70710678118655, 0.0, 0.0 });
	pistonPart3->setqX(qX);
	pistonPart3->setqE(qE);
	TheSystem.parts->push_back(pistonPart3);
	{
		auto partFrame = pistonPart3->partFrame;
		auto marker1 = std::make_shared<MarkerFrame>("Marker1");
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
		auto marker2 = std::make_shared<MarkerFrame>("Marker2");
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
	auto cylJoint4 = std::make_shared<CylindricalJoint>("CylJoint4");
	cylJoint4->connectsItoJ(pistonPart3->partFrame->endFrame("Marker2"), assembly1->partFrame->endFrame("Marker1"));
	TheSystem.jointsMotions->push_back(cylJoint4);

	auto revJoint3 = std::make_shared<RevoluteJoint>("RevJoint3");
	revJoint3->connectsItoJ(conrodPart2->partFrame->endFrame("Marker2"), pistonPart3->partFrame->endFrame("Marker1"));
	TheSystem.jointsMotions->push_back(revJoint3);
	
	auto revJoint2 = std::make_shared<RevoluteJoint>("RevJoint2");
	revJoint2->connectsItoJ(crankPart1->partFrame->endFrame("Marker2"), conrodPart2->partFrame->endFrame("Marker1"));
	TheSystem.jointsMotions->push_back(revJoint2);
	
	auto revJoint1 = std::make_shared<RevoluteJoint>("RevJoint1");
	revJoint1->connectsItoJ(assembly1->partFrame->endFrame("Marker2"), crankPart1->partFrame->endFrame("Marker1"));
	TheSystem.jointsMotions->push_back(revJoint1);
	
	auto rotMotion1 = std::make_shared<ZRotation>("RotMotion1");
	rotMotion1->connectsItoJ(assembly1->partFrame->endFrame("Marker2"), crankPart1->partFrame->endFrame("Marker1"));
	TheSystem.jointsMotions->push_back(rotMotion1);
	//
	TheSystem.runKINEMATICS();
}