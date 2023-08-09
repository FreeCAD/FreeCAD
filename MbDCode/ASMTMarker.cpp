/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ASMTMarker.h"
#include "ASMTRefItem.h"
#include "ASMTPart.h"
#include "Part.h"
#include "PartFrame.h"
#include "MarkerFrame.h"

using namespace MbD;

void MbD::ASMTMarker::parseASMT(std::vector<std::string>& lines)
{
	readName(lines);
	readPosition3D(lines);
	readRotationMatrix(lines);
}

FColDsptr MbD::ASMTMarker::rpmp()
{
	//p is cm
	auto refItem = static_cast<ASMTRefItem*>(owner);
	auto& rPrefP = refItem->position3D;
	auto& aAPref = refItem->rotationMatrix;
	auto& rrefmref = position3D;
	auto rPmP = rPrefP->plusFullColumn(aAPref->timesFullColumn(rrefmref));
	auto& principalMassMarker = static_cast<ASMTPart*>(refItem->owner)->principalMassMarker;
	auto& rPcmP = principalMassMarker->position3D;
	auto& aAPcm = principalMassMarker->rotationMatrix;
	auto rpmp = aAPcm->transposeTimesFullColumn(rPmP->minusFullColumn(rPcmP));
	return rpmp;
}

FMatDsptr MbD::ASMTMarker::aApm()
{
	//p is cm
	auto refItem = static_cast<ASMTRefItem*>(owner);
	auto& aAPref = refItem->rotationMatrix;
	auto& aArefm = rotationMatrix;
	auto& principalMassMarker = static_cast<ASMTPart*>(refItem->owner)->principalMassMarker;
	auto& aAPcm = principalMassMarker->rotationMatrix;
	auto aApm = aAPcm->transposeTimesFullMatrix(aAPref->timesFullMatrix(aArefm));
	return aApm;
}

void MbD::ASMTMarker::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	auto mkr = CREATE<MarkerFrame>::With(name.c_str());
	auto prt = std::static_pointer_cast<Part>(part()->mbdObject);
	prt->partFrame->addMarkerFrame(mkr);

	mkr->rpmp = rpmp()->times(1.0 / mbdUnits->length);
	mkr->aApm = aApm();
	mbdObject = mkr->endFrames->at(0);
}
