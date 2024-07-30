/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include<algorithm>

#include "PartFrame.h"
#include "MarkerFrame.h"
#include "EndFramec.h"
#include "EndFrameqc.h"
#include "EulerParameters.h"
#include "CREATE.h"

using namespace MbD;

MarkerFrame::MarkerFrame()
{
}

MarkerFrame::MarkerFrame(const std::string& str) : CartesianFrame(str) {
}

System* MarkerFrame::root()
{
	return partFrame->root();
}

void MarkerFrame::initialize()
{
	prOmOpE = std::make_shared<FullMatrix<double>>(3, 4);
	pAOmpE = std::make_shared<FullColumn<FMatDsptr>>(4);
	endFrames = std::make_shared<std::vector<EndFrmsptr>>();
	auto endFrm = CREATE<EndFrameqc>::With();
	this->addEndFrame(endFrm);
}

void MarkerFrame::initializeLocally()
{
	pprOmOpEpE = EulerParameters<double>::ppApEpEtimesColumn(rpmp);
	ppAOmpEpE = EulerParameters<double>::ppApEpEtimesMatrix(aApm);
	for (size_t i = 0; i < endFrames->size(); i++)
	{
		auto eFrmqc = std::dynamic_pointer_cast<EndFrameqc>(endFrames->at(i));
		if (eFrmqc) {
			if (eFrmqc->endFrameqct) {
				endFrames->at(i) = eFrmqc->endFrameqct;
			}
		}
	}
	endFramesDo([](EndFrmsptr endFrame) { endFrame->initializeLocally(); });
}

void MarkerFrame::initializeGlobally()
{
	endFramesDo([](EndFrmsptr endFrame) { endFrame->initializeGlobally(); });
}

void MarkerFrame::postInput()
{
	CartesianFrame::postInput();
	endFramesDo([](EndFrmsptr endFrame) { endFrame->postInput(); });
}

void MarkerFrame::calcPostDynCorrectorIteration()
{
	auto rOpO = partFrame->rOpO();
	auto aAOp = partFrame->aAOp();
	rOmO = rOpO->plusFullColumn(aAOp->timesFullColumn(rpmp));
	aAOm = aAOp->timesFullMatrix(aApm);
	auto pAOppE = partFrame->pAOppE();
	for (size_t i = 0; i < 4; i++)
	{
		auto& pAOppEi = pAOppE->at(i);
		prOmOpE->atijputFullColumn(0, i, pAOppEi->timesFullColumn(rpmp));
		pAOmpE->at(i) = pAOppEi->timesFullMatrix(aApm);
	}
}

void MarkerFrame::prePosIC()
{
	CartesianFrame::prePosIC();
	endFramesDo([](EndFrmsptr endFrame) { endFrame->prePosIC(); });
}

size_t MarkerFrame::iqX()
{
	return partFrame->iqX;
}

size_t MarkerFrame::iqE()
{
	return partFrame->iqE;
}

void MarkerFrame::endFramesDo(const std::function<void(EndFrmsptr)>& f)
{
	std::for_each(endFrames->begin(), endFrames->end(), f);
}

void MarkerFrame::fillqsu(FColDsptr col)
{
	endFramesDo([&](const EndFrmsptr& endFrame) { endFrame->fillqsu(col); });
}

void MarkerFrame::fillqsuWeights(DiagMatDsptr diagMat)
{
	endFramesDo([&](const EndFrmsptr& endFrame) { endFrame->fillqsuWeights(diagMat); });
}

void MbD::MarkerFrame::fillqsuddotlam(FColDsptr col)
{
	endFramesDo([&](const EndFrmsptr& endFrame) { endFrame->fillqsuddotlam(col); });
}

void MarkerFrame::fillqsulam(FColDsptr col)
{
	endFramesDo([&](const EndFrmsptr& endFrame) { endFrame->fillqsulam(col); });
}

void MarkerFrame::fillqsudot(FColDsptr col)
{
	endFramesDo([&](const EndFrmsptr& endFrame) { endFrame->fillqsudot(col); });
}

void MarkerFrame::fillqsudotWeights(DiagMatDsptr diagMat)
{
	endFramesDo([&](const EndFrmsptr& endFrame) { endFrame->fillqsudotWeights(diagMat); });
}

void MarkerFrame::setqsu(FColDsptr col)
{
	endFramesDo([&](const EndFrmsptr& endFrame) { endFrame->setqsu(col); });
}

void MarkerFrame::setqsulam(FColDsptr col)
{
	endFramesDo([&](const EndFrmsptr& endFrame) { endFrame->setqsulam(col); });
}

void MarkerFrame::setqsudot(FColDsptr col)
{
	endFramesDo([&](const EndFrmsptr& endFrame) { endFrame->setqsudot(col); });
}

void MarkerFrame::setqsudotlam(FColDsptr col)
{
	endFramesDo([&](const EndFrmsptr& endFrame) { endFrame->setqsudotlam(col); });
}

void MarkerFrame::postPosICIteration()
{
	CartesianFrame::postPosICIteration();
	endFramesDo([](EndFrmsptr endFrame) { endFrame->postPosICIteration(); });
}

void MarkerFrame::postPosIC()
{
	endFramesDo([](EndFrmsptr endFrame) { endFrame->postPosIC(); });
}

void MarkerFrame::preDyn()
{
	endFramesDo([](EndFrmsptr endFrame) { endFrame->preDyn(); });
}

void MarkerFrame::storeDynState()
{
	endFramesDo([](EndFrmsptr endFrame) { endFrame->storeDynState(); });
}

void MarkerFrame::preVelIC()
{
	CartesianFrame::preVelIC();
	endFramesDo([](EndFrmsptr endFrame) { endFrame->preVelIC(); });
}

void MarkerFrame::postVelIC()
{
	endFramesDo([](EndFrmsptr endFrame) { endFrame->postVelIC(); });
}

void MarkerFrame::preAccIC()
{
	CartesianFrame::preAccIC();
	endFramesDo([](EndFrmsptr endFrame) { endFrame->preAccIC(); });
}

FColDsptr MarkerFrame::qXdot()
{
	return partFrame->qXdot;
}

std::shared_ptr<EulerParametersDot<double>> MarkerFrame::qEdot()
{
	return partFrame->qEdot;
}

FColDsptr MarkerFrame::qXddot()
{
	return partFrame->qXddot;
}

FColDsptr MarkerFrame::qEddot()
{
	return partFrame->qEddot;
}

void MarkerFrame::setqsuddotlam(FColDsptr col)
{
	endFramesDo([&](const EndFrmsptr& endFrame) { endFrame->setqsuddotlam(col); });
}

FColFMatDsptr MarkerFrame::pAOppE()
{
	return partFrame->pAOppE();
}

FMatDsptr MarkerFrame::aBOp()
{
	return partFrame->aBOp();
}

void MarkerFrame::postDynStep()
{
	endFramesDo([](EndFrmsptr endFrame) { endFrame->postDynStep(); });
}

void MarkerFrame::setPartFrame(PartFrame* partFrm)
{
	partFrame = partFrm;
}

PartFrame* MarkerFrame::getPartFrame() {
	return partFrame;
}

void MarkerFrame::setrpmp(FColDsptr x)
{
	rpmp->copyFrom(x);
}

void MarkerFrame::setaApm(FMatDsptr mat)
{
	aApm->copyFrom(mat);
}
void MarkerFrame::addEndFrame(EndFrmsptr endFrm)
{
	endFrm->setMarkerFrame(this);
	endFrames->push_back(endFrm);
}
