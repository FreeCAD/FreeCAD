#include <memory>

#include "EndFrameqc.h"
#include "EndFrameqct.h"
#include "Variable.h"
#include "MarkerFrame.h"

using namespace MbD;

EndFrameqc::EndFrameqc() {
	initialize();
}

EndFrameqc::EndFrameqc(const char* str) : EndFramec(str) {
	initialize();
}

void EndFrameqc::initialize()
{
	prOeOpE = std::make_shared<FullMatrix<double>>(3, 4);
	pprOeOpEpE = std::make_shared<FullMatrix<std::shared_ptr<FullColumn<double>>>>(4, 4);
	pAOepE = std::make_shared<FullColumn<std::shared_ptr<FullMatrix<double>>>>(4);
	ppAOepEpE = std::make_shared<FullMatrix<std::shared_ptr<FullMatrix<double>>>>(4, 4);
}

void EndFrameqc::initializeLocally()
{
	if (endFrameqct) {
		endFrameqct->initializeLocally();
	}
}

void EndFrameqc::initializeGlobally()
{
	if (endFrameqct) {
		endFrameqct->initializeGlobally();
	}
	else {
		pprOeOpEpE = markerFrame->pprOmOpEpE;
		ppAOepEpE = markerFrame->ppAOmpEpE;
	}
}

void MbD::EndFrameqc::EndFrameqctFrom(EndFrmcptr& frm)
{
	endFrameqct = std::make_shared<EndFrameqct>();
}

void MbD::EndFrameqc::setrmemBlks(std::shared_ptr<FullColumn<std::shared_ptr<Symbolic>>> xyzBlks)
{
	std::static_pointer_cast<EndFrameqct>(endFrameqct)->rmemBlks = xyzBlks;
}

void MbD::EndFrameqc::setphiThePsiBlks(std::shared_ptr<FullColumn<std::shared_ptr<Symbolic>>> xyzRotBlks)
{
	std::static_pointer_cast<EndFrameqct>(endFrameqct)->phiThePsiBlks = xyzRotBlks;
}
