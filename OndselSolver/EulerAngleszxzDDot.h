/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "EulerArray.h"
#include "EulerAngleszxzDot.h"

namespace MbD {

    template<typename T>
    class EulerAngleszxzDDot : public EulerArray<T>
    {
        //phiThePsiDot phiAddot theAddot psiAddot aAddot 
    public:
        EulerAngleszxzDDot() : EulerArray<T>(3) {}
        void initialize() override;
        void calc() override;

        std::shared_ptr<EulerAngleszxzDot<double>> phiThePsiDot;
        FMatDsptr phiAddot, theAddot, psiAddot, aAddot;
    };
    template<typename T>
    inline void EulerAngleszxzDDot<T>::initialize()
    {
        phiAddot = std::make_shared<FullMatrix<double>>(3, 3);
        phiAddot->zeroSelf();
        theAddot = std::make_shared<FullMatrix<double>>(3, 3);
        theAddot->zeroSelf();
        psiAddot = std::make_shared<FullMatrix<double>>(3, 3);
        psiAddot->zeroSelf();
    }
    template<typename T>
    inline void EulerAngleszxzDDot<T>::calc()
    {
		//| zero phiThePsi phi sphi cphi phidot phiddot cphiddot sphiddot the sthe cthe thedot theddot ctheddot stheddot 
		// psi spsi cpsi psidot psiddot cpsiddot spsiddot phiA theA psiA phiAdot theAdot psiAdot |
		double zero = 0.0;
		auto& phiThePsi = phiThePsiDot->phiThePsi;
		auto& phi = phiThePsi->at(0);
		auto sphi = std::sin(phi);
		auto cphi = std::cos(phi);
		auto& phidot = phiThePsiDot->at(0);
		auto& phiddot = this->at(0);
		auto cphiddot = zero - (sphi * phiddot) - (cphi * phidot * phidot);
		auto sphiddot = cphi * phiddot - (sphi * phidot * phidot);
		auto& the = phiThePsi->at(1);
		auto sthe = std::sin(the);
		auto cthe = std::cos(the);
		auto& thedot = phiThePsiDot->at(1);
		auto& theddot = this->at(1);
		auto ctheddot = zero - (sthe * theddot) - (cthe * thedot * thedot);
		auto stheddot = cthe * theddot - (sthe * thedot * thedot);
		auto& psi = phiThePsi->at(2);
		auto spsi = std::sin(psi);
		auto cpsi = std::cos(psi);
		auto& psidot = phiThePsiDot->at(2);
		auto& psiddot = this->at(2);
		auto cpsiddot = zero - (spsi * psiddot) - (cpsi * psidot * psidot);
		auto spsiddot = cpsi * psiddot - (spsi * psidot * psidot);
		phiAddot->at(0)->atiput(0, cphiddot);
		phiAddot->at(0)->atiput(1, zero - sphiddot);
		phiAddot->at(1)->atiput(0, sphiddot);
		phiAddot->at(1)->atiput(1, cphiddot);
		theAddot->at(1)->atiput(1, ctheddot);
		theAddot->at(1)->atiput(2, zero - stheddot);
		theAddot->at(2)->atiput(1, stheddot);
		theAddot->at(2)->atiput(2, ctheddot);
		psiAddot->at(0)->atiput(0, cpsiddot);
		psiAddot->at(0)->atiput(1, zero - spsiddot);
		psiAddot->at(1)->atiput(0, spsiddot);
		psiAddot->at(1)->atiput(1, cpsiddot);
		auto& phiA = phiThePsi->phiA;
		auto& theA = phiThePsi->theA;
		auto& psiA = phiThePsi->psiA;
		auto& phiAdot = phiThePsiDot->phiAdot;
		auto& theAdot = phiThePsiDot->theAdot;
		auto& psiAdot = phiThePsiDot->psiAdot;
		auto mat = *(phiAddot->timesFullMatrix(theA->timesFullMatrix(psiA)))
			+ *(phiAdot->timesFullMatrix(theAdot->timesFullMatrix(psiA)))
			+ *(phiAdot->timesFullMatrix(theA->timesFullMatrix(psiAdot)))
			+ *(phiAdot->timesFullMatrix(theAdot->timesFullMatrix(psiA)))
			+ *(phiA->timesFullMatrix(theAddot->timesFullMatrix(psiA)))
			+ *(phiA->timesFullMatrix(theAdot->timesFullMatrix(psiAdot)))
			+ *(phiAdot->timesFullMatrix(theA->timesFullMatrix(psiAdot)))
			+ *(phiA->timesFullMatrix(theAdot->timesFullMatrix(psiAdot)))
			+ *(phiA->timesFullMatrix(theA->timesFullMatrix(psiAddot)));
		aAddot = std::make_shared<FullMatrix<double>>(mat);
	}
}

