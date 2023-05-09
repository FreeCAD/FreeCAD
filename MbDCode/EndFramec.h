#pragma once
#include "CartesianFrame.h"
#include "MarkerFrame.h"
#include "FullColumn.h"
#include "FullMatrix.h"

namespace MbD {
    class MarkerFrame;

    class EndFramec : public CartesianFrame
    {
        //markerFrame rOeO aAOe 
    public:
        EndFramec();
        EndFramec(const char* str);
        void initialize();
        void setMarkerFrame(MarkerFrame* markerFrm);
        MarkerFrame* getMarkerFrame();
        void initializeLocally() override;
        void initializeGlobally() override;
        virtual void EndFrameqctFrom(std::shared_ptr<EndFramec>& frm);

        MarkerFrame* markerFrame;
        FColDsptr rOeO = std::make_shared<FullColumn<double>>(3);
        FMatDsptr aAOe = std::make_shared<FullMatrix<double>>(3, 3);
    };
}

