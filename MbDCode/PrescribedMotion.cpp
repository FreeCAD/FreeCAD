#include <iostream>	
#include <memory>
#include <typeinfo>
#include <assert.h>

#include "PrescribedMotion.h"
#include "EndFrameqct.h"
#include "Constant.h"

using namespace MbD;

PrescribedMotion::PrescribedMotion() {
	initialize();
}

PrescribedMotion::PrescribedMotion(const char* str) : Joint(str) {
	initialize();
}

void PrescribedMotion::initialize()
{
	xBlk = std::make_shared<Constant>(0.0);
	yBlk = std::make_shared<Constant>(0.0);
	zBlk = std::make_shared<Constant>(0.0);
	phiBlk = std::make_shared<Constant>(0.0);
	theBlk = std::make_shared<Constant>(0.0);
	psiBlk = std::make_shared<Constant>(0.0);
}

void PrescribedMotion::connectsItoJ(std::shared_ptr<EndFramec> frmi, std::shared_ptr<EndFramec> frmj)
{
	Joint::connectsItoJ(frmi, frmj);
	frmI->EndFrameqctFrom(frmI);
}

//void PrescribedMotion::connectsItoJ(std::shared_ptr<EndFramec> frmi, std::shared_ptr<EndFramec> frmj)
//{
//	Joint::connectsItoJ(frmi, frmj);
//	frmI->EndFrameqctFrom(frmI);
//	decltype(frmi) dddd;
//	std::cout << "typeid(dddd).name() " << typeid(dddd).name() << std::endl;
//	std::cout << "typeid(frmI).name() " << typeid(frmI).name() << std::endl;
//	if (typeid(frmI).name() != "EndFrameqct") {
//		std::shared_ptr<EndFramec> newFrmI;
//		newFrmI = std::make_shared<EndFrameqct>(frmI->getName().c_str());
//		decltype(newFrmI) ffff;
//		std::cout << "typeid(ffff).name() " << typeid(ffff).name() << std::endl;
//		auto gggg = std::make_shared<EndFrameqct>(frmI->getName().c_str());
//		std::cout << "typeid(gggg).name() " << typeid(gggg).name() << std::endl;
//		std::cout << "typeid(newFrmI).name() " << typeid(newFrmI).name() << std::endl;
//		std::swap(frmI, newFrmI);
//		assert(typeid(frmI).name() != "EndFrameqct");
//	}
//}
