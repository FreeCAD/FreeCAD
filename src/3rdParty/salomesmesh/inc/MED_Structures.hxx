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

#ifndef MED_Structures_HeaderFile
#define MED_Structures_HeaderFile

#include "MED_Common.hxx"
#include "MED_Utilities.hxx"

#ifdef WIN32
#pragma warning(disable:4251)
#endif

namespace MED
{

  //---------------------------------------------------------------
  //! Defines a type for managing sequence of strings
  typedef TVector<char> TString; 
  typedef SharedPtr<TString> PString;

  //! Extract a substring from the sequence of the strings
  MEDWRAPPER_EXPORT
  std::string 
  GetString(TInt theId, TInt theStep, 
            const TString& theString);
  
  //! Set a substring in the sequence of the strings
  MEDWRAPPER_EXPORT 
  void
  SetString(TInt theId, TInt theStep, 
                 TString& theString, 
                 const std::string& theValue);

  //! Set a substring in the sequence of the strings
  MEDWRAPPER_EXPORT
  void
  SetString(TInt theId, TInt theStep, 
                 TString& theString, 
                 const TString& theValue);

  //---------------------------------------------------------------
  //! Define a parent class for all MEDWrapper classes
  struct MEDWRAPPER_EXPORT TBase
  {
    virtual ~TBase() {} 
  };


  //---------------------------------------------------------------
  //! Define a parent class for all named MED entities
  struct MEDWRAPPER_EXPORT TNameInfo: virtual TBase
  {
    TString myName; //!< Keeps its name
    virtual std::string GetName() const = 0; //!< Gets its name
    virtual void SetName(const std::string& theValue) = 0; //!< Set a new name
    virtual void SetName(const TString& theValue) = 0; //!< Set a new name
  };


  //---------------------------------------------------------------
  //! Define a parent class for all MED entities that contains a sequence of numbers
  /*!
    It defines through corresponding enumeration (EModeSwitch) how the sequence 
    should be interpreted in C or Fortran mode (eFULL_INTERLACE or eNON_INTERLACE).
  */
  struct MEDWRAPPER_EXPORT TModeSwitchInfo: virtual TBase
  {
    //! To construct instance of the class by default
    TModeSwitchInfo():
      myModeSwitch(eFULL_INTERLACE)
    {}

    //! To construct instance of the class
    TModeSwitchInfo(EModeSwitch theModeSwitch):
      myModeSwitch(theModeSwitch)
    {}

    EModeSwitch myModeSwitch; //!< Keeps the 
    EModeSwitch GetModeSwitch() const { return myModeSwitch;}
  };


  //---------------------------------------------------------------
  //! Define a base class which represents MED Mesh entity
  struct MEDWRAPPER_EXPORT TMeshInfo: virtual TNameInfo
  {
    TInt myDim; //!< Dimension of the mesh (0, 1, 2 or 3)
    TInt GetDim() const { return myDim;} //!< Gets dimension of the mesh

    TInt mySpaceDim;
    TInt GetSpaceDim() const { return mySpaceDim; }

    EMaillage myType; //!< Type of the mesh
    EMaillage GetType() const { return myType;} //!< Gets type of the mesh

    TString myDesc; //!< Description of the mesh
    virtual std::string GetDesc() const = 0; //!< Get description for the mesh
    virtual void SetDesc(const std::string& theValue) = 0; //!< Sets description for the mesh

    
  };
  

  //---------------------------------------------------------------
  typedef TVector<TInt> TIntVector;
  typedef TSlice<TInt> TIntVecSlice;
  typedef TCSlice<TInt> TCIntVecSlice;

  typedef TIntVector TFamAttr;

  //! Define a base class which represents MED Family entity
  struct MEDWRAPPER_EXPORT TFamilyInfo: virtual TNameInfo
  {
    PMeshInfo myMeshInfo; //!< A reference to corresponding MED Mesh
    //! Get a reference to corresponding MED Mesh
    const PMeshInfo& GetMeshInfo() const { return myMeshInfo;} 

    TInt myId; //!< An unique index of the MED FAMILY
    TInt GetId() const { return myId;} //!< Gets number of the MED FAMILY
    void SetId(TInt theId) { myId = theId;} //! Define number of the MED FAMILY

    TInt myNbGroup; //!< Defines number MED Groups connected to
    //! Gets number of MED GROUPS the MED FAMILY is bound to
    TInt GetNbGroup() const { return myNbGroup;} 

    //! Contains sequence of the names for the MED Groups connected to
    TString myGroupNames; 
    //! Gets name of a bound MED GROUP by its number
    virtual std::string GetGroupName(TInt theId) const = 0;
    //! Sets name of the defined MED GROUP by its number
    virtual void SetGroupName(TInt theId, const std::string& theValue) = 0;

    TInt myNbAttr; //!< Defines number of the MED Family attributes 
    //! Gets number of attached attributes for the MED FAMILY
    TInt GetNbAttr() const { return myNbAttr;} 

    //! Defines sequence of the indexes of the MED Family attributes
    TFamAttr myAttrId; 
    //! Get MED FAMILY attribute by its number
    TInt GetAttrId(TInt theId) const;
    //! Set MED FAMILY attribute by its number
    void SetAttrId(TInt theId, TInt theVal);

    //! Defines sequence of the values of the MED Family attributes
    TFamAttr myAttrVal;
    //! Get MED FAMILY attribute by its number
    TInt GetAttrVal(TInt theId) const;
    //! Set MED FAMILY attribute by its number
    void SetAttrVal(TInt theId, TInt theVal);

    //! Defines sequence of the names of the MED Family attributes
    TString myAttrDesc;
    //! Get value of the MED FAMILY attribute by its number
    virtual std::string GetAttrDesc(TInt theId) const = 0;
    //! Set value of the MED FAMILY attribute by its number
    virtual void SetAttrDesc(TInt theId, const std::string& theValue) = 0;
  };


  //---------------------------------------------------------------
  typedef TIntVector TElemNum;
  typedef SharedPtr<TElemNum> PElemNum;
  
  //! Define a parent class for all MED entities that describes mesh entities such as nodes and cells.
  struct MEDWRAPPER_EXPORT TElemInfo: virtual TBase
  {
    PMeshInfo myMeshInfo; //!< A reference to corresponding MED Mesh
    //! Get a reference to corresponding MED Mesh
    const PMeshInfo& GetMeshInfo() const { return myMeshInfo;}

    TInt myNbElem; //<! Number of corresponding mesh entities
    TInt GetNbElem() const { return myNbElem;} //! Get number of mesh elements
    
    //! Defines sequence MED Family indexes for corresponding mesh entities
    PElemNum myFamNum; 
    //! Get number of a MED FAMILY by order number of the mesh element
    TInt GetFamNum(TInt theId) const;
    //! Set number of a MED FAMILY for the mesh element with the  order number
    void SetFamNum(TInt theId, TInt theVal);

    //! Defines if the mesh elements are indexed
    EBooleen myIsElemNum;
    //! Let know if the mesh elements are indexed
    EBooleen IsElemNum() const { return myIsElemNum;}
    
    //! Defines if the mesh elements family are indexed
    EBooleen myIsFamNum;
    //! Let know if the mesh elements family are indexed
    EBooleen IsFamNum() const { return myIsFamNum;}
    

    //! Contains sequence of the indexes for the mesh elements
    PElemNum myElemNum;
    //! Get a reference number of the mesh element by its order number
    TInt GetElemNum(TInt theId) const;
    //! Set a reference number for the mesh element by its order number
    void SetElemNum(TInt theId, TInt theVal);

    //! Defines if the mesh elements are named
    EBooleen myIsElemNames;
    //! Let know if the mesh elements have names
    EBooleen IsElemNames() const { return myIsElemNames;}

    //! Contains sequence of the names for the mesh elements
    PString myElemNames;
    //! Get name of the mesh element by its order number
    virtual std::string GetElemName(TInt theId) const = 0;
    //! Set name of the mesh element by its order number
    virtual void SetElemName(TInt theId, const std::string& theValue) = 0;
  };


  //---------------------------------------------------------------
  typedef TVector<TFloat> TFloatVector;
  typedef TSlice<TFloat> TFloatVecSlice;
  typedef TCSlice<TFloat> TCFloatVecSlice;

  typedef TFloatVector TNodeCoord;
  typedef SharedPtr<TNodeCoord> PNodeCoord;

  typedef TFloatVecSlice TCoordSlice;
  typedef TCFloatVecSlice TCCoordSlice;

  //! Define a base class which represents MED Nodes entity
  struct MEDWRAPPER_EXPORT TNodeInfo: 
    virtual TElemInfo,
    virtual TModeSwitchInfo 
  {
    PNodeCoord myCoord; //!< Contains all nodal coordinates

    //! Gives coordinates for mesh node by its number (const version)
    TCCoordSlice GetCoordSlice(TInt theId) const;
    //! Gives coordinates for mesh node by its number
    TCoordSlice GetCoordSlice(TInt theId);

    ERepere mySystem; //!< Defines, which coordinate system is used
    //! Get which coordinate system is used for the node describing
    ERepere GetSystem() const { return mySystem;}
    //! Set coordinate system to be used for the node describing
    void SetSystem(ERepere theSystem) { mySystem = theSystem;}

    TString myCoordNames; //!< Contains names for the coordinate dimensions
    //! Get name of the coordinate dimension by its order number
    virtual std::string GetCoordName(TInt theId) const = 0;
    //! Set name of the coordinate dimension by its order number
    virtual void SetCoordName(TInt theId, const std::string& theValue) = 0;

    TString myCoordUnits; //!< Contains units for the coordinate dimensions
    //! Get name of unit for the coordinate dimension by its order number
    virtual std::string GetCoordUnit(TInt theId) const = 0;
    //! Set name of unit for the coordinate dimension by its order number
    virtual void SetCoordUnit(TInt theId, const std::string& theValue) = 0;
  };


  //---------------------------------------------------------------
  typedef TIntVecSlice TConnSlice;
  typedef TCIntVecSlice TCConnSlice;

  //! Define a base class which represents MED Cells entity
  struct MEDWRAPPER_EXPORT TCellInfo: 
    virtual TElemInfo,
    virtual TModeSwitchInfo 
  {
    EEntiteMaillage myEntity; //!< Defines the MED Entity where the mesh cells belongs to
    //! Let known what MED ENTITY the cells belong to
    EEntiteMaillage GetEntity() const { return myEntity;}

    EGeometrieElement myGeom; //!< Defines the MED Geometric type of the instance
    //! Let known what MED geometrical type the cells belong to
    EGeometrieElement GetGeom() const { return myGeom;}

    EConnectivite myConnMode; //!< Defines connectivity mode
    //! Let known in what connectivity the cells are written
    EConnectivite GetConnMode() const { return myConnMode;}

    virtual TInt GetConnDim() const = 0; //!< Gives step in the connectivity sequence

    PElemNum myConn; //!< Defines sequence which describe connectivity for each of mesh cell

    //! Gives connectivities for mesh cell by its number (const version)
    TCConnSlice GetConnSlice(TInt theElemId) const;
    //! Gives connectivities for mesh cell by its number
    TConnSlice GetConnSlice(TInt theElemId);
  };

  //---------------------------------------------------------------
  //! Define a base class which represents MED Polygon entity
  struct MEDWRAPPER_EXPORT TPolygoneInfo: 
    virtual TElemInfo
  {
    //! Defines the MED Entity where the polygons belongs to
    EEntiteMaillage myEntity; // MED_FACE|MED_MAILLE
    //! Let known what MED ENTITY the MED Polygons belong to
    EEntiteMaillage GetEntity() const { return myEntity;}

    //! Defines the MED Geometric type of the instance
    EGeometrieElement myGeom; // ePOLYGONE
    //! Let known what MED geometrical type the MED Polygons belong to
    EGeometrieElement GetGeom() const { return ePOLYGONE;}

    //! Defines connectivity mode
    EConnectivite myConnMode; // eNOD|eDESC(eDESC not used)
    //! Let known in what connectivity the cells are written
    EConnectivite GetConnMode() const { return myConnMode;}

    PElemNum myConn; //!< Table de connectivities
    PElemNum myIndex; //!< Table de indexes

    //! Gives number of the connectivities for the defined polygon
    TInt GetNbConn(TInt theElemId) const;

    //! Gives connectivities for polygon by its number (const version)
    TCConnSlice GetConnSlice(TInt theElemId) const;
    //! Gives connectivities for polygon by its number
    TConnSlice GetConnSlice(TInt theElemId);
  };

  //---------------------------------------------------------------
  //! Define a class representing MED_BALL structure element.
  //
  //  This could be a generic class for any structure element
  //  holding any number of contant and variable attributes
  //  but it's too hard to implement
  //
  struct MEDWRAPPER_EXPORT TBallInfo: 
    virtual TCellInfo
  {
    TFloatVector myDiameters;
  };

  //---------------------------------------------------------------
  typedef TVector<TCConnSlice> TCConnSliceArr;
  typedef TVector<TConnSlice> TConnSliceArr;

  //! Define a base class which represents MED Polyedre entity
  struct MEDWRAPPER_EXPORT TPolyedreInfo: 
    virtual TElemInfo
  {
    //! Defines the MED Entity where the polyedres belongs to
    EEntiteMaillage myEntity; // MED_FACE|MED_MAILLE
    //! Let known what MED ENTITY the MED Polyedres belong to
    EEntiteMaillage GetEntity() const { return myEntity;}

    //! Defines the MED Geometric type of the instance
    EGeometrieElement myGeom; // ePOLYEDRE
    //! Let known what MED geometrical type the MED Polyedres belong to
    EGeometrieElement GetGeom() const { return ePOLYEDRE;}

    //! Defines connectivity mode
    EConnectivite myConnMode; // eNOD|eDESC(eDESC not used)
    //! Let known in what connectivity the cells are written
    EConnectivite GetConnMode() const { return myConnMode;}

    PElemNum myConn; //!< Table de connectivities
    PElemNum myFaces; //!< Table de faces indexes
    PElemNum myIndex; //!< Table de indexes

    //! Gives number of the faces for the defined polyedre (const version)
    TInt GetNbFaces(TInt theElemId) const;
    //! Gives number of the nodes for the defined polyedre
    TInt GetNbNodes(TInt theElemId) const;

    //! Gives sequence of the face connectivities for polyedre by its number (const version)
    TCConnSliceArr GetConnSliceArr(TInt theElemId) const;
    //! Gives sequence of the face connectivities for polyedre by its number
    TConnSliceArr GetConnSliceArr(TInt theElemId);
  };

  //---------------------------------------------------------------
  //! Define a base class which represents MED Field entity
  struct MEDWRAPPER_EXPORT TFieldInfo: 
    virtual TNameInfo
  {
    PMeshInfo myMeshInfo; //!< A reference to corresponding MED Mesh
    //! Get a reference to corresponding MED Mesh
    const PMeshInfo& GetMeshInfo() const { return myMeshInfo;}

    ETypeChamp myType; //!< Defines type of the MED Field
    //! Let known what type of the MED FIELD is used
    ETypeChamp GetType() const { return myType;}

    TInt myNbComp; //!< Defines number of components stored in the field
    //! Get number of components for the MED FIELD
    TInt GetNbComp() const { return myNbComp;}

    EBooleen myIsLocal; //!< Defines if the MED Field is local
    //! Let known is the MED FIELD is local or not
    EBooleen GetIsLocal() const { return myIsLocal;}

    TInt myNbRef; //!< Defines number of references of the field
    //! Let known number of references for the MED FIELD
    TInt GetNbRef() const { return myNbRef;}

    TString myCompNames; //!< Contains names for each of MED Field components
    //! Get name of the component by its order number
    virtual std::string GetCompName(TInt theId) const = 0;
    //! Set name for the component by its order number
    virtual void SetCompName(TInt theId, const std::string& theValue) = 0;

    TString myUnitNames; //!< Contains units for each of MED Field components
    //! Get unit of the component by its order number
    virtual std::string GetUnitName(TInt theId) const = 0;
    //! Set unit for the component by its order number
    virtual void SetUnitName(TInt theId, const std::string& theValue) = 0;

  };


  //---------------------------------------------------------------
  //! Get dimension of the Gauss coordinates for the defined type of mesh cell
  MEDWRAPPER_EXPORT
  TInt
  GetDimGaussCoord(EGeometrieElement theGeom);

  //! Get number of referenced nodes for the defined type of mesh cell
  MEDWRAPPER_EXPORT
  TInt
  GetNbRefCoord(EGeometrieElement theGeom);

  typedef TFloatVector TWeight;

  //! The class represents MED Gauss entity
  struct MEDWRAPPER_EXPORT TGaussInfo: 
    virtual TNameInfo,
    virtual TModeSwitchInfo 
  {
    typedef boost::tuple<EGeometrieElement,std::string> TKey;
    typedef boost::tuple<TKey,TInt> TInfo;
    struct MEDWRAPPER_EXPORT TLess
    {
      bool
      operator()(const TKey& theLeft, const TKey& theRight) const;

      bool
      operator()(const TGaussInfo& theLeft, const TGaussInfo& theRight) const;
    };

    //! Defines, which geometrical type the MED Gauss entity belongs to
    EGeometrieElement myGeom; 
    //! Let known what MED geometrical type the MED GAUSS entity belong to
    EGeometrieElement GetGeom() const { return myGeom;}

    //! Contains coordinates for the refereced nodes
    TNodeCoord myRefCoord; 

    //! Gives coordinates for the referenced node by its number
    TCCoordSlice GetRefCoordSlice(TInt theId) const;
    //! Gives coordinates for the referenced node by its number
    TCoordSlice GetRefCoordSlice(TInt theId);

    //! Contains coordinates for the Gauss points
    TNodeCoord myGaussCoord;

    //! Gives coordinates for the Gauss points by its number
    TCCoordSlice GetGaussCoordSlice(TInt theId) const;
    //! Gives coordinates for the Gauss points by its number
    TCoordSlice GetGaussCoordSlice(TInt theId);

    //! Contains wheights for the Gauss points
    TWeight myWeight;

    //! Gives number of the referenced nodes
    TInt GetNbRef() const { return GetNbRefCoord(GetGeom());}

    //! Gives dimension of the referenced nodes
    TInt GetDim() const { return GetDimGaussCoord(GetGeom());}

    //! Gives number of the Gauss Points
    TInt GetNbGauss() const { return (TInt)(myGaussCoord.size()/GetDim());}
  };


  //---------------------------------------------------------------
  typedef std::map<EGeometrieElement,PGaussInfo> TGeom2Gauss;
  typedef std::map<EGeometrieElement,TInt> TGeom2NbGauss;

  //! Define a base class which represents MED TimeStamp
  struct MEDWRAPPER_EXPORT TTimeStampInfo: 
    virtual TBase
  {
    PFieldInfo myFieldInfo; //!< A reference to corresponding MED Field
    //! Get a reference to corresponding MED Field
    const PFieldInfo& GetFieldInfo() const { return myFieldInfo;}

    //! Defines the MED Entity where the MED TimeStamp belongs to
    EEntiteMaillage myEntity;
    //! Let known to what MED Entity the MED TimeStamp belong to
    EEntiteMaillage GetEntity() const { return myEntity;}

    //! Keeps map of number of cells per geometric type where the MED TimeStamp belongs to
    TGeom2Size myGeom2Size;
    //! Get map of number of cells per geometric type where the MED TimeStamp belongs to
    const TGeom2Size& GetGeom2Size() const { return myGeom2Size;}

    TGeom2NbGauss myGeom2NbGauss; //!< Keeps number of the Gauss Points for the MED TimeStamp
    TInt GetNbGauss(EGeometrieElement theGeom) const; //!< Gives number of the Gauss Points for the MED TimeStamp

    TInt myNumDt; //!< Keeps number in time for the MED TimeStamp
    TInt GetNumDt() const { return myNumDt;} //!< Defines number in time for the MED TimeStamp

    TInt myNumOrd; //!< Keeps number for the MED TimeStamp
    TInt GetNumOrd() const { return myNumOrd;} //!< Defines number for the MED TimeStamp

    TFloat myDt; //!< Keeps time for the MED TimeStamp
    TFloat GetDt() const { return myDt;} //!< Defines time for the MED TimeStamp

    //! Keeps map of MED Gauss entityes per geometric type
    TGeom2Gauss myGeom2Gauss;
    //! Gets a map of MED Gauss entityes per geometric type
    const TGeom2Gauss& GetGeom2Gauss() const { return myGeom2Gauss;}

    TString myUnitDt; //!< Defines unit for the time for the MED TimeStamp
    //! Get unit of time for the MED TimeStamp
    virtual std::string GetUnitDt() const = 0;
    //! Set unit of time for the MED TimeStamp
    virtual void SetUnitDt(const std::string& theValue) = 0;
  };
  

  //---------------------------------------------------------------
  //! The class represents MED Profile entity
  struct MEDWRAPPER_EXPORT TProfileInfo: 
    virtual TNameInfo
  {
    typedef std::string TKey;
    typedef boost::tuple<TKey,TInt> TInfo;

    EModeProfil myMode; //!< Keeps mode for the MED Profile
    //! Let known what mode of MED Profile is used
    EModeProfil GetMode() const { return myMode;}
    //! Set mode for the MED Profile
    void SetMode(EModeProfil theMode) { myMode = theMode;}

    PElemNum myElemNum; //!< Keeps sequence of cell by its number which belong to the profile
    //! Get number of mesh elelemts by its order number
    TInt GetElemNum(TInt theId) const;
    //! Set number of mesh elelemts by its order number
    void SetElemNum(TInt theId, TInt theVal);

    //! Let known is the MED Profile defined
    bool IsPresent() const { return GetName() != "";}

    //! Let known size of the MED Profile
    TInt GetSize() const { return (TInt)myElemNum->size();}
  };


  //---------------------------------------------------------------
  //! The class is a helper one. It provide safe and flexible way to get access to values for a MED TimeStamp
  struct MEDWRAPPER_EXPORT TMeshValueBase:
    virtual TModeSwitchInfo 
  {
    TInt myNbElem;
    TInt myNbComp;
    TInt myNbGauss;
    TInt myStep;

    TMeshValueBase();

    //! Initialize the class
    void
    Allocate(TInt theNbElem,
             TInt theNbGauss,
             TInt theNbComp,
             EModeSwitch theMode = eFULL_INTERLACE);

    //! Returns size of the value container
    size_t
    GetSize() const;
    
    //! Returns MED interpretation of the value size
    size_t
    GetNbVal() const;
    
    //! Returns number of Gauss Points bounded with the value
    size_t
    GetNbGauss() const;
    
    //! Returns step inside of the data array
    size_t
    GetStep() const;
    
    //! Returns bare pointer on the internal value representation
    virtual
    unsigned char*
    GetValuePtr() = 0;
  };

  //---------------------------------------------------------------
  //! The class is a helper one. It provide safe and flexible way to get access to values for a MED TimeStamp
  template<class TValueType>
  struct TTMeshValue:
    virtual TMeshValueBase 
  {
    typedef TValueType TValue;
    typedef typename TValueType::value_type TElement;

    typedef TSlice<TElement> TValueSlice;
    typedef TCSlice<TElement> TCValueSlice;
    
    typedef TVector<TCValueSlice> TCValueSliceArr;
    typedef TVector<TValueSlice> TValueSliceArr;
    
    TValue myValue;

    //! Initialize the class
    void
    Allocate(TInt theNbElem,
             TInt theNbGauss,
             TInt theNbComp,
             EModeSwitch theMode = eFULL_INTERLACE)
    {
      TMeshValueBase::Allocate(theNbElem, theNbGauss, theNbComp, theMode);
      myValue.resize(theNbElem * this->GetStep());
    }

    //! Returns bare pointer on the internal value representation
    virtual
    unsigned char*
    GetValuePtr()
    {
      return (unsigned char*)&myValue[0];
    }

    //! Returns bare pointer on the internal value representation
    virtual
    TElement*
    GetPointer()
    {
      return &myValue[0];
    }

    //! Returns bare pointer on the internal value representation
    virtual
    const TElement*
    GetPointer() const
    {
      return &myValue[0];
    }

    //! Iteration through Gauss Points by their components
    TCValueSliceArr
    GetGaussValueSliceArr(TInt theElemId) const
    {
      TCValueSliceArr aValueSliceArr(myNbGauss);
      if(GetModeSwitch() == eFULL_INTERLACE){
        TInt anId = theElemId * myStep;
        for(TInt aGaussId = 0; aGaussId < myNbGauss; aGaussId++){
          aValueSliceArr[aGaussId] =
            TCValueSlice(myValue, std::slice(anId, myNbComp, 1));
          anId += myNbComp;
        }
      }
      else{
        for(TInt aGaussId = 0; aGaussId < myNbGauss; aGaussId++){
          aValueSliceArr[aGaussId] =
            TCValueSlice(myValue, std::slice(theElemId, myNbComp, myStep));
        }
      }
      return aValueSliceArr;
    }

    //! Iteration through Gauss Points by their components
    TValueSliceArr 
    GetGaussValueSliceArr(TInt theElemId)
    {
      TValueSliceArr aValueSliceArr(myNbGauss);
      if(GetModeSwitch() == eFULL_INTERLACE){
        TInt anId = theElemId*myStep;
        for(TInt aGaussId = 0; aGaussId < myNbGauss; aGaussId++){
          aValueSliceArr[aGaussId] =
            TValueSlice(myValue, std::slice(anId, myNbComp, 1));
          anId += myNbComp;
        }
      }
      else{
        for(TInt aGaussId = 0; aGaussId < myNbGauss; aGaussId++){
          aValueSliceArr[aGaussId] =
            TValueSlice(myValue, std::slice(theElemId, myNbComp, myStep));
        }
      }
      return aValueSliceArr;
    }

    //! Iteration through components by corresponding Gauss Points
    TCValueSliceArr
    GetCompValueSliceArr(TInt theElemId) const
    {
      TCValueSliceArr aValueSliceArr(myNbComp);
      if(GetModeSwitch() == eFULL_INTERLACE){
        TInt anId = theElemId*myStep;
        for(TInt aCompId = 0; aCompId < myNbComp; aCompId++){
          aValueSliceArr[aCompId] =
            TCValueSlice(myValue, std::slice(anId, myNbGauss, myNbComp));
          anId += 1;
        }
      }
      else{
        for(TInt aCompId = 0; aCompId < myNbComp; aCompId++){
          aValueSliceArr[aCompId] =
            TCValueSlice(myValue, std::slice(theElemId, myNbGauss, myStep));
        }
      }
      return aValueSliceArr;
    }

    //! Iteration through components by corresponding Gauss Points
    TValueSliceArr 
    GetCompValueSliceArr(TInt theElemId)
    {
      if(GetModeSwitch() == eFULL_INTERLACE){
        TValueSliceArr aValueSliceArr(myNbComp);
        TInt anId = theElemId*myStep;
        for(TInt aCompId = 0; aCompId < myNbComp; aCompId++){
          aValueSliceArr[aCompId] =
            TValueSlice(myValue, std::slice(anId, myNbGauss, myNbComp));
          anId += 1;
        }
        return aValueSliceArr;
      }
      else{
        TValueSliceArr aValueSliceArr(myNbGauss);
        for(TInt aGaussId = 0; aGaussId < myNbGauss; aGaussId++){
          aValueSliceArr[aGaussId] =
            TValueSlice(myValue,std::slice(theElemId, myNbComp, myStep));
        }
        return aValueSliceArr;
      }
    }
  };

  typedef TTMeshValue<TFloatVector> TFloatMeshValue;
  typedef TTMeshValue<TIntVector> TIntMeshValue;

  //---------------------------------------------------------------
  // Backward compatibility  declarations
  typedef TFloatVector TValue;
  typedef TSlice<TFloat> TValueSlice;
  typedef TCSlice<TFloat> TCValueSlice;
  
  typedef TVector<TCValueSlice> TCValueSliceArr;
  typedef TVector<TValueSlice> TValueSliceArr;
    
  typedef TFloatMeshValue TMeshValue;
  typedef std::map<EGeometrieElement,TMeshValue> TGeom2Value;

  //---------------------------------------------------------------
  typedef std::map<EGeometrieElement,PProfileInfo> TGeom2Profile;
  typedef std::set<EGeometrieElement> TGeom;

  //! The class is a base class for MED TimeStamp values holder
  struct MEDWRAPPER_EXPORT TTimeStampValueBase: 
    virtual TModeSwitchInfo 
  {
    //! A reference to corresponding MED TimeStamp
    PTimeStampInfo myTimeStampInfo;
    //!< Get a reference to corresponding MED TimeStamp
    const PTimeStampInfo& GetTimeStampInfo() const { return myTimeStampInfo;}

    //! Keeps set of MED EGeometrieElement which contains values for the timestamp
    TGeomSet myGeomSet;
    const TGeomSet& GetGeomSet() const { return myGeomSet;}

    //! Keeps map of MED Profiles per geometric type
    TGeom2Profile myGeom2Profile;
    //! Gets a map of MED Profiles per geometric type
    const TGeom2Profile& GetGeom2Profile() const { return myGeom2Profile;}

    //! Gets type of the champ
    virtual 
    ETypeChamp
    GetTypeChamp() const = 0;

    //! Allocates values for the given geometry
    virtual 
    void
    AllocateValue(EGeometrieElement theGeom,
                  TInt theNbElem,
                  TInt theNbGauss,
                  TInt theNbComp,
                  EModeSwitch theMode = eFULL_INTERLACE) = 0;
    
    virtual 
    size_t
    GetValueSize(EGeometrieElement theGeom) const = 0;
    
    virtual 
    size_t
    GetNbVal(EGeometrieElement theGeom) const = 0;
    
    virtual 
    size_t
    GetNbGauss(EGeometrieElement theGeom) const = 0;

    virtual 
    unsigned char*
    GetValuePtr(EGeometrieElement theGeom) = 0;
  };


  //---------------------------------------------------------------
  //! The class implements a container for MED TimeStamp values
  template<class TMeshValueType>
  struct TTimeStampValue: 
    virtual TTimeStampValueBase 
  {
    typedef TMeshValueType TTMeshValue;
    typedef SharedPtr<TMeshValueType> PTMeshValue;
    typedef typename TMeshValueType::TElement TElement;
    typedef std::map<EGeometrieElement, PTMeshValue> TTGeom2Value;

    ETypeChamp myTypeChamp; //<! Keeps type of the champ

    //! Gets type of the champ
    virtual 
    ETypeChamp
    GetTypeChamp() const
    {
      return myTypeChamp;
    }

    //! Keeps map of MED TimeStamp values per geometric type (const version)
    TTGeom2Value myGeom2Value;

    const TTGeom2Value& 
    GetGeom2Value() const
    {
      return myGeom2Value;
    }

    //! Gets MED TimeStamp values for the given geometric type (const version)
    const PTMeshValue& 
    GetMeshValuePtr(EGeometrieElement theGeom) const
    {
      typename TTGeom2Value::const_iterator anIter = myGeom2Value.find(theGeom);
      if(anIter == myGeom2Value.end())
        EXCEPTION(std::runtime_error,"TTimeStampValue::GetMeshValuePtr - myGeom2Value.find(theGeom) fails");
      return anIter->second;
    }

    //! Gets MED TimeStamp values for the given geometric type
    PTMeshValue& 
    GetMeshValuePtr(EGeometrieElement theGeom)
    {
      myGeomSet.insert(theGeom);
      if(myGeom2Value.find(theGeom) == myGeom2Value.end()){
        myGeom2Value[theGeom] = PTMeshValue(new TTMeshValue());
        return myGeom2Value[theGeom];
      }
      return myGeom2Value[theGeom];
    }

    //! Gets MED TimeStamp values for the given geometric type (const version)
    const TTMeshValue& 
    GetMeshValue(EGeometrieElement theGeom) const
    {
      return *(this->GetMeshValuePtr(theGeom));
    }

    //! Gets MED TimeStamp values for the given geometric type
    TTMeshValue& 
    GetMeshValue(EGeometrieElement theGeom)
    {
      return *(this->GetMeshValuePtr(theGeom));
    }
  };


  //---------------------------------------------------------------
  typedef TTimeStampValue<TFloatMeshValue> TFloatTimeStampValue;
  typedef SharedPtr<TFloatTimeStampValue> PFloatTimeStampValue;

  PFloatTimeStampValue MEDWRAPPER_EXPORT
  CastToFloatTimeStampValue(const PTimeStampValueBase& theTimeStampValue);

  typedef TTimeStampValue<TIntMeshValue> TIntTimeStampValue;
  typedef SharedPtr<TIntTimeStampValue> PIntTimeStampValue;
  
  PIntTimeStampValue MEDWRAPPER_EXPORT
  CastToIntTimeStampValue(const PTimeStampValueBase& theTimeStampValue);


  //---------------------------------------------------------------
  template<class TMeshValueTypeFrom, class TMeshValueTypeTo>
  void
  CopyTimeStampValue(SharedPtr<TTimeStampValue<TMeshValueTypeFrom> > theTimeStampValueFrom,
                     SharedPtr<TTimeStampValue<TMeshValueTypeTo> > theTimeStampValueTo)
  {
    typedef TTimeStampValue<TMeshValueTypeFrom> TimeStampValueTypeFrom;
    typedef TTimeStampValue<TMeshValueTypeTo> TimeStampValueTypeTo;
    typedef typename TMeshValueTypeTo::TElement TElementTo;

    typename TimeStampValueTypeFrom::TTGeom2Value& aGeom2Value = theTimeStampValueFrom->myGeom2Value;
    typename TimeStampValueTypeFrom::TTGeom2Value::const_iterator anIter = aGeom2Value.begin();
    for(; anIter != aGeom2Value.end(); anIter++){
      const EGeometrieElement& aGeom = anIter->first;
      const typename TimeStampValueTypeFrom::TTMeshValue& aMeshValue = *anIter->second;
      typename TimeStampValueTypeTo::TTMeshValue& aMeshValue2 = theTimeStampValueTo->GetMeshValue(aGeom);
      aMeshValue2.Allocate(aMeshValue.myNbElem, 
                           aMeshValue.myNbGauss, 
                           aMeshValue.myNbComp,
                           aMeshValue.myModeSwitch);
      const typename TimeStampValueTypeFrom::TTMeshValue::TValue& aValue = aMeshValue.myValue;
      typename TimeStampValueTypeTo::TTMeshValue::TValue& aValue2 = aMeshValue2.myValue;
      TInt aSize = aValue.size();
      for(TInt anId = 0; anId < aSize; anId++)
        aValue2[anId] = TElementTo(aValue[anId]);
    }
  }

  template<class TMeshValueType>
  void
  CopyTimeStampValue(SharedPtr<TTimeStampValue<TMeshValueType> > theTimeStampValueFrom,
                     SharedPtr<TTimeStampValue<TMeshValueType> > theTimeStampValueTo)
  {
    typedef TTimeStampValue<TMeshValueType> TimeStampValueType;
    typename TimeStampValueType::TTGeom2Value& aGeom2Value = theTimeStampValueFrom->myGeom2Value;
    typename TimeStampValueType::TTGeom2Value::const_iterator anIter = aGeom2Value.begin();
    for(; anIter != aGeom2Value.end(); anIter++){
      const EGeometrieElement& aGeom = anIter->first;
      const typename TimeStampValueType::TTMeshValue& aMeshValue = *anIter->second;
      typename TimeStampValueType::TTMeshValue& aMeshValue2 = theTimeStampValueTo->GetMeshValue(aGeom);
      aMeshValue2 = aMeshValue;
    }
  }

  //---------------------------------------------------------------
  inline
  void
  CopyTimeStampValueBase(const PTimeStampValueBase& theValueFrom, 
                         const PTimeStampValueBase& theValueTo)
  {
    if(theValueFrom->GetTypeChamp() == theValueTo->GetTypeChamp()){
      if(theValueFrom->GetTypeChamp() == eFLOAT64)
        CopyTimeStampValue<TFloatMeshValue>(theValueFrom, theValueTo);
      else if(theValueFrom->GetTypeChamp() == eINT)
        CopyTimeStampValue<TIntMeshValue>(theValueFrom, theValueTo);
    }else{
      if(theValueFrom->GetTypeChamp() == eFLOAT64 && theValueTo->GetTypeChamp() == eINT)
        CopyTimeStampValue<TFloatMeshValue, TIntMeshValue>(theValueFrom, theValueTo);
      else if(theValueFrom->GetTypeChamp() == eINT && theValueTo->GetTypeChamp() == eFLOAT64)
        CopyTimeStampValue<TIntMeshValue, TFloatMeshValue>(theValueFrom, theValueTo);
    }
  }


  //---------------------------------------------------------------
  // Backward compatibility  declarations
  typedef TFloatTimeStampValue TTimeStampVal;
  typedef PFloatTimeStampValue PTimeStampVal;

  //---------------------------------------------------------------
  typedef std::map<TInt,TFloatVector> TIndexes;
  typedef std::map<TInt,TString> TNames;
  
  //! Define a base class which represents MED Grille (structured mesh)
  struct MEDWRAPPER_EXPORT TGrilleInfo:
    virtual TModeSwitchInfo
  {

    PMeshInfo myMeshInfo;
    const PMeshInfo& GetMeshInfo() const { return myMeshInfo;} 

    TNodeCoord myCoord; //!< Contains all nodal coordinates, now used only for eGRILLE_STANDARD
    //! Gives coordinates for mesh nodes (const version)
    const TNodeCoord& GetNodeCoord() const;
    TNodeCoord& GetNodeCoord();
    //! Gives coordinates for mesh node by its number, array index from 0
    TNodeCoord GetCoord(TInt theId);
    //! Gives ids of nodes for mesh cell or sub-cell by its number, array index from 0
    TIntVector GetConn(TInt theId, const bool isSub=false);

    EGrilleType myGrilleType; //!< Defines grille type (eGRILLE_CARTESIENNE,eGRILLE_POLAIRE,eGRILLE_STANDARD)
    //!Gets grille type (const version)
    const EGrilleType& GetGrilleType() const;
    //!Gets grille type
    EGrilleType GetGrilleType();
    //!Sets grille type
    void SetGrilleType(EGrilleType theGrilleType);


    
    TString myCoordNames; //!< Contains names for the coordinate dimensions
    //! Get name of the coordinate dimension by its order number
    virtual std::string GetCoordName(TInt theId) const = 0 ;
    //! Set name of the coordinate dimension by its order number
    virtual void SetCoordName(TInt theId, const std::string& theValue) = 0;

    TString myCoordUnits; //!< Contains units for the coordinate dimensions
    //! Get name of unit for the coordinate dimension by its order number
    virtual std::string GetCoordUnit(TInt theId) const = 0;
    //! Set name of unit for the coordinate dimension by its order number
    virtual void SetCoordUnit(TInt theId, const std::string& theValue) = 0;


    //! Map of index of axes and Table of indexes for certain axe, now used for eGRILLE_CARTESIENNE and eGRILLE_POLAIRE
    TIndexes myIndixes;
    //!Gets a map of Tables (const version)
    const TIndexes& GetMapOfIndexes() const ;
    //!Gets a map of Tables
    TIndexes& GetMapOfIndexes();
    //!Gets a Table of indexes for certain axe(const version)
    const TFloatVector& GetIndexes(TInt theAxisNumber) const;
    //!Gets a Table of indexes for certain axe
    TFloatVector& GetIndexes(TInt theAxisNumber);
    //!Gets a number of indices per axe
    TInt GetNbIndexes(TInt theAxisNumber);
    
    TInt GetNbNodes();//! Return count of all points
    TInt GetNbCells();//! Return count of all cells
    TInt GetNbSubCells();//! Return count of all entities of <mesh dimension-1>
    EGeometrieElement GetGeom();//! Return geometry of cells (calculated from mesh dimension)
    EGeometrieElement GetSubGeom();//! Return geometry of subcells (calculated from mesh dimension)
    EEntiteMaillage GetEntity();//! Return entity (eMAILLE)
    EEntiteMaillage GetSubEntity();//! Return sub entity

    /*!
     *Vector of grille structure (Example: {3,4,5}, 3 nodes in X axe, 4 nodes in Y axe, ...)
     */
    TIntVector myGrilleStructure;
    //!Gets grille structure(const version)
    const TIntVector& GetGrilleStructure() const;
    //!Gets grille structure
    TIntVector GetGrilleStructure();
    //!Sets the grille structure of theAxis axe to theNb.
    void SetGrilleStructure(TInt theAxis,TInt theNb);
    
    /*!
     *Defines sequence MED Family indexes for corresponding mesh entities
     */
    TElemNum myFamNum; 
    //! Get number of a MED FAMILY by order number of the mesh element
    TInt GetFamNum(TInt theId) const;
    //! Set number of a MED FAMILY for the mesh element with the  order number
    void SetFamNum(TInt theId, TInt theVal);
    
    /*!
     *Defines sequence MED Family indexes for sub entities
     */
    TElemNum myFamSubNum; 
    //! Get number of a MED FAMILY by order number of sub element
    TInt GetFamSubNum(TInt theId) const;
    //! Set number of a MED FAMILY for theId-th sub element
    void SetFamSubNum(TInt theId, TInt theVal);
    
    /*!
     *Defines sequence MED Family indexes for corresponding mesh nodes
     */
    TElemNum myFamNumNode;
    //! Get number of a MED FAMILY by order number of the mesh node
    TInt GetFamNumNode(TInt theId) const;
    //! Set number of a MED FAMILY for the mesh node with the  order number
    void SetFamNumNode(TInt theId, TInt theVal);

  };


}

#endif
