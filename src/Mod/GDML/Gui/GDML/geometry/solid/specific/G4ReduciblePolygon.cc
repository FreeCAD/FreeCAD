//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
//
// $Id: G4ReduciblePolygon.cc,v 1.11 2006/06/29 18:48:53 gunter Exp $
// GEANT4 tag $Name: geant4-09-03-patch-01 $
//
// 
// --------------------------------------------------------------------
// GEANT 4 class source file
//
//
// G4ReduciblePolygon.cc
//
// Implementation of a utility class used to specify, test, reduce,
// and/or otherwise manipulate a 2D polygon.
//
// See G4ReduciblePolygon.hh for more info.
//
// --------------------------------------------------------------------

#include "G4ReduciblePolygon.hh"
///#include "globals.hh"
#include "../../../global/globals.hh"

//
// Constructor: with simple arrays
//
G4ReduciblePolygon::G4ReduciblePolygon( const G4double a[],
                                        const G4double b[],
                                              G4int n )
  : aMin(0.), aMax(0.), bMin(0.), bMax(0.),
    vertexHead(0)
{
  //
  // Do all of the real work in Create
  //
  Create( a, b, n );
}


//
// Constructor: special PGON/PCON case
//
G4ReduciblePolygon::G4ReduciblePolygon( const G4double rmin[],
                                        const G4double rmax[], 
                                        const G4double z[], G4int n )
  : aMin(0.), aMax(0.), bMin(0.), bMax(0.),
    vertexHead(0)
{
  //
  // Translate
  //
  G4double *a = new G4double[n*2];
  G4double *b = new G4double[n*2];
  
  G4double *rOut = a + n,
           *zOut = b + n,
            *rIn = rOut-1,
            *zIn = zOut-1;
  
  G4int i;   
  for( i=0; i < n; i++, rOut++, zOut++, rIn--, zIn-- )
  {
    *rOut = rmax[i];
    *rIn  = rmin[i];
    *zOut = *zIn = z[i];
  }
  
  Create( a, b, n*2 );
  
  delete [] a;
  delete [] b;
}


//
// Create
//
// To be called by constructors, fill in the list and statistics for a new
// polygon
//
void G4ReduciblePolygon::Create( const G4double a[],
                                 const G4double b[], G4int n )
{
/*  if (n<3)
   G4Exception("G4ReduciblePolygon::Create()", "WrongArgumentValue",
               FatalException, "Less than 3 vertices specified.");
  */
  const G4double *anext = a, *bnext = b;
  ABVertex *prev = 0;
  do
  {
    ABVertex *newVertex = new ABVertex;
    newVertex->a = *anext;
    newVertex->b = *bnext;
    newVertex->next = 0;
    if (prev==0)
    {
      vertexHead = newVertex;
    }
    else
    {
      prev->next = newVertex;
    }
      
    prev = newVertex;
  } while( ++anext, ++bnext < b+n );

  numVertices = n;
  
  CalculateMaxMin();
}


//
// Fake default constructor - sets only member data and allocates memory
//                            for usage restricted to object persistency.
//
G4ReduciblePolygon::G4ReduciblePolygon( __void__& )
  : aMin(0.), aMax(0.), bMin(0.), bMax(0.), vertexHead(0)
{
}


//
// Destructor
//
G4ReduciblePolygon::~G4ReduciblePolygon()
{
  ABVertex *curr = vertexHead;
  while( curr )
  {
    ABVertex *toDelete = curr;
    curr = curr->next;
    delete toDelete;
  }
}


//
// CopyVertices
//
// Copy contents into simple linear arrays.
// ***** CAUTION ***** Be care to declare the arrays to a large
// enough size!
//
void G4ReduciblePolygon::CopyVertices( G4double a[], G4double b[] ) const
{
  G4double *anext = a, *bnext = b;
  ABVertex *curr = vertexHead;
  while( curr )
  {
    *anext++ = curr->a;
    *bnext++ = curr->b;
    curr = curr->next;
  }
}


//
// ScaleA
//
// Multiply all a values by a common scale
//
void G4ReduciblePolygon::ScaleA( G4double scale )
{
  ABVertex *curr = vertexHead;
  while( curr )
  {
    curr->a *= scale;
    curr = curr->next;
  }
}  


//
// ScaleB
//
// Multiply all b values by a common scale
//
void G4ReduciblePolygon::ScaleB( G4double scale )
{
  ABVertex *curr = vertexHead;
  while( curr )
  {
    curr->b *= scale;
    curr = curr->next;
  }
}  


//
// RemoveDuplicateVertices
//
// Remove adjacent vertices that are equal. Returns "false" if there
// is a problem (too few vertices remaining).
//
G4bool G4ReduciblePolygon::RemoveDuplicateVertices( G4double tolerance )
{
  ABVertex *curr = vertexHead, 
           *prev = 0,
           *next = curr->next;  // A little dangerous
  while( curr )
  {
    next = curr->next;
    if (next == 0) next = vertexHead;
    
    if (std::fabs(curr->a-next->a) < tolerance &&
        std::fabs(curr->b-next->b) < tolerance     )
    {
      //
      // Duplicate found: do we have > 3 vertices?
      //
      if (numVertices <= 3)
      {
        CalculateMaxMin();
        return false;
      }
      
      //
      // Delete
      //
      ABVertex *toDelete = curr;
      curr = curr->next;
      delete toDelete;
      
      numVertices--;
      
      if (prev) prev->next = curr; else vertexHead = curr;
    }
    else
    {
      prev = curr;
      curr = curr->next;
    }
  }
  
  //
  // In principle, this is not needed, but why not just play it safe?
  //
  CalculateMaxMin();
  
  return true;
}


//
// RemoveRedundantVertices
//
// Remove any unneeded vertices, i.e. those vertices which
// are on the line connecting the previous and next vertices.
//
G4bool G4ReduciblePolygon::RemoveRedundantVertices( G4double tolerance )
{
  //
  // Under these circumstances, we can quit now!
  //
  if (numVertices <= 2) return false;
  
  G4double tolerance2 = tolerance*tolerance;

  //
  // Loop over all vertices
  //
  ABVertex *curr = vertexHead, 
           *next = curr->next;  // A little dangerous
  while( curr )
  {
    next = curr->next;
    if (next == 0) next = vertexHead;
    
    G4double da = next->a - curr->a,
             db = next->b - curr->b;
    
    //
    // Loop over all subsequent vertices, up to curr
    //
    for(;;)
    {
      //
      // Get vertex after next
      //
      ABVertex *test = next->next;
      if (test == 0) test = vertexHead;
      
      //
      // If we are back to the original vertex, stop
      //
      if (test==curr) break;
    
      //
      // Test for parallel line segments
      //
      G4double dat = test->a - curr->a,
               dbt = test->b - curr->b;
         
      if (std::fabs(dat*db-dbt*da)>tolerance2) break;
      
      //
      // Redundant vertex found: do we have > 3 vertices?
      // 
      if (numVertices <= 3)
      {
        CalculateMaxMin();
        return false;
      }

      //
      // Delete vertex pointed to by next. Carefully!
      //
      if (curr->next)
      {    // next is not head
        if (next->next)
          curr->next = test;  // next is not tail
        else
          curr->next = 0;    // New tail
      }
      else
        vertexHead = test;  // New head
        
      delete next;
      
      numVertices--;
      
      //
      // Replace next by the vertex we just tested,
      // and keep on going...
      //
      next = test;
      da = dat; db = dbt;
    }
    curr = curr->next;
  }
  
  //
  // In principle, this is not needed, but why not just play it safe?
  //
  CalculateMaxMin();
  
  return true;
}


//
// ReverseOrder
//
// Reverse the order of the vertices
//
void G4ReduciblePolygon::ReverseOrder()
{
  //
  // Loop over all vertices
  //
  ABVertex *prev = vertexHead;
  if (prev==0) return;    // No vertices
  
  ABVertex *curr = prev->next;
  if (curr==0) return;    // Just one vertex
  
  //
  // Our new tail
  //
  vertexHead->next = 0;
  
  for(;;)
  {
    //
    // Save pointer to next vertex (in original order)
    //
    ABVertex *save = curr->next;
    
    //
    // Replace it with a pointer to the previous one
    // (in original order)
    //
    curr->next = prev;
    
    //
    // Last vertex?
    //
    if (save == 0) break;
    
    //
    // Next vertex
    //
    prev = curr;
    curr = save;
  }
  
  //
  // Our new head
  //
  vertexHead = curr;
}


//
// CrossesItself
//
// Return "true" if the polygon crosses itself
//
// Warning: this routine is not very fast (runs as N**2)
//
G4bool G4ReduciblePolygon::CrossesItself( G4double tolerance )
{
  G4double tolerance2 = tolerance*tolerance;
  G4double one = 1.0-tolerance,
     zero = tolerance;
  //
  // Top loop over line segments. By the time we finish
  // with the second to last segment, we're done.
  //
  ABVertex *curr1 = vertexHead, *next1=0;
  while (curr1->next) {
          next1 = curr1->next;
    G4double da1 = next1->a-curr1->a,
       db1 = next1->b-curr1->b;
    
    //
    // Inner loop over subsequent line segments
    //
    ABVertex *curr2 = next1->next;
    while( curr2 ) {
      ABVertex *next2 = curr2->next;
      if (next2==0) next2 = vertexHead;
      G4double da2 = next2->a-curr2->a,
         db2 = next2->b-curr2->b;
      G4double a12 = curr2->a-curr1->a,
         b12 = curr2->b-curr1->b;
         
      //
      // Calculate intersection of the two lines
      //
      G4double deter = da1*db2 - db1*da2;
      if (std::fabs(deter) > tolerance2) {
        G4double s1, s2;
        s1 = (a12*db2-b12*da2)/deter;
        
        if (s1 >= zero && s1 < one) {
          s2 = -(da1*b12-db1*a12)/deter;
          if (s2 >= zero && s2 < one) return true;
        }
      }
      curr2 = curr2->next;   
    }
    curr1 = next1;
  }
  return false;
}



//
// BisectedBy
//
// Decide if a line through two points crosses the polygon, within tolerance
//
G4bool G4ReduciblePolygon::BisectedBy( G4double a1, G4double b1,
                                       G4double a2, G4double b2,
                                       G4double tolerance )
{
  G4int nNeg = 0, nPos = 0;
  
  G4double a12 = a2-a1, b12 = b2-b1;
  G4double len12 = std::sqrt( a12*a12 + b12*b12 );
  a12 /= len12; b12 /= len12;
  
  ABVertex *curr = vertexHead;
  do
  {
    G4double av = curr->a - a1,
       bv = curr->b - b1;
       
    G4double cross = av*b12 - bv*a12;
    
    if (cross < -tolerance)
    {
      if (nPos) return true;
      nNeg++;
    }
    else if (cross > tolerance)
    {
      if (nNeg) return true;
      nPos++;
    }
    curr = curr->next;
  } while( curr );
    
  return false;
}



//
// Area
//
// Calculated signed polygon area, where polygons specified in a
// clockwise manner (where x==a, y==b) have negative area
//
//    References: [O' Rourke (C)] pp. 18-27; [Gems II] pp. 5-6:
//    "The Area of a Simple Polygon", Jon Rokne.
//
G4double G4ReduciblePolygon::Area()
{
  G4double answer = 0;
  
  ABVertex *curr = vertexHead, *next;
  do
  {
    next = curr->next;
    if (next==0) next = vertexHead;
    
    answer += curr->a*next->b - curr->b*next->a;
    curr = curr->next;
  } while( curr );
  
  return 0.5*answer;
}


//
// Print
//
void G4ReduciblePolygon::Print()
{
  ABVertex *curr = vertexHead;
  do
  {
//    G4cerr << curr->a << " " << curr->b << G4endl;
    curr = curr->next;
  } while( curr );
}


//
// CalculateMaxMin
//
// To be called when the vertices are changed, this
// routine re-calculates global values
//
void G4ReduciblePolygon::CalculateMaxMin()
{
  ABVertex *curr = vertexHead;
  aMin = aMax = curr->a;
  bMin = bMax = curr->b;
  curr = curr->next;
  while( curr )
  {
    if (curr->a < aMin)
      aMin = curr->a;
    else if (curr->a > aMax)
      aMax = curr->a;

    if (curr->b < bMin)
      bMin = curr->b;
    else if (curr->b > bMax)
      bMax = curr->b;
    
    curr = curr->next;
  }
}
