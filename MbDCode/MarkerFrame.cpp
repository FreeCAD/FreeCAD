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
	std::for_each(endFrames->begin(), endFrames->end(), [](const auto& endFrame) { endFrame->initializeLocally(); });
}

void MarkerFrame::initializeGlobally()
{
	std::for_each(endFrames->begin(), endFrames->end(), [](const auto& endFrame) { endFrame->initializeGlobally(); });
}

void MbD::MarkerFrame::postInput()
{
	Item::postInput();
	std::for_each(endFrames->begin(), endFrames->end(), [](const auto& endFrame) { endFrame->postInput(); });
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
	std::for_each(endFrames->begin(), endFrames->end(), [](const auto& endFrame) { endFrame->prePosIC(); });
}

int MbD::MarkerFrame::iqX()
{
	return partFrame->iqX;
}

int MbD::MarkerFrame::iqE()
{
	return partFrame->iqE;
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
