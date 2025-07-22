// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
// File   : MED_GaussDef.hxx
// Author : Edward AGAPOV (eap)
//
#include "MED_GaussDef.hxx"
#include "MED_Utilities.hxx"
#include "MED_GaussUtils.hxx"

namespace MED
{
  using namespace std;
  using namespace MED;
  //---------------------------------------------------------------

  void TGaussDef::add(const double x, const double weight)
  {
    if ( dim() != 1 )
      EXCEPTION( logic_error,"dim() != 1");
    if ( myWeights.capacity() == myWeights.size() )
      EXCEPTION( logic_error,"Extra gauss point");
    myCoords.push_back( x );
    myWeights.push_back( weight );
  }
  void TGaussDef::add(const double x, const double y, const double weight)
  {
    if ( dim() != 2 )
      EXCEPTION( logic_error,"dim() != 2");
    if ( myWeights.capacity() == myWeights.size() )
      EXCEPTION( logic_error,"Extra gauss point");
    myCoords.push_back( x );
    myCoords.push_back( y );
    myWeights.push_back( weight );
  }
  void TGaussDef::add(const double x, const double y, const double z, const double weight)
  {
    if ( dim() != 3 )
      EXCEPTION( logic_error,"dim() != 3");
    if ( myWeights.capacity() == myWeights.size() )
      EXCEPTION( logic_error,"Extra gauss point");
    myCoords.push_back( x );
    myCoords.push_back( y );
    myCoords.push_back( z );
    myWeights.push_back( weight );
  }
  void TGaussDef::setRefCoords(const TShapeFun& aShapeFun)
  {
    myRefCoords.reserve( aShapeFun.myRefCoord.size() );
    myRefCoords.assign( aShapeFun.myRefCoord.begin(),
                        aShapeFun.myRefCoord.end() );
  }


  //---------------------------------------------------------------
  /*!
   * \brief Fill definition of gauss points family
   */
  //---------------------------------------------------------------

  TGaussDef::TGaussDef(const int geom, const int nbGauss, const int variant)
  {
    myType = geom;
    myCoords .reserve( nbGauss * dim() );
    myWeights.reserve( nbGauss );

    switch ( geom ) {

    case eSEG2:
    case eSEG3:
      if (geom == eSEG2) setRefCoords( TSeg2a() );
      else               setRefCoords( TSeg3a() );
      switch ( nbGauss ) {
      case 1: {
        add( 0.0, 2.0 ); break;
      }
      case 2: {
        const double a = 0.577350269189626;
        add(  a,  1.0 );
        add( -a,  1.0 ); break;
      }
      case 3: {
        const double a = 0.774596669241;
        const double P1 = 1./1.8;
        const double P2 = 1./1.125;
        add( -a,  P1 );
        add(  0,  P2 ); 
        add(  a,  P1 ); break;
      }
      case 4: {
        const double a  = 0.339981043584856, b  = 0.861136311594053;
        const double P1 = 0.652145154862546, P2 = 0.347854845137454 ;
        add(  a,  P1 );
        add( -a,  P1 );
        add(  b,  P2 ); 
        add( -b,  P2 ); break;
      }
      default:
        EXCEPTION( logic_error,"Invalid nb of gauss points for SEG"<<nbGauss);
      }
      break;

    case eTRIA3:
    case eTRIA6:
      if ( variant == 1 ) {
        if (geom == eTRIA3) setRefCoords( TTria3b() );
        else                setRefCoords( TTria6b() );
        switch ( nbGauss ) {
        case 1: { // FPG1
          add( 1/3., 1/3., 1/2. ); break;
        }
        case 3: { // FPG3
          // what about COT3 ???
          add( 1/6., 1/6., 1/6. );
          add( 2/3., 1/6., 1/6. );
          add( 1/6., 2/3., 1/6. ); break;
        }
        case 4: { // FPG4
          add( 1/5., 1/5.,  25/(24*4.) );
          add( 3/5., 1/5.,  25/(24*4.) );
          add( 1/5., 3/5.,  25/(24*4.) );
          add( 1/3., 1/3., -27/(24*4.) ); break;
        }
        case 6: { // FPG6
          const double P1 = 0.11169079483905, P2 = 0.0549758718227661;
          const double a  = 0.445948490915965, b = 0.091576213509771;
          add(     b,     b, P2 ); 
          add( 1-2*b,     b, P2 );
          add(     b, 1-2*b, P2 );
          add(     a, 1-2*a, P1 );
          add(     a,     a, P1 ); 
          add( 1-2*a,     a, P1 ); break;
        }
        case 7: { // FPG7
          const double A  = 0.470142064105115;
          const double B  = 0.101286507323456;
          const double P1 = 0.066197076394253;
          const double P2 = 0.062969590272413;
          add(  1/3.,  1/3., 9/80. ); 
          add(     A,     A, P1 ); 
          add( 1-2*A,     A, P1 );
          add(     A, 1-2*A, P1 );
          add(     B,     B, P2 ); 
          add( 1-2*B,     B, P2 );
          add(     B, 1-2*B, P2 ); break;
        }
        case 12: { // FPG12
          const double A  = 0.063089014491502;
          const double B  = 0.249286745170910;
          const double C  = 0.310352451033785;
          const double D  = 0.053145049844816;
          const double P1 = 0.025422453185103;
          const double P2 = 0.058393137863189;
          const double P3 = 0.041425537809187;
          add(     A,     A, P1 ); 
          add( 1-2*A,     A, P1 );
          add(     A, 1-2*A, P1 );
          add(     B,     B, P2 ); 
          add( 1-2*B,     B, P2 );
          add(     B, 1-2*B, P2 );
          add(     C,     D, P3 );
          add(     D,     C, P3 );
          add( 1-C-D,     C, P3 );
          add( 1-C-D,     D, P3 );
          add(     C, 1-C-D, P3 );
          add(     D, 1-C-D, P3 ); break;
        }
        default:
          EXCEPTION( logic_error,"Invalid nb of gauss points for TRIA, variant 1: "
                     <<nbGauss);
        }
      }
      else if ( variant == 2 ) {
        if (geom == eTRIA3) setRefCoords( TTria3a() );
        else                setRefCoords( TTria6a() );
        switch ( nbGauss ) {
        case 1: {
          add( -1/3., -1/3., 2. ); break;
        }
        case 3: {
          add( -2/3.,  1/3., 2/3. );
          add( -2/3., -2/3., 2/3. );
          add(  1/3., -2/3., 2/3. ); break;
        }
        case 6: {
          const double P1 = 0.11169079483905, P2 = 0.0549758718227661;
          const double A  = 0.445948490915965, B = 0.091576213509771;
          add( 2*B-1, 1-4*B, 4*P2 ); 
          add( 2*B-1, 2*B-1, 4*P2 );
          add( 1-4*B, 2*B-1, 4*P2 );
          add( 1-4*A, 2*A-1, 4*P1 );
          add( 2*A-1, 1-4*A, 4*P1 ); 
          add( 2*A-1, 2*A-1, 4*P1 ); break;
        }
        default:
          EXCEPTION( logic_error,"Invalid nb of gauss points for TRIA, variant 2: "
                     <<nbGauss);
        }
      }
      else if ( variant == 3 ) {
        if (geom == eTRIA3) setRefCoords( TTria3b() );
        else                setRefCoords( TTria6b() );
        switch ( nbGauss ) {
        case 4: {
          add( 1/3., 1/3., -27/96 );
          add( 0.2 , 0.2 ,  25/96 );
          add( 0.6 , 0.2 ,  25/96 );
          add( 0.2 , 0.6 ,  25/96 ); break;
        }
        default:
          EXCEPTION( logic_error,"Invalid nb of gauss points for TRIA, variant 3: "
                     <<nbGauss);
        }
      }
      break;

    case eQUAD4:
    case eQUAD8:
      if ( variant == 1 ) {
        if (geom == eQUAD4) setRefCoords( TQuad4b() );
        else                setRefCoords( TQuad8b() );
        switch ( nbGauss ) {
        case 1: { // FPG1
          add(  0,  0,  4 ); break;
        }
        case 4: { // FPG4
          const double a = 1/sqrt(3.);
          add( -a, -a,  1 );
          add(  a, -a,  1 );
          add(  a,  a,  1 );
          add( -a,  a,  1 ); break;
        }
        case 9: { // FPG9
          const double a = 0.774596669241483;
          add( -a, -a,  25/81. );
          add(  a, -a,  25/81. );
          add(  a,  a,  25/81. );
          add( -a,  a,  25/81. );
          add( 0., -a,  40/81. );
          add(  a, 0.,  40/81. );
          add( 0.,  a,  40/81. );
          add( -a, 0.,  40/81. );
          add( 0., 0.,  64/81. ); break;
        }
        default:
          EXCEPTION( logic_error,"Invalid nb of gauss points for QUAD, variant 1: "
                     <<nbGauss);
        }
      }
      else if ( variant == 2 ) {
        if (geom == eQUAD4) setRefCoords( TQuad4a() );
        else                setRefCoords( TQuad8a() );
        switch ( nbGauss ) {
        case 4: {
          const double a = 1/sqrt(3.);
          add( -a,  a,  1 );
          add( -a, -a,  1 );
          add(  a, -a,  1 );
          add(  a,  a,  1 ); break;
        }
        case 9: {
          const double a = 0.774596669241483;
          add( -a,  a,  25/81. );
          add( -a, -a,  25/81. );
          add(  a, -a,  25/81. );
          add(  a,  a,  25/81. );
          add( -a, 0.,  40/81. );
          add( 0., -a,  40/81. );
          add(  a, 0.,  40/81. );
          add( 0.,  a,  40/81. );
          add( 0., 0.,  64/81. ); break;
        }
        default:
          EXCEPTION( logic_error,"Invalid nb of gauss points for QUAD, variant 1: "
                     <<nbGauss);
        }
      }
      else if ( variant == 3 ) {
        if (geom == eQUAD4) setRefCoords( TQuad4b() );
        else                setRefCoords( TQuad8b() );
        switch ( nbGauss ) {
        case 4: {
          const double a = 3/sqrt(3.);
          add( -a, -a,  1 );
          add( -a,  a,  1 );
          add(  a, -a,  1 );
          add(  a,  a,  1 ); break;
        }
        case 9: {
          const double a = sqrt(3/5.), c1 = 5/9., c2 = 8/9.;
          const double c12 = c1*c2, c22 = c2*c2, c1c2 = c1*c2;
          add( -a, -a,  c12  );
          add( -a, 0.,  c1c2 );
          add( -a,  a,  c12  );
          add( 0., -a,  c1c2 );
          add( 0., 0.,  c22  );
          add( 0.,  a,  c1c2 );
          add(  a, -a,  c12  );
          add(  a, 0.,  c1c2 );
          add(  a,  a,  c12  ); break;
        }
        default:
          EXCEPTION( logic_error,"Invalid nb of gauss points for QUAD, variant 3: "
                     <<nbGauss);
        }
      }
      break;

    case eTETRA4:
    case eTETRA10:
      if (geom == eTETRA4) setRefCoords( TTetra4a() );
      else                 setRefCoords( TTetra10a() );
      switch ( nbGauss ) {
      case 4: { // FPG4
        const double a = (5 - sqrt(5.))/20., b = (5 + 3*sqrt(5.))/20.;
        add(  a,  a,  a,  1/24. );
        add(  a,  a,  b,  1/24. );
        add(  a,  b,  a,  1/24. );
        add(  b,  a,  a,  1/24. ); break;
      }
      case 5: { // FPG5
        const double a = 0.25, b = 1/6., c = 0.5;
        add(  a,  a,  a, -2/15. );
        add(  b,  b,  b,  3/40. );
        add(  b,  b,  c,  3/40. );
        add(  b,  c,  b,  3/40. );
        add(  c,  b,  b,  3/40. ); break;
      }
      case 15: { // FPG15
        const double a = 0.25;
        const double b1 = (7 + sqrt(15.))/34., c1 = (13 + 3*sqrt(15.))/34., d = (5 - sqrt(15.))/20.;
        const double b2 = (7 - sqrt(15.))/34., c2 = (13 - 3*sqrt(15.))/34., e = (5 + sqrt(15.))/20.;
        const double P1 = (2665 - 14*sqrt(15.))/226800.;
        const double P2 = (2665 + 14*sqrt(15.))/226800.;
        add(  a,  a,  a,  8/405.);//_____
        add( b1, b1, b1,  P1    );
        add( b1, b1, c1,  P1    );
        add( b1, c1, b1,  P1    );
        add( c1, b1, b1,  P1    );//_____
        add( b2, b2, b2,  P2    );
        add( b2, b2, c2,  P2    );
        add( b2, c2, b2,  P2    );
        add( c2, b2, b2,  P2    );//_____
        add(  d,  d,  e,  5/567.);
        add(  d,  e,  d,  5/567.);
        add(  e,  d,  d,  5/567.);
        add(  d,  e,  e,  5/567.);
        add(  e,  d,  e,  5/567.);
        add(  e,  e,  d,  5/567.);
        break;
      }
      default:
        EXCEPTION( logic_error,"Invalid nb of gauss points for TETRA: "<<nbGauss);
      }
      break;

    case ePYRA5:
    case ePYRA13:
      if (geom == ePYRA5) setRefCoords( TPyra5a() );
      else                setRefCoords( TPyra13a() );
      switch ( nbGauss ) {
      case 5: { // FPG5
        const double h1 = 0.1531754163448146;
        const double h2 = 0.6372983346207416;
        add(  .5,  0.,  h1,  2/15. );
        add(  0.,  .5,  h1,  2/15. );
        add( -.5,  0.,  h1,  2/15. );
        add(  0., -.5,  h1,  2/15. );
        add(  0.,  0.,  h2,  2/15. ); break;
      }
      case 6: { // FPG6
        const double p1 = 0.1024890634400000 ;
        const double p2 = 0.1100000000000000 ;
        const double p3 = 0.1467104129066667 ;
        const double a  = 0.5702963741068025 ;
        const double h1 = 0.1666666666666666 ;
        const double h2 = 0.08063183038464675;
        const double h3 = 0.6098484849057127 ;
        add(  a, 0.,  h1,  p1 );
        add( 0.,  a,  h1,  p1 );
        add( -a, 0.,  h1,  p1 );
        add( 0., -a,  h1,  p1 );
        add( 0., 0.,  h2,  p2 );
        add( 0., 0.,  h3,  p3 ); break;
      }
      case 27: { // FPG27
        const double a1  = 0.788073483; 
        const double b6  = 0.499369002; 
        const double b1  = 0.848418011; 
        const double c8  = 0.478508449; 
        const double c1  = 0.652816472; 
        const double d12 = 0.032303742; 
        const double d1  = 1.106412899;
        double z = 1/2., fz = b1/2*(1 - z);
        add(  0.,  0.,   z,  a1 ); // 1
        add(  fz,  fz,   z,  b6 ); // 2
        add( -fz,  fz,   z,  b6 ); // 3
        add( -fz, -fz,   z,  b6 ); // 4
        add(  fz, -fz,   z,  b6 ); // 5
        z = (1 - b1)/2.;
        add(  0.,  0.,   z,  b6 ); // 6
        z = (1 + b1)/2.;
        add(  0.,  0.,   z,  b6 ); // 7
        z = (1 - c1)/2.; fz = c1*(1 - z);
        add(  fz,  0.,   z,  c8 ); // 8
        add(  0.,  fz,   z,  c8 ); // 9
        add( -fz,  0.,   z,  c8 ); // 10
        add(  0., -fz,   z,  c8 ); // 11
        z = (1 + c1)/2.; fz = c1*(1 - z);
        add(  fz,  0.,   z,  c8 ); // 12
        add(  0.,  fz,   z,  c8 ); // 13
        add( -fz,  0.,   z,  c8 ); // 14
        add(  0., -fz,   z,  c8 ); // 15
        z = (1 - d1)/2., fz = d1/2*(1 - z);
        add(  fz,  fz,   z,  d12); // 16
        add( -fz,  fz,   z,  d12); // 17
        add( -fz, -fz,   z,  d12); // 18
        add(  fz, -fz,   z,  d12); // 19
        z = 1/2.; fz = d1*(1 - z);
        add(  fz,  0.,   z,  d12); // 20
        add(  0.,  fz,   z,  d12); // 21
        add( -fz,  0.,   z,  d12); // 22
        add(  0., -fz,   z,  d12); // 23
        z = (1 + d1)/2., fz = d1/2*(1 - z);
        add(  fz,  fz,   z,  d12); // 24
        add( -fz,  fz,   z,  d12); // 25
        add( -fz, -fz,   z,  d12); // 26
        add(  fz, -fz,   z,  d12); // 27
        break;
      }
      default:
        EXCEPTION( logic_error,"Invalid nb of gauss points for PYRA: "<<nbGauss);
      }
      break;
    case ePENTA6:
    case ePENTA15:
      if (geom == ePENTA6) setRefCoords( TPenta6a() );
      else                 setRefCoords( TPenta15a() );
      switch ( nbGauss ) {
      case 6: { // FPG6
        const double a = sqrt(3.)/3.;
        add( -a, .5, .5,  1/6. );
        add( -a, 0., .5,  1/6. );
        add( -a, .5, 0.,  1/6. );
        add(  a, .5, .5,  1/6. );
        add(  a, 0., .5,  1/6. );
        add(  a, .5, 0.,  1/6. ); break;
      }
      case 8: { // FPG8
        const double a = 0.577350269189626;
        add( -a, 1/3., 1/3., -27/96. );
        add( -a,  0.6,  0.2,  25/96. );
        add( -a,  0.2,  0.6,  25/96. );
        add( -a,  0.2,  0.2,  25/96. );
        add( +a, 1/3., 1/3., -27/96. );
        add( +a,  0.6,  0.2,  25/96. );
        add( +a,  0.2,  0.6,  25/96. );
        add( +a,  0.2,  0.2,  25/96. ); break;
      }
      case 21: { // FPG21
        const double d = sqrt(3/5.), c1 = 5/9., c2 = 8/9.; // d <=> alfa
        const double a = (6 + sqrt(15.))/21.;
        const double b = (6 - sqrt(15.))/21.;
        const double P1 = (155 + sqrt(15.))/2400.;
        const double P2 = (155 - sqrt(15.))/2400.;  //___
        add( -d,  1/3.,  1/3., c1*9/80. );//___
        add( -d,     a,     a, c1*P1    );
        add( -d, 1-2*a,     a, c1*P1    );
        add( -d,     a, 1-2*a, c1*P1    );//___
        add( -d,     b,     b, c1*P2    );
        add( -d, 1-2*b,     b, c1*P2    );
        add( -d,     b, 1-2*b, c1*P2    );//___
        add( 0.,  1/3.,  1/3., c2*9/80. );//___
        add( 0.,     a,     a, c2*P1    );
        add( 0., 1-2*a,     a, c2*P1    );
        add( 0.,     a, 1-2*a, c2*P1    );//___
        add( 0.,     b,     b, c2*P2    );
        add( 0., 1-2*b,     b, c2*P2    );
        add( 0.,     b, 1-2*b, c2*P2    );//___
        add(  d,  1/3.,  1/3., c1*9/80. );//___
        add(  d,     a,     a, c1*P1    );
        add(  d, 1-2*a,     a, c1*P1    );
        add(  d,     a, 1-2*a, c1*P1    );//___
        add(  d,     b,     b, c1*P2    );
        add(  d, 1-2*b,     b, c1*P2    );
        add(  d,     b, 1-2*b, c1*P2    );//___
        break;
      }
      default:
        EXCEPTION( logic_error,"Invalid nb of gauss points for PENTA: " <<nbGauss);
      }
      break;

    case eHEXA8:
    case eHEXA20:
      if (geom == eHEXA8) setRefCoords( THexa8a() );
      else                setRefCoords( THexa20a() );
      switch ( nbGauss ) {
      case 8: { // FPG8
        const double a = sqrt(3.)/3.;
        add( -a, -a, -a,  1. );
        add( -a, -a,  a,  1. );
        add( -a,  a, -a,  1. );
        add( -a,  a,  a,  1. );
        add(  a, -a, -a,  1. );
        add(  a, -a,  a,  1. );
        add(  a,  a, -a,  1. );
        add(  a,  a,  a,  1. ); break;
      }
      case 27: { // FPG27
        const double a = sqrt(3/5.), c1 = 5/9., c2 = 8/9.;
        const double c12 = c1*c1, c13 = c1*c1*c1;
        const double c22 = c2*c2, c23 = c2*c2*c2;
        add( -a, -a, -a,   c13  ); // 1
        add( -a, -a, 0., c12*c2 ); // 2
        add( -a, -a,  a,   c13  ); // 3
        add( -a, 0., -a, c12*c2 ); // 4
        add( -a, 0., 0., c1*c22 ); // 5
        add( -a, 0.,  a, c12*c2 ); // 6
        add( -a,  a, -a,   c13  ); // 7
        add( -a,  a, 0., c12*c2 ); // 8
        add( -a,  a,  a,   c13  ); // 9
        add( 0., -a, -a, c12*c2 ); // 10
        add( 0., -a, 0., c1*c22 ); // 11
        add( 0., -a,  a, c12*c2 ); // 12
        add( 0., 0., -a, c1*c22 ); // 13
        add( 0., 0., 0.,   c23  ); // 14
        add( 0., 0.,  a, c1*c22 ); // 15
        add( 0.,  a, -a, c12*c2 ); // 16
        add( 0.,  a, 0., c1*c22 ); // 17
        add( 0.,  a,  a, c12*c2 ); // 18
        add(  a, -a, -a,   c13  ); // 19
        add(  a, -a, 0., c12*c2 ); // 20
        add(  a, -a,  a,   c13  ); // 21
        add(  a, 0., -a, c12*c2 ); // 22
        add(  a, 0., 0., c1*c22 ); // 23
        add(  a, 0.,  a, c12*c2 ); // 24
        add(  a,  a, -a,   c13  ); // 25
        add(  a,  a, 0., c12*c2 ); // 26
        add(  a,  a,  a,   c13  ); // 27
        break;
      }
      default:
        EXCEPTION( logic_error,"Invalid nb of gauss points for PENTA: " <<nbGauss);
      }
      break;

    default:
      EXCEPTION( logic_error,"unexpected EGeometrieElement: "<< geom);
    }

    if ( myWeights.capacity() != myWeights.size() )
      EXCEPTION( logic_error,"Not all gauss points defined");
  }
}
