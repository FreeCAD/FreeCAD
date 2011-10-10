//----------------------------------------------------------------------
// File:			pr_queue.h
// Programmer:		Sunil Arya and David Mount
// Description:		Include file for priority queue and related
// 					structures.
// Last modified:	01/04/05 (Version 1.0)
//----------------------------------------------------------------------
// Copyright (c) 1997-2005 University of Maryland and Sunil Arya and
// David Mount.  All Rights Reserved.
// 
// This software and related documentation is part of the Approximate
// Nearest Neighbor Library (ANN).  This software is provided under
// the provisions of the Lesser GNU Public License (LGPL).  See the
// file ../ReadMe.txt for further information.
// 
// The University of Maryland (U.M.) and the authors make no
// representations about the suitability or fitness of this software for
// any purpose.  It is provided "as is" without express or implied
// warranty.
//----------------------------------------------------------------------
// History:
//	Revision 0.1  03/04/98
//		Initial release
//----------------------------------------------------------------------

#ifndef PR_QUEUE_H
#define PR_QUEUE_H

#include <ANN/ANNx.h>					// all ANN includes
#include <ANN/ANNperf.h>				// performance evaluation

//----------------------------------------------------------------------
//	Basic types.
//----------------------------------------------------------------------
typedef void			*PQinfo;		// info field is generic pointer
typedef ANNdist			PQkey;			// key field is distance

//----------------------------------------------------------------------
//	Priority queue
//		A priority queue is a list of items, along with associated
//		priorities.  The basic operations are insert and extract_minimum.
//
//		The priority queue is maintained using a standard binary heap.
//		(Implementation note: Indexing is performed from [1..max] rather
//		than the C standard of [0..max-1].  This simplifies parent/child
//		computations.)  User information consists of a void pointer,
//		and the user is responsible for casting this quantity into whatever
//		useful form is desired.
//
//		Because the priority queue is so central to the efficiency of
//		query processing, all the code is inline.
//----------------------------------------------------------------------

class ANNpr_queue {

	struct pq_node {					// node in priority queue
		PQkey			key;			// key value
		PQinfo			info;			// info field
	};
	int			n;						// number of items in queue
	int			max_size;				// maximum queue size
	pq_node		*pq;					// the priority queue (array of nodes)

public:
	ANNpr_queue(int max)				// constructor (given max size)
		{
			n = 0;						// initially empty
			max_size = max;				// maximum number of items
			pq = new pq_node[max+1];	// queue is array [1..max] of nodes
		}

	~ANNpr_queue()						// destructor
		{ delete [] pq; }

	ANNbool empty()						// is queue empty?
		{ if (n==0) return ANNtrue; else return ANNfalse; }

	ANNbool non_empty()					// is queue nonempty?
		{ if (n==0) return ANNfalse; else return ANNtrue; }

	void reset()						// make existing queue empty
		{ n = 0; }

	inline void insert(					// insert item (inlined for speed)
		PQkey kv,						// key value
		PQinfo inf)						// item info
		{
			if (++n > max_size) annError("Priority queue overflow.", ANNabort);
			register int r = n;
			while (r > 1) {				// sift up new item
				register int p = r/2;
				ANN_FLOP(1)				// increment floating ops
				if (pq[p].key <= kv)	// in proper order
					break;
				pq[r] = pq[p];			// else swap with parent
				r = p;
			}
			pq[r].key = kv;				// insert new item at final location
			pq[r].info = inf;
		}

	inline void extr_min(				// extract minimum (inlined for speed)
		PQkey &kv,						// key (returned)
		PQinfo &inf)					// item info (returned)
		{
			kv = pq[1].key;				// key of min item
			inf = pq[1].info;			// information of min item
			register PQkey kn = pq[n--].key;// last item in queue
			register int p = 1;			// p points to item out of position
			register int r = p<<1;		// left child of p
			while (r <= n) {			// while r is still within the heap
				ANN_FLOP(2)				// increment floating ops
										// set r to smaller child of p
				if (r < n  && pq[r].key > pq[r+1].key) r++;
				if (kn <= pq[r].key)	// in proper order
					break;
				pq[p] = pq[r];			// else swap with child
				p = r;					// advance pointers
				r = p<<1;
			}
			pq[p] = pq[n+1];			// insert last item in proper place
		}
};

#endif
