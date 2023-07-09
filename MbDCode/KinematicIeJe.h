#pragma once

#include "Item.h"
#include "EndFramec.h"

namespace MbD {
    class KinematicIeJe : public Item
    {
        //frmI frmJ 
    public:
        KinematicIeJe();
        KinematicIeJe(EndFrmcptr frmi, EndFrmcptr frmj);

        bool isKineIJ() override;
        virtual FRowDsptr pvaluepXI();
        virtual FRowDsptr pvaluepEI();
        virtual FMatDsptr ppvaluepXIpXI();
        virtual FMatDsptr ppvaluepXIpEI();
        virtual FMatDsptr ppvaluepEIpEI();
        virtual FRowDsptr pvaluepXJ();
        virtual FRowDsptr pvaluepEJ();
        virtual FMatDsptr ppvaluepXIpXJ();
        virtual FMatDsptr ppvaluepXIpEJ();
        virtual FMatDsptr ppvaluepEIpXJ();
        virtual FMatDsptr ppvaluepEIpEJ();
        virtual FMatDsptr ppvaluepXJpXJ();
        virtual FMatDsptr ppvaluepXJpEJ();
        virtual FMatDsptr ppvaluepEJpEJ();
        virtual FRowDsptr pvaluepXK();
        virtual FRowDsptr pvaluepEK();
        virtual FMatDsptr ppvaluepXIpEK();
        virtual FMatDsptr ppvaluepEIpEK();
        virtual FMatDsptr ppvaluepXJpEK();
        virtual FMatDsptr ppvaluepEJpEK();
        virtual FMatDsptr ppvaluepEKpEK();
        virtual double pvaluept();
        virtual double ppvalueptpt();
        virtual FRowDsptr ppvaluepXIpt();
        virtual FRowDsptr ppvaluepEIpt();
        virtual FRowDsptr ppvaluepXJpt();
        virtual FRowDsptr ppvaluepEJpt();
        virtual FRowDsptr ppvaluepXKpt();
        virtual FRowDsptr ppvaluepEKpt();
        virtual double value();

        EndFrmcptr frmI, frmJ;
    };
}

