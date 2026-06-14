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

//  SMESH SMDS : implementation of Salome mesh data structure
//  File   : SMDS_MeshGroup.hxx
//  Module : SMESH
//
#ifndef _SMDS_MeshGroup_HeaderFile
#define _SMDS_MeshGroup_HeaderFile

#include "SMESH_SMDS.hxx"

#include "SMDS_Mesh.hxx"
#include <set>

class SMDS_EXPORT SMDS_MeshGroup:public SMDS_MeshObject
{
  public:
        SMDS_MeshGroup(const SMDS_Mesh * theMesh,
                       const SMDSAbs_ElementType theType = SMDSAbs_All);
        const SMDS_MeshGroup * AddSubGroup
                      (const SMDSAbs_ElementType theType = SMDSAbs_All);
        virtual bool RemoveSubGroup(const SMDS_MeshGroup* theGroup);
        virtual bool RemoveFromParent();

        const SMDS_Mesh* GetMesh() const { return myMesh; }

        void SetType (const SMDSAbs_ElementType theType);
        void Clear();
        bool Add(const SMDS_MeshElement * theElem);
        bool Remove(const SMDS_MeshElement * theElem);
        bool IsEmpty() const { return myElements.empty(); }
        int Extent() const { return myElements.size(); }
        int Tic() const { return myTic; }

        int SubGroupsNb() const { return myChildren.size(); }

        SMDSAbs_ElementType GetType() const { return myType; }

        bool Contains(const SMDS_MeshElement * theElem) const;

        void InitIterator() const
        { const_cast<TIterator&>(myIterator) = myElements.begin(); }

        bool More() const { return myIterator != myElements.end(); }

        const SMDS_MeshElement* Next() const
        { return *(const_cast<TIterator&>(myIterator))++; }

        void InitSubGroupsIterator() const
        { const_cast<TGroupIterator&>(myGroupIterator) = myChildren.begin(); }

        bool MoreSubGroups() const { return myGroupIterator != myChildren.end(); }

        const SMDS_MeshGroup* NextSubGroup() const
        { return *(const_cast<TGroupIterator&>(myGroupIterator))++; }

  private:
        SMDS_MeshGroup(SMDS_MeshGroup* theParent,
                       const SMDSAbs_ElementType theType = SMDSAbs_All);

        typedef std::set<const SMDS_MeshElement *>::const_iterator TIterator;
        typedef std::list<const SMDS_MeshGroup *>::const_iterator TGroupIterator;

        const SMDS_Mesh *                       myMesh;
        SMDSAbs_ElementType                     myType;
        std::set<const SMDS_MeshElement*>       myElements; /* - not sorted by ID because it */
        SMDS_MeshGroup *                        myParent;   /* can contain deleted elements */
        std::list<const SMDS_MeshGroup*>        myChildren;
        TIterator                               myIterator;
        TGroupIterator                          myGroupIterator;
        int                                     myTic; // to track changes
};
#endif
