/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <string>
#include <iostream>

#include "CADSystem.h"
#include "CREATE.h"
#include "System.h"
#include "Item.h"
#include "Product.h"
#include "Constant.h"
#include "ZRotation.h"
#include "RevoluteJoint.h"
#include "CylindricalJoint.h"
#include "SystemSolver.h"
#include "Part.h"
#include "MarkerFrame.h"
#include "PartFrame.h"
#include "SymTime.h"
#include "StateData.h"
#include "EulerParameters.h"

using namespace MbD;

void CADSystem::outputFor(AnalysisType)
{
	auto str = std::to_string(mbdSystem->mbdTimeValue());
	this->logString(str);
	mbdSystem->partsJointsMotionsLimitsForcesTorquesDo([](std::shared_ptr<Item> item) {
		std::cout << std::endl;
		std::cout << item->classname() << " " << item->name << std::endl;
		auto data = item->stateData();
		std::cout << *data << std::endl;
		});
}

void CADSystem::logString(const std::string& str)
{
	std::cout << str << std::endl;
}

void CADSystem::logString(double)
{
}

void CADSystem::runOndselSinglePendulum()
{
	//Double pendulum with easy input numbers for exact port from Smalltalk
	//GEOAssembly calcCharacteristicDimensions must set mbdUnits to unity.
	std::cout << "runOndselSinglePendulum" << std::endl;
	auto& TheSystem = mbdSystem;
	TheSystem->clear();
	std::string name = "TheSystem";
	TheSystem->name = name;
	std::cout << "TheSystem->name " << TheSystem->name << std::endl;
	auto& systemSolver = TheSystem->systemSolver;
	systemSolver->errorTolPosKine = 1.0e-6;
	systemSolver->errorTolAccKine = 1.0e-6;
	systemSolver->iterMaxPosKine = 25;
	systemSolver->iterMaxAccKine = 25;
	systemSolver->tstart = 0.0;
	systemSolver->tend = 0.04;
	systemSolver->hmin = 1.0e-9;
	systemSolver->hmax = 1.0;
	systemSolver->hout = 0.04;
	systemSolver->corAbsTol = 1.0e-6;
	systemSolver->corRelTol = 1.0e-6;
	systemSolver->intAbsTol = 1.0e-6;
	systemSolver->intRelTol = 1.0e-6;
	systemSolver->iterMaxDyn = 4;
	systemSolver->orderMax = 5;
	systemSolver->translationLimit = 1.0e10;
	systemSolver->rotationLimit = 0.5;

	std::string str;
	FColDsptr qX, qE, qXdot, omeOpO, qXddot, alpOpO;
	FColDsptr rpmp;
	FMatDsptr aAap, aApm;
	FRowDsptr fullRow;
	//
	auto assembly1 = CREATE<Part>::With("/Assembly1");
	std::cout << "assembly1->name " << assembly1->name << std::endl;
	assembly1->m = 0.0;
	assembly1->aJ = std::make_shared<DiagonalMatrix<double>>(ListD{ 0, 0, 0 });
	qX = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	aAap = std::make_shared<FullMatrix<double>>(ListListD{
		{ 1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 }
		});
	assembly1->setqX(qX);
	assembly1->setaAap(aAap);
	qXdot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	omeOpO = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	assembly1->setqXdot(qXdot);
	assembly1->setomeOpO(omeOpO);
	TheSystem->addPart(assembly1);
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
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.0, 3.0, 0.0 });
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
	std::cout << "crankPart1->name " << crankPart1->name << std::endl;
	crankPart1->m = 1.0;
	crankPart1->aJ = std::make_shared<DiagonalMatrix<double>>(ListD{ 1, 1, 1 });
	qX = std::make_shared<FullColumn<double>>(ListD{ 0.4, 0.0, -0.05 });
	aAap = std::make_shared<FullMatrix<double>>(ListListD{
		{ 1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 }
		});
	crankPart1->setqX(qX);
	crankPart1->setaAap(aAap);
	qXdot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	omeOpO = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	crankPart1->setqXdot(qXdot);
	crankPart1->setomeOpO(omeOpO);
	TheSystem->addPart(crankPart1);
	{
		auto& partFrame = crankPart1->partFrame;
		auto marker1 = CREATE<MarkerFrame>::With("/Assembly1/Part1/Marker1");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ -0.4, 0.0, 0.05 });
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
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.4, 0.0, 0.05 });
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
	auto revJoint1 = CREATE<RevoluteJoint>::With("/Assembly1/Joint1");
	std::cout << "revJoint1->name " << revJoint1->name << std::endl;
	revJoint1->connectsItoJ(assembly1->partFrame->endFrame("/Assembly1/Marker2"), crankPart1->partFrame->endFrame("/Assembly1/Part1/Marker1"));
	TheSystem->addJoint(revJoint1);
	//
	auto rotMotion1 = CREATE<ZRotation>::With("/Assembly1/Motion1");
	rotMotion1->connectsItoJ(assembly1->partFrame->endFrame("/Assembly1/Marker2"), crankPart1->partFrame->endFrame("/Assembly1/Part1/Marker1"));
	std::cout << "rotMotion1->name " << rotMotion1->name << std::endl;
	rotMotion1->phiBlk = std::make_shared<Constant>(1.0);
	std::cout << "rotMotion1->phiBlk " << *(rotMotion1->phiBlk) << std::endl;
	TheSystem->addJoint(rotMotion1);
	//
	TheSystem->runKINEMATIC(TheSystem);
}

void CADSystem::runOndselDoublePendulum()
{
	//Double pendulum with easy input numbers for exact port from Smalltalk
	//GEOAssembly calcCharacteristicDimensions must set mbdUnits to unity.
	std::cout << "runOndselDoublePendulum" << std::endl;
	auto& TheSystem = mbdSystem;
	TheSystem->clear();
	std::string name = "TheSystem";
	TheSystem->name = name;
	std::cout << "TheSystem->name " << TheSystem->name << std::endl;
	auto& systemSolver = TheSystem->systemSolver;
	systemSolver->errorTolPosKine = 1.0e-6;
	systemSolver->errorTolAccKine = 1.0e-6;
	systemSolver->iterMaxPosKine = 25;
	systemSolver->iterMaxAccKine = 25;
	systemSolver->tstart = 0.0;
	systemSolver->tend = 0.04;
	systemSolver->hmin = 1.0e-9;
	systemSolver->hmax = 1.0;
	systemSolver->hout = 0.04;
	systemSolver->corAbsTol = 1.0e-6;
	systemSolver->corRelTol = 1.0e-6;
	systemSolver->intAbsTol = 1.0e-6;
	systemSolver->intRelTol = 1.0e-6;
	systemSolver->iterMaxDyn = 4;
	systemSolver->orderMax = 5;
	systemSolver->translationLimit = 1.0e10;
	systemSolver->rotationLimit = 0.5;

	std::string str;
	FColDsptr qX, qE, qXdot, omeOpO, qXddot, alpOpO;
	FColDsptr rpmp;
	FMatDsptr aAap, aApm;
	FRowDsptr fullRow;
	//
	auto assembly1 = CREATE<Part>::With("/Assembly1");
	std::cout << "assembly1->name " << assembly1->name << std::endl;
	assembly1->m = 0.0;
	assembly1->aJ = std::make_shared<DiagonalMatrix<double>>(ListD{ 0, 0, 0 });
	qX = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	aAap = std::make_shared<FullMatrix<double>>(ListListD{
		{ 1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 }
		});
	assembly1->setqX(qX);
	assembly1->setaAap(aAap);
	qXdot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	omeOpO = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	assembly1->setqXdot(qXdot);
	assembly1->setomeOpO(omeOpO);
	TheSystem->addPart(assembly1);
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
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.0, 3.0, 0.0 });
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
	std::cout << "crankPart1->name " << crankPart1->name << std::endl;
	crankPart1->m = 1.0;
	crankPart1->aJ = std::make_shared<DiagonalMatrix<double>>(ListD{ 1, 1, 1 });
	qX = std::make_shared<FullColumn<double>>(ListD{ 0.4, 0.0, -0.05 });
	aAap = std::make_shared<FullMatrix<double>>(ListListD{
		{ 1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 }
		});
	crankPart1->setqX(qX);
	crankPart1->setaAap(aAap);
	qXdot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	omeOpO = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	crankPart1->setqXdot(qXdot);
	crankPart1->setomeOpO(omeOpO);
	TheSystem->addPart(crankPart1);
	{
		auto& partFrame = crankPart1->partFrame;
		auto marker1 = CREATE<MarkerFrame>::With("/Assembly1/Part1/Marker1");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ -0.4, 0.0, 0.05 });
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
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.4, 0.0, 0.05 });
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
	std::cout << "conrodPart2->name " << conrodPart2->name << std::endl;
	conrodPart2->m = 1.0;
	conrodPart2->aJ = std::make_shared<DiagonalMatrix<double>>(ListD{ 1, 1, 1 });
	qX = std::make_shared<FullColumn<double>>(ListD{ 0.15, 0.1, 0.05 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0.0, 0.0, 1.0, 0.0 });
	auto eulerParameters = CREATE<EulerParameters<double>>::With(ListD{ 0.0, 0.0, 1.0, 0.0 });
	eulerParameters->calcABC();
	aAap = eulerParameters->aA;
	conrodPart2->setqX(qX);
	conrodPart2->setaAap(aAap);
	qXdot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	omeOpO = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	conrodPart2->setqXdot(qXdot);
	conrodPart2->setomeOpO(omeOpO);
	TheSystem->addPart(conrodPart2);
	{
		auto& partFrame = conrodPart2->partFrame;
		auto marker1 = CREATE<MarkerFrame>::With("/Assembly1/Part2/Marker1");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ -0.65, 0.0, -0.05 });
		marker1->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{1.0, 0.0, 0.0},
			{0.0, 1.0, 0.0},
			{0.0, 0.0, 1.0}
			});
		marker1->setaApm(aApm);
		partFrame->addMarkerFrame(marker1);
		//
		auto marker2 = CREATE<MarkerFrame>::With("/Assembly1/Part2/Marker2");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.65, 0.0, -0.05 });
		marker2->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{1.0, 0.0, 0.0},
			{0.0, 1.0, 0.0},
			{0.0, 0.0, 1.0}
			});
		marker2->setaApm(aApm);
		partFrame->addMarkerFrame(marker2);
	}
	//
	auto revJoint1 = CREATE<RevoluteJoint>::With("/Assembly1/Joint1");
	std::cout << "revJoint1->name " << revJoint1->name << std::endl;
	revJoint1->connectsItoJ(assembly1->partFrame->endFrame("/Assembly1/Marker2"), crankPart1->partFrame->endFrame("/Assembly1/Part1/Marker1"));
	TheSystem->addJoint(revJoint1);

	auto revJoint2 = CREATE<RevoluteJoint>::With("/Assembly1/Joint2");
	std::cout << "revJoint2->name " << revJoint2->name << std::endl;
	revJoint2->connectsItoJ(crankPart1->partFrame->endFrame("/Assembly1/Part1/Marker2"), conrodPart2->partFrame->endFrame("/Assembly1/Part2/Marker1"));
	TheSystem->addJoint(revJoint2);
	//
	TheSystem->runKINEMATIC(TheSystem);
}

void CADSystem::runOndselPiston()
{
	//Piston with easy input numbers for exact port from Smalltalk
	//GEOAssembly calcCharacteristicDimensions must set mbdUnits to unity.
	std::cout << "runOndselPiston" << std::endl;
	auto& TheSystem = mbdSystem;
	TheSystem->clear();
	std::string name = "TheSystem";
	TheSystem->name = name;
	std::cout << "TheSystem->name " << TheSystem->name << std::endl;
	auto& systemSolver = TheSystem->systemSolver;
	systemSolver->errorTolPosKine = 1.0e-6;
	systemSolver->errorTolAccKine = 1.0e-6;
	systemSolver->iterMaxPosKine = 25;
	systemSolver->iterMaxAccKine = 25;
	systemSolver->tstart = 0.0;
	systemSolver->tend = 1.0;
	systemSolver->hmin = 1.0e-9;
	systemSolver->hmax = 1.0;
	systemSolver->hout = 0.04;
	systemSolver->corAbsTol = 1.0e-6;
	systemSolver->corRelTol = 1.0e-6;
	systemSolver->intAbsTol = 1.0e-6;
	systemSolver->intRelTol = 1.0e-6;
	systemSolver->iterMaxDyn = 4;
	systemSolver->orderMax = 5;
	systemSolver->translationLimit = 1.0e10;
	systemSolver->rotationLimit = 0.5;

	std::string str;
	FColDsptr qX, qE, qXdot, omeOpO, qXddot, qEddot;
	FColDsptr rpmp;
	FMatDsptr aApm;
	FRowDsptr fullRow;
	//
	auto assembly1 = CREATE<Part>::With("/Assembly1");
	std::cout << "assembly1->name " << assembly1->name << std::endl;
	assembly1->m = 0.0;
	assembly1->aJ = std::make_shared<DiagonalMatrix<double>>(ListD{ 0, 0, 0 });
	qX = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0, 1 });
	assembly1->setqX(qX);
	assembly1->setqE(qE);
	qXdot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	omeOpO = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	assembly1->setqXdot(qXdot);
	assembly1->setomeOpO(omeOpO);
	qXddot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	qEddot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0, 0 });
	assembly1->setqXddot(qXddot);
	assembly1->setqEddot(qEddot);
	std::cout << "assembly1->getqX() " << *assembly1->getqX() << std::endl;
	std::cout << "assembly1->getqE() " << *assembly1->getqE() << std::endl;
	TheSystem->addPart(assembly1);
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
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.0, 3.0, 0.0 });
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
	std::cout << "crankPart1->name " << crankPart1->name << std::endl;
	crankPart1->m = 1.0;
	crankPart1->aJ = std::make_shared<DiagonalMatrix<double>>(ListD{ 1, 1, 1 });
	qX = std::make_shared<FullColumn<double>>(ListD{ 0.4, 0.0, -0.05 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0.0, 0.0, 0.0, 1.0 });
	crankPart1->setqX(qX);
	crankPart1->setqE(qE);
	qXdot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	omeOpO = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	crankPart1->setqXdot(qXdot);
	crankPart1->setomeOpO(omeOpO);
	qXddot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	qEddot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0, 0 });
	crankPart1->setqXddot(qXddot);
	crankPart1->setqEddot(qEddot);
	TheSystem->addPart(crankPart1);
	{
		auto& partFrame = crankPart1->partFrame;
		auto marker1 = CREATE<MarkerFrame>::With("/Assembly1/Part1/Marker1");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ -0.4, 0.0, 0.05 });
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
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.4, 0.0, 0.05 });
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
	std::cout << "conrodPart2->name " << conrodPart2->name << std::endl;
	conrodPart2->m = 1.0;
	conrodPart2->aJ = std::make_shared<DiagonalMatrix<double>>(ListD{ 1, 1, 1 });
	qX = std::make_shared<FullColumn<double>>(ListD{ 0.15, 0.1, 0.05 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0.0, 0.0, 1.0, 0.0 });
	conrodPart2->setqX(qX);
	conrodPart2->setqE(qE);
	qXdot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	omeOpO = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	conrodPart2->setqXdot(qXdot);
	conrodPart2->setomeOpO(omeOpO);
	qXddot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	qEddot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0, 0 });
	conrodPart2->setqXddot(qXddot);
	conrodPart2->setqEddot(qEddot);
	TheSystem->addPart(conrodPart2);
	{
		auto& partFrame = conrodPart2->partFrame;
		auto marker1 = CREATE<MarkerFrame>::With("/Assembly1/Part2/Marker1");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ -0.65, 0.0, -0.05 });
		marker1->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{1.0, 0.0, 0.0},
			{0.0, 1.0, 0.0},
			{0.0, 0.0, 1.0}
			});
		marker1->setaApm(aApm);
		partFrame->addMarkerFrame(marker1);
		//
		auto marker2 = CREATE<MarkerFrame>::With("/Assembly1/Part2/Marker2");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.65, 0.0, -0.05 });
		marker2->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{1.0, 0.0, 0.0},
			{0.0, 1.0, 0.0},
			{0.0, 0.0, 1.0}
			});
		marker2->setaApm(aApm);
		partFrame->addMarkerFrame(marker2);
	}
	//
	auto pistonPart3 = CREATE<Part>::With("/Assembly1/Part3");
	std::cout << "pistonPart3->name " << pistonPart3->name << std::endl;
	pistonPart3->m = 1.0;
	pistonPart3->aJ = std::make_shared<DiagonalMatrix<double>>(ListD{ 1, 1, 1 });
	qX = std::make_shared<FullColumn<double>>(ListD{ -0.0, 1.5, 0.0 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0.70710678118655, 0.70710678118655, 0.0, 0.0 });
	pistonPart3->setqX(qX);
	pistonPart3->setqE(qE);
	qXdot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	omeOpO = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	pistonPart3->setqXdot(qXdot);
	pistonPart3->setomeOpO(omeOpO);
	qXddot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	qEddot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0, 0 });
	pistonPart3->setqXddot(qXddot);
	pistonPart3->setqEddot(qEddot);
	TheSystem->addPart(pistonPart3);
	{
		auto& partFrame = pistonPart3->partFrame;
		auto marker1 = CREATE<MarkerFrame>::With("/Assembly1/Part3/Marker1");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ -0.5, 0.0, 0.0 });
		marker1->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{0.0, 1.0, 0.0},
			{1.0, 0.0, 0.0},
			{0.0, 0.0, -1.0}
			});
		marker1->setaApm(aApm);
		partFrame->addMarkerFrame(marker1);
		//
		auto marker2 = CREATE<MarkerFrame>::With("/Assembly1/Part3/Marker2");
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.5, 0.0, 0.0 });
		marker2->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{0.0, 0.0, 1.0},
			{1.0, 0.0, 0.0},
			{0.0, 1.0, 0.0}
			});
		marker2->setaApm(aApm);
		partFrame->addMarkerFrame(marker2);
	}
	//
	auto revJoint1 = CREATE<RevoluteJoint>::With("/Assembly1/Joint1");
	std::cout << "revJoint1->name " << revJoint1->name << std::endl;
	revJoint1->connectsItoJ(assembly1->partFrame->endFrame("/Assembly1/Marker2"), crankPart1->partFrame->endFrame("/Assembly1/Part1/Marker1"));
	TheSystem->addJoint(revJoint1);

	auto revJoint2 = CREATE<RevoluteJoint>::With("/Assembly1/Joint2");
	std::cout << "revJoint2->name " << revJoint2->name << std::endl;
	revJoint2->connectsItoJ(crankPart1->partFrame->endFrame("/Assembly1/Part1/Marker2"), conrodPart2->partFrame->endFrame("/Assembly1/Part2/Marker1"));
	TheSystem->addJoint(revJoint2);

	auto revJoint3 = CREATE<RevoluteJoint>::With("/Assembly1/Joint3");
	std::cout << "revJoint3->name " << revJoint3->name << std::endl;
	revJoint3->connectsItoJ(conrodPart2->partFrame->endFrame("/Assembly1/Part2/Marker2"), pistonPart3->partFrame->endFrame("/Assembly1/Part3/Marker1"));
	TheSystem->addJoint(revJoint3);

	auto cylJoint4 = CREATE<CylindricalJoint>::With("/Assembly1/Joint4");
	std::cout << "cylJoint4->name " << cylJoint4->name << std::endl;
	cylJoint4->connectsItoJ(pistonPart3->partFrame->endFrame("/Assembly1/Part3/Marker2"), assembly1->partFrame->endFrame("/Assembly1/Marker1"));
	TheSystem->addJoint(cylJoint4);

	auto rotMotion1 = CREATE<ZRotation>::With("/Assembly1/Motion1");
	rotMotion1->connectsItoJ(assembly1->partFrame->endFrame("/Assembly1/Marker2"), crankPart1->partFrame->endFrame("/Assembly1/Part1/Marker1"));
	std::cout << "rotMotion1->name " << rotMotion1->name << std::endl;
	auto omega = std::make_shared<Constant>(6.2831853071796);
	auto timeScale = std::make_shared<Constant>(1.0);
	auto time = std::make_shared<Product>(timeScale, TheSystem->time);
	rotMotion1->phiBlk = std::make_shared<Product>(omega, time);
	std::cout << "rotMotion1->phiBlk " << *(rotMotion1->phiBlk) << std::endl;
	TheSystem->addJoint(rotMotion1);
	//
	TheSystem->runKINEMATIC(TheSystem);
}

void CADSystem::runPiston()
{
	std::cout << "runPiston" << std::endl;
	auto& TheSystem = mbdSystem;
	TheSystem->clear();
	std::string name = "TheSystem";
	TheSystem->name = name;
	std::cout << "TheSystem->name " << TheSystem->name << std::endl;
	auto& systemSolver = TheSystem->systemSolver;
	systemSolver->errorTolPosKine = 1.0e-6;
	systemSolver->errorTolAccKine = 1.0e-6;
	systemSolver->iterMaxPosKine = 25;
	systemSolver->iterMaxAccKine = 25;
	systemSolver->tstart = 0.0;
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
	FColDsptr qX, qE, qXdot, omeOpO, qXddot, qEddot;
	FColDsptr rpmp;
	FMatDsptr aApm;
	FRowDsptr fullRow;
	//
	auto assembly1 = CREATE<Part>::With("/Assembly1");
	std::cout << "assembly1->name " << assembly1->name << std::endl;
	assembly1->m = 0.0;
	assembly1->aJ = std::make_shared<DiagonalMatrix<double>>(ListD{ 0, 0, 0 });
	qX = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0, 1 });
	assembly1->setqX(qX);
	assembly1->setqE(qE);
	qXdot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	omeOpO = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	assembly1->setqXdot(qXdot);
	assembly1->setomeOpO(omeOpO);
	qXddot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	qEddot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0, 0 });
	assembly1->setqXddot(qXddot);
	assembly1->setqEddot(qEddot);
	std::cout << "assembly1->getqX() " << *assembly1->getqX() << std::endl;
	std::cout << "assembly1->getqE() " << *assembly1->getqE() << std::endl;
	TheSystem->addPart(assembly1);
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
	std::cout << "crankPart1->name " << crankPart1->name << std::endl;
	crankPart1->m = 0.045210530089461;
	crankPart1->aJ = std::make_shared<DiagonalMatrix<double>>(ListD{ 1.7381980042084e-4, 0.003511159968501, 0.0036154518487535 });
	qX = std::make_shared<FullColumn<double>>(ListD{ 0.38423368514246, 2.6661567755108e-17, -0.048029210642807 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0.0, 0.0, 0.0, 1.0 });
	crankPart1->setqX(qX);
	crankPart1->setqE(qE);
	qXdot = std::make_shared<FullColumn<double>>(ListD{ 0, 0.096568457800423, 0 });
	omeOpO = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0.25132741228718 });
	crankPart1->setqXdot(qXdot);
	crankPart1->setomeOpO(omeOpO);
	qXddot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	qEddot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0, 0 });
	crankPart1->setqXddot(qXddot);
	crankPart1->setqEddot(qEddot);
	TheSystem->addPart(crankPart1);
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
	std::cout << "conrodPart2->name " << conrodPart2->name << std::endl;
	conrodPart2->m = 0.067815795134192;
	conrodPart2->aJ = std::make_shared<DiagonalMatrix<double>>(ListD{ 2.6072970063126e-4, 0.011784982468533, 0.011941420288912 });
	qX = std::make_shared<FullColumn<double>>(ListD{ 0.38423368514246, 0.49215295678475, 0.048029210642807 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0.0, 0.0, 0.89871703427292, 0.43852900965351 });
	conrodPart2->setqX(qX);
	conrodPart2->setqE(qE);
	qXdot = std::make_shared<FullColumn<double>>(ListD{ 0, 0.19313691560085, 0 });
	omeOpO = std::make_shared<FullColumn<double>>(ListD{ 1.670970041317e-34, 1.3045598281729e-34, -1.2731200314796e-35 });
	conrodPart2->setqXdot(qXdot);
	conrodPart2->setomeOpO(omeOpO);
	qXddot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	qEddot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0, 0 });
	conrodPart2->setqXddot(qXddot);
	conrodPart2->setqEddot(qEddot);
	TheSystem->addPart(conrodPart2);
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
	std::cout << "pistonPart3->name " << pistonPart3->name << std::endl;
	pistonPart3->m = 1.730132083368;
	pistonPart3->aJ = std::make_shared<DiagonalMatrix<double>>(ListD{ 0.19449049546716, 0.23028116340971, 0.23028116340971 });
	qX = std::make_shared<FullColumn<double>>(ListD{ -1.283972762056e-18, 1.4645980199976, -4.7652385308244e-17 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0.70710678118655, 0.70710678118655, 0.0, 0.0 });
	pistonPart3->setqX(qX);
	pistonPart3->setqE(qE);
	qXdot = std::make_shared<FullColumn<double>>(ListD{ -6.3364526821409e-32, 0.19313691560085, -1.933731897626e-34 });
	omeOpO = std::make_shared<FullColumn<double>>(ListD{ 1.670970041317e-34, 1.3045598281729e-34, 1.8896472173894e-50 });
	pistonPart3->setqXdot(qXdot);
	pistonPart3->setomeOpO(omeOpO);
	qXddot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	qEddot = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0, 0 });
	pistonPart3->setqXddot(qXddot);
	pistonPart3->setqEddot(qEddot);
	TheSystem->addPart(pistonPart3);
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
	std::cout << "revJoint1->name " << revJoint1->name << std::endl;
	revJoint1->connectsItoJ(assembly1->partFrame->endFrame("/Assembly1/Marker2"), crankPart1->partFrame->endFrame("/Assembly1/Part1/Marker1"));
	TheSystem->addJoint(revJoint1);

	auto revJoint2 = CREATE<RevoluteJoint>::With("/Assembly1/Joint2");
	std::cout << "revJoint2->name " << revJoint2->name << std::endl;
	revJoint2->connectsItoJ(crankPart1->partFrame->endFrame("/Assembly1/Part1/Marker2"), conrodPart2->partFrame->endFrame("/Assembly1/Part2/Marker1"));
	TheSystem->addJoint(revJoint2);

	auto revJoint3 = CREATE<RevoluteJoint>::With("/Assembly1/Joint3");
	std::cout << "revJoint3->name " << revJoint3->name << std::endl;
	revJoint3->connectsItoJ(conrodPart2->partFrame->endFrame("/Assembly1/Part2/Marker2"), pistonPart3->partFrame->endFrame("/Assembly1/Part3/Marker1"));
	TheSystem->addJoint(revJoint3);

	auto cylJoint4 = CREATE<CylindricalJoint>::With("/Assembly1/Joint4");
	std::cout << "cylJoint4->name " << cylJoint4->name << std::endl;
	cylJoint4->connectsItoJ(pistonPart3->partFrame->endFrame("/Assembly1/Part3/Marker2"), assembly1->partFrame->endFrame("/Assembly1/Marker1"));
	TheSystem->addJoint(cylJoint4);

	auto rotMotion1 = CREATE<ZRotation>::With("/Assembly1/Motion1");
	rotMotion1->connectsItoJ(assembly1->partFrame->endFrame("/Assembly1/Marker2"), crankPart1->partFrame->endFrame("/Assembly1/Part1/Marker1"));
	std::cout << "rotMotion1->name " << rotMotion1->name << std::endl;
	auto omega = std::make_shared<Constant>(6.2831853071796);
	auto timeScale = std::make_shared<Constant>(0.04);
	auto time = std::make_shared<Product>(timeScale, TheSystem->time);
	rotMotion1->phiBlk = std::make_shared<Product>(omega, time);
	std::cout << "rotMotion1->phiBlk " << *(rotMotion1->phiBlk) << std::endl;
	TheSystem->addJoint(rotMotion1);
	//
	TheSystem->runKINEMATIC(TheSystem);
}

void MbD::CADSystem::preMbDrun(std::shared_ptr<System>)
{
}

void CADSystem::postMbDrun()
{
}

void MbD::CADSystem::updateFromMbD()
{
}
