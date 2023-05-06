#pragma once
#include <memory>
#include <vector>

#include "CartesianFrame.h"
#include "Part.h"
#include "MarkerFrame.h"
#include "EndFramec.h"
#include "EulerConstraint.h"
#include "AbsConstraint.h"
#include "FullColumn.h"

namespace MbD {
	class Part;
	class MarkerFrame;
	class EndFramec;

	class PartFrame : public CartesianFrame
	{
		//ToDo: part iqX iqE qX qE qXdot qEdot qXddot qEddot aGeu aGabs markerFrames 
	public:
		PartFrame();
		PartFrame(const char* str);
		void initialize();

		void setqX(FullColDptr x);
		FullColDptr getqX();
		void setqE(FullColDptr x);
		FullColDptr getqE();
		void setPart(Part* x);
		Part* getPart();
		void addMarkerFrame(std::shared_ptr<MarkerFrame> x);
		std::shared_ptr<EndFramec> endFrame(std::string name);

		Part* part;
		int iqX, iqE;	//Position index of frame variables qX and qE in system list of variables
		FullColDptr qX = std::make_shared<FullColumn<double>>(3);
		FullColDptr qE = std::make_shared<FullColumn<double>>(4);
		std::shared_ptr<EulerConstraint> aGeu;
		std::unique_ptr<std::vector<std::shared_ptr<AbsConstraint>>> aGabs;
		std::unique_ptr<std::vector<std::shared_ptr<MarkerFrame>>> markerFrames;
	};
}

