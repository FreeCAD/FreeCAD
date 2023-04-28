#include <iostream>
#include "System.h"
#include "FullColumn.h"
#include "FullMatrix.h"
#include "Part.h"
#include "MbDCode.h"

using namespace MbD;

int main()
{
	std::cout << "Hello World!\n";
	System& TheSystem = System::getInstance();
	std::string str = "TheSystem";
	TheSystem.setName(str);
	std::cout << "TheSystem.getName() " << TheSystem.getName() << std::endl;
	FullColumn<double>* qX, * qE;
	FullColumn<double>* rpmp;
	FullMatrix<double>* aApm;
	FullRow<double>* fullRow;
	auto row = new FullRow<double>{ 0, 0, 0, 1 };
	fullRow = new FullRow<double>(4);
	fullRow->copy(row);
	//
	auto assembly1 = std::make_shared<Part>();
	str = "Assembly1";
	assembly1->setName(str);
	std::cout << "assembly1->getName() " << assembly1->getName() << std::endl;
	qX = new FullColumn<double>{ 0, 0, 0 };
	qE = new FullColumn<double>{ 0, 0, 0, 1 };
	assembly1->setqX(qX);
	assembly1->setqE(qE);
	std::cout << "assembly1->getqX() " << assembly1->getqX()->toString() << std::endl;
	std::cout << "assembly1->getqE() " << assembly1->getqE()->toString() << std::endl;
	TheSystem.addPart(assembly1);
	{
		auto marker1 = std::make_shared<MarkerFrame>();
		str = "Marker1";
		marker1->setName(str);
		rpmp = new FullColumn<double>{ 0.38423366582893, 6.8384291794733e-9, -0.048029210642807 };
		marker1->setrpmp(rpmp);
		aApm = new FullMatrix<double>();
		fullRow = new FullRow<double>{ 0.38423366582893, 6.8384291794733e-9, -0.048029210642807 };
		aApm->push_back(fullRow);
		fullRow = new FullRow<double>{ 0.38423366582893, 6.8384291794733e-9, -0.048029210642807 };
		aApm->push_back(fullRow);
		fullRow = new FullRow<double>{ 0.38423366582893, 6.8384291794733e-9, -0.048029210642807 };
		aApm->push_back(fullRow);
		marker1->setaApm(aApm);
		assembly1->partFrame->addMarkerFrame(marker1);
		//
		auto marker2 = std::make_shared<MarkerFrame>();
		str = "Marker2";
		marker2->setName(str);
		rpmp = new FullColumn<double>{ 0.0, 0.0, 0.0 };
		marker2->setrpmp(rpmp);
		aApm = new FullMatrix<double>();
		fullRow = new FullRow<double>{ 1.0, 0.0, 0.0 };
		aApm->push_back(fullRow);
		fullRow = new FullRow<double>{ 0.0, 1.0, 0.0 };
		aApm->push_back(fullRow);
		fullRow = new FullRow<double>{ 0.0, 0.0, 1.0 };
		aApm->push_back(fullRow);
		marker2->setaApm(aApm);
		assembly1->partFrame->addMarkerFrame(marker2);
	}
	//
	auto part1 = std::make_shared<Part>();
	str = "Part1";
	part1->setName(str);
	qX = new FullColumn<double>{ 0.38423366582893, 6.8384291794733e-9, -0.048029210642807 };
	qE = new FullColumn<double>{ 0.0, 0.0, 1.4248456266393e-10, 1.0 };
	part1->setqX(qX);
	part1->setqE(qE);
	TheSystem.parts.push_back(part1);
	//
	auto part2 = std::make_shared<Part>();
	str = "Part2";
	part2->setName(str);
	qX = new FullColumn<double>{ 0.38423366582893,  0.49215308269277,  0.048029210642807 };
	qE = new FullColumn<double>{ 0.0, 0.0, 0.89871701272344, 0.4385290538168 };
	part2->setqX(qX);
	part2->setqE(qE);
	TheSystem.parts.push_back(part2);
	//
	auto part3 = std::make_shared<Part>();
	str = "Part3";
	part3->setName(str);
	qX = new FullColumn<double>{ -1.284772285311e-18, 1.4645982581368, -4.788228906425e-17 };
	qE = new FullColumn<double>{ 0.70710678118655, 0.70710678118655, 0.0, 0.0 };
	part3->setqX(qX);
	part3->setqE(qE);
	TheSystem.parts.push_back(part3);
	//
}