/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <string>

#include "Symbolic.h"
namespace MbD {
    class Variable : public Symbolic
    {
        //name value 
	public:
		Variable();
		Variable(const std::string& str);
		Variable(double val);
		void initialize() override;
		void setName(const std::string& str);
		const std::string& getName() const;
		double getValue() override;
		std::ostream& printOn(std::ostream& s) const override;
		void setValue(double val) override;
		Symsptr expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
		Symsptr simplifyUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
		bool isVariable() override;

		std::string name;
		double value;
	};
}

