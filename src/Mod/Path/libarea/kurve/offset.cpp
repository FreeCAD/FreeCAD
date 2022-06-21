////////////////////////////////////////////////////////////////////////////////////////////////
//                    2d geometry classes - implements 2d kurve offset for use in dll
//
//                    g.j.hawkesford August 2003
//
// This program is released under the BSD license. See the file COPYING for details.
//
////////////////////////////////////////////////////////////////////////////////////////////////
#include "geometry.h"
using namespace geoff_geometry;

namespace geoff_geometry {
	static Kurve eliminateLoops(const Kurve& k , const Kurve& originalk, double offset, int& ret);
	static bool DoesIntersInterfere(const Point& pInt, const Kurve& k, double offset);

	int Kurve::Offset(vector<Kurve*>&OffsetKurves, double offset, int direction, int method, int& ret)const {

		switch(method) {
	case NO_ELIMINATION:
	case BASIC_OFFSET:
		{
			Kurve* ko = new Kurve;
			int n = OffsetMethod1(*ko, offset, direction, method, ret);
			OffsetKurves.push_back(ko);
			return n;
		}

	default:
		FAILURE(L"Requested Offsetting Method not available");
		}
		return 0;
	}

	int Kurve::OffsetMethod1(Kurve& kOffset, double off, int direction,  int method, int& ret)const
	{
		// offset kurve with simple span elimination
		// direction 1 = left,  -1 = right

		// ret  = 0		- kurve offset ok
		//		= 1		- kurve has differential scale (not allowed)
		//		= 2		- offset failed
		//      = 3		- offset too large
		if(this == &kOffset) FAILURE(L"Illegal Call - 'this' must not be kOffset");
		double offset = (direction == GEOFF_LEFT)?off : -off;

		if(fabs(offset) < geoff_geometry::TOLERANCE || m_nVertices < 2) {
			kOffset = *this;
			ret = 0;
			return 1;
		}

		Span curSpan, curSpanOff;	// current & offset spans
		Span prevSpanOff;			// previous offset span
		Point p0, p1;				// Offset span intersections

		// offset Kurve
		kOffset = Matrix(*this);

		if(m_mirrored) offset = -offset;
		int RollDir = ( off < 0 ) ? direction : - direction;				// Roll arc direction

		double scalex;
		if(!GetScale(scalex)) {
			ret = 1;
			return 0;	// differential scale
		}
		offset /= scalex;

		bool bClosed = Closed();
		int nspans = nSpans();
		if(bClosed) {
			Get(nspans, curSpan, true);						// assign previous span for closed

			prevSpanOff = curSpan.Offset(offset);
			nspans++; // read first again
		}

		for(int spannumber = 1; spannumber <= nspans; spannumber++) {
			if(spannumber > nSpans())
				Get(1, curSpan, true);						// closed kurve - read first span again
			else
				Get(spannumber, curSpan, true);

			if(!curSpan.NullSpan) {
				int numint = 0;
				curSpanOff = curSpan.Offset(offset);
				curSpanOff.ID = 0;
				if(!kOffset.m_started) {
					kOffset.Start(curSpanOff.p0);
					kOffset.AddSpanID(0);
				}

				if(spannumber > 1) {
					// see if tangent
					double d = curSpanOff.p0.Dist(prevSpanOff.p1);
					if((d > geoff_geometry::TOLERANCE) && (!curSpanOff.NullSpan && !prevSpanOff.NullSpan)) {
						// see if offset spans intersect

						double cp = prevSpanOff.ve ^ curSpanOff.vs;
						bool inters = (cp > 0 && direction == GEOFF_LEFT) || (cp < 0 && direction == GEOFF_RIGHT);

						if(inters) {
							double t[4];
							numint = prevSpanOff.Intof(curSpanOff, p0, p1, t);
						}

						if(numint == 1) {
							// intersection - modify previous endpoint
							kOffset.Replace(kOffset.m_nVertices-1, prevSpanOff.dir, p0, prevSpanOff.pc, prevSpanOff.ID);
						}
						else {
							// 0 or 2 intersections, add roll around (remove -ve loops in elimination function)
							if(kOffset.Add(RollDir, curSpanOff.p0, curSpan.p0, false))	kOffset.AddSpanID(ROLL_AROUND);
						}
					}
				}

				// add span
				if(spannumber < m_nVertices) {
					curSpanOff.ID = spannumber;
					kOffset.Add(curSpanOff, false);
				}
				else if(numint == 1)		// or replace the closed first span
					kOffset.Replace(0, 0, p0, Point(0, 0), 0);

			}
			if(!curSpanOff.NullSpan)prevSpanOff = curSpanOff;
		}		// end of main pre-offsetting loop


#ifdef _DEBUG
//testDraw->AddKurve("", &kOffset, 0, GREEN);
//		outXML oxml(L"c:\\temp\\eliminateLoops.xml");
//		oxml.startElement(L"eliminateLoops");
//		oxml.Write(kOffset, L"kOffset");
//		oxml.endElement();
#endif
		// eliminate loops
		if(method == NO_ELIMINATION) {
			ret = 0;
			return 1;
		}
		kOffset = eliminateLoops(kOffset, *this, offset, ret);

		if(ret == 0 && bClosed) {
			// check for inverted offsets of closed kurves
			if(kOffset.Closed()) {
				double a = Area();
				int dir = (a < 0);
				double ao = kOffset.Area();
				int dirOffset = ao < 0;

				if(dir != dirOffset)
					ret = 3;
				else {
					// check area change compatible with offset direction - catastrophic failure
					bool bigger = (a > 0 && offset > 0) || (a < 0 && offset < 0);
					if(bigger && fabs(ao) < fabs(a)) ret = 2;
				}
			}
			else
				ret = 2;			// started closed but now open??
		}
		return (ret == 0)?1 : 0;
	}


	static Kurve eliminateLoops(const Kurve& k , const Kurve& originalk, double offset, int& ret) {
		// a simple loop elimination routine based on first offset ideas in Peps
		// this needs extensive work for future
		// start point mustn't disappear & only one valid offset is determined
		//
		// ret = 0 for ok
		// ret = 2 for impossible geometry
		
		Span sp0, sp1;
		Point pInt, pIntOther;

		Kurve ko;											// eliminated output
		ko = Matrix(k);
		int kinVertex = 0;

		while(kinVertex <= k.nSpans()) {
			bool clipped = false ;                                       // not in a clipped section (assumption with this simple method)

			sp0.dir = k.Get(kinVertex, sp0.p0, sp0.pc);
			sp0.ID = k.GetSpanID(kinVertex++);
			if (kinVertex == 1)	{
				ko.Start(sp0.p0);							// start point mustn't disappear for this simple method
				ko.AddSpanID(sp0.ID);
			}
			if (kinVertex <= k.nSpans()) {   // any more?
				int ksaveVertex = kinVertex ;
				sp0.dir = k.Get(kinVertex, sp0.p1, sp0.pc);	// first span
				sp0.ID = k.GetSpanID(kinVertex++);

				sp0.SetProperties(true);

				int ksaveVertex1 = kinVertex;									// mark position AA		
				if (kinVertex <= k.nSpans()) {	// get the next but one span			
					sp1.dir = k.Get(kinVertex, sp1.p0, sp1.pc);
					sp1.ID = k.GetSpanID(kinVertex++);
					int ksaveVertex2 = kinVertex;								// mark position BB

					int fwdCount = 0;
					while(kinVertex <= k.nSpans()) {					
						sp1.dir = k.Get(kinVertex, sp1.p1, sp1.pc);			// check span
						sp1.ID = k.GetSpanID(kinVertex++);
						sp1.SetProperties(true);
			
						double t[4];
						int numint = sp0.Intof(sp1, pInt, pIntOther, t);			// find span intersections
						if(numint && sp0.p0.Dist(pInt) < geoff_geometry::TOLERANCE ) numint=0;	// check that intersection is not at the start of the check span					
						if(numint ) {

							if(numint == 2) {
								// choose first intercept on sp0
								Span spd = sp0;
								spd.p1 = pInt;
								spd.SetProperties(true);
								double dd = spd.length;

								spd.p1 = pIntOther;
								spd.SetProperties(true);
								if(dd > spd.length) pInt = pIntOther;
								numint = 1;

							}
							ksaveVertex = ksaveVertex1 ;

							clipped = true ;			// in a clipped section		
							if(!DoesIntersInterfere(pInt, originalk, offset)) {
								sp0.p1 = pInt;			// ok so truncate this span to the intersection
								clipped = false;		// end of clipped section
								break;
							}
							// no valid intersection found so carry on
						}
						sp1.p0 = sp1.p1 ;		// next
						ksaveVertex1 = ksaveVertex2 ;							// pos AA = BB
						ksaveVertex2 = kinVertex;								// mark 

						if((kinVertex > k.nSpans() || fwdCount++ > 25) && !clipped)
							break;
					}
				}

				if(clipped) {
					ret = 2;	// still in a clipped section - error

					return ko;
				}

				ko.Add(sp0, false);

				kinVertex = ksaveVertex;
			}
		}
		ret = 0;

		return ko; // no more spans - seems ok
	}


	static bool DoesIntersInterfere(const Point& pInt, const Kurve& k, double offset)  {
		// check that intersections don't interfere with the original kurve 
		Span sp;
		Point dummy;
		int kCheckVertex = 0;
		k.Get(kCheckVertex++, sp.p0, sp.pc);

		offset = fabs(offset) - geoff_geometry::TOLERANCE;
		while(kCheckVertex <= k.nSpans()) {
			sp.dir = k.Get(kCheckVertex++, sp.p1, sp.pc);
			sp.SetProperties(true);
			// check for interference 
			if(Dist(sp, pInt, dummy) < offset)
			    return true;
			sp.p0 = sp.p1;
		}
		return false;	// intersection is ok
	}
}


static struct iso {
		 Span sp;
		 Span off;
	} isodata;
static void isoRadius(Span& before, Span& blend, Span& after, double radius);

int Kurve::OffsetISOMethod(Kurve& kOut, double off, int direction, bool BlendAll)const {
		// produces a special offset Kurve - observing so-called ISO radii
		// eg line/arc/line tangent - keep arc radius constant
		// this method also considers arc/arc/arc etc.
		// interior radius must be smallest of triplet for above.

		// parameters:-
		// Output	kOut		resulting kurve
		// Input	off			offset amount
		// Input	direction	offset direction (LEFT or RIGHT)
		// Input	BlendAall	if false only consider ISO radius for LINE/ARC/LINE
		//						if true consider all blended radii (ARC/ARC/ARC etc.)		
		double offset = (direction == GEOFF_LEFT)?off : -off;
		if(FEQZ(off) || nSpans() < 1) {
			kOut = *this;
			return 1;
		}
		double cptol = 1.0e-05;
		std::vector<iso> spans;
		for(int i = 0; i < nSpans(); i++) {	// store all spans and offsets
			Get(i+1, isodata.sp, true, true);
			isodata.off = isodata.sp.Offset(offset);
			spans.push_back(isodata);
		}

		for(int i = 0; i < nSpans() - 1; i++)		// calculate intersections for none tangent spans
			if(fabs(spans[i].off.ve ^ spans[i+1].off.vs) > cptol)	spans[i].off.JoinSeparateSpans(spans[i+1].off);

		for(int i = 1; i < nSpans() - 1; i++) {			// deal with isoradii
			if(spans[i].off.dir) {
				if(BlendAll) {						// interior radius should be smaller than neighbours
					if(spans[i-1].sp.dir)
						if(spans[i-1].sp.radius < spans[i].sp.radius) continue;
					if(spans[i+1].sp.dir)
						if(spans[i+1].sp.radius < spans[i].sp.radius) continue;
				}
				else {
					if((spans[i-1].off.dir || spans[i+1].off.dir)) continue;		// linear neighbours only
				}

				if((fabs(spans[i-1].sp.ve ^ spans[i].sp.vs) < cptol) && (fabs(spans[i].sp.ve ^ spans[i+1].sp.vs) < cptol)) {
					// isoradius - calculate the new offset radius and modify neighbouring spans
					isoRadius(spans[i-1].off, spans[i].off, spans[i+1].off, spans[i].sp.radius);
				}
			}
		}

		kOut.Start(spans[0].off.p0);										// start point
		for(int i = 0; i < nSpans(); i++)
			kOut.Add(spans[i].off.dir, spans[i].off.p1, spans[i].off.pc);	// output all spans
		return 1;
	}

static void isoRadius(Span& before, Span& blend, Span& after, double radius) {
	// calculate the new offset radius and modify neighbouring spans
	int direction = ((before.ve ^ after.vs) > 0)? 1 : -1;				// offset direction
	Span beforeOff = before.Offset(direction * radius);
	Span afterOff = after.Offset(direction * radius);
	int turnLeft = ((before.ve ^ after.vs) > 0)? 1 : -1;
	if(before.dir == LINEAR) {
		CLine b(beforeOff);
		if(after.dir == LINEAR) {
			CLine a(afterOff);
			blend.pc = b.Intof(a);
		}
		else {
			Circle a(afterOff);
			b.Intof(turnLeft * after.dir, a, blend.pc);
		}
	}
	else {
		Circle b(beforeOff);

		if(after.dir == LINEAR) {
			CLine a(afterOff);
			a.Intof(-turnLeft * before.dir, b, blend.pc);
		}
		else {
			// arc arc
			Circle a(afterOff);
			int leftright = ((Vector2d(b.pc, blend.pc) ^ Vector2d(b.pc, a.pc)) < 0)? 1 : -1;
			b.Intof(leftright, a, blend.pc);
		}
	}
	before.p1 = blend.p0 = before.Near(blend.pc);
	after.p0 = blend.p1 = after.Near(blend.pc);
}
