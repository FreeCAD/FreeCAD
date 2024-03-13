// AreaOrderer.h

/*==============================
Copyright (c) 2011-2015 Dan Heeks

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
==============================*/


#pragma once
#include <memory>
#include <list>
#include <set>

class CArea;
class CCurve;

class CAreaOrderer;

class CInnerCurves: public std::enable_shared_from_this<CInnerCurves>
{
    std::shared_ptr<CInnerCurves> m_pOuter;
    std::shared_ptr<CCurve> m_curve; // always empty if top level
	std::set<std::shared_ptr<CInnerCurves> > m_inner_curves;
    std::shared_ptr<CArea> m_unite_area; // new curves made by uniting are stored here

public:
	static CAreaOrderer* area_orderer;
	CInnerCurves(std::shared_ptr<CInnerCurves> pOuter, std::shared_ptr<CCurve> curve);
	CInnerCurves(){}
	~CInnerCurves();

	void Insert(std::shared_ptr<CCurve> pcurve);
	void GetArea(CArea &area, bool outside = true, bool use_curve = true);
	void Unite(std::shared_ptr<CInnerCurves> c);
};

class CAreaOrderer
{
public:
    std::shared_ptr<CInnerCurves> m_top_level;

	CAreaOrderer();

	void Insert(std::shared_ptr<CCurve> pcurve);
	CArea ResultArea()const;
};
