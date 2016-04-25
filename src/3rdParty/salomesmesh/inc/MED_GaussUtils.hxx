// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
//
// Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
// CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
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
#ifndef MED_GaussUtils_HeaderFile
#define MED_GaussUtils_HeaderFile

#include "MED_WrapperBase.hxx"

#include "MED_Structures.hxx"

namespace MED
{
  //---------------------------------------------------------------
  typedef TVector<TCCoordSlice> TCCoordSliceArr;
  typedef TVector<TCoordSlice> TCoordSliceArr;

  //! Define a helper class to handle Gauss Points coordinates
  class MEDWRAPPER_EXPORT TGaussCoord:
    virtual TModeSwitchInfo 
  {
    TInt myNbElem;
    TInt myNbGauss;
    TInt myDim;

    TInt myGaussStep;

    TNodeCoord myGaussCoord;

  public:
    
    TGaussCoord();

    //! To init the class
    void
    Init(TInt theNbElem,
         TInt theNbGauss,
         TInt theDim,
         EModeSwitch theMode = eFULL_INTERLACE);

    TInt
    GetNbElem() const;

    TInt
    GetNbGauss() const;

    TInt
    GetDim() const;

    unsigned char*
    GetValuePtr();

    //! Get slice of the coordinate that corresponds to defined cell (const version)
    TCCoordSliceArr
    GetCoordSliceArr(TInt theElemId) const;

    //! Get slice of the coordinate that corresponds to defined cell
    TCoordSliceArr 
    GetCoordSliceArr(TInt theElemId);
  };
  typedef SharedPtr<TGaussCoord> PGaussCoord;


  //---------------------------------------------------------------
  //! To calculate Gauss Points coordinates
  MEDWRAPPER_EXPORT 
  bool
  GetGaussCoord3D(const TGaussInfo& theGaussInfo, 
                  const TCellInfo& theCellInfo,
                  const TNodeInfo& theNodeInfo,
                  TGaussCoord& theGaussCoord,
                  const TElemNum& theElemNum = TElemNum(),
                  EModeSwitch theMode = eFULL_INTERLACE);


  //---------------------------------------------------------------
  //! To calculate Gauss Points coordinates for defined TCellInfo as its bary center
  MEDWRAPPER_EXPORT 
  bool
  GetBaryCenter(const TCellInfo& theCellInfo,
                const TNodeInfo& theNodeInfo,
                TGaussCoord& theGaussCoord,
                const TElemNum& theElemNum = TElemNum(),
                EModeSwitch theMode = eFULL_INTERLACE);

  //! To calculate Gauss Points coordinates for defined TPolygoneInfo as its bary center
  MEDWRAPPER_EXPORT 
  bool
  GetBaryCenter(const TPolygoneInfo& thePolygoneInfo,
                const TNodeInfo& theNodeInfo,
                TGaussCoord& theGaussCoord,
                const TElemNum& theElemNum = TElemNum(),
                EModeSwitch theMode = eFULL_INTERLACE);

  //! To calculate Gauss Points coordinates for defined TPolyedreInfo as its bary center
  MEDWRAPPER_EXPORT 
  bool
  GetBaryCenter(const TPolyedreInfo& thePolyedreInfo,
                const TNodeInfo& theNodeInfo,
                TGaussCoord& theGaussCoord,
                const TElemNum& theElemNum = TElemNum(),
                EModeSwitch theMode = eFULL_INTERLACE);

  //---------------------------------------------------------------
  //! Shape function definitions
  //---------------------------------------------------------------
  struct MEDWRAPPER_EXPORT TShapeFun
  {
    class TFun;
    
    TFloatVector myRefCoord;
    TInt myDim;
    TInt myNbRef;

    TShapeFun(TInt theDim = 0, TInt theNbRef = 0);

    TInt GetNbRef() const { return myNbRef; }

    TCCoordSlice GetCoord(TInt theRefId) const;

    TCoordSlice GetCoord(TInt theRefId);

    void GetFun(const TCCoordSliceArr& theRef,
                const TCCoordSliceArr& theGauss,
                TFun& theFun) const;
    virtual 
    void InitFun(const TCCoordSliceArr& theRef,
                 const TCCoordSliceArr& theGauss,
                 TFun& theFun) const = 0;
    virtual
    bool IsSatisfy(const TCCoordSliceArr& theRefCoord) const;

    bool Eval(const TCellInfo&       theCellInfo,
              const TNodeInfo&       theNodeInfo,
              const TElemNum&        theElemNum,
              const TCCoordSliceArr& theRef,
              const TCCoordSliceArr& theGauss,
              TGaussCoord&           theGaussCoord,
              EModeSwitch            theMode);
  };
  //---------------------------------------------------------------
  struct TSeg2a: TShapeFun {
    TSeg2a();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TSeg3a: TShapeFun {
    TSeg3a();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TTria3a: TShapeFun {
    TTria3a();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TTria6a: TShapeFun {
    TTria6a();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TTria3b: TShapeFun {
    TTria3b();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TTria6b: TShapeFun {
    TTria6b();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TQuad4a: TShapeFun {
    TQuad4a();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TQuad8a: TShapeFun {
    TQuad8a();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TQuad9a: TShapeFun {
    TQuad9a();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TQuad4b: TShapeFun {
    TQuad4b();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TQuad8b: TShapeFun {
    TQuad8b();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TQuad9b: TShapeFun {
    TQuad9b();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TTetra4a: TShapeFun {
    TTetra4a();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TTetra10a: TShapeFun {
    TTetra10a();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TTetra4b: TShapeFun {
    TTetra4b();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TTetra10b: TShapeFun {
    TTetra10b();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct THexa8a: TShapeFun {
    THexa8a();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct THexa20a: TShapeFun {
    THexa20a(TInt theDim = 3, TInt theNbRef = 20);
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct THexa27a: THexa20a {
    THexa27a();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct THexa8b: TShapeFun {
    THexa8b();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct THexa20b: TShapeFun {
    THexa20b(TInt theDim = 3, TInt theNbRef = 20);
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TPenta6a: TShapeFun {
    TPenta6a();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TPenta6b: TShapeFun {
    TPenta6b();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TPenta15a: TShapeFun {
    TPenta15a();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TPenta15b: TShapeFun {
    TPenta15b();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TPyra5a: TShapeFun {
    TPyra5a();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TPyra5b: TShapeFun {
    TPyra5b();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TPyra13a: TShapeFun {
    TPyra13a();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------
  struct TPyra13b: TShapeFun {
    TPyra13b();
    virtual void InitFun(const TCCoordSliceArr& theRef,
                         const TCCoordSliceArr& theGauss,
                         TFun& theFun) const;
  };
  //---------------------------------------------------------------

}

#endif
