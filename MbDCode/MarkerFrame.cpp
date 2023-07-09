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

MarkerFrame::MarkerFrame(const char* str) : CartesianFrame(str) {
}

System* MarkerFrame::root()
{
	return partFrame->root();
}

void MarkerFrame::initialize()
{
	prOmOpE = std::make_shared<FullMatrix<double>>(3, 4);
	pAOmpE = std::make_shared<FullColumn<std::shared_ptr<FullMatrix<double>>>>(4);
	endFrames = std::make_shared<std::vector<EndFrmcptr>>();
	auto endFrm = CREATE<EndFrameqc>::With();
	this->addEndFrame(endFrm);
}

void MarkerFrame::initializeLocally()
{
	pprOmOpEpE = EulerParameters<double>::ppApEpEtimesColumn(rpmp);
	ppAOmpEpE = EulerParameters<double>::ppApEpEtimesMatrix(aApm);
	for (int i = 0; i < endFrames->size(); i++)
	{
		auto eFrmqc = std::dynamic_pointer_cast<EndFrameqc>(endFrames->at(i));
		if (eFrmqc) {
			if (eFrmqc->endFrameqct) {
				endFrames->at(i) = eFrmqc->endFrameqct;
			}
		}
	}
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->initializeLocally(); });
}

void MarkerFrame::initializeGlobally()
{
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->initializeGlobally(); });
}

void MarkerFrame::postInput()
{
	Item::postInput();
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->postInput(); });
}

void MarkerFrame::calcPostDynCorrectorIteration()
{
	auto rOpO = partFrame->rOpO();
	auto aAOp = partFrame->aAOp();
	rOmO = rOpO->plusFullColumn(aAOp->timesFullColumn(rpmp));
	aAOm = aAOp->timesFullMatrix(aApm);
	auto pAOppE = partFrame->pAOppE();
	for (int i = 0; i < 4; i++)
	{
		auto& pAOppEi = pAOppE->at(i);
		prOmOpE->atijputFullColumn(0, i, pAOppEi->timesFullColumn(rpmp));
		pAOmpE->at(i) = pAOppEi->timesFullMatrix(aApm);
	}
}

void MarkerFrame::prePosIC()
{
	Item::prePosIC();
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->prePosIC(); });
}

int MarkerFrame::iqX()
{
	return partFrame->iqX;
}

int MarkerFrame::iqE()
{
	return partFrame->iqE;
}

void MarkerFrame::endFramesDo(const std::function<void(std::shared_ptr<EndFramec>)>& f)
{
	std::for_each(endFrames->begin(), endFrames->end(), f);
}

void MarkerFrame::fillqsu(FColDsptr col)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->fillqsu(col); });
}

void MarkerFrame::fillqsuWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->fillqsuWeights(diagMat); });
}

void MbD::MarkerFrame::fillqsuddotlam(FColDsptr col)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->fillqsuddotlam(col); });
}

void MarkerFrame::fillqsulam(FColDsptr col)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->fillqsulam(col); });
}

void MarkerFrame::fillqsudot(FColDsptr col)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->fillqsudot(col); });
}

void MarkerFrame::fillqsudotWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->fillqsudotWeights(diagMat); });
}

void MarkerFrame::setqsu(FColDsptr col)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->setqsu(col); });
}

void MarkerFrame::setqsulam(FColDsptr col)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->setqsulam(col); });
}

void MarkerFrame::setqsudot(FColDsptr col)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->setqsudot(col); });
}

void MarkerFrame::setqsudotlam(FColDsptr col)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->setqsudotlam(col); });
}

void MarkerFrame::postPosICIteration()
{
	Item::postPosICIteration();
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->postPosICIteration(); });
}

void MarkerFrame::postPosIC()
{
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->postPosIC(); });
}

void MarkerFrame::preDyn()
{
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->preDyn(); });
}

void MarkerFrame::storeDynState()
{
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->storeDynState(); });
}

void MarkerFrame::preVelIC()
{
	Item::preVelIC();
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->preVelIC(); });
}

void MarkerFrame::postVelIC()
{
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->postVelIC(); });
}

void MarkerFrame::preAccIC()
{
	Item::preAccIC();
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->preAccIC(); });
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
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->setqsuddotlam(col); });
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
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->postDynStep(); });
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

void MarkerFrame::setaApm(FMatDsptr x)
{
	aApm->copyFrom(x);
}
void MarkerFrame::addEndFrame(EndFrmcptr endFrm)
{
	endFrm->setMarkerFrame(this);
	endFrames->push_back(endFrm);
}
