#pragma once
#include <memory>
#include <vector>

#include "CartesianFrame.h"
#include "Part.h"
#include "MarkerFrame.h"
#include "EulerConstraint.h"
#include "AbsConstraint.h"
#include "FullColumn.h"

namespace MbD {
	class Part;
	class MarkerFrame;

	class PartFrame : public CartesianFrame
	{
	public:
		PartFrame();
		void setqX(FullColumn<double>* x) {
			qX = x;
		}
		FullColumn<double>* getqX() {
			return qX;
		}
		void setPart(std::shared_ptr<Part> x) {
			part = x;
		}
		std::shared_ptr<Part> getPart() {
			return part.lock();
		}
		//part iqX iqE qX qE qXdot qEdot qXddot qEddot aGeu aGabs markerFrames 
		std::weak_ptr<Part> part;
		int iqX, iqE;	//Position index of frame variables qX and qE in system list of variables
		FullColumn<double>* qX;
		FullColumn<double>* qE;
		std::shared_ptr<EulerConstraint> aGeu;
		std::vector<std::shared_ptr<AbsConstraint>> aGabs;
		std::vector<std::shared_ptr<MarkerFrame>> markerFrames;
	};
}

