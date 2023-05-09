#include<algorithm>

#include "PartFrame.h"
#include "MarkerFrame.h"
#include "EndFramec.h"
#include "EndFrameqc.h"
#include "EulerParameters.h"

using namespace MbD;

MarkerFrame::MarkerFrame()
{
	initialize();
}

MarkerFrame::MarkerFrame(const char* str) : CartesianFrame(str) {
	initialize();
}

void MarkerFrame::initialize()
{
	//prOmOpE: = StMFullMatrix new : 3 by : 4.
	//pAOmpE : = StMFullColumn new : 4.
	//endFrames : = OrderedCollection new
	prOmOpE = std::make_shared<FullMatrix<double>>(3, 4);
	pAOmpE = std::make_unique<FullColumn<FullMatrix<double>>>(4);
	endFrames = std::make_unique<std::vector<std::shared_ptr<EndFramec>>>();
	auto endFrm = std::make_shared<EndFrameqc>("EndFrame1");
	this->addEndFrame(endFrm);
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
	rpmp->copy(x);
}

void MarkerFrame::setaApm(FMatDsptr x)
{
	aApm->copy(x);
}
void MarkerFrame::addEndFrame(std::shared_ptr<EndFramec> endFrm)
{
	endFrm->setMarkerFrame(this);
	endFrames->push_back(endFrm);
}

void MarkerFrame::initializeLocally()
{
	pprOmOpEpE = EulerParameters::ppApEpEtimesColumn(rpmp);
	ppAOmpEpE = EulerParameters::ppApEpEtimesMatrix(aApm);
	std::for_each(endFrames->begin(), endFrames->end(), [](const auto& endFrame) { endFrame->initializeLocally(); });
}

void MarkerFrame::initializeGlobally()
{
}
