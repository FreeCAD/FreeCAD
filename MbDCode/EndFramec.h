#pragma once

#include <memory>

#include "CartesianFrame.h"
#include "FullColumn.h"     //FColDsptr is defined
#include "FullMatrix.h"     //FMatDsptr is defined

namespace MbD {
    class MarkerFrame;
    //template <typename T>
    //class FullColumn;
    //template <typename T>
    //class FullMatrix;

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
        virtual void initEndFrameqct();
        void calcPostDynCorrectorIteration() override;

        MarkerFrame* markerFrame;
        FColDsptr rOeO = std::make_shared<FullColumn<double>>(3);
        FMatDsptr aAOe = std::make_shared<FullMatrix<double>>(3, 3);
    };
    using EndFrmcptr = std::shared_ptr<EndFramec>;
}

