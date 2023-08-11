/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <cassert>

#include "ASMTTime.h"
#include "Time.h"
#include "Constant.h"
#include "Product.h"

using namespace MbD;

void MbD::ASMTTime::deleteMbD()
{
	xx = nullptr;
	expression = nullptr;
}

void MbD::ASMTTime::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	auto mbdTime = mbdSys->time;
	if (xx == mbdTime) return;
	auto timeScale = std::make_shared<Constant>(mbdUnits->time);
	auto geoTime = std::make_shared<Product>(timeScale, mbdTime);
	this->xexpression(mbdTime, geoTime->simplified(geoTime));
}
