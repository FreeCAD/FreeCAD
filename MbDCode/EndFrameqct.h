#pragma once
#include "EndFrameqc.h"

namespace MbD {
    class EndFrameqct : public EndFrameqc
    {
        //time rmemBlks prmemptBlks pprmemptptBlks phiThePsiBlks pPhiThePsiptBlks ppPhiThePsiptptBlks 
        //rmem prmempt pprmemptpt aAme pAmept ppAmeptpt prOeOpt pprOeOpEpt pprOeOptpt pAOept ppAOepEpt ppAOeptpt 
    public:
        double time;
        FullColDptr rmem, prmempt, pprmemptpt;
        FullMatDptr aAme, pAmept, ppAmeptpt;
    };
}

