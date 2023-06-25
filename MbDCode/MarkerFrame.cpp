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

void MbD::MarkerFrame::postInput()
{
	Item::postInput();
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->postInput(); });
}

void MbD::MarkerFrame::calcPostDynCorrectorIteration()
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

void MbD::MarkerFrame::prePosIC()
{
	Item::prePosIC();
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->prePosIC(); });
}

int MbD::MarkerFrame::iqX()
{
	return partFrame->iqX;
}

int MbD::MarkerFrame::iqE()
{
	return partFrame->iqE;
}

void MbD::MarkerFrame::endFramesDo(const std::function<void(std::shared_ptr<EndFramec>)>& f)
{
	std::for_each(endFrames->begin(), endFrames->end(), f);
}

void MbD::MarkerFrame::fillqsu(FColDsptr col)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->fillqsu(col); });
}

void MbD::MarkerFrame::fillqsuWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->fillqsuWeights(diagMat); });
}

void MbD::MarkerFrame::fillqsulam(FColDsptr col)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->fillqsulam(col); });
}

void MbD::MarkerFrame::fillqsudot(FColDsptr col)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->fillqsudot(col); });
}

void MbD::MarkerFrame::fillqsudotWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->fillqsudotWeights(diagMat); });
}

void MbD::MarkerFrame::setqsu(FColDsptr col)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->setqsu(col); });
}

void MbD::MarkerFrame::setqsulam(FColDsptr col)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->setqsulam(col); });
}

void MbD::MarkerFrame::setqsudotlam(FColDsptr col)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->setqsudotlam(col); });
}

void MbD::MarkerFrame::postPosICIteration()
{
	Item::postPosICIteration();
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->postPosICIteration(); });
}

void MbD::MarkerFrame::postPosIC()
{
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->postPosIC(); });
}

void MbD::MarkerFrame::preDyn()
{
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->preDyn(); });
}

void MbD::MarkerFrame::storeDynState()
{
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->storeDynState(); });
}

void MbD::MarkerFrame::preVelIC()
{
	Item::preVelIC();
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->preVelIC(); });
}

void MbD::MarkerFrame::postVelIC()
{
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->postVelIC(); });
}

void MbD::MarkerFrame::preAccIC()
{
	Item::preAccIC();
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->preAccIC(); });
}

FColDsptr MbD::MarkerFrame::qXdot()
{
	return partFrame->qXdot;
}

std::shared_ptr<EulerParametersDot<double>> MbD::MarkerFrame::qEdot()
{
	return partFrame->qEdot;
}

FColDsptr MbD::MarkerFrame::qXddot()
{
	return partFrame->qXddot;
}

FColDsptr MbD::MarkerFrame::qEddot()
{
	return partFrame->qEddot;
}

void MbD::MarkerFrame::setqsuddotlam(FColDsptr qsudotlam)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->setqsuddotlam(qsudotlam); });
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
