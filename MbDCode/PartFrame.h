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
		void initializeLocally() override;
		void initializeGlobally() override;
		void asFixed();

		void setqX(FColDsptr x);
		FColDsptr getqX();
		void setqE(FColDsptr x);
		FColDsptr getqE();
		void setPart(Part* x);
		Part* getPart();
		void addMarkerFrame(std::shared_ptr<MarkerFrame> x);
		std::shared_ptr<EndFramec> endFrame(std::string name);

		Part* part;
		int iqX, iqE;	//Position index of frame variables qX and qE in system list of variables
		FColDsptr qX = std::make_shared<FullColumn<double>>(3);
		FColDsptr qE = std::make_shared<FullColumn<double>>(4);
		FColDsptr qXdot = std::make_shared<FullColumn<double>>(3);
		FColDsptr qEdot = std::make_shared<FullColumn<double>>(4);
		FColDsptr qXddot = std::make_shared<FullColumn<double>>(3);
		FColDsptr qEddot = std::make_shared<FullColumn<double>>(4);
		std::shared_ptr<EulerConstraint> aGeu;
		std::unique_ptr<std::vector<std::shared_ptr<AbsConstraint>>> aGabs;
		std::unique_ptr<std::vector<std::shared_ptr<MarkerFrame>>> markerFrames;
	};
}

