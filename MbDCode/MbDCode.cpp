#include <iostream>
#include "System.h"
#include "FullColumn.h"
#include "FullMatrix.h"
#include "Part.h"
#include "Joint.h"
#include "CylindricalJoint.h"
#include "RevoluteJoint.h"
#include "ZRotation.h"
#include "MbDCode.h"

using namespace MbD;

int main()
{
	std::cout << "Hello World!\n";
	System& TheSystem = System::getInstance();
	std::string str = "TheSystem";
	TheSystem.setName(str);
	std::cout << "TheSystem.getName() " << TheSystem.getName() << std::endl;
	FullColDptr qX, qE;
	FullColDptr rpmp;
	FullMatDptr aApm;
	FullRowDptr fullRow;
	auto row = std::make_shared<FullRow<double>>(ListD{ 0.0, 0.0, 0.0, 1.0 });
	fullRow = std::make_shared<FullRow<double>>(4);
	fullRow->copy(row);
	//
	auto assembly1 = std::make_shared<Part>();
	str = "Assembly1";
	assembly1->setName(str);
	std::cout << "assembly1->getName() " << assembly1->getName() << std::endl;
	qX = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0, 1 });
	assembly1->setqX(qX);
	assembly1->setqE(qE);
	std::cout << "assembly1->getqX() " << assembly1->getqX()->toString() << std::endl;
	std::cout << "assembly1->getqE() " << assembly1->getqE()->toString() << std::endl;
	TheSystem.addPart(assembly1);
	{
		auto marker1 = std::make_shared<MarkerFrame>();
		str = "Marker1";
		marker1->setName(str);
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.38423366582893, 6.8384291794733e-9, -0.048029210642807 });
		marker1->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{ 1, 0, 0 },
			{ 0, 0, 1 },
			{ 0, -1, 0 }
			});
		marker1->setaApm(aApm);
		assembly1->partFrame->addMarkerFrame(marker1);
		//
		auto marker2 = std::make_shared<MarkerFrame>();
		str = "Marker2";
		marker2->setName(str);
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.0, 0.0, 0.0 });
		marker2->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{ 1, 0, 0 },
			{ 0, 1, 0 },
			{ 0, 0, 1 }
			});
		marker2->setaApm(aApm);
		assembly1->partFrame->addMarkerFrame(marker2);
	}
	//
	auto part1 = std::make_shared<Part>();
	str = "Part1";
	part1->setName(str);
	qX = std::make_shared<FullColumn<double>>(ListD{ 0.38423366582893, 6.8384291794733e-9, -0.048029210642807 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0.0, 0.0, 1.4248456266393e-10, 1.0 });
	part1->setqX(qX);
	part1->setqE(qE);
	TheSystem.parts.push_back(part1);
	{
		auto marker1 = std::make_shared<MarkerFrame>();
		str = "Marker1";
		marker1->setName(str);
		rpmp = std::make_shared<FullColumn<double>>(ListD{ -0.38423368514246, -2.6661567755108e-17, 0.048029210642807 });
		marker1->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{ 1, 0, 0 },
			{ 0, 1, 0 },
			{ 0, 0, 1 }
			});
		marker1->setaApm(aApm);
		part1->partFrame->addMarkerFrame(marker1);
		//
		auto marker2 = std::make_shared<MarkerFrame>();
		str = "Marker2";
		marker2->setName(str);
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.38423368514246, -2.6661567755108e-17, 0.048029210642807 });
		marker2->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{ 1, 0, 0 },
			{ 0, 1, 0 },
			{ 0, 0, 1 }
			});
		marker2->setaApm(aApm);
		part1->partFrame->addMarkerFrame(marker2);
	}
	//
	auto part2 = std::make_shared<Part>();
	str = "Part2";
	part2->setName(str);
	qX = std::make_shared<FullColumn<double>>(ListD{ 0.38423366582893,  0.49215308269277,  0.048029210642807 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0.0, 0.0, 0.89871701272344, 0.4385290538168 });
	part2->setqX(qX);
	part2->setqE(qE);
	TheSystem.parts.push_back(part2);
	{
		auto marker1 = std::make_shared<MarkerFrame>();
		str = "Marker1";
		marker1->setName(str);
		rpmp = std::make_shared<FullColumn<double>>(ListD{ -0.6243797383565, 1.1997705489799e-16, -0.048029210642807 });
		marker1->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{1.0, 2.7755575615629e-16, 0.0},
			{-2.7755575615629e-16, 1.0, 0.0},
			{0.0, 0.0, 1.0} 
			});
		marker1->setaApm(aApm);
		part2->partFrame->addMarkerFrame(marker1);
		//
		auto marker2 = std::make_shared<MarkerFrame>();
		str = "Marker2";
		marker2->setName(str);
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.6243797383565, -2.1329254204087e-16, -0.048029210642807 });
		marker2->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{1.0, 2.4980018054066e-16, 2.2204460492503e-16},
			{-2.4980018054066e-16, 1.0, 4.1633363423443e-17},
			{-2.2204460492503e-16, -4.1633363423443e-17, 1.0} 
			});
		marker2->setaApm(aApm);
		part2->partFrame->addMarkerFrame(marker2);
	}
	//
	auto part3 = std::make_shared<Part>();
	str = "Part3";
	part3->setName(str);
	qX = std::make_shared<FullColumn<double>>(ListD{ -1.284772285311e-18, 1.4645982581368, -4.788228906425e-17 });
	qE = std::make_shared<FullColumn<double>>(ListD{ 0.70710678118655, 0.70710678118655, 0.0, 0.0 });
	part3->setqX(qX);
	part3->setqE(qE);
	TheSystem.parts.push_back(part3);
	{
		auto marker1 = std::make_shared<MarkerFrame>();
		str = "Marker1";
		marker1->setName(str);
		rpmp = std::make_shared<FullColumn<double>>(ListD{ -0.48029210642807, 7.6201599718927e-18, -2.816737703896e-17 });
		marker1->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{9.2444637330587e-33, 1.0, 2.2204460492503e-16},
			{1.0, -9.2444637330587e-33, -1.0785207688569e-32},
			{-1.0785207688569e-32, 2.2204460492503e-16, -1.0} 
			});
		marker1->setaApm(aApm);
		part3->partFrame->addMarkerFrame(marker1);
		//
		auto marker2 = std::make_shared<MarkerFrame>();
		str = "Marker2";
		marker2->setName(str);
		rpmp = std::make_shared<FullColumn<double>>(ListD{ 0.48029210642807, 1.7618247880058e-17, 2.5155758471256e-17 });
		marker2->setrpmp(rpmp);
		aApm = std::make_shared<FullMatrix<double>>(ListListD{
			{6.9388939039072e-18, -6.4146353042213e-50, 1.0},
			{1.0, -6.9388939039072e-18, 6.9388939039072e-18},
			{-6.9388939039072e-18, 1.0, -7.4837411882581e-50} 
			});
		marker2->setaApm(aApm);
		part3->partFrame->addMarkerFrame(marker2);
	}
	//
	auto cylJoint4 = std::shared_ptr<CylindricalJoint>();
	auto revJoint3 = std::shared_ptr<RevoluteJoint>();
	auto revJoint2 = std::shared_ptr<RevoluteJoint>();
	auto revJoint1 = std::shared_ptr<RevoluteJoint>();
	auto rotMotion1 = std::shared_ptr<ZRotation>();
}