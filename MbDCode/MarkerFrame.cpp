#include "PartFrame.h"
#include "MarkerFrame.h"
#include "EndFramec.h"
#include "EndFrameqc.h"

using namespace MbD;

MarkerFrame::MarkerFrame()
{
	initialize();
}

MbD::MarkerFrame::MarkerFrame(const char* str) : CartesianFrame(str) {
	initialize();
}

void MbD::MarkerFrame::initialize()
{
	partFrame = nullptr;
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

void MarkerFrame::setrpmp(FullColDptr x)
{
	rpmp->copy(x);
}

void MarkerFrame::setaApm(FullMatDptr x)
{
	aApm->copy(x);
}
void MarkerFrame::addEndFrame(std::shared_ptr<EndFramec> endFrm)
{
	endFrm->setMarkerFrame(this);
	endFrames->push_back(endFrm);
}
