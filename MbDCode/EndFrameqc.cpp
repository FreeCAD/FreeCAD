#include <memory>

#include "EndFrameqc.h"
#include "EndFrameqct.h"

using namespace MbD;

EndFrameqc::EndFrameqc() {
	initialize();
}

EndFrameqc::EndFrameqc(const char* str) : EndFramec(str) {
	initialize();
}

void EndFrameqc::initialize()
{
    EndFramec::initialize();
    prOeOpE = std::make_unique<FullMatrix<double>>(3, 4);
    pprOeOpEpE = std::make_unique<FullMatrix<std::shared_ptr<FullColumn<double>>>>(4, 4);
    pAOepE = std::make_unique<FullColumn<std::shared_ptr<FullMatrix<double>>>>(4);
    ppAOepEpE = std::make_unique<FullMatrix<std::shared_ptr<FullMatrix<double>>>>(4, 4);
}

void EndFrameqc::initializeLocally()
{
}

void EndFrameqc::initializeGlobally()
{
}

void MbD::EndFrameqc::EndFrameqctFrom(std::shared_ptr<EndFramec>& frm)
{
	std::shared_ptr<EndFramec> newFrm;
	newFrm = std::make_shared<EndFrameqct>(frm->getName().c_str());
	newFrm->setMarkerFrame(frm->getMarkerFrame());
	//frm.swap(newFrm);
	std::swap(*(frm.get()), *(newFrm.get()));
}
