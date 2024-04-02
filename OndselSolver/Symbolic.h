/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <iostream>
#include <unordered_set>
#include <memory>
#include <ostream>

#include "MbDMath.h"
#include "System.h"
#include "Units.h"
//#include "Constant.h"

namespace MbD {
	class Constant;
	class Symbolic;
	using Symsptr = std::shared_ptr<Symbolic>;

	class Symbolic : public MbDMath
	{
	public:
		Symbolic();
		virtual ~Symbolic() {}
		static Symsptr times(Symsptr arg, Symsptr arg1);
		static Symsptr sum(Symsptr arg, Symsptr arg1);
		static Symsptr raisedTo(Symsptr x, Symsptr y);

		virtual void initialize();
		virtual Symsptr differentiateWRT(Symsptr var);
		virtual Symsptr integrateWRT(Symsptr var);
		virtual Symsptr simplified();
		virtual Symsptr simplified(Symsptr sptr);
		virtual Symsptr expandUntil(std::shared_ptr<std::unordered_set<Symsptr>> set);
		virtual Symsptr expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set);
		virtual Symsptr simplifyUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set);
		virtual bool isZero();
		virtual bool isOne();
		virtual bool isSum();
		virtual bool isProduct();
		virtual bool isConstant();
		virtual std::shared_ptr<std::vector<Symsptr>> getTerms();
		virtual void addTerm(Symsptr trm);
		virtual double getValue();
		virtual double getValue(double arg);
		virtual void setValue(double val);
		virtual void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits);
		virtual Symsptr clonesptr();
		std::shared_ptr<Constant> sptrConstant(double value);
		virtual bool isVariable();
		virtual void setIntegrationConstant(double integConstant);

		virtual std::ostream& printOn(std::ostream& s) const;
		friend std::ostream& operator<<(std::ostream& s, const Symbolic& sym)
		{
			return sym.printOn(s);
		}
	};
}

