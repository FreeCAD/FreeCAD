#include "PartFrame.h"
#include "AbsConstraint.h"
#include "MarkerFrame.h"

namespace MbD {

	PartFrame::PartFrame()
	{
		aGabs = std::vector<std::shared_ptr<AbsConstraint>>();
		markerFrames = std::vector<std::shared_ptr<MarkerFrame>>();
	}
}