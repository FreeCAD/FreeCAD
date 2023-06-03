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
	for (size_t i = 0; i < endFrames->size(); i++)
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
	auto rOmO = rOpO->plusFullColumn(aAOp->timesFullColumn(rpmp));
	auto aAOm = aAOp->timesFullMatrix(aApm);
	auto pAOppE = partFrame->pAOppE();
	for (size_t i = 0; i < 4; i++)
	{
		auto& pAOppEi = pAOppE->at(i);
		prOmOpE->atijputFullColumn(1, i, pAOppEi->timesFullColumn(rpmp));
		pAOmpE->at(i) = pAOppEi->timesFullMatrix(aApm);
	}
}

void MbD::MarkerFrame::prePosIC()
{
	Item::prePosIC();
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->prePosIC(); });
}

size_t MbD::MarkerFrame::iqX()
{
	return partFrame->iqX;
}

size_t MbD::MarkerFrame::iqE()
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

void MbD::MarkerFrame::setqsulam(FColDsptr col)
{
	endFramesDo([&](const std::shared_ptr<EndFramec>& endFrame) { endFrame->setqsulam(col); });
}

void MbD::MarkerFrame::postPosICIteration()
{
	endFramesDo([](std::shared_ptr<EndFramec> endFrame) { endFrame->postPosICIteration(); });
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
